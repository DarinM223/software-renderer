#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "geometry.h"
#include "model.h"
#include "stb_ds.h"
#include "stb_image_write.h"
#include <float.h>

#define WIDTH 800
#define HEIGHT 800
#define CHANNELS 3
#include "utils.h"

mat4_4 viewport_create(int32_t const x, int32_t const y, int32_t const w,
                       int32_t const h)
{
    mat4_4 viewport = {{{{w / 2., 0, 0, x + w / 2.}},
                        {{0, h / 2., 0, y + h / 2.}},
                        {{0, 0, 1, 0}},
                        {{0, 0, 0, 1}}}};
    return viewport;
}

mat4_4 perspective_create(double const f)
{
    mat4_4 perspective = {
        {{{1, 0, 0, 0}}, {{0, 1, 0, 0}}, {{0, 0, 1, 0}}, {{0, 0, -1 / f, 1}}}};
    return perspective;
}

mat4_4 look_at(vec3 const eye, vec3 const center, vec3 const up)
{
    vec3 n = VEC_NORMALIZED(3, vec3_sub(eye, center));
    vec3 l = VEC_NORMALIZED(3, vec3_cross(up, n));
    vec3 m = VEC_NORMALIZED(3, vec3_cross(n, l));
    mat4_4 basis = {{vec3_extend(l, 0),
                     vec3_extend(m, 0),
                     vec3_extend(n, 0),
                     {{0, 0, 0, 1}}}};
    mat4_4 translate = {{{{1, 0, 0, -VEC_X(center)}},
                         {{0, 1, 0, -VEC_Y(center)}},
                         {{0, 0, 1, -VEC_Z(center)}},
                         {{0, 0, 0, 1}}}};
    return mat4_4_mul_mat4_4(&basis, &translate);
}

static void framebuffer_rasterize(mat4_4 const *viewport, vec4 const *clip,
                                  double *zbuffer, uint8_t *buf, color color)
{
    vec4 const ndc[3] = {vec4_div_scalar(clip[0], VEC_W(clip[0])),
                         vec4_div_scalar(clip[1], VEC_W(clip[1])),
                         vec4_div_scalar(clip[2], VEC_W(clip[2]))};
    vec2 const screen[3] = {VEC_XY(mat4_4_mul_vec4(viewport, ndc[0])),
                            VEC_XY(mat4_4_mul_vec4(viewport, ndc[1])),
                            VEC_XY(mat4_4_mul_vec4(viewport, ndc[2]))};
    mat3_3 const abc = {{vec2_extend(screen[0], 1.), vec2_extend(screen[1], 1.),
                         vec2_extend(screen[2], 1.)}};
    // Backface culling + discarding triangles that cover less than a pixel.
    if (mat3_3_det(&abc) < 1) {
        return;
    }
    double const bbminx =
        MIN(MIN(VEC_X(screen[0]), VEC_X(screen[1])), VEC_X(screen[2]));
    double const bbminy =
        MIN(MIN(VEC_Y(screen[0]), VEC_Y(screen[1])), VEC_Y(screen[2]));
    double const bbmaxx =
        MAX(MAX(VEC_X(screen[0]), VEC_X(screen[1])), VEC_X(screen[2]));
    double const bbmaxy =
        MAX(MAX(VEC_Y(screen[0]), VEC_Y(screen[1])), VEC_Y(screen[2]));

#pragma omp parallel for
    for (int32_t x = MAX(0, (int64_t)bbminx);
         x <= MIN(WIDTH - 1, (int64_t)bbmaxx); ++x) {
        for (int32_t y = MAX(0, (int64_t)bbminy);
             y <= MIN(HEIGHT - 1, (int64_t)bbmaxy); ++y) {
            // Determine barycentric coordinates for (x, y).
            mat3_3 invert_transpose = mat3_3_invert_transpose(&abc);
            vec3 barycentric =
                mat3_3_mul_vec3(&invert_transpose, (vec3){{x, y, 1.}});
            // Don't color coordinates that are not in the triangle.
            if (VEC_X(barycentric) < 0 || VEC_Y(barycentric) < 0 ||
                VEC_Z(barycentric) < 0) {
                continue;
            }
            double z = vec3_mul_dot(
                barycentric,
                (vec3){{VEC_Z(ndc[0]), VEC_Z(ndc[1]), VEC_Z(ndc[2])}});
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

    vec3 const eye = {{-1, 0, 2}};
    vec3 const center = {{0, 0, 0}};
    vec3 const up = {{0, 1, 0}};
    mat4_4 model_view = look_at(eye, center, up);
    mat4_4 perspective =
        perspective_create(VEC_NORMALIZE(3, vec3_sub(eye, center)));
    mat4_4 viewport =
        viewport_create(WIDTH / 16, HEIGHT / 16, WIDTH * 7 / 8, HEIGHT * 7 / 8);

    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
    double zbuffer[WIDTH * HEIGHT];
    // Initialize zbuffer to the lowest negative double because z values can be
    // negative now.
    for (size_t i = 0; i < WIDTH * HEIGHT; ++i) {
        zbuffer[i] = -DBL_MAX;
    }
    mat4_4 const composed = mat4_4_mul_mat4_4(&perspective, &model_view);
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        vec4 clip[3];
        for (size_t d = 0; d < 3; ++d) {
            vec3 const v = model_vert_at_face(&model, i, d);
            clip[d] = mat4_4_mul_vec4(&composed, vec3_extend(v, 1.));
        }
        color rnd = {.r = rand() % 255, .g = rand() % 255, .b = rand() % 255};
        framebuffer_rasterize(&viewport, clip, zbuffer, buf, rnd);
    }

    stbi_write_tga("framebuffer.tga", WIDTH, HEIGHT, CHANNELS, buf);
    model_deinit(&model);
}