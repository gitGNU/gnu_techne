/* Copyright (C) 2009 Papavasileiou Dimitris
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ALGEBRA_H_
#define _ALGEBRA_H_

#include <string.h>
#include <math.h>

#define t_scale_3(v, c)                         \
    {						\
        typeof((v)[0]) *_v = (v);               \
                                                \
        _v[0] *= c;				\
        _v[1] *= c;				\
        _v[2] *= c;				\
    }

#define t_scale_3r(M, c, i)                     \
    {						\
        typeof((M)[0]) *_M = (M);               \
                                                \
        _M[3 * i] *= c;                         \
        _M[3 * i + 1] *= c;                     \
        _M[3 * i + 2] *= c;                     \
    }

#define t_scale_3c(M, c, i)                     \
    {						\
        typeof((M)[0]) *_M = (M);               \
                                                \
        _M[i] *= c;				\
        _M[i + 3] *= c;				\
        _M[i + 6] *= c;				\
    }

#define t_dot_3(u, v) ((u)[0] * (v)[0] + (u)[1] * (v)[1] + (u)[2] * (v)[2])
#define t_dot_3c(M, v, i) ((M)[i] * (v)[0] + (M)[i + 3] * (v)[1] +      \
                           (M)[i + 6] * (v)[2])
#define t_dot_3r(M, v, i) ((M)[3 * i] * (v)[0] + (M)[3 * i + 1] * (v)[1] + \
                           (M)[3 * i + 2] * (v)[2])
#define t_length_3(u) (sqrt(t_dot_3(u, u)))

/* r = u . (v, 1), where u a 4-vector and v a 3-vector. */

#define t_dot_43(u, v) ((u)[0] * (v)[0] + (u)[1] * (v)[1] + (u)[2] * (v)[2] + (u[3]))

#define t_normalize_3(v)			\
    {						\
        typeof((v)[0]) m, *_v = v;              \
                                                \
        m = sqrt(t_dot_3(_v, _v));		\
        _v[0] /= m;				\
        _v[1] /= m;				\
        _v[2] /= m;				\
    }

#define t_cross(u, v_1, v_2)				\
    {							\
        typeof(u[0]) *_u = u;                           \
        typeof(v_1[0]) *_v_1 = v_1;                     \
        typeof(v_2[0]) *_v_2 = v_2;                     \
                                                        \
        _u[0] = _v_1[1] * _v_2[2] - _v_1[2] * _v_2[1];	\
        _u[1] = _v_1[2] * _v_2[0] - _v_1[0] * _v_2[2];	\
        _u[2] = _v_1[0] * _v_2[1] - _v_1[1] * _v_2[0];	\
    }

#define t_concatenate_3(C, A, B)                                       \
    {                                                                  \
       typeof(C[0]) *_C = C;                                           \
       typeof(A[0]) *_A = A;                                           \
       typeof(B[0]) *_B = B;                                           \
                                                                       \
       _C[0] = _A[0] * _B[0] + _A[1] * _B[3] + _A[2] * _B[6];          \
       _C[1] = _A[0] * _B[1] + _A[1] * _B[4] + _A[2] * _B[7];          \
       _C[2] = _A[0] * _B[2] + _A[1] * _B[5] + _A[2] * _B[8];          \
                                                                       \
       _C[3] = _A[3] * _B[0] + _A[4] * _B[3] + _A[5] * _B[6];          \
       _C[4] = _A[3] * _B[1] + _A[4] * _B[4] + _A[5] * _B[7];          \
       _C[5] = _A[3] * _B[2] + _A[4] * _B[5] + _A[5] * _B[8];          \
                                                                       \
       _C[6] = _A[6] * _B[0] + _A[7] * _B[3] + _A[8] * _B[6];          \
       _C[7] = _A[6] * _B[1] + _A[7] * _B[4] + _A[8] * _B[7];          \
       _C[8] = _A[6] * _B[2] + _A[7] * _B[5] + _A[8] * _B[8];          \
    }


