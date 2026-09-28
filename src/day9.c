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

typedef struct smooth_shader {
    model *model;
    vec3 norms[3];
    double ambient;
    double shininess;
    vec3 l;
} smooth_shader;

void smooth_shader_init(smooth_shader *shader, vec3 light)
{
    // extend light with 0. not 1. so the camera translation doesn't affect it.
    shader->l = VEC_XYZ(VEC_NORMALIZED(
        4, mat4_4_mul_vec4(&model_view, vec3_extend(light, 0.))));
}

static vec4 smooth_vertex(void *ctx, int32_t const face, int32_t const vert)
{
    smooth_shader *shader = ctx;
    vec3 v = model_vert_at_face(shader->model, face, vert);
    vec3 n = model_norm_at_face(shader->model, face, vert);
    vec4 gl_Position = mat4_4_mul_vec4(&model_view, vec3_extend(v, 1.));
    // If a model is transformed by the matrix M, we can convert its normal
    // vectors by the matrix (M^(-1))^T
    mat4_4 invert_transposed = mat4_4_invert_transpose(&model_view);
    shader->norms[vert] =
        VEC_XYZ(mat4_4_mul_vec4(&invert_transposed, vec3_extend(n, 0.)));
    return mat4_4_mul_vec4(&perspective, gl_Position);
}

static bool smooth_fragment(void *ctx, vec3 const bar, color *color)
{
    (void)color;
    smooth_shader *shader = ctx;
    vec3 n = VEC_NORMALIZED(
        3,
        vec3_add(vec3_mul_scalar_lhs(bar.data[0], shader->norms[0]),
                 vec3_add(vec3_mul_scalar_lhs(bar.data[1], shader->norms[1]),
                          vec3_mul_scalar_lhs(bar.data[2], shader->norms[2]))));
    double diffuse = MAX(0, vec3_mul_dot(n, shader->l));
    vec3 r = vec3_sub(vec3_mul_scalar_rhs(n, vec3_mul_dot(n, shader->l) * 2),
                      shader->l);
    // note that the camera lies on the z-axis (in eye coordinates), therefore
    // r.z, since (0, 0, 1) * (r.x, r.y, r.z) = r.z
    double specular = pow(MAX(0, VEC_Z(r)), shader->shininess);
    double brightness = MIN(1, shader->ambient + .4 * diffuse + .9 * specular);
    *color = (struct color){
        .r = 255. * brightness, .g = 255. * brightness, .b = 255. * brightness};
    return false;
}

typedef struct sample_shader {
    vec4 l;
    model *model;
    vec2 uv[3];
    double ambient;
    double shininess;
} sample_shader;

void sample_shader_init(sample_shader *shader, vec3 light)
{
    // extend light with 0. not 1. so the camera translation doesn't affect it.
    shader->l =
        VEC_NORMALIZED(4, mat4_4_mul_vec4(&model_view, vec3_extend(light, 0.)));
}

static vec4 sample_vertex(void *ctx, int32_t const face, int32_t const vert)
{
    sample_shader *shader = ctx;
    vec3 v = model_vert_at_face(shader->model, face, vert);
    // vec3 n = model_norm_at_face(shader->model, face, vert);
    vec4 gl_Position = mat4_4_mul_vec4(&model_view, vec3_extend(v, 1.));
    // If a model is transformed by the matrix M, we can convert its normal
    // vectors by the matrix (M^(-1))^T
    // mat4_4 invert_transposed = mat4_4_invert_transpose(&model_view);
    // shader->norms[vert] =
    //     VEC_XYZ(mat4_4_mul_vec4(&invert_transposed, vec3_extend(n, 0.)));
    return mat4_4_mul_vec4(&perspective, gl_Position);
}

static bool sample_fragment(void *ctx, vec3 const bar, color *color)
{
    (void)color;
    sample_shader *shader = ctx;
    // vec3 n = VEC_NORMALIZED(
    //     3,
    //     vec3_add(vec3_mul_scalar_lhs(bar.data[0], shader->norms[0]),
    //              vec3_add(vec3_mul_scalar_lhs(bar.data[1], shader->norms[1]),
    //                       vec3_mul_scalar_lhs(bar.data[2],
    //                       shader->norms[2]))));
    // double diffuse = MAX(0, vec3_mul_dot(n, shader->l));
    // vec3 r = vec3_sub(vec3_mul_scalar_rhs(n, vec3_mul_dot(n, shader->l) * 2),
    //                   shader->l);
    // // note that the camera lies on the z-axis (in eye coordinates),
    // therefore
    // // r.z, since (0, 0, 1) * (r.x, r.y, r.z) = r.z
    // double specular = pow(MAX(0, VEC_Z(r)), shader->shininess);
    // double brightness = MIN(1, shader->ambient + .4 * diffuse + .9 *
    // specular); *color = (struct color){
    //     .r = 255. * brightness, .g = 255. * brightness, .b = 255. *
    //     brightness};
    return false;
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

    vec3 const light = {{1, 1, 1}};
    vec3 const eye = {{-1, 0, 2}};
    vec3 const center = {{0, 0, 0}};
    vec3 const up = {{0, 1, 0}};
    look_at(eye, center, up);
    perspective_init(VEC_NORMALIZE(3, vec3_sub(eye, center)));
    viewport_init(WIDTH / 16, HEIGHT / 16, WIDTH * 7 / 8, HEIGHT * 7 / 8);

    /*
    When drawing line segments we can interpolate between them:
    P(t) = (1 - t)*P0 + t*P1
    This is called linear interpolation between P0 and P1.

    If we use a higher order polynomial:
    P(t) = (1 - t)^3*P0 + 3(1 - t)^2*t*P1 + 3(1 - t)*t^2*P2 + t^3*P3

    Then as t varies from 0 to 1, the point sweeps out a curve
    with four control points P0, P1, P2, P3.

    Another way is to define the same curve by specifying endpoint
    normal vectors instead of having control points:

                  ___       n1
      n0        /     \    ^
              /        \  /
       ^    /           *
        \ /
         *

    This is known as a cubic Hermite spline.

    Using this, we can make curved triangles if we give normal vectors
    for each vertex in a triangle.

    Each fragment is already computed as a barycentric interpolation
    of triangle vertices:

    P = alpha0*P0 + alpha1*P1 + alpha2*P2

    Instead of doing:

    n = P0*P1 x P0*P2

    inside the phong shader, we interpolate the vertex normals like:

    n = alpha0*n0 + alpha1*n1 + alpha2*n2
    */
    zbuffer_init(WIDTH, HEIGHT);
    smooth_shader ctx = {
        .model = &model,
        .ambient = 0.3,
        .shininess = 35,
    };
    smooth_shader_init(&ctx, light);
    shader const shader = {
        .vertex = &smooth_vertex,
        .fragment = &smooth_fragment,
        .ctx = &ctx,
    };

    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        triangle clip = {shader.vertex(shader.ctx, i, 0),
                         shader.vertex(shader.ctx, i, 1),
                         shader.vertex(shader.ctx, i, 2)};
        rasterize(clip, shader, buf, WIDTH, HEIGHT, CHANNELS);
    }
    stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, buf);
    zbuffer_deinit();

    model_deinit(&model);
}