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

static double dist(void)
{
    double const min = -.1, max = .1;
    return min + ((double)rand() / RAND_MAX) * (max - min);
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

    vec3 const eye = {{-1, 0, 2}};
    vec3 const center = {{0, 0, 0}};
    vec3 const up = {{0, 1, 0}};

    // First pass: render from camera point of view:
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
    }

    mat4_4 const M = mat4_4_invert(&viewport);
    double const ao_radius = .1;
    int const nsamples = 128;

    // Post processing for fragments:
    // For each fragment: sample some points around it
    // and check the zbuffer for these points.
#pragma omp parallel for
    for (int32_t x = 0; x < WIDTH; ++x) {
        for (int32_t y = 0; y < HEIGHT; ++y) {
            double z = zbuffer[y * WIDTH + x];
            if (z < -100) continue;
            vec4 const fragment = mat4_4_mul_vec4(&M, (vec4){{x, y, z, 1.}});
            double vote = 0;
            double voters = 0;
            for (int32_t i = 0; i < nsamples; ++i) {
                // Randomly sample a point around the fragment.
                vec4 p = mat4_4_mul_vec4(
                    &viewport,
                    vec4_add(fragment, (vec4){{dist(), dist(), dist(), 0.}}));
                if (VEC_X(p) < 0 || VEC_X(p) >= WIDTH || VEC_Y(p) < 0 ||
                    VEC_Y(p) >= HEIGHT) {
                    continue;
                }

                double d = zbuffer[(int)VEC_Y(p) * WIDTH + (int)VEC_X(p)];
                // Ignore sampled points with z values deeper than the expected
                // radius around the original z value.
                if (z + 5 * ao_radius < d) {
                    continue;
                }

                // If the sampled point depth is greater than the fragment's
                // depth, contribute to its ambient shadowing.
                voters++;
                vote += d > VEC_Z(p);
            }

            double ssao = smoothstep(0, 1, 1 - vote / voters * .4);
            color c = framebuffer_get_color(camera_buf, x, y);
            framebuffer_set_color(camera_buf, x, y,
                                  (struct color){.r = c.r * ssao,
                                                 .g = c.g * ssao,
                                                 .b = c.b * ssao});
        }
    }
    stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, camera_buf);

cleanup_model:
    zbuffer_deinit();
    for (ptrdiff_t i = 0; i < arrlen(models); ++i) {
        model_deinit(&models[i]);
    }
    arrfree(models);
    return err;
}