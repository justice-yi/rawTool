/* smoke_test.c - 核心库端到端冒烟测试（非 GUI）
 * 生成合成 Bayer 斜坡图 → RAW 回路 → ISP 流水线 → 校验统计与保存。
 * 注意：所有调用都用 expect() 显式检查返回值，在 Release（-DNDEBUG）
 * 下依然生效 —— 不得把带副作用的调用放进 assert。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/itypes.h"
#include "core/raw_loader.h"
#include "core/bayer.h"
#include "core/isp_pipeline.h"
#include "core/analysis.h"
#include "core/it_persist.h"
#include "core/imgio.h"

static void expect(ItStatus st, const char *what)
{
    if (st != IT_OK) {
        fprintf(stderr, "FAIL: %s -> %s\n", what, it_status_str(st));
        exit(1);
    }
}

static void require(int cond, const char *what)
{
    if (!cond) {
        fprintf(stderr, "FAIL: %s\n", what);
        exit(1);
    }
}

/* BMP 头部字节级校验：文件长度、'BM'、offbits=54 与文件大小字段 */
static void verify_bmp_header(const char *path, long expect_size)
{
    FILE *f = fopen(path, "rb");
    require(f != NULL, "bmp reopen");
    unsigned char h[54];
    require(fread(h, 1, 54, f) == 54, "bmp header read");
    fclose(f);
    require(h[0] == 'B' && h[1] == 'M', "bmp magic");
    unsigned long size = h[2] | (h[3] << 8) | (h[4] << 16) | ((unsigned long)h[5] << 24);
    unsigned long offbits = h[10] | (h[11] << 8) | (h[12] << 16) | ((unsigned long)h[13] << 24);
    require(size == (unsigned long)expect_size, "bmp size field");
    require(offbits == 54, "bmp offbits field");
}

/* CSV 内容抽查：包含表头行与 Mean 行 */
static void verify_csv(const char *path)
{
    FILE *f = fopen(path, "r");
    require(f != NULL, "csv reopen");
    char line[256];
    int have_header = 0, have_mean = 0;
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Stat,Y,R,G,B", 12) == 0) have_header = 1;
        if (strncmp(line, "Mean,", 5) == 0)         have_mean = 1;
    }
    fclose(f);
    require(have_header, "csv header row");
    require(have_mean, "csv mean row");
}

