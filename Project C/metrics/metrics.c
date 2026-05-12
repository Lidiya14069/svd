#include "metrics.h"

#include <math.h>

static double gray_at(const Image *img, int i) {
    double r = img->data[i * 3 + 0];
    double g = img->data[i * 3 + 1];
    double b = img->data[i * 3 + 2];

    return 0.299 * r + 0.587 * g + 0.114 * b;
}

double psnr(const Image *a, const Image *b) {
    if (a->width != b->width || a->height != b->height) return 0.0;

    int n = a->width * a->height;
    if (n <= 0) return 0.0;

    double mse = 0.0;

    for (int i = 0; i < n; i++) {
        double da = gray_at(a, i);
        double db = gray_at(b, i);
        double d = da - db;
        mse += d * d;
    }

    mse /= n;
    if (mse == 0.0) return 99.0;

    return 10.0 * log10((255.0 * 255.0) / mse);
}

double ssim_simple(const Image *a, const Image *b) {
    if (a->width != b->width || a->height != b->height) return 0.0;

    int n = a->width * a->height;
    if (n <= 0) return 0.0;

    double mean_a = 0.0;
    double mean_b = 0.0;

    for (int i = 0; i < n; i++) {
        mean_a += gray_at(a, i);
        mean_b += gray_at(b, i);
    }

    mean_a /= n;
    mean_b /= n;

    if (n == 1) {
        double c1 = 6.5025;
        return (2 * mean_a * mean_b + c1) / (mean_a * mean_a + mean_b * mean_b + c1);
    }

    double var_a = 0.0;
    double var_b = 0.0;
    double cov = 0.0;

    for (int i = 0; i < n; i++) {
        double da = gray_at(a, i) - mean_a;
        double db = gray_at(b, i) - mean_b; 

        var_a += da * da;
        var_b += db * db;
        cov += da * db;
    }

    var_a /= n - 1;
    var_b /= n - 1;
    cov /= n - 1;

    double c1 = 6.5025;
    double c2 = 58.5225;

    return ((2 * mean_a * mean_b + c1) * (2 * cov + c2)) / ((mean_a * mean_a + mean_b * mean_b + c1) * (var_a + var_b + c2));
}
