/* raw_loader.c - RAW/Bayer 文件加载实现 */
#include "raw_loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ---- 已知分辨率（.data 节内置表，含 dummy 行/列版本）---- */
static const struct { int w, h; } kKnown[] = {
    {  640,   480 }, {  800,   600 },
    { 1280,  1024 }, { 1288,  1032 },
    { 1600,  1200 }, { 1608,  1208 },
    { 2056,  1544 },
    { 2608,  1952 }, { 2616,  1960 },
};

int it_raw_known_sizes(int *out_w, int *out_h, int max_entries)
{
    int n = (int)(sizeof(kKnown) / sizeof(kKnown[0]));
    if (max_entries < n)
        n = max_entries;
    for (int i = 0; i < n; i++) {
        out_w[i] = kKnown[i].w;
        out_h[i] = kKnown[i].h;
    }
    return n;
}

/* 文件是否看起来是十进制 ASCII 文本（.dat）*/
static int looks_ascii(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return 0;
    unsigned char buf[512];
    size_t n = fread(buf, 1, sizeof(buf), f);
    fclose(f);
    if (n == 0)
        return 0;
    size_t digits = 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = buf[i];
        if (isdigit(c) || c == ' ' || c == '\t' || c == '\r' || c == '\n')
            digits++;
        else
            return 0;   /* 出现任何非数字/空白字节 → 二进制 */
    }
    return digits > 0;
}

ItStatus it_raw_guess_format(const char *path, ItRawFormat *fmt)
{
    if (!path || !fmt)
        return IT_E_PARAM;

    if (fmt->width > 0 && fmt->height > 0)
        return IT_OK;                       /* 已显式指定，无需推断 */

    FILE *f = fopen(path, "rb");
    if (!f)
        return IT_E_OPEN;
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return IT_E_IO; }
    long fsize = ftell(f);
    fclose(f);
    if (fsize <= 0)
        return IT_E_FORMAT;

    if (looks_ascii(path)) {
        fmt->bits = IT_BITS_10;             /* 原程序 .dat 默认 10bit */
        return IT_OK;                       /* ASCII 无法推断尺寸 */
    }

    /* bytes/pixel 依位深候选逐一匹配已知分辨率 */
    int bpp_cand[3] = { 2, 1 };             /* 10/16bit=2字节，8bit=1字节 */
    for (int b = 0; b < 2; b++) {
        long bpp = bpp_cand[b];
        for (size_t i = 0; i < sizeof(kKnown) / sizeof(kKnown[0]); i++) {
            if (kKnown[i].w * (long)kKnown[i].h * bpp == fsize) {
                fmt->width  = kKnown[i].w;
                fmt->height = kKnown[i].h;
                fmt->bits   = (bpp == 1) ? IT_BITS_8 : IT_BITS_10;
                return IT_OK;
            }
        }
    }
    return IT_E_SIZE;
}

/* ASCII 十进制加载（.dat）—— 复刻 0x472040 行为 */
static ItStatus load_ascii_dat(const char *path, const ItRawFormat *fmt,
                               ItImage *img)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return IT_E_OPEN;

    ItStatus st = it_image_alloc(img, fmt->width, fmt->height);
    if (st != IT_OK) { fclose(f); return st; }

    long vmax = (fmt->bits == IT_BITS_8) ? 255 : 1023;  /* Yes→10bit, No→8bit */
    img->full_scale = (int)vmax;
    long total = (long)fmt->width * fmt->height;
    long v;
    long i = 0;
    while (i < total && fscanf(f, "%ld", &v) == 1) {
        if (v < 0)       v = 0;
        if (v > vmax)    v = vmax;
        img->data[i++] = (uint16_t)v;
    }
    fclose(f);
    if (i < total) {
        it_image_free(img);
        return IT_E_SIZE;
    }
    return IT_OK;
}

