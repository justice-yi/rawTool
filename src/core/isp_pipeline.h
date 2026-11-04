/* isp_pipeline.h - ISP 仿真流水线（复刻 0x404d20 驱动的分级处理）
 *
 * 原程序流程（反编译确认）：
 *   PREPROCESS & SHADING CORRECTION
 *     1. 数字钳位 + CIS 增益   (0x405b60, 逐像素，Q5 定点)
 *     2. 白平衡 1 通道          (0x4060e0/0x406320)
 *     3. Shading 校正           (0x402bf0 增益 / 0x404590 应用)
 *     4. BPR 坏点校正           (0x40b230, 3x3 邻域)
 *     5. GRGB/NR 绿不平衡       (GRGB_ON 参数组)
 *   RGB 域（demosaic 后）：
 *     6. RGB 二级增益/偏移      (gain_*_2nd / offset_*_2nd)
 *     7. RGB MATRIX PROCESS     (0x4050f0, 3x3)
 *     8. Gamma                  (GAM_SEL / gamma_value)
 *     9. Color Correct          (COLOR_CORRECT)
 *    10. Y 生成                 (RY/GY/BY_COEF)
 * 每级可选转储 00_..04_*.ppm（0x40d110，P2 ASCII / raw）。
 */
#ifndef ISP_PIPELINE_H
#define ISP_PIPELINE_H

#include "itypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 运行整条流水线：输入 16bit Bayer 平面，输出 demosaic+ISP 后的 RGB。
 * cfg 中各使能位为 0 的级自动跳过（与原程序行为一致）。
 * dump 非 0 时按 cfg->dump_* 输出阶段文件到 cfg->dump_dir。 */
ItStatus it_isp_run(const ItImage *bayer_in, const ItConfig *cfg,
                    int dump, ItRgb *out);

/* 16bit Bayer 平面 → 8bit RGB（仅 demosaic，供"原始预览"）*/
ItStatus it_isp_preview(const ItImage *bayer_in, const ItConfig *cfg,
                        ItRgb *out);

/* 亮度平面提取（Y = (RY*R + GY*G + BY*B) >> 8，DIALOG 184 系数）*/
ItStatus it_y_from_rgb(const ItRgb *in, const ItConfig *cfg, ItImage *out);

#ifdef __cplusplus
}
#endif
#endif /* ISP_PIPELINE_H */
