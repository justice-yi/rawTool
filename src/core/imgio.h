/* imgio.h - 图像写出（BMP24 / PPM P2/P3 / RAW16；复刻原保存路径过滤器
 * "Supported Files(*.raw,*.bmp,*.ppm)"）*/
#ifndef IMGIO_H
#define IMGIO_H

#include "itypes.h"

#ifdef __cplusplus
extern "C" {
#endif

ItStatus it_bmp_write(const char *path, const ItRgb *rgb);
ItStatus it_ppm_gray_write(const char *path, const ItImage *img, int maxval);
ItStatus it_ppm_rgb_write(const char *path, const ItRgb *rgb);
ItStatus it_raw16_write(const char *path, const ItImage *img);

#ifdef __cplusplus
}
#endif
#endif /* IMGIO_H */
