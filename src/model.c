#include "model.h"
#include "geometry.h"
#include "stb_ds.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

bool model_init(model *model, char const *filename)
{
    vec3 *verts = NULL;
    int32_t *facet_vrt = NULL;
    char buf[512];
    FILE *file = fopen(filename, "r");
    if (!file) {
        return false;
    }
    while (fgets(buf, 512, file)) {
        if (buf[0] == 'v' && buf[1] == ' ') {
            strtok(buf, " ");
            vec3 v = {0};
            char *vert;
            for (size_t i = 0; (vert = strtok(NULL, " ")) && i < 3; i++) {
                switch (i) {
                case 0:
                    v.x = atof(vert);
                    break;
                case 1:
                    v.y = atof(vert);
                    break;
                case 2:
                    v.z = atof(vert);
                    break;
                }
            }
            arrput(verts, v);
        } else if (buf[0] == 'f' && buf[1] == ' ') {
            strtok(buf, " ");

            char *face;
            for (size_t i = 0; (face = strtok(NULL, " ")) && i < 3; i++) {
                // Face is something like 6/1/6, get the first number out.
                for (char *ptr = face; *ptr != '\0'; ptr++) {
                    if (*ptr == '/') {
                        *ptr = '\0';
                        break;
                    }
                }
                arrput(facet_vrt, atoi(face) - 1);
            }
        }
    }
    model->verts = verts;
    model->facet_vrt = facet_vrt;
    return true;
}

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// Not reentrant but it's ok for now.
static model *g_model = NULL;
static int32_t sort_by_z(void const *a, void const *b)
{
    int32_t const a2 = *(int32_t const *)a;
    int32_t const b2 = *(int32_t const *)b;
    float aminz = MIN(model_vert_at_face(g_model, a2, 0).z,
                      MIN(model_vert_at_face(g_model, a2, 1).z,
                          model_vert_at_face(g_model, a2, 2).z));
    float bminz = MIN(model_vert_at_face(g_model, b2, 0).z,
                      MIN(model_vert_at_face(g_model, b2, 1).z,
                          model_vert_at_face(g_model, b2, 2).z));
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

void model_deinit(model *model)
{
    arrfree(model->verts);
    arrfree(model->facet_vrt);
}