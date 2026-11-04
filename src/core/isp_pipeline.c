/* isp_pipeline.c - ISP 流水线实现（各阶段公式与 0x404d20/0x405b60/0x402bf0/
 * 0x404590/0x40b230/0x4050f0 反编译结果对应，见 docs/模块流程文档.md §2.1）*/
#include "isp_pipeline.h"
#include "bayer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Q5 定点：原 0x405b60 公式 out=(in*gain)/32+offset，除数 32 */
#define CISGAIN_SHIFT 5
/* P2 ASCII 转储的 maxval 随数据量程（原程序 fprintf "P2 %d %d 1023"）*/

/* ---------- 阶段调试转储（复刻 0x40d110）---------- */
static void dump_plane(const ItConfig *cfg, int enable, const char *base,
                       const uint16_t *plane, int w, int h, int full_scale)
{
    if (!enable || !cfg->dump_dir[0])
        return;
    char path[320];
    snprintf(path, sizeof(path), "%s/%s.%s", cfg->dump_dir, base,
             cfg->dump_ppm ? "ppm" : "raw");
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    if (cfg->dump_ppm) {
        fprintf(f, "P2\n%d %d\n%u\n", w, h, full_scale);
        for (long i = 0; i < (long)w * h; i++) {
            unsigned v = plane[i] > (unsigned)full_scale
                             ? (unsigned)full_scale : plane[i];
            fprintf(f, "%u%c", v, ((i + 1) % w == 0) ? '\n' : ' ');
        }
    } else {
        fwrite(plane, sizeof(uint16_t), (size_t)w * h, f);
    }
    fclose(f);
}

static void dump_rgb(const ItConfig *cfg, const char *base, const ItRgb *rgb)
{
    if (!cfg->dump_dir[0])
        return;
    char path[320];
    snprintf(path, sizeof(path), "%s/%s.ppm", cfg->dump_dir, base);
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    fprintf(f, "P3\n%d %d\n255\n", rgb->width, rgb->height);
    for (long i = 0; i < (long)rgb->width * rgb->height * 3; i++)
        fprintf(f, "%u%c", rgb->data[i],
                ((i / 3 + 1) % rgb->width == 0 && (i % 3) == 2) ? '\n' : ' ');
    fclose(f);
}

static void dump_ybmp(const ItConfig *cfg, const char *base, const ItImage *y)
{
    if (!cfg->dump_dir[0])
        return;
    char path[320];
    snprintf(path, sizeof(path), "%s/%s.ppm", cfg->dump_dir, base);
    FILE *f = fopen(path, "wb");
    if (!f)
        return;
    fprintf(f, "P2\n%d %d\n255\n", y->width, y->height);
    for (long i = 0; i < (long)y->width * y->height; i++)
        fprintf(f, "%u%c", y->data[i] > 255 ? 255 : y->data[i],
                ((i + 1) % y->width == 0) ? '\n' : ' ');
    fclose(f);
}

/* ---------- 限幅（原 FUN_00401840(value,hi,lo)）---------- */
static uint16_t clamp_u16(int v, int hi, int lo)
{
    if (v > hi) return (uint16_t)hi;
    if (v < lo) return (uint16_t)lo;
    return (uint16_t)v;
}

/* ---------- 级 1：数字钳位 + CIS 增益（0x405b60）----------
 * 原公式：out = clamp((in * gain) >> 5 + offset, 0, 1023)
 * dummy 区（< STRT_H/STRT_V 的行列）与有效区使用不同 gain/offset 对；
 * 复刻按行/列阈值分区，通道由 Bayer 位置决定。 */
