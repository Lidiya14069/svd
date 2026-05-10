#include "image.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push, 1)
typedef struct {
    unsigned short bfType;
    unsigned int bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int bfOffBits;
} BMPFileHeader;

typedef struct {
    unsigned int biSize;
    int biWidth;
    int biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int biCompression;
    unsigned int biSizeImage;
    int biXPelsPerMeter;
    int biYPelsPerMeter;
    unsigned int biClrUsed;
    unsigned int biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

unsigned char clamp_to_byte(double x) {
    if (x < 0.0) return 0;
    if (x > 255.0) return 255;
    return (unsigned char)(x + 0.5);
}

void free_image(Image *img) {
    if (!img) return;

    free(img->data);
    img->data = NULL;
    img->width = 0;
    img->height = 0;
}

int read_bmp(const char *filename, Image *img) {
    FILE *f = fopen(filename, "rb");
    if (!f) return 1;

    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    if (fread(&file_header, sizeof(file_header), 1, f) != 1) {
        fclose(f);
        return 1;
    }

    if (fread(&info_header, sizeof(info_header), 1, f) != 1) {
        fclose(f);
        return 1;
    }

    if (file_header.bfType != 0x4D42) {
        fclose(f);
        return 1;
    }

    if (info_header.biBitCount != 24 || info_header.biCompression != 0) {
        fclose(f);
        return 1;
    }

    int width = info_header.biWidth;
    int height = info_header.biHeight;
    int abs_height = height > 0 ? height : -height;
    int row_padded = (width * 3 + 3) & (~3);

    unsigned char *data = (unsigned char *)malloc(width * abs_height * 3);
    unsigned char *row = (unsigned char *)malloc(row_padded);

    if (!data || !row) {
        free(data);
        free(row);
        fclose(f);
        return 1;
    }

    fseek(f, file_header.bfOffBits, SEEK_SET);

    for (int y = 0; y < abs_height; y++) {
        if (fread(row, 1, row_padded, f) != (size_t)row_padded) {
            free(data);
            free(row);
            fclose(f);
            return 1;
        }

        int dst_y = height > 0 ? abs_height - 1 - y : y;

        for (int x = 0; x < width; x++) {
            int src = x * 3;
            int dst = (dst_y * width + x) * 3;

            data[dst + 0] = row[src + 2];
            data[dst + 1] = row[src + 1];
            data[dst + 2] = row[src + 0];
        }
    }

    free(row);
    fclose(f);

    img->width = width;
    img->height = abs_height;
    img->data = data;
    return 0;
}

int write_bmp(const char *filename, const Image *img) {
    FILE *f = fopen(filename, "wb");
    if (!f) return 1;

    int width = img->width;
    int height = img->height;
    int row_padded = (width * 3 + 3) & (~3);
    int image_size = row_padded * height;

    BMPFileHeader file_header;
    BMPInfoHeader info_header;

    memset(&file_header, 0, sizeof(file_header));
    memset(&info_header, 0, sizeof(info_header));

    file_header.bfType = 0x4D42;
    file_header.bfOffBits = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader);
    file_header.bfSize = file_header.bfOffBits + image_size;

    info_header.biSize = sizeof(BMPInfoHeader);
    info_header.biWidth = width;
    info_header.biHeight = height;
    info_header.biPlanes = 1;
    info_header.biBitCount = 24;
    info_header.biCompression = 0;
    info_header.biSizeImage = image_size;

    fwrite(&file_header, sizeof(file_header), 1, f);
    fwrite(&info_header, sizeof(info_header), 1, f);

    unsigned char *row = (unsigned char *)calloc(row_padded, 1);
    if (!row) {
        fclose(f);
        return 1;
    }

    for (int y = height - 1; y >= 0; y--) {
        memset(row, 0, row_padded);

        for (int x = 0; x < width; x++) {
            int src = (y * width + x) * 3;
            int dst = x * 3;

            row[dst + 0] = img->data[src + 2];
            row[dst + 1] = img->data[src + 1];
            row[dst + 2] = img->data[src + 0];
        }

        fwrite(row, 1, row_padded, f);
    }

    free(row);
    fclose(f);
    return 0;
}
