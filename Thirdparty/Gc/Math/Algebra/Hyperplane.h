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
    Linear algebra general N-dimensional plane class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_MATH_ALGEBRA_HYPERPLANE_H
#define GC_MATH_ALGEBRA_HYPERPLANE_H

#include "Vector.h"

namespace Gc {
namespace Math {
namespace Algebra {
/** Class implementing general N-dimensional plane from linear algebra.
            
                @tparam N Dimensionality. For \c N=2 this class represents a line. For 
                    \c N=3 it represents a plane and so on. It holds that the represented
                    object divides the space into two halves.
                @tparam T %Data type used to store the coefficients.
            */
template <Size N, class T>
class Hyperplane
{
  protected:
    /** Normal vector. */
    Vector<N, T> m_normal;
    /** Distance from the origin. */
    T m_d;

  public:
    /** Constructor. */
    Hyperplane() = default;

    /** Constructor. */
    Hyperplane(const Vector<N, T> & normal, const Vector<N, T> & point)
    {
        Set(normal, point);
    }

    /** Copy constructor */
    Hyperplane(const Hyperplane<N, T> & p)
    {
        *this = p;
    }

    /** Specify hyperplane by specyfing its normal vector and point lying on it. */
    void Set(const Vector<N, T> & normal, const Vector<N, T> & point)
    {
        m_normal = normal.Normalized();
        m_d = m_normal.DotProduct(point);
    }

    /** Get normal vector of the hyperplane. */
    const Vector<N, T> & Normal() const
    {
        return m_normal;
    }

    /** Get distance from the origin. */
    T Distance() const
    {
        return m_d;
    }

    /** Decide whether given point is above or below the hyperplane. */
    bool IsAbove(const Vector<N, T> & point) const
    {
        return m_normal.DotProduct(point) > m_d;
    }

    /** Assignment operator. */
    Hyperplane<N, T> & operator=(const Hyperplane<N, T> & p)
    {
        m_normal = p.m_normal;
        m_d = p.m_d;
        return *this;
    }
};
}
}
} // namespace Gc::Math::Algebra

#endif
