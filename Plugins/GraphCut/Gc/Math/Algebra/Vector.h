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
    Linear algebra vector class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_MATH_ALGEBRA_VECTOR_H
#define GC_MATH_ALGEBRA_VECTOR_H

#include <math.h>
#include "../../Type.h"
#include "../../System/Collection/Tuple.h"
#include "../Basic.h"
#include "../Constant.h"
#include "Number.h"

namespace Gc
{
    namespace Math
    {
        /** Implementations of various objects from algebra. */
        namespace Algebra
        {
            // Forward declaration
            template <Size N, Size M, class T> class Matrix;

            /** Class representing vector object from linear algebra. 
            
                @tparam N Dimensionality of the space.
                @tparam T %Data type.
            */
            template <Size N, class T>
            class Vector
                : public System::Collection::Tuple<N,T>
            {
            protected:
                using System::Collection::Tuple<N,T>::m_data;

            public:
                /** Default constructor. 
                
                    No initialization of the elements is done.
                */
                Vector()
                {}

                /** Constructor. 
                
                    Initializes all elements with given value.
                */
                explicit Vector(const T& v)
                    : System::Collection::Tuple<N,T>(v)
                {}

                /** Constructor useful for 2D vectors. */
                Vector(const T& v1, const T& v2)
                    : System::Collection::Tuple<N,T>(v1,v2)
                {}

                /** Constructor useful for 3D vectors. */
                Vector(const T& v1, const T& v2, const T& v3)
                    : System::Collection::Tuple<N,T>(v1,v2,v3)
                {}

                /** Constructor useful for 4D vectors. */
                Vector(const T& v1, const T& v2, const T& v3, const T& v4)
                    : System::Collection::Tuple<N,T>(v1,v2,v3,v4)
                {}

                /** Copy constructor. */
                Vector(const Vector<N,T> &v)
                {
                    *this = v;
                }

                /** Assignment operator. */
                Vector<N,T>& operator= (const Vector<N,T> &v)
                {
                    System::Collection::Tuple<N,T>::operator=(v);
                    return *this;
                }

                /** %Vector addition operator. */
                Vector<N,T>& operator+= (const Vector<N,T> &v)
                {
                    for (Size i = 0; i < N; i++)
                    {
                        m_data[i] += v.m_data[i];
                    }

                    return *this;
                }

                /** %Vector substraction operator. */
                Vector<N,T>& operator-= (const Vector<N,T> &v)
                {
                    for (Size i = 0; i < N; i++)
                    {
                        m_data[i] -= v.m_data[i];
                    }

                    return *this;
                }

                /** Pointwise vector multiplication operator. */
                Vector<N,T>& operator*= (const Vector<N,T> &v)
                {
                    for (Size i = 0; i < N; i++)
                    {
                        m_data[i] *= v.m_data[i];
                    }

                    return *this;
                }

                /** Multiplication by scalar value. */
                Vector<N,T>& operator*= (const T& v)
                {
                    for (Size i = 0; i < N; i++)
                    {
                        m_data[i] *= v;
                    }

                    return *this;
                }

                /** Pointwise vector division operator. */
                Vector<N,T>& operator/= (const Vector<N,T> &v)
                {
                    for (Size i = 0; i < N; i++)
                    {
                        m_data[i] /= v.m_data[i];
                    }

                    return *this;
                }

                /** %Vector division by scalar. */
                Vector<N,T>& operator/= (const T& v)
                {
                    for (Size i = 0; i < N; i++)
                    {
                        m_data[i] /= v;
                    }

                    return *this;
                }

                /** %Vector addition operator. */
                Vector<N,T> operator+ (const Vector<N,T>& v) const
                {
                    Vector<N,T> nv;

                    for (Size i = 0; i < N; i++)
                    {
                        nv.m_data[i] = m_data[i] + v.m_data[i];
                    }

                    return nv;
                }

                /** %Vector substraction operator. */
                Vector<N,T> operator- (const Vector<N,T>& v) const
                {
                    Vector<N,T> nv;

                    for (Size i = 0; i < N; i++)
                    {
                        nv.m_data[i] = m_data[i] - v.m_data[i];
                    }

                    return nv;
                }

                /** Pointwise vector multiplication. */
                Vector<N,T> operator* (const Vector<N,T> &v) const
                {
                    Vector<N,T> nv;

                    for (Size i = 0; i < N; i++)
                    {
                        nv.m_data[i] = m_data[i] * v.m_data[i];
                    }

                    return nv;
                }

                /** %Vector multiplication by scalar. */
                Vector<N,T> operator* (const T& v) const
                {
                    Vector<N,T> nv;

                    for (Size i = 0; i < N; i++)
                    {
                        nv.m_data[i] = m_data[i] * v;
                    }

                    return nv;
                }

                /** Pointwise vector division. */
                Vector<N,T> operator/ (const Vector<N,T> &v) const
                {
                    Vector<N,T> nv;

                    for (Size i = 0; i < N; i++)
                    {
                        nv.m_data[i] = m_data[i] / v.m_data[i];
                    }

                    return nv;
                }

                /** %Vector division by scalar. */
                Vector<N,T> operator/ (const T& v) const
                {
                    Vector<N,T> nv;

                    for (Size i = 0; i < N; i++)
                    {
                        nv.m_data[i] = m_data[i] / v;
                    }

                    return nv;
                }

                /** Unary minus operator. */
                Vector<N,T> operator- () const
                {
                    return *this * -1;
                }

                /** %Vector by matrix multiplication operator. */
                template <Size M>
                Vector<M,T> Mul (const Matrix<N,M,T> &m) const
                {
                    Vector<M,T> v;

                    for (Size i = 0; i < M; i++)
                    {
                        Size p = i;
                        T sum = 0;

                        for (Size k = 0; k < N; k++, p += M)
                        {
                            sum += m_data[k] * m[p];
                        }

                        v[i] = sum;
                    }

                    return v;
                }

                /** Lexicographical comparison.
                
                    @remarks Most significant value is the last element.
                */
                bool operator< (const Vector<N,T> &v) const
                {
                    for (Size i = N; i-- > 0; )
                    {
                        if (m_data[i] < v.m_data[i])
                        {
                            return true;
                        }
                        else if (m_data[i] > v.m_data[i])
                        {
                            return false;
                        }
                    }

                    return false;
                }

                /** Lexicographical comparison. 
                
                    @remarks Most significant value is the last element.
                */
                bool operator<= (const Vector<N,T> &v) const
                {
                    for (Size i = N; i-- > 0; )
                    {
                        if (m_data[i] < v.m_data[i])
                        {
                            return true;
                        }
                        else if (m_data[i] > v.m_data[i])
                        {
                            return false;
                        }
                    }

                    return true;
                }

                /** Lexicographical comparison. */
                bool operator> (const Vector<N,T> &v) const
                {
                    return v < *this;
                }

                /** Lexicographical comparison. */
                bool operator>= (const Vector<N,T> &v) const
                {
                    return v <= *this;
                }

                /** Get maximum of vector element values. */
                T Max() const
                {
                    T mx = m_data[0];

                    for (Size i = 1; i < N; i++)
                    {
                        if (m_data[i] > mx)
                        {
                            mx = m_data[i];
                        }
                    }

                    return mx;
                }

                /** Get position of the element with the maximum value. */
                Size MaxPos() const
                {
                    Size mp(0);

                    for (Size i = 1; i < N; i++)
                    {
                        if (m_data[i] > m_data[mp])
                        {
                            mp = i;
                        }
                    }

                    return mp;
                }

                /** Get minimum of vector element values. */
                T Min() const
                {
                    T mn = m_data[0];

                    for (Size i = 1; i < N; i++)
                    {
                        if (m_data[i] < mn)
                        {
                            mn = m_data[i];
                        }
                    }

                    return mn;
                }

                /** Get position of the element with the minimum value. */
                Size MinPos() const
                {
                    Size mp(0);

                    for (Size i = 1; i < N; i++)
                    {
                        if (m_data[i] < m_data[mp])
                        {
                            mp = i;
                        }
                    }

                    return mp;
                }

                /** Compute the sum of vector elements. */
                T Sum () const
                {
                    T sum = 0;

                    for (Size i = 0; i < N; i++)
                    {
                        sum += m_data[i];
                    }

                    return sum;
                }

                /** Compute the product of vector elements. */
                T Product () const
                {
                    T prod = 1;

                    for (Size i = 0; i < N; i++)
                    {
                        prod *= m_data[i];
                    }

                    return prod;
                }

                /** Outer %vector product. */
                template <Size M>
                Matrix<N,M,T> OuterProduct (const Vector<M,T> &v)
                {
                    Matrix<N,M,T> m;
                    Size p = 0;

                    for (Size i = 0; i < N; i++)
                    {
                        for (Size j = 0; j < M; j++, p++)
                        {
                            m[p] = m_data[i] * v[j];
                        }
                    }

                    return m;
                }

                /** Compute Euclidean dot product. */
                T DotProduct (const Vector<N,T> &v) const
                {
                    T sum = 0;

                    for (Size i = 0; i < N; i++)
                    {
                        sum += m_data[i] * v.m_data[i];
                    }

                    return sum;
                }

                /** Compute inner product using given matrix. */
                template <Size M>
                T InnerProduct (const Matrix<N,M,T> &m, const Vector<N,T> &v) const
                {
                    return Mul(m).DotProduct(v);
                }

                /** Convert row vector to a matrix. */
                Matrix<1,N,T> ToRowMatrix() const
                {
                    Matrix<1,N,T> m;

                    for (Size i = 0; i < N; i++)
                    {
                        m[i] = m_data[i];
                    }

                    return m;
                }

                /** Convert column vector to a matrix. */
                Matrix<N,1,T> ToColumnMatrix() const
                {
                    Matrix<N,1,T> m;

                    for (Size i = 0; i < N; i++)
                    {
                        m[i] = m_data[i];
                    }

                    return m;
                }

                /** Euclidean length of the vector. */
                T Length() const
                {
                    return Math::Sqrt(DotProduct(*this));
                }

                /** Length of the vector in specific metric tensor space. 
                
                    This method computes \f$ \sqrt{v^T \cdot m \cdot v} \f$.
                    
                    @param[in] m Metric tensor. Positive definite symmetric bilinear form.
                */
                T Length(const Matrix<N,N,T> &m) const
                {
                    return Math::Sqrt(InnerProduct(m,*this));
                }

                /** Angle between two vectors in Euclidean space. 

                    @return Angle in radians.
                */
                T Angle(const Vector<N,T> &v) const
                {
                    return acos(DotProduct(v) / (Length() * v.Length()));
                }

                /** Angle between two vectors in specific metric tensor space. 

                    @param[in] v A vector.
                    @param[in] m Metric tensor. Positive definite symmetric bilinear form.

                    @return Angle in radians.
                */
                T Angle(const Vector<N,T> &v, const Matrix<N,N,T> &m) const
                {
                    return acos(InnerProduct(m,v) / (Length(m) * v.Length(m)));
                }

                /** Normalize the %vector. */
                Vector<N,T>& Normalize()
                {
                    T len = Length();

                    for (Size i = 0; i < N; i++)
                    {
                        m_data[i] /= len;
                    }

                    return *this;
                }

                /** Get normalized version of the %vector. */
                Vector<N,T> Normalized() const
                {
                    Vector<N,T> nv;
                    T len = Length();

                    for (Size i = 0; i < N; i++)
                    {
                        nv.m_data[i] = m_data[i] / len;
                    }

                    return nv;
                }

                /** Convert %vector to another data type or another dimension. 
                
                    If \c N2 is greater than \c N, then the %vector is truncated,
                    otherwise it is padded with \c pad_val.
                */
                template <Size CN,class CT>
                Vector<CN,CT> Convert(CT pad_val = 0) const
                {
                    Vector<CN,CT> n;

                    if (CN < N)
                    {
                        for (Size i = 0; i < CN; i++)
                        {
                            n[i] = (CT)m_data[i];
                        }
                    }
                    else
                    {
                        for (Size i = 0; i < N; i++)
                        {
                            n[i] = (CT)m_data[i];
                        }
                        for (Size i = N; i < CN; i++)
                        {
                            n[i] = pad_val;
                        }
                    }

                    return n;
                }

                /** %Vector sign. 
                
                    @return -1 for vectors that are less than zero vector, 1 for vectors 
                    greater than zero vector and 0 for zero vector.
                    
                    @remarks Most significant value is the last element.
                */
                Int8 Sgn() const
                {
                    for (Size i = N; i-- > 0; )
                    {
                        if (m_data[i] < 0)
                        {
                            return -1;
                        }
                        else if (m_data[i] > 0)
                        {
                            return 1;
                        }
                    }

                    return 0;
                }

                /** Set all vector elements to zero. */
                Vector<N,T>& SetZero()
                {
                    System::Algo::Range::Fill(this->Begin(), this->End(), T(0));
                    return *this;
                }

                /** Returns the greatest common divisor. */
                T Gcd() const
                {
                    return Algebra::Gcd(m_data, N);
                }

                /** Returns the least common multiple. */
                T Lcm() const
                {
                    return Algebra::Lcm(m_data, N);
                }
            };

