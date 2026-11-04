/* it_persist.c - ISP 参数 INI 持久化实现
 * 键名 = 原程序 .data 内的参数符号名（Ghidra 反编译 0x40f2d0）。 */
#include "it_persist.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---- 写出 ---- */
ItStatus it_config_save_ini(const ItConfig *cfg, const char *path)
{
    if (!cfg || !path)
        return IT_E_PARAM;
    FILE *f = fopen(path, "w");
    if (!f)
        return IT_E_OPEN;

    fprintf(f, "[ISP_P1]\n");
    fprintf(f, "; ImageTool-Qt ISP parameters (original names from ISP_P1)\n");
    fprintf(f, "PreprocessTh=%d\n",  cfg->preprocess_th);
    fprintf(f, "PAT_SEL=%d\n",       (int)cfg->pat_sel);
    fprintf(f, "DCLP_ON=%d\n",       cfg->dclp_on);
    fprintf(f, "DCLP_STRT_H=%d\n",   cfg->dclp_strt_h);
    fprintf(f, "DCLP_STRT_V=%d\n",   cfg->dclp_strt_v);
    fprintf(f, "POFFSET=%d\n",       cfg->poffset);
    fprintf(f, "CISGAIN_GR=%d\n",    cfg->cisgain_gr);
    fprintf(f, "CISGAIN_GB=%d\n",    cfg->cisgain_gb);
    fprintf(f, "CISGAIN_R=%d\n",     cfg->cisgain_r);
    fprintf(f, "CISGAIN_B=%d\n",     cfg->cisgain_b);
    fprintf(f, "POFFSET_GR=%d\n",    cfg->poffset_gr);
    fprintf(f, "POFFSET_GB=%d\n",    cfg->poffset_gb);
    fprintf(f, "POFFSET_R=%d\n",     cfg->poffset_r);
    fprintf(f, "POFFSET_B=%d\n",     cfg->poffset_b);
    fprintf(f, "BPR_ON=%d\n",        cfg->bpr_on);
    fprintf(f, "BPR_TH1=%d\n",       cfg->bpr_th1);
    fprintf(f, "BPR_TH3=%d\n",       cfg->bpr_th3);
    fprintf(f, "BPR_TH5=%d\n",       cfg->bpr_th5);
    fprintf(f, "BPR_TH7=%d\n",       cfg->bpr_th7);
    fprintf(f, "GRGB_ON=%d\n",       cfg->grgb_on);
    fprintf(f, "GRGB_GRB=%d\n",      cfg->grgb_grb);
    fprintf(f, "GRGB_AVG=%d\n",      cfg->grgb_avg);
    fprintf(f, "GTHR01=%d\n",        cfg->gthr01);
    fprintf(f, "RGB_FILTER_ON=%d\n", cfg->rgb_filter_on);
    fprintf(f, "RGB_GMEDIAN_ON=%d\n",cfg->rgb_gmedian_on);
    fprintf(f, "SIG01_CONST=%d\n",   cfg->sig01_const);
    fprintf(f, "SIG01_MULT=%d\n",    cfg->sig01_mult);
    fprintf(f, "SIG02_MULT=%d\n",    cfg->sig02_mult);
    fprintf(f, "SHADE_ON=%d\n",      cfg->shade_on);
    fprintf(f, "SHADE_RADIAL=%d\n",  cfg->shade_radial);
    fprintf(f, "SHADE_XC=%d\n",      cfg->shade_xc);
    fprintf(f, "SHADE_YC=%d\n",      cfg->shade_yc);
    fprintf(f, "GAIN_H1=%d\nGAIN_H2=%d\nGAIN_H3=%d\n",
            cfg->gain_h[0], cfg->gain_h[1], cfg->gain_h[2]);
    fprintf(f, "GAIN_H4=%d\nGAIN_H5=%d\nGAIN_H6=%d\n",
            cfg->gain_h[3], cfg->gain_h[4], cfg->gain_h[5]);
    fprintf(f, "GAIN_H7=%d\nGAIN_H8=%d\nGAIN_H9=%d\n",
            cfg->gain_h[6], cfg->gain_h[7], cfg->gain_h[8]);
    fprintf(f, "GAIN_V1=%d\nGAIN_V2=%d\nGAIN_V3=%d\n",
            cfg->gain_v[0], cfg->gain_v[1], cfg->gain_v[2]);
    fprintf(f, "GAIN_V4=%d\nGAIN_V5=%d\nGAIN_V6=%d\n",
            cfg->gain_v[3], cfg->gain_v[4], cfg->gain_v[5]);
    fprintf(f, "GAIN_V7=%d\nGAIN_V8=%d\nGAIN_V9=%d\n",
            cfg->gain_v[6], cfg->gain_v[7], cfg->gain_v[8]);
    fprintf(f, "CRWB=%d\n",          cfg->wb_gain_r);
    fprintf(f, "CGWB=%d\n",          cfg->wb_gain_g);
    fprintf(f, "CBWB=%d\n",          cfg->wb_gain_b);
    fprintf(f, "CRDS=%d\n",          cfg->wb_ds_r);
    fprintf(f, "CGDS=%d\n",          cfg->wb_ds_g);
    fprintf(f, "CBDS=%d\n",          cfg->wb_ds_b);
    fprintf(f, "gain_r_2nd=%d\n",    cfg->gain_r_2nd);
    fprintf(f, "gain_g_2nd=%d\n",    cfg->gain_g_2nd);
    fprintf(f, "gain_b_2nd=%d\n",    cfg->gain_b_2nd);
    fprintf(f, "offset_r_2nd=%d\n",  cfg->offset_r_2nd);
    fprintf(f, "offset_g_2nd=%d\n",  cfg->offset_g_2nd);
    fprintf(f, "offset_b_2nd=%d\n",  cfg->offset_b_2nd);
    fprintf(f, "GAM_SEL=%d\n",       cfg->gam_sel);
    fprintf(f, "GammaValue=%.3f\n",  cfg->gamma_value);
    fprintf(f, "RGM10=%d\nRGM11=%d\nRGM12=%d\n",
            cfg->rgm[0][0], cfg->rgm[0][1], cfg->rgm[0][2]);
    fprintf(f, "RGM20=%d\nRGM21=%d\nRGM22=%d\n",
            cfg->rgm[1][0], cfg->rgm[1][1], cfg->rgm[1][2]);
    fprintf(f, "RGM30=%d\nRGM31=%d\nRGM32=%d\n",
            cfg->rgm[2][0], cfg->rgm[2][1], cfg->rgm[2][2]);
    fprintf(f, "RY_COEF=%d\n",       cfg->ry_coef);
    fprintf(f, "GY_COEF=%d\n",       cfg->gy_coef);
    fprintf(f, "BY_COEF=%d\n",       cfg->by_coef);

    if (fclose(f) != 0)
        return IT_E_IO;
    return IT_OK;
}

