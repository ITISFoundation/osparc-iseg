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

#include <math.h>
#include "../../System/ArgumentException.h"
#include "../../Math/Basic.h"
#include "../../Math/ConvergenceException.h"
#include "../../Math/Algebra/SquareMatrix.h"
#include "../../Energy/Potential/Metric/RiemannianDanek.h"
#include "../../Data/BorderIterator.h"
#include "Mask.h"
#include "RoussonDeriche.h"

namespace Gc
{
    namespace Algo
    {
        namespace Segmentation
        {
            namespace RoussonDeriche
            {
                //////////////////////////////////////////////////////////////////////////
                //
                // Initialization method
                //
                //////////////////////////////////////////////////////////////////////////

                /******************************************************************************/

                template <Size N, class T>
                Size InitialEstimate(const System::Collection::Array<N,T> &im, Size max_iter,
                    Params<T> &pm)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__);

                    if (im.IsEmpty())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "Empty array.");
                    }

                    // Compute mean intensity and min and max values
                    const T *pi = im.Begin();
                    T dmin = *pi, dmax = *pi, davg = 0;
                    
                    for (Size i = 0; i < im.Elements(); i++, pi++)
                    {
                        davg += *pi;

                        if (*pi > dmax)
                        {
                            dmax = *pi;
                        }
                        else if (*pi < dmin)
                        {
                            dmin = *pi;
                        }
                    }

                    // Initial c1 and c2 estimate
                    davg /= T(im.Elements());
                    pm.m_c1 = davg - (davg - dmin) / T(2);
                    pm.m_c2 = davg + (dmax - davg) / T(2);

                    // Initial variance estimate
                    pm.m_v1 = pm.m_v2 = T(0.1);

                    // Minimization
                    Size iter = 0;        

                    while (iter < max_iter)
                    {
                        iter++;

                        T vl1 = log(pm.m_v1);
                        T vl2 = log(pm.m_v2);

                        Params<T> opm = pm;

                        pm.m_c1 = pm.m_c2 = 0;
                        pm.m_v1 = pm.m_v2 = 0;
                        Size c1n = 0, c2n = 0;

                        pi = im.Begin();         

                        for (Size i = 0; i < im.Elements(); i++, pi++)
                        {
                            T dt1 = vl1 + (Math::Sqr(*pi - opm.m_c1) / opm.m_v1);
                            T dt2 = vl2 + (Math::Sqr(*pi - opm.m_c2) / opm.m_v2);

                            if (dt1 > dt2)
                            {
                                c2n++;
                                pm.m_c2 += *pi;
                                pm.m_v2 += Math::Sqr(*pi);
                            }
                            else
                            {
                                c1n++;
                                pm.m_c1 += *pi;
                                pm.m_v1 += Math::Sqr(*pi);
                            }
                        }

                        if (!c1n || !c2n)
                        {
                            throw Math::ConvergenceException(__FUNCTION__, __LINE__);
                        }

                        pm.m_c1 /= T(c1n);
                        pm.m_c2 /= T(c2n);
                        pm.m_v1 = (pm.m_v1 / T(c1n)) - Gc::Math::Sqr(pm.m_c1);
                        pm.m_v2 = (pm.m_v2 / T(c2n)) - Gc::Math::Sqr(pm.m_c2);

                        if (opm.m_c1 == pm.m_c1 && opm.m_c2 == pm.m_c2 && 
                            opm.m_v1 == pm.m_v1 && opm.m_v2 == pm.m_v2)
                        {
                            // Steady state reached
                            break;
                        }
                    }

                    return iter;
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template Size GC_DLL_EXPORT InitialEstimate
                    (const System::Collection::Array<2,Float32> &im, Size max_iter,
                    Params<Float32> &pm);
                template Size GC_DLL_EXPORT InitialEstimate
                    (const System::Collection::Array<2,Float64> &im, Size max_iter,
                    Params<Float64> &pm);
                template Size GC_DLL_EXPORT InitialEstimate
                    (const System::Collection::Array<3,Float32> &im, Size max_iter,
                    Params<Float32> &pm);
                template Size GC_DLL_EXPORT InitialEstimate
                    (const System::Collection::Array<3,Float64> &im, Size max_iter,
                    Params<Float64> &pm);
                /** @endcond */

