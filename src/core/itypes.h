/* itypes.h - 公共类型与错误码（复刻 ImageTool0_360R 的 ISP 对象布局语义）
 *
 * 逆向依据：原程序 ISP 对象为一大块以 this+偏移访问的状态区
 * (Ghidra 反编译 0x404d20/0x40f2d0/0x405b60 等)，此处重构为 ItConfig +
 * ItImage 两个 POD 结构，字段与原偏移一一对应并注释原偏移。
 */
#ifndef ITYPES_H
#define ITYPES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- 错误码 ---- */
typedef enum {
    IT_OK = 0,
    IT_E_OPEN,          /* 文件无法打开                     */
    IT_E_FORMAT,        /* 无法识别的格式                    */
    IT_E_SIZE,          /* 尺寸与文件长度不匹配              */
    IT_E_MEMORY,        /* 内存分配失败                      */
    IT_E_PARAM,         /* 参数非法                          */
    IT_E_IO             /* 写文件失败                        */
} ItStatus;

/* ---- Bayer 排列（对应原参数 PAT_SEL，偏移 0xda0）----
 * 命名按 2x2 左上角两个颜色：
 *   RGGB: R G / G B      GRBG: G R / B G
 *   GBRG: G B / R G      BGGR: B G / G R
 */
typedef enum {
    IT_PAT_RGGB = 0,
    IT_PAT_GRBG = 1,
    IT_PAT_GBRG = 2,
    IT_PAT_BGGR = 3
} ItBayerPattern;

/* ---- RAW 文件格式描述（对应 Raw Format 对话框，资源 DIALOG 182）---- */
typedef enum {
    IT_BITS_8  = 8,     /* 每 1 字节 1 像素                  */
    IT_BITS_10 = 10,    /* 每 2 字节 1 像素，低 10 位有效     */
    IT_BITS_16 = 16     /* 每 2 字节 1 像素，16 位            */
} ItBitDepth;

typedef enum {          /* 10bit packed 时的字节序（对话框按钮 HL / LH） */
    IT_BYTEORDER_LH = 0,/* 低字节在前 (little/high?) 原程序 HL/LH 二选一 */
    IT_BYTEORDER_HL = 1
} ItByteOrder;

typedef struct {
    ItBitDepth   bits;
    ItByteOrder  order;
    int          width;     /* H size（文件中的总列数）*/
    int          height;    /* V size（文件中的总行数）*/
    ItBayerPattern pattern;
    int          dummy_rows;/* 裁掉的 dummy 行总数（上下各半）*/
    int          dummy_cols; /* 裁掉的 dummy 列总数（左右各半）*/
} ItRawFormat;

/* ---- 16bit 单通道平面（ISP 全程的数据载体，对应原 +0x7c/+0x80 缓冲）---- */
typedef struct {
    int      width;
    int      height;
    int      full_scale;    /* 量程满值：8bit=255 / 10bit=1023 / 16bit=65535 */
    uint16_t *data;         /* 行主序，长度 width*height */
} ItImage;

/* ---- 8bit RGB 图（demosaic 及之后各级的载体）---- */
typedef struct {
    int      width;
    int      height;
    uint8_t  *data;         /* 行主序 interleaved R,G,B */
} ItRgb;

/* ---- 统计结果（对应原对象 0x10d0.. Mean/Std/Max/Min/SNR 区）---- */
typedef struct {
    double mean[4];         /* [0..3] = Y,R,G,B —— 原 hist_summary 输出序 */
    double std[4];
    double snr[4];
    long   maxv[4];
    long   minv[4];
    long   total;           /* Total Pixel —— 原 +0x68 */
} ItStat;

/* ---- ISP 参数集（对应原 INI 节 ISP_P1，函数 0x40f2d0 注册的命名参数）----
 * 注释给出原对象偏移；默认值取自原注册代码。
 */
