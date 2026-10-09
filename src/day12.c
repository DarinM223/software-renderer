#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define WIDTH 800
#define HEIGHT 800
#define SHADOW_WIDTH 8000
#define SHADOW_HEIGHT 8000
#define CHANNELS 3
#include "geometry.h"
#include "model.h"
#include "our_gl.h"
#include "stb_ds.h"
#include "stb_image.h"
#include "stb_image_write.h"
#include "utils.h"
#include <time.h>

extern mat4_4 model_view, perspective, viewport;
extern double *zbuffer;

typedef struct blank_shader {
    model const *model;
} blank_shader;

static vec4 blank_vertex(void *ctx, int32_t const face, int32_t const vert)
{
    blank_shader *shader = ctx;
    vec4 v = vec3_extend(model_vert_at_face(shader->model, face, vert), 1.);
    vec4 gl_Position = mat4_4_mul_vec4(&model_view, v);
    return mat4_4_mul_vec4(&perspective, gl_Position);
}

static bool blank_fragment(void *ctx, vec3 const bar, color *color)
{
    (void)ctx, (void)bar;
    *color = (struct color){255, 255, 255};
    return false;
}

static double smoothstep(double edge0, double edge1, double x)
{
    // Hermite interpolation
    double t = MIN(MAX(0., (x - edge0) / (edge1 - edge0)), 1.);
    return t * t * (3 - 2 * t);
}

int main(int32_t argc, char **argv)
{
    int err = 0;
    if (argc < 2) {
        printf("Model object needed as parameter\n");
        return 1;
    }

    model *models = NULL;
    for (int32_t i = 1; i < argc; ++i) {
        model model = {0};
        if (!model_init(&model, argv[i])) {
            printf("Couldn't load model from filename %s\n", argv[1]);
            return 1;
        }
        if ((err = !tga_load_extra(argv[1], "_nm_tangent.tga",
                                   &model.normal_map))) {
            printf("Couldn't load model normals for %s\n", argv[1]);
            goto cleanup_model;
        }
        arrpush(models, model);
    }

    vec3 const eye = {{-1, 0, 2}};
    vec3 const center = {{0, 0, 0}};
    vec3 const up = {{0, 1, 0}};

    // First pass: render from camera point of view:
    double camera_zbuffer[WIDTH * HEIGHT] = {0};
    uint8_t camera_buf[WIDTH * HEIGHT * CHANNELS] = {0};
    {
        for (size_t x = 0; x < WIDTH; ++x) {
            for (size_t y = 0; y < HEIGHT; ++y) {
                size_t idx = (y * WIDTH + x) * CHANNELS;
                camera_buf[idx] = 177;
                camera_buf[idx + 1] = 195;
                camera_buf[idx + 2] = 209;
            }
        }
        zbuffer_init(WIDTH, HEIGHT);
        look_at(eye, center, up);
        perspective_init(VEC_NORMALIZE(3, vec3_sub(eye, center)));
        viewport_init(WIDTH / 16, HEIGHT / 16, WIDTH * 7 / 8, HEIGHT * 7 / 8);

        blank_shader ctx = {0};
        shader const shader = {
            .vertex = &blank_vertex, .fragment = &blank_fragment, .ctx = &ctx};

        for (ptrdiff_t i = 0; i < arrlen(models); ++i) {
            ctx.model = &models[i];
            for (size_t j = 0; j < model_nfaces(ctx.model); ++j) {
                triangle clip = {shader.vertex(shader.ctx, j, 0),
                                 shader.vertex(shader.ctx, j, 1),
                                 shader.vertex(shader.ctx, j, 2)};
                rasterize(clip, shader, camera_buf, WIDTH, HEIGHT, CHANNELS);
            }
        }
        stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, camera_buf);
        // Save camera zbuffer to stack before deallocating.
        for (size_t x = 0; x < WIDTH; ++x) {
            for (size_t y = 0; y < HEIGHT; ++y) {
                camera_zbuffer[y * WIDTH + x] = zbuffer[y * WIDTH + x];
            }
        }
        zbuffer_deinit();
    }

    // Save camera matrix before switching state to the light's viewpoint.
    mat4_4 M;
    {
        mat4_4 const perspective_mv =
            mat4_4_mul_mat4_4(&perspective, &model_view);
        mat4_4 const times_viewport =
            mat4_4_mul_mat4_4(&viewport, &perspective_mv);
        M = mat4_4_invert(&times_viewport);
    }

    // Second pass: render from many lights points of views
    double mask[WIDTH * HEIGHT] = {0};
    srand(time(NULL));
    for (size_t i = 0; i < 1000; ++i) {
        double y = (double)rand() / (double)RAND_MAX;
        double theta = 2. * M_PI * ((double)rand() / (double)RAND_MAX);
        double r = sqrt(1. - y * y);
        vec3 light = {{r * cos(theta), y, r * sin(theta)}};
        look_at(light, center, up);
        perspective_init(VEC_NORMALIZE(3, vec3_sub(eye, center)));
        viewport_init(SHADOW_WIDTH / 16, SHADOW_HEIGHT / 16,
                      SHADOW_WIDTH * 7 / 8, SHADOW_HEIGHT * 7 / 8);
        zbuffer_init(SHADOW_WIDTH, SHADOW_HEIGHT);
        blank_shader ctx = {0};
        shader const shader = {
            .vertex = &blank_vertex, .fragment = &blank_fragment, .ctx = &ctx};

        uint8_t *shadow_buf = malloc(SHADOW_WIDTH * SHADOW_HEIGHT * CHANNELS);
        memset(shadow_buf, 0, SHADOW_WIDTH * SHADOW_HEIGHT * CHANNELS);
        for (ptrdiff_t i = 0; i < arrlen(models); ++i) {
            ctx.model = &models[i];
            for (size_t j = 0; j < model_nfaces(ctx.model); ++j) {
                triangle clip = {shader.vertex(shader.ctx, j, 0),
                                 shader.vertex(shader.ctx, j, 1),
                                 shader.vertex(shader.ctx, j, 2)};
                rasterize(clip, shader, shadow_buf, SHADOW_WIDTH, SHADOW_HEIGHT,
                          CHANNELS);
            }
        }

        mat4_4 N;
        {
            mat4_4 const perspective_mv =
                mat4_4_mul_mat4_4(&perspective, &model_view);
            N = mat4_4_mul_mat4_4(&viewport, &perspective_mv);
        }

        // Post-processing of fragments.
#pragma omp parallel for
        for (int32_t x = 0; x < WIDTH; ++x) {
            for (int32_t y = 0; y < HEIGHT; ++y) {
                vec4 fragment = mat4_4_mul_vec4(
                    &M, (vec4){{x, y, camera_zbuffer[y * WIDTH + x], 1.}});
                vec4 q = mat4_4_mul_vec4(&N, fragment);
                vec3 p = vec3_div_scalar(VEC_XYZ(q), VEC_W(q));
                bool lit =
                    // In background:
                    (VEC_Z(fragment) < -100) ||
                    // Out of bounds:
                    (VEC_X(p) < 0 || VEC_X(p) >= SHADOW_WIDTH || VEC_Y(p) < 0 ||
                     VEC_Y(p) >= SHADOW_HEIGHT) ||
                    // Is visible:
                    (VEC_Z(p) > zbuffer[(int32_t)VEC_X(p) +
                                        (int32_t)VEC_Y(p) * SHADOW_WIDTH] -
                                    .03);
                mask[y * WIDTH + x] += (lit - mask[y * WIDTH + x]) / (i + 1);
            }
        }

        free(shadow_buf);
        zbuffer_deinit();
    }

