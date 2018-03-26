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
    Implementation of the Chan-Vese segmentation model using graph cuts.

    This file implements algorithms for minimization of the two-phase
    piecewise constant Mumford-Shah functional, also known as the
    Chan-Vese model.

    @see Gc::Algo::Segmentation::ChanVese.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_ALGO_SEGMENTATION_CHANVESE_H
#define GC_ALGO_SEGMENTATION_CHANVESE_H

#include "../../Core.h"
#include "../../Type.h"
#include "../../Energy/Neighbourhood.h"
#include "../../Data/Image.h"
#include "../../Flow/IMaxFlow.h"
#include "../../Flow/IGridMaxFlow.h"
#include "../../System/Time/StopWatch.h"
#include "Tools.h"

namespace Gc
{
    /** Graph cut based image processing algorithms. */
    namespace Algo
    {
        /** Image segmentation methods. */
        namespace Segmentation
        {
            /** This files implement graph cut based minimization of the two-phase piecewise
                constant Mumford-Shah model, also known as the Chan-Vese model for
                image segmentation.

                This is a regional based foreground/background image segmentation
                useful mainly for detection of objects whose boundaries are not 
                necessarily defined by a gradient. It models the segmentation as
                minimization of the following functional:
                \f[
                E_{CV}(C,c_1,c_2)= \lambda_{1} \int_{\Omega_1} (f(x)-c_1)^2\,dx + 
                    \lambda_{2} \int_{\Omega_2} (f(x)-c_2)^2\,dx + \mu |C|
                \f]
                where \f$ C \f$ is the contour separating the foreground and background
                regions, \f$ f(x) \f$ is the image, \f$ c_1 \f$ and \f$ c_2 \f$ is the 
                mean intensity value of background and foreground, respectively,
                and \f$ \lambda_1, \lambda_2 \mbox{ and } \mu \f$ are fixed parameters.

                Related papers:
                - D. Mumford and J. Shah. <em>Optimal approximations by piecewise 
                smooth functions and associated variational problems</em>. 
                Communications on Pure and Applied Mathematics, Vol. 42, 
                No. 5. (1989), pp. 577-685.
                - T. F. Chan and L. A. Vese. <em>Active contours without edges. 
                IEEE Transactions on Image Processing</em>, 10(2):266-277, 2001.
                - Y. Zeng, W. Chen, and Q. Peng. <em>Effciently solving the piecewise 
                constant mumford-shah model using graph cuts</em>. Technical report, 
                Dept. of Computer Science, Zhejiang University, P.R. China, 2006.
                - Our yet unpublished improvements

                This file offers the following routines:
                - Compute() - computes the Chan-Vese segmentation of given image
                - ComputeMasked() - computes the Chan-Vese segmentation of an image with a voxel mask
                - ComputeTwoStage() - two-stage Chan-Vese algorithm
                - ComputeTwoStageMasked() - two-stage Chan-Vese algorithm with an initial voxel mask

                @remark We always assume that the fixed parameters \f$ \mu, \lambda_1, \lambda_2 \f$ are 
                    normalized, i.e., divided by the value of \f$ \mu \f$ (so this parameter is not
                    an input for the computation routines because it is assumed to be always 1).
            */
            namespace ChanVese
            {
                //////////////////////////////////////////////////////////////////////////
                //
                // Chan-Vese segmentation routines
                //
                //////////////////////////////////////////////////////////////////////////

