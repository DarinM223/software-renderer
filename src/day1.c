#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef struct color {
    char r;
    char g;
    char b;
} color;

#define WIDTH 64
#define HEIGHT 64
#define CHANNELS 3

color const white = {255, 255, 255};
color const green = {0, 255, 0};
color const red = {255, 0, 0};
color const blue = {64, 128, 255};
color const yellow = {255, 200, 0};

void framebuffer_set_color(char *buf, int x, int y, color color)
{
    // y = 0 is at the bottom of the screen instead of the top.
    int const index = CHANNELS * ((HEIGHT - y) * WIDTH + x);
    buf[index] = color.r;
    buf[index + 1] = color.g;
    buf[index + 2] = color.b;
}

int main(void)
{
    char buf[WIDTH * HEIGHT * CHANNELS] = {0};
    framebuffer_set_color(buf, 7, 3, white);
    framebuffer_set_color(buf, 12, 37, white);
    framebuffer_set_color(buf, 62, 53, white);
    stbi_write_png("framebuffer.tga", WIDTH, HEIGHT, CHANNELS, buf,
                   WIDTH * CHANNELS);
}