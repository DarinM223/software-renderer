#include "our_gl.h"
#include "geometry.h"
#include <float.h>
#include <math.h> // IWYU pragma: keep
#include <stdlib.h>

mat4_4 model_view, viewport, perspective;
double *zbuffer;

void viewport_init(int32_t const x, int32_t const y, int32_t const w,
                   int32_t const h)
{
    viewport = (mat4_4){{{{w / 2., 0, 0, x + w / 2.}},
                         {{0, h / 2., 0, y + h / 2.}},
                         {{0, 0, 1, 0}},
                         {{0, 0, 0, 1}}}};
}

void perspective_init(double const f)
{
    perspective = (mat4_4){
        {{{1, 0, 0, 0}}, {{0, 1, 0, 0}}, {{0, 0, 1, 0}}, {{0, 0, -1 / f, 1}}}};
}

void zbuffer_init(int32_t const width, int32_t const height)
{
    zbuffer = malloc((size_t)width * height * sizeof(*zbuffer));
    for (size_t i = 0; i < (size_t)width * height; ++i) {
        zbuffer[i] = -1000.;
    }
}

void zbuffer_deinit(void)
{
    free(zbuffer);
}

void look_at(vec3 const eye, vec3 const center, vec3 const up)
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
    model_view = mat4_4_mul_mat4_4(&basis, &translate);
}

static void framebuffer_set_color(uint8_t *buf, int64_t const width,
                                  int64_t const height, int64_t const channels,
                                  int32_t x, int32_t y, color color)
{
    if (x < 0 || y < 0 || x >= width || y >= height) return;
    // y = 0 is at the bottom of the screen instead of the top.
    int const index = channels * ((height - 1 - y) * width + x);
    buf[index] = color.r;
    buf[index + 1] = color.g;
    buf[index + 2] = color.b;
}

void rasterize(triangle const clip, shader const shader, uint8_t *framebuffer,
               int64_t const framebuffer_width,
               int64_t const framebuffer_height,
               int64_t const framebuffer_channels)
{
    vec4 const ndc[3] = {vec4_div_scalar(clip[0], VEC_W(clip[0])),
                         vec4_div_scalar(clip[1], VEC_W(clip[1])),
                         vec4_div_scalar(clip[2], VEC_W(clip[2]))};
    vec2 const screen[3] = {VEC_XY(mat4_4_mul_vec4(&viewport, ndc[0])),
                            VEC_XY(mat4_4_mul_vec4(&viewport, ndc[1])),
                            VEC_XY(mat4_4_mul_vec4(&viewport, ndc[2]))};
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
         x <= MIN(framebuffer_width - 1, (int64_t)bbmaxx); ++x) {
        for (int32_t y = MAX(0, (int64_t)bbminy);
             y <= MIN(framebuffer_height - 1, (int64_t)bbmaxy); ++y) {
            // Determine barycentric coordinates for (x, y).
            mat3_3 invert_transpose = mat3_3_invert_transpose(&abc);
            vec3 barycentric_screen =
                mat3_3_mul_vec3(&invert_transpose, (vec3){{x, y, 1.}});
            vec3 barycentric_clip = {{
                VEC_X(barycentric_screen) / VEC_W(clip[0]),
                VEC_Y(barycentric_screen) / VEC_W(clip[1]),
                VEC_Z(barycentric_screen) / VEC_W(clip[2]),
            }};
            barycentric_clip =
                vec3_div_scalar(barycentric_clip, VEC_X(barycentric_clip) +
                                                      VEC_Y(barycentric_clip) +
                                                      VEC_Z(barycentric_clip));
            // Don't color coordinates that are not in the triangle.
            if (VEC_X(barycentric_screen) < 0 ||
                VEC_Y(barycentric_screen) < 0 ||
                VEC_Z(barycentric_screen) < 0) {
                continue;
            }
            double z = vec3_mul_dot(
                barycentric_screen,
                (vec3){{VEC_Z(ndc[0]), VEC_Z(ndc[1]), VEC_Z(ndc[2])}});
            // If pixel's z value is inferior to the one already painted, then
            // discard it.
            if (z <= zbuffer[y * framebuffer_width + x]) {
                continue;
            }
            color color;
            bool discard =
                shader.fragment(shader.ctx, barycentric_clip, &color);
            if (discard) {
                continue;
            }

            zbuffer[y * framebuffer_width + x] = z;
            framebuffer_set_color(framebuffer, framebuffer_width,
                                  framebuffer_height, framebuffer_channels, x,
                                  y, color);
        }
    }
}