                /** Compute the Chan-Vese segmentation on a given image.                    

                    The graph construction is based on the following paper:
                    - Y. Zeng, W. Chen, and Q. Peng. <em>Effciently solving the piecewise 
                    constant mumford-shah model using graph cuts</em>. Technical report, 
                    Dept. of Computer Science, Zhejiang University, P.R. China, 2006.

                    The algorithm performs following steps:
                    -# Create a graph using current values of \c c1 and \c c2.
                    -# Find minimal cut.
                    -# Update the mean intensity values \c c1 and \c c2 according to 
                        the new segmentation (mean values of foreground and background)
                    -# If \f$ |\Delta c1| + |\Delta c2| > \mbox{conv\_crit} \f$ go to 1.
            
                    To obtain initial guess of \c c1 and \c c2 values we recommend to
                    use the Clustering::KMeans::Lloyd() function. The minimization 
                    may fail and throw Gc::Math::ConvergenceException.

                    @param[in] im Input image. It should be normalized using NormalizeImage(). It is
                        also better to regularize it using Gaussian, Median or similar filter.
                        Voxel spacing is taken into account.
                    @param[in] l1 Weight \f$ \lambda_1 \f$.
                    @param[in] l2 Weight \f$ \lambda_2 \f$.
                    @param[in,out] c1 Initial and final value of \c c1.
                    @param[in,out] c2 Initial and final value of \c c2.
                    @param[in] conv_crit Convergence criterion. See above. Value 
                        around \c 0.001 recommended. Set to \c 0 to enforce convergence to
                        a steady state.
                    @param[in,out] max_iter Input: Maximum number of iterations. Output: Number of 
                        iterations performed.
                    @param[in] nb Neighbourhood to use. Greater neighbourhoods mean better
                        and smoother boundaries but also a longer computation and higher memory
                        consumption. The neighbourhood must be symmetrical.
                    @param[in] mf Maximum flow algorithm to use. Flow::Grid::Kohli recommended.
                    @param[out] seg Final binary mask of the segmentation.

                    @return %Energy of the final segmentation.

                    @see Clustering::KMeans::Lloyd(), Tools::NormalizeImage().
                */
                template <Size N, class T>
                T GC_DLL_EXPORT Compute(const Data::Image<N,T,T> &im, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IMaxFlow<T,T,T> &mf, System::Collection::Array<N,bool> &seg);

                template <Size N, class T>
                T GC_DLL_EXPORT Compute(const Data::Image<N,T,T> &im, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IGridMaxFlow<N,T,T,T> &mf, System::Collection::Array<N,bool> &seg);

                //////////////////////////////////////////////////////////////////////////
                //
                // Masked Chan-Vese segmentation routines
                //
                //////////////////////////////////////////////////////////////////////////

                /** Compute Chan-Vese segmentation of an image with a voxel mask.
                
                    This function computes the same thing as Compute() does. However, it
                    is possible to mark selected voxels as being known to belong to
                    foreground or background and to exclude some voxels from the computation
                    completely. This is done by supplying additional mask where
                    a flag is specified for each voxel.

                    @return %Energy of the final segmentation.

                    @see Compute, Segmentation::MaskFlag.
                */
                template <Size N, class T>
                T GC_DLL_EXPORT ComputeMasked(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IMaxFlow<T,T,T> &mf, System::Collection::Array<N,bool> &seg);

                template <Size N, class T>
                T GC_DLL_EXPORT ComputeMasked(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IGridMaxFlow<N,T,T,T> &mf, System::Collection::Array<N,bool> &seg);

                //////////////////////////////////////////////////////////////////////////
                //
                // Two-stage Chan-Vese segmentation routines
                //
                //////////////////////////////////////////////////////////////////////////

