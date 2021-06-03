/*
    This file is part of Graph Cut (Gc) combinatorial optimization library.
    Copyright (C) 2008-2010 Centre for Biomedical Image Analysis (CBIA)
    Copyright (C) 2008-2010 Ondrej Danek

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published 
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Gc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Graph Cut library. If not, see <http://www.gnu.org/licenses/>.
*/

/**
    @file
    Eigenvectors and eigenvalues decomposition routines.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#include "../Basic.h"
#include "Eigenvectors.h"

namespace Gc {
namespace Math {
namespace Algebra {
namespace Eigenvectors {
/************************************************************************/

/** Symmetric Householder reduction to tridiagonal form. */
template <Size N, class T>
static void SymmetricHouseholder(Matrix<N, N, T> & mat, Vector<N, T> & d, Vector<N, T> & e)
{
    d = mat.Row(N - 1);

    // Householder reduction to tridiagonal form.
    for (Size i = N - 1; i > 0; i--)
    {
        // Scale to avoid under/overflow.
        T scale(0), h(0);

        for (Size k = 0; k < i; k++)
        {
            scale = scale + Math::Abs(d[k]);
        }

        if (scale == 0.0)
        {
            e[i] = d[i - 1];
            for (Size j = 0; j < i; j++)
            {
                d[j] = mat(i - 1, j);
                mat(i, j) = 0;
                mat(j, i) = 0;
            }
        }
        else
        {
            for (Size k = 0; k < i; k++)
            {
                d[k] /= scale;
                h += d[k] * d[k];
            }

            T f = d[i - 1];
            T g = Math::Sqrt(h);

            if (f > 0)
            {
                g = -g;
            }

            e[i] = scale * g;
            h = h - f * g;
            d[i - 1] = f - g;

            for (Size j = 0; j < i; j++)
            {
                e[j] = 0;
            }

            // Apply similarity transformation to remaining columns.
            for (Size j = 0; j < i; j++)
            {
                f = d[j];
                mat(j, i) = f;
                g = e[j] + mat(j, j) * f;
                for (Size k = j + 1; k <= i - 1; k++)
                {
                    g += mat(k, j) * d[k];
                    e[k] += mat(k, j) * f;
                }
                e[j] = g;
            }

            f = 0.0;
            for (Size j = 0; j < i; j++)
            {
                e[j] /= h;
                f += e[j] * d[j];
            }

            T hh = f / (h + h);
            for (Size j = 0; j < i; j++)
            {
                e[j] -= hh * d[j];
            }

            for (Size j = 0; j < i; j++)
            {
                f = d[j];
                g = e[j];
                for (Size k = j; k <= i - 1; k++)
                {
                    mat(k, j) -= f * e[k] + g * d[k];
                }

                d[j] = mat(i - 1, j);
                mat(i, j) = 0.0;
            }
        }

        d[i] = h;
    }

    // Accumulate transformations.
    for (Size i = 0; i < N - 1; i++)
    {
        mat(N - 1, i) = mat(i, i);
        mat(i, i) = 1.0;
        T h = d[i + 1];
        if (h != 0)
        {
            for (Size k = 0; k <= i; k++)
            {
                d[k] = mat(k, i + 1) / h;
            }
            for (Size j = 0; j <= i; j++)
            {
                T g = 0.0;
                for (Size k = 0; k <= i; k++)
                {
                    g += mat(k, i + 1) * mat(k, j);
                }
                for (Size k = 0; k <= i; k++)
                {
                    mat(k, j) -= g * d[k];
                }
            }
        }

        for (Size k = 0; k <= i; k++)
        {
            mat(k, i + 1) = 0;
        }
    }

    for (Size j = 0; j < N; j++)
    {
        d[j] = mat(N - 1, j);
        mat(N - 1, j) = 0.0;
    }

    mat(N - 1, N - 1) = T(1);
    e[0] = 0;
}

/************************************************************************/