#pragma omp parallel for
    for (int32_t x = 0; x < WIDTH; ++x) {
        for (int32_t y = 0; y < HEIGHT; ++y) {
            double m = smoothstep(-1, 1, mask[y * WIDTH + x]);
            color c = framebuffer_get_color(camera_buf, x, y);
            framebuffer_set_color(
                camera_buf, x, y,
                (color){.r = c.r * m, .g = c.g * m, .b = c.b * m});
        }
    }
    stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, camera_buf);

    // Edge detection
    double const threshold = .15;
    for (int32_t y = 1; y < HEIGHT - 1; ++y) {
        for (int32_t x = 1; x < WIDTH - 1; ++x) {
            vec2 sum = {0};
            for (int32_t j = -1; j <= 1; ++j) {
                for (int32_t i = -1; i <= 1; ++i) {
                    int32_t const Gx[3][3] = {
                        {-1, 0, 1},
                        {-2, 0, 2},
                        {-1, 0, 1},
                    };
                    int32_t const Gy[3][3] = {
                        {-1, -2, -1},
                        {0, 0, 0},
                        {1, 2, 1},
                    };
                    sum = vec2_add(
                        sum,
                        (vec2){{Gx[j + 1][i + 1] *
                                    camera_zbuffer[(y + j) * WIDTH + x + i],
                                Gy[j + 1][i + 1] *
                                    camera_zbuffer[(y + j) * WIDTH + x + i]}});
                }
            }
            if (VEC_NORMALIZE(2, sum) > threshold) {
                framebuffer_set_color(camera_buf, x, y, (color){0, 0, 0});
            }
        }
    }
    stbi_write_tga("framebuffer3.tga", WIDTH, HEIGHT, CHANNELS, camera_buf);

cleanup_model:
    for (ptrdiff_t i = 0; i < arrlen(models); ++i) {
        model_deinit(&models[i]);
    }
    arrfree(models);
    return err;
}