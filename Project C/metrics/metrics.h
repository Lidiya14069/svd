#ifndef METRICS_H
#define METRICS_H

#include "../image/image.h"

double psnr(const Image *a, const Image *b);
double ssim_simple(const Image *a, const Image *b);

#endif