/** Symmetric tridiagonal QL algorithm. */
template <Size N, class T>
static void SymmetricTridiagonalQL(Matrix<N, N, T> & mat, Vector<N, T> & d, Vector<N, T> & e)
{
    for (Size i = 1; i < N; i++)
    {
        e[i - 1] = e[i];
    }
    e[N - 1] = 0;

    T f = 0;
    T tst1 = 0;
    T eps = Math::Pow(T(2), -T(52));

    for (Size l = 0; l < N; l++)
    {
        // Find small subdiagonal element
        tst1 = Math::Max(tst1, Math::Abs(d[l]) + Math::Abs(e[l]));
        Size m = l;
        while (m < N && Math::Abs(e[m]) > eps * tst1)
        {
            m++;
        }

        // If m == l, d[l] is an eigenvalue, otherwise, iterate.
        if (m > l)
        {
            Size iter = 0;

            do
            {
                iter = iter + 1; // (Could check iteration count here.)

                // Compute implicit shift
                T g = d[l];
                T p = (d[l + 1] - g) / (T(2) * e[l]);
                T r = Math::Sqrt(p * p + T(1));

                if (p < 0)
                {
                    r = -r;
                }
                d[l] = e[l] / (p + r);
                d[l + 1] = e[l] * (p + r);
                T dl1 = d[l + 1];
                T h = g - d[l];

                for (Size i = l + 2; i < N; i++)
                {
                    d[i] -= h;
                }
                f = f + h;

                // Implicit QL transformation.
                p = d[m];
                T c = 1.0;
                T c2 = c;
                T c3 = c;
                T el1 = e[l + 1];
                T s = 0.0;
                T s2 = 0.0;

                for (Size i = m - 1; i >= l; i--)
                {
                    c3 = c2;
                    c2 = c;
                    s2 = s;
                    g = c * e[i];
                    h = c * p;
                    r = Math::Sqrt(p * p + e[i] * e[i]);
                    e[i + 1] = s * r;
                    s = e[i] / r;
                    c = p / r;
                    p = c * d[i] - s * g;
                    d[i + 1] = h + s * (c * g + s * d[i]);

                    // Accumulate transformation.
                    for (Size k = 0; k < N; k++)
                    {
                        h = mat(k, i + 1);
                        mat(k, i + 1) = s * mat(k, i) + c * h;
                        mat(k, i) = c * mat(k, i) - s * h;
                    }
                }

                p = -s * s2 * c3 * el1 * e[l] / dl1;
                e[l] = s * p;
                d[l] = c * p;

            } while (Math::Abs(e[l]) > eps * tst1);
        }

        d[l] = d[l] + f;
        e[l] = 0;
    }
}

/************************************************************************/

template <Size N, class T>
void DecomposeSymmetric(const Matrix<N, N, T> & mat, Matrix<N, N, T> & eig_vec, Vector<N, T> & eig_val)
{
    Vector<N, T> e;

    eig_vec = mat;
    SymmetricHouseholder(eig_vec, eig_val, e);
    SymmetricTridiagonalQL(eig_vec, eig_val, e);
}

/************************************************************************/

// Explicit instantiations

/** @cond */
template GC_DLL_EXPORT void DecomposeSymmetric(const Matrix<2, 2, Float32> & mat, Matrix<2, 2, Float32> & eig_vec,
                                               Vector<2, Float32> & eig_val);
template GC_DLL_EXPORT void DecomposeSymmetric(const Matrix<2, 2, Float64> & mat, Matrix<2, 2, Float64> & eig_vec,
                                               Vector<2, Float64> & eig_val);
template GC_DLL_EXPORT void DecomposeSymmetric(const Matrix<3, 3, Float32> & mat, Matrix<3, 3, Float32> & eig_vec,
                                               Vector<3, Float32> & eig_val);
template GC_DLL_EXPORT void DecomposeSymmetric(const Matrix<3, 3, Float64> & mat, Matrix<3, 3, Float64> & eig_vec,
                                               Vector<3, Float64> & eig_val);
template GC_DLL_EXPORT void DecomposeSymmetric(const Matrix<4, 4, Float32> & mat, Matrix<4, 4, Float32> & eig_vec,
                                               Vector<4, Float32> & eig_val);
template GC_DLL_EXPORT void DecomposeSymmetric(const Matrix<4, 4, Float64> & mat, Matrix<4, 4, Float64> & eig_vec,
                                               Vector<4, Float64> & eig_val);
/** @endcond */

/************************************************************************/
}
}
}
} // namespace Gc::Math::Algebra::Eigenvectors
