/* analysis.h - 统计分析（复刻 0x4338b0/0x433ce0 的 Mean/Std/SNR/Max/Min）*/
#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "itypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 对 RGB 图做 Y/R/G/B 四通道统计；mask为NULL时全图统计。
 * SNR = Mean / Std（0x4338b0 的 s_SNR___6_2f 输出语义）。 */
ItStatus it_stat_compute(const ItRgb *rgb, const ItImage *ymask,
                         ItStat *stat);

/* 16bit 单通道平面 → 256 级直方图。bits 为数据位深（8/10/16），
 * 决定折叠移位（bits-8），10bit 数据占低 4 级、16bit 占满 256 级。 */
ItStatus it_hist_compute(const ItImage *img, int bits, long hist[256]);

/* 按原程序 hist_summary.txt / statistic_summary.txt 的格式写摘要 */
ItStatus it_stat_write_summary(const char *path, const char *image_name,
                               const ItStat *stat);

/* 统计导出为 CSV（替代原程序的 Excel OLE 自动化导出，见 0x44a220）*/
ItStatus it_stat_write_csv(const char *path, const char *image_name,
                           const ItStat *stat);

#ifdef __cplusplus
}
#endif
#endif /* ANALYSIS_H */
