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
    Linear algebra matrix class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_MATH_ALGEBRA_MATRIX_H
#define GC_MATH_ALGEBRA_MATRIX_H

#include "../../Type.h"
#include "../../System/Collection/Tuple.h"

namespace Gc {
namespace Math {
namespace Algebra {
// Forward declaration
template <Size N, class T>
class Vector;

/** Class representing general matrix object from linear algebra.
            
                @tparam N Number of rows.
                @tparam M Number of columns.
                @tparam T %Data type.
            */
template <Size N, Size M, class T>
class Matrix
    : public System::Collection::Tuple<N * M, T>
{
  protected:
    using System::Collection::Tuple<N * M, T>::m_data;

  public:
    /** Default constructor. 
                
                    No initialization of the data is done.
                */
    Matrix() = default;

    /** Constructor. 
                
                    Initializes all elements with given value.
                */
    explicit Matrix(const T & v)
        : System::Collection::Tuple<N * M, T>(v)
    {}

    /** Copy constructor. */
    Matrix(const Matrix<N, M, T> & m)
    {
        *this = m;
    }

    /** Assignment operator. */
    Matrix<N, M, T> & operator=(const Matrix<N, M, T> & m)
    {
        System::Collection::Tuple<N * M, T>::operator=(m);
        return *this;
    }

    /** Get number of rows. */
    Size Rows() const
    {
        return N;
    }

    /** Get number of columns. */
    Size Columns() const
    {
        return M;
    }

    /** Get element at given position. */
    T & operator[](Size i)
    {
        return m_data[i];
    }

    /** Get element at given position. */
    const T & operator[](Size i) const
    {
        return m_data[i];
    }

    /** Get element at given position. 
                
                    @param[in] i Row number.
                    @param[in] j Column number. 
                    */
    T & operator()(Size i, Size j)
    {
        return m_data[i * M + j];
    }

    /** Get element at given position. 
                
                    @param[in] i Row number.
                    @param[in] j Column number. 
                    */
    const T & operator()(Size i, Size j) const
    {
        return m_data[i * M + j];
    }

    /** Transpose %matrix. */
    Matrix<M, N, T> Transpose() const
    {
        Matrix<M, N, T> m;
        Size p1 = 0, p2;

        for (Size i = 0; i < N; i++)
        {
            p2 = i;

            for (Size j = 0; j < M; j++, p1++, p2 += N)
            {
                m[p2] = m_data[p1];
            }
        }

        return m;
    }

    /** Pointwise substraction. */
    Matrix<N, M, T> operator-(const Matrix<N, M, T> & m) const
    {
        Matrix<N, M, T> n;

        for (Size i = N * M; i-- > 0;)
        {
            n.m_data[i] = m_data[i] - m.m_data[i];
        }

        return n;
    }

    /** Pointwise substraction. */
    Matrix<N, M, T> & operator-=(const Matrix<N, M, T> & m)
    {
        for (Size i = N * M; i-- > 0;)
        {
            m_data[i] -= m.m_data[i];
        }

        return *this;
    }

    /** Pointwise addition. */
    Matrix<N, M, T> operator+(const Matrix<N, M, T> & m) const
    {
        Matrix<N, M, T> n;

        for (Size i = N * M; i-- > 0;)
        {
            n.m_data[i] = m_data[i] + m.m_data[i];
        }

        return n;
    }

    /** Pointwise addition. */
    Matrix<N, M, T> & operator+=(const Matrix<N, M, T> & m)
    {
        for (Size i = N * M; i-- > 0;)
        {
            m_data[i] += m.m_data[i];
        }

        return *this;
    }

    /** Pointwise multiplication. */
    Matrix<N, M, T> operator*(const Matrix<N, M, T> & m) const
    {
        Matrix<N, M, T> n;

        for (Size i = N * M; i-- > 0;)
        {
            n.m_data[i] = m_data[i] * m.m_data[i];
        }

        return n;
    }

    /** Pointwise multiplication. */
    Matrix<N, M, T> & operator*=(const Matrix<N, M, T> & m)
    {
        for (Size i = N * M; i-- > 0;)
        {
            m_data[i] *= m.m_data[i];
        }

        return *this;
    }

    /** Pointwise division. */
    Matrix<N, M, T> operator/(const Matrix<N, M, T> & m) const
    {
        Matrix<N, M, T> n;

        for (Size i = N * M; i-- > 0;)
        {
            n.m_data[i] = m_data[i] * m.m_data[i];
        }

        return n;
    }

    /** Pointwise division. */
    Matrix<N, M, T> & operator/=(const Matrix<N, M, T> & m)
    {
        for (Size i = N * M; i-- > 0;)
        {
            m_data[i] /= m.m_data[i];
        }

        return *this;
    }

    /** Multiplication by scalar. */
    Matrix<N, M, T> operator*(T v) const
    {
        Matrix<N, M, T> n;

        for (Size i = N * M; i-- > 0;)
        {
            n.m_data[i] = m_data[i] * v;
        }

        return n;
    }

    /** Multiplication by scalar. */
    Matrix<N, M, T> & operator*=(T v)
    {
        for (Size i = N * M; i-- > 0;)
        {
            m_data[i] *= v;
        }

        return *this;
    }

    /** Division by scalar. */
    Matrix<N, M, T> operator/(T v) const
    {
        Matrix<N, M, T> n;

        for (Size i = N * M; i-- > 0;)
        {
            n.m_data[i] = m_data[i] / v;
        }

        return n;
    }

    /** Division by scalar. */
    Matrix<N, M, T> & operator/=(T v)
    {
        for (Size i = N * M; i-- > 0;)
        {
            m_data[i] /= v;
        }

        return *this;
    }

    /** Get sum of matrix elements. */
    T Sum() const
    {
        T sum = 0;

        for (Size i = N * M; i-- > 0;)
        {
            sum += m_data[i];
        }

        return sum;
    }

    /** Get the product of matrix elements. */
    T Product() const
    {
        T prod = 1;

        for (Size i = N * M; i-- > 0;)
        {
            prod *= m_data[i];
        }

        return prod;
    }

    /** Get selected row. */
    Vector<M, T> Row(Size r) const
    {
        Vector<M, T> v;
        Size p = r * M;

        for (Size i = 0; i < M; i++, p++)
        {
            v[i] = m_data[p];
        }

        return v;
    }

    /** Set selected row. */
    Matrix<N, M, T> & SetRow(Size r, const Vector<M, T> & v)
    {
        Size p = r * M;

        for (Size i = 0; i < M; i++, p++)
        {
            m_data[p] = v[i];
        }

        return *this;
    }

    /** Get selected column. */
    Vector<N, T> Column(Size c) const
    {
        Vector<N, T> v;
        Size p = c;

        for (Size i = 0; i < M; i++, p += M)
        {
            v[i] = m_data[p];
        }

        return v;
    }

    /** Set selected column. */
    Matrix<N, M, T> & SetCol(Size c, const Vector<N, T> & v)
    {
        Size p = c;

        for (Size i = 0; i < N; i++, p += M)
        {
            m_data[p] = v[i];
        }

        return *this;
    }

    /** %Matrix multiplication. */
    template <Size K>
    Matrix<N, K, T> Mul(const Matrix<M, K, T> & m) const
    {
        Matrix<N, K, T> n;
        Size p3 = 0;

        for (Size i = 0; i < N; i++)
        {
            for (Size j = 0; j < K; j++)
            {
                Size p1 = i * M, p2 = j;
                T sum = 0;

                for (Size k = 0; k < M; k++)
                {
                    sum += m_data[p1] * m[p2];

                    p1++;
                    p2 += K;
                }

                n[p3++] = sum;
            }
        }

        return n;
    }

    /** Multiply %matrix by a vector. */
    Vector<N, T> Mul(const Vector<M, T> & v) const
    {
        Vector<N, T> w;

        for (Size i = 0; i < N; i++)
        {
            Size p = i * M;
            T sum = 0;

            for (Size k = 0; k < M; k++, p++)
            {
                sum += m_data[p] * v[k];
            }

            w[i] = sum;
        }

        return w;
    }

    /** Set all matrix elements to zero. */
    Matrix<N, M, T> & SetZero()
    {
        System::Algo::Range::Fill(this->Begin(), this->End(), T(0));
        return *this;
    }
};

/** Multiplication by scalar. */
template <Size N, Size M, class T>
Matrix<N, M, T> operator*(const Matrix<N, M, T> & m, const T & v)
{
    return m * v;
}

/** Division by scalar. */
template <Size N, Size M, class T>
Matrix<N, M, T> operator/(const Matrix<N, M, T> & m, const T & v)
{
    Matrix<N, M, T> n;

    for (Size i = N * M; i-- > 0;)
    {
        n[i] = v / m[i];
    }

    return n;
}
}
}
} // namespace Gc::Math::Algebra

#endif
