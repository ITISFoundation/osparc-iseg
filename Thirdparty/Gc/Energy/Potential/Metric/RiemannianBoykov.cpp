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
    Pairwise clique potentials approximating a general Riemannian metric
    (Y. Boykov approximation).

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include "RiemannianDanek.h"
#include "RiemannianBoykov.h"

namespace Gc {
namespace Energy {
namespace Potential {
namespace Metric {
/******************************************************************************/

// Auxiliary method - exponent of the denominator (u^T * D * u) in the formulas of
// riemannian metric edge weights.
/** @cond */
template <Size N, class T>
static inline T RiemannCoef(const T & val);

template <>
inline Float32 RiemannCoef<2, Float32>(const Float32 & val)
{
    return Math::Pow(val, 1.5f);
}

template <>
inline Float64 RiemannCoef<2, Float64>(const Float64 & val)
{
    return Math::Pow(val, 1.5);
}

template <>
inline Float32 RiemannCoef<3, Float32>(const Float32 & val)
{
    return val * val;
}

template <>
inline Float64 RiemannCoef<3, Float64>(const Float64 & val)
{
    return val * val;
}
/** @endcond */

/******************************************************************************/

template <Size N, class T>
RiemannianBoykov<N, T>::RiemannianBoykov(const Neighbourhood<N, T> & n)
{
    // Resize arrays
    m_rew.Resize(n.Elements());
    m_nno.Resize(n.Elements());

    // Calculate edge weights for standard Euclidean metric on isotropic grid
    RiemannianDanek<N, T> rs(n);
    rs.SetTransformationMatrix(Math::Algebra::SquareMatrix<N, T>::Identity());

    m_em = rs.EdgeWeights();

    // Calculate normalized neighbour offset
    for (Size i = 0; i < n.Elements(); i++)
    {
        m_nno[i] = n[i].Normalized();
    }
}

/******************************************************************************/

template <Size N, class T>
RiemannianBoykov<N, T> & RiemannianBoykov<N, T>::SetTransformationMatrix(const Math::Algebra::SquareMatrix<N, T> & mt)
{
    // Determinant of the metric tensor is square of determinant of trans. matrix
    T nom = Math::Sqr(mt.Determinant());

    for (Size i = 0; i < m_nno.Elements(); i++)
    {
        Math::Algebra::Vector<N, T> v = mt.Mul(m_nno[i]);
        T den = v.DotProduct(v); // u^T * (D^T * D) * u = (D * u)^T * (D * u)
        m_rew[i] = (m_em[i] * nom) / RiemannCoef<N, T>(den);
    }

    return *this;
}

/******************************************************************************/

template <Size N, class T>
RiemannianBoykov<N, T> & RiemannianBoykov<N, T>::SetMetricTensor(const Math::Algebra::SquareMatrix<N, T> & mt)
{
    T nom = mt.Determinant();

    for (Size i = 0; i < m_nno.Elements(); i++)
    {
        T den = m_nno[i].InnerProduct(mt, m_nno[i]); // u^T * D * u
        m_rew[i] = (m_em[i] * nom) / RiemannCoef<N, T>(den);
    }

    return *this;
}

/******************************************************************************/

// Explicit instantiations
/** @cond */
template class GC_DLL_EXPORT RiemannianBoykov<2, Float32>;
template class GC_DLL_EXPORT RiemannianBoykov<2, Float64>;
template class GC_DLL_EXPORT RiemannianBoykov<3, Float32>;
template class GC_DLL_EXPORT RiemannianBoykov<3, Float64>;
/** @endcond */

/******************************************************************************/
}
}
}
} // namespace Gc::Energy::Potential::Metric
