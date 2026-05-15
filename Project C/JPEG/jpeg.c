#include "jpeg.h"
#include "dct.h"

#include <math.h>
#include <stdlib.h>

static const int Q[8][8] = { // таблица квантования jpeg
    {16, 11, 10, 16, 24, 40, 51, 61},
    {12, 12, 14, 19, 26, 58, 60, 55},
    {14, 13, 16, 24, 40, 57, 69, 56},
    {14, 17, 22, 29, 51, 87, 80, 62},
    {18, 22, 37, 56, 68, 109, 103, 77},
    {24, 35, 55, 64, 81, 104, 113, 92},
    {49, 64, 78, 87, 103, 121, 120, 101},
    {72, 92, 95, 98, 112, 100, 103, 99}
};

static double get_gray(const Image *img, int x, int y) { // функ получения серого цвета из пикселя
    if (x >= img->width) x = img->width - 1; // если х вышел за границу изображения
    if (y >= img->height) y = img->height - 1; // если у вышел

    int id = (y * img->width + x) * 3; // индекс пикселя
    double r = img->data[id + 0]; // читаб краснный канал 
    double g = img->data[id + 1];//  зеленый канал 
    double b = img->data[id + 2]; // синий канал 

    return 0.299 * r + 0.587 * g + 0.114 * b; // перевод rgb в оттенок серого 
}

// функ jpeg подобного сжатия со статистикой
int jpeg_like_compress_with_stats(const Image *src, Image *dst, int quality_scale, JpegStats *stats) {
    if (!src || !src->data) return 1; //  проверка изображенрия
    if (quality_scale < 1) quality_scale = 1; // минимальное качество 

    if (stats) { // если статистика существует 
        stats->block_count = 0; // количество блоков 
        stats->total_coeffs = 0; // общее число коэффицентов
        stats->nonzero_coeffs = 0; // число ненулевых коэф
        stats->estimated_bytes = 0; //примерный размер 
    }

    dst->width = src->width; // копирую ширину
    dst->height = src->height; // высоту
    dst->data = (unsigned char *)malloc(dst->width * dst->height * 3); // выделяю память ппод результат
    if (!dst->data) return 1; // если память не выделились

    for (int by = 0; by < src->height; by += 8) { //  по блокам у
        for (int bx = 0; bx < src->width; bx += 8) { // по блокам х 
            double block[8][8]; //  исходный блок
            double coeffs[8][8]; // коэфф dct
            double restored[8][8]; // востанновленный блок

            // увеличиваю число блоков
            if (stats) { 
                stats->block_count++;
            }

            // читаю блок 8*8 
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    block[i][j] = get_gray(src, bx + j, by + i) - 128.0; // беру серый цвет и центрирую
                }
            }

            dct_8x8(block, coeffs); // применяю dct 

            // квантование коэффицентов
            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int q = Q[i][j] * quality_scale;
                    coeffs[i][j] = round(coeffs[i][j] / q) * q;

                    if (stats) {
                        stats->total_coeffs++;
                        if (coeffs[i][j] != 0.0) {
                            stats->nonzero_coeffs++;
                        }
                    }
                }
            }

            idct_8x8(coeffs, restored);

            for (int i = 0; i < 8; i++) {
                for (int j = 0; j < 8; j++) {
                    int x = bx + j;
                    int y = by + i;

                    if (x < src->width && y < src->height) {
                        unsigned char v = clamp_to_byte(restored[i][j] + 128.0);
                        int id = (y * dst->width + x) * 3;

                        dst->data[id + 0] = v;
                        dst->data[id + 1] = v;
                        dst->data[id + 2] = v;
                    }
                }
            }
        }
    }

    if (stats) {
        stats->estimated_bytes = (stats->nonzero_coeffs + stats->block_count) * (long)sizeof(short);
    }

    return 0;
}

int jpeg_like_compress(const Image *src, Image *dst, int quality_scale) {
    return jpeg_like_compress_with_stats(src, dst, quality_scale, NULL);
}
