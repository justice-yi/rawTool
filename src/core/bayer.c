/* bayer.c - Bayer 解马赛克实现 */
#include "bayer.h"
#include <string.h>

const char *it_bayer_pattern_name(ItBayerPattern pattern)
{
    switch (pattern) {
    case IT_PAT_RGGB: return "RGGB";
    case IT_PAT_GRBG: return "GRBG";
    case IT_PAT_GBRG: return "GBRG";
    case IT_PAT_BGGR: return "BGGR";
    }
    return "?";
}

/* (x,y) 处的颜色：0=R 1=G 2=B（依据排列与像素奇偶） */
static int color_at(ItBayerPattern p, int x, int y)
{
    int row = (y & 1) << 1;     /* 0 或 2 */
    int col = (x & 1);
    switch (p) {
    case IT_PAT_RGGB: { static const int m[2][2] = {{0,1},{1,2}}; return m[row?1:0][col]; }
    case IT_PAT_GRBG: { static const int m[2][2] = {{1,0},{2,1}}; return m[row?1:0][col]; }
    case IT_PAT_GBRG: { static const int m[2][2] = {{1,2},{0,1}}; return m[row?1:0][col]; }
    case IT_PAT_BGGR: { static const int m[2][2] = {{2,1},{1,0}}; return m[row?1:0][col]; }
    }
    return 1;
}

ItStatus it_bayer_demosaic(const ItImage *in, ItBayerPattern pattern,
                           ItDemosaicMethod method, ItRgb *out)
{
    if (!in || !in->data || !out || in->width <= 0 || in->height <= 0)
        return IT_E_PARAM;
    if (pattern < IT_PAT_RGGB || pattern > IT_PAT_BGGR)
        return IT_E_PARAM;

    ItStatus st = it_rgb_alloc(out, in->width, in->height);
    if (st != IT_OK)
        return st;

    /* 满量程：优先用图像自带的量程字段（raw_loader 按位深设置），
     * 字段异常（<=0）时回退为按数据探测。 */
    unsigned full;
    if (in->full_scale == 255 || in->full_scale == 1023 ||
        in->full_scale == 65535) {
        full = (unsigned)in->full_scale;
    } else {
        unsigned vmax = 0;
        for (long i = 0; i < (long)in->width * in->height; i++)
            if (in->data[i] > vmax) vmax = in->data[i];
        full = (vmax <= 1023) ? 1023u : 65535u;
    }

    const int w = in->width, h = in->height;
    const uint16_t *src = in->data;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            unsigned c[3] = { 0, 0, 0 };
            if (method == IT_DEMOSAIC_NEAREST) {
                /* 最近同色插值：本通道取原值，缺失通道取最近的同色
                 * 邻居（距离相等取扫描序最先者，确定性）*/
                int own = color_at(pattern, x, y);
                c[own] = src[(long)y * w + x];
                for (int mc = 0; mc < 3; mc++) {
                    if (mc == own)
                        continue;
                    int best = -1, bestD = 0;
                    for (int dy = -2; dy <= 2; dy++) {
                        int yy = y + dy;
                        if (yy < 0 || yy >= h) continue;
                        for (int dx = -2; dx <= 2; dx++) {
                            int xx = x + dx;
                            if ((dx == 0 && dy == 0) || xx < 0 || xx >= w)
                                continue;
                            if (color_at(pattern, xx, yy) != mc)
                                continue;
                            int d = dx * dx + dy * dy;
                            if (best < 0 || d < bestD) {
                                best = yy * w + xx;
                                bestD = d;
                            }
                        }
                    }
                    if (best >= 0)
                        c[mc] = src[best];
                }
            } else {
                /* 双线性：按本像素颜色补充另外两色（3x3 均值）*/
                int own = color_at(pattern, x, y);
                int cnt[3] = { 0, 0, 0 };
                for (int dy = -1; dy <= 1; dy++) {
                    int yy = y + dy;
                    if (yy < 0 || yy >= h) continue;
                    for (int dx = -1; dx <= 1; dx++) {
                        int xx = x + dx;
                        if (xx < 0 || xx >= w) continue;
                        int cc = color_at(pattern, xx, yy);
                        c[cc] += src[(long)yy * w + xx];
                        cnt[cc]++;
                    }
                }
                for (int k = 0; k < 3; k++) {
                    if (cnt[k] > 0)
                        c[k] = (c[k] + cnt[k] / 2) / cnt[k];   /* 四舍五入 */
                }
                c[own] = src[(long)y * w + x];   /* 本像素真值优先 */
            }
            uint8_t *px = &out->data[((long)y * w + x) * 3];
            for (int k = 0; k < 3; k++) {
                unsigned v = (c[k] * 255u + full / 2) / full;
                if (v > 255) v = 255;
                px[k] = (uint8_t)v;
            }
        }
    }
    return IT_OK;
}
