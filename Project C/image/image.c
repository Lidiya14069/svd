#include "image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1) // отключаю выравнивание структуры, чтобы поля BMP-заголовка шли подряд без лишних байтов

typedef struct { // структура заголовка BMP-файла
    unsigned short bfType; // тип файла, для BMP должен быть 0x4D42
    unsigned int bfSize; // общий размер BMP-файла в байтах
    unsigned short bfReserved1; // зарезервированное поле, обычно равно 0
    unsigned short bfReserved2; // второе зарезервированное поле, обычно равно 0
    unsigned int bfOffBits; // смещение до начала массива пикселей
} BMPFileHeader; 

typedef struct { // структура информационного заголовка BMP
    unsigned int biSize; // размер этой структуры
    int biWidth; // ширина изображения
    int biHeight; // высота изображения
    unsigned short biPlanes; // количество цветовых плоскостей, обычно 1
    unsigned short biBitCount;// количество бит на пиксель
    unsigned int biCompression;// тип сжатия, 0 означает без сжатия
    unsigned int biSizeImage; // размер массива пикселей
    int biXPelsPerMeter;  // горизонтальное разрешение
    int biYPelsPerMeter; // вертикальное разрешение
    unsigned int biClrUsed;// количество используемых цветов
    unsigned int biClrImportant;  // количество важных цветов
} BMPInfoHeader; 

#pragma pack(pop) // возвращаю стандартное выравнивание структур

unsigned char clamp_to_byte(double x) { // функция ограничивает число диапазоном одного байта
    if (x < 0.0) return 0; // если число меньше 0, возвращаю 0
    if (x > 255.0) return 255;  // если число больше 255, возвращаю 255
    return (unsigned char)(x + 0.5); // иначе округляю число и привожу к unsigned char
} 

void free_image(Image *img) { // функция освобождает память изображения
    if (!img) return;  // если указатель пустой, ничего не делаю

    free(img->data); // освобождаю память с пикселями изображения
    img->data = NULL;// обнуляю указатель, чтобы не было висячего указателя
    img->width = 0;  // сбрасываю ширину изображения
    img->height = 0;// сбрасываю высоту изображения
} 

int read_bmp(const char *filename, Image *img) { // функция читает BMP-файл
    FILE *f = fopen(filename, "rb"); // открываю файл для чтения в бинарном режиме
    if (!f) return 1; // если файл не открылся, возвращаю ошибку

    BMPFileHeader file_header;// создаю переменную для заголовка файла
    BMPInfoHeader info_header; // создаю переменную для информационного заголовка

    if (fread(&file_header, sizeof(file_header), 1, f) != 1) { // читаю заголовок BMP-файла
        fclose(f);                                             
        return 1;                                              
    }                                                          

    if (fread(&info_header, sizeof(info_header), 1, f) != 1) { // читаю информационный заголовок BMP
        fclose(f);                                            
        return 1;                                              
    }                                                          

    if (file_header.bfType != 0x4D42) { // проверяю, что файл действительно BMP
        fclose(f);                      
        return 1;                       
    }                                  
    
    if (info_header.biBitCount != 24 || info_header.biCompression != 0) { // проверяю, что BMP 24-битный и без сжатия
        fclose(f);                                                       
        return 1;                                                        
    }                                                                     

    int width = info_header.biWidth; // сохраняю ширину изображения
    int height = info_header.biHeight;  // сохраняю высоту изображения
    int abs_height = height > 0 ? height : -height;  // получаю положительную высоту изображения
    int row_padded = (width * 3 + 3) & (~3); // считаю размер строки с выравниванием до 4 байт

    unsigned char *data = (unsigned char *)malloc(width * abs_height * 3); // выделяю память под RGB-пиксели
    unsigned char *row = (unsigned char *)malloc(row_padded); // выделяю память под одну строку BMP

    if (!data || !row) { // проверяю, что память выделилась
        free(data);     
        free(row);       
        fclose(f);      
        return 1;       
    }                   

    fseek(f, file_header.bfOffBits, SEEK_SET); // перехожу к началу пиксельных данных

    for (int y = 0; y < abs_height; y++) { // читаю изображение построчно
        if (fread(row, 1, row_padded, f) != (size_t)row_padded) { // читаю одну строку с учётом padding
            free(data);                                          
            free(row);                                           
            fclose(f);                                          
            return 1;                                        
        }                                                        

        int dst_y = height > 0 ? abs_height - 1 - y : y; // если BMP снизу вверх, переворачиваю строку

        for (int x = 0; x < width; x++) { // прохожу по всем пикселям строки
            int src = x * 3;              // индекс пикселя в считанной строке BMP
            int dst = (dst_y * width + x) * 3; // индекс пикселя в моём массиве RGB

            data[dst + 0] = row[src + 2]; // записываю красный канал, потому что BMP хранит BGR
            data[dst + 1] = row[src + 1]; // записываю зелёный канал
            data[dst + 2] = row[src + 0]; // записываю синий канал
        }                                
    }                                    

    free(row);                            
    fclose(f);                          

    img->width = width;  // записываю ширину в структуру изображения
    img->height = abs_height; // записываю высоту в структуру изображения
    img->data = data;  // записываю указатель на массив пикселей
    return 0; // возвращаю 0, если чтение прошло успешно
} 

