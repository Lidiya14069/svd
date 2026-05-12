#include "svd.h"
#include "jacobi.h"
#include "../matrix/matrix.h"

#include <math.h>
#include <stdlib.h>

typedef struct {
    double value;
    int index;
} Pair;

static int cmp_pair_desc(const void *a, const void *b) {
    double x = ((const Pair *)a) -> value;
    double y = ((const Pair *)b) -> value;

    if (x < y) return 1;
    if (x > y) return -1;
    return 0;
}

static double gray_pixel(const Image *img, int x, int y) {
    int id = (y * img -> width + x) * 3;
    double r = img -> data[id + 0];
    double g = img -> data[id + 1];
    double b = img -> data[id + 2];

    return 0.299 * r + 0.587 * g + 0.114 * b;
}

int svd_compress_grayscale(const Image *src, Image *dst, int k) {
    if (!src || !src -> data) return 1;

    int h = src -> height;
    int w = src -> width;

    if (w > 160 || h > 160) {
        return 1;
    }

    if (k < 1) k = 1;
    if (k > w) k = w;

    double **A = matrix_alloc(h, w);
    double **B = matrix_alloc(w, w);
    double **V = matrix_alloc(w, w);
    double *eig = (double *) calloc (w, sizeof(double));
    Pair *pairs = (Pair *) calloc (w, sizeof(Pair));

    if (!A || !B || !V || !eig || !pairs) {
        matrix_free(A, h);
        matrix_free(B, w);
        matrix_free(V, w);
        free(eig);
        free(pairs);
        return 1;
    }

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            A[y][x] = gray_pixel(src, x, y);
        }
    }

    for (int i = 0; i < w; i++) {
        for (int j = 0; j < w; j++) {
            double sum = 0.0;

            for (int y = 0; y < h; y++) {
                sum += A[y][i] * A[y][j];
            }

            B[i][j] = sum;
        }
    }

    jacobi_eigen(B, w, eig, V);

    for (int i = 0; i < w; i++) {
        pairs[i].value = eig[i] > 0.0 ? sqrt(eig[i]) : 0.0;
        pairs[i].index = i;
    }

    qsort(pairs, w, sizeof(Pair), cmp_pair_desc);

    dst -> width = w;
    dst -> height = h;
    dst -> data = (unsigned char *) calloc (w * h * 3, 1);

    if (!dst -> data) {
        matrix_free(A, h);
        matrix_free(B, w);
        matrix_free(V, w);
        free(eig);
        free(pairs);
        return 1;
    }

    for (int t = 0; t < k; t++) {
        int col = pairs[t].index;
        double sigma = pairs[t].value;

        if (sigma < 1e-10) continue;

        double *u = (double *) calloc (h, sizeof(double));
        if (!u) continue;

        for (int y = 0; y < h; y++) {
            double sum = 0.0;

            for (int x = 0; x < w; x++) {
                sum += A[y][x] * V[x][col];
            }

            u[y] = sum / sigma;
        }

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                int id = (y * w + x) * 3;
                double add = sigma * u[y] * V[x][col];
                double old = dst -> data[id];
                unsigned char val = clamp_to_byte(old + add);

                dst->data[id + 0] = val;
                dst->data[id + 1] = val;
                dst->data[id + 2] = val;
            }
        }

        free(u);
    }

    matrix_free(A, h);
    matrix_free(B, w);
    matrix_free(V, w);
    free(eig);
    free(pairs);
    return 0;
}