static void stage_dclamp(const ItImage *in, const ItConfig *cfg, ItImage *out)
{
    int w = in->width, h = in->height;
    int fs = (in->full_scale > 0) ? in->full_scale : 1023;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            long i  = (long)y * w + x;
            int  px = in->data[i];

            /* 通道判定（与 bayer.c color_at 一致的 pattern 映射）*/
            int row = (y & 1), col = (x & 1), c;
            switch (cfg->pat_sel) {
            case IT_PAT_RGGB: c = row ? (col ? 2 : 1) : (col ? 1 : 0); break;
            case IT_PAT_GRBG: c = row ? (col ? 1 : 2) : (col ? 0 : 1); break;
            case IT_PAT_GBRG: c = row ? (col ? 0 : 1) : (col ? 1 : 2); break;
            case IT_PAT_BGGR: c = row ? (col ? 1 : 0) : (col ? 2 : 1); break;
            default:          c = 1; break;
            }

            int gain, off;
            if (y < cfg->dclp_strt_v || x < cfg->dclp_strt_h) {
                /* dummy 区：原程序对 dummy 行/列用独立参数对（+0x1064 区）*/
                gain = 1 << CISGAIN_SHIFT;
                off  = cfg->poffset;
            } else {
                switch (c) {
                case 0: gain = cfg->cisgain_r;  off = cfg->poffset_r;  break;
                case 2: gain = cfg->cisgain_b;  off = cfg->poffset_b;  break;
                default:
                    /* 绿通道区分 Gr/Gb：原 CISGAIN_GR/GB 按行奇偶 */
                    gain = ((y & 1) == 0) ? cfg->cisgain_gr : cfg->cisgain_gb;
                    off  = ((y & 1) == 0) ? cfg->poffset_gr : cfg->poffset_gb;
                    break;
                }
            }
            long v = ((long)px * gain >> CISGAIN_SHIFT) + off;  /* long 防 INI 大增益溢出 */
            /* PreprocessTh 按 10bit 语义定义，随量程等比换算 */
            int thCut = (int)((long)cfg->preprocess_th * fs / 1023);
            if (v < thCut)
                v = 0;                    /* PreprocessTh：黑电平以下切除 */
            out->data[i] = clamp_u16(v, fs, 0);
        }
    }
}

/* ---------- 级 2：白平衡 1 通道（0x4060e0/0x406320）----------
 * out = clamp((in * WB_gain) >> 8 - DS, 0, 1023)，按通道独立。
 * DS（Dark Shading，CRDS/CGDS/CBDS）先行扣除再乘增益。 */
static void stage_wb(const ItImage *in, const ItConfig *cfg, ItImage *out)
{
    int w = in->width, h = in->height;
    int fs = (in->full_scale > 0) ? in->full_scale : 1023;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            long i = (long)y * w + x;
            int row = (y & 1), col = (x & 1), c;
            switch (cfg->pat_sel) {
            case IT_PAT_RGGB: c = row ? (col ? 2 : 1) : (col ? 1 : 0); break;
            case IT_PAT_GRBG: c = row ? (col ? 1 : 2) : (col ? 0 : 1); break;
            case IT_PAT_GBRG: c = row ? (col ? 0 : 1) : (col ? 1 : 2); break;
            case IT_PAT_BGGR: c = row ? (col ? 1 : 0) : (col ? 2 : 1); break;
            default:          c = 1; break;
            }
            int gain = (c == 0) ? cfg->wb_gain_r :
                       (c == 2) ? cfg->wb_gain_b : cfg->wb_gain_g;
            int ds   = (c == 0) ? cfg->wb_ds_r :
                       (c == 2) ? cfg->wb_ds_b   : cfg->wb_ds_g;
            long v = ((long)(in->data[i] - ds) * gain) >> 8;   /* long 防 INI 大增益溢出 */
            out->data[i] = clamp_u16((int)v, fs, 0);
        }
    }
}

/* ---------- 级 3：Shading 校正（0x402bf0 增益 / 0x404590 应用）----------
 * 径向模式：gain = 1 + k * ((x-xc)^2 + (y-yc)^2) / r^2，k 由 SH_VAL 决定；
 * 分窗模式：3x3 窗增益表 GAIN_H1..9 / GAIN_V1..9 双线性合成。
 * 按通道独立（SH_VAL_R/G/B 数组）。输出 Q8 定点除法。 */