int main(void)
{
    ItConfig cfg;
    it_config_default(&cfg);

    /* 1. 合成 64x48 RGGB Bayer 斜坡 */
    const int W = 64, H = 48;
    ItImage bayer;
    expect(it_image_alloc(&bayer, W, H), "alloc bayer");
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
            int row = y & 1, col = x & 1, c;
            switch (cfg.pat_sel) {
            case IT_PAT_RGGB: c = row ? (col ? 2 : 1) : (col ? 1 : 0); break;
            case IT_PAT_GRBG: c = row ? (col ? 1 : 2) : (col ? 0 : 1); break;
            case IT_PAT_GBRG: c = row ? (col ? 0 : 1) : (col ? 1 : 2); break;
            default:          c = row ? (col ? 1 : 0) : (col ? 2 : 1); break;
            }
            bayer.data[(long)y * W + x] =
                (uint16_t)((x * 8 + y * 4 + c * 64) % 1023);
        }

    /* 2. RAW16 写出 → RAW 读回（加载器回路测试）*/
    expect(it_raw16_write("test_loop.raw", &bayer), "raw16 write");
    ItImage reloaded;
    ItRawFormat fmt = { IT_BITS_16, IT_BYTEORDER_LH, W, H, IT_PAT_RGGB };
    expect(it_raw_load("test_loop.raw", &fmt, 0, &reloaded), "raw load");
    require(memcmp(reloaded.data, bayer.data,
                   (size_t)W * H * sizeof(uint16_t)) == 0,
            "raw roundtrip content");
    it_image_free(&reloaded);

    /* 2b. dummy 裁剪回路：2 行 2 列 → 中心 (W-2)x(H-2)，偏移 (1,1) */
    ItRawFormat fmtCrop = fmt;
    fmtCrop.dummy_rows = 2;
    fmtCrop.dummy_cols = 2;
    ItImage cropped;
    expect(it_raw_load("test_loop.raw", &fmtCrop, 0, &cropped), "raw crop load");
    require(cropped.width == W - 2 && cropped.height == H - 2, "crop size");
    require(cropped.data[0] == bayer.data[(long)1 * W + 1], "crop origin");
    require(cropped.data[(long)(H - 3) * (W - 2) + (W - 3)] ==
            bayer.data[(long)(H - 2) * W + (W - 2)], "crop end");
    it_image_free(&cropped);
    remove("test_loop.raw");

    /* 2c. HL 字节序读取：LE 写入的 0x0123 按 HL 应读为 0x2301 */
    {
        ItImage one;
        expect(it_image_alloc(&one, 1, 1), "alloc one");
        one.data[0] = 0x0123;
        expect(it_raw16_write("test_swap.raw", &one), "raw16 write swap");
        ItRawFormat fhl = { IT_BITS_16, IT_BYTEORDER_HL, 1, 1, IT_PAT_RGGB, 0, 0 };
        ItImage rhl;
        expect(it_raw_load("test_swap.raw", &fhl, 0, &rhl), "raw HL load");
        require(rhl.data[0] == 0x2301, "HL byte order");
        it_image_free(&one);
        it_image_free(&rhl);
        remove("test_swap.raw");
    }

    /* 2d. ASCII .dat 路径（原 0x472040 行为）*/
    {
        FILE *f = fopen("test_ascii.dat", "w");
        require(f != NULL, "dat create");
        fprintf(f, "0 511 1023\n");
        fclose(f);
        ItRawFormat fdat = { IT_BITS_10, IT_BYTEORDER_LH, 3, 1, IT_PAT_RGGB, 0, 0 };
        ItImage rdat;
        expect(it_raw_load("test_ascii.dat", &fdat, 1, &rdat), "dat load");
        require(rdat.data[0] == 0 && rdat.data[1] == 511 &&
                rdat.data[2] == 1023, "dat values");
        it_image_free(&rdat);
        remove("test_ascii.dat");
    }

    /* 3. 全流水线（默认参数 = 恒等；开启钳位与调试转储以覆盖对应路径）*/
    ItRgb out;
    strcpy(cfg.dump_dir, ".");
    cfg.dump_ppm    = 1;
    cfg.dump_dclamp = 1;
    cfg.dump_wb     = 1;
    cfg.dclp_on     = 1;
    expect(it_isp_run(&bayer, &cfg, 1, &out), "isp run");
    require(out.width == W && out.height == H, "isp output size");

    /* 3b. BPR 热点校正：注入孤立热点后应被邻域均值拉回 */
    {
        ItImage hot;
        expect(it_image_alloc(&hot, W, H), "alloc hot");
        memcpy(hot.data, bayer.data, (size_t)W * H * sizeof(uint16_t));
        int hx = 20, hy = 10;
        hot.data[(long)hy * W + hx] = 1023;      /* 孤立热点 */
        cfg.bpr_on  = 1;
        cfg.bpr_th1 = 100;                        /* 3x3 检出 */
        cfg.bpr_th3 = 150;
        cfg.bpr_th5 = 200;
        cfg.bpr_th7 = 250;
        ItRgb outHot;
        expect(it_isp_run(&hot, &cfg, 0, &outHot), "isp hot run");
        /* (20,10) 在 RGGB 下为 Gb；输出取 G 通道 */
        int gAfter = outHot.data[((long)hy * W + hx) * 3 + 1];
        require(gAfter < 200, "bpr corrected hot pixel");
        cfg.bpr_on = 0;
        it_rgb_free(&outHot);
        it_image_free(&hot);
    }

    /* 3d. full_scale 量程：16bit 高值不被截断到 10bit */
    {
        ItImage hi;
        expect(it_image_alloc(&hi, 8, 8), "alloc hi");
        hi.full_scale = 65535;
        for (long i = 0; i < 64; i++)
            hi.data[i] = 40000;
        cfg.dclp_on     = 0;            /* 直通各级，仅验证量程传递 */
        cfg.wb_gain_r   = 256;
        cfg.wb_gain_g   = 256;
        cfg.wb_gain_b   = 256;
        ItRgb outHi;
        expect(it_isp_run(&hi, &cfg, 0, &outHi), "isp hi run");
        require(outHi.data[0] > 100, "16bit range preserved");
        cfg.dclp_on = 1;                /* 恢复后续用例的默认 */
        it_rgb_free(&outHi);
        it_image_free(&hi);
    }

    /* 3c. RGB 前置滤波：平滑斜坡图上均值应近似保持 */
    {
        cfg.rgb_filter_on = 1;
        cfg.sig01_const   = 64;
        cfg.sig01_mult    = 0;
        cfg.sig02_mult    = 256;
        ItRgb outF;
        expect(it_isp_run(&bayer, &cfg, 0, &outF), "isp prefilter run");
        double mBase = 0.0, mFilt = 0.0;
        for (long i = 0; i < (long)W * H; i++) {
            mBase += out.data[i * 3];
            mFilt += outF.data[i * 3];
        }
        mBase /= (long)W * H;
        mFilt /= (long)W * H;
        double d = mBase - mFilt;
        if (d < 0) d = -d;
        require(d < 3.0, "prefilter preserves smooth mean");
        cfg.rgb_filter_on = 0;
        it_rgb_free(&outF);
    }

    /* 3e. Bypass 预览必须是纯灰度 R==G==B（用户报告回归：曾泛绿）*/
    {
        ItRgb pv;
        expect(it_isp_preview(&bayer, &cfg, &pv), "preview run");
        int allGray = 1;
        for (long i = 0; i < (long)W * H; i++)
            if (pv.data[i * 3] != pv.data[i * 3 + 1] ||
                pv.data[i * 3] != pv.data[i * 3 + 2]) { allGray = 0; break; }
        require(allGray, "preview is grayscale");
        it_rgb_free(&pv);
    }

    /* 3f. 最近邻 demosaic 必须填满三通道（内部像素）*/
    {
        ItRgb nn;
        expect(it_bayer_demosaic(&bayer, IT_PAT_RGGB, IT_DEMOSAIC_NEAREST,
                                 &nn), "nearest demosaic");
        long i = ((long)40 * W + 50) * 3;
        require(nn.data[i] > 0 && nn.data[i + 1] > 0 && nn.data[i + 2] > 0,
                "nearest fills all channels");
        it_rgb_free(&nn);
    }

    /* 3g. 行裁剪仿真（丢首行）：第 2 行成为新首行，量程继承 */
    {
        ItImage cropped;
        expect(it_image_crop(&bayer, 1, 0, W, H - 1, &cropped),
               "crop rows");
        require(cropped.height == H - 1 &&
                cropped.data[0] == bayer.data[(long)1 * W] &&
                cropped.full_scale == bayer.full_scale,
                "crop top row content");
        /* 丢首行后按原 pattern 解析会错位——此处仅验证数据位移 */
        require(it_image_crop(&bayer, 0, 0, W, H + 1, &cropped) == IT_E_PARAM,
                "crop out of range rejected");
        it_image_free(&cropped);
    }

    /* 4. 统计 */
    ItStat stat;
    expect(it_stat_compute(&out, NULL, &stat), "stat compute");
    require(stat.total == (long)W * H, "stat total");

    /* 4b. 掩码统计（区域 SNR 路径）：只统计左半幅 */
    ItImage roi;
    expect(it_image_alloc(&roi, W, H), "alloc roi mask");
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W / 2; x++)
            roi.data[(long)y * W + x] = 1;
    ItStat statRoi;
    expect(it_stat_compute(&out, &roi, &statRoi), "stat roi");
    require(statRoi.total == (long)W / 2 * H, "stat roi total");
    it_image_free(&roi);

    /* 4c. 16bit 原始直方图：10bit 数据折叠后总和守恒 */
    long hist[256];
    expect(it_hist_compute(&bayer, 10, hist), "hist compute");
    long hsum = 0;
    for (int i = 0; i < 256; i++)
        hsum += hist[i];
    require(hsum == (long)W * H, "hist total");
    require(hist[0] > 0, "hist nonzero bin");
    expect(it_stat_write_summary("test_summary.txt", "bayer_synthetic",
                                 &stat), "stat summary");
    expect(it_stat_write_csv("test_stat.csv", "bayer_synthetic", &stat),
           "stat csv");
    verify_csv("test_stat.csv");

    /* 5. 保存（BMP 头部字节级校验）*/
    long rowsz = ((long)W * 3 + 3) & ~3L;
    expect(it_bmp_write("test_out.bmp", &out), "bmp write");
    verify_bmp_header("test_out.bmp", 54 + rowsz * H);

    ItImage y;
    expect(it_y_from_rgb(&out, &cfg, &y), "y gen");
    expect(it_ppm_gray_write("test_out_y.ppm", &y, 255), "ppm gray write");
    it_image_free(&y);

    /* 6. INI 参数回路：改参数 → 存 → 改回默认 → 载入 → 比对 */
    cfg.cisgain_r = 123;
    cfg.wb_gain_b = 400;
    cfg.rgm[1][1] = 250;
    cfg.gamma_value = 2.2;
    cfg.gain_h[4] = 123;
    cfg.gain_v[8] = -5;
    expect(it_config_save_ini(&cfg, "test_isp.ini"), "ini save");

    ItConfig cfg2;
    it_config_default(&cfg2);
    int unknown = -1;
    expect(it_config_load_ini("test_isp.ini", &cfg2, &unknown), "ini load");
    require(cfg2.cisgain_r == 123, "ini cisgain_r");
    require(cfg2.wb_gain_b == 400, "ini wb_gain_b");
    require(cfg2.rgm[1][1] == 250, "ini rgm21");
    require(cfg2.gamma_value == 2.2, "ini gamma");
    require(cfg2.gain_h[4] == 123 && cfg2.gain_v[8] == -5,
            "ini gain keys");
    require(cfg2.dclp_strt_v == cfg.dclp_strt_v, "ini untouched key");
    require(unknown == 0, "ini unknown keys");
    remove("test_isp.ini");

    /* 7. 清理与输出 */
    remove("test_stat.csv");
    it_rgb_free(&out);
    it_image_free(&bayer);

    printf("mean Y=%.2f R=%.2f G=%.2f B=%.2f  SNR R=%.2f\n",
           stat.mean[0], stat.mean[1], stat.mean[2], stat.mean[3],
           stat.snr[1]);
    printf("SMOKE TEST PASSED\n");
    return 0;
}
