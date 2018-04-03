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

#ifndef GC_ALGO_SEGMENTATION_MUMFORDSHAH_H
#define GC_ALGO_SEGMENTATION_MUMFORDSHAH_H

#include "../../Core.h"
#include "../../Type.h"
#include "../../Energy/Neighbourhood.h"
#include "../../Data/Image.h"
#include "../../Flow/IGridMaxFlow.h"

namespace Gc
{
    namespace Algo
    {
        namespace Segmentation
        {
            /** Methods for minimization of the Mumford-Shah model for image segmentation.

                For an input image \f$ u_0 \f$ the Mumford-Shah model aims to find a pair
                \f$ (u,C) \f$ such that \f$ u \f$ is a piecewise smooth approximation of 
                \f$ u_0 \f$ and \f$ C \f$ is a smooth segmenting contour, minimizing
                the following energy functional:

                \f[
                E_{MS}(C,u)= \mu |C| + \lambda \int_{\Omega} (u_0(x)-u(x))^2\,dx +
                    \int_{\Omega \setminus C}|\nabla u(x)|^2\,dx
                \f]
                
                References:
                - D. Mumford and J. Shah: <em>Optimal approximation by piecewise smooth
                functions and associated variational problems</em>, Communications on Pure
                and Applied Mathematics, vol. 42, no. 5, pp. 577-685, 1989.
            */
            namespace MumfordShah
            {
                /** Piecewise constant Mumford-Shah segmentation via graph cuts.

                    In this simplification of the Mumford-Shah model we assume that image
                    \c u consists of \c k piecewise constant regions (possibly disconnected).
                    Each of these regions is characterized by its mean intensity \f$ c_i \f$.
                    Thus, the energy being minimized has the following form:

                    \f[
                    E_{MS}(C,c_1,..,c_k)= \mu |C| + \sum_{i = 1}^k \lambda_{i} \int_{\Omega_i} (f(x)-c_i)^2\,dx
                    \f]

                    The minimization is based on the \f$\alpha\f$-expansion algorithm.
                    For 2 labels this model corresponds to the so-called Chan-Vese model, so
                    the same results as when using ChanVese::Compute() will be obtained.
                    However, for better performance it is recommended to use the latter method
                    in this case.

                    The algorithm performs following steps:
                    -# Minimize \f$ E_{MS} \f$ for fixed \f$ c_i \f$ using \f$\alpha\f$-expansion.
                    -# Update the mean intensity values \f$ c_i \f$ according to 
                        the new segmentation.
                    -# If \f$ \sum_{i = 1}^k |\Delta c_i| > \mbox{conv\_crit} \f$ go to 1.
            
                    To obtain initial guess of \f$ c_i \f$ we recommend to
                    use the Clustering::KMeans::Lloyd() function. The minimization 
                    may fail and throw Gc::Math::ConvergenceException.

                    @param[in] im Input image. It should be normalized using NormalizeImage(). It is
                        also better to regularize it using Gaussian, Median or similar filter.
                        Voxel spacing is taken into account.
                    @param[in] k The number of partitions/possible labels.
                    @param[in] lambda Weights \f$ \lambda_i \f$ of the model.
                    @param[in,out] c Initial and final value of \f$ c_i \f$.
                    @param[in] conv_crit Convergence criterion. See above. Value 
                        around \c 0.001 recommended. Set to \c 0 to enforce convergence to
                        a steady state.
                    @param[in,out] max_iter Input: Maximum number of iterations. Output: Number of 
                        iterations performed.
                    @param[in] nb Neighbourhood to use. Greater neighbourhoods mean better
                        and smoother boundaries but also a longer computation and higher memory
                        consumption. The neighbourhood must be symmetrical.
                    @param[in] mf Maximum flow algorithm to use. Flow::Grid::Kohli or
                        Flow::Grid::PushRelabel::HighestLevel recommended.
                    @param[out] seg Final labeling of the image.

                    @return %Energy of the final labeling.
                
                    @see Energy::Min::Grid::AlphaExpansion(), ChanVese, 
                        Tools::NormalizeImage(), Clustering::KMeans::Lloyd().
                */
                template <Size N, class T, class L>
                T GC_DLL_EXPORT ComputePiecewiseConstant(const Data::Image<N,T,T> &im, Size k,
                    const System::Collection::Array<1,T> &lambda, System::Collection::Array<1,T> &c,
                    T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IGridMaxFlow<N,T,T,T> &mf, System::Collection::Array<N,L> &seg);
            }
        }
    }
}

#endif
