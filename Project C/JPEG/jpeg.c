#include "jpeg.h"
#include "dct.h"

#include <math.h>
#include <stdlib.h>

static const int Q[8][8] = {
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

static double get_gray(const Image *img, int x, int y) {
    if (x >= img->width) x = img->width - 1;
    if (y >= img->height) y = img->height - 1;

    int id = (y * img->width + x) * 3;
    double r = img->data[id + 0];
    double g = img->data[id + 1];
    double b = img->data[id + 2];

    return 0.299 * r + 0.587 * g + 0.114 * b;
}

int jpeg_like_compress_with_stats(const Image *src, Image *dst, int quality_scale, JpegStats *stats) {
    if (!src || !src->data) return 1;
    if (quality_scale < 1) quality_scale = 1;

    if (stats) {
        stats->total_coeffs = 0;
        stats->nonzero_coeffs = 0;
        stats->estimated_bytes = 0;
    }

    dst->width = src->width;
    dst->height = src->height;
    dst->data = (unsigned char *)malloc(dst->width * dst->height * 3);
    if (!dst->data) return 1;

    for (int by = 0; by < src->height; by += 8) {
        for (int bx = 0; bx < src->width; bx += 8) {
            double block[8][8];
            double coeffs[8][8];
            double restored[8][8];

            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    block[i][j] = get_gray(src, bx + j, by + i) - 128.0;
                }
            }

            dct_8x8(block, coeffs);

            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int q = Q[i][j] * quality_scale;
                    coeffs[i][j] = round(coeffs[i][j] / q) * q;

                    if (stats) {
                        stats->total_coeffs++;
                        if (coeffs[i][j] != 0.0) {
                            stats->nonzero_coeffs++;
                        }
                    }
                }
            }

            idct_8x8(coeffs, restored);

            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int x = bx + j;
                    int y = by + i;

                    if (x < src->width && y < src->height) {
                        unsigned char v = clamp_to_byte(restored[i][j] + 128.0);
                        int id = (y * dst->width + x) * 3;

                        dst->data[id + 0] = v;
                        dst->data[id + 1] = v;
                        dst->data[id + 2] = v;
                    }
                }
            }
        }
    }

    if (stats) {
        stats->estimated_bytes = stats->nonzero_coeffs * (long)sizeof(short);
    }

    return 0;
}

int jpeg_like_compress(const Image *src, Image *dst, int quality_scale) {
    return jpeg_like_compress_with_stats(src, dst, quality_scale, NULL);
}
