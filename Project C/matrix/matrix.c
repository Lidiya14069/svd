#include "matrix.h"

#include <stdlib.h>

double **matrix_alloc(int rows, int cols) {
    double **a = (double **)malloc(rows * sizeof(double *));
    if (!a) return NULL;

    for (int i = 0; i < rows; i++) {
        a[i] = (double *)calloc(cols, sizeof(double));
        if (!a[i]) {
            for (int j = 0; j < i; j++) {
                free(a[j]);
            }
            free(a);
            return NULL;
        }
    }

    return a;
}

void matrix_free(double **a, int rows) {
    if (!a) return;

    for (int i = 0; i < rows; i++) {
        free(a[i]);
    }

    free(a);
}

void matrix_identity(double **a, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            a[i][j] = i == j ? 1.0 : 0.0;
        }
    }
}
