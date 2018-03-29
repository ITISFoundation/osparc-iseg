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

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include "../../Math/Basic.h"
#include "../../Math/ConvergenceException.h"
#include "../../Math/Algebra/SquareMatrix.h"
#include "../../Energy/Potential/Metric/RiemannianDanek.h"
#include "../../Data/BorderIterator.h"
#include "Mask.h"
#include "ChanVese.h"

namespace Gc
{
    namespace Algo
    {
        namespace Segmentation
        {
            namespace ChanVese
            {
                //////////////////////////////////////////////////////////////////////////
                //
                // Chan-Vese segmentation routines
                //
                //////////////////////////////////////////////////////////////////////////

                /******************************************************************************/

                /** Using the current segmentation mask calculate mean values \c c1 and \c c2. */
                template <Size N, class T>
                static void UpdateMeans(const System::Collection::Array<N,T> &im, 
                    const System::Collection::Array<N,bool> &seg, T &c1, T &c2)
                {
                    const bool *dm = seg.Begin();
                    Size n1 = 0, n2 = 0;

                    c1 = 0;
                    c2 = 0;

                    for (const T *di = im.Begin(); di != im.End(); di++, dm++)
                    {
                        if (*dm)
                        {
                            c2 += *di;
                            n2++;
                        }
                        else
                        {
                            c1 += *di;
                            n1++;
                        }
                    }

                    if (!n1 || !n2)
                    {
                        throw Math::ConvergenceException(__FUNCTION__, __LINE__,
                            "Whole image segmented as foreground/background.");
                    }

                    c1 /= T(n1);
                    c2 /= T(n2);
                }

                /******************************************************************************/

                /** Creates graph and sets up N-link capacities for general max-flow. */
                template <Size N, class T>
                static void SetGraphNLinks(const Data::Image<N,T,T> &im, 
                    const Energy::Neighbourhood<N,Int32> &nb, 
                    const System::Collection::Array<1,T> &cap,
                    Flow::IMaxFlow<T,T,T> &ff)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__);

                    // Init graph
                    Size imsz = im.Elements();
                    ff.Init(imsz, (nb.Elements() / 2) * imsz, imsz, imsz);

                    // We will work only with the positive half of the neighbourhood
                    Energy::Neighbourhood<N,Int32> pnb(nb.Elements() / 2);
                    System::Collection::Array<1,Size> pofs(nb.Elements() / 2);
                    System::Collection::Array<1,T> pcap(nb.Elements() / 2);
                    Size j = 0;

                    for (Size i = 0; i < nb.Elements(); i++)
                    {
                        if (nb[i].Sgn() > 0)
                        {
                            pnb[j] = nb[i];
                            pofs[j] = Size(im.Index(nb[i]));
                            pcap[j] = cap[i];
                            j++;
                        }
                    }

                    // Get neighbourhood extent
                    Math::Algebra::Vector<N,Size> bleft, bright;
                    pnb.Extent(bleft, bright);

                    // Create data iterator
                    Data::BorderIterator<N> iter(im.Dimensions(), bleft, bright);
                    iter.Start(false);

                    Math::Algebra::Vector<N,Size> ncur = iter.CurPos();
                    Size bsize;
                    bool border;

                    // Setup N-links
                    Size n = 0;

                    while (!iter.IsFinished())
                    {
                        border = iter.NextBlock(bsize);

                        while (bsize-- > 0)
                        {
                            for (Size i = 0; i < pofs.Elements(); i++)
                            {
                                if (!border || im.IsNeighbourInside(ncur, pnb[i]))
                                {
                                    ff.SetArcCap(n, n + pofs[i], pcap[i], pcap[i]);
                                }
                            }

                            iter.NextPos(ncur);
                            n++;
                        }
                    }
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
                template <Size N, class T, class MAXFLOW>
                static void SetGraphTLinks(const Data::Image<N,T,T> &im, T l1, T l2, T c1,
                    T c2, MAXFLOW &mf)
                {
                    // Scale lambdas by voxel size (important)
                    l1 *= im.VoxelSize();
                    l2 *= im.VoxelSize();

                    // Update T-links
                    const T *dp = im.Begin();
                    T cap1, cap2;
                    
                    for (Size i = 0; i < im.Elements(); i++, dp++)
                    {
                        cap1 = l1 * Math::Sqr(*dp - c1);
                        cap2 = l2 * Math::Sqr(*dp - c2);
                        mf.SetTerminalArcCap(i, cap2, cap1);
                    }
                }

