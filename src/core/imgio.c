/* imgio.c - BMP/PPM/RAW 写出实现 */
#include "imgio.h"
#include <stdio.h>
#include <string.h>

/* 显式小端字节序列化：不依赖结构体打包布局（避免非 pack 构建下
 * BMP 文件头字段错位）。 */
static int w16(FILE *f, unsigned v)
{
    unsigned char b[2] = { (unsigned char)(v & 0xFF), (unsigned char)(v >> 8) };
    return fwrite(b, 1, 2, f) == 2 ? 0 : -1;
}

static int w32(FILE *f, unsigned long v)
{
    unsigned char b[4] = {
        (unsigned char)(v & 0xFF),         (unsigned char)((v >> 8) & 0xFF),
        (unsigned char)((v >> 16) & 0xFF), (unsigned char)((v >> 24) & 0xFF)
    };
    return fwrite(b, 1, 4, f) == 4 ? 0 : -1;
}

ItStatus it_bmp_write(const char *path, const ItRgb *rgb)
{
    if (!path || !rgb || !rgb->data)
        return IT_E_PARAM;

    long rowsz = ((long)rgb->width * 3 + 3) & ~3L;
    long datasz = rowsz * rgb->height;
    unsigned long offbits = 14 + 40;
    unsigned long filesize = offbits + (unsigned long)datasz;

    FILE *f = fopen(path, "wb");
    if (!f)
        return IT_E_OPEN;

    /* BITMAPFILEHEADER（14 字节）*/
    int err = 0;
    err |= w16(f, 0x4D42);          /* 'BM' */
    err |= w32(f, filesize);
    err |= w16(f, 0);               /* 保留 */
    err |= w16(f, 0);               /* 保留 */
    err |= w32(f, offbits);
    /* BITMAPINFOHEADER（40 字节）*/
    err |= w32(f, 40);
    err |= w32(f, (unsigned long)rgb->width);
    err |= w32(f, (unsigned long)rgb->height);
    err |= w16(f, 1);               /* planes */
    err |= w16(f, 24);              /* bitcount */
    err |= w32(f, 0);               /* BI_RGB */
    err |= w32(f, (unsigned long)datasz);
    err |= w32(f, 2835);            /* 72 DPI */
    err |= w32(f, 2835);
    err |= w32(f, 0);               /* 颜色数 */
    err |= w32(f, 0);               /* 重要颜色 */
    if (err) {
        fclose(f);
        return IT_E_IO;
    }

    /* 像素：自底向上、BGR 序、行 4 字节对齐 */
    unsigned char pad[3] = { 0, 0, 0 };
    for (int y = rgb->height - 1; y >= 0; y--) {
        for (int x = 0; x < rgb->width; x++) {
            const uint8_t *p = &rgb->data[((long)y * rgb->width + x) * 3];
            unsigned char bgr[3] = { p[2], p[1], p[0] };
            if (fwrite(bgr, 1, 3, f) != 3) { fclose(f); return IT_E_IO; }
        }
        long padn = rowsz - (long)rgb->width * 3;
        if (padn > 0 && fwrite(pad, 1, (size_t)padn, f) != (size_t)padn) {
            fclose(f);
            return IT_E_IO;
        }
    }
    fclose(f);
    return IT_OK;
}

ItStatus it_ppm_gray_write(const char *path, const ItImage *img, int maxval)
{
    if (!path || !img || !img->data || maxval <= 0 || maxval > 65535)
        return IT_E_PARAM;
    FILE *f = fopen(path, "wb");
    if (!f)
        return IT_E_OPEN;
    fprintf(f, "P2\n%d %d\n%d\n", img->width, img->height, maxval);
    for (long i = 0; i < (long)img->width * img->height; i++) {
        unsigned v = img->data[i] > (unsigned)maxval ? (unsigned)maxval
                                                     : img->data[i];
        fprintf(f, "%u%c", v, ((i + 1) % img->width == 0) ? '\n' : ' ');
    }
    fclose(f);
    return IT_OK;
}

ItStatus it_ppm_rgb_write(const char *path, const ItRgb *rgb)
{
    if (!path || !rgb || !rgb->data)
        return IT_E_PARAM;
    FILE *f = fopen(path, "wb");
    if (!f)
        return IT_E_OPEN;
    fprintf(f, "P3\n%d %d\n255\n", rgb->width, rgb->height);
    for (long i = 0; i < (long)rgb->width * rgb->height * 3; i++)
        fprintf(f, "%u%c", rgb->data[i],
                ((i / 3 + 1) % rgb->width == 0 && (i % 3) == 2) ? '\n' : ' ');
    fclose(f);
    return IT_OK;
}

ItStatus it_raw16_write(const char *path, const ItImage *img)
{
    if (!path || !img || !img->data)
        return IT_E_PARAM;
    FILE *f = fopen(path, "wb");
    if (!f)
        return IT_E_OPEN;
    size_t n = (size_t)img->width * img->height;
    if (fwrite(img->data, sizeof(uint16_t), n, f) != n) {
        fclose(f);
        return IT_E_IO;
    }
    fclose(f);
    return IT_OK;
}