/* 二进制加载：8bit 直接扩展；10/16bit 按 LH/HL 字节序取低 N 位 */
static ItStatus load_binary(const char *path, const ItRawFormat *fmt,
                            ItImage *img)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return IT_E_OPEN;

    long need = (long)fmt->width * fmt->height * ((fmt->bits == IT_BITS_8) ? 1 : 2);
    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); return IT_E_IO; }
    long fsize = ftell(f);
    if (fsize < need) { fclose(f); return IT_E_SIZE; }
    if (fseek(f, 0, SEEK_SET) != 0) { fclose(f); return IT_E_IO; }

    ItStatus st = it_image_alloc(img, fmt->width, fmt->height);
    if (st != IT_OK) { fclose(f); return st; }

    img->full_scale = (fmt->bits == IT_BITS_8)  ? 255 :
                      (fmt->bits == IT_BITS_10) ? 1023 : 65535;

    long total = (long)fmt->width * fmt->height;

    if (fmt->bits == IT_BITS_8) {
        unsigned char *buf = (unsigned char *)malloc((size_t)total);
        if (!buf) { fclose(f); it_image_free(img); return IT_E_MEMORY; }
        if (fread(buf, 1, (size_t)total, f) != (size_t)total) {
            free(buf); fclose(f); it_image_free(img); return IT_E_IO;
        }
        for (long i = 0; i < total; i++)
            img->data[i] = buf[i];
        free(buf);
    } else {
        unsigned char *buf = (unsigned char *)malloc((size_t)total * 2);
        if (!buf) { fclose(f); it_image_free(img); return IT_E_MEMORY; }
        if (fread(buf, 1, (size_t)total * 2, f) != (size_t)total * 2) {
            free(buf); fclose(f); it_image_free(img); return IT_E_IO;
        }
        unsigned mask = (fmt->bits == IT_BITS_10) ? 0x03FFu : 0xFFFFu;
        for (long i = 0; i < total; i++) {
            unsigned b0 = buf[2 * i];           /* 文件第 1 字节 */
            unsigned b1 = buf[2 * i + 1];       /* 文件第 2 字节 */
            unsigned v;
            if (fmt->order == IT_BYTEORDER_HL)  /* HL：高字节在前 */
                v = (b0 << 8) | b1;
            else                                /* LH：低字节在前 */
                v = (b1 << 8) | b0;
            img->data[i] = (uint16_t)(v & mask);
        }
        free(buf);
    }
    fclose(f);
    return IT_OK;
}

ItStatus it_raw_load(const char *path, const ItRawFormat *fmt,
                     int prefer_ascii, ItImage *img)
{
    if (!path || !fmt || !img || fmt->width <= 0 || fmt->height <= 0)
        return IT_E_PARAM;
    if (fmt->bits != IT_BITS_8 && fmt->bits != IT_BITS_10 && fmt->bits != IT_BITS_16)
        return IT_E_PARAM;
    if (fmt->dummy_rows < 0 || fmt->dummy_cols < 0 ||
        fmt->dummy_rows >= fmt->height || fmt->dummy_cols >= fmt->width)
        return IT_E_PARAM;

    if (prefer_ascii || looks_ascii(path))
        return load_ascii_dat(path, fmt, img);

    ItImage full;
    ItStatus st = load_binary(path, fmt, &full);
    if (st != IT_OK)
        return st;

    if (fmt->dummy_rows == 0 && fmt->dummy_cols == 0) {
        *img = full;                        /* 无 dummy：直接交付 */
        return IT_OK;
    }

    /* 中心对称裁剪 dummy 边界 */
    int ew = fmt->width  - fmt->dummy_cols;
    int eh = fmt->height - fmt->dummy_rows;
    int ox = fmt->dummy_cols / 2;
    int oy = fmt->dummy_rows / 2;
    st = it_image_alloc(img, ew, eh);
    if (st != IT_OK) { it_image_free(&full); return st; }
    for (int y = 0; y < eh; y++)
        memcpy(img->data + (long)y * ew,
               full.data + (long)(y + oy) * fmt->width + ox,
               (size_t)ew * sizeof(uint16_t));
    it_image_free(&full);
    return IT_OK;
}
