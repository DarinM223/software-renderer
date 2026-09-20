#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "geometry.h"
#include "model.h"
#include "stb_ds.h"
#include "stb_image_write.h"
#include <math.h>

#define WIDTH 800
#define HEIGHT 800
#define CHANNELS 3
#include "utils.h"

static vec3 vec3_rot(vec3 v)
{
    double const a = M_PI / 6;
    mat3_3 const Ry = {
        {{{cos(a), 0, sin(a)}}, {{0, 1, 0}}, {{-sin(a), 0, cos(a)}}}};
    return mat3_3_mul_vec3(&Ry, &v);
}

static vec3 vec3_perspective(vec3 v)
{
    double const c = 3.;
    return vec3_div_scalar(v, 1 - VEC_Z(v) / c);
}

static void vec3_project(vec3 v, int32_t *x, int32_t *y, int32_t *z)
{
    *x = (VEC_X(v) + 1.) * WIDTH / 2;
    *y = (VEC_Y(v) + 1.) * HEIGHT / 2;
    *z = (VEC_Z(v) + 1.) * 255. / 2;
}

int main(int32_t argc, char **argv)
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

    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
    uint8_t zbuffer[WIDTH * HEIGHT] = {0};

    // Orthographic projection: the depth (z coordinate) doesn't influence
    // the size or shape of objects.
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        int32_t ax, ay, az, bx, by, bz, cx, cy, cz;
        vec3_project(vec3_rot(model_vert_at_face(&model, i, 0)), &ax, &ay, &az);
        vec3_project(vec3_rot(model_vert_at_face(&model, i, 1)), &bx, &by, &bz);
        vec3_project(vec3_rot(model_vert_at_face(&model, i, 2)), &cx, &cy, &cz);
        color rnd = {.r = rand() % 255, .g = rand() % 255, .b = rand() % 255};
        framebuffer_draw_triangle_z(zbuffer, buf, ax, ay, az, bx, by, bz, cx,
                                    cy, cz, rnd);
    }
    stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, buf);
    stbi_write_tga("zbuffer1.tga", WIDTH, HEIGHT, 1, zbuffer);

    // Central (perspective) projection: closer objects appear larger than
    // distance ones.
    memset(buf, 0, sizeof(buf));
    memset(zbuffer, 0, sizeof(zbuffer));
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        int32_t ax, ay, az, bx, by, bz, cx, cy, cz;
        vec3_project(
            vec3_perspective(vec3_rot(model_vert_at_face(&model, i, 0))), &ax,
            &ay, &az);
        vec3_project(
            vec3_perspective(vec3_rot(model_vert_at_face(&model, i, 1))), &bx,
            &by, &bz);
        vec3_project(
            vec3_perspective(vec3_rot(model_vert_at_face(&model, i, 2))), &cx,
            &cy, &cz);
        color rnd = {.r = rand() % 255, .g = rand() % 255, .b = rand() % 255};
        framebuffer_draw_triangle_z(zbuffer, buf, ax, ay, az, bx, by, bz, cx,
                                    cy, cz, rnd);
    }
    stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, buf);
    stbi_write_tga("zbuffer2.tga", WIDTH, HEIGHT, 1, zbuffer);

    // Fix clipping with diablo model by using floats for zbuffer instead of u8.
    memset(buf, 0, sizeof(buf));
    float zbufferf[WIDTH * HEIGHT] = {0};
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        int32_t ax, ay, az, bx, by, bz, cx, cy, cz;
        vec3_project(
            vec3_perspective(vec3_rot(model_vert_at_face(&model, i, 0))), &ax,
            &ay, &az);
        vec3_project(
            vec3_perspective(vec3_rot(model_vert_at_face(&model, i, 1))), &bx,
            &by, &bz);
        vec3_project(
            vec3_perspective(vec3_rot(model_vert_at_face(&model, i, 2))), &cx,
            &cy, &cz);
        color rnd = {.r = rand() % 255, .g = rand() % 255, .b = rand() % 255};
        framebuffer_draw_triangle_z_float(zbufferf, buf, ax, ay, az, bx, by, bz,
                                          cx, cy, cz, rnd);
    }
    stbi_write_tga("framebuffer3.tga", WIDTH, HEIGHT, CHANNELS, buf);
    // Convert to image by clamping depth values to 255.
    memset(zbuffer, 0, sizeof(zbuffer));
    for (size_t x = 0; x < WIDTH; ++x) {
        for (size_t y = 0; y < HEIGHT; ++y) {
            zbuffer[y * WIDTH + x] =
                zbufferf[y * WIDTH + x] > 255 ? 255 : zbufferf[y * WIDTH + x];
        }
    }
    stbi_write_tga("zbuffer3.tga", WIDTH, HEIGHT, 1, zbuffer);

    model_deinit(&model);
}