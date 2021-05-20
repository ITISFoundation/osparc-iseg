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
    Implementation of the Mumford-Shah segmentation model using graph cuts.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#include "../../Math/Basic.h"
#include "../../Math/ConvergenceException.h"
#include "../../Math/Algebra/SquareMatrix.h"
#include "../../Energy/Potential/Metric/RiemannianDanek.h"
#include "../../Energy/IGridEnergy.h"
#include "../../Energy/Min/Grid/AlphaExpansion.h"
#include "../../System/Collection/EmptyArrayMask.h"
#include "../../System/ArgumentException.h"
#include "../../Math/ConvergenceException.h"
#include "../../System/Time/StopWatch.h"
#include "MumfordShah.h"

namespace Gc {
namespace Algo {
namespace Segmentation {
namespace MumfordShah {
/***********************************************************************************/

/** Piecewise-constant Mumford-Shah energy specification. */
template <Size N, class T, class L>
class PiecewiseConstantMsEnergy
    : public Energy::IGridEnergy<N, T, L>
{
  protected:
    /** Image data. */
    const Data::Image<N, T, T> & m_im;
    /** Neighbourhood system. */
    const Energy::Neighbourhood<N, Int32> & m_nb;
    /** Number of possible labels. */
    Size m_k;
    /** Mean intensity values for each partition. */
    const System::Collection::Array<1, T> & m_c;
    /** Weight for each partition. */
    const System::Collection::Array<1, T> & m_lambda;
    /** Euclidean metric approximation edge capacities. */
    Energy::Potential::Metric::RiemannianDanek<N, T> m_em;

  public:
    /** Constructor. */
    PiecewiseConstantMsEnergy(const Data::Image<N, T, T> & im, Size k,
                              const System::Collection::Array<1, T> & c,
                              const System::Collection::Array<1, T> & lambda,
                              const Energy::Neighbourhood<N, Int32> & nb)
        : m_im(im)
        , m_nb(nb)
        , m_k(k)
        , m_c(c)
        , m_lambda(lambda)
        , m_em(nb.template Convert<N, T>(0))
    {
        // Setup euclidean metric approximation for given grid spacing
        m_em.SetTransformationMatrix(Math::Algebra::SquareMatrix<N, T>(im.Spacing()));
    }

    /** Number of possible labels. */
    Size Labels() const override
    {
        return m_k;
    }

    /** Grid dimensions. */
    Math::Algebra::Vector<N, Size> Dimensions() const override
    {
        return m_im.Dimensions();
    }

    /** Get grid neighbourhood system. */
    const Energy::Neighbourhood<N, Int32> & NbSystem() const override
    {
        return m_nb;
    }

    /** Unary clique potentials, i.e., label penalties (data costs). */
    T CliquePotential(Size node, L label) const override
    {
        return m_lambda[label] * Math::Sqr(m_im[node] - m_c[label]);
    }

    /** Pairwise clique potentials, i.e., smoothness costs. */
    T CliquePotential(Size n1, Size n2, Size i, L l1, L l2) const override
    {
        return (l1 != l2) ? m_em[i] : T(0);
    }
};

/***********************************************************************************/

template <Size N, class T, class L>
T GC_DLL_EXPORT ComputePiecewiseConstant(const Data::Image<N, T, T> & im, Size k,
                                         const System::Collection::Array<1, T> & lambda, System::Collection::Array<1, T> & c,
                                         T conv_crit, Size & max_iter, const Energy::Neighbourhood<N, Int32> & nb,
                                         Flow::IGridMaxFlow<N, T, T, T> & mf, System::Collection::Array<N, L> & seg)
{
    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

    if (k < 2)
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__, "At least 2 labels required.");
    }

    if (lambda.Elements() != k)
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__, "Invalid number of weights.");
    }

