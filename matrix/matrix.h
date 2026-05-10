#ifndef MATRIX_H
#define MATRIX_H

double **matrix_alloc(int rows, int cols);
void matrix_free(double **a, int rows);
void matrix_identity(double **a, int n);

#endif
