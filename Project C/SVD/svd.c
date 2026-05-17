#include "svd.h"                 
#include "jacobi.h"              
#include "../matrix/matrix.h"    

#include <math.h>             
#include <stdlib.h>             

typedef struct {  // структуру для хранения пары значений
    double value; // значение сингулярного числа
    int index; // индекс соответствующего собственного вектора
} Pair;                         

static int cmp_pair_desc(const void *a, const void *b) { // функция сравнения для сортировки по убыванию
    double x = ((const Pair *)a)->value; // получаю значение первого элемента
    double y = ((const Pair *)b)->value; // получаю значение второго элемента

    if (x < y) return 1;   // если первое меньше второго, ставлю его позже
    if (x > y) return -1; // если первое больше второго, ставлю его раньше
    return 0;  // если значения равны, порядок не меняю
}                                                     

static double gray_pixel(const Image *img, int x, int y) { // функция для получения яркости пикселя
    int id = (y * img->width + x) * 3; // считаю индекс пикселя в массиве RGB
    double r = img->data[id + 0];  // беру красную компоненту пикселя
    double g = img->data[id + 1]; // берузелёную компоненту пикселя
    double b = img->data[id + 2];  // беру синюю компоненту пикселя

    return 0.299 * r + 0.587 * g + 0.114 * b; // возвращаем яркость пикселя по стандартной формуле
}                                                         

int svd_compress_grayscale(const Image *src, Image *dst, int k) { // функция SVD-сжатия изображения в оттенках серого
    if (!src || !src->data) return 1; // проверяю, что исходное изображение существует

    int h = src->height; // сохраняю высоту изображения
    int w = src->width; // сохраняю ширину изображения

    if (w > 160 || h > 160) { // проверяю ограничение размера изображения
        return 1;   // если изображение слишком большое, возвращаю ошибку
    }                                                            
    if (k < 1) k = 1; // если k меньше 1, делаю его равным 1
    if (k > w) k = w;  // если k больше ширины, ограничиваю его шириной изображения

    double **A = matrix_alloc(h, w); // выделяю матрицу A для яркости исходного изображения
    double **B = matrix_alloc(w, w);  // выделяю матрицу B = A^T * A
    double **V = matrix_alloc(w, w); // выделяю матрицу V для собственных векторов
    double *eig = (double *)calloc(w, sizeof(double)); // выделяю массив для собственных значений
    Pair *pairs = (Pair *)calloc(w, sizeof(Pair)); // выделяю массив пар сингулярное число + индекс

    if (!A || !B || !V || !eig || !pairs) {// проверяю, что вся память выделилась успешно
        matrix_free(A, h);                                        
        matrix_free(B, w);                                       
        matrix_free(V, w);
        free(eig);                                                
        free(pairs);                                             
        return 1;                                                
    }                                          
    
    for (int y = 0; y < h; y++) { // по всем строкам изображения
        for (int x = 0; x < w; x++) {  // по всем столбцам изображения
            A[y][x] = gray_pixel(src, x, y); // записываю яркость пикселя в матрицу A
        }                                                       
    }                                                            

    for (int i = 0; i < w; i++) { //по строкам матрицы B
        for (int j = 0; j < w; j++) { ///по столбцам матрицы B
            double sum = 0.0;// создаю переменную для суммы произведений

            for (int y = 0; y < h; y++) { //по строкам матрицы A
                sum += A[y][i] * A[y][j]; // считаю элемент матрицы A^T * A
            }                                                    

            B[i][j] = sum; // записываю найденное значение в матрицу B
        }                                                        
    }                                                            

    jacobi_eigen(B, w, eig, V);// нахожу собственные значения и собственные векторы матрицы B

    for (int i = 0; i < w; i++) {  // проходжу по всем собственным значениям
        pairs[i].value = eig[i] > 0.0 ? sqrt(eig[i]) : 0.0; // получаю сингулярное число как корень из собственного значения
        pairs[i].index = i; // сохраняю индекс собственного вектора
    }                                                           

    qsort(pairs, w, sizeof(Pair), cmp_pair_desc);// сортирую сингулярные числа по убыванию

    dst->width = w; // задаю ширину выходного изображения
    dst->height = h; // задаю высоту выходного изображения
    dst->data = (unsigned char *)calloc(w * h * 3, 1); // выделяю память под RGB-данные выходного изображения

    if (!dst->data) { // проверяю, что память под выходное изображение выделилась
        matrix_free(A, h);                                        
        matrix_free(B, w);                                        
        matrix_free(V, w);                                        
        free(eig);                                                
        free(pairs);                                              
        return 1;                                                 
    }                                                            
    for (int t = 0; t < k; t++) {  // беру только первые k сингулярных значений
        int col = pairs[t].index; // получаю индекс нужного собственного вектора
        double sigma = pairs[t].value;// получаю текущее сингулярное число

        if (sigma < 1e-10) continue; // если сингулярное число почти нулевое, пропускаю его

        double *u = (double *)calloc(h, sizeof(double));// выделяю память под левый сингулярный вектор u
        if (!u) continue; // если память не выделилась, пропускаю эту компоненту

        for (int y = 0; y < h; y++) {  // проходю по строкам изображения
            double sum = 0.0;  // создаю переменную для суммы

            for (int x = 0; x < w; x++) {// прохожу по столбцам изображения
                sum += A[y][x] * V[x][col];  // считаю произведение A на выбранный вектор V
            }                                                    

            u[y] = sum / sigma;  // получаю элемент левого сингулярного вектора u
        }                                                       

        // прохожу по выходному изображению
        for (int y = 0; y < h; y++) {//  по строкам
            for (int x = 0; x < w; x++) { //по столбцам 
                int id = (y * w + x) * 3; // считаю индекс пикселя в RGB-массиве
                double add = sigma * u[y] * V[x][col]; // считаю вклад текущей SVD-компоненты
                double old = dst->data[id];  // беру старое значение пикселя
                unsigned char val = clamp_to_byte(old + add); // ограничиваю новое значение диапазоном 0..255

                dst->data[id + 0] = val;  // записываю значение в красный канал
                dst->data[id + 1] = val; // записываю значение в зелёный канал
                dst->data[id + 2] = val; // записываю значение в синий канал
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
