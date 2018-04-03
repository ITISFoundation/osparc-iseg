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

#ifndef GC_MATH_ALGEBRA_EIGENVECTORS_H
#define GC_MATH_ALGEBRA_EIGENVECTORS_H

#include "../../Core.h"
#include "Matrix.h"
#include "Vector.h"

namespace Gc
{
    namespace Math
    {
        namespace Algebra
        {
            /** %Eigenvectors and eigenvalues computation routines. */
            namespace Eigenvectors
            {
                /** Find eigenvectors and eigenvalues of a symmetric matrix.

                    @param[in] mat Input matrix.
                    @param[out] eig_vec Columns of this matrix are the eigenvectors.
                    @param[out] eig_val Vector of eigenvalues.

                    @remarks Based on the source from http://barnesc.blogspot.com/2007/02/eigenvectors-of-3x3-symmetric-matrix.html.
                */
                template <Size N, class T>
                void GC_DLL_EXPORT DecomposeSymmetric(const Matrix<N,N,T> &mat, Matrix<N,N,T> &eig_vec, Vector<N,T> &eig_val);
            }
        }
    }
}

#endif