                /******************************************************************************/

                //////////////////////////////////////////////////////////////////////////
                //
                // Rousson-Deriche segmentation routines
                //
                //////////////////////////////////////////////////////////////////////////

                /******************************************************************************/

                /** Using the current segmentation mask update model unknowns. */
                template <Size N, class T>
                static void UpdateParams(const System::Collection::Array<N,T> &im, 
                    const System::Collection::Array<N,bool> &seg, Params<T> &p)
                {
                    const T *di = im.Begin();
                    const bool *dm = seg.Begin();
                    Size n1 = 0, n2 = 0;

                    p.m_c1 = p.m_c2 = p.m_v1 = p.m_v2 = 0;

                    for (Size i = 0; i < im.Elements(); i++, di++, dm++)
                    {
                        if (*dm)
                        {
                            p.m_c2 += *di;
                            p.m_v2 += Gc::Math::Sqr(*di);
                            n2++;
                        }
                        else
                        {
                            p.m_c1 += *di;
                            p.m_v1 += Gc::Math::Sqr(*di);
                            n1++;
                        }
                    }

                    if (!n1 || !n2)
                    {
                        throw Math::ConvergenceException(__FUNCTION__, __LINE__,
                            "Whole image segmented as foreground/background.");
                    }

                    // Mean intensity
                    p.m_c1 /= T(n1); 
                    p.m_c2 /= T(n2);

                    // Intensity variance
                    p.m_v1 = (p.m_v1 / T(n1)) - Gc::Math::Sqr(p.m_c1);
                    p.m_v2 = (p.m_v2 / T(n2)) - Gc::Math::Sqr(p.m_c2);
                }

                /******************************************************************************/

