#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <stdint.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define SWAP(T, X, Y)                                                          \
    do {                                                                       \
        T _tmp = X;                                                            \
        X = Y;                                                                 \
        Y = _tmp;                                                              \
    } while (0)

typedef struct color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} color;

static color const white = {255, 255, 255};
static color const green = {0, 255, 0};
static color const red = {255, 0, 0};
static color const blue = {64, 128, 255};
static color const yellow = {255, 200, 0};

#if defined(WIDTH) && defined(HEIGHT) && defined(CHANNELS)
#include <stdlib.h>
color framebuffer_get_color(uint8_t *buf, uint8_t x, uint8_t y)
{
    int const index = CHANNELS * ((HEIGHT - 1 - y) * WIDTH + x);
    return (color){.r = buf[index], .g = buf[index + 1], .b = buf[index + 2]};
}
void framebuffer_set_color(uint8_t *buf, int32_t x, int32_t y, color color)
{
    if (x < 0 || y < 0 || x >= WIDTH || y >= HEIGHT) return;
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

#ifdef _OPENMP
#include "geometry.h"
#include <omp.h>

void framebuffer_draw_triangle(uint8_t *buf, int32_t ax, int32_t ay, int32_t bx,
                               int32_t by, int32_t cx, int32_t cy, color color)
{
    int32_t bbminx = MIN(MIN(ax, bx), cx);
    int32_t bbminy = MIN(MIN(ay, by), cy);
    int32_t bbmaxx = MAX(MAX(ax, bx), cx);
    int32_t bbmaxy = MAX(MAX(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    // Backface culling + discarding triangles that cover less than a pixel.
    if (total_area < 1) {
        return;
    }

#pragma omp parallel for
    for (int32_t x = bbminx; x <= bbmaxx; ++x) {
        for (int32_t y = bbminy; y <= bbmaxy; ++y) {
            // Determine barycentric coordinates for (x, y).
            double alpha =
                signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta =
                signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma =
                signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            // Don't color coordinates that are not in the triangle.
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }
            framebuffer_set_color(buf, x, y, color);
        }
    }
}

void framebuffer_draw_triangle_z(uint8_t *zbuffer, uint8_t *buf, int32_t ax,
                                 int32_t ay, int32_t az, int32_t bx, int32_t by,
                                 int32_t bz, int32_t cx, int32_t cy, int32_t cz,
                                 color color)
{
    // Sneaky clipping logic added in day6, would cause crashing if omitted.
    int32_t bbminx = MAX(0, MIN(MIN(ax, bx), cx));
    int32_t bbminy = MAX(0, MIN(MIN(ay, by), cy));
    int32_t bbmaxx = MIN(WIDTH - 1, MAX(MAX(ax, bx), cx));
    int32_t bbmaxy = MIN(HEIGHT - 1, MAX(MAX(ay, by), cy));
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    // Backface culling + discarding triangles that cover less than a pixel.
    if (total_area < 1) {
        return;
    }

#pragma omp parallel for
    for (int32_t x = bbminx; x <= bbmaxx; ++x) {
        for (int32_t y = bbminy; y <= bbmaxy; ++y) {
            // Determine barycentric coordinates for (x, y).
            double alpha =
                signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta =
                signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma =
                signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            // Don't color coordinates that are not in the triangle.
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }
            uint8_t z = alpha * az + beta * bz + gamma * cz;
            // If pixel's z value is inferior to the one already painted, then
            // discard it.
            if (z <= zbuffer[y * WIDTH + x]) {
                continue;
            }
            zbuffer[y * WIDTH + x] = z;
            framebuffer_set_color(buf, x, y, color);
        }
    }
}
void framebuffer_draw_triangle_z_float(float *zbuffer, uint8_t *buf, int32_t ax,
                                       int32_t ay, int32_t az, int32_t bx,
                                       int32_t by, int32_t bz, int32_t cx,
                                       int32_t cy, int32_t cz, color color)
{
    // Sneaky clipping logic added in day6, would cause crashing if omitted.
    int32_t bbminx = MAX(0, MIN(MIN(ax, bx), cx));
    int32_t bbminy = MAX(0, MIN(MIN(ay, by), cy));
    int32_t bbmaxx = MIN(WIDTH - 1, MAX(MAX(ax, bx), cx));
    int32_t bbmaxy = MIN(HEIGHT - 1, MAX(MAX(ay, by), cy));
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    // Backface culling + discarding triangles that cover less than a pixel.
    if (total_area < 1) {
        return;
    }

#pragma omp parallel for
    for (int32_t x = bbminx; x <= bbmaxx; ++x) {
        for (int32_t y = bbminy; y <= bbmaxy; ++y) {
            // Determine barycentric coordinates for (x, y).
            double alpha =
                signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta =
                signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma =
                signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            // Don't color coordinates that are not in the triangle.
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }
            float z = alpha * az + beta * bz + gamma * cz;
            // If pixel's z value is inferior to the one already painted, then
            // discard it.
            if (z <= zbuffer[y * WIDTH + x]) {
                continue;
            }
            zbuffer[y * WIDTH + x] = z;
            framebuffer_set_color(buf, x, y, color);
        }
    }
}
#else
void framebuffer_draw_triangle(uint8_t *buf, int32_t ax, int32_t ay, int32_t bx,
                               int32_t by, int32_t cx, int32_t cy, color color)
{
    if (ay > by) {
        SWAP(int32_t, ax, bx);
        SWAP(int32_t, ay, by);
    }
    if (ay > cy) {
        SWAP(int32_t, ax, cx);
        SWAP(int32_t, ay, cy);
    }
    if (by > cy) {
        SWAP(int32_t, bx, cx);
        SWAP(int32_t, by, cy);
    }

    int32_t total_height = cy - ay;
    if (ay != by) {
        int32_t segment_height = by - ay;
        for (int32_t y = ay; y <= by; ++y) {
            int32_t x1 = ax + ((cx - ax) * (y - ay)) / total_height;
            int32_t x2 = ax + ((bx - ax) * (y - ay)) / segment_height;
            for (int32_t x = MIN(x1, x2); x < MAX(x1, x2); ++x) {
                framebuffer_set_color(buf, x, y, color);
            }
        }
    }
    if (by != cy) {
        int32_t segment_height = cy - by;
        for (int32_t y = by; y <= cy; ++y) {
            int32_t x1 = ax + ((cx - ax) * (y - ay)) / total_height;
            int32_t x2 = bx + ((cx - bx) * (y - by)) / segment_height;
            for (int32_t x = MIN(x1, x2); x < MAX(x1, x2); ++x) {
                framebuffer_set_color(buf, x, y, color);
            }
        }
    }
}
#endif
#endif

#endif // UTILS_H