                /******************************************************************************/

                /** Generic Chan-Vese implementation. */
                template <Size N, class T, class MAXFLOW>
                static T ComputeImp(const Data::Image<N,T,T> &im, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    MAXFLOW &mf, System::Collection::Array<N,bool> &seg)
                {
                    // Init segmentation mask
                    seg.Resize(im.Dimensions());

                    // Calculate capacities
                    Energy::Potential::Metric::RiemannianDanek<N,T> em(nb.template Convert<N,T>(0));
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
                            SetGraphNLinks(im, nb, em.EdgeWeights(), mf);
                        }
                        SetGraphTLinks(im, l1, l2, c1, c2, mf);

                        // Find maximum flow
                        flow = mf.FindMaxFlow();

                        // Create the segmentation mask
                        bool *dm = seg.Begin();
                        for (Size i = 0; i < im.Elements(); i++, dm++)
                        {
                            *dm = (mf.NodeOrigin(i) != Flow::Source);
                        }

                        // Update means
                        T oc1 = c1, oc2 = c2;
                        UpdateMeans(im, seg, c1, c2);

                        iter++;

                        if (Math::Abs(c1 - oc1) + Math::Abs(c2 - oc2) <= conv_crit)
                        {
                            break;
                        }
                    }

                    max_iter = iter;

                    return flow;
                }

                /******************************************************************************/

