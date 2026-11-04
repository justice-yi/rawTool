/* bayer.h - Bayer → RGB888 解马赛克（复刻 0x421850 的 Bayer 显示路径）*/
#ifndef BAYER_H
#define BAYER_H

#include "itypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    IT_DEMOSAIC_NEAREST = 0,    /* 复制最近同色像素（原程序预览路径）*/
    IT_DEMOSAIC_BILINEAR = 1    /* 3x3 双线性插值                    */
} ItDemosaicMethod;

/* 单通道 Bayer 平面 → RGB888。pattern 决定 (0,0) 像素颜色。
 * scale16：输入满量程（1023 或 65535），自动线性映射到 0..255。 */
ItStatus it_bayer_demosaic(const ItImage *in, ItBayerPattern pattern,
                           ItDemosaicMethod method, ItRgb *out);

/* 返回排列的英文名（"RGGB"…，供状态栏/文件名使用） */
const char *it_bayer_pattern_name(ItBayerPattern pattern);

#ifdef __cplusplus
}
#endif
#endif /* BAYER_H */
