/* raw_loader.h - RAW/Bayer 文件加载（复刻 0x472040 的 .dat ASCII 与 raw 二进制度路径）
 *
 * 支持格式（与原程序文件过滤器一致）：
 *   .raw/.byr  二进制，8/10/16 bit（Raw Format 对话框选择）
 *   .dat       10bit ASCII 十进制文本（原程序以
 *              "10 bit (Yes) 8 bit (No)" MessageBox 询问位深）
 *   .sraw      Samsung RAW（同 .raw 路径）
 */
#ifndef RAW_LOADER_H
#define RAW_LOADER_H

#include "itypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 按显式格式加载：fmt 的 bits/width/height/order 均须有效。
 * 输出 img 为 16bit 平面，数值范围 [0, (1<<bits)-1]。
 * fmt->dummy_rows/dummy_cols > 0 时按中心对称裁掉对应 dummy 行/列
 * （对应原程序 1288x1032 等带 dummy 边界的传感器输出）。
 * prefer_ascii：非 0 时把文件按十进制 ASCII 文本解析（.dat）。 */
ItStatus it_raw_load(const char *path, const ItRawFormat *fmt,
                     int prefer_ascii, ItImage *img);

/* 从文件长度与已知分辨率表推断格式（原程序 .data 内置的
 * 640x480/800x600/1280x1024/1600x1200/2056x1544/2608x1952 及其
 * dummy 行版本 1288x1032/1608x1208/2616x1960）。成功时回填 fmt。
 * 仅当 width/height 尚未指定（<=0）时推断。 */
ItStatus it_raw_guess_format(const char *path, ItRawFormat *fmt);

/* 已知分辨率表（供 GUI 下拉框复用） */
int it_raw_known_sizes(int *out_w, int *out_h, int max_entries);

#ifdef __cplusplus
}
#endif
#endif /* RAW_LOADER_H */