                template <Size N, class T>
                T Compute(const Data::Image<N,T,T> &im, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IMaxFlow<T,T,T> &mf, System::Collection::Array<N,bool> &seg)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");
                    return ComputeImp(im, l1, l2, c1, c2, conv_crit, max_iter, nb, mf, seg);
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template GC_DLL_EXPORT Float32 Compute(const Data::Image<2,Float32,Float32> &im, 
                    Float32 l1, Float32 l2, Float32 &c1, Float32 &c2, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IMaxFlow<Float32,Float32,Float32> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float64 Compute(const Data::Image<2,Float64,Float64> &im, 
                    Float64 l1, Float64 l2, Float64 &c1, Float64 &c2, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IMaxFlow<Float64,Float64,Float64> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float32 Compute(const Data::Image<3,Float32,Float32> &im, 
                    Float32 l1, Float32 l2, Float32 &c1, Float32 &c2, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IMaxFlow<Float32,Float32,Float32> &mf, 
                    System::Collection::Array<3,bool> &seg);
                template GC_DLL_EXPORT Float64 Compute(const Data::Image<3,Float64,Float64> &im, 
                    Float64 l1, Float64 l2, Float64 &c1, Float64 &c2, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IMaxFlow<Float64,Float64,Float64> &mf, 
                    System::Collection::Array<3,bool> &seg);
                /** @endcond */

                /******************************************************************************/

                template <Size N, class T>
                T Compute(const Data::Image<N,T,T> &im, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IGridMaxFlow<N,T,T,T> &mf, System::Collection::Array<N,bool> &seg)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

                    // Sort neighbourhood for grid max-flow - IMPORTANT!!!
                    Energy::Neighbourhood<N,Int32> sort_nb(nb);
                    System::Algo::Sort::Heap(sort_nb.Begin(), sort_nb.End());

                    return ComputeImp(im, l1, l2, c1, c2, conv_crit, max_iter, sort_nb, mf, seg);
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template GC_DLL_EXPORT Float32 Compute(const Data::Image<2,Float32,Float32> &im, 
                    Float32 l1, Float32 l2, Float32 &c1, Float32 &c2, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IGridMaxFlow<2,Float32,Float32,Float32> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float64 Compute(const Data::Image<2,Float64,Float64> &im, 
                    Float64 l1, Float64 l2, Float64 &c1, Float64 &c2, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IGridMaxFlow<2,Float64,Float64,Float64> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float32 Compute(const Data::Image<3,Float32,Float32> &im, 
                    Float32 l1, Float32 l2, Float32 &c1, Float32 &c2, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IGridMaxFlow<3,Float32,Float32,Float32> &mf, 
                    System::Collection::Array<3,bool> &seg);
                template GC_DLL_EXPORT Float64 Compute(const Data::Image<3,Float64,Float64> &im, 
                    Float64 l1, Float64 l2, Float64 &c1, Float64 &c2, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IGridMaxFlow<3,Float64,Float64,Float64> &mf, 
                    System::Collection::Array<3,bool> &seg);
                /** @endcond */

                /******************************************************************************/

                //////////////////////////////////////////////////////////////////////////
                //
                // Masked Chan-Vese segmentation routines
                //
                //////////////////////////////////////////////////////////////////////////

                /******************************************************************************/

                template <Size N, class T>
                static void UpdateMeansMask (const System::Collection::Array<N,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, 
                    const System::Collection::Array<N,bool> &seg, T &c1, T &c2)
                {
                    const bool *ds = seg.Begin();
                    const Uint8 *dm = mask.Begin();
                    Size n1 = 0, n2 = 0;

                    c1 = 0;
                    c2 = 0;

                    for (const T *di = im.Begin(); di != im.End(); di++, ds++, dm++)
                    {
                        if (*dm != MaskIgnore)
                        {
                            if (*ds)
                            {
                                c2 += *di;
                                n2++;
                            }
                            else
                            {
                                c1 += *di;
                                n1++;
                            }
                        }
                    }

                    if (!n1 || !n2)
                    {
                        throw Math::ConvergenceException(__FUNCTION__, __LINE__,
                            "Whole image segmented as foreground/background.");
                    }

                    c1 /= T(n1);
                    c2 /= T(n2);
                }

                /******************************************************************************/

                /** Create graph and setup N-link capacities for general max-flow. */
                template <Size N, class T>
                static void SetGraphNLinksMask(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, const Segmentation::Mask<N> &cmask,
                    const Energy::Neighbourhood<N,Int32> &nb, const System::Collection::Array<1,T> &cap, 
                    Flow::IMaxFlow<T,T,T> &ff, System::Collection::Array<1,T> &etl)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__);

                    // Init max-flow
                    ff.Init(cmask.UnmaskedElements(), (nb.Elements() / 2) * cmask.UnmaskedElements(), 
                        cmask.UnmaskedElements(), cmask.UnmaskedElements());

                    // Backward indexes
                    const System::Collection::Array<N,Size> &idx(cmask.BackwardIndexes());

                    // Setup additional T-link capacities (they substitute N-links to constraints)
                    etl.Resize(cmask.UnmaskedElements(), 0);

                    // We will work only with the positive half of the neighbourhood
                    Energy::Neighbourhood<N,Int32> pnb(nb.Elements() / 2);
                    System::Collection::Array<1,Size> pofs(nb.Elements() / 2);
                    System::Collection::Array<1,T> pcap(nb.Elements() / 2);
                    Size j = 0;

                    for (Size i = 0; i < nb.Elements(); i++)
                    {
                        if (nb[i].Sgn() > 0)
                        {
                            pnb[j] = nb[i];
                            pofs[j] = Size(im.Index(nb[i]));
                            pcap[j] = cap[i];
                            j++;
                        }
                    }

                    // Get neighbourhood extent
                    Math::Algebra::Vector<N,Size> bleft, bright;
                    pnb.Extent(bleft, bright);

                    // Create data iterator
                    Data::BorderIterator<N> iter(im.Dimensions(), bleft, bright);
                    iter.Start(false);

                    Math::Algebra::Vector<N,Size> ncur = iter.CurPos();
                    Size bsize;
                    bool border;

                    // Setup N-links
                    Size n = 0;

                    while (!iter.IsFinished())
                    {
                        border = iter.NextBlock(bsize);

                        while (bsize-- > 0)
                        {
                            if (mask[n] != MaskIgnore)
                            {
                                for (Size i = 0; i < pnb.Elements(); i++)
                                {
                                    if (!border || im.IsNeighbourInside(ncur, pnb[i]))
                                    {
                                        Size ni = n + pofs[i];

                                        if (mask[n] != MaskUnknown && mask[ni] != MaskUnknown)
                                        {
                                            continue;
                                        }
                                        else if (mask[n] == mask[ni]) // Both unknown
                                        {
                                            ff.SetArcCap(idx[n], idx[ni], pcap[i], pcap[i]);
                                        }
                                        else if (mask[n] == MaskSource) // Source + unknown
                                        {
                                            etl[idx[ni]] += pcap[i];
                                        }
                                        else if (mask[ni] == MaskSource) // Unknown + source
                                        {
                                            etl[idx[n]] += pcap[i];
                                        }
                                        else if (mask[n] == MaskSink) // Sink + unknown
                                        {
                                            etl[idx[ni]] -= pcap[i];
                                        }
                                        else if (mask[ni] == MaskSink) // Unknown + sink
                                        {
                                            etl[idx[n]] -= pcap[i];
                                        }
                                    }
                                }
                            }

                            iter.NextPos(ncur);
                            n++;
                        }
                    }
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
                template <Size N, class T, class MAXFLOW>
                static void SetGraphTLinksMask (const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, T l1, T l2, T c1,
                    T c2, MAXFLOW &mf, const System::Collection::Array<1,T> &etl)
                {
                    // Scale lambdas by voxel size (important)
                    l1 *= im.VoxelSize();
                    l2 *= im.VoxelSize();

                    // Update T-links
                    const T *etlp = etl.Begin();
                    const Uint8 *dm = mask.Begin();
                    Size j = 0;
                    T cap1, cap2;
                    
                    // Setup capacities for regular nodes
                    for (const T *dp = im.Begin(); dp != im.End(); dp++, dm++)
                    {
                        if (*dm == MaskUnknown)
                        {
                            cap1 = l1 * Math::Sqr(*dp - c1);
                            cap2 = l2 * Math::Sqr(*dp - c2);

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

                /** Generic masked chan-vese implementation. */
                template <Size N, class T, class MAXFLOW>
                static T ComputeMaskedImp(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    MAXFLOW &mf, System::Collection::Array<N,bool> &seg)
                {
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
                    Energy::Potential::Metric::RiemannianDanek<N,T> em(nb.template Convert<N,T>(0));
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
                            SetGraphNLinksMask(im, mask, cmask, nb, em.EdgeWeights(), mf, etl);
                        }
                        SetGraphTLinksMask(im, mask, l1, l2, c1, c2, mf, etl);

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

                        // Update means
                        T oc1 = c1, oc2 = c2;
                        UpdateMeansMask(im, mask, seg, c1, c2);

                        iter++;

                        if (Math::Abs(c1 - oc1) + Math::Abs(c2 - oc2) <= conv_crit)
                        {
                            break;
                        }
                    }

                    // Add the energy of the data terms outside the band to obtain global 
                    // energy value. However, if an MaskUnknown voxel has both MaskSource 
                    // and MaskSink neighbours then the energy still won't be correct because 
                    // some flow might get canceled during the computation of etl vector.
                    T w1 = l1 * im.VoxelSize();
                    T w2 = l2 * im.VoxelSize();

                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        if (mask[i] == MaskSource)
                        {
                            flow += w1 * Math::Sqr(im[i] - c1);
                        }
                        else if (mask[i] == MaskSink)
                        {
                            flow += w2 * Math::Sqr(im[i] - c2);
                        }
                    }

                    max_iter = iter;

                    return flow;
                }

                /******************************************************************************/

                template <Size N, class T>
                T ComputeMasked(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IMaxFlow<T,T,T> &mf, System::Collection::Array<N,bool> &seg)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");
                    return ComputeMaskedImp(im, mask, l1, l2, c1, c2, conv_crit, max_iter,
                        nb, mf, seg);
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template GC_DLL_EXPORT Float32 ComputeMasked(const Data::Image<2,Float32,Float32> &im, 
                    const System::Collection::Array<2,Uint8> &mask, Float32 l1, Float32 l2, Float32 &c1, 
                    Float32 &c2, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IMaxFlow<Float32,Float32,Float32> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float64 ComputeMasked(const Data::Image<2,Float64,Float64> &im, 
                    const System::Collection::Array<2,Uint8> &mask, Float64 l1, Float64 l2, Float64 &c1, 
                    Float64 &c2, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IMaxFlow<Float64,Float64,Float64> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float32 ComputeMasked(const Data::Image<3,Float32,Float32> &im, 
                    const System::Collection::Array<3,Uint8> &mask, Float32 l1, Float32 l2, Float32 &c1, 
                    Float32 &c2, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IMaxFlow<Float32,Float32,Float32> &mf, 
                    System::Collection::Array<3,bool> &seg);
                template GC_DLL_EXPORT Float64 ComputeMasked(const Data::Image<3,Float64,Float64> &im, 
                    const System::Collection::Array<3,Uint8> &mask, Float64 l1, Float64 l2, Float64 &c1, 
                    Float64 &c2, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IMaxFlow<Float64,Float64,Float64> &mf, 
                    System::Collection::Array<3,bool> &seg);
                /** @endcond */

                /******************************************************************************/

                template <Size N, class T>
                T ComputeMasked(const Data::Image<N,T,T> &im, 
                    const System::Collection::Array<N,Uint8> &mask, T l1, T l2, T &c1, 
                    T &c2, T conv_crit, Size &max_iter, const Energy::Neighbourhood<N,Int32> &nb, 
                    Flow::IGridMaxFlow<N,T,T,T> &mf, System::Collection::Array<N,bool> &seg)
                {
                    System::Time::StopWatch sw(__FUNCTION__, __LINE__, "total");

                    // Sort neighbourhood for grid max-flow - IMPORTANT!!!
                    Energy::Neighbourhood<N,Int32> sort_nb(nb);
                    System::Algo::Sort::Heap(sort_nb.Begin(), sort_nb.End());

                    return ComputeMaskedImp(im, mask, l1, l2, c1, c2, conv_crit, max_iter,
                        sort_nb, mf, seg);
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template GC_DLL_EXPORT Float32 ComputeMasked(const Data::Image<2,Float32,Float32> &im, 
                    const System::Collection::Array<2,Uint8> &mask, Float32 l1, Float32 l2, Float32 &c1, 
                    Float32 &c2, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IGridMaxFlow<2,Float32,Float32,Float32> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float64 ComputeMasked(const Data::Image<2,Float64,Float64> &im, 
                    const System::Collection::Array<2,Uint8> &mask, Float64 l1, Float64 l2, Float64 &c1, 
                    Float64 &c2, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<2,Int32> &nb, Flow::IGridMaxFlow<2,Float64,Float64,Float64> &mf, 
                    System::Collection::Array<2,bool> &seg);
                template GC_DLL_EXPORT Float32 ComputeMasked(const Data::Image<3,Float32,Float32> &im, 
                    const System::Collection::Array<3,Uint8> &mask, Float32 l1, Float32 l2, Float32 &c1, 
                    Float32 &c2, Float32 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IGridMaxFlow<3,Float32,Float32,Float32> &mf, 
                    System::Collection::Array<3,bool> &seg);
                template GC_DLL_EXPORT Float64 ComputeMasked(const Data::Image<3,Float64,Float64> &im, 
                    const System::Collection::Array<3,Uint8> &mask, Float64 l1, Float64 l2, Float64 &c1, 
                    Float64 &c2, Float64 conv_crit, Size &max_iter, 
                    const Energy::Neighbourhood<3,Int32> &nb, Flow::IGridMaxFlow<3,Float64,Float64,Float64> &mf, 
                    System::Collection::Array<3,bool> &seg);
                /** @endcond */

                /******************************************************************************/
            }
        }
    }
}
