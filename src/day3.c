#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_DS_IMPLEMENTATION
#include "geometry.h"
#include "model.h"
#include "stb_ds.h"
#include "stb_image_write.h"
#include <omp.h>
#include <stdint.h>
#include <time.h>

#define WIDTH 128
#define HEIGHT 128
#define CHANNELS 3
#include "utils.h"

// My attempt at scanline rendering.
void draw_triangle_scanline_attempt1(uint8_t *buf, int32_t ax, int32_t ay,
                                     int32_t bx, int32_t by, int32_t cx,
                                     int32_t cy, color color)
{
    framebuffer_draw_line(buf, ax, ay, bx, by, color);
    framebuffer_draw_line(buf, bx, by, cx, cy, color);
    framebuffer_draw_line(buf, cx, cy, ax, ay, color);

    // Sort points by y.
    int32_t tmp;
    if (ay > by) {
        tmp = ay, ay = by, by = tmp;
        tmp = ax, ax = bx, bx = tmp;
    }
    if (by > cy) {
        tmp = by, by = cy, cy = tmp;
        tmp = bx, bx = cx, cx = tmp;
    }

    // Determine left and right side point.
    int32_t lx, ly, rx, ry;
    if (bx < cx) {
        lx = bx, ly = by, rx = cx, ry = cy;
    } else {
        lx = cx, ly = cy, rx = bx, ry = by;
    }

    // Fill lines starting from lowest y to highest y:
    for (int32_t y = ay; y <= cy; ++y) {
        int left_x, right_x;
        // If y > ly, use line from left to c, otherwise use line from a to
        // left.
        if (y > ly) {
            float const t = (y - ly) / (float)(cy - ly);
            left_x = round(lx + t * (cx - lx));
        } else {
            float const t = (y - ay) / (float)(ly - ay);
            left_x = round(ax + t * (lx - ax));
        }
        // If y > ry, use line from right to c, otherwise use line from a to
        // right.
        if (y > ry) {
            float const t = (y - ry) / (float)(cy - ry);
            right_x = round(rx + t * (cx - rx));
        } else {
            float const t = (y - ay) / (float)(ry - ay);
            right_x = round(ax + t * (rx - ax));
        }
        framebuffer_draw_line(buf, left_x, y, right_x, y, color);
    }
}

// The book's implementation of scanline rendering.
void draw_triangle_scanline_attempt2(uint8_t *buf, int32_t ax, int32_t ay,
                                     int32_t bx, int32_t by, int32_t cx,
                                     int32_t cy, color color)
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

// Draw the bounding boxes for each triangle.
void draw_bbox_rasterization(uint8_t *buf, int32_t ax, int32_t ay, int32_t bx,
                             int32_t by, int32_t cx, int32_t cy, color color)
{
    int32_t bbminx = MIN(MIN(ax, bx), cx);
    int32_t bbminy = MIN(MIN(ay, by), cy);
    int32_t bbmaxx = MAX(MAX(ax, bx), cx);
    int32_t bbmaxy = MAX(MAX(ay, by), cy);

#pragma omp parallel for
    for (int32_t x = bbminx; x < bbmaxx; ++x) {
        for (int32_t y = bbminy; y < bbmaxy; ++y) {
            framebuffer_set_color(buf, x, y, color);
        }
    }
}