int write_bmp(const char *filename, const Image *img) { // функция записывает изображение в BMP-файл
    FILE *f = fopen(filename, "wb");  // открываю файл для записи в бинарном режиме
    if (!f) return 1; // если файл не открылся, возвращаю ошибку

    int width = img->width; // сохраняю ширину изображения
    int height = img->height;   // сохраняю высоту изображения
    int row_padded = (width * 3 + 3) & (~3); // считаю размер строки с выравниванием до 4 байт
    int image_size = row_padded * height;  // считаю общий размер пиксельных данных

    BMPFileHeader file_header; // создаю заголовок BMP-файла
    BMPInfoHeader info_header;  // создаю информационный заголовок BMP

    memset(&file_header, 0, sizeof(file_header)); // заполняю заголовок файла нулями
    memset(&info_header, 0, sizeof(info_header)); // заполняю информационный заголовок нулями

    file_header.bfType = 0x4D42;   // задаю тип файла BMP
    file_header.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader); // задаю смещение до пикселей
    file_header.bfSize = file_header.bfOffBits + image_size; // задаю полный размер файла

    info_header.biSize = sizeof(BMPInfoHeader); // задаю размер информационного заголовка
    info_header.biWidth = width; // записываю ширину изображения
    info_header.biHeight = height;  // записываю высоту изображения
    info_header.biPlanes = 1;   // задаю количество плоскостей
    info_header.biBitCount = 24; // задаю 24 бита на пиксель
    info_header.biCompression = 0;    // указываю, что сжатия нет
    info_header.biSizeImage = image_size;  // записываю размер пиксельных данных

    fwrite(&file_header, sizeof(file_header), 1, f); // записываю заголовок BMP-файла
    fwrite(&info_header, sizeof(info_header), 1, f); // записываю информационный заголовок BMP

    unsigned char *row = (unsigned char *)calloc(row_padded, 1); // выделяю память под строку с padding
    if (!row) {// проверяю, что память выделилась
        fclose(f);                                              
        return 1;                                              
    }                                                           

    for (int y = height - 1; y >= 0; y--) { // записываю строки снизу вверх, как принято в BMP
        memset(row, 0, row_padded);  // очищаю строку и padding нулями

        for (int x = 0; x < width; x++) {   // прохожу по всем пикселям строки
            int src = (y * width + x) * 3;  // индекс пикселя в моём RGB-массиве
            int dst = x * 3;  // индекс пикселя в строке BMP

            row[dst + 0] = img->data[src + 2]; // записываю синий канал, потому что BMP хранит BGR
            row[dst + 1] = img->data[src + 1]; // записываю зелёный канал
            row[dst + 2] = img->data[src + 0]; // записываю красный канал
        }                                      // конец цикла по пикселям

        fwrite(row, 1, row_padded, f); // записываю строку изображения в файл
    }                                         

    free(row);                                
    fclose(f);                                 
    return 0;                                  
} // конец функции write_bmp
