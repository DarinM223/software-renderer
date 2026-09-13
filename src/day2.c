#include <sys/types.h>
#include <time.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "math.h"
#include "stb_image_write.h"
#include <stdbool.h>

#define WIDTH 64
#define HEIGHT 64
#define CHANNELS 3

#include "utils.h"

// Step 1. Drawing lines between points to make a triangle.
//
// x(t) = a_x + t * (b_x - a_x)
// y(t) = a_y + t * (b_y - a_y)
// when t = 0, x = a_x, y = a_y
// when t = 1, x = b_x, y = b_y
// As t increases, the point moves closer to the second point.

// Attempt 1. Start at t = 0 and increase by a very small float.
void framebuffer_draw_line_attempt1(uint8_t *buf, int32_t a_x, int32_t a_y,
                                    int32_t b_x, int32_t b_y, color color)
{
    for (float t = 0.0; t < 1.0; t += 0.02) {
        int const x = round(a_x + t * (b_x - a_x));
        int const y = round(a_y + t * (b_y - a_y));
        framebuffer_set_color(buf, x, y, color);
    }
}

// Attempt 2. Get t from x using the equation and then calculate y from t.
void framebuffer_draw_line_attempt2(uint8_t *buf, int32_t a_x, int32_t a_y,
                                    int32_t b_x, int32_t b_y, color color)
{
    for (int32_t x = a_x; x <= b_x; ++x) {
        float const t = (x - a_x) / (float)(b_x - a_x);
        int const y = round(a_y + t * (b_y - a_y));
        framebuffer_set_color(buf, x, y, color);
    }
}

// Attempt 3. Swap xs if going from high to low and transpose x and y for steep
// vertical lines.
void framebuffer_draw_line_attempt3(uint8_t *buf, int32_t a_x, int32_t a_y,
                                    int32_t b_x, int32_t b_y, color color)
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
    for (int32_t x = a_x; x <= b_x; ++x) {
        float const t = (a_x == b_x) ? a_x : (x - a_x) / (float)(b_x - a_x);
        int32_t const y = round(a_y + t * (b_y - a_y));
        // Then de-transpose x and y when writing pixel.
        if (steep) {
            framebuffer_set_color(buf, y, x, color);
        } else {
            framebuffer_set_color(buf, x, y, color);
        }
    }
}

// Optimized version that uses branchless form to improve loop performance.
void framebuffer_draw_line_attempt4(uint8_t *buf, int32_t a_x, int32_t a_y,
                                    int32_t b_x, int32_t b_y, color color)
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

static int32_t const ax = 7, ay = 3, bx = 12, by = 37, cx = 62, cy = 53;

static void framebuffer_init(uint8_t *buf, size_t size)
{
    memset(buf, 0, size);
    framebuffer_set_color(buf, ax, ay, white);
    framebuffer_set_color(buf, bx, by, white);
    framebuffer_set_color(buf, cx, cy, white);
}

int main(void)
{
    static uint8_t buf[WIDTH * HEIGHT * CHANNELS];

    framebuffer_init(buf, sizeof(buf));
    framebuffer_draw_line_attempt1(buf, ax, ay, bx, by, blue);
    framebuffer_draw_line_attempt1(buf, cx, cy, bx, by, green);
    framebuffer_draw_line_attempt1(buf, cx, cy, ax, ay, yellow);
    framebuffer_draw_line_attempt1(buf, ax, ay, cx, cy, red);
    stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, buf);

    // For steep vertical lines, sampling x will make huge gaps in the line.
    framebuffer_init(buf, sizeof(buf));
    framebuffer_draw_line_attempt2(buf, ax, ay, bx, by, blue);
    framebuffer_draw_line_attempt2(buf, cx, cy, bx, by, green);
    framebuffer_draw_line_attempt2(buf, cx, cy, ax, ay, yellow);
    framebuffer_draw_line_attempt2(buf, ax, ay, cx, cy, red);
    stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, buf);

    // Fixes bugs sampling x from high to low and steep vertical lines.
    framebuffer_init(buf, sizeof(buf));
    framebuffer_draw_line_attempt3(buf, ax, ay, bx, by, blue);
    framebuffer_draw_line_attempt3(buf, cx, cy, bx, by, green);
    framebuffer_draw_line_attempt3(buf, cx, cy, ax, ay, yellow);
    framebuffer_draw_line_attempt3(buf, ax, ay, cx, cy, red);
    stbi_write_tga("framebuffer3.tga", WIDTH, HEIGHT, CHANNELS, buf);

    // Benchmark for optimized version.
    srand(time(NULL));
    framebuffer_init(buf, sizeof(buf));
    for (size_t i = 0; i < (1 << 24); ++i) {
        int32_t const ax = rand() % WIDTH, ay = rand() % HEIGHT;
        int32_t const bx = rand() % WIDTH, by = rand() % HEIGHT;
        framebuffer_draw_line_attempt4(
            buf, ax, ay, bx, by,
            (color){rand() % 255, rand() % 255, rand() % 255});
    }
    stbi_write_tga("framebuffer4.tga", WIDTH, HEIGHT, CHANNELS, buf);
}