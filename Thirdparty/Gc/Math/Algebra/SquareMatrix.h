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
    Square matrix class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_MATH_ALGEBRA_SQUAREMATRIX_H
#define GC_MATH_ALGEBRA_SQUAREMATRIX_H

#include "../../System/NotImplementedException.h"
#include "Matrix.h"

namespace Gc
{
    namespace Math
    {
        namespace Algebra
        {
            /** Class representing square matrices from linear algebra.

                @tparam N Number of rows and columns.
                @tparam T %Data type.
            */
            template <Size N, class T>
            class SquareMatrix
                : public Matrix<N,N,T>
            {
            protected:
                using Matrix<N,N,T>::m_data;

            public:
                /** Default constructor. 
                
                    No initialization of the data is done.
                */
                SquareMatrix()
                    : Matrix<N,N,T>()
                {}

                /** Constructor. 
                
                    @param[in] val All matrix elements will be set to given val.
                */
                explicit SquareMatrix(const T& val)
                    : Matrix<N,N,T>(val)
                {}

                /** Constructor. */
                explicit SquareMatrix(const Matrix<N,N,T>& m)
                    : Matrix<N,N,T>(m)
                {}

                /** Constructor. 
                
                    @param[in] v Vector to be put on the main diagonal of the matrix. All
                        other elements will be zero.
                */
                explicit SquareMatrix(const Vector<N,T> &v)
                    : Matrix<N,N,T>()
                {
                    SetMainDiagonal(v);
                }

                /** Set main diagonal of the matrix to given vector. 
                
                    Remaining matrix elements are set to zero.
                */
                SquareMatrix<N,T>& SetMainDiagonal(const Vector<N,T> &v)
                {
                    System::Algo::Range::Fill(this->Begin(), this->End(), T(0));

                    T *dst = this->Begin();
                    for (Size i = 0; i < N; i++, dst += N+1)
                    {
                        *dst = v[i];
                    }

                    return *this;
                }

                /** Get main diagonal as a vector. */
                Vector<N,T> MainDiagonal() const
                {
                    Vector<N,T> v;
                    
                    T *src = this->Begin();
                    for (Size i = 0; i < N; i++, src += N + 1)
                    {
                        v[i] = *src;
                    }

                    return v;
                }

                /** Calculate the determinant of the matrix. */
                T Determinant() const
                {
                    if (N == 2)
                    {
                        return m_data[0] * m_data[3] - m_data[1] * m_data[2];
                    }
                    else if (N == 3)
                    {
                        return m_data[0]*m_data[4]*m_data[8] + 
                               m_data[1]*m_data[5]*m_data[6] + 
                               m_data[2]*m_data[3]*m_data[7] -
                               m_data[2]*m_data[4]*m_data[6] - 
                               m_data[1]*m_data[3]*m_data[8] - 
                               m_data[0]*m_data[5]*m_data[7];
                    }
                    else
                    {
                        throw System::NotImplementedException(__FUNCTION__, __LINE__,
                            "Determinant computation implemented only for 2x2 and 3x3 matrices.");
                    }

                    return 0;
                }

                /** Set the matrix to identity. */
                SquareMatrix<N,T>& SetIdentity()
                {
                    System::Algo::Range::Fill(this->Begin(), this->End(), T(0));

                    T *dst = this->Begin();
                    for (Size i = 0; i < N; i++, dst += N + 1)
                    {
                        *dst = T(1);
                    }

                    return *this;
                }

                /** Returns identity matrix. */
                static SquareMatrix<N,T> Identity()
                {
                    SquareMatrix<N,T> m;
                    m.SetIdentity();
                    return m;
                }
            };
        }
    }
}

#endif
