#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stdint.h>

typedef struct vec3 {
    double x;
    double y;
    double z;
} vec3;

static inline double signed_triangle_area(int32_t ax, int32_t ay, int32_t bx,
                                          int32_t by, int32_t cx, int32_t cy)
{
    return 0.5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
                  (ay - cy) * (ax + cx));
}

#endif // GEOMETRY_H