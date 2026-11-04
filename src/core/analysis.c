/* analysis.c - 统计分析实现（输出格式复刻原 hist_summary.txt，见 0x4338b0）*/
#include "analysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* 单通道累加器 */
typedef struct {
    double sum, sum2;
    long   mx, mn, n;
} Acc;

ItStatus it_hist_compute(const ItImage *img, int bits, long hist[256])
{
    if (!img || !img->data || !hist)
        return IT_E_PARAM;
    if (bits != 8 && bits != 10 && bits != 16)
        return IT_E_PARAM;
    memset(hist, 0, 256 * sizeof(long));
    long n = (long)img->width * img->height;
    for (long i = 0; i < n; i++)
        hist[img->data[i] >> (bits - 8)]++;   /* 折叠到 256 级 */
    return IT_OK;
}

static void acc_init(Acc *a)
{
    a->sum = a->sum2 = 0.0;
    a->mx = 0;
    a->mn = 0x7FFFFFFF;
    a->n  = 0;
}

static void acc_add(Acc *a, long v)
{
    a->sum  += (double)v;
    a->sum2 += (double)v * v;
    if (v > a->mx) a->mx = v;
    if (v < a->mn) a->mn = v;
    a->n++;
}

static void acc_finish(const Acc *a, double *mean, double *std,
                       double *snr, long *mx, long *mn)
{
    if (a->n == 0) {
        *mean = *std = *snr = 0.0;
        *mx = *mn = 0;
        return;
    }
    double m = a->sum / a->n;
    double var = a->sum2 / a->n - m * m;
    if (var < 0.0) var = 0.0;
    double s = sqrt(var);
    *mean = m;
    *std  = s;
    *snr  = (s > 0.0) ? (m / s) : 0.0;
    *mx = a->mx;
    *mn = a->mn;
}

ItStatus it_stat_compute(const ItRgb *rgb, const ItImage *ymask, ItStat *stat)
{
    if (!rgb || !rgb->data || !stat)
        return IT_E_PARAM;
    if (ymask && (ymask->width != rgb->width || ymask->height != rgb->height))
        return IT_E_SIZE;

    int w = rgb->width, h = rgb->height;
    Acc acc[4];                 /* 0=Y 1=R 2=G 3=B（原输出列序）*/
    for (int c = 0; c < 4; c++)
        acc_init(&acc[c]);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (ymask && ymask->data[(long)y * w + x] == 0)
                continue;       /* 0 像素 = 排除区（Analysis 页掩码）*/
            const uint8_t *p = &rgb->data[((long)y * w + x) * 3];
            long ly = (299L * p[0] + 587L * p[1] + 114L * p[2]) / 1000;
            acc_add(&acc[0], ly);
            acc_add(&acc[1], p[0]);
            acc_add(&acc[2], p[1]);
            acc_add(&acc[3], p[2]);
        }
    }

    stat->total = acc[0].n;
    acc_finish(&acc[0], &stat->mean[0], &stat->std[0], &stat->snr[0],
               &stat->maxv[0], &stat->minv[0]);
    for (int c = 1; c < 4; c++)
        acc_finish(&acc[c], &stat->mean[c], &stat->std[c], &stat->snr[c],
                   &stat->maxv[c], &stat->minv[c]);
    return IT_OK;
}

ItStatus it_stat_write_summary(const char *path, const char *image_name,
                               const ItStat *stat)
{
    if (!path || !stat)
        return IT_E_PARAM;

    FILE *f = fopen(path, "w");
    if (!f)
        return IT_E_OPEN;

    time_t now = time(NULL);
    char date[32] = "?", tim[32] = "?";
    {
        struct tm *tmv = localtime(&now);
        if (tmv) {
            strftime(date, sizeof(date), "%b %d %Y", tmv);
            strftime(tim,  sizeof(tim),  "%H:%M:%S", tmv);
        }
    }

    fprintf(f, "-----------------------------------------------\n");
    fprintf(f, "  Histogram Summary\n");
    fprintf(f, "  File Name : %s\n", image_name ? image_name : "-");
    fprintf(f, "  Time  : %s  %s\n", date, tim);
    fprintf(f, "-----------------------------------------------\n");
    fprintf(f, "        Y       R       G       B\n");
    fprintf(f, " Mean : %6.2f  %6.2f  %6.2f  %6.2f\n",
            stat->mean[0], stat->mean[1], stat->mean[2], stat->mean[3]);
    fprintf(f, " Std  : %6.2f  %6.2f  %6.2f  %6.2f\n",
            stat->std[0], stat->std[1], stat->std[2], stat->std[3]);
    fprintf(f, " SNR  : %6.2f  %6.2f  %6.2f  %6.2f\n",
            stat->snr[0], stat->snr[1], stat->snr[2], stat->snr[3]);
    fprintf(f, " Max  : %ld   %ld   %ld   %ld\n",
            stat->maxv[0], stat->maxv[1], stat->maxv[2], stat->maxv[3]);
    fprintf(f, " Min  : %ld   %ld   %ld   %ld\n",
            stat->minv[0], stat->minv[1], stat->minv[2], stat->minv[3]);
    fprintf(f, "-----------------------------------------------\n");
    fprintf(f, "  Total Pixel : %ld\n", stat->total);
    fprintf(f, "-----------------------------------------------\n");
    fclose(f);
    return IT_OK;
}

ItStatus it_stat_write_csv(const char *path, const char *image_name,
                           const ItStat *stat)
{
    if (!path || !stat)
        return IT_E_PARAM;

    FILE *f = fopen(path, "w");
    if (!f)
        return IT_E_OPEN;

    fprintf(f, "Image,%s\n", image_name ? image_name : "-");
    fprintf(f, "TotalPixel,%ld\n", stat->total);
    fprintf(f, "Stat,Y,R,G,B\n");
    fprintf(f, "Mean,%.2f,%.2f,%.2f,%.2f\n",
            stat->mean[0], stat->mean[1], stat->mean[2], stat->mean[3]);
    fprintf(f, "Std,%.2f,%.2f,%.2f,%.2f\n",
            stat->std[0], stat->std[1], stat->std[2], stat->std[3]);
    fprintf(f, "SNR,%.2f,%.2f,%.2f,%.2f\n",
            stat->snr[0], stat->snr[1], stat->snr[2], stat->snr[3]);
    fprintf(f, "Max,%ld,%ld,%ld,%ld\n",
            stat->maxv[0], stat->maxv[1], stat->maxv[2], stat->maxv[3]);
    fprintf(f, "Min,%ld,%ld,%ld,%ld\n",
            stat->minv[0], stat->minv[1], stat->minv[2], stat->minv[3]);

    fclose(f);
    return IT_OK;
}