#define t_concatenate_4(C, A, B)					\
    {									\
        typeof(C[0]) *_C = C;						\
        typeof(A[0]) *_A = A;						\
        typeof(B[0]) *_B = B;						\
                                                                        \
        _C[0] = _A[0] * _B[0] + _A[1] * _B[4] +                         \
            _A[2] * _B[8] + _A[3] * _B[12];                             \
        _C[1] = _A[0] * _B[1] + _A[1] * _B[5] +                         \
            _A[2] * _B[9] + _A[3] * _B[13];                             \
        _C[2] = _A[0] * _B[2] + _A[1] * _B[6] +                         \
            _A[2] * _B[10] + _A[3] * _B[14];                            \
        _C[3] = _A[0] * _B[3] + _A[1] * _B[7] +                         \
            _A[2] * _B[11] + _A[3] * _B[15];                            \
                                                                        \
        _C[4] = _A[4] * _B[0] + _A[5] * _B[4] +                         \
            _A[6] * _B[8] + _A[7] * _B[12];                             \
        _C[5] = _A[4] * _B[1] + _A[5] * _B[5] +                         \
            _A[6] * _B[9] + _A[7] * _B[13];                             \
        _C[6] = _A[4] * _B[2] + _A[5] * _B[6] +                         \
            _A[6] * _B[10] + _A[7] * _B[14];                            \
        _C[7] = _A[4] * _B[3] + _A[5] * _B[7] +                         \
            _A[6] * _B[11] + _A[7] * _B[15];                            \
                                                                        \
        _C[8] = _A[8] * _B[0] + _A[9] * _B[4] +                         \
            _A[10] * _B[8] + _A[11] * _B[12];                           \
        _C[9] = _A[8] * _B[1] + _A[9] * _B[5] +                         \
            _A[10] * _B[9] + _A[11] * _B[13];                           \
        _C[10] = _A[8] * _B[2] + _A[9] * _B[6] +                        \
            _A[10] * _B[10] + _A[11] * _B[14];                          \
        _C[11] = _A[8] * _B[3] + _A[9] * _B[7] +                        \
            _A[10] * _B[11] + _A[11] * _B[15];                          \
                                                                        \
        _C[12] = _A[12] * _B[0] + _A[13] * _B[4] +                      \
            _A[14] * _B[8] + _A[15] * _B[12];                           \
        _C[13] = _A[12] * _B[1] + _A[13] * _B[5] +                      \
            _A[14] * _B[9] + _A[15] * _B[13];                           \
        _C[14] = _A[12] * _B[2] + _A[13] * _B[6] +                      \
            _A[14] * _B[10] + _A[15] * _B[14];                          \
        _C[15] = _A[12] * _B[3] + _A[13] * _B[7] +                      \
            _A[14] * _B[11] + _A[15] * _B[15];                          \
    }

#define t_concatenate_4T(C, A, B)					\
    {									\
        typeof(C[0]) *_C = C;						\
        typeof(A[0]) *_A = A;						\
        typeof(B[0]) *_B = B;						\
                                                                        \
        _C[0] = _A[0] * _B[0] + _A[4] * _B[1] +                         \
            _A[8] * _B[2] + _A[12] * _B[3];                             \
        _C[4] = _A[0] * _B[4] + _A[4] * _B[5] +                         \
            _A[8] * _B[6] + _A[12] * _B[7];                             \
        _C[8] = _A[0] * _B[8] + _A[4] * _B[9] +                         \
            _A[8] * _B[10] + _A[12] * _B[11];                           \
        _C[12] = _A[0] * _B[12] + _A[4] * _B[13] +                      \
            _A[8] * _B[14] + _A[12] * _B[15];                           \
                                                                        \
        _C[1] = _A[1] * _B[0] + _A[5] * _B[1] +                         \
            _A[9] * _B[2] + _A[13] * _B[3];                             \
        _C[5] = _A[1] * _B[4] + _A[5] * _B[5] +                         \
            _A[9] * _B[6] + _A[13] * _B[7];                             \
        _C[9] = _A[1] * _B[8] + _A[5] * _B[9] +                         \
            _A[9] * _B[10] + _A[13] * _B[11];                           \
        _C[13] = _A[1] * _B[12] + _A[5] * _B[13] +                      \
            _A[9] * _B[14] + _A[13] * _B[15];                           \
                                                                        \
        _C[2] = _A[2] * _B[0] + _A[6] * _B[1] +                         \
            _A[10] * _B[2] + _A[14] * _B[3];                            \
        _C[6] = _A[2] * _B[4] + _A[6] * _B[5] +                         \
            _A[10] * _B[6] + _A[14] * _B[7];                            \
        _C[10] = _A[2] * _B[8] + _A[6] * _B[9] +                        \
            _A[10] * _B[10] + _A[14] * _B[11];                          \
        _C[14] = _A[2] * _B[12] + _A[6] * _B[13] +                      \
            _A[10] * _B[14] + _A[14] * _B[15];                          \
                                                                        \
        _C[3] = _A[3] * _B[0] + _A[7] * _B[1] +                         \
            _A[11] * _B[2] + _A[15] * _B[3];                            \
        _C[7] = _A[3] * _B[4] + _A[7] * _B[5] +                         \
            _A[11] * _B[6] + _A[15] * _B[7];                            \
        _C[11] = _A[3] * _B[8] + _A[7] * _B[9] +                        \
            _A[11] * _B[10] + _A[15] * _B[11];                          \
        _C[15] = _A[3] * _B[12] + _A[7] * _B[13] +                      \
            _A[11] * _B[14] + _A[15] * _B[15];                          \
    }

