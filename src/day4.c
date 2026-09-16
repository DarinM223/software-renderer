#define STB_IMAGE_WRITE_IMPLEMENTATION
#define WIDTH 64
#define HEIGHT 64
#define CHANNELS 3
#include "geometry.h"
#include "stb_image_write.h"
#include "utils.h"

/*

Barycentric coordinates:

Given a 1D segment defined by A and B, and given a point P on the
segment between A and B, its position can be expressed as a weighted
combination of A and B:

P = alpha * A + beta * B
alpha + beta = 1.

Solving the two equations gives:

alpha = (B - P) / (B - A)
beta = 1 - alpha = (P - A) / (B - A)

These weights tell us how much of A and B contribute to the position of P.

- If P = A then alpha = 1 and beta = 0.
- If P = B then alpha = 0 and beta = 1.
- If both coordinates are strictly between 0 and 1, then P is strictly inside
the segment between A and B.
- If P is outside [A, B], then one coordinate is negative.

2D Barycentric coordinates:
For 2D Barycentric coordinates we need a triangle instead of a line. Given a
triangle with vertices A, B, and C, any point P inside the triangle can be
written as:

Px = alpha * Ax + beta * Bx + gamma * Cx
Py = alpha * Ay + beta * By + gamma * Cy
alpha + beta + gamma = 1.

We can rewrite this system of equations in matrix form:

(Ax Bx Cx) (alpha)   (Px)
(Ay By Cy) (beta ) = (Py)
(1  1  1 ) (gamma)   (1 )

Then we have:

                    -1
(alpha)   (Ax Bx Cx)   (Px)
(beta ) = (Ay By Cy)   (Py)
(gamma)   (1  1  1 )   (1 )

             (Ax Bx Cx) (Px)
          adj(Ay By Cy) (Py)
             (1  1  1 ) (1 )
        = ______________________
               Ax Bx Cx
               Ay By Cy
               1  1  1

          (By-Cy  Cx-Bx  Bx*Cy-By*Cx) (Px)
          (Cy-Ay  Ax-Cx  Ay*Cx-Ax*Cy) (Py)
          (Ay-By  Bx-Ax  Ax*By-Ay*Bx) (1 )
       =  ________________________________
                   Ax Bx Cx
                   Ay By Cy
                   1  1  1

                                            T
          (Px Bx Cx    Px Cx Ax    Px Ax Bx)
          (Py By Cy    Py Cy Ay    Py Ay By)
          (1  1  1     1  1  1     1  1  1 )
       =  __________________________________
                      Ax Bx Cx
                      Ay By Cy
                      1  1  1

It turns out that to find the weights alpha, beta, and gamma, we need to compute
determinants of four matrices that of a very particular form. These determinants
give twice the signed area of the triangle formed by the points in the matrices.

Then:

alpha = Area(PBC) / Area(ABC)
beta = Area(PCA) / Area(ABC)
gamma = Area(PAB) / Area(ABC)

Where PBC is the subtriangle formed by points P, B, and C, etc.

The shoelace formula allows for calculation of signed area:

Area(ABC) = 1/2*((By-Ay)*(Bx+Ax)+(Cy-By)*(Cx+Bx)+(Ay-Cy)*(Ax+Cx))

- If alpha, beta, and gamma are all between 0 and 1, P is inside the triangle.
- If any coordinate is negative, P is outside the triangle.
- If one coordinate is exactly zero, P lies on an edge.
- If two coordinates are exactly zero, P lies on a vertex.

*/

// Interpolate grey color for triangle for each vertex.
void draw_triangle_interp_grey(uint8_t *buf, int32_t ax, int32_t ay, int32_t az,
                               int32_t bx, int32_t by, int32_t bz, int32_t cx,
                               int32_t cy, int32_t cz)
{
    int32_t bbminx = MIN(MIN(ax, bx), cx);
    int32_t bbminy = MIN(MIN(ay, by), cy);
    int32_t bbmaxx = MAX(MAX(ax, bx), cx);
    int32_t bbmaxy = MAX(MAX(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area < 1) {
        return;
    }

#pragma omp parallel for
    for (int32_t x = bbminx; x < bbmaxx; ++x) {
        for (int32_t y = bbminy; y < bbmaxy; ++y) {
            double alpha =
                signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta =
                signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma =
                signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }
            // Interpolate grey color.
            uint8_t z = alpha * az + beta * bz + gamma * cz;
            framebuffer_set_color(buf, x, y, (color){.r = z, .g = z, .b = z});
        }
    }
}