                /** Compute two-stage Chan-Vese segmentation of a given image.

                    In the first stage segmentation is computed using (smaller)
                    neighbourhood \c nb1 and then in the second stage it is smoothed
                    by calculating the same segmentation using a larger neighbourhood \c nb2
                    but only in a narrow band around the boundary obtained in the first stage.
                    This approach speeds up the computation and decreseases the memory
                    consumption rapidly.
                
                    To obtain initial guess of \c c1 and \c c2 values we recommend to
                    use the Clustering::KMeans::Lloyd() function. The minimization 
                    may fail and throw Gc::Math::ConvergenceException.

                    @param[in] im Input image. It should be normalized using Tools::NormalizeImage(). 
                        It is also better to regularize it using Gaussian, Median or similar filter.
                        Voxel spacing is taken into account.
                    @param[in] l1 Weight \f$ \lambda_1 \f$.
                    @param[in] l2 Weight \f$ \lambda_2 \f$.
                    @param[in,out] c1 Initial and final value of \c c1.
                    @param[in,out] c2 Initial and final value of \c c2.
                    @param[in] conv_crit1 Convergence criterion for the first stage. See Compute().
                    @param[in] conv_crit2 Convergence criterion for the second stage. See Compute().
                    @param[in,out] max_iter1 Input: Maximum number of iterations in the first stage. 
                        Output: Number of iterations performed.
                    @param[in,out] max_iter2 Input: Maximum number of iterations in the second stage. 
                        Output: Number of iterations performed.
                    @param[in] nb1 Neighbourhood to use in the first stage. 
                    @param[in] nb2 Neighbourhood to use in the second stage. 
                    @param[in] band_size Band size for the second stage. City block 
                        metric is used.
                    @param[in] mf1 Maximum flow algorithm to use in the first 
                        stage. Flow::Grid::Kohli recommended.
                    @param[in] mf2 Maximum flow algorithm to use in the second 
                        stage. Flow::Grid::PushRelabel::HighestLevel recommended.
                    @param[out] seg Final binary mask of the segmentation.

                    @return %Energy of the final segmentation.

                    @see Compute(), Clustering::KMeans::Lloyd(), Tools::NormalizeImage().
                */
                template <Size N, class T, class MAXFLOW1, class MAXFLOW2>
                T ComputeTwoStage(const Data::Image<N,T,T> &im, T l1, T l2, 
                    T &c1, T &c2, T conv_crit1, T conv_crit2, 
                    Size &max_iter1, Size &max_iter2,
                    const Energy::Neighbourhood<N,Int32> &nb1, 
                    const Energy::Neighbourhood<N,Int32> &nb2,
                    Size band_size, MAXFLOW1 &mf1, MAXFLOW2 &mf2,
                    System::Collection::Array<N,bool> &seg)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

                    // First phase - coarse segmentation
                    Compute(im, l1, l2, c1, c2, conv_crit1, max_iter1, nb1, mf1, seg);
                    mf1.Dispose();
                    
                    // Create band mask
                    System::Collection::Array<N,Uint8> mask;
                    Tools::CreateBandMask(seg, band_size, mask);

                    // Second phase - refinement of the boundary in given band
                    return ComputeMasked(im, mask, l1, l2, c1, c2, conv_crit2, max_iter2, nb2, mf2, seg);
                }

                //////////////////////////////////////////////////////////////////////////
                //
                // Two-stage masked Chan-Vese segmentation routines
                //
                //////////////////////////////////////////////////////////////////////////

                /** Compute two-stage Chan-Vese segmentation of an image with a voxel mask.
                
                    This function computes the same thing as ComputeTwoStage() does. However, it
                    is possible to mark selected voxels as being known to belong to
                    foreground or background and to exclude some voxels from the computation
                    completely. This is done by supplying additional mask where
                    a flag is specified for each voxel.

                    @return %Energy of the final segmentation.

                    @see ComputeTwoStage, ComputeMasked, Segmentation::MaskFlag.
                */
                template <Size N, class T, class MAXFLOW1, class MAXFLOW2>
                T ComputeTwoStageMasked(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, T l1, T l2, 
                    T &c1, T &c2, T conv_crit1, T conv_crit2, 
                    Size &max_iter1, Size &max_iter2,
                    const Energy::Neighbourhood<N,Int32> &nb1, 
                    const Energy::Neighbourhood<N,Int32> &nb2,
                    Size band_size, MAXFLOW1 &mf1, 
                    MAXFLOW2 &mf2, System::Collection::Array<N,bool> &seg)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

                    // First phase - coarse segmentation
                    ComputeMasked(im, mask, l1, l2, c1, c2, conv_crit1, max_iter1, nb1, mf1, seg);
                    mf1.Dispose();
                    
                    // Create band mask
                    System::Collection::Array<N,Uint8> stage2_mask;
                    Tools::CreateBandMask(seg, mask, band_size, stage2_mask);

                    // Second phase - refinement of the boundary in given band mask
                    return ComputeMasked(im, stage2_mask, l1, l2, c1, c2, conv_crit2, max_iter2, nb2, mf2, seg);
                }
            }
        }
    }
}

#endif
