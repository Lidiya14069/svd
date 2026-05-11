#include "dct.h"

#include <math.h>

#define PI 3.14159265358979323846

static double alpha(int u) {
    return u == 0 ? 1.0 / sqrt(2.0) : 1.0;
}

void dct_8x8(double input[8][8], double output[8][8]) {
    for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {
            double sum = 0.0;

            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    sum += input[x][y] *
                           cos(((2 * x + 1) * u * PI) / 16.0) *
                           cos(((2 * y + 1) * v * PI) / 16.0);
                }
            }

            output[u][v] = 0.25 * alpha(u) * alpha(v) * sum;
        }
    }
}

void idct_8x8(double input[8][8], double output[8][8]) {
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            double sum = 0.0;

            for (int u = 0; u < 8; u++) {
                for (int v = 0; v < 8; v++) {
                    sum += alpha(u) * alpha(v) * input[u][v] *
                           cos(((2 * x + 1) * u * PI) / 16.0) *
                           cos(((2 * y + 1) * v * PI) / 16.0);
                }
            }

            output[x][y] = 0.25 * sum;
        }
    }
}
