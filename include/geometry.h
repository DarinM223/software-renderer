#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <stdint.h>
#include <stdio.h>

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
DEFINE_VEC(4)

static inline vec3 vec2_extend(vec2 const v, double d)
{
    return (vec3){{v.data[0], v.data[1], d}};
}

static inline vec4 vec3_extend(vec3 const v, double d)
{
    return (vec4){{v.data[0], v.data[1], v.data[2], d}};
}

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

VEC_MULTIPLY_DOT(1)
VEC_MULTIPLY_COMP(1)
VEC_MULTIPLY_SCALAR_LHS(1)
VEC_MULTIPLY_SCALAR_RHS(1)
VEC_DIVIDE_SCALAR(1)
VEC_ADD(1)
VEC_SUB(1)
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
VEC_MULTIPLY_DOT(4)
VEC_MULTIPLY_COMP(4)
VEC_MULTIPLY_SCALAR_LHS(4)
VEC_MULTIPLY_SCALAR_RHS(4)
VEC_DIVIDE_SCALAR(4)
VEC_ADD(4)
VEC_SUB(4)

#define VEC_NORMALIZE(N, V) sqrt(vec##N##_mul_dot((V), (V)))
#define VEC_NORMALIZED(N, V) vec##N##_div_scalar((V), VEC_NORMALIZE(N, V))
#define VEC_X(V) (V).data[0]
#define VEC_Y(V) (V).data[1]
#define VEC_Z(V) (V).data[2]
#define VEC_W(V) (V).data[3]
#define VEC_XY(V)                                                              \
    (vec2)                                                                     \
    {                                                                          \
        {                                                                      \
            VEC_X(V), VEC_Y(V)                                                 \
        }                                                                      \
    }
#define VEC_XYZ(V)                                                             \
    (vec3)                                                                     \
    {                                                                          \
        {                                                                      \
            VEC_X(V), VEC_Y(V), VEC_Z(V)                                       \
        }                                                                      \
    }

static inline vec3 vec3_cross(vec3 const v1, vec3 const v2)
{
    return (vec3){{VEC_Y(v1) * VEC_Z(v2) - VEC_Z(v1) * VEC_Y(v2),
                   VEC_Z(v1) * VEC_X(v2) - VEC_X(v1) * VEC_Z(v2),
                   VEC_X(v1) * VEC_Y(v2) - VEC_Y(v1) * VEC_X(v2)}};
}

#define MAT(R, C) GLUE4(mat, R, _, C)
#define DEFINE_MAT(NROWS, NCOLS)                                               \
    typedef struct mat##NROWS##_##NCOLS {                                      \
        vec##NCOLS rows[NROWS];                                                \
    } mat##NROWS##_##NCOLS;

DEFINE_MAT(1, 1)
DEFINE_MAT(1, 2)
DEFINE_MAT(1, 3)
DEFINE_MAT(1, 4)
DEFINE_MAT(2, 1)
DEFINE_MAT(2, 2)
DEFINE_MAT(2, 3)
DEFINE_MAT(2, 4)
DEFINE_MAT(3, 1)
DEFINE_MAT(3, 2)
DEFINE_MAT(3, 3)
DEFINE_MAT(3, 4)
DEFINE_MAT(4, 1)
DEFINE_MAT(4, 2)
DEFINE_MAT(4, 3)
DEFINE_MAT(4, 4)

#define MAT_IDX(MAT, R, C) (MAT)->rows[R].data[C]

static inline double mat1_1_det(mat1_1 const *src)
{
    return MAT_IDX(src, 0, 0);
}

#define DEFINE_DET(N)                                                          \
    static inline double mat##N##_##N##_det(MAT(N, N) const *src)              \
    {                                                                          \
        double ret = 0;                                                        \
        for (int32_t i = N; i--;                                               \
             ret += MAT_IDX(src, 0, i) * mat##N##_##N##_cofactor(src, 0, i))   \
            ;                                                                  \
        return ret;                                                            \
    }

#define MAT_COFACTOR(NROWS, NCOLS)                                             \
    static inline double mat##NROWS##_##NCOLS##_cofactor(                      \
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
        return GLUE2(MAT(PREV(NCOLS), PREV(NCOLS)), _det)(&submatrix) *        \
               ((row + col) % 2 ? -1 : 1);                                     \
    }

