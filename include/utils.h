#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

typedef struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color;

color const white = {255, 255, 255};
color const green = {0, 255, 0};
color const red = {255, 0, 0};
color const blue = {64, 128, 255};
color const yellow = {255, 200, 0};

#if defined(WIDTH) && defined(HEIGHT) && defined(CHANNELS)
void framebuffer_set_color(uint8_t *buf, int32_t x, int32_t y, color color)
{
    // y = 0 is at the bottom of the screen instead of the top.
    int const index = CHANNELS * ((HEIGHT - 1 - y) * WIDTH + x);
    buf[index] = color.r;
    buf[index + 1] = color.g;
    buf[index + 2] = color.b;
}
#endif

#endif // UTILS_H