static void stage_shading(const ItImage *in, const ItConfig *cfg, ItImage *out)
{
    int w = in->width, h = in->height;
    int fs = (in->full_scale > 0) ? in->full_scale : 1023;
    double cx = cfg->shade_xc ? cfg->shade_xc : (w - 1) / 2.0;
    double cy = cfg->shade_yc ? cfg->shade_yc : (h - 1) / 2.0;
    double rmax2 = cx * cx + cy * cy;
    if (rmax2 <= 0.0) rmax2 = 1.0;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            long i = (long)y * w + x;
            int row = (y & 1), col = (x & 1), c;
            switch (cfg->pat_sel) {
            case IT_PAT_RGGB: c = row ? (col ? 2 : 1) : (col ? 1 : 0); break;
            case IT_PAT_GRBG: c = row ? (col ? 1 : 2) : (col ? 0 : 1); break;
            case IT_PAT_GBRG: c = row ? (col ? 0 : 1) : (col ? 1 : 2); break;
            default:          c = row ? (col ? 1 : 0) : (col ? 2 : 1); break;
            }
            const int *sh = (c == 0) ? cfg->sh_val_r :
                            (c == 2) ? cfg->sh_val_b : cfg->sh_val_g;

            double g;
            if (cfg->shade_radial) {
                double dx = x - cx, dy = y - cy;
                double t  = (dx * dx + dy * dy) / rmax2;
                g = 1.0 + (sh[1] - sh[0]) * t;      /* 中心 sh[0] → 角 sh[1] */
            } else {
                /* 3x3 分窗：把画面划成 3x3，取对应窗增益 */
                int gx = x * 3 / w, gy = y * 3 / h;
                if (gx > 2) gx = 2;
                if (gy > 2) gy = 2;
                g = 1.0 + (cfg->gain_v[gy] + cfg->gain_h[gx]) / 256.0;
            }
            int v = (int)(in->data[i] * g + 0.5);
            out->data[i] = clamp_u16(v, fs, 0);
        }
    }
}

/* ---------- 级 4：BPR 坏点校正（0x40b230）----------
 * 半径级联确认（对应原 BPR_TH1/TH3/TH5/TH7 的命名语义）：
 * 半径 1/3/5/7 的方形窗口各配一个阈值——像素与 3x3 均值偏差超过
 * BPR_TH1 才成为候选；候选需继续通过 7x7>BPR_TH3、11x11>BPR_TH5、
 * 15x15>BPR_TH7 的逐级确认，全部通过才以最大窗口均值替换
 * （逐级确认抑制误检）。窗口越界时收缩到图像内取均值。 */
#define BPR_LEVELS 4
static const int kBprRadius[BPR_LEVELS] = { 1, 3, 5, 7 };

static long bpr_window_mean(const ItImage *in, int x, int y, int radius,
                            long *count_out)
{
    int w = in->width, h = in->height;
    long sum = 0, n = 0;
    int y0 = y - radius, y1 = y + radius;
    int x0 = x - radius, x1 = x + radius;
    if (y0 < 0) y0 = 0;
    if (x0 < 0) x0 = 0;
    if (y1 >= h) y1 = h - 1;
    if (x1 >= w) x1 = w - 1;
    for (int yy = y0; yy <= y1; yy++)
        for (int xx = x0; xx <= x1; xx++) {
            if (xx == x && yy == y)
                continue;
            sum += in->data[(long)yy * w + xx];
            n++;
        }
    *count_out = n;
    return n ? sum / n : 0;
}