#define MAT_INVERT_TRANSPOSE(NROWS, NCOLS)                                     \
    static inline MAT(NROWS, NCOLS) mat##NROWS##_##NCOLS##_invert_transpose(   \
        MAT(NROWS, NCOLS) const *matrix)                                       \
    {                                                                          \
        MAT(NROWS, NCOLS) adjugate_transpose;                                  \
        for (int32_t i = NROWS; i--;) {                                        \
            for (int32_t j = NCOLS; j--;                                       \
                 MAT_IDX(&adjugate_transpose, i, j) =                          \
                     mat##NROWS##_##NCOLS##_cofactor(matrix, i, j))            \
                ;                                                              \
        }                                                                      \
        return mat##NROWS##_##NCOLS##_div_scalar(                              \
            &adjugate_transpose,                                               \
            vec##NCOLS##_mul_dot(adjugate_transpose.rows[0],                   \
                                 matrix->rows[0]));                            \
    }

#define MAT_INVERT(NROWS, NCOLS)                                               \
    static inline MAT(NROWS, NCOLS)                                            \
        mat##NROWS##_##NCOLS##_invert(MAT(NROWS, NCOLS) const *matrix)         \
    {                                                                          \
        MAT(NROWS, NCOLS)                                                      \
        inv_trans = mat##NROWS##_##NCOLS##_invert_transpose(matrix);           \
        return mat##NROWS##_##NCOLS##_transpose(&inv_trans);                   \
    }

#define MAT_TRANSPOSE(NROWS, NCOLS)                                            \
    static inline MAT(NCOLS, NROWS)                                            \
        mat##NROWS##_##NCOLS##_transpose(MAT(NROWS, NCOLS) const *matrix)      \
    {                                                                          \
        MAT(NCOLS, NROWS) ret;                                                 \
        for (int32_t i = NCOLS; i--;) {                                        \
            for (int32_t j = NROWS; j--;                                       \
                 MAT_IDX(&ret, i, j) = MAT_IDX(matrix, j, i))                  \
                ;                                                              \
        }                                                                      \
        return ret;                                                            \
    }

#define MAT_MUL_MAT(R1, C1, C2)                                                \
    static inline MAT(R1, C2) mat##R1##_##C1##_mul_mat##C1##_##C2(             \
        MAT(R1, C1) const *lhs, MAT(C1, C2) const *rhs)                        \
    {                                                                          \
        MAT(R1, C2) ret;                                                       \
        for (int32_t i = R1; i--;) {                                           \
            for (int32_t j = C2; j--;) {                                       \
                for (int32_t k = C1; k--;                                      \
                     MAT_IDX(&ret, i, j) +=                                    \
                     MAT_IDX(lhs, i, k) * MAT_IDX(rhs, k, j))                  \
                    ;                                                          \
            }                                                                  \
        }                                                                      \
        return ret;                                                            \
    }

#define VEC_MUL_MAT(NROWS, NCOLS)                                              \
    static inline VEC(NCOLS) vec##NROWS##_mul_mat##NROWS##_##NCOLS(            \
        VEC(NROWS) const *lhs, MAT(NROWS, NCOLS) const *rhs)                   \
    {                                                                          \
        MAT(1, NROWS) mat_lhs = {{*lhs}};                                      \
        return mat1_##NROWS##_mul##_mat##NROWS##_##NCOLS(&mat_lhs, rhs)        \
            .rows[0];                                                          \
    }

