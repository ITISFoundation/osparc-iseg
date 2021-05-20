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
    Pairwise clique potentials approximating general Riemannian metric
    (O. Danek approximation).

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include "../../../Math/Constant.h"
#include "../../../Math/Geometry/Voronoi.h"
#include "../../../System/Algo/Sort/Heap.h"
#include "RiemannianDanek.h"

namespace Gc {
namespace Energy {
namespace Potential {
namespace Metric {
/******************************************************************************/

// Auxiliary method - right hand side of the cauchy-crofton formulas
/** @cond */
template <Size N, class T>
static inline T CauchyCroftonCoef();

template <>
inline Float32 CauchyCroftonCoef<2, Float32>()
{
    return 2.0f;
}

template <>
inline Float64 CauchyCroftonCoef<2, Float64>()
{
    return 2.0;
}

template <>
inline Float32 CauchyCroftonCoef<3, Float32>()
{
    return float(Math::Constant::Pi);
}

template <>
inline Float64 CauchyCroftonCoef<3, Float64>()
{
    return Math::Constant::Pi;
}
/** @endcond */

/******************************************************************************/

template <Size N, class T>
RiemannianDanek<N, T> & RiemannianDanek<N, T>::SetTransformationMatrix(const Math::Algebra::SquareMatrix<N, T> & mt)
{
    // Calculate delta phi of the transformed neighbourhood
    System::Collection::Array<1, Math::Algebra::Vector<N, T>> nv(m_nb.Elements());

    for (Size i = 0; i < m_nb.Elements(); i++)
    {
        nv[i] = mt.Mul(m_nb[i]).Normalized();
    }

    System::Collection::Array<1, T> dphi;
    Math::Geometry::HypersphereVoronoiDiagram(nv, dphi);

    // Calculate delta rho and capacities
    T coef = CauchyCroftonCoef<N, T>();
    T gridCellArea = mt.Determinant();

    for (Size i = 0; i < m_nb.Elements(); i++)
    {
        T drho = gridCellArea / mt.Mul(m_nb[i]).Length();
        m_rw[i] = (dphi[i] * drho) / coef;
    }

    return *this;
}

/******************************************************************************/

// Explicit instantiations
/** @cond */
template class GC_DLL_EXPORT RiemannianDanek<2, Float32>;
template class GC_DLL_EXPORT RiemannianDanek<2, Float64>;
template class GC_DLL_EXPORT RiemannianDanek<3, Float32>;
template class GC_DLL_EXPORT RiemannianDanek<3, Float64>;
/** @endcond */

/******************************************************************************/

template <class T, class SZ>
struct OrientationPred
{
    bool operator()(const System::Collection::Pair<Math::Algebra::Vector<2, T>, SZ> & v1,
                    const System::Collection::Pair<Math::Algebra::Vector<2, T>, SZ> & v2) const
    {
        return (Math::Algebra::AngularOrientation(v1.m_first) <
                Math::Algebra::AngularOrientation(v2.m_first));
    }
};

/******************************************************************************/

template <class T>
RiemannianDanek2D<T>::RiemannianDanek2D(const Neighbourhood<2, T> & n)
{
    // Init members
    m_rw.Resize(n.Elements());

    // Sort neighbourhood according to the angular orientation
    // but remember indexes to the original array
    m_nb.Resize(n.Elements());

    for (Size i = 0; i < n.Elements(); i++)
    {
        m_nb[i].m_first = n[i];
        m_nb[i].m_second = (Uint8)i;
    }

    // Sort angular orientations and the indexes
    System::Algo::Sort::Heap(m_nb.Begin(), m_nb.End(), OrientationPred<T, Uint8>());
}

/******************************************************************************/

template <class T>
RiemannianDanek2D<T> & RiemannianDanek2D<T>::SetTransformationMatrix(const Math::Algebra::SquareMatrix<2, T> & mt)
{
    Math::Algebra::Vector<2, T> vecPrev, vecCur, vecNext;
    T gridCellArea = mt.Determinant();

    // Working set of transformed vectors
    vecPrev = mt.Mul(m_nb[m_nb.Elements() - 1].m_first);
    vecCur = mt.Mul(m_nb[0].m_first);

    for (Size i = 0; i < m_nb.Elements(); i++)
    {
        // Following edge
        Size nextIdx = (i == m_nb.Elements() - 1) ? 0 : (i + 1);
        vecNext = mt.Mul(m_nb[nextIdx].m_first);

        // Delta phi
        T dphi = Math::Algebra::ClockwiseAngle(vecNext, vecPrev) / T(2);

        // Delta rho
        T drho = gridCellArea / vecCur.Length();

        // Edge weight
        m_rw[m_nb[i].m_second] = (dphi * drho) / T(2.0);

        // Shift working set
        vecPrev = vecCur;
        vecCur = vecNext;
    }

    return *this;
}

/******************************************************************************/

// Explicit instantiations
/** @cond */
template class GC_DLL_EXPORT RiemannianDanek2D<Float32>;
template class GC_DLL_EXPORT RiemannianDanek2D<Float64>;
/** @endcond */

/******************************************************************************/
}
}
}
} // namespace Gc::Energy::Potential::Metric
