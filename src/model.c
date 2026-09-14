#include "model.h"
#include "stb_ds.h"
#include <stdbool.h>
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