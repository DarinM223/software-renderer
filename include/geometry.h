#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stdint.h>

#define PREV_4 3
#define PREV_3 2
#define PREV_2 1
#define PREV_1 0

#define NEXT_3 4
#define NEXT_2 3
#define NEXT_1 2
#define NEXT_0 1

#define PREV(N) PREV_##N
#define NEXT(N) NEXT_##N

#define GLUE2_RAW(a, b) a##b
#define GLUE2(a, b) GLUE2_RAW(a, b)
#define GLUE3_RAW(a, b, c) a##b##c
#define GLUE3(a, b, c) GLUE3_RAW(a, b, c)
#define GLUE4_RAW(a, b, c, d) a##b##c##d
#define GLUE4(a, b, c, d) GLUE4_RAW(a, b, c, d)

#define VEC(N) GLUE2(vec, N)
#define DEFINE_VEC(N)                                                          \
    typedef struct vec##N {                                                    \
        double data[N];                                                        \
    } vec##N;

DEFINE_VEC(1)
DEFINE_VEC(2)
DEFINE_VEC(3)

#define VEC_MULTIPLY_DOT(N)                                                    \
    static inline double vec##N##_mul_dot(vec##N const lhs, vec##N const rhs)  \
    {                                                                          \
        double ret = 0;                                                        \
        for (int32_t i = (N); i--; ret += lhs.data[i] * rhs.data[i])           \
            ;                                                                  \
        return ret;                                                            \
    }

#define VEC_MULTIPLY_COMP(N)                                                   \
    static inline vec##N vec##N##_mul_comp(vec##N const lhs, vec##N const rhs) \
    {                                                                          \
        vec##N ret = lhs;                                                      \
        for (int32_t i = (N); i--; ret.data[i] *= rhs.data[i])                 \
            ;                                                                  \
        return ret;                                                            \
    }

#define VEC_MULTIPLY_SCALAR_RHS(N)                                             \
    static inline vec##N vec##N##_mul_scalar_rhs(vec##N const lhs,             \
                                                 double const rhs)             \
    {                                                                          \
        vec##N ret = lhs;                                                      \
        for (int32_t i = (N); i--; ret.data[i] *= rhs)                         \
            ;                                                                  \
        return ret;                                                            \
    }

#define VEC_MULTIPLY_SCALAR_LHS(N)                                             \
    static inline vec##N vec##N##_mul_scalar_lhs(double const lhs,             \
                                                 vec##N const rhs)             \
    {                                                                          \
        vec##N ret = rhs;                                                      \
        for (int32_t i = (N); i--; ret.data[i] *= lhs)                         \
            ;                                                                  \
        return ret;                                                            \
    }

#define VEC_ADD(N)                                                             \
    static inline vec##N vec##N##_add(vec##N const lhs, vec##N const rhs)      \
    {                                                                          \
        vec##N ret = lhs;                                                      \
        for (int32_t i = (N); i--; ret.data[i] += rhs.data[i])                 \
            ;                                                                  \
        return ret;                                                            \
    }

#define VEC_SUB(N)                                                             \
    static inline vec##N vec##N##_sub(vec##N const lhs, vec##N const rhs)      \
    {                                                                          \
        vec##N ret = lhs;                                                      \
        for (int32_t i = (N); i--; ret.data[i] -= rhs.data[i])                 \
            ;                                                                  \
        return ret;                                                            \
    }

#define VEC_DIVIDE_SCALAR(N)                                                   \
    static inline vec##N vec##N##_div_scalar(vec##N const lhs,                 \
                                             double const rhs)                 \
    {                                                                          \
        vec##N ret = lhs;                                                      \
        for (int32_t i = (N); i--; ret.data[i] /= rhs)                         \
            ;                                                                  \
        return ret;                                                            \
    }