static void stage_bpr(const ItImage *in, const ItConfig *cfg, ItImage *out)
{
    int w = in->width, h = in->height;
    int fs = (in->full_scale > 0) ? in->full_scale : 1023;
    /* 阈值按 10bit 语义定义，随量程等比换算 */
    const int th[BPR_LEVELS] = {
        cfg->bpr_th1 * fs / 1023, cfg->bpr_th3 * fs / 1023,
        cfg->bpr_th5 * fs / 1023, cfg->bpr_th7 * fs / 1023 };
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            long i = (long)y * w + x;
            int own = in->data[i];
            long n = 0;
            long mean = bpr_window_mean(in, x, y, kBprRadius[0], &n);
            int diff = own - mean;
            if (diff < 0) diff = -diff;
            if (th[0] == 0 || diff <= th[0] || n == 0) {
                out->data[i] = (uint16_t)own;   /* 非候选，直通 */
                continue;
            }
            /* 逐级确认；失败一级即视为有效像素 */
            int confirmed = 1;
            long largest = mean;
            for (int lv = 1; lv < BPR_LEVELS; lv++) {
                mean = bpr_window_mean(in, x, y, kBprRadius[lv], &n);
                if (n == 0)
                    break;
                largest = mean;
                diff = own - mean;
                if (diff < 0) diff = -diff;
                if (th[lv] == 0 || diff <= th[lv]) {
                    confirmed = 0;
                    break;
                }
            }
            out->data[i] = confirmed ? (uint16_t)largest : (uint16_t)own;
        }
    }
}
#undef BPR_LEVELS

/* ---------- 级 5：GRGB 绿不平衡校正（GRGB_ON 参数组）----------
 * 行内 Gr/Gb 均值差 d 超过 GTHR01 时对 Gb 校正到 Gr 水平
 * （GRGB_GRB 选择方向）。GRGB_AVG=1 时对该行全部 Gb 应用校正；
 * =0 时仅校正局部偏差同样超过 GTHR01 的 Gb（局部准则）。 */
static int is_gr_pixel(const ItConfig *cfg, int x, int y)
{
    int row = (y & 1), col = (x & 1);
    switch (cfg->pat_sel) {
    case IT_PAT_RGGB: return !row &&  col;
    case IT_PAT_GRBG: return !row && !col;
    case IT_PAT_GBRG: return  row && !col;
    default:          return  row &&  col;
    }
}

static int is_gb_pixel(const ItConfig *cfg, int x, int y)
{
    int row = (y & 1), col = (x & 1);
    switch (cfg->pat_sel) {
    case IT_PAT_RGGB: return  row && !col;
    case IT_PAT_GRBG: return  row &&  col;
    case IT_PAT_GBRG: return !row &&  col;
    default:          return !row && !col;
    }
}

static void stage_grgb(const ItImage *in, const ItConfig *cfg, ItImage *out)
{
    int w = in->width, h = in->height;
    int fs = (in->full_scale > 0) ? in->full_scale : 1023;
    int gth = cfg->gthr01 * fs / 1023;      /* 阈值随量程换算 */
    memcpy(out->data, in->data, (size_t)w * h * sizeof(uint16_t));
    for (int y = 0; y < h; y++) {
        long sum_gr = 0, sum_gb = 0;
        int  n_gr = 0, n_gb = 0;
        for (int x = 0; x < w; x++) {
            if (is_gr_pixel(cfg, x, y)) { sum_gr += in->data[(long)y * w + x]; n_gr++; }
            if (is_gb_pixel(cfg, x, y)) { sum_gb += in->data[(long)y * w + x]; n_gb++; }
        }
        if (!n_gr || !n_gb)
            continue;
        int d = (int)(sum_gr / n_gr) - (int)(sum_gb / n_gb);
        if (d > gth || d < -gth) {
            int rowGbMean = (int)(sum_gb / n_gb);
            for (int x = 0; x < w; x++) {
                if (!is_gb_pixel(cfg, x, y))
                    continue;
                long i = (long)y * w + x;
                int own = in->data[i];
                if (!cfg->grgb_avg) {
                    /* 局部准则：仅校正偏离行 Gb 均值超过 GTHR01 的像素 */
                    int dev = own - rowGbMean;
                    if (dev < 0) dev = -dev;
                    if (dev <= gth)
                        continue;
                }
                int v = own + (cfg->grgb_grb ? d : -d);
                out->data[i] = clamp_u16(v, fs, 0);
            }
        }
    }
}

