#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "image/image.h"
#include "JPEG/jpeg.h"
#include "SVD/svd.h"
#include "metrics/metrics.h"

static int already_processed(const int *values, int count, int value) {
    for (int i = 0; i < count; i++) {
        if (values[i] == value) {
            return 1;
        }
    }

    return 0;
}

static long estimate_svd_bytes(const Image *img, int k) {
    return (long)k * (img->height + img->width + 1) * (long)sizeof(double);
}

static double compression_ratio(long original_bytes, long compressed_bytes) {
    if (compressed_bytes <= 0) return 0.0;
    return (double)original_bytes / (double)compressed_bytes;
}

static double compression_percent(long original_bytes, long compressed_bytes) {
    if (original_bytes <= 0) return 0.0;
    if (compressed_bytes <= 0) return 0.0;
    if (compressed_bytes >= original_bytes) return 0.0;
    return 100.0 * (1.0 - (double)compressed_bytes / (double)original_bytes);
}

static void print_header(FILE *csv) {
    fprintf(csv, "метод,параметр,psnr,ssim,время_мс,оценка_байт,коэффициент_сжатия,процент_сжатия,файл\n");
}

static void run_jpeg_series(const Image *img, FILE *csv, long original_bytes) {
    const int quality_values[] = {1, 2, 4, 8};
    int quality_count = (int)(sizeof(quality_values) / sizeof(quality_values[0]));

    printf("\nСравнение JPEG-подобного сжатия:\n");

    for (int i = 0; i < quality_count; i++) {
        int quality = quality_values[i];
        Image jpeg_img = {0};
        JpegStats stats = {0};
        char filename[128];
        clock_t start;
        clock_t finish;
        double time_ms;
        double current_psnr;
        double current_ssim;
        double ratio;
        double percent;

        start = clock();
        if (jpeg_like_compress_with_stats(img, &jpeg_img, quality, &stats) != 0) {
            printf("q=%d: ошибка сжатия\n", quality);
            continue;
        }
        finish = clock();

        snprintf(filename, sizeof(filename), "output/jpeg_q%d.bmp", quality);
        if (write_bmp(filename, &jpeg_img) != 0) {
            printf("q=%d: не удалось сохранить %s\n", quality, filename);
        }
        if (quality == 1 && write_bmp("output/jpeg_like.bmp", &jpeg_img) != 0) {
            printf("q=%d: не удалось сохранить output/jpeg_like.bmp\n", quality);
        }

        time_ms = 1000.0 * (double)(finish - start) / (double)CLOCKS_PER_SEC;
        current_psnr = psnr(img, &jpeg_img);
        current_ssim = ssim_simple(img, &jpeg_img);
        ratio = compression_ratio(original_bytes, stats.estimated_bytes);
        percent = compression_percent(original_bytes, stats.estimated_bytes);

        printf("q=%d | PSNR=%.4f | SSIM=%.6f | время=%.2f мс | оценка=%ld байт | коэффициент=%.2f\n",
               quality, current_psnr, current_ssim, time_ms, stats.estimated_bytes, ratio);

        fprintf(csv, "JPEG-подобное,q=%d,%.4f,%.6f,%.2f,%ld,%.2f,%.2f,%s\n",
                quality, current_psnr, current_ssim, time_ms, stats.estimated_bytes, ratio, percent, filename);

        free_image(&jpeg_img);
    }
}

static void run_svd_series(const Image *img, FILE *csv, long original_bytes) {
    const int requested_k_values[] = {5, 10, 20, 30};
    int k_count = (int)(sizeof(requested_k_values) / sizeof(requested_k_values[0]));
    int processed_k_values[16];
    int processed_count = 0;

    printf("\nСравнение SVD:\n");

    if (img->width > 160 || img->height > 160) {
        printf("SVD пропущен: текущая реализация рассчитана на изображения до 160x160.\n");
        fprintf(csv, "SVD,пропуск,0,0,0,0,0,0,изображение слишком большое\n");
        return;
    }

    for (int i = 0; i < k_count; i++) {
        int k = requested_k_values[i];
        Image svd_img = {0};
        char filename[128];
        clock_t start;
        clock_t finish;
        double time_ms;
        double current_psnr;
        double current_ssim;
        long estimated_bytes;
        double ratio;
        double percent;

        if (k > img->width) k = img->width;
        if (k < 1) k = 1;

        if (already_processed(processed_k_values, processed_count, k)) {
            continue;
        }

        processed_k_values[processed_count++] = k;

        start = clock();
        if (svd_compress_grayscale(img, &svd_img, k) != 0) {
            printf("k=%d: ошибка сжатия\n", k);
            continue;
        }
        finish = clock();

        snprintf(filename, sizeof(filename), "output/svd_k%d.bmp", k);
        if (write_bmp(filename, &svd_img) != 0) {
            printf("k=%d: не удалось сохранить %s\n", k, filename);
        }
        if (write_bmp("output/svd.bmp", &svd_img) != 0) {
            printf("k=%d: не удалось сохранить output/svd.bmp\n", k);
        }

        time_ms = 1000.0 * (double)(finish - start) / (double)CLOCKS_PER_SEC;
        current_psnr = psnr(img, &svd_img);
        current_ssim = ssim_simple(img, &svd_img);
        estimated_bytes = estimate_svd_bytes(img, k);
        ratio = compression_ratio(original_bytes, estimated_bytes);
        percent = compression_percent(original_bytes, estimated_bytes);

        printf("k=%d | PSNR=%.4f | SSIM=%.6f | время=%.2f мс | оценка=%ld байт | коэффициент=%.2f\n",
               k, current_psnr, current_ssim, time_ms, estimated_bytes, ratio);

        fprintf(csv, "SVD,k=%d,%.4f,%.6f,%.2f,%ld,%.2f,%.2f,%s\n",
                k, current_psnr, current_ssim, time_ms, estimated_bytes, ratio, percent, filename);

        free_image(&svd_img);
    }
}

int main(int argc, char **argv) {
    const char *input_name = "input/test.bmp";
    Image img = {0};
    FILE *csv = NULL;
    long original_bytes;

    if (argc >= 2) {
        input_name = argv[1];
    }

    if (read_bmp(input_name, &img) != 0) {
        printf("Ошибка: не удалось прочитать BMP файл: %s\n", input_name);
        if (argc < 2) {
            printf("Укажи путь к корректному 24-bit BMP, например: ./program <file.bmp>\n");
        }
        return 1;
    }

    printf("Изображение загружено: %d x %d\n", img.width, img.height);
    if (img.width < 32 || img.height < 32) {
        printf("Замечание: для наглядного сравнения лучше взять картинку хотя бы 64x64.\n");
    }

    if (write_bmp("output/copy.bmp", &img) == 0) {
        printf("Сохранена копия: output/copy.bmp\n");
    }

    csv = fopen("output/results.csv", "w");
    if (!csv) {
        printf("Ошибка: не удалось создать output/results.csv\n");
        free_image(&img);
        return 1;
    }

    original_bytes = (long)img.width * img.height * 3L;
    print_header(csv);

    printf("Оригинальный объём RGB-данных: %ld байт\n", original_bytes);

    run_jpeg_series(&img, csv, original_bytes);
    run_svd_series(&img, csv, original_bytes);

    fclose(csv);
    printf("\nТаблица результатов сохранена в output/results.csv\n");

    free_image(&img);
    return 0;
}
