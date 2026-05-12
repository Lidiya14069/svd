#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../image/image.h"
#include "../JPEG/jpeg.h"
#include "../SVD/svd.h"
#include "../metrics/metrics.h"

#define EXPECT(condition, message)                                       \
    do {                                                                 \
        if (!(condition)) {                                              \
            fprintf(stderr, "Ошибка теста: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            return 1;                                                    \
        }                                                                \
    } while (0)

typedef int (*TestFn)(void);

static Image make_image(int width, int height) {
    Image img;
    img.width = width;
    img.height = height;
    img.data = (unsigned char *)calloc((size_t)width * height * 3, 1);
    return img;
}

static void set_pixel(Image *img, int x, int y, unsigned char r, unsigned char g, unsigned char b) {
    int id = (y * img -> width + x) * 3;
    img -> data[id + 0] = r;
    img -> data[id + 1] = g;
    img -> data[id + 2] = b;
}

static void set_gray(Image *img, int x, int y, unsigned char value) {
    set_pixel(img, x, y, value, value, value);
}

static Image make_rgb_pattern_image(int width, int height) {
    Image img = make_image(width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char r = (unsigned char)((x * 70 + y * 15 + 20) % 256);
            unsigned char g = (unsigned char)((x * 30 + y * 90 + 40) % 256);
            unsigned char b = (unsigned char)((x * 55 + y * 45 + 60) % 256);
            set_pixel(&img, x, y, r, g, b);
        }
    }

    return img;
}

static Image make_gray_pattern_image(int width, int height) {
    Image img = make_image(width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            unsigned char value = (unsigned char)((x * 17 + y * 31 + ((x ^ y) & 1) * 80) % 256);
            set_gray(&img, x, y, value);
        }
    }

    return img;
}

static Image make_rank_two_image(int width, int height) {
    Image img = make_image(width, height);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int first = (y + 1) * (x + 2) * 2;
            int second = ((y % 2) + 1) * (x + 1) * 5;
            unsigned char value = (unsigned char)(first + second);
            set_gray(&img, x, y, value);
        }
    }

    return img;
}

static int image_is_grayscale(const Image *img) {
    int pixels = img -> width * img -> height;

    for (int i = 0; i < pixels; i++) {
        int id = i * 3;
        if (img -> data[id + 0] != img -> data[id + 1] ||
            img -> data[id + 1] != img -> data[id + 2]) {
            return 0;
        }
    }

    return 1;
}

static int max_channel_diff(const Image *a, const Image *b) {
    int total = a -> width * a -> height * 3;
    int max_diff = 0;

    for (int i = 0; i < total; i++) {
        int diff = abs((int)a -> data[i] - (int)b -> data[i]);
        if (diff > max_diff) {
            max_diff = diff;
        }
    }

    return max_diff;
}

static int test_bmp_roundtrip(void) {
    const char *path = "/tmp/project_c_roundtrip.bmp";
    Image src = make_rgb_pattern_image(3, 2);
    Image loaded = {0};

    EXPECT(src.data != NULL, "память BMP");
    EXPECT(write_bmp(path, &src) == 0, "запись BMP");
    EXPECT(read_bmp(path, &loaded) == 0, "чтение BMP");
    EXPECT(loaded.width == src.width && loaded.height == src.height, "размер BMP");
    EXPECT(memcmp(src.data, loaded.data, (size_t)src.width * src.height * 3) == 0, "данные BMP");

    free_image(&src);
    free_image(&loaded);
    return 0;
}

static int test_invalid_bmp_is_rejected(void) {
    const char *path = "/tmp/project_c_invalid.bmp";
    FILE *f = fopen(path, "wb");
    Image img = {0};

    EXPECT(f != NULL, "битый BMP");
    fwrite("NOT_A_BMP", 1, 9, f);
    fclose(f);

    EXPECT(read_bmp(path, &img) != 0, "проверка битого BMP");
    return 0;
}

