/* itypes.c - 平面内存管理与默认参数 */
#include "itypes.h"
#include <stdlib.h>
#include <string.h>

void it_image_free(ItImage *img)
{
    if (img && img->data) {
        free(img->data);
        img->data = NULL;
    }
}

void it_rgb_free(ItRgb *rgb)
{
    if (rgb && rgb->data) {
        free(rgb->data);
        rgb->data = NULL;
    }
}

ItStatus it_image_alloc(ItImage *img, int width, int height)
{
    if (!img || width <= 0 || height <= 0)
        return IT_E_PARAM;
    img->width  = width;
    img->height = height;
    img->full_scale = 1023;         /* 默认 10bit 量程；加载器按位深覆盖 */
    img->data   = (uint16_t *)malloc((size_t)width * height * sizeof(uint16_t));
    if (!img->data) {
        img->width = img->height = 0;
        return IT_E_MEMORY;
    }
    memset(img->data, 0, (size_t)width * height * sizeof(uint16_t));
    return IT_OK;
}

ItStatus it_rgb_alloc(ItRgb *rgb, int width, int height)
{
    if (!rgb || width <= 0 || height <= 0)
        return IT_E_PARAM;
    rgb->width  = width;
    rgb->height = height;
    rgb->data   = (uint8_t *)malloc((size_t)width * height * 3);
    if (!rgb->data) {
        rgb->width = rgb->height = 0;
        return IT_E_MEMORY;
    }
    memset(rgb->data, 0, (size_t)width * height * 3);
    return IT_OK;
}

ItStatus it_image_crop(const ItImage *in, int top, int left,
                       int out_w, int out_h, ItImage *out)
{
    if (!in || !in->data || !out || top < 0 || left < 0 ||
        out_w <= 0 || out_h <= 0 ||
        left + out_w > in->width || top + out_h > in->height)
        return IT_E_PARAM;
    ItStatus st = it_image_alloc(out, out_w, out_h);
    if (st != IT_OK)
        return st;
    out->full_scale = in->full_scale;   /* 量程随数据传递 */
    for (int y = 0; y < out_h; y++)
        memcpy(out->data + (long)y * out_w,
               in->data + (long)(y + top) * in->width + left,
               (size_t)out_w * sizeof(uint16_t));
    return IT_OK;
}

/* 默认值策略：Ghidra 反编译 0x40f2d0 给出参数名/对象偏移/类型
 * （第二实参为类型：0=int、1=short、2=float；INI 节 ISP_P1）。
 * 复刻统一增益语义为定点：CISGAIN 用 Q5（32=×1.0，对应原 0x405b60
 * 的 /32），WB 与二级增益用 Q8（256=×1.0），默认全部为恒等 ×1.0。 */
void it_config_default(ItConfig *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    cfg->preprocess_th = 2;
    cfg->pat_sel       = IT_PAT_RGGB;

    /* 数字钳位（对话框 176）*/
    cfg->dclp_on     = 0;
    cfg->dclp_strt_h = 0;
    cfg->dclp_strt_v = 1;
    cfg->poffset     = 0;
    cfg->cisgain_gr  = 32;      /* Q5: 32 = ×1.0 */
    cfg->cisgain_gb  = 32;
    cfg->cisgain_r   = 32;
    cfg->cisgain_b   = 32;
    cfg->poffset_gr  = 0;
    cfg->poffset_gb  = 0;
    cfg->poffset_r   = 0;
    cfg->poffset_b   = 0;

    /* 白平衡（对话框 181）*/
    cfg->wb_gain_r = 256;       /* Q8: 256 = ×1.0 */
    cfg->wb_gain_g = 256;
    cfg->wb_gain_b = 256;
    cfg->wb_ds_r   = 0;
    cfg->wb_ds_g   = 0;
    cfg->wb_ds_b   = 0;

    /* Shading 恒等：中心=角落、无分窗增益 */
    cfg->sh_val_r[0] = cfg->sh_val_r[1] = 0;
    cfg->sh_val_g[0] = cfg->sh_val_g[1] = 0;
    cfg->sh_val_b[0] = cfg->sh_val_b[1] = 0;

    /* RGB 二级增益/偏移 */
    cfg->gain_r_2nd   = 256;
    cfg->gain_g_2nd   = 256;
    cfg->gain_b_2nd   = 256;
    cfg->offset_r_2nd = 0;
    cfg->offset_g_2nd = 0;
    cfg->offset_b_2nd = 0;

    /* 颜色矩阵 = 单位阵 */
    cfg->rgm[0][0] = 1; cfg->rgm[0][1] = 0; cfg->rgm[0][2] = 0;
    cfg->rgm[1][0] = 0; cfg->rgm[1][1] = 1; cfg->rgm[1][2] = 0;
    cfg->rgm[2][0] = 0; cfg->rgm[2][1] = 0; cfg->rgm[2][2] = 1;

    cfg->gamma_value = 1.0;
    cfg->gam_sel     = 0;

    /* Y 生成系数（原对话框 RY/GY/BY_COEF：BT.601 整定，Q8）*/
    cfg->ry_coef = 77;   /* 0.299*256 */
    cfg->gy_coef = 150;  /* 0.587*256 */
    cfg->by_coef = 29;   /* 0.114*256 */

    cfg->dump_dir[0] = '\0';
}

const char *it_status_str(ItStatus st)
{
    switch (st) {
    case IT_OK:        return "OK";
    case IT_E_OPEN:    return "cannot open file";
    case IT_E_FORMAT:  return "unrecognized format";
    case IT_E_SIZE:    return "size mismatch";
    case IT_E_MEMORY:  return "out of memory";
    case IT_E_PARAM:   return "invalid parameter";
    case IT_E_IO:      return "file write failed";
    }
    return "unknown error";
}
