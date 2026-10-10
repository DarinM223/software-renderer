#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define WIDTH 800
#define HEIGHT 800
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

typedef struct toon_shader {
    model *model;
    vec3 color;
    vec4 norms[3];
    double ambient;
    double shininess;
    vec4 l;
} toon_shader;

void toon_shader_init(toon_shader *shader, vec3 color, vec3 light)
{
    // extend light with 0. not 1. so the camera translation doesn't affect it.
    shader->l =
        VEC_NORMALIZED(4, mat4_4_mul_vec4(&model_view, vec3_extend(light, 0.)));
    shader->color = color;
}

static vec4 toon_vertex(void *ctx, int32_t const face, int32_t const vert)
{
    toon_shader *shader = ctx;
    vec4 v = vec3_extend(model_vert_at_face(shader->model, face, vert), 1.);
    vec4 n = vec3_extend(model_norm_at_face(shader->model, face, vert), 0.);
    vec4 gl_Position = mat4_4_mul_vec4(&model_view, v);
    // If a model is transformed by the matrix M, we can convert its normal
    // vectors by the matrix (M^(-1))^T
    mat4_4 invert_transposed = mat4_4_invert_transpose(&model_view);
    shader->norms[vert] = mat4_4_mul_vec4(&invert_transposed, n);
    return mat4_4_mul_vec4(&perspective, gl_Position);
}

static bool toon_fragment(void *ctx, vec3 const bar, color *color)
{
    toon_shader *shader = ctx;
    vec4 n = VEC_NORMALIZED(
        4,
        vec4_add(vec4_mul_scalar_lhs(bar.data[0], shader->norms[0]),
                 vec4_add(vec4_mul_scalar_lhs(bar.data[1], shader->norms[1]),
                          vec4_mul_scalar_lhs(bar.data[2], shader->norms[2]))));
    double diffuse = MAX(0, vec4_mul_dot(n, shader->l));
    double intensity = .15 + diffuse;
    if (intensity > .66) {
        intensity = 1;
    } else if (intensity > .33) {
        intensity = .66;
    } else {
        intensity = .33;
    }
    *color = (struct color){.r = MIN(255, shader->color.data[0] * intensity),
                            .g = MIN(255, shader->color.data[1] * intensity),
                            .b = MIN(255, shader->color.data[2] * intensity)};
    return false;
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
        if ((err = !model_init(&model, argv[i]))) {
            printf("Couldn't load model from filename %s\n", argv[1]);
            goto cleanup_model;
        }
        if ((err = !tga_load_extra(argv[1], "_nm_tangent.tga",
                                   &model.normal_map))) {
            printf("Couldn't load model normals for %s\n", argv[1]);
            goto cleanup_model;
        }
        arrpush(models, model);
    }

    vec3 const light = {{1, 1, 1}};
    vec3 const eye = {{-1, 0, 2}};
    vec3 const center = {{0, 0, 0}};
    vec3 const up = {{0, 1, 0}};
    vec3 const colors[] = {
        {{88, 98, 123}},             // Dark color for floor
        {{147 * 4, 56 * 4, 22 * 4}}, // Lighter color for diablo model
    };

    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
    {
        for (size_t x = 0; x < WIDTH; ++x) {
            for (size_t y = 0; y < HEIGHT; ++y) {
                size_t idx = (y * WIDTH + x) * CHANNELS;
                buf[idx] = 177;
                buf[idx + 1] = 195;
                buf[idx + 2] = 209;
            }
        }
        zbuffer_init(WIDTH, HEIGHT);
        look_at(eye, center, up);
        perspective_init(VEC_NORMALIZE(3, vec3_sub(eye, center)));
        viewport_init(WIDTH / 16, HEIGHT / 16, WIDTH * 7 / 8, HEIGHT * 7 / 8);

        toon_shader ctx = {0};

        for (ptrdiff_t i = 0; i < arrlen(models); ++i) {
            toon_shader_init(&ctx, colors[i], light);
            shader const shader = {.vertex = &toon_vertex,
                                   .fragment = &toon_fragment,
                                   .ctx = &ctx};
            ctx.model = &models[i];
            for (size_t j = 0; j < model_nfaces(ctx.model); ++j) {
                triangle clip = {shader.vertex(shader.ctx, j, 0),
                                 shader.vertex(shader.ctx, j, 1),
                                 shader.vertex(shader.ctx, j, 2)};
                rasterize(clip, shader, buf, WIDTH, HEIGHT, CHANNELS);
            }
        }
        stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, buf);
    }

    // Post processing: Sobel edge detection
    double const threshold = .15;
    // Convolution kernel that emphasizes horizontal changes
    // in intensity (detecting vertical edges)
    int32_t const Kx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    // Convolution kernel that emphasizes vertical changes
    // in intensity (detecting horizontal edges)
    int32_t const Ky[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    for (int32_t y = 1; y < HEIGHT - 1; ++y) {
        for (int32_t x = 1; x < WIDTH - 1; ++x) {
            vec2 sum = {0};
            for (int32_t j = -1; j <= 1; ++j) {
                for (int32_t i = -1; i <= 1; ++i) {
                    double Gx =
                        Kx[j + 1][i + 1] * zbuffer[(y + j) * WIDTH + x + i];
                    double Gy =
                        Ky[j + 1][i + 1] * zbuffer[(y + j) * WIDTH + x + i];
                    sum = vec2_add(sum, (vec2){{Gx, Gy}});
                }
            }
            if (VEC_NORMALIZE(2, sum) > threshold) {
                framebuffer_set_color(buf, x, y, (color){0, 0, 0});
            }
        }
    }
    stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, buf);

cleanup_model:
    zbuffer_deinit();
    for (ptrdiff_t i = 0; i < arrlen(models); ++i) {
        model_deinit(&models[i]);
    }
    arrfree(models);
    return err;
}