/* ---------- 级 6-9（RGB 域）---------- */
static void stage_gain2nd(ItRgb *rgb, const ItConfig *cfg)
{
    long n = (long)rgb->width * rgb->height;
    for (long i = 0; i < n; i++) {
        uint8_t *p = &rgb->data[i * 3];
        for (int c = 0; c < 3; c++) {
            int gain = (c == 0) ? cfg->gain_r_2nd :
                       (c == 1) ? cfg->gain_g_2nd : cfg->gain_b_2nd;
            int off  = (c == 0) ? cfg->offset_r_2nd :
                       (c == 1) ? cfg->offset_g_2nd : cfg->offset_b_2nd;
            int v = (p[c] * gain >> 8) + off;
            p[c] = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
        }
    }
}

static void stage_matrix(ItRgb *rgb, const ItConfig *cfg)
{
    long n = (long)rgb->width * rgb->height;
    for (long i = 0; i < n; i++) {
        uint8_t *p = &rgb->data[i * 3];
        int r = p[0], g = p[1], b = p[2];
        int o[3];
        o[0] = (cfg->rgm[0][0] * r + cfg->rgm[0][1] * g + cfg->rgm[0][2] * b) >> 8;
        o[1] = (cfg->rgm[1][0] * r + cfg->rgm[1][1] * g + cfg->rgm[1][2] * b) >> 8;
        o[2] = (cfg->rgm[2][0] * r + cfg->rgm[2][1] * g + cfg->rgm[2][2] * b) >> 8;
        for (int c = 0; c < 3; c++)
            p[c] = (uint8_t)(o[c] < 0 ? 0 : (o[c] > 255 ? 255 : o[c]));
    }
}

static void stage_gamma(ItRgb *rgb, const ItConfig *cfg)
{
    if (cfg->gam_sel == 0 || cfg->gamma_value <= 0.0)
        return;
    uint8_t lut[256];
    for (int i = 0; i < 256; i++) {
        double v = 255.0 * pow(i / 255.0, 1.0 / cfg->gamma_value);
        lut[i] = (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v + 0.5));
    }
    long n = (long)rgb->width * rgb->height;
    for (long i = 0; i < n; i++) {
        rgb->data[i * 3 + 0] = lut[rgb->data[i * 3 + 0]];
        rgb->data[i * 3 + 1] = lut[rgb->data[i * 3 + 1]];
        rgb->data[i * 3 + 2] = lut[rgb->data[i * 3 + 2]];
    }
}

/* ---------- 级 10：Y 生成（DIALOG 184 系数）---------- */
ItStatus it_y_from_rgb(const ItRgb *in, const ItConfig *cfg, ItImage *out)
{
    if (!in || !cfg || !out)
        return IT_E_PARAM;
    ItStatus st = it_image_alloc(out, in->width, in->height);
    if (st != IT_OK)
        return st;
    long n = (long)in->width * in->height;
    for (long i = 0; i < n; i++) {
        const uint8_t *p = &in->data[i * 3];
        int y = (cfg->ry_coef * p[0] + cfg->gy_coef * p[1] +
                 cfg->by_coef * p[2]) >> 8;
        out->data[i] = (uint16_t)(y < 0 ? 0 : (y > 255 ? 255 : y));
    }
    return IT_OK;
}

/* ---------- 级 6a：RGB 前置滤波（RGB_FILTER_ON 参数组）----------
 * 对应原程序 rgb_prefilter 阶段（在 demosaic 之后、二级增益之前）。
 * RGB_GMEDIAN_ON：先对 G 通道做 3x3 中值（去绿域椒盐噪声）。
 * RGB_FILTER_ON：3x3 边缘保持平滑——邻域权重
 *   w = exp(-diff²/(2σ²)) · (SIG02_MULT/256)（空间衰减），
 *   σ = SIG01_CONST + (SIG01_MULT·本像素值)/256，下限 1。
 * 全部 Sigma 参数按原 ISP_P1 命名消费。 */