/* u = T * v, where T a 3x3 matrix and v a 3-vector. */

#define t_transform_3(u, T, v)                                         \
    {                                                                  \
       typeof(u[0]) *_u = u;                                           \
       typeof(T[0]) *_T = T;                                           \
       typeof(v[0]) *_v = v;                                           \
                                                                       \
       _u[0] = _T[0] * _v[0] + _T[1] * _v[1] + _T[2] * _v[2];          \
       _u[1] = _T[3] * _v[0] + _T[4] * _v[1] + _T[5] * _v[2];          \
       _u[2] = _T[6] * _v[0] + _T[7] * _v[1] + _T[8] * _v[2];          \
    }

#define t_transform_3T(u, T, v)                                        \
    {                                                                  \
       typeof(u[0]) *_u = u;                                           \
       typeof(T[0]) *_T = T;                                           \
       typeof(v[0]) *_v = v;                                           \
                                                                       \
       _u[0] = _T[0] * _v[0] + _T[3] * _v[1] + _T[6] * _v[2];          \
       _u[1] = _T[1] * _v[0] + _T[4] * _v[1] + _T[7] * _v[2];          \
       _u[2] = _T[2] * _v[0] + _T[5] * _v[1] + _T[8] * _v[2];          \
    }

/* u = T * v + w, where T a 3x3 matrix and v, w are 3-vectors.  (H
 * stands for homogenous). */

#define t_transform_3H(u, T, v, w)                                     \
    {                                                                  \
       typeof(u[0]) *_u = u;                                           \
       typeof(T[0]) *_T = T;                                           \
       typeof(v[0]) *_v = v;                                           \
       typeof(w[0]) *_w = w;                                           \
                                                                       \
       _u[0] = _T[0] * _v[0] + _T[1] * _v[1] + _T[2] * _v[2] + _w[0];  \
       _u[1] = _T[3] * _v[0] + _T[4] * _v[1] + _T[5] * _v[2] + _w[1];  \
       _u[2] = _T[6] * _v[0] + _T[7] * _v[1] + _T[8] * _v[2] + _w[2];  \
    }

/* u = T * (v, 1), where T a 3x4 matrix and v a 3-vector. */

#define t_transform_43(u, T, v)                                         \
    {									\
        typeof(u[0]) *_u = u;						\
        typeof(T[0]) *_T = T;						\
        typeof(v[0]) *_v = v;						\
                                                                        \
        _u[0] = _T[0] * _v[0] + _T[1] * _v[1] + _T[2] * _v[2] + _T[3];  \
        _u[1] = _T[4] * _v[0] + _T[5] * _v[1] + _T[6] * _v[2] + _T[7];  \
        _u[2] = _T[8] * _v[0] + _T[8] * _v[1] + _T[10] * _v[2] + _T[11]; \
    }

#define t_transform_4T3(u, T, v)                                        \
    {									\
        typeof(u[0]) *_u = u;						\
        typeof(T[0]) *_T = T;						\
        typeof(v[0]) *_v = v;						\
                                                                        \
        _u[0] = _T[0] * _v[0] + _T[4] * _v[1] + _T[8] * _v[2] + _T[12]; \
        _u[1] = _T[1] * _v[0] + _T[5] * _v[1] + _T[9] * _v[2] + _T[13]; \
        _u[2] = _T[2] * _v[0] + _T[6] * _v[1] + _T[10] * _v[2] + _T[14]; \
    }

