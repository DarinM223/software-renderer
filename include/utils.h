#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
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
#include <stdlib.h>
void framebuffer_set_color(uint8_t *buf, int32_t x, int32_t y, color color)
{
    // y = 0 is at the bottom of the screen instead of the top.
    int const index = CHANNELS * ((HEIGHT - 1 - y) * WIDTH + x);
    buf[index] = color.r;
    buf[index + 1] = color.g;
    buf[index + 2] = color.b;
}

void framebuffer_draw_line(uint8_t *buf, int32_t a_x, int32_t a_y, int32_t b_x,
                           int32_t b_y, color color)
{
    bool const steep = abs(a_x - b_x) < abs(a_y - b_y);
    // If steep, transpose x and y.
    if (steep) {
        int32_t tmp = a_x;
        a_x = a_y;
        a_y = tmp;
        tmp = b_x;
        b_x = b_y;
        b_y = tmp;
    }
    // Make sure x always goes from lower to higher.
    if (a_x > b_x) {
        int32_t tmp = a_x;
        a_x = b_x;
        b_x = tmp;
        tmp = a_y;
        a_y = b_y;
        b_y = tmp;
    }
    int32_t y = a_y;
    int32_t ierror = 0;
    for (int32_t x = a_x; x <= b_x; ++x) {
        // Then de-transpose x and y when writing pixel.
        if (steep) {
            framebuffer_set_color(buf, y, x, color);
        } else {
            framebuffer_set_color(buf, x, y, color);
        }
        ierror += 2 * abs(b_y - a_y);
        y += (b_y > a_y ? 1 : -1) * (ierror > b_x - a_x);
        ierror -= 2 * (b_x - a_x) * (ierror > b_x - a_x);
    }
}
#endif

#endif // UTILS_H