static void stage_gmedian(ItRgb *rgb)
{
    int w = rgb->width, h = rgb->height;
    uint8_t *gbuf = (uint8_t *)malloc((size_t)w * h);
    if (!gbuf)
        return;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            uint8_t nb[9];
            int n = 0;
            for (int dy = -1; dy <= 1; dy++) {
                int yy = y + dy;
                if (yy < 0 || yy >= h) continue;
                for (int dx = -1; dx <= 1; dx++) {
                    int xx = x + dx;
                    if (xx < 0 || xx >= w) continue;
                    nb[n++] = rgb->data[((long)yy * w + xx) * 3 + 1];
                }
            }
            for (int i = 1; i < n; i++) {           /* 插入排序取中值 */
                uint8_t key = nb[i];
                int j = i - 1;
                while (j >= 0 && nb[j] > key) { nb[j + 1] = nb[j]; j--; }
                nb[j + 1] = key;
            }
            gbuf[(long)y * w + x] = nb[n / 2];
        }
    for (long i = 0; i < (long)w * h; i++)
        rgb->data[i * 3 + 1] = gbuf[i];
    free(gbuf);
}

static void stage_prefilter(ItRgb *rgb, const ItConfig *cfg)
{
    int w = rgb->width, h = rgb->height;
    uint8_t *out = (uint8_t *)malloc((size_t)w * h * 3);
    if (!out)
        return;
    memcpy(out, rgb->data, (size_t)w * h * 3);
    double spatial = cfg->sig02_mult / 256.0;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            uint8_t *p = &rgb->data[((long)y * w + x) * 3];
            double sumw[3] = { 1.0, 1.0, 1.0 }, sumv[3];
            for (int c = 0; c < 3; c++) sumv[c] = p[c];
            for (int dy = -1; dy <= 1; dy++) {
                int yy = y + dy;
                if (yy < 0 || yy >= h) continue;
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int xx = x + dx;
                    if (xx < 0 || xx >= w) continue;
                    uint8_t *q = &rgb->data[((long)yy * w + xx) * 3];
                    for (int c = 0; c < 3; c++) {
                        int sigma = cfg->sig01_const +
                                    ((cfg->sig01_mult * p[c]) >> 8);
                        if (sigma < 1) sigma = 1;
                        int diff = q[c] - p[c];
                        double wgt = exp(-(double)diff * diff /
                                         (2.0 * sigma * sigma)) * spatial;
                        sumw[c] += wgt;
                        sumv[c] += wgt * q[c];
                    }
                }
            }
            for (int c = 0; c < 3; c++) {
                int v = (int)(sumv[c] / sumw[c] + 0.5);
                out[((long)y * w + x) * 3 + c] =
                    (uint8_t)(v < 0 ? 0 : (v > 255 ? 255 : v));
            }
        }
    }
    memcpy(rgb->data, out, (size_t)w * h * 3);
    free(out);
}

/* ---------- 预览（原始强度平面 → 灰度，与原工具 raw 视图一致）---------- */
ItStatus it_isp_preview(const ItImage *bayer_in, const ItConfig *cfg,
                        ItRgb *out)
{
    if (!bayer_in || !bayer_in->data || !cfg || !out)
        return IT_E_PARAM;
    ItStatus st = it_rgb_alloc(out, bayer_in->width, bayer_in->height);
    if (st != IT_OK)
        return st;
    unsigned fs = (bayer_in->full_scale > 0)
                      ? (unsigned)bayer_in->full_scale : 1023u;
    long n = (long)out->width * out->height;
    for (long i = 0; i < n; i++) {
        unsigned v = bayer_in->data[i];
        if (v > fs) v = fs;
        unsigned g = (v * 255u + fs / 2) / fs;    /* 强度 → 8bit 灰阶 */
        out->data[i * 3 + 0] = (uint8_t)g;        /* R=G=B：纯灰度 */
        out->data[i * 3 + 1] = (uint8_t)g;
        out->data[i * 3 + 2] = (uint8_t)g;
    }
    return IT_OK;
}