// Color in bounding box except for the points that do not belong to the
// triangle ABC given vertices A, B, and C. We can express a vertex P as
// a combination of the triangle's vertices:
// P = alpha * A + beta * B + gamma * C, where alpha, beta, and gamma are
// weights to the vertex to make it become P. With this, the barycenter
// of the system is at the point P,and alpha, beta, and gamma are the
// barycentric coordinates.
//
// The barycentric coordinates are proportional to subtriangle areas
// Area(PBC), Area(PCA), and Area(PAB). If all three coordinates are
// positive, P must be inside the triangle ABC.
void draw_triangle_rasterization_attempt1(uint8_t *buf, int32_t ax, int32_t ay,
                                          int32_t bx, int32_t by, int32_t cx,
                                          int32_t cy, color color)
{
    int32_t bbminx = MIN(MIN(ax, bx), cx);
    int32_t bbminy = MIN(MIN(ay, by), cy);
    int32_t bbmaxx = MAX(MAX(ax, bx), cx);
    int32_t bbmaxy = MAX(MAX(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);

#pragma omp parallel for
    for (int32_t x = bbminx; x < bbmaxx; ++x) {
        for (int32_t y = bbminy; y < bbmaxy; ++y) {
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

// Improve rendering by culling backfaces. If the signed area is negative,
// its back-facing, if it's positive, its front-facing.
void draw_triangle_rasterization_attempt2(uint8_t *buf, int32_t ax, int32_t ay,
                                          int32_t bx, int32_t by, int32_t cx,
                                          int32_t cy, color color)
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
    for (int32_t x = bbminx; x < bbmaxx; ++x) {
        for (int32_t y = bbminy; y < bbmaxy; ++y) {
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

static void vec3_project(vec3 v, int32_t *x, int32_t *y)
{
    *x = (v.x + 1.) * (WIDTH - 1) / 2;
    *y = (v.y + 1.) * (HEIGHT - 1) / 2;
}

int main(void)
{
    srand(time(NULL));
    uint8_t buf[WIDTH * HEIGHT * CHANNELS];

    memset(buf, 0, sizeof(buf));
    draw_triangle_scanline_attempt1(buf, 7, 45, 35, 100, 45, 60, red);
    draw_triangle_scanline_attempt1(buf, 120, 35, 90, 5, 45, 110, white);
    draw_triangle_scanline_attempt1(buf, 115, 83, 80, 90, 85, 120, green);
    stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, buf);

    memset(buf, 0, sizeof(buf));
    draw_triangle_scanline_attempt2(buf, 7, 45, 35, 100, 45, 60, red);
    draw_triangle_scanline_attempt2(buf, 120, 35, 90, 5, 45, 110, white);
    draw_triangle_scanline_attempt2(buf, 115, 83, 80, 90, 85, 120, green);
    stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, buf);

    memset(buf, 0, sizeof(buf));
    draw_bbox_rasterization(buf, 7, 45, 35, 100, 45, 60, red);
    draw_bbox_rasterization(buf, 120, 35, 90, 5, 45, 110, white);
    draw_bbox_rasterization(buf, 115, 83, 80, 90, 85, 120, green);
    stbi_write_tga("framebuffer3.tga", WIDTH, HEIGHT, CHANNELS, buf);

    memset(buf, 0, sizeof(buf));
    draw_triangle_rasterization_attempt1(buf, 7, 45, 35, 100, 45, 60, red);
    draw_triangle_rasterization_attempt1(buf, 120, 35, 90, 5, 45, 110, white);
    draw_triangle_rasterization_attempt1(buf, 115, 83, 80, 90, 85, 120, green);
    stbi_write_tga("framebuffer4.tga", WIDTH, HEIGHT, CHANNELS, buf);

    model model = {0};
    char *const filename = "obj/african_head.obj";
    if (!model_init(&model, filename)) {
        printf("Couldn't load model from filename %s\n", filename);
        return 1;
    }

    memset(buf, 0, sizeof(buf));
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        int32_t ax, ay, bx, by, cx, cy;
        vec3_project(model_vert_at_face(&model, i, 0), &ax, &ay);
        vec3_project(model_vert_at_face(&model, i, 1), &bx, &by);
        vec3_project(model_vert_at_face(&model, i, 2), &cx, &cy);
        draw_triangle_rasterization_attempt1(
            buf, ax, ay, bx, by, cx, cy,
            (color){rand() % 255, rand() % 255, rand() % 255});
    }
    stbi_write_tga("framebuffer5.tga", WIDTH, HEIGHT, CHANNELS, buf);

    memset(buf, 0, sizeof(buf));
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        int32_t ax, ay, bx, by, cx, cy;
        vec3_project(model_vert_at_face(&model, i, 0), &ax, &ay);
        vec3_project(model_vert_at_face(&model, i, 1), &bx, &by);
        vec3_project(model_vert_at_face(&model, i, 2), &cx, &cy);
        draw_triangle_rasterization_attempt2(
            buf, ax, ay, bx, by, cx, cy,
            (color){rand() % 255, rand() % 255, rand() % 255});
    }
    stbi_write_tga("framebuffer6.tga", WIDTH, HEIGHT, CHANNELS, buf);

    model_deinit(&model);
}