/* R stands for Reduced, or perhaps, Rotation.  In any case only the
 * upper 3x3 rotation matrix is used. */

#define t_transform_4R3(u, T, v)                                        \
    {									\
        typeof(u[0]) *_u = u;						\
        typeof(T[0]) *_T = T;						\
        typeof(v[0]) *_v = v;						\
                                                                        \
        _u[0] = _T[0] * _v[0] + _T[1] * _v[1] + _T[2] * _v[2];          \
        _u[1] = _T[4] * _v[0] + _T[5] * _v[1] + _T[6] * _v[2];          \
        _u[2] = _T[8] * _v[0] + _T[8] * _v[1] + _T[10] * _v[2];         \
    }

#define t_transform_4RT3(u, T, v)                                       \
    {									\
        typeof(u[0]) *_u = u;						\
        typeof(T[0]) *_T = T;						\
        typeof(v[0]) *_v = v;						\
                                                                        \
        _u[0] = _T[0] * _v[0] + _T[4] * _v[1] + _T[8] * _v[2];          \
        _u[1] = _T[1] * _v[0] + _T[5] * _v[1] + _T[9] * _v[2];          \
        _u[2] = _T[2] * _v[0] + _T[6] * _v[1] + _T[10] * _v[2];         \
    }

#define t_transpose_4(T, A)                                             \
    {									\
        T[0] = A[0]; T[1] = A[4]; T[2] = A[8]; T[3] = A[12];            \
        T[4] = A[1]; T[5] = A[5]; T[6] = A[9]; T[7] = A[13];            \
        T[8] = A[2]; T[9] = A[6]; T[10] = A[10]; T[11] = A[14];         \
        T[12] = A[3]; T[13] = A[7]; T[14] = A[11]; T[15] = A[15];       \
    }

#define t_load_orthographic(M, l, r, b, t, n, f)	\
    {							\
        typeof(M[0]) dx, dy, dz;			\
                                                        \
        dx = r - l;					\
        dy = t - b;					\
        dz = f - n;					\
                                                        \
        memset (M, 0, 16 * sizeof(M[0]));		\
        M[0] = 2.0 / dx;				\
        M[12] = -(r + l) / dx;				\
        M[5] = 2.0 / dy;				\
        M[13] = -(t + b) / dy;				\
        M[10] = -2.0 / dz;				\
        M[14] = -(f + n) / dz;				\
        M[15] = 1;					\
    }

#define t_load_identity_3(M)                           \
    {                                                  \
       memset (M, 0, 9 * sizeof(M[0]));                \
       M[0] = 1;                                       \
       M[4] = 1;                                       \
       M[8] = 1;                                       \
    }


#define t_load_identity_4(M)				\
    {							\
        memset (M, 0, 16 * sizeof(M[0]));		\
        M[0] = 1;					\
        M[5] = 1;					\
        M[10] = 1;					\
        M[15] = 1;					\
    }

#define t_load_zero_4(M)				\
    {							\
        memset (M, 0, 16 * sizeof(M[0]));		\
    }

#define t_rotate_axis_angle(M, u, theta)                \
    {                                                   \
        double c, c_1, s;                               \
                                                        \
        c = cos(theta);                                 \
        c_1 = 1.0 - c;                                  \
        s = sin(theta);                                 \
                                                        \
        M[0 * 3 + 0] = c + u[0] * u[0] * c_1;           \
        M[0 * 3 + 1] = u[0] * u[1] * c_1 - u[2] * s;    \
        M[0 * 3 + 2] = u[0] * u[2] * c_1 + u[1] * s;    \
                                                        \
        M[1 * 3 + 0] = u[1] * u[0] * c_1 + u[2] * s;    \
        M[1 * 3 + 1] = c + u[1] * u[1] * c_1;           \
        M[1 * 3 + 2] = u[1] * u[2] * c_1 - u[0] * s;    \
                                                        \
        M[2 * 3 + 0] = u[2] * u[0] * c_1 - u[1] * s;    \
        M[2 * 3 + 1] = u[2] * u[1] * c_1 + u[0] * s;    \
        M[2 * 3 + 2] = c + u[2] * u[2] * c_1;           \
    }

#endif