/* ---------- 主驱动（复刻 0x404d20 的级联结构）---------- */
ItStatus it_isp_run(const ItImage *bayer_in, const ItConfig *cfg,
                    int dump, ItRgb *out)
{
    if (!bayer_in || !cfg || !out)
        return IT_E_PARAM;
    int w = bayer_in->width, h = bayer_in->height;

    ItImage cur, next;
    ItStatus st = it_image_alloc(&cur, w, h);
    if (st != IT_OK) return st;
    st = it_image_alloc(&next, w, h);
    if (st != IT_OK) { it_image_free(&cur); return st; }
    int fs = (bayer_in->full_scale > 0) ? bayer_in->full_scale : 1023;
    cur.full_scale  = fs;           /* 量程随数据在中间缓冲间传递 */
    next.full_scale = fs;

    /* 1. 数字钳位 + CIS 增益 */
    if (cfg->dclp_on) {
        stage_dclamp(bayer_in, cfg, &cur);
        dump_plane(cfg, dump && cfg->dump_dclamp, "00_d_clamp_out",
                   cur.data, w, h, fs);
    } else {
        memcpy(cur.data, bayer_in->data, (size_t)w * h * sizeof(uint16_t));
    }

    /* 2. 白平衡 1 通道 */
    stage_wb(&cur, cfg, &next);
    dump_plane(cfg, dump && cfg->dump_wb, "01_wb_1ch_out",
               next.data, w, h, fs);
    { ItImage t = cur; cur = next; next = t; }

    /* 3. Shading */
    if (cfg->shade_on) {
        stage_shading(&cur, cfg, &next);
        dump_plane(cfg, dump && cfg->dump_shading, "02_shade_1ch_out",
                   next.data, w, h, fs);
        { ItImage t = cur; cur = next; next = t; }
    }

    /* 4. BPR */
    if (cfg->bpr_on) {
        stage_bpr(&cur, cfg, &next);
        dump_plane(cfg, dump && cfg->dump_bpr, "03_bpr_out",
                   next.data, w, h, fs);
        { ItImage t = cur; cur = next; next = t; }
    }

    /* 5. GRGB/NR */
    if (cfg->grgb_on) {
        stage_grgb(&cur, cfg, &next);
        dump_plane(cfg, dump && cfg->dump_grgb, "04_nr_or_grgb_out",
                   next.data, w, h, fs);
        { ItImage t = cur; cur = next; next = t; }
    }

    /* 6-9. demosaic 后 RGB 域 */
    st = it_bayer_demosaic(&cur, cfg->pat_sel, IT_DEMOSAIC_BILINEAR, out);
    if (st != IT_OK) {
        it_image_free(&cur);
        it_image_free(&next);
        return st;
    }

    /* RGB 前置滤波（原 rgb_prefilter 阶段）*/
    if (cfg->rgb_gmedian_on)
        stage_gmedian(out);
    if (cfg->rgb_filter_on) {
        stage_prefilter(out, cfg);
        dump_rgb(cfg, "rgb_prefilter", out);
    }

    stage_gain2nd(out, cfg);
    dump_rgb(cfg, "06_rgb_gain_2nd", out);

    if (cfg->rgm[0][0] != 1 || cfg->rgm[0][1] != 0 || cfg->rgm[0][2] != 0 ||
        cfg->rgm[1][0] != 0 || cfg->rgm[1][1] != 1 || cfg->rgm[1][2] != 0 ||
        cfg->rgm[2][0] != 0 || cfg->rgm[2][1] != 0 || cfg->rgm[2][2] != 1) {
        stage_matrix(out, cfg);
        dump_rgb(cfg, "05_rgb_mat", out);
    }

    stage_gamma(out, cfg);
    dump_rgb(cfg, "10_gamma", out);

    /* 10. Y 生成（可选输出 11_rgb2y / 12_shade_y 对应物）*/
    if (cfg->ry_coef || cfg->gy_coef || cfg->by_coef) {
        ItImage y;
        if (it_y_from_rgb(out, cfg, &y) == IT_OK) {
            dump_ybmp(cfg, "11_rgb2y", &y);
            it_image_free(&y);
        }
    }

    it_image_free(&cur);
    it_image_free(&next);
    return IT_OK;
}
