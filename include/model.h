#ifndef MODEL_H
#define MODEL_H

#include "geometry.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct tga_image {
    uint8_t *buf;
    int32_t width;
    int32_t height;
    int32_t channels;
} tga_image;

typedef struct model {
    vec3 *verts;
    vec3 *normals;
    vec2 *tex;
    int32_t *facet_vrt;
    int32_t *facet_tex;
    int32_t *facet_norm;
    tga_image normal_map;
} model;

bool model_init(model *model, char const *filename);
void model_sort_by_z(model *model);
size_t model_nverts(model const *model);
size_t model_nfaces(model const *model);
vec3 model_vert_at_idx(model const *model, size_t const i);
vec3 model_vert_at_face(model const *model, size_t const iface,
                        size_t const nthvert);
vec3 model_norm_at_face(model const *model, size_t const iface,
                        size_t const nthvert);
vec4 model_norm_at_uv(model const *model, vec2 const uv);
vec2 model_uv_at_face(model const *model, size_t const iface,
                      size_t const nthvert);
void model_deinit(model *model);

#endif // MODEL_H