                /** Creates graph and sets up N-link capacities for grid max-flow. */
                template <Size N, class T>
                static void SetGraphNLinks(const Data::Image<N,T,T> &im, 
                    const Energy::Neighbourhood<N,Int32> &nb, 
                    const System::Collection::Array<1,T> &cap,
                    Flow::IGridMaxFlow<N,T,T,T> &ff)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__);

                    // Init graph
                    ff.Init(im.Dimensions(), nb);

                    // Setup N-links
                    for (Size i = 0; i < im.Elements(); i++)
                    {
                        for (Size j = 0; j < nb.Elements(); j++)
                        {
                            ff.SetArcCap(i, j, cap[j]);
                        }
                    }
                }

                /******************************************************************************/

                /** Setup T-link capacities. */
                template <Size N, class T>
                static void SetGraphTLinks(const Data::Image<N,T,T> &im, 
                    T lambda, Params<T> &p, Flow::IGridMaxFlow<N,T,T,T> &mf)
                {                    
                    // Voxel size scaling
                    lambda *= im.VoxelSize();

                    // Variance logarithm
                    T vl1 = log(p.m_v1);
                    T vl2 = log(p.m_v2);

                    // Update T-links
                    const T *dp = im.Begin();
                    T cap1, cap2;

                    for (Size i = 0; i < im.Elements(); i++, dp++)
                    {
                        cap1 = vl1 + (Math::Sqr(*dp - p.m_c1) / p.m_v1);
                        cap2 = vl2 + (Math::Sqr(*dp - p.m_c2) / p.m_v2);
                        mf.SetTerminalArcCap(i, lambda * cap2, lambda * cap1);
                    }
                }

                /******************************************************************************/

                template <Size N, class T>
                T Compute(const Data::Image<N,T,T> &im, T lambda, Params<T> &pm, 
                    T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb,
                    Flow::IGridMaxFlow<N,T,T,T> &mf, System::Collection::Array<N,bool> &seg)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

                    // Sort neighbourhood for grid max-flow
                    Energy::Neighbourhood<N,Int32> snb(nb);
                    System::Algo::Sort::Heap(snb.Begin(), snb.End());

                    // Init segmentation mask
                    seg.Resize(im.Dimensions());

                    // Calculate Euclidean metric capacities
                    Energy::Potential::Metric::RiemannianDanek<N,T> em(snb.template Convert<N,T>(0));
                    em.SetTransformationMatrix(Math::Algebra::SquareMatrix<N,T>(im.Spacing()));

                    // Start segmentation
                    Size iter = 0;
                    T flow = 0;

                    while (iter < max_iter)
                    {
                        System::Time::StopWatch sw(__FUNCTION__, __LINE__, "iteration");

                        // Create graph
                        if (!iter || !mf.IsDynamic())
                        {
                            SetGraphNLinks(im, snb, em.EdgeWeights(), mf);
                        }
                        SetGraphTLinks(im, lambda, pm, mf);

                        // Find maximum flow
                        flow = mf.FindMaxFlow();

                        // Create the segmentation mask
                        bool *dm = seg.Begin();
                        for (Size i = 0; i < im.Elements(); i++, dm++)
                        {
                            *dm = (mf.NodeOrigin(i) != Flow::Source);
                        }

                        // Update unknowns
                        Params<T> opm = pm;
                        UpdateParams(im, seg, pm);

                        iter++;

                        if (Math::Abs(opm.m_c1 - pm.m_c1) + Math::Abs(opm.m_c2 - pm.m_c2) +
                            Math::Abs(opm.m_v1 - pm.m_v1) + Math::Abs(opm.m_v2 - pm.m_v2) <= conv_crit)
                        {
                            break;
                        }
                    }

                    max_iter = iter;

                    return flow;
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template GC_DLL_EXPORT Float32 Compute(const Data::Image<2,Float32,Float32> &im, 
                    Float32 lambda, Params<Float32> &pm, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IGridMaxFlow<2,Float32,Float32,Float32> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float64 Compute(const Data::Image<2,Float64,Float64> &im, 
                    Float64 lambda, Params<Float64> &pm, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IGridMaxFlow<2,Float64,Float64,Float64> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float32 Compute(const Data::Image<3,Float32,Float32> &im, 
                    Float32 lambda, Params<Float32> &pm, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IGridMaxFlow<3,Float32,Float32,Float32> &mf, 
                    System::Collection::Array<3,bool> &seg);
                template GC_DLL_EXPORT Float64 Compute(const Data::Image<3,Float64,Float64> &im, 
                    Float64 lambda, Params<Float64> &pm, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IGridMaxFlow<3,Float64,Float64,Float64> &mf, 
                    System::Collection::Array<3,bool> &seg);
                /** @endcond */

                /******************************************************************************/

                //////////////////////////////////////////////////////////////////////////
                //
                // Masked Rousson-Deriche segmentation routines
                //
                //////////////////////////////////////////////////////////////////////////

                /******************************************************************************/

                /** Using the current segmentation mask update model unknowns. */
                template <Size N, class T>
                static void UpdateParamsMask(const System::Collection::Array<N,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask,
                    const System::Collection::Array<N,bool> &seg, Params<T> &pm)
                {
                    const T *di = im.Begin();
                    const bool *ds = seg.Begin();
                    const Uint8 *dm = mask.Begin();
                    Size n1 = 0, n2 = 0;

                    pm.m_c1 = pm.m_c2 = pm.m_v1 = pm.m_v2 = 0;

                    for (Size i = 0; i < im.Elements(); i++, di++, dm++, ds++)
                    {
                        if (*dm != MaskIgnore)
                        {
                            if (*ds)
                            {
                                pm.m_c2 += *di;
                                pm.m_v2 += Gc::Math::Sqr(*di);
                                n2++;
                            }
                            else
                            {
                                pm.m_c1 += *di;
                                pm.m_v1 += Gc::Math::Sqr(*di);
                                n1++;
                            }
                        }
                    }

                    if (!n1 || !n2)
                    {
                        throw Math::ConvergenceException(__FUNCTION__, __LINE__,
                            "Whole image segmented as foreground/background.");
                    }

                    // Mean intensity
                    pm.m_c1 /= T(n1); 
                    pm.m_c2 /= T(n2);

                    // Intensity variance
                    pm.m_v1 = (pm.m_v1 / T(n1)) - Gc::Math::Sqr(pm.m_c1);
                    pm.m_v2 = (pm.m_v2 / T(n2)) - Gc::Math::Sqr(pm.m_c2);
                }

                /******************************************************************************/

                /** Create graph and setup N-link capacities for grid max-flow. */
                template <Size N, class T>
                static void SetGraphNLinksMask (const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, const Segmentation::Mask<N> &cmask,
                    const Energy::Neighbourhood<N,Int32> &nb, const System::Collection::Array<1,T> &cap, 
                    Flow::IGridMaxFlow<N,T,T,T> &ff, System::Collection::Array<1,T> &etl)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__);

                    // Init max-flow
                    ff.InitMask(im.Dimensions(), nb, cmask);

                    // Setup additional T-link capacities (they substitute N-links to constraints)
                    etl.Resize(cmask.UnmaskedElements(), 0);

                    // Get neighbour offsets
                    System::Collection::Array<1,Int32> ofs;
                    im.Indexes(nb, ofs);

                    // Get neighbourhood extent
                    Math::Algebra::Vector<N,Size> bleft, bright;
                    nb.Extent(bleft, bright);

                    // Create data iterator
                    Data::BorderIterator<N> iter(im.Dimensions(), bleft, bright);
                    iter.Start(false);

                    Math::Algebra::Vector<N,Size> ncur = iter.CurPos();
                    Size bsize;
                    bool border;

                    // Setup N-links
                    Size n = 0, bidx = 0;

                    while (!iter.IsFinished())
                    {
                        border = iter.NextBlock(bsize);

                        while (bsize-- > 0)
                        {
                            if (mask[n] == MaskUnknown)
                            {
                                for (Size i = 0; i < nb.Elements(); i++)
                                {
                                    if (!border || im.IsNeighbourInside(ncur, nb[i]))
                                    {
                                        Uint8 nm = mask[n + ofs[i]];

                                        if (nm == MaskUnknown)
                                        {
                                            ff.SetArcCap(bidx, i, cap[i]);
                                        }
                                        else if (nm == MaskSource)
                                        {
                                            etl[bidx] += cap[i];
                                        }
                                        else if (nm == MaskSink)
                                        {
                                            etl[bidx] -= cap[i];
                                        }
                                    }
                                }

                                bidx++;
                            }

                            iter.NextPos(ncur);
                            n++;
                        }
                    }
                }

                /******************************************************************************/

                /** Setup T-link capacities. */
                template <Size N, class T>
                static void SetGraphTLinksMask(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, 
                    T lambda, Params<T> &pm, Flow::IGridMaxFlow<N,T,T,T> &mf,
                    const System::Collection::Array<1,T> &etl)
                {                    
                    // Voxel size scaling
                    lambda *= im.VoxelSize();

                    // Variance logarithm
                    T vl1 = log(pm.m_v1);
                    T vl2 = log(pm.m_v2);

                    // Update T-links
                    const T *dp = im.Begin();
                    const T *etlp = etl.Begin();
                    const Uint8 *dm = mask.Begin();
                    Size j = 0;
                    T cap1, cap2;

                    for (Size i = 0; i < im.Elements(); i++, dp++, dm++)
                    {
                        if (*dm == MaskUnknown)
                        {
                            cap1 = vl1 + (Math::Sqr(*dp - pm.m_c1) / pm.m_v1);
                            cap2 = vl2 + (Math::Sqr(*dp - pm.m_c2) / pm.m_v2);

                            cap1 *= lambda;
                            cap2 *= lambda;

                            // Additional T-link capacity (substitute N-links to constraints)
                            if (*etlp > 0)
                            {
                                cap2 += *etlp;
                            }
                            else
                            {
                                cap1 -= *etlp;
                            }

                            mf.SetTerminalArcCap(j, cap2, cap1);
                            j++;
                            etlp++;
                        }
                    }
                }

                /******************************************************************************/

                template <Size N, class T>
                T ComputeMasked(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, T lambda, Params<T> &pm,
                    T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IGridMaxFlow<N,T,T,T> &mf, System::Collection::Array<N,bool> &seg)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

                    // Sort neighbourhood for grid max-flow
                    Energy::Neighbourhood<N,Int32> snb(nb);
                    System::Algo::Sort::Heap(snb.Begin(), snb.End());

                    // Create array mask
                    Segmentation::Mask<N> cmask(mask);

                    // Init segmentation
                    seg.Resize(mask.Dimensions());
                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        if (mask[i] != MaskIgnore)
                        {
                            seg[i] = (mask[i] == MaskSink);
                        }
                    }

                    // Calculate capacities
                    Energy::Potential::Metric::RiemannianDanek<N,T> em(snb.template Convert<N,T>(0));
                    em.SetTransformationMatrix(Math::Algebra::SquareMatrix<N,T>(im.Spacing()));

                    // Start segmentation
                    System::Collection::Array<1,T> etl;
                    Size iter = 0;
                    T flow = 0;

                    while (iter < max_iter)
                    {
                        System::Time::StopWatch sw(__FUNCTION__, __LINE__, "iteration");

                        // Create graph
                        if (!iter || !mf.IsDynamic())
                        {
                            SetGraphNLinksMask(im, mask, cmask, snb, em.EdgeWeights(), mf, etl);
                        }
                        SetGraphTLinksMask(im, mask, lambda, pm, mf, etl);

                        // Find maximum flow
                        flow = mf.FindMaxFlow();

                        // Create the segmentation mask
                        Size j = 0;
                        const Uint8 *dm = mask.Begin();
                        for (Size i = 0; i < im.Elements(); i++, dm++)
                        {
                            if (*dm == MaskUnknown)
                            {
                                seg[i] = (mf.NodeOrigin(j) != Flow::Source);
                                j++;
                            }
                        }

                        // Update unknowns
                        Params<T> opm = pm;
                        UpdateParamsMask(im, mask, seg, pm);

                        iter++;

                        if (Math::Abs(opm.m_c1 - pm.m_c1) + Math::Abs(opm.m_c2 - pm.m_c2) +
                            Math::Abs(opm.m_v1 - pm.m_v1) + Math::Abs(opm.m_v2 - pm.m_v2) <= conv_crit)
                        {
                            break;
                        }
                    }

                    // Add the energy of the data terms outside the band to obtain global 
                    // energy value. However, if an MaskUnknown voxel has both MaskSource 
                    // and MaskSink neighbours then the energy still won't be correct because 
                    // some flow might get canceled during the computation of etl vector.
                    T w = lambda * im.VoxelSize();
                    T vl1 = log(pm.m_v1);
                    T vl2 = log(pm.m_v2);

                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        if (mask[i] == MaskSource)
                        {
                            flow += w * (vl1 + (Math::Sqr(im[i] - pm.m_c1) / pm.m_v1));
                        }
                        else if (mask[i] == MaskSink)
                        {
                            flow += w * (vl2 + (Math::Sqr(im[i] - pm.m_c2) / pm.m_v2));
                        }
                    }

                    max_iter = iter;

                    return flow;
                }

                /******************************************************************************/

                /** @cond */
                template GC_DLL_EXPORT Float32 ComputeMasked(const Data::Image<2,Float32,Float32> &im, 
                    const System::Collection::Array<2,Uint8> &mask, Float32 lambda, 
                    Params<Float32> &pm, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, 
                    Flow::IGridMaxFlow<2,Float32,Float32,Float32> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float64 ComputeMasked(const Data::Image<2,Float64,Float64> &im, 
                    const System::Collection::Array<2,Uint8> &mask, Float64 lambda,
                    Params<Float64> &pm, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, 
                    Flow::IGridMaxFlow<2,Float64,Float64,Float64> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float32 ComputeMasked(const Data::Image<3,Float32,Float32> &im, 
                    const System::Collection::Array<3,Uint8> &mask, Float32 lambda, 
                    Params<Float32> &pm, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, 
                    Flow::IGridMaxFlow<3,Float32,Float32,Float32> &mf, 
                    System::Collection::Array<3,bool> &seg);
                template GC_DLL_EXPORT Float64 ComputeMasked(const Data::Image<3,Float64,Float64> &im, 
                    const System::Collection::Array<3,Uint8> &mask, Float64 lambda,
                    Params<Float64> &pm, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, 
                    Flow::IGridMaxFlow<3,Float64,Float64,Float64> &mf, 
                    System::Collection::Array<3,bool> &seg);
                /** @endcond */

                /******************************************************************************/
            }
        }
    }
}
