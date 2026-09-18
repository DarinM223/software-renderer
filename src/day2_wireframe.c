#define STB_DS_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "model.h"
#include "stb_ds.h"
#include "stb_image_write.h"

#define WIDTH 800
#define HEIGHT 800
#define CHANNELS 3
#include "utils.h"

static void vec3_project(vec3 v, int32_t *x, int32_t *y)
{
    *x = (VEC_X(v) + 1.) * (WIDTH - 1) / 2;
    *y = (VEC_Y(v) + 1.) * (HEIGHT - 1) / 2;
}

int main(int argc, char **argv)
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
    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};

    for (size_t i = 0; i < model_nfaces(&model); ++i) {
        int32_t ax, ay, bx, by, cx, cy;
        vec3_project(model_vert_at_face(&model, i, 0), &ax, &ay);
        vec3_project(model_vert_at_face(&model, i, 1), &bx, &by);
        vec3_project(model_vert_at_face(&model, i, 2), &cx, &cy);
        framebuffer_draw_line(buf, ax, ay, bx, by, red);
        framebuffer_draw_line(buf, bx, by, cx, cy, red);
        framebuffer_draw_line(buf, cx, cy, ax, ay, red);
    }

    for (size_t i = 0; i < model_nverts(&model); ++i) {
        vec3 v = model_vert_at_idx(&model, i);
        int32_t x, y;
        vec3_project(v, &x, &y);
        framebuffer_set_color(buf, x, y, white);
    }

    stbi_write_tga("wireframe.tga", WIDTH, HEIGHT, CHANNELS, buf);
    model_deinit(&model);
}