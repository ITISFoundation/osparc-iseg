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
    Implementation of the Rousson-Deriche segmentation model using graph cuts.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_ALGO_SEGMENTATION_ROUSSONDERICHE_H
#define GC_ALGO_SEGMENTATION_ROUSSONDERICHE_H

#include "../../Core.h"
#include "../../Type.h"
#include "../../Energy/Neighbourhood.h"
#include "../../Data/Image.h"
#include "../../Flow/IGridMaxFlow.h"
#include "../../System/Time/StopWatch.h"
#include "Tools.h"

namespace Gc {
namespace Algo {
namespace Segmentation {
/** This files implement graph cut based minimization of the Rousson-Deriche
                bayesian model for image segmentation.

                This is a regional based foreground/background image segmentation
                useful mainly for detection of objects whose boundaries are not 
                necessarily defined by a gradient. It models the segmentation as
                minimization of the following functional:
                \f[
                E_{RD}(C,c_1,\sigma^2_1,c_2,\sigma^2_2,)= \lambda \int_{\Omega_1} e_1(x)\,dx + 
                    \lambda \int_{\Omega_2} e_2(x)\,dx + |C|
                \f]
                where \f$ C \f$ is the contour separating the foreground and background
                regions, \f$ f(x) \f$ is the input image, \f$ \lambda \f$ is a fixed 
                parameter, \f$ c_1 \f$ and \f$ c_2 \f$ are mean intensity values of
                foreground and background, \f$ \sigma^2_1 \f$ and \f$ \sigma^2_2 \f$
                is the intensity variance of foreground and background and 
                \f$ e_k \f$ is defined as:
                \f[
                    e_k = \frac{(f(x) - c_k)^2}{\sigma^2_k} + log \, \sigma^2_k
                \f]

                Related papers:
                - M. Rousson and R. Deriche. <em>A variational framework for active and adaptative 
                segmentation of vector valued images</em>. Proceedings of IEEE Workshop on Motion 
                and Video Computing, 2002, pp. 56-61

                This file offers the following routines:
                - Compute() - computes the Rousson-Deriche segmentation of a given image
                - ComputeMasked() - computes the Rousson-Deriche segmentation of an image with a voxel mask
                - ComputeTwoStage() - two-stage Rousson-Deriche algorithm
                - ComputeTwoStageMasked() - two-stage Rousson-Deriche algorithm with an initial voxel mask
            */
namespace RoussonDeriche {
/** Unknowns of the Rousson-Deriche segmentation model. */
template <class T>
class Params
{
  public:
    /** Mean intensity of the background. */
    T m_c1;
    /** Mean intensity of the foreground. */
    T m_c2;
    /** Background intensity variance. */
    T m_v1;
    /** Foreground intensity variance. */
    T m_v2;
};

//////////////////////////////////////////////////////////////////////////
//
// Initialization method
//
//////////////////////////////////////////////////////////////////////////

/** Obtain an initial estimate of the unknowns in the Rousson-Deriche segmentation
                    model.

                    This method estimates initial mean intensity and varance of both
                    foreground and background. It is based on modified KMean clustering
                    algorithm.

                    @param[in] im Input image.
                    @param[in] max_iter Maximum number of iterations in the initialization.
                    @param[out] pm Initial estimate of the unknowns.

                    @return Number of iterations performed.
                */
template <Size N, class T>
Size InitialEstimate(const System::Collection::Array<N, T> & im, Size max_iter,
                     Params<T> & pm);

//////////////////////////////////////////////////////////////////////////
//
// Rousson-Deriche segmentation routines
//
//////////////////////////////////////////////////////////////////////////

/** Compute the Rousson-Deriche segmentation on a given image.

                    The algorithm performs following steps:
                    -# Create a graph using current values of \c c1, \c c2, \c v1 and \c v2.
                    -# Find a minimum cut.
                    -# Update the unknowns \c c1, \c c2, \c v1 and \c v2 according to the
                        obtained segmentation.
                    -# If \f$ |\Delta c1| + |\Delta c2| + |\Delta v1| + |\Delta v2| > \mbox{conv\_crit} \f$ 
                        go to 1.
            
                    To obtain initial guess of the unknowns \c pm use the function InitialEstimate(). 
                    The minimization may fail and throw Gc::Math::ConvergenceException.

                    @param[in] im Input image. It should be normalized using Tools::NormalizeImage(). It is
                        also better to regularize it using Gaussian, Median or similar filter.
                        Voxel spacing is taken into account.
                    @param[in] lambda %Data term weight \f$ \lambda \f$.
                    @param[in,out] pm Initial and final values of the unknowns in the Rousson-Deriche
                        model (i.e., the mean values and variances).
                    @param[in] conv_crit Convergence criterion. See above. Value 
                        around \c 0.0001 recommended. Set to \c 0 to enforce convergence to
                        a steady state.
                    @param[in,out] max_iter Input: Maximum number of iterations. Output: Number of 
                        iterations performed.
                    @param[in] nb Neighbourhood to use. Greater neighbourhoods mean better
                        and smoother boundaries but also a longer computation and higher memory
                        consumption. The neighbourhood must be symmetrical.
                    @param[in] mf Maximum flow algorithm to use. Flow::Grid::Kohli recommended.
                    @param[out] seg Final binary mask of the segmentation.

                    @return %Energy of the final segmentation.

                    @see InitialEstimate(), Tools::NormalizeImage(), Params.
                */
template <Size N, class T>
T GC_DLL_EXPORT Compute(const Data::Image<N, T, T> & im, T lambda, Params<T> & pm,
                        T conv_crit, Size & max_iter, const Energy::Neighbourhood<N, Int32> & nb,
                        Flow::IGridMaxFlow<N, T, T, T> & mf, System::Collection::Array<N, bool> & seg);

//////////////////////////////////////////////////////////////////////////
//
// Masked Rousson-Deriche segmentation routines
//
//////////////////////////////////////////////////////////////////////////

/** Compute Rousson-Deriche segmentation of an image with a voxel mask.
                
                    This function computes the same thing as Compute() does. However, it
                    is possible to mark selected voxels as being known to belong to
                    foreground or background and to exclude some voxels from the computation
                    completely. This is done by supplying additional mask where
                    a flag is specified for each voxel.

                    @return %Energy of the final segmentation.

                    @see Compute(), Segmentation::MaskFlag.
                */
template <Size N, class T>
T GC_DLL_EXPORT ComputeMasked(const Data::Image<N, T, T> & im,
                              const System::Collection::Array<N, Uint8> & mask, T lambda, Params<T> & pm,
                              T conv_crit, Size & max_iter, const Energy::Neighbourhood<N, Int32> & nb,
                              Flow::IGridMaxFlow<N, T, T, T> & mf, System::Collection::Array<N, bool> & seg);

//////////////////////////////////////////////////////////////////////////
//
// Two-stage Rousson-Deriche segmentation routines
//
//////////////////////////////////////////////////////////////////////////

/** Compute two-stage Rousson-Deriche segmentation of a given image.

                    In the first stage segmentation is computed using (smaller)
                    neighbourhood \c nb1 and then in the second stage it is smoothed
                    by calculating the same segmentation using a larger neighbourhood \c nb2
                    but only in a narrow band around the boundary obtained in the first stage.
                    This approach speeds up the computation and decreseases the memory
                    consumption rapidly.
                
                    To obtain initial guess of the unknowns \c pm use the function InitialEstimate(). 
                    The minimization may fail and throw Gc::Math::ConvergenceException.

                    @param[in] im Input image. It should be normalized using Tools::NormalizeImage(). 
                        It is also better to regularize it using Gaussian, Median or similar filter.
                        Voxel spacing is taken into account.
                    @param[in] lambda %Data term weight \f$ \lambda \f$.
                    @param[in,out] pm Initial and final values of the unknowns in the Rousson-Deriche
                        model (i.e., the mean values and variances).
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

                    @see Compute(), InitialEstimate(), Tools::NormalizeImage().
                */
template <Size N, class T>
T ComputeTwoStage(const Data::Image<N, T, T> & im, T lambda,
                  Params<T> & pm, T conv_crit1, T conv_crit2,
                  Size & max_iter1, Size & max_iter2,
                  const Energy::Neighbourhood<N, Int32> & nb1,
                  const Energy::Neighbourhood<N, Int32> & nb2,
                  Size band_size, Flow::IGridMaxFlow<N, T, T, T> & mf1,
                  Flow::IGridMaxFlow<N, T, T, T> & mf2,
                  System::Collection::Array<N, bool> & seg)
{
    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

    // First phase - coarse segmentation
    Compute(im, lambda, pm, conv_crit1, max_iter1, nb1, mf1, seg);
    mf1.Dispose();

    // Create band mask
    System::Collection::Array<N, Uint8> mask;
    Tools::CreateBandMask(seg, band_size, mask);

    // Second phase - refinement of the boundary in given band
    return ComputeMasked(im, mask, lambda, pm, conv_crit2, max_iter2, nb2, mf2, seg);
}

//////////////////////////////////////////////////////////////////////////
//
// Two-stage masked Rousson-Deriche segmentation routines
//
//////////////////////////////////////////////////////////////////////////

/** Compute two-stage RoussonDeriche segmentation of an image with a voxel mask.
                
                    This function computes the same thing as ComputeTwoStage() does. However, it
                    is possible to mark selected voxels as being known to belong to
                    foreground or background and to exclude some voxels from the computation
                    completely. This is done by supplying additional mask where
                    a flag is specified for each voxel.

                    @return %Energy of the final segmentation.

                    @see ComputeTwoStage(), ComputeMasked(), Segmentation::MaskFlag.
                */
template <Size N, class T>
T ComputeTwoStageMasked(const Data::Image<N, T, T> & im,
                        const System::Collection::Array<N, Uint8> & mask, T lambda,
                        Params<T> & pm, T conv_crit1, T conv_crit2,
                        Size & max_iter1, Size & max_iter2,
                        const Energy::Neighbourhood<N, Int32> & nb1,
                        const Energy::Neighbourhood<N, Int32> & nb2,
                        Size band_size, Flow::IGridMaxFlow<N, T, T, T> & mf1,
                        Flow::IGridMaxFlow<N, T, T, T> & mf2,
                        System::Collection::Array<N, bool> & seg)
{
    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

    // First phase - coarse segmentation
    ComputeMasked(im, mask, lambda, pm, conv_crit1, max_iter1, nb1, mf1, seg);
    mf1.Dispose();

    // Create band mask
    System::Collection::Array<N, Uint8> stage2_mask;
    Tools::CreateBandMask(seg, mask, band_size, stage2_mask);

    // Second phase - refinement of the boundary in given band mask
    return ComputeMasked(im, stage2_mask, lambda, pm, conv_crit2, max_iter2, nb2, mf2, seg);
}
} // namespace RoussonDeriche
}
}
} // namespace Gc::Algo::Segmentation

#endif
