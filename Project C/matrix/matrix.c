#include "matrix.h" 

#include <stdlib.h> 

double **matrix_alloc(int rows, int cols) { // функция выделяет память под матрицу размера rows x cols
    double **a = (double **)malloc(rows * sizeof(double *)); // выделяю память под массив указателей на строки
    if (!a) return NULL; // если память не выделилась, возвращаю NULL

    for (int i = 0; i < rows; i++) { // прохожу по всем строкам матрицы
        a[i] = (double *)calloc(cols, sizeof(double)); // выделяю память под одну строку и заполняю её нулями
        if (!a[i]) { // если память под строку не выделилась
            for (int j = 0; j < i; j++) { // прохожу по уже выделенным строкам
                free(a[j]);
            } 
            free(a);
            return NULL; 
        } 
    } 
    
    return a; 
} 

void matrix_free(double **a, int rows) { // функция освобождает память, выделенную под матрицу
    if (!a) return; // если указатель пустой, ничего не делаю

    for (int i = 0; i < rows; i++) { // прохожу по всем строкам матрицы
        free(a[i]);
    }

    free(a); 
} 

void matrix_identity(double **a, int n) { // функция делает матрицу единичной размера n x n
    for (int i = 0; i < n; i++) { // прохожу по строкам матрицы
        for (int j = 0; j < n; j++) { // прохожу по столбцам матрицы
            a[i][j] = i == j ? 1.0 : 0.0; // на главной диагонали ставлю 1, в остальных местах 0
        } 
    }
} 
