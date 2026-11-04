/* gen_sample.c - 生成可验证的 Bayer RAW 样图 + ISP 预览 BMP
 * 用 itcore 库自身生成与处理：既是样图，也是库的端到端演示。
 * 样图内容：8 条彩色竖带（白/黄/青/绿/品红/红/蓝/黑，100% 饱和度 10bit 值）
 *           + 底部 1/4 高度的水平亮度渐变区。 */
#include <stdio.h>
#include <string.h>
#include "core/itypes.h"
#include "core/raw_loader.h"
#include "core/isp_pipeline.h"
#include "core/imgio.h"

int main(int argc, char **argv)
{
    const char *dir = (argc > 1) ? argv[1] : ".";
    const int W = 640, H = 480;                 /* 原程序内置分辨率之一 */

    /* SMPTE 风格 8 色带（10bit 全值 1023 / 半值 ~512）*/
    const int bars[8][3] = {
        {1023, 1023, 1023},   /* 白 */
        {1023, 1023,    0},   /* 黄 */
        {   0, 1023, 1023},   /* 青 */
        {   0, 1023,    0},   /* 绿 */
        {1023,    0, 1023},   /* 品红 */
        {1023,    0,    0},   /* 红 */
        {   0,    0, 1023},   /* 蓝 */
        {   0,    0,    0},   /* 黑 */
    };

    ItImage img;
    if (it_image_alloc(&img, W, H) != IT_OK) {
        fprintf(stderr, "alloc failed\n");
        return 1;
    }
    img.full_scale = 1023;                      /* 10bit 量程 */

    ItConfig cfg;
    it_config_default(&cfg);
    cfg.pat_sel = IT_PAT_RGGB;

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int band = x * 8 / W;               /* 竖向色带 */
            int r = bars[band][0], g = bars[band][1], b = bars[band][2];
            if (y >= H * 3 / 4) {               /* 底部渐变：向黑色过渡 */
                double k = 1.0 - (double)(y - H * 3 / 4) / (H / 4);
                r = (int)(r * k); g = (int)(g * k); b = (int)(b * k);
            }
            /* 按 RGGB 排列写入对应通道值 */
            int row = y & 1, col = x & 1, c;
            switch (cfg.pat_sel) {
            case IT_PAT_RGGB: c = row ? (col ? 2 : 1) : (col ? 1 : 0); break;
            default: c = 1; break;
            }
            img.data[(long)y * W + x] =
                (uint16_t)(c == 0 ? r : c == 2 ? b : g);
        }
    }

    char path[512];
    snprintf(path, sizeof(path), "%s/sample_colorbar_640x480_rggb10.raw", dir);
    if (it_raw16_write(path, &img) != IT_OK) {  /* 2 字节小端容器，值域 0..1023 */
        fprintf(stderr, "raw write failed\n");
        return 1;
    }

    /* 端到端：重新从磁盘加载 → ISP 全链 → BMP 预览 */
    ItRawFormat fmt = { IT_BITS_10, IT_BYTEORDER_LH, W, H, IT_PAT_RGGB, 0, 0 };
    ItImage loaded;
    if (it_raw_load(path, &fmt, 0, &loaded) != IT_OK) {
        fprintf(stderr, "reload failed\n");
        return 1;
    }
    ItRgb out;
    if (it_isp_run(&loaded, &cfg, 0, &out) != IT_OK) {
        fprintf(stderr, "isp failed\n");
        return 1;
    }
    snprintf(path, sizeof(path), "%s/sample_preview.bmp", dir);
    if (it_bmp_write(path, &out) != IT_OK) {
        fprintf(stderr, "bmp write failed\n");
        return 1;
    }

    it_image_free(&img);
    it_image_free(&loaded);
    it_rgb_free(&out);
    printf("OK sample written to %s\n", dir);
    return 0;
}