// Interpolate color between three RGB coordinates for each vertex.
void draw_triangle_interp_color(uint8_t *buf, int32_t ax, int32_t ay,
                                int32_t ar, int32_t ag, int32_t ab, int32_t bx,
                                int32_t by, int32_t br, int32_t bg, int32_t bb,
                                int32_t cx, int32_t cy, int32_t cr, int32_t cg,
                                int32_t cb)
{
    int32_t bbminx = MIN(MIN(ax, bx), cx);
    int32_t bbminy = MIN(MIN(ay, by), cy);
    int32_t bbmaxx = MAX(MAX(ax, bx), cx);
    int32_t bbmaxy = MAX(MAX(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area < 1) {
        return;
    }

#pragma omp parallel for
    for (int32_t x = bbminx; x < bbmaxx; ++x) {
        for (int32_t y = bbminy; y < bbmaxy; ++y) {
            double alpha =
                signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta =
                signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma =
                signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }
            uint8_t r = alpha * ar + beta * br + gamma * cr;
            uint8_t g = alpha * ag + beta * bg + gamma * cg;
            uint8_t b = alpha * ab + beta * bb + gamma * cb;
            framebuffer_set_color(buf, x, y, (color){.r = r, .g = g, .b = b});
        }
    }
}

// Draw a colored "wireframe" triangle without using the draw_line function.
void draw_triangle_interp_wireframe(uint8_t *buf, int32_t ax, int32_t ay,
                                    int32_t ar, int32_t ag, int32_t ab,
                                    int32_t bx, int32_t by, int32_t br,
                                    int32_t bg, int32_t bb, int32_t cx,
                                    int32_t cy, int32_t cr, int32_t cg,
                                    int32_t cb)
{
    int32_t bbminx = MIN(MIN(ax, bx), cx);
    int32_t bbminy = MIN(MIN(ay, by), cy);
    int32_t bbmaxx = MAX(MAX(ax, bx), cx);
    int32_t bbmaxy = MAX(MAX(ay, by), cy);
    double total_area = signed_triangle_area(ax, ay, bx, by, cx, cy);
    if (total_area < 1) {
        return;
    }

#pragma omp parallel for
    for (int32_t x = bbminx; x < bbmaxx; ++x) {
        for (int32_t y = bbminy; y < bbmaxy; ++y) {
            double alpha =
                signed_triangle_area(x, y, bx, by, cx, cy) / total_area;
            double beta =
                signed_triangle_area(x, y, cx, cy, ax, ay) / total_area;
            double gamma =
                signed_triangle_area(x, y, ax, ay, bx, by) / total_area;
            if (alpha < 0 || beta < 0 || gamma < 0) {
                continue;
            }
            if (MIN(alpha, MIN(beta, gamma)) < 0.1) {
                uint8_t r = alpha * ar + beta * br + gamma * cr;
                uint8_t g = alpha * ag + beta * bg + gamma * cg;
                uint8_t b = alpha * ab + beta * bb + gamma * cb;
                framebuffer_set_color(buf, x, y,
                                      (color){.r = r, .g = g, .b = b});
            }
        }
    }
}

int main(void)
{
    uint8_t buf[WIDTH * HEIGHT * CHANNELS];

    int32_t ax = 17, ay = 4, az = 13;
    int32_t bx = 55, by = 39, bz = 128;
    int32_t cx = 23, cy = 59, cz = 255;

    memset(buf, 0, sizeof(buf));
    draw_triangle_interp_grey(buf, ax, ay, az, bx, by, bz, cx, cy, cz);
    stbi_write_tga("framebuffer1.tga", WIDTH, HEIGHT, CHANNELS, buf);

    memset(buf, 0, sizeof(buf));
    draw_triangle_interp_color(buf, ax, ay, 0, 0, 255, bx, by, 0, 255, 0, cx,
                               cy, 255, 0, 0);
    stbi_write_tga("framebuffer2.tga", WIDTH, HEIGHT, CHANNELS, buf);

    memset(buf, 0, sizeof(buf));
    draw_triangle_interp_wireframe(buf, ax, ay, 0, 0, 255, bx, by, 0, 255, 0,
                                   cx, cy, 255, 0, 0);
    stbi_write_tga("framebuffer3.tga", WIDTH, HEIGHT, CHANNELS, buf);
}