typedef struct {
    /* 预处理 / 通用 */
    int  preprocess_th;         /* 0xd9c  PreprocessTh   */
    ItBayerPattern pat_sel;     /* 0xda0  PAT_SEL        */

    /* 数字钳位（DIALOG 176）*/
    int  dclp_on;               /* 0xda4  DCLP_ON        */
    int  dclp_strt_h;           /* 0xda8  DCLP_STRT_H    */
    int  dclp_strt_v;           /* 0xdac  DCLP_STRT_V    */
    int  poffset;               /* 0xdae  POFFSET        */
    int  cisgain_gr;            /* 0xdb0  CISGAIN_GR     */
    int  cisgain_gb;            /* 0xdb2  CISGAIN_GB     */
    int  cisgain_r;             /* 0xdb4  CISGAIN_R      */
    int  cisgain_b;             /* 0xdb6  CISGAIN_B      */
    int  poffset_gr;            /* 0xdb8  POFFSET_GR     */
    int  poffset_gb;            /* 0xdba  POFFSET_GB     */
    int  poffset_r;             /* 0xdbc  POFFSET_R      */
    int  poffset_b;             /* 0xdbe  POFFSET_B      */

    /* 坏点校正 */
    int  bpr_on;                /* 0xdc0  BPR_ON         */
    int  bpr_th1;               /* 0xdc4  BPR_TH1        */
    int  bpr_th3;               /* 0xdc8  BPR_TH3        */
    int  bpr_th5;               /* 0xdcc  BPR_TH5        */
    int  bpr_th7;               /* 0xdd0  BPR_TH7        */

    /* 绿通道不平衡 / NR（GRGB）*/
    int  grgb_on;               /* 0xdd4  GRGB_ON        */
    int  grgb_grb;              /* 0xdd8  GRGB_GRB       */
    int  grgb_avg;              /* 0xddc  GRGB_AVG       */
    int  gthr01;                /* 0xde0  GTHR01         */

    /* RGB 前置滤波 */
    int  rgb_filter_on;         /* 0xe98  RGB_FILTER_ON  */
    int  rgb_gmedian_on;        /* 0xe9c  RGB_GMEDIAN_ON */
    int  sig01_const;           /* 0xea0  SIG01_CONST    */
    int  sig01_mult;            /* 0xea4  SIG01_MULT     */
    int  sig02_mult;            /* 0xea8  SIG02_MULT     */

    /* Shading（径向 / 分窗，DIALOG 内 SHADE_* 参数）*/
    int  shade_on;              /* SHADE_ON  */
    int  shade_radial;          /* SHADE_RADIAL */
    int  shade_xc;              /* SHADE_XC  径向中心 x */
    int  shade_yc;              /* SHADE_YC  径向中心 y */
    int  gain_h[9];             /* GAIN_H1..H9 分窗水平增益 */
    int  gain_v[9];             /* GAIN_V1..V9 分窗垂直增益 */
    int  sh_val_r[6];           /* SH_VAL_R0..R5 */
    int  sh_val_g[6];           /* SH_VAL_G0..G5 */
    int  sh_val_b[6];           /* SH_VAL_B0..B5 */

    /* 白平衡（DIALOG 181：CRWB/CGWB/CBWB 增益 + CRDS/CGDS/CBDS 暗电平）*/
    int  wb_gain_r;             /* CRWB */
    int  wb_gain_g;             /* CGWB */
    int  wb_gain_b;             /* CBWB */
    int  wb_ds_r;               /* CRDS */
    int  wb_ds_g;               /* CGDS */
    int  wb_ds_b;               /* CBDS */

    /* RGB 二级增益/偏移 */
    int  gain_r_2nd;            /* 0xef4 gain_r_2nd_r */
    int  gain_g_2nd;            /* 0xef8 gain_g_2nd_r */
    int  gain_b_2nd;            /* 0xefc gain_b_2nd_r */
    int  offset_r_2nd;          /* 0xf00 offset_r_2nd_r */
    int  offset_g_2nd;          /* 0xf04 offset_g_2nd_r */
    int  offset_b_2nd;          /* 0xf08 offset_b_2nd_r */

    /* 颜色矩阵（DIALOG 103，系数 RGM10..RGM32，行主序 3x3）*/
    int  rgm[3][3];             /* 0x10a0.. RGM10,RGM11,RGM12 / RGM20.. / RGM30.. */

    /* Gamma */
    int  gam_sel;               /* 0x1074 GAM_SEL */
    double gamma_value;         /* 0x107c.. Gamma 校正系数（Options/Gamma 对话框） */

    /* Y 生成（DIALOG 184：RY_COEF/GY_COEF/BY_COEF）*/
    int  ry_coef;               /* 0x?  RY_COEF */
    int  gy_coef;               /* 0x?  GY_COEF */
    int  by_coef;               /* 0x?  BY_COEF */

    /* 阶段调试转储（0x40d110 的 +0x1430..+0x1440 使能位）*/
    int  dump_ppm;              /* +0x142c 1=PPM(P2) 0=raw */
    int  dump_dclamp;           /* +0x1430 00_d_clamp_out */
    int  dump_wb;               /* +0x1434 01_wb_1ch_out */
    int  dump_shading;          /* +0x1438 02_shade_1ch_out */
    int  dump_bpr;              /* +0x143c 03_bpr_out */
    int  dump_grgb;             /* +0x1440 04_nr_or_grgb_out */
    char dump_dir[260];         /* +0x1490 CString 输出目录 */
} ItConfig;

/* ---- 内存管理（模块化边界：GUI 不直接 malloc 平面数据）---- */
void     it_image_free(ItImage *img);
void     it_rgb_free(ItRgb *rgb);
ItStatus it_image_alloc(ItImage *img, int width, int height);
ItStatus it_rgb_alloc(ItRgb *rgb, int width, int height);

/* 行列裁剪：取 in 的 [top, top+out_h) × [left, left+out_w) 区域。
 * 仿真用途：丢首行（top=N）后按 pattern 重解析的效果预览。
 * out->full_scale 继承 in。 */
ItStatus it_image_crop(const ItImage *in, int top, int left,
                       int out_w, int out_h, ItImage *out);

/* 取出 ItConfig 的原程序默认值（0x40f2d0 注册的 default） */
void     it_config_default(ItConfig *cfg);

const char *it_status_str(ItStatus st);

#ifdef __cplusplus
}
#endif
#endif /* ITYPES_H */
