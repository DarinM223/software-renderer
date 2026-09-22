#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "geometry.h"
#include "model.h"
#include "our_gl.h"
#include "stb_ds.h"
#include "stb_image_write.h"

#define WIDTH 800
#define HEIGHT 800
#define CHANNELS 3

extern mat4_4 model_view, perspective;

typedef struct random_shader {
    model *model;
    vec3 tri[3];
    color color;
} random_shader;

static vec4 random_vertex(void *ctx, int32_t const face, int32_t const vert)
{
    random_shader *shader = ctx;
    vec3 v = model_vert_at_face(shader->model, face, vert);
    vec4 gl_Position = mat4_4_mul_vec4(&model_view, vec3_extend(v, 1.));
    shader->tri[vert] = VEC_XYZ(gl_Position);
    return mat4_4_mul_vec4(&perspective, gl_Position);
}

static bool random_fragment(void *ctx, vec3 const bar, color *color)
{
    (void)bar;
    random_shader *shader = ctx;
    *color = shader->color;
    return false; // do not discard pixel
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
    look_at(eye, center, up);
    perspective_init(VEC_NORMALIZE(3, vec3_sub(eye, center)));
    viewport_init(WIDTH / 16, HEIGHT / 16, WIDTH * 7 / 8, HEIGHT * 7 / 8);
    zbuffer_init(WIDTH, HEIGHT);

    random_shader ctx = {.model = &model, .color = {0}};
    shader const shader = {
        .vertex = &random_vertex,
        .fragment = &random_fragment,
        .ctx = &ctx,
    };

    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        ctx.color = (color){rand() % 255, rand() % 255, rand() % 255};
        triangle clip = {shader.vertex(shader.ctx, i, 0),
                         shader.vertex(shader.ctx, i, 1),
                         shader.vertex(shader.ctx, i, 2)};
        rasterize(clip, shader, buf, WIDTH, HEIGHT, CHANNELS);
    }
    stbi_write_tga("framebuffer.tga", WIDTH, HEIGHT, CHANNELS, buf);

    zbuffer_deinit();
    model_deinit(&model);
}