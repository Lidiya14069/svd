#include "jacobi.h"
#include "../matrix/matrix.h"

#include <math.h>

int jacobi_eigen(double **a, int n, double *eigenvalues, double **eigenvectors) {
    if (!a || !eigenvalues || !eigenvectors || n <= 0) {
        return 1;
    }

    matrix_identity(eigenvectors, n);

    if (n == 1) {
        eigenvalues[0] = a[0][0];
        return 0;
    }

    int max_iter = 100 * n * n;
    double eps = 1e-8;

    for (int iter = 0; iter < max_iter; iter++) {
        int p = 0;
        int q = 1;
        double max_val = fabs(a[p][q]);

        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                if (fabs(a[i][j]) > max_val) {
                    max_val = fabs(a[i][j]);
                    p = i;
                    q = j;
                }
            }
        }

        if (max_val < eps) break;

        double phi = 0.5 * atan2(2.0 * a[p][q], a[q][q] - a[p][p]);
        double c = cos(phi);
        double s = sin(phi);

        double app = c * c * a[p][p] - 2 * s * c * a[p][q] + s * s * a[q][q];
        double aqq = s * s * a[p][p] + 2 * s * c * a[p][q] + c * c * a[q][q];

        a[p][p] = app;
        a[q][q] = aqq;
        a[p][q] = 0.0;
        a[q][p] = 0.0;

        for (int k = 0; k < n; k++) {
            if (k != p && k != q) {
                double akp = a[k][p];
                double akq = a[k][q];

                a[k][p] = c * akp - s * akq;
                a[p][k] = a[k][p];

                a[k][q] = s * akp + c * akq;
                a[q][k] = a[k][q];
            }
        }

        for (int k = 0; k < n; k++) {
            double vkp = eigenvectors[k][p];
            double vkq = eigenvectors[k][q];

            eigenvectors[k][p] = c * vkp - s * vkq;
            eigenvectors[k][q] = s * vkp + c * vkq;
        }
    }

    for (int i = 0; i < n; i++) {
        eigenvalues[i] = a[i][i];
    }

    return 0;
}
