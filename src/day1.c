#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define WIDTH 64
#define HEIGHT 64
#define CHANNELS 3

#include "utils.h"

int main(void)
{
    uint8_t buf[WIDTH * HEIGHT * CHANNELS] = {0};
    framebuffer_set_color(buf, 7, 3, white);
    framebuffer_set_color(buf, 12, 37, white);
    framebuffer_set_color(buf, 62, 53, white);
    stbi_write_png("framebuffer.tga", WIDTH, HEIGHT, CHANNELS, buf,
                   WIDTH * CHANNELS);
}