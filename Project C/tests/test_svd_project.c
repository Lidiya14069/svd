#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../image/image.h"
#include "../JPEG/jpeg.h"
#include "../SVD/svd.h"
#include "../metrics/metrics.h"

#define EXPECT(condition, message)                                       \ // макрос для проверки условия в тестах
    do {                                                                 \ // начало блока макроса
        if (!(condition)) {                                              \ // если условие ложно
            fprintf(stderr, "Ошибка теста: %s (%s:%d)\n", message, __FILE__, __LINE__); \ // вывожу сообщение об ошибке
            return 1;                                                   
        }                                                                
    } while (0) // конец макроса EXPECT

typedef int (*TestFn)(void); // тип указателя на функцию теста

static Image make_image(int width, int height) { // функция создаёт пустое изображение
    Image img; // создаю структуру изображения
    img.width = width; // задаю ширину изображения
    img.height = height; // задаю высоту изображения
    img.data = (unsigned char *)calloc((size_t)width * height * 3, 1); // выделяю память под RGB-пиксели
    return img; 
} 

static void set_pixel(Image *img, int x, int y, unsigned char r, unsigned char g, unsigned char b) { // функция задаёт цвет пикселя
    int id = (y * img -> width + x) * 3; // считаю индекс пикселя в массиве RGB
    img -> data[id + 0] = r; // записываю красный канал
    img -> data[id + 1] = g; // записываю зелёный канал
    img -> data[id + 2] = b; // записываю синий канал
} 

static void set_gray(Image *img, int x, int y, unsigned char value) { // функция задаёт серый пиксель
    set_pixel(img, x, y, value, value, value); // записываю одинаковое значение во все RGB-каналы
} 

static Image make_rgb_pattern_image(int width, int height) { // функция создаёт тестовое RGB-изображение
    Image img = make_image(width, height); // создаю пустое изображение

    for (int y = 0; y < height; y++) { // прохожу по строкам изображения
        for (int x = 0; x < width; x++) { // прохожу по столбцам изображения
            unsigned char r = (unsigned char)((x * 70 + y * 15 + 20) % 256); // считаю значение красного канала
            unsigned char g = (unsigned char)((x * 30 + y * 90 + 40) % 256); // считаю значение зелёного канала
            unsigned char b = (unsigned char)((x * 55 + y * 45 + 60) % 256); // считаю значение синего канала
            set_pixel(&img, x, y, r, g, b); // записываю цвет пикселя
        } 
    }

    return img; // возвращаю изображение
} 

static Image make_gray_pattern_image(int width, int height) { // функция создаёт тестовое серое изображение
    Image img = make_image(width, height); // создаю пустое изображение

    for (int y = 0; y < height; y++) { // прохожу по строкам изображения
        for (int x = 0; x < width; x++) { // прохожу по столбцам изображения
            unsigned char value = (unsigned char)((x * 17 + y * 31 + ((x ^ y) & 1) * 80) % 256); // считаю яркость пикселя
            set_gray(&img, x, y, value); // записываю серый пиксель
        } 
    } 

    return img; // возвращаю изображение
} 

static Image make_rank_two_image(int width, int height) { // функция создаёт изображение ранга 2 для теста SVD
    Image img = make_image(width, height); // создаю пустое изображение

    for (int y = 0; y < height; y++) { // прохожу по строкам
        for (int x = 0; x < width; x++) { // прохожу по столбцам
            int first = (y + 1) * (x + 2) * 2; // считаю первую компоненту
            int second = ((y % 2) + 1) * (x + 1) * 5; // считаю вторую компоненту
            unsigned char value = (unsigned char)(first + second); // получаю итоговую яркость
            set_gray(&img, x, y, value); // записываю серый пиксель
        } 
    } 

    return img; // возвращаю изображение
} 

static int image_is_grayscale(const Image *img) { // функция проверяет, является ли изображение серым
    int pixels = img -> width * img -> height; // считаю количество пикселей

    for (int i = 0; i < pixels; i++) { // прохожу по всем пикселям
        int id = i * 3; // считаю индекс пикселя
        if (img -> data[id + 0] != img -> data[id + 1] || // сравниваю красный и зелёный каналы
            img -> data[id + 1] != img -> data[id + 2]) { // сравниваю зелёный и синий каналы
            return 0; // если каналы отличаются, изображение не серое
        } 
    }

    return 1; // если все каналы одинаковые, изображение серое
}

static int max_channel_diff(const Image *a, const Image *b) { // функция ищет максимальную разницу между изображениями
    int total = a -> width * a -> height * 3; // считаю количество всех RGB-компонент
    int max_diff = 0; // максимальная найденная разница

    for (int i = 0; i < total; i++) { // прохожу по всем байтам изображений
        int diff = abs((int)a -> data[i] - (int)b -> data[i]); // считаю модуль разницы
        if (diff > max_diff) { // если разница больше текущего максимума
            max_diff = diff; // обновляю максимальную разницу
        } 
    } 

    return max_diff; // возвращаю максимальную разницу
} 

static int test_bmp_roundtrip(void) { // тест проверки записи и чтения BMP
    const char *path = "/tmp/project_c_roundtrip.bmp"; // путь к временному BMP-файлу
    Image src = make_rgb_pattern_image(3, 2); // создаю тестовое RGB-изображение
    Image loaded = {0}; // создаю пустое изображение для загрузки

    EXPECT(src.data != NULL, "память BMP"); // проверяю выделение памяти
    EXPECT(write_bmp(path, &src) == 0, "запись BMP"); // проверяю запись BMP
    EXPECT(read_bmp(path, &loaded) == 0, "чтение BMP"); // проверяю чтение BMP
    EXPECT(loaded.width == src.width && loaded.height == src.height, "размер BMP"); // проверяю размеры изображения
    EXPECT(memcmp(src.data, loaded.data, (size_t)src.width * src.height * 3) == 0, "данные BMP"); // проверяю совпадение пикселей

    free_image(&src); // освобождаю память исходного изображения
    free_image(&loaded); // освобождаю память загруженного изображения
    return 0; // тест пройден успешно
} 

static int test_invalid_bmp_is_rejected(void) { // тест проверки битого BMP-файла
    const char *path = "/tmp/project_c_invalid.bmp"; // путь к битому BMP
    FILE *f = fopen(path, "wb"); // открываю файл для записи
    Image img = {0}; // создаю пустое изображение

    EXPECT(f != NULL, "битый BMP"); // проверяю открытие файла
    fwrite("NOT_A_BMP", 1, 9, f); // записываю неверные данные вместо BMP
    fclose(f);

    EXPECT(read_bmp(path, &img) != 0, "проверка битого BMP"); // проверяю, что чтение завершилось ошибкой
    return 0; 
}
