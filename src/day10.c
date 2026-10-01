#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "geometry.h"
#include "model.h"
#include "our_gl.h"
#include "stb_ds.h"
#include "stb_image.h"
#include "stb_image_write.h"

#define WIDTH 800
#define HEIGHT 800
#define CHANNELS 3

extern mat4_4 model_view, perspective;

typedef struct tangent_shader {
    vec4 l;
    model *model;
    vec4 tri[3];
    vec2 uv[3];
    vec4 norm[3];
    tga_image color_map;
    tga_image specular_map;
    double ambient;
    double shininess;
} tangent_shader;

bool tangent_shader_init(tangent_shader *shader, char const *model_name,
                         vec3 light)
{
    bool success = true;
    shader->l =
        VEC_NORMALIZED(4, mat4_4_mul_vec4(&model_view, vec3_extend(light, 0.)));
    if (!(success =
              tga_load_extra(model_name, "_diffuse.tga", &shader->color_map))) {
        return success;
    }
    if (!(success =
              tga_load_extra(model_name, "_spec.tga", &shader->specular_map))) {
        goto cleanup_color_map;
    }
    return success;
cleanup_color_map:
    stbi_image_free(shader->color_map.buf);
    return success;
}

void tangent_shader_free(tangent_shader *shader)
{
    stbi_image_free(shader->color_map.buf);
    stbi_image_free(shader->specular_map.buf);
}

static vec4 tangent_vertex(void *ctx, int32_t const face, int32_t const vert)
{
    tangent_shader *shader = ctx;
    shader->uv[vert] = model_uv_at_face(shader->model, face, vert);
    vec4 v = vec3_extend(model_vert_at_face(shader->model, face, vert), 1.);
    vec4 gl_Position = mat4_4_mul_vec4(&model_view, v);
    shader->tri[vert] = gl_Position;
    mat4_4 invert_transposed = mat4_4_invert_transpose(&model_view);
    shader->norm[vert] = mat4_4_mul_vec4(
        &invert_transposed,
        vec3_extend(model_norm_at_face(shader->model, face, vert), 0.));
    return mat4_4_mul_vec4(&perspective, gl_Position);
}

static bool tangent_fragment(void *ctx, vec3 const bar, color *color)
{
    (void)color;
    tangent_shader *shader = ctx;
    vec2 uv =
        vec2_add(vec2_mul_scalar_rhs(shader->uv[0], bar.data[0]),
                 vec2_add(vec2_mul_scalar_rhs(shader->uv[1], bar.data[1]),
                          vec2_mul_scalar_rhs(shader->uv[2], bar.data[2])));
    mat2_4 const E = {{vec4_sub(shader->tri[1], shader->tri[0]),
                       vec4_sub(shader->tri[2], shader->tri[0])}};
    mat2_2 const U = {{vec2_sub(shader->uv[1], shader->uv[0]),
                       vec2_sub(shader->uv[2], shader->uv[0])}};
    mat2_2 const U_inverted = mat2_2_invert(&U);
    mat2_4 const T = mat2_2_mul_mat2_4(&U_inverted, &E);
    mat4_4 const D = {
        {VEC_NORMALIZED(4, T.rows[0]),
         VEC_NORMALIZED(4, T.rows[1]),
         VEC_NORMALIZED(
             4,
             vec4_add(
                 vec4_mul_scalar_rhs(shader->norm[0], bar.data[0]),
                 vec4_add(vec4_mul_scalar_rhs(shader->norm[1], bar.data[1]),
                          vec4_mul_scalar_rhs(shader->norm[2], bar.data[2])))),
         {{0, 0, 0, 1}}}};
    mat4_4 const D_transposed = mat4_4_transpose(&D);
    vec4 n = VEC_NORMALIZED(
        4, mat4_4_mul_vec4(&D_transposed, model_norm_at_uv(shader->model, uv)));

    vec4 r = VEC_NORMALIZED(
        4, vec4_sub(vec4_mul_scalar_rhs(n, vec4_mul_dot(n, shader->l) * 2),
                    shader->l));
    *color = tga_sample_2d(&shader->color_map, uv);
    double diffuse = 1. * MAX(0, vec4_mul_dot(n, shader->l));
    double specular =
        (.5 + 2. * tga_sample_2d(&shader->specular_map, uv).r / 255.) *
        pow(MAX(0, VEC_Z(r)), shader->shininess);
    double brightness = shader->ambient + diffuse + specular;
    color->r = MIN(255, (uint32_t)color->r * brightness);
    color->g = MIN(255, (uint32_t)color->g * brightness);
    color->b = MIN(255, (uint32_t)color->b * brightness);
    return false;
}

int main(int32_t argc, char **argv)
{
    int err = 0;
    if (argc < 2) {
        printf("Model object needed as parameter\n");
        return 1;
    }

    model model = {0};
    if (!model_init(&model, argv[1])) {
        printf("Couldn't load model from filename %s\n", argv[1]);
        return 1;
    }
    if ((err =
             !tga_load_extra(argv[1], "_nm_tangent.tga", &model.normal_map))) {
        printf("Couldn't load model normals for %s\n", argv[1]);
        goto cleanup_model;
    }

    vec3 const light = {{1, 1, 1}};
    vec3 const eye = {{-1, 0, 2}};
    vec3 const center = {{0, 0, 0}};
    vec3 const up = {{0, 1, 0}};
    look_at(eye, center, up);
    perspective_init(VEC_NORMALIZE(3, vec3_sub(eye, center)));
    viewport_init(WIDTH / 16, HEIGHT / 16, WIDTH * 7 / 8, HEIGHT * 7 / 8);

    zbuffer_init(WIDTH, HEIGHT);
    tangent_shader ctx = {
        .model = &model,
        .ambient = 0.4,
        .shininess = 35,
    };
    if ((err = !tangent_shader_init(&ctx, argv[1], light))) {
        printf("Error initializing color specular shader\n");
        goto cleanup_zbuffer;
    }
    shader const shader = {
        .vertex = &tangent_vertex,
        .fragment = &tangent_fragment,
        .ctx = &ctx,
    };

    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        triangle clip = {shader.vertex(shader.ctx, i, 0),
                         shader.vertex(shader.ctx, i, 1),
                         shader.vertex(shader.ctx, i, 2)};
        rasterize(clip, shader, buf, WIDTH, HEIGHT, CHANNELS);
    }
    stbi_write_tga("framebuffer.tga", WIDTH, HEIGHT, CHANNELS, buf);
    tangent_shader_free(&ctx);
cleanup_zbuffer:
    zbuffer_deinit();
cleanup_model:
    model_deinit(&model);
    return err;
}