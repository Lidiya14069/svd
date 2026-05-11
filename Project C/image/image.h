#ifndef IMAGE_H
#define IMAGE_H

typedef struct {
    int width;
    int height;
    unsigned char *data;
} Image;

int read_bmp(const char *filename, Image *img);
int write_bmp(const char *filename, const Image *img);
void free_image(Image *img);
unsigned char clamp_to_byte(double x);

#endif
