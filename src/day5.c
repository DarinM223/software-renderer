#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "model.h"
#include "stb_ds.h"
#include "stb_image_write.h"

#define WIDTH 800
#define HEIGHT 800
#define CHANNELS 3
#include "utils.h"

static void vec3_project(vec3 v, int32_t *x, int32_t *y)
{
    *x = (v.x + 1.) * (WIDTH - 1) / 2;
    *y = (v.y + 1.) * (HEIGHT - 1) / 2;
}

static void vec3_project_z(vec3 v, int32_t *x, int32_t *y, int32_t *z)
{
    *x = (v.x + 1.) * (WIDTH - 1) / 2;
    *y = (v.y + 1.) * (HEIGHT - 1) / 2;
    *z = (v.z + 1.) * 255. / 2;
}

void framebuffer_draw_triangle_zbuffer(uint8_t *zbuffer, uint8_t *buf,
                                       int32_t ax, int32_t ay, int32_t az,
                                       int32_t bx, int32_t by, int32_t bz,
                                       int32_t cx, int32_t cy, int32_t cz,
                                       color color)
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
            uint8_t z = alpha * az + beta * bz + gamma * cz;
            // If pixel's z value is inferior to the one already painted, then
            // discard it.
            // NOTE: have to index zbuffer manually because using
            // framebuffer_set_color writes three values at once, which seems to
            // cause a race condition with OpenMP.
            if (z <= zbuffer[(HEIGHT - 1 - y) * WIDTH + x]) {
                continue;
            }
            zbuffer[(HEIGHT - 1 - y) * WIDTH + x] = z;
            framebuffer_set_color(buf, x, y, color);
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Model object needed as parameter\n");
        return 1;
    }

    model model = {0};
    if (!model_init(&model, argv[1])) {
        printf("Couldn't load model from filename %s\n", argv[1]);
        return 1;
    }

    // Per-triangle painter's algorithm. Sorts triangles by minimum z vertex
    // before drawing them.
    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
    model_sort_by_z(&model);
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        int32_t ax, ay, bx, by, cx, cy;
        vec3_project(model_vert_at_face(&model, i, 0), &ax, &ay);
        vec3_project(model_vert_at_face(&model, i, 1), &bx, &by);
        vec3_project(model_vert_at_face(&model, i, 2), &cx, &cy);
        color rnd = {.r = rand() % 255, .g = rand() % 255, .b = rand() % 255};
        framebuffer_draw_triangle(buf, ax, ay, bx, by, cx, cy, rnd);
    }
    stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, buf);

    // Per-pixel painter's algorithm using z-buffer.
    memset(buf, 0, sizeof(buf));
    uint8_t zbuffer[WIDTH * HEIGHT] = {0};
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        int32_t ax, ay, az, bx, by, bz, cx, cy, cz;
        vec3_project_z(model_vert_at_face(&model, i, 0), &ax, &ay, &az);
        vec3_project_z(model_vert_at_face(&model, i, 1), &bx, &by, &bz);
        vec3_project_z(model_vert_at_face(&model, i, 2), &cx, &cy, &cz);
        color rnd = {.r = rand() % 255, .g = rand() % 255, .b = rand() % 255};
        framebuffer_draw_triangle_zbuffer(zbuffer, buf, ax, ay, az, bx, by, bz,
                                          cx, cy, cz, rnd);
    }

    stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, buf);
    stbi_write_tga("zbuffer.tga", WIDTH, HEIGHT, 1, zbuffer);
    model_deinit(&model);
}