/* ---- 读取 ---- */
typedef struct {
    ItConfig *cfg;
    int unknown;
} IniCtx;

static void apply_kv(IniCtx *ctx, const char *k, const char *v)
{
    ItConfig *c = ctx->cfg;
    /* 统一钳位：损坏/恶意 INI 的无界整数不得流入定点乘法（int 溢出 UB）*/
    int iv = atoi(v);
    if (iv > 65535)  iv = 65535;
    if (iv < -65535) iv = -65535;
    double dv = atof(v);

    /* 逐键分发：结构完全对应 save_ini 的写出集 */
    if      (!strcmp(k, "PreprocessTh"))   c->preprocess_th = iv;
    else if (!strcmp(k, "PAT_SEL"))        c->pat_sel = (iv >= 0 && iv <= 3)
                                                ? (ItBayerPattern)iv : c->pat_sel;
    else if (!strcmp(k, "DCLP_ON"))        c->dclp_on = iv;
    else if (!strcmp(k, "DCLP_STRT_H"))    c->dclp_strt_h = iv;
    else if (!strcmp(k, "DCLP_STRT_V"))    c->dclp_strt_v = iv;
    else if (!strcmp(k, "POFFSET"))        c->poffset = iv;
    else if (!strcmp(k, "CISGAIN_GR"))     c->cisgain_gr = iv;
    else if (!strcmp(k, "CISGAIN_GB"))     c->cisgain_gb = iv;
    else if (!strcmp(k, "CISGAIN_R"))      c->cisgain_r = iv;
    else if (!strcmp(k, "CISGAIN_B"))      c->cisgain_b = iv;
    else if (!strcmp(k, "POFFSET_GR"))     c->poffset_gr = iv;
    else if (!strcmp(k, "POFFSET_GB"))     c->poffset_gb = iv;
    else if (!strcmp(k, "POFFSET_R"))      c->poffset_r = iv;
    else if (!strcmp(k, "POFFSET_B"))      c->poffset_b = iv;
    else if (!strcmp(k, "BPR_ON"))         c->bpr_on = iv;
    else if (!strcmp(k, "BPR_TH1"))        c->bpr_th1 = iv;
    else if (!strcmp(k, "BPR_TH3"))        c->bpr_th3 = iv;
    else if (!strcmp(k, "BPR_TH5"))        c->bpr_th5 = iv;
    else if (!strcmp(k, "BPR_TH7"))        c->bpr_th7 = iv;
    else if (!strcmp(k, "GRGB_ON"))        c->grgb_on = iv;
    else if (!strcmp(k, "GRGB_GRB"))       c->grgb_grb = iv;
    else if (!strcmp(k, "GRGB_AVG"))       c->grgb_avg = iv;
    else if (!strcmp(k, "GTHR01"))         c->gthr01 = iv;
    else if (!strcmp(k, "RGB_FILTER_ON"))  c->rgb_filter_on = iv;
    else if (!strcmp(k, "RGB_GMEDIAN_ON")) c->rgb_gmedian_on = iv;
    else if (!strcmp(k, "SIG01_CONST"))    c->sig01_const = iv;
    else if (!strcmp(k, "SIG01_MULT"))     c->sig01_mult = iv;
    else if (!strcmp(k, "SIG02_MULT"))     c->sig02_mult = iv;
    else if (!strcmp(k, "SHADE_ON"))       c->shade_on = iv;
    else if (!strcmp(k, "SHADE_RADIAL"))   c->shade_radial = iv;
    else if (!strcmp(k, "SHADE_XC"))       c->shade_xc = iv;
    else if (!strcmp(k, "SHADE_YC"))       c->shade_yc = iv;
    else if (!strncmp(k, "GAIN_H", 6) && k[6] >= '1' && k[6] <= '9' && !k[7])
        c->gain_h[k[6] - '1'] = iv;
    else if (!strncmp(k, "GAIN_V", 6) && k[6] >= '1' && k[6] <= '9' && !k[7])
        c->gain_v[k[6] - '1'] = iv;
    else if (!strcmp(k, "CRWB"))           c->wb_gain_r = iv;
    else if (!strcmp(k, "CGWB"))           c->wb_gain_g = iv;
    else if (!strcmp(k, "CBWB"))           c->wb_gain_b = iv;
    else if (!strcmp(k, "CRDS"))           c->wb_ds_r = iv;
    else if (!strcmp(k, "CGDS"))           c->wb_ds_g = iv;
    else if (!strcmp(k, "CBDS"))           c->wb_ds_b = iv;
    else if (!strcmp(k, "gain_r_2nd"))     c->gain_r_2nd = iv;
    else if (!strcmp(k, "gain_g_2nd"))     c->gain_g_2nd = iv;
    else if (!strcmp(k, "gain_b_2nd"))     c->gain_b_2nd = iv;
    else if (!strcmp(k, "offset_r_2nd"))   c->offset_r_2nd = iv;
    else if (!strcmp(k, "offset_g_2nd"))   c->offset_g_2nd = iv;
    else if (!strcmp(k, "offset_b_2nd"))   c->offset_b_2nd = iv;
    else if (!strcmp(k, "GAM_SEL"))        c->gam_sel = iv;
    else if (!strcmp(k, "GammaValue"))     c->gamma_value = (dv > 0.0) ? dv : c->gamma_value;
    else if (!strcmp(k, "RGM10"))          c->rgm[0][0] = iv;
    else if (!strcmp(k, "RGM11"))          c->rgm[0][1] = iv;
    else if (!strcmp(k, "RGM12"))          c->rgm[0][2] = iv;
    else if (!strcmp(k, "RGM20"))          c->rgm[1][0] = iv;
    else if (!strcmp(k, "RGM21"))          c->rgm[1][1] = iv;
    else if (!strcmp(k, "RGM22"))          c->rgm[1][2] = iv;
    else if (!strcmp(k, "RGM30"))          c->rgm[2][0] = iv;
    else if (!strcmp(k, "RGM31"))          c->rgm[2][1] = iv;
    else if (!strcmp(k, "RGM32"))          c->rgm[2][2] = iv;
    else if (!strcmp(k, "RY_COEF"))        c->ry_coef = iv;
    else if (!strcmp(k, "GY_COEF"))        c->gy_coef = iv;
    else if (!strcmp(k, "BY_COEF"))        c->by_coef = iv;
    else
        ctx->unknown++;
}

