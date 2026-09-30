#define STB_IMAGE_IMPLEMENTATION
#include "model.h"
#include "geometry.h"
#include "stb_ds.h"
#include "stb_image.h"
#include "utils.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define BUF_SIZE 512

bool tga_load_extra(char const *filename, char const *suffix, tga_image *result)
{
    size_t const filename_normal_len = strlen(filename) + strlen(suffix);
    char *filename_normal = malloc(filename_normal_len);
    memset(filename_normal, 0, filename_normal_len);
    strcat(filename_normal, filename);
    // Strip file extension from file if it exists.
    char *last = strrchr(filename_normal, '.');
    if (last) {
        filename_normal[last - filename_normal] = '\0';
    }
    strcat(filename_normal, suffix);
    result->buf = stbi_load(filename_normal, &result->width, &result->height,
                            &result->channels, 0);
    if (!result->buf) {
        printf("Cannot load image at %s: %s\n", filename_normal,
               stbi_failure_reason());
    }
    free(filename_normal);
    return result->buf;
}

color tga_sample_2d(tga_image const *image, vec2 const uv)
{
    int32_t uv_x = VEC_X(uv) * image->width;
    int32_t uv_y = VEC_Y(uv) * image->height;
    int32_t const index = image->channels * (uv_y * image->width + uv_x);
    color color;
    color.r = image->buf[index];
    color.g = image->buf[index + 1];
    color.b = image->buf[index + 2];
    return color;
}

bool model_init(model *model, char const *filename)
{
    bool success = true;
    vec3 *verts = NULL;
    vec3 *normals = NULL;
    vec2 *tex = NULL;
    int32_t *facet_vrt = NULL;
    int32_t *facet_tex = NULL;
    int32_t *facet_norm = NULL;
    char buf[BUF_SIZE];
    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }
    while (fgets(buf, BUF_SIZE, file)) {
        if (buf[0] == 'v' && buf[1] == ' ') {
            strtok(buf, " ");
            vec3 v = {0};
            char *vert;
            for (size_t i = 0; (vert = strtok(NULL, " ")) && i < 3; i++) {
                switch (i) {
                case 0:
                    v.data[0] = atof(vert);
                    break;
                case 1:
                    v.data[1] = atof(vert);
                    break;
                case 2:
                    v.data[2] = atof(vert);
                    break;
                }
            }
            arrput(verts, v);
        } else if (buf[0] == 'v' && buf[1] == 'n') {
            strtok(buf, " ");
            vec4 n = {0};
            char *vert;
            for (size_t i = 0; (vert = strtok(NULL, " ")) && i < 3; i++) {
                switch (i) {
                case 0:
                    n.data[0] = atof(vert);
                    break;
                case 1:
                    n.data[1] = atof(vert);
                    break;
                case 2:
                    n.data[2] = atof(vert);
                    break;
                }
            }
            arrput(normals, VEC_XYZ(VEC_NORMALIZED(4, n)));
        } else if (buf[0] == 'v' && buf[1] == 't') {
            strtok(buf, " ");
            vec2 t = {0};
            char *vert;
            for (size_t i = 0; (vert = strtok(NULL, " ")) && i < 2; i++) {
                switch (i) {
                case 0:
                    t.data[0] = atof(vert);
                    break;
                case 1:
                    t.data[1] = 1. - atof(vert);
                    break;
                }
            }
            arrput(tex, t);
        } else if (buf[0] == 'f' && buf[1] == ' ') {
            char *ptr = buf + 1;
            while (isspace(*ptr)) {
                ++ptr;
            }
            for (size_t i = 0; i < 3; ++i) {
                char *face = ptr, *tex, *norm;
                // Face is something like 6/1/6, get the first number out.
                for (; ptr - buf < BUF_SIZE && *ptr != '\0'; ++ptr) {
                    if (*ptr == '/') {
                        *ptr = '\0';
                        break;
                    }
                }
                // Texture coordinate is the second number.
                tex = ++ptr;
                for (; ptr - buf < BUF_SIZE && *ptr != '\0'; ++ptr) {
                    if (*ptr == '/') {
                        *ptr = '\0';
                        break;
                    }
                }
                // Normal is the third number.
                norm = ++ptr;
                for (; ptr - buf < BUF_SIZE && *ptr != '\0'; ++ptr) {
                    if (isspace(*ptr)) {
                        *ptr = '\0';
                        break;
                    }
                }
                ++ptr;
                arrput(facet_vrt, atoi(face) - 1);
                arrput(facet_tex, atoi(tex) - 1);
                arrput(facet_norm, atoi(norm) - 1);
            }
        }
    }
    model->verts = verts;
    model->normals = normals;
    model->tex = tex;
    model->facet_vrt = facet_vrt;
    model->facet_tex = facet_tex;
    model->facet_norm = facet_norm;
    if (!(success = tga_load_extra(filename, "_nm.tga", &model->normal_map))) {
        goto cleanup_model;
    }
    return success;
