#ifndef OUR_GL_H
#define OUR_GL_H

#include "geometry.h"
#include "utils.h"

void perspective_init(double const f);
void viewport_init(int32_t const x, int32_t const y, int32_t const w,
                   int32_t const h);
void zbuffer_init(int32_t const width, int32_t const height);
void zbuffer_deinit(void);
void look_at(vec3 const eye, vec3 const center, vec3 const up);

typedef struct shader {
    vec4 (*vertex)(void *, int32_t const face, int32_t const vert);
    bool (*fragment)(void *, vec3 const bar, color *color);
    void *ctx;
} shader;

typedef vec4 triangle[3];

void rasterize(triangle const clip, shader const shader, uint8_t *framebuffer,
               int64_t const framebuffer_width,
               int64_t const framebuffer_height,
               int64_t const framebuffer_channels);

#endif // OUR_GL_H