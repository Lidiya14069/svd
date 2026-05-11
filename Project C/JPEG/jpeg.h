#ifndef JPEG_H
#define JPEG_H

#include "../image/image.h"

typedef struct {
    long total_coeffs;
    long nonzero_coeffs;
    long estimated_bytes;
} JpegStats;

int jpeg_like_compress(const Image *src, Image *dst, int quality_scale);
int jpeg_like_compress_with_stats(const Image *src, Image *dst, int quality_scale, JpegStats *stats);

#endif