    if (c.Elements() != k)
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__, "Invalid number of initial means.");
    }

    // Sort neighbourhood - required by alpha expansion method
    Energy::Neighbourhood<N, Int32> snb(nb);
    System::Algo::Sort::Heap(snb.Begin(), snb.End());

    // Setup energy
    PiecewiseConstantMsEnergy<N, T, L> ms(im, k, c, lambda, snb);

    // Minimize
    Size iter = 0;
    T energy = 0;

    System::Collection::Array<1, L> lab(im.Elements());
    System::Collection::Array<1, T> oc(k);
    System::Collection::Array<1, Size> cn(k);
    System::Collection::EmptyArrayMask<N> mask(im.Dimensions());

    while (iter < max_iter)
    {
        System::Time::StopWatch swi(__FUNCTION__, __LINE__, "iteration");
        iter++;

        // Minimize MS with respect to fixed mean values
        Size a_max_iter = max_iter;
        energy = Energy::Min::Grid::AlphaExpansion(ms, mask, mf, a_max_iter, lab);

        // Update means
        oc = c;
        System::Algo::Range::Fill(c.Begin(), c.End(), T(0));
        System::Algo::Range::Fill(cn.Begin(), cn.End(), Size(0));

        for (Size i = 0; i < im.Elements(); i++)
        {
            c[lab[i]] += im[i];
            cn[lab[i]]++;
        }

        T cdelta = 0;
        for (Size i = 0; i < k; i++)
        {
            if (!cn[i])
            {
                throw Math::ConvergenceException(__FUNCTION__, __LINE__, "Empty partition.");
            }

            c[i] /= T(cn[i]);
            cdelta += Math::Abs(c[i] - oc[i]);
        }

        if (cdelta <= conv_crit)
        {
            break;
        }
    }

    // Create labeling
    seg.Resize(im.Dimensions());
    for (Size i = 0; i < im.Elements(); i++)
    {
        seg[i] = lab[i];
    }

    max_iter = iter;

    return energy;
}

/***********************************************************************************/

// Explicit instantiations
/** @cond */
template Float32 GC_DLL_EXPORT ComputePiecewiseConstant(const Data::Image<2, Float32, Float32> & im,
                                                        Size k, const System::Collection::Array<1, Float32> & lambda, System::Collection::Array<1, Float32> & c,
                                                        Float32 conv_crit, Size & max_iter, const Energy::Neighbourhood<2, Int32> & nb,
                                                        Flow::IGridMaxFlow<2, Float32, Float32, Float32> & mf, System::Collection::Array<2, Uint8> & seg);
template Float64 GC_DLL_EXPORT ComputePiecewiseConstant(const Data::Image<2, Float64, Float64> & im,
                                                        Size k, const System::Collection::Array<1, Float64> & lambda, System::Collection::Array<1, Float64> & c,
                                                        Float64 conv_crit, Size & max_iter, const Energy::Neighbourhood<2, Int32> & nb,
                                                        Flow::IGridMaxFlow<2, Float64, Float64, Float64> & mf, System::Collection::Array<2, Uint8> & seg);
template Float32 GC_DLL_EXPORT ComputePiecewiseConstant(const Data::Image<3, Float32, Float32> & im,
                                                        Size k, const System::Collection::Array<1, Float32> & lambda, System::Collection::Array<1, Float32> & c,
                                                        Float32 conv_crit, Size & max_iter, const Energy::Neighbourhood<3, Int32> & nb,
                                                        Flow::IGridMaxFlow<3, Float32, Float32, Float32> & mf, System::Collection::Array<3, Uint8> & seg);
template Float64 GC_DLL_EXPORT ComputePiecewiseConstant(const Data::Image<3, Float64, Float64> & im,
                                                        Size k, const System::Collection::Array<1, Float64> & lambda, System::Collection::Array<1, Float64> & c,
                                                        Float64 conv_crit, Size & max_iter, const Energy::Neighbourhood<3, Int32> & nb,
                                                        Flow::IGridMaxFlow<3, Float64, Float64, Float64> & mf, System::Collection::Array<3, Uint8> & seg);
/** @endcond */

/***********************************************************************************/
}
}
}
} // namespace Gc::Algo::Segmentation::MumfordShah