VEC_MULTIPLY_DOT(2)
VEC_MULTIPLY_COMP(2)
VEC_MULTIPLY_SCALAR_LHS(2)
VEC_MULTIPLY_SCALAR_RHS(2)
VEC_DIVIDE_SCALAR(2)
VEC_ADD(2)
VEC_SUB(2)
VEC_MULTIPLY_DOT(3)
VEC_MULTIPLY_COMP(3)
VEC_MULTIPLY_SCALAR_LHS(3)
VEC_MULTIPLY_SCALAR_RHS(3)
VEC_DIVIDE_SCALAR(3)
VEC_ADD(3)
VEC_SUB(3)

#define VEC_NORMALIZE(N, V) sqrt(vec##N##_mul_dot((V), (V)))
#define VEC_NORMALIZED(N, V) vec##n##_div_scalar((V), VEC_NORMALIZE(N, V))
#define VEC_X(V) (V).data[0]
#define VEC_Y(V) (V).data[1]
#define VEC_Z(V) (V).data[2]

static inline vec3 cross(vec3 const v1, vec3 const v2)
{
    return (vec3){{VEC_Y(v1) * VEC_Z(v2) - VEC_Z(v1) * VEC_Y(v2),
                   VEC_Z(v1) * VEC_X(v2) - VEC_X(v1) * VEC_Z(v2),
                   VEC_X(v1) * VEC_Y(v2) - VEC_Y(v1) * VEC_X(v2)}};
}

#define MAT(R, C) GLUE4(mat_, R, _, C)
#define DEFINE_MAT(NROWS, NCOLS)                                               \
    typedef struct mat_##NROWS##_##NCOLS {                                     \
        vec##NCOLS rows[NROWS];                                                \
    } mat_##NROWS##_##NCOLS;

DEFINE_MAT(1, 1)
DEFINE_MAT(1, 2)
DEFINE_MAT(1, 3)
DEFINE_MAT(2, 1)
DEFINE_MAT(2, 2)
DEFINE_MAT(2, 3)
DEFINE_MAT(3, 1)
DEFINE_MAT(3, 2)
DEFINE_MAT(3, 3)

#define MAT_IDX(MAT, R, C) (MAT)->rows[R].data[C]

static inline double det_1(mat_1_1 *const src)
{
    return MAT_IDX(src, 0, 0);
}

#define DEFINE_DET(N)                                                          \
    static inline double det_##N(mat_##N##_##N *const src)                     \
    {                                                                          \
        double ret = 0;                                                        \
        for (int32_t i = N; i--;                                               \
             ret += MAT_IDX(src, 0, i) * mat_##N##_##N##_cofactor(src, 0, i))  \
            ;                                                                  \
        return ret;                                                            \
    }

#define MAT_COFACTOR(NROWS, NCOLS)                                             \
    static inline double mat_##NROWS##_##NCOLS##_cofactor(                     \
        MAT(NROWS, NCOLS) const *matrix, int32_t const row, int32_t const col) \
    {                                                                          \
        MAT(PREV(NROWS), PREV(NCOLS)) submatrix;                               \
        for (int32_t i = NROWS - 1; i--;) {                                    \
            for (int32_t j = NCOLS - 1; j--;                                   \
                 MAT_IDX(&submatrix, i, j) =                                   \
                     MAT_IDX(matrix, i + (i >= row), j + (j >= col)))          \
                ;                                                              \
            ;                                                                  \
        }                                                                      \
        return GLUE2(det_, PREV(NCOLS))(&submatrix) *                          \
               ((row + col) % 2 ? -1 : 1);                                     \
    }

MAT_COFACTOR(2, 2)
DEFINE_DET(2)
MAT_COFACTOR(3, 3)

static inline double signed_triangle_area(int32_t ax, int32_t ay, int32_t bx,
                                          int32_t by, int32_t cx, int32_t cy)
{
    return 0.5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
                  (ay - cy) * (ax + cx));
}

#endif // GEOMETRY_H