cleanup_model:
    model_deinit(model);
    return success;
}

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// Not reentrant but it's ok for now.
static model *g_model = NULL;
static int32_t sort_by_z(void const *a, void const *b)
{
    int32_t const a2 = *(int32_t const *)a;
    int32_t const b2 = *(int32_t const *)b;
    float aminz = MIN(VEC_Z(model_vert_at_face(g_model, a2, 0)),
                      MIN(VEC_Z(model_vert_at_face(g_model, a2, 1)),
                          VEC_Z(model_vert_at_face(g_model, a2, 2))));
    float bminz = MIN(VEC_Z(model_vert_at_face(g_model, b2, 0)),
                      MIN(VEC_Z(model_vert_at_face(g_model, b2, 1)),
                          VEC_Z(model_vert_at_face(g_model, b2, 2))));
    return aminz < bminz ? -1 : aminz > bminz ? 1 : 0;
}

void model_sort_by_z(model *model)
{
    int32_t *idx = malloc(model_nfaces(model) * sizeof(*idx));
    for (size_t i = 0; i < model_nfaces(model); ++i) {
        idx[i] = i;
    }

    g_model = model;
    qsort(idx, model_nfaces(model), sizeof(*idx), &sort_by_z);
    g_model = NULL;

    int32_t *facet_vrt = NULL;
    arrsetlen(facet_vrt, model_nfaces(model) * 3);
    for (size_t i = 0; i < model_nfaces(model); ++i) {
        for (size_t j = 0; j < 3; ++j) {
            facet_vrt[i * 3 + j] = model->facet_vrt[idx[i] * 3 + j];
        }
    }

    arrfree(model->facet_vrt);
    model->facet_vrt = facet_vrt;
    free(idx);
}

size_t model_nverts(model const *model)
{
    return arrlen(model->verts);
}

size_t model_nfaces(model const *model)
{
    return arrlen(model->facet_vrt) / 3;
}

vec3 model_vert_at_idx(model const *model, size_t const i)
{
    return model->verts[i];
}

vec3 model_vert_at_face(model const *model, size_t const iface,
                        size_t const nthvert)
{
    return model->verts[model->facet_vrt[iface * 3 + nthvert]];
}

vec3 model_norm_at_face(model const *model, size_t const iface,
                        size_t const nthvert)
{
    return model->normals[model->facet_norm[iface * 3 + nthvert]];
}

vec4 model_norm_at_uv(model const *model, vec2 const uv)
{
    color color = tga_sample_2d(&model->normal_map, uv);
    // stbi_image loads in RGBA order instead of BGRA order in the tutorial
    // TGA loader.
    return vec4_sub(
        vec4_mul_scalar_rhs(
            (vec4){{(double)color.r, (double)color.g, (double)color.b, 0.}},
            2. / 255),
        (vec4){{1, 1, 1, 0}});
}

vec2 model_uv_at_face(model const *model, size_t const iface,
                      size_t const nthvert)
{
    return model->tex[model->facet_tex[iface * 3 + nthvert]];
}

void model_deinit(model *model)
{
    arrfree(model->verts);
    arrfree(model->normals);
    arrfree(model->tex);
    arrfree(model->facet_vrt);
    arrfree(model->facet_tex);
    arrfree(model->facet_norm);
    stbi_image_free(model->normal_map.buf);
}