static int test_metrics_on_identical_and_changed_images(void) {
    Image base = make_gray_pattern_image(8, 8);
    Image same = make_gray_pattern_image(8, 8);
    Image changed = make_gray_pattern_image(8, 8);

    EXPECT(base.data && same.data && changed.data, "память метрик");

    for (int y = 0; y < changed.height; y++) {
        for (int x = 0; x < changed.width; x++) {
            int id = (y * changed.width + x) * 3;
            unsigned char value = clamp_to_byte(changed.data[id] + ((x + y) % 3) * 15.0);
            changed.data[id + 0] = value;
            changed.data[id + 1] = value;
            changed.data[id + 2] = value;
        }
    }

    EXPECT(psnr(&base, &same) == 99.0, "PSNR одинаковых");
    EXPECT(fabs(ssim_simple(&base, &same) - 1.0) < 1e-9, "SSIM одинаковых");
    EXPECT(psnr(&base, &changed) < 99.0, "PSNR после изменения");
    EXPECT(ssim_simple(&base, &changed) < 1.0, "SSIM после изменения");

    free_image(&base);
    free_image(&same);
    free_image(&changed);
    return 0;
}

static int test_jpeg_quality_scale_affects_quality(void) {
    Image src = make_gray_pattern_image(16, 16);
    Image q1 = {0};
    Image q4 = {0};

    EXPECT(src.data != NULL, "память JPEG");
    EXPECT(jpeg_like_compress(&src, &q1, 1) == 0, "q=1");
    EXPECT(jpeg_like_compress(&src, &q4, 4) == 0, "q=4");
    EXPECT(q1.width == src.width && q1.height == src.height, "размер q=1");
    EXPECT(q4.width == src.width && q4.height == src.height, "размер q=4");
    EXPECT(image_is_grayscale(&q1), "серый q=1");
    EXPECT(image_is_grayscale(&q4), "серый q=4");
    EXPECT(psnr(&src, &q1) > psnr(&src, &q4), "качество JPEG");

    free_image(&src);
    free_image(&q1);
    free_image(&q4);
    return 0;
}

static int test_svd_rank_controls_reconstruction_quality(void) {
    Image src = make_rank_two_image(8, 8);
    Image k1 = {0};
    Image k2 = {0};

    EXPECT(src.data != NULL, "память SVD");
    EXPECT(svd_compress_grayscale(&src, &k1, 1) == 0, "k=1");
    EXPECT(svd_compress_grayscale(&src, &k2, 2) == 0, "k=2");
    EXPECT(image_is_grayscale(&k1), "серый k=1");
    EXPECT(image_is_grayscale(&k2), "серый k=2");
    EXPECT(psnr(&src, &k2) > psnr(&src, &k1) + 5.0, "качество SVD");
    EXPECT(max_channel_diff(&src, &k2) <= 1, "точность SVD");

    free_image(&src);
    free_image(&k1);
    free_image(&k2);
    return 0;
}

static int test_svd_rejects_large_images(void) {
    Image src = make_gray_pattern_image(161, 4);
    Image dst = {0};

    EXPECT(src.data != NULL, "большое SVD");
    EXPECT(svd_compress_grayscale(&src, &dst, 5) != 0, "лимит SVD");

    free_image(&src);
    return 0;
}

int main(void) {
    struct {
        const char *name;
        TestFn fn;
    } tests[] = {
        {"BMP", test_bmp_roundtrip},
        {"Битый BMP", test_invalid_bmp_is_rejected},
        {"Метрики", test_metrics_on_identical_and_changed_images},
        {"JPEG", test_jpeg_quality_scale_affects_quality},
        {"SVD", test_svd_rank_controls_reconstruction_quality},
        {"Лимит SVD", test_svd_rejects_large_images},
    };

    int test_count = (int)(sizeof(tests) / sizeof(tests[0]));

    for (int i = 0; i < test_count; i++) {
        if (tests[i].fn() != 0) {
            return 1;
        }
        printf("Пройдено: %s\n", tests[i].name);
    }

    printf("Все %d тестов пройдены.\n", test_count);
    return 0;
}