#define MAT_MUL_VEC(NROWS, NCOLS)                                              \
    static inline VEC(NROWS) mat##NROWS##_##NCOLS##_mul_vec##NROWS(            \
        MAT(NROWS, NCOLS) const *lhs, VEC(NCOLS) const rhs)                    \
    {                                                                          \
        VEC(NROWS) ret;                                                        \
        for (int32_t i = NROWS; i--;                                           \
             ret.data[i] = vec##NCOLS##_mul_dot(lhs->rows[i], rhs))            \
            ;                                                                  \
        return ret;                                                            \
    }

#define MAT_MUL_SCALAR(NROWS, NCOLS)                                           \
    static inline MAT(NROWS, NCOLS) mat##NROWS##_##NCOLS##_mul_scalar(         \
        MAT(NROWS, NCOLS) const *lhs, double const val)                        \
    {                                                                          \
        MAT(NROWS, NCOLS) result;                                              \
        for (int32_t i = NROWS; i--;                                           \
             result.rows[i] = vec##NCOLS##_mul_scalar_rhs(lhs->rows[i], val))  \
            ;                                                                  \
        return result;                                                         \
    }

#define MAT_DIV_SCALAR(NROWS, NCOLS)                                           \
    static inline MAT(NROWS, NCOLS) mat##NROWS##_##NCOLS##_div_scalar(         \
        MAT(NROWS, NCOLS) const *lhs, double const val)                        \
    {                                                                          \
        MAT(NROWS, NCOLS) result;                                              \
        for (int32_t i = NROWS; i--;                                           \
             result.rows[i] = vec##NCOLS##_div_scalar(lhs->rows[i], val))      \
            ;                                                                  \
        return result;                                                         \
    }

#define MAT_ADD(NROWS, NCOLS)                                                  \
    static inline MAT(NROWS, NCOLS) mat##NROWS##_##NCOLS##_add(                \
        MAT(NROWS, NCOLS) const *lhs, MAT(NROWS, NCOLS) const *rhs)            \
    {                                                                          \
        MAT(NROWS, NCOLS) result;                                              \
        for (int32_t i = NROWS; i--;) {                                        \
            for (int32_t j = NCOLS; j--;                                       \
                 MAT_IDX(&result, i, j) =                                      \
                     MAT_IDX(lhs, i, j) + MAT_IDX(rhs, i, j))                  \
                ;                                                              \
        }                                                                      \
        return result;                                                         \
    }

#define MAT_SUB(NROWS, NCOLS)                                                  \
    static inline MAT(NROWS, NCOLS) mat##NROWS##_##NCOLS##_sub(                \
        MAT(NROWS, NCOLS) const *lhs, MAT(NROWS, NCOLS) const *rhs)            \
    {                                                                          \
        MAT(NROWS, NCOLS) result;                                              \
        for (int32_t i = NROWS; i--;) {                                        \
            for (int32_t j = NCOLS; j--;                                       \
                 MAT_IDX(&result, i, j) =                                      \
                     MAT_IDX(lhs, i, j) - MAT_IDX(rhs, i, j))                  \
                ;                                                              \
        }                                                                      \
        return result;                                                         \
    }

#define VEC_PRINT(N)                                                           \
    static inline void vec##N##_print(VEC(N) v, FILE *stream)                  \
    {                                                                          \
        for (int32_t i = 0; i < N; ++i) {                                      \
            fprintf(stream, "%f ", v.data[i]);                                 \
        }                                                                      \
    }

#define MAT_PRINT(NROWS, NCOLS)                                                \
    static inline void mat##NROWS##_##NCOLS##_print(                           \
        MAT(NROWS, NCOLS) const *matrix, FILE *stream)                         \
    {                                                                          \
        for (int32_t i = 0; i < NROWS; ++i) {                                  \
            vec##NCOLS##_print(matrix->rows[i], stream);                       \
            fprintf(stream, "\n");                                             \
        }                                                                      \
    }

MAT_COFACTOR(2, 2)
DEFINE_DET(2)
MAT_COFACTOR(3, 3)
DEFINE_DET(3)
MAT_COFACTOR(4, 4)
MAT_TRANSPOSE(2, 2)
MAT_TRANSPOSE(3, 3)
MAT_TRANSPOSE(4, 4)

