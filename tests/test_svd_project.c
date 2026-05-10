#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../image/image.h"
#include "../JPEG/jpeg.h"
#include "../SVD/svd.h"
#include "../metrics/metrics.h"

#define EXPECT(condition, message)                                      \
    do {                                                                \
        if (!(condition)) {                                             \
            fprintf(stderr, "FAIL: %s (%s:%d)\n", message, __FILE__, __LINE__); \
            return 1;                                                   \
        }                                                               \
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
    int id = (y * img->width + x) * 3;
    img->data[id + 0] = r;
    img->data[id + 1] = g;
    img->data[id + 2] = b;
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
    int pixels = img->width * img->height;

    for (int i = 0; i < pixels; i++) {
        int id = i * 3;
        if (img->data[id + 0] != img->data[id + 1] ||
            img->data[id + 1] != img->data[id + 2]) {
            return 0;
        }
    }

    return 1;
}

static int max_channel_diff(const Image *a, const Image *b) {
    int total = a->width * a->height * 3;
    int max_diff = 0;

    for (int i = 0; i < total; i++) {
        int diff = abs((int)a->data[i] - (int)b->data[i]);
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

    EXPECT(src.data != NULL, "failed to allocate source image");
    EXPECT(write_bmp(path, &src) == 0, "write_bmp should write a valid BMP");
    EXPECT(read_bmp(path, &loaded) == 0, "read_bmp should read back written BMP");
    EXPECT(loaded.width == src.width && loaded.height == src.height, "BMP roundtrip should preserve dimensions");
    EXPECT(memcmp(src.data, loaded.data, (size_t)src.width * src.height * 3) == 0, "BMP roundtrip should preserve pixel data");

    free_image(&src);
    free_image(&loaded);
    return 0;
}

static int test_invalid_bmp_is_rejected(void) {
    const char *path = "/tmp/project_c_invalid.bmp";
    FILE *f = fopen(path, "wb");
    Image img = {0};

    EXPECT(f != NULL, "failed to create invalid BMP fixture");
    fwrite("NOT_A_BMP", 1, 9, f);
    fclose(f);

    EXPECT(read_bmp(path, &img) != 0, "read_bmp should reject invalid input");
    return 0;
}

static int test_metrics_on_identical_and_changed_images(void) {
    Image base = make_gray_pattern_image(8, 8);
    Image same = make_gray_pattern_image(8, 8);
    Image changed = make_gray_pattern_image(8, 8);

    EXPECT(base.data && same.data && changed.data, "failed to allocate metric fixtures");

    for (int y = 0; y < changed.height; y++) {
        for (int x = 0; x < changed.width; x++) {
            int id = (y * changed.width + x) * 3;
            unsigned char value = clamp_to_byte(changed.data[id] + ((x + y) % 3) * 15.0);
            changed.data[id + 0] = value;
            changed.data[id + 1] = value;
            changed.data[id + 2] = value;
        }
    }

    EXPECT(psnr(&base, &same) == 99.0, "PSNR of identical images should be 99.0");
    EXPECT(fabs(ssim_simple(&base, &same) - 1.0) < 1e-9, "SSIM of identical images should be 1");
    EXPECT(psnr(&base, &changed) < 99.0, "PSNR should drop after image changes");
    EXPECT(ssim_simple(&base, &changed) < 1.0, "SSIM should drop after image changes");

    free_image(&base);
    free_image(&same);
    free_image(&changed);
    return 0;
}

static int test_jpeg_quality_scale_affects_quality(void) {
    Image src = make_gray_pattern_image(16, 16);
    Image q1 = {0};
    Image q4 = {0};

    EXPECT(src.data != NULL, "failed to allocate JPEG fixture");
    EXPECT(jpeg_like_compress(&src, &q1, 1) == 0, "jpeg_like_compress quality 1 should succeed");
    EXPECT(jpeg_like_compress(&src, &q4, 4) == 0, "jpeg_like_compress quality 4 should succeed");
    EXPECT(q1.width == src.width && q1.height == src.height, "JPEG-like compression should preserve image size");
    EXPECT(q4.width == src.width && q4.height == src.height, "JPEG-like compression should preserve image size");
    EXPECT(image_is_grayscale(&q1), "JPEG-like output should be grayscale");
    EXPECT(image_is_grayscale(&q4), "JPEG-like output should be grayscale");
    EXPECT(psnr(&src, &q1) > psnr(&src, &q4), "smaller quality_scale should preserve more detail");

    free_image(&src);
    free_image(&q1);
    free_image(&q4);
    return 0;
}

static int test_svd_rank_controls_reconstruction_quality(void) {
    Image src = make_rank_two_image(8, 8);
    Image k1 = {0};
    Image k2 = {0};

    EXPECT(src.data != NULL, "failed to allocate SVD fixture");
    EXPECT(svd_compress_grayscale(&src, &k1, 1) == 0, "svd_compress_grayscale with k=1 should succeed");
    EXPECT(svd_compress_grayscale(&src, &k2, 2) == 0, "svd_compress_grayscale with k=2 should succeed");
    EXPECT(image_is_grayscale(&k1), "SVD output should be grayscale");
    EXPECT(image_is_grayscale(&k2), "SVD output should be grayscale");
    EXPECT(psnr(&src, &k2) > psnr(&src, &k1) + 5.0, "larger k should noticeably improve SVD reconstruction");
    EXPECT(max_channel_diff(&src, &k2) <= 1, "rank-2 image should be reconstructed almost exactly with k=2");

    free_image(&src);
    free_image(&k1);
    free_image(&k2);
    return 0;
}

static int test_svd_rejects_large_images(void) {
    Image src = make_gray_pattern_image(161, 4);
    Image dst = {0};

    EXPECT(src.data != NULL, "failed to allocate oversized SVD fixture");
    EXPECT(svd_compress_grayscale(&src, &dst, 5) != 0, "SVD should reject images larger than 160 pixels per side");

    free_image(&src);
    return 0;
}

int main(void) {
    struct {
        const char *name;
        TestFn fn;
    } tests[] = {
        {"bmp roundtrip", test_bmp_roundtrip},
        {"invalid bmp", test_invalid_bmp_is_rejected},
        {"metrics", test_metrics_on_identical_and_changed_images},
        {"jpeg quality", test_jpeg_quality_scale_affects_quality},
        {"svd rank", test_svd_rank_controls_reconstruction_quality},
        {"svd size limit", test_svd_rejects_large_images},
    };

    int test_count = (int)(sizeof(tests) / sizeof(tests[0]));

    for (int i = 0; i < test_count; i++) {
        if (tests[i].fn() != 0) {
            return 1;
        }
        printf("PASS: %s\n", tests[i].name);
    }

    printf("All %d tests passed.\n", test_count);
    return 0;
}
