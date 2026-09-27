#include "model.h"
#include "geometry.h"
#include "stb_ds.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define BUF_SIZE 512

bool model_init(model *model, char const *filename)
{
    vec3 *verts = NULL;
    vec3 *normals = NULL;
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
            vec3 n = {0};
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
            arrput(normals, n);
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
                arrput(facet_tex, atoi(tex));
                arrput(facet_norm, atoi(norm) - 1);
            }
        }
    }
    model->verts = verts;
    model->normals = normals;
    model->facet_vrt = facet_vrt;
    model->facet_tex = facet_tex;
    model->facet_norm = facet_norm;
    return true;
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

void model_deinit(model *model)
{
    arrfree(model->verts);
    arrfree(model->normals);
    arrfree(model->facet_vrt);
    arrfree(model->facet_tex);
    arrfree(model->facet_norm);
}