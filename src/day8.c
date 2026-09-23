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

typedef struct phong_shader {
    model *model;
    vec3 tri[3];
    double ambient;
    double shininess;
    vec3 l;
} phong_shader;

void phong_shader_init(phong_shader *shader, vec3 light)
{
    // extend light with 0. not 1. so the camera translation doesn't affect it.
    shader->l = VEC_XYZ(VEC_NORMALIZED(
        4, mat4_4_mul_vec4(&model_view, vec3_extend(light, 0.))));
}

static vec4 phong_vertex(void *ctx, int32_t const face, int32_t const vert)
{
    phong_shader *shader = ctx;
    vec3 v = model_vert_at_face(shader->model, face, vert);
    vec4 gl_Position = mat4_4_mul_vec4(&model_view, vec3_extend(v, 1.));
    shader->tri[vert] = VEC_XYZ(gl_Position);
    return mat4_4_mul_vec4(&perspective, gl_Position);
}

static bool phong_fragment(void *ctx, vec3 const bar, color *color)
{
    (void)color, (void)bar;
    phong_shader *shader = ctx;
    vec3 AB = vec3_sub(shader->tri[1], shader->tri[0]);
    vec3 AC = vec3_sub(shader->tri[2], shader->tri[0]);
    vec3 n = VEC_NORMALIZED(3, vec3_cross(AB, AC));
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

    // Converting previous days code to use the new shader pipeline:
    {
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
        stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, buf);

        zbuffer_deinit();
    }

    /* Phong reflection model:
       Brightness is a sum of three components each scaled by a weight:
       Ambient: indirect light - constant background illumination
       Diffuse: light scattered equally in all directions from rough surfaces
       Specular: bright highlights on shiny surfaces caused by reflection

       Diffuse:

        (sun)
       \ \ \ \ \ \
        \ \ \ \ \ \/
       \ \ \ \ \ /
        \ \ \ \/  (panel)
         \ \ /
          \/) angle alpha

       The energy output of the panel depends on how many sun rays enter
       the panel. For a 1D unit length panel, the corridor width is
       |cosine alpha|: it is 1 meter when the panel is orthogonal to the
       sun rays and 0 when the panel is aligned with the sun's rays.

       To compute cosine of alpha, imagine we have a triangle with three points
       A, B, and C. Then its normal n is the cross product between the edges:

       n = (AB x AC) / ||AB x AC||

       For l being the unit length vector towards the sun then:

       cos(alpha) = n . l

       and the diffuse term can be found as max(0, n . l).

       Specular:

       Because specular simulates highlights caused by reflection, its
       intensity depends on the angle between the viewers direction and
       the reflection direction of the light.

       Given v, the direction from the fragment to the camera, n being the
       normal of the fragment, and l being the vector towards the sun, find r,
       the reflection of the sun light.

       r = 2n(n . l) - l

       Then to find beta, the angle between the viewing direction and the
       perfect reflection direction:

       cos(beta) = r . v

       Then the specular term is max(0, cos(beta))^e where e is a shininess
       exponent.
    */
    {
        zbuffer_init(WIDTH, HEIGHT);

        phong_shader ctx = {
            .model = &model,
            .ambient = 0.3,
            .shininess = 35,
        };
        phong_shader_init(&ctx, light);
        shader const shader = {
            .vertex = &phong_vertex,
            .fragment = &phong_fragment,
            .ctx = &ctx,
        };

        uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
        for (size_t i = 0; i < model_nfaces(&model); ++i) {
            triangle clip = {shader.vertex(shader.ctx, i, 0),
                             shader.vertex(shader.ctx, i, 1),
                             shader.vertex(shader.ctx, i, 2)};
            rasterize(clip, shader, buf, WIDTH, HEIGHT, CHANNELS);
        }
        stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, buf);

        zbuffer_deinit();
    }
    model_deinit(&model);
}