MAT_MUL_SCALAR(1, 1)
MAT_MUL_SCALAR(1, 2)
MAT_MUL_SCALAR(1, 3)
MAT_MUL_SCALAR(1, 4)
MAT_MUL_SCALAR(2, 1)
MAT_MUL_SCALAR(2, 2)
MAT_MUL_SCALAR(2, 3)
MAT_MUL_SCALAR(2, 4)
MAT_MUL_SCALAR(3, 1)
MAT_MUL_SCALAR(3, 2)
MAT_MUL_SCALAR(3, 3)
MAT_MUL_SCALAR(3, 4)
MAT_MUL_SCALAR(4, 1)
MAT_MUL_SCALAR(4, 2)
MAT_MUL_SCALAR(4, 3)
MAT_MUL_SCALAR(4, 4)
MAT_DIV_SCALAR(1, 1)
MAT_DIV_SCALAR(1, 2)
MAT_DIV_SCALAR(1, 3)
MAT_DIV_SCALAR(1, 4)
MAT_DIV_SCALAR(2, 1)
MAT_DIV_SCALAR(2, 2)
MAT_DIV_SCALAR(2, 3)
MAT_DIV_SCALAR(2, 4)
MAT_DIV_SCALAR(3, 1)
MAT_DIV_SCALAR(3, 2)
MAT_DIV_SCALAR(3, 3)
MAT_DIV_SCALAR(3, 4)
MAT_DIV_SCALAR(4, 1)
MAT_DIV_SCALAR(4, 2)
MAT_DIV_SCALAR(4, 3)
MAT_DIV_SCALAR(4, 4)
MAT_MUL_VEC(1, 1)
MAT_MUL_VEC(1, 2)
MAT_MUL_VEC(1, 3)
MAT_MUL_VEC(1, 4)
MAT_MUL_VEC(2, 1)
MAT_MUL_VEC(2, 2)
MAT_MUL_VEC(2, 3)
MAT_MUL_VEC(2, 4)
MAT_MUL_VEC(3, 1)
MAT_MUL_VEC(3, 2)
MAT_MUL_VEC(3, 3)
MAT_MUL_VEC(3, 4)
MAT_MUL_VEC(4, 1)
MAT_MUL_VEC(4, 2)
MAT_MUL_VEC(4, 3)
MAT_MUL_VEC(4, 4)
MAT_MUL_MAT(1, 1, 1)
MAT_MUL_MAT(1, 2, 1)
MAT_MUL_MAT(1, 3, 1)
MAT_MUL_MAT(1, 4, 1)
MAT_MUL_MAT(1, 1, 2)
MAT_MUL_MAT(1, 2, 2)
MAT_MUL_MAT(1, 3, 2)
MAT_MUL_MAT(1, 4, 2)
MAT_MUL_MAT(1, 1, 3)
MAT_MUL_MAT(1, 2, 3)
MAT_MUL_MAT(1, 3, 3)
MAT_MUL_MAT(1, 4, 3)
MAT_MUL_MAT(1, 1, 4)
MAT_MUL_MAT(1, 2, 4)
MAT_MUL_MAT(1, 3, 4)
MAT_MUL_MAT(1, 4, 4)
MAT_MUL_MAT(2, 1, 1)
MAT_MUL_MAT(2, 2, 1)
MAT_MUL_MAT(2, 3, 1)
MAT_MUL_MAT(2, 4, 1)
MAT_MUL_MAT(2, 1, 2)
MAT_MUL_MAT(2, 2, 2)
MAT_MUL_MAT(2, 3, 2)
MAT_MUL_MAT(2, 4, 2)
MAT_MUL_MAT(2, 1, 3)
MAT_MUL_MAT(2, 2, 3)
MAT_MUL_MAT(2, 3, 3)
MAT_MUL_MAT(2, 4, 3)
MAT_MUL_MAT(2, 1, 4)
MAT_MUL_MAT(2, 2, 4)
MAT_MUL_MAT(2, 3, 4)
MAT_MUL_MAT(2, 4, 4)
MAT_MUL_MAT(3, 1, 1)
MAT_MUL_MAT(3, 2, 1)
MAT_MUL_MAT(3, 3, 1)
MAT_MUL_MAT(3, 4, 1)
MAT_MUL_MAT(3, 1, 2)
MAT_MUL_MAT(3, 2, 2)
MAT_MUL_MAT(3, 3, 2)
MAT_MUL_MAT(3, 4, 2)
MAT_MUL_MAT(3, 1, 3)
MAT_MUL_MAT(3, 2, 3)
MAT_MUL_MAT(3, 3, 3)
MAT_MUL_MAT(3, 4, 3)
MAT_MUL_MAT(3, 1, 4)
MAT_MUL_MAT(3, 2, 4)
MAT_MUL_MAT(3, 3, 4)
MAT_MUL_MAT(3, 4, 4)
MAT_MUL_MAT(4, 1, 1)
MAT_MUL_MAT(4, 2, 1)
MAT_MUL_MAT(4, 3, 1)
MAT_MUL_MAT(4, 4, 1)
MAT_MUL_MAT(4, 1, 2)
MAT_MUL_MAT(4, 2, 2)
MAT_MUL_MAT(4, 3, 2)
MAT_MUL_MAT(4, 4, 2)
MAT_MUL_MAT(4, 1, 3)
MAT_MUL_MAT(4, 2, 3)
MAT_MUL_MAT(4, 3, 3)
MAT_MUL_MAT(4, 4, 3)
MAT_MUL_MAT(4, 1, 4)
MAT_MUL_MAT(4, 2, 4)
MAT_MUL_MAT(4, 3, 4)
MAT_MUL_MAT(4, 4, 4)
VEC_MUL_MAT(1, 1)
VEC_MUL_MAT(1, 2)
VEC_MUL_MAT(1, 3)
VEC_MUL_MAT(1, 4)
VEC_MUL_MAT(2, 1)
VEC_MUL_MAT(2, 2)
VEC_MUL_MAT(2, 3)
VEC_MUL_MAT(2, 4)
VEC_MUL_MAT(3, 1)
VEC_MUL_MAT(3, 2)
VEC_MUL_MAT(3, 3)
VEC_MUL_MAT(3, 4)
VEC_MUL_MAT(4, 1)
VEC_MUL_MAT(4, 2)
VEC_MUL_MAT(4, 3)
VEC_MUL_MAT(4, 4)
MAT_INVERT_TRANSPOSE(2, 2)
MAT_INVERT_TRANSPOSE(3, 3)
MAT_INVERT_TRANSPOSE(4, 4)
MAT_INVERT(2, 2)
MAT_INVERT(3, 3)
MAT_INVERT(4, 4)
MAT_ADD(1, 1)
MAT_ADD(1, 2)
MAT_ADD(1, 3)
MAT_ADD(1, 4)
MAT_ADD(2, 1)
MAT_ADD(2, 2)
MAT_ADD(2, 3)
MAT_ADD(2, 4)
MAT_ADD(3, 1)
MAT_ADD(3, 2)
MAT_ADD(3, 3)
MAT_ADD(3, 4)
MAT_ADD(4, 1)
MAT_ADD(4, 2)
MAT_ADD(4, 3)
MAT_ADD(4, 4)
MAT_SUB(1, 1)
MAT_SUB(1, 2)
MAT_SUB(1, 3)
MAT_SUB(1, 4)
MAT_SUB(2, 1)
MAT_SUB(2, 2)
MAT_SUB(2, 3)
MAT_SUB(2, 4)
MAT_SUB(3, 1)
MAT_SUB(3, 2)
MAT_SUB(3, 3)
MAT_SUB(3, 4)
MAT_SUB(4, 1)
MAT_SUB(4, 2)
MAT_SUB(4, 3)
MAT_SUB(4, 4)

VEC_PRINT(1)
VEC_PRINT(2)
VEC_PRINT(3)
VEC_PRINT(4)
MAT_PRINT(1, 1)
MAT_PRINT(1, 2)
MAT_PRINT(1, 3)
MAT_PRINT(1, 4)
MAT_PRINT(2, 1)
MAT_PRINT(2, 2)
MAT_PRINT(2, 3)
MAT_PRINT(2, 4)
MAT_PRINT(3, 1)
MAT_PRINT(3, 2)
MAT_PRINT(3, 3)
MAT_PRINT(3, 4)
MAT_PRINT(4, 1)
MAT_PRINT(4, 2)
MAT_PRINT(4, 3)
MAT_PRINT(4, 4)

static inline double signed_triangle_area(int32_t ax, int32_t ay, int32_t bx,
                                          int32_t by, int32_t cx, int32_t cy)
{
    return 0.5 * ((by - ay) * (bx + ax) + (cy - by) * (cx + bx) +
                  (ay - cy) * (ax + cx));
}

#endif // GEOMETRY_H