            /** %Vector by scalar multiplication operator. */
            template <Size N, class T>
            Vector<N,T> operator* (const T& val, const Vector<N,T> &v)
            {
                return v * val;
            }

            /** Inverse vector by scalar division operator. */
            template <Size N, class T>
            Vector<N,T> operator/ (const T& val, const Vector<N,T> &v)
            {
                Vector<N,T> nv;

                for (Size i = 0; i < N; i++)
                {
                    nv[i] = val / v[i];
                }

                return nv;
            }

            /** Cross product of two 3D vectors. */
            template <class T>
            Vector<3,T> CrossProduct(const Vector<3,T> &v1, const Vector<3,T> &v2)
            {
                Vector<3,T> nv;

                nv[0] = v1[1] * v2[2] - v1[2] * v2[1];
                nv[1] = v1[2] * v2[0] - v1[0] * v2[2];
                nv[2] = v1[0] * v2[1] - v1[1] * v2[0];

                return nv;
            }

            /** Calculates the clockwise angle between two 2D vectors. */
            template <class T>
            T ClockwiseAngle(const Vector<2,T> &v1, const Vector<2,T> &v2)
            {
                T ang = v1.Angle(v2);
                
                // Check the third coordinate of cross product
                if (v1[0] * v2[1] - v1[1] * v2[0] > 0)
                {
                    return 2 * T(Constant::Pi) - ang;
                }

                return ang;
            }

            /** Calculate the angular orientation of a vector. */
            template <class T>
            T AngularOrientation(const Vector<2,T> &v)
            {
                T angle = acos(v[0] / v.Length());
                return (v[1] < 0) ? (2 * T(Constant::Pi) - angle) : angle;
            }
        }
    }
}

#endif