ItStatus it_config_load_ini(const char *path, ItConfig *cfg, int *unknown_keys)
{
    if (!path || !cfg)
        return IT_E_PARAM;
    FILE *f = fopen(path, "r");
    if (!f)
        return IT_E_OPEN;

    IniCtx ctx = { cfg, 0 };
    char line[512];
    int in_section = 0;     /* 当前是否处于 [ISP_P1] */
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (isspace((unsigned char)*p)) p++;
        if (*p == ';' || *p == '#' || *p == '\0')
            continue;
        if (*p == '[') {
            char *close = strchr(p, ']');
            /* 精确匹配节名长度，避免 "[]" 因 strncmp(n=0) 误匹配 */
            in_section = close && (close - p - 1) == 6 &&
                strncmp(p + 1, "ISP_P1", 6) == 0;
            continue;
        }
        if (!in_section)
            continue;
        char *eq = strchr(p, '=');
        if (!eq)
            continue;
        *eq = '\0';
        char *key = p;
        char *val = eq + 1;
        /* 去键尾/值尾空白 */
        char *e = key + strlen(key);
        while (e > key && isspace((unsigned char)e[-1])) *--e = '\0';
        while (isspace((unsigned char)*val)) val++;
        e = val + strlen(val);
        while (e > val && isspace((unsigned char)e[-1])) *--e = '\0';
        apply_kv(&ctx, key, val);
    }
    fclose(f);

    if (unknown_keys)
        *unknown_keys = ctx.unknown;
    return IT_OK;
}
