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
    MRF energy minimization using the alpha/beta-swap algorithm for
    labeling problems defined on regular grids.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#include "../../../System/ArgumentException.h"
#include "../../../System/InvalidOperationException.h"
#include "../../../System/Collection/BoolArrayMask.h"
#include "../../../System/Format.h"
#include "../../../Data/BorderIterator.h"
#include "LabelingEnergy.h"
#include "AlphaBetaSwap.h"

namespace Gc
{    
	namespace Energy
	{
        namespace Min
        {
            namespace Grid
            {
                /***********************************************************************************/

                template <Size N, class T, class L>
                static T SwapMove(const IGridEnergy<N,T,L> &e, 
                    const System::Collection::IArrayMask<N> &mask,
                    Flow::IGridMaxFlow<N,T,T,T> &mf,
                    System::Collection::Array<1,L> &lab, L l1, L l2)
                {
                    // Create mask for given two labels
                    L *plab = lab.Begin();
                    System::Collection::Array<N,bool> l12mask;
                    l12mask.Resize(e.Dimensions());

                    for (Size i = 0; i < l12mask.Elements(); i++)
                    {
                        if (mask.IsMasked(i))
                        {
                            l12mask[i] = true;
                        }
                        else
                        {
                            l12mask[i] = ((*plab) != l1 && (*plab) != l2);
                            plab++;
                        }
                    }

                    System::Collection::BoolArrayMask<N> gmask(l12mask, true);

                    // Create arrays for pre-computed T-links
                    System::Collection::Array<1,T> tsrc, tsnk;

                    tsrc.Resize(gmask.UnmaskedElements());
                    System::Algo::Range::Fill(tsrc.Begin(), tsrc.End(), T(0));
                    tsnk.Resize(gmask.UnmaskedElements());
                    System::Algo::Range::Fill(tsnk.Begin(), tsnk.End(), T(0));

                    // Backward indexes
                    const System::Collection::Array<N,Size> &bw_idx(gmask.BackwardIndexes());

                    // Get neighbourhood system
                    const Neighbourhood<N,Int32> &nb = e.NbSystem();

                    // Get neighbour offsets
                    System::Collection::Array<1,Int32> ofs;
                    l12mask.Indexes(nb, ofs);

                    // Get neighbourhood extent
                    Math::Algebra::Vector<N,Size> bleft, bright;
                    nb.Extent(bleft, bright);

                    // Create data iterator
                    Data::BorderIterator<N> iter(l12mask.Dimensions(), bleft, bright);
                    iter.Start(false);
                    
                    Math::Algebra::Vector<N,Size> ncur = iter.CurPos();
                    Size bsize;
                    bool border;

                    // Setup N-links and pre-compute T-link weights
                    Size n1 = 0, n2, n1u = 0, n2u;
                    T a, b, c, d;
                    
                    mf.InitMask(e.Dimensions(), nb, gmask);
                                        
                    while (!iter.IsFinished())
                    {
                        border = iter.NextBlock(bsize);

                        while (bsize-- > 0)
                        {
                            if (!l12mask[n1])
                            {
                                // Data costs
                                tsrc[n1u] += e.CliquePotential(n1, l2);
                                tsnk[n1u] += e.CliquePotential(n1, l1);

                                // Smoothness costs
                                for (Size i = 0; i < nb.Elements() / 2; i++)
                                {
                                    if (!border || l12mask.IsNeighbourInside(ncur, nb[i]))
                                    {
                                        n2 = n1 + ofs[i];

                                        if (!l12mask[n2])
                                        {
                                            n2u = bw_idx[n2];

                                            // Setup arc capacities based on Vladimir Kolmogorov's paper
                                            a = e.CliquePotential(n1, n2, i, l1, l1);
                                            b = e.CliquePotential(n1, n2, i, l1, l2);
                                            c = e.CliquePotential(n1, n2, i, l2, l1);
                                            d = e.CliquePotential(n1, n2, i, l2, l2);

                                            tsrc[n1u] += d;
                                            tsnk[n1u] += a;

                                            b -= a;
                                            c -= d;

                                            if (b + c < 0)
                                            {
                                                throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                                                    System::Format("Given energy is not submodular for node pair "
                                                        "n1 = {0}, n2 = {1} and labels l1 = {2}, l2 = {3}.") 
                                                        << n1 << n2 << l1 << l2);
                                            }

                                            if (b < 0)
                                            {
                                                tsnk[n1u] += b;
                                                tsnk[n2u] -= b;
                                                mf.SetArcCap(n2u, nb.Elements() - i - 1, b + c); 
                                            }
                                            else if (c < 0)
                                            {
                                                tsnk[n1u] -= c;
                                                tsnk[n2u] += c;
                                                mf.SetArcCap(n1u, i, b + c);
                                            }
                                            else
                                            {
                                                mf.SetArcCap(n1u, i, b);
                                                mf.SetArcCap(n2u, nb.Elements() - i - 1, c); 
                                            }
                                        }
                                    }
                                }

                                n1u++;
                            }

                            iter.NextPos(ncur);
                            n1++;
                        }
                    }

                    // Setup T-links
                    Size nuel = gmask.UnmaskedElements();
                    for (Size i = 0; i < nuel; i++)
                    {
                        mf.SetTerminalArcCap(i, tsrc[i], tsnk[i]);
                    }

                    // Find maximum flow
                    T flow = mf.FindMaxFlow();

                    // Update labeling
                    n1 = 0;
                    n2 = 0;
                    for (Size i = 0; i < l12mask.Elements(); i++)
                    {
                        if (!mask.IsMasked(i))
                        {
                            if (!l12mask[i])
                            {
                                lab[n1] = (mf.NodeOrigin(n2) == Flow::Source) ? l1 : l2;
                                n2++;
                            }

                            n1++;
                        }
                    }

                    return flow;
                }

                /***********************************************************************************/

                template <Size N, class T, class L>
                T AlphaBetaSwap(const IGridEnergy<N,T,L> &e, const System::Collection::IArrayMask<N> &mask,
                    Flow::IGridMaxFlow<N,T,T,T> &mf, Size &max_iter, System::Collection::Array<1,L> &lab)
                {
                    // Check input
                    if (!e.Dimensions().Product())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__,
                            "Empty energy, nothing to minimize.");
                    }

                    if (e.Labels() < 2)
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__,
                            "Number of labels must be at least 2.");
                    }

                    if (e.Dimensions() != mask.Dimensions())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__,
                            "Incompatible mask and energy dimensions.");
                    }

                    // Check that neighbourhood system is symmetric and sorted
                    if (e.NbSystem().Elements() % 2 != 0)
                    {
                        throw System::InvalidOperationException(__FUNCTION__, __LINE__, "The energy "
                            "neighbourhood system must have even number of elements.");
                    }

                    Size nbels = e.NbSystem().Elements();
                    for (Size i = 0; i < nbels / 2; i++)
                    {
                        if (e.NbSystem()[i] != -e.NbSystem()[nbels - i - 1])
                        {
                            throw System::InvalidOperationException(__FUNCTION__, __LINE__, "The energy "
                                "neighbourhood system is not symmetric.");
                        }
                    }

                    // Create initial labeling - label with minimal data cost is assigned
                    // to each node
                    Size glabs = e.Labels();
                    lab.Resize(mask.UnmaskedElements());
                    L *plab = lab.Begin();

                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        if (!mask.IsMasked(i))
                        {
                            T emin = e.CliquePotential(i, 0);
                            *plab = 0;

                            for (L l = 1; Size(l) < glabs; l++)
                            {
                                if (e.CliquePotential(i, l) < emin)
                                {
                                    emin = e.CliquePotential(i, l);
                                    *plab = l;
                                }
                            }

                            plab++;
                        }
                    }

                    // Get enerrgy of initial labeling
                    T esmooth, edata;
                    LabelingEnergy(e, mask, lab, esmooth, edata);

                    // Perform alpha-beta swaps while energy decreases
                    T min_energy = esmooth + edata, cur_energy = min_energy;
                    Size iter = 0;

                    while (iter < max_iter)
                    {
                        iter++;

                        // Do swap move for all pairs of labels
                        for (L l1 = 0; Size(l1) < glabs - 1; l1++)
                        {
                            for (L l2 = l1 + 1; Size(l2) < glabs; l2++)
                            {
                                SwapMove(e, mask, mf, lab, l1, l2);
                            }
                        }

                        // Get energy of updated labeling
                        LabelingEnergy(e, mask, lab, esmooth, edata);
                        cur_energy = edata + esmooth;

                        if (cur_energy >= min_energy)
                        {
                            break;
                        }

                        min_energy = cur_energy;
                    }

                    max_iter = iter;
                    mf.Dispose();

                    return cur_energy;
                }

                /***********************************************************************************/

    		    // Explicit instantiations
                /** @cond */
		        template GC_DLL_EXPORT Float32 AlphaBetaSwap(const IGridEnergy<2,Float32,Uint8> &e, 
                    const System::Collection::IArrayMask<2> &mask,
                    Flow::IGridMaxFlow<2,Float32,Float32,Float32> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint8> &lab);
		        template GC_DLL_EXPORT Float64 AlphaBetaSwap(const IGridEnergy<2,Float64,Uint8> &e, 
                    const System::Collection::IArrayMask<2> &mask,
                    Flow::IGridMaxFlow<2,Float64,Float64,Float64> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint8> &lab);
		        template GC_DLL_EXPORT Float32 AlphaBetaSwap(const IGridEnergy<3,Float32,Uint8> &e, 
                    const System::Collection::IArrayMask<3> &mask,
                    Flow::IGridMaxFlow<3,Float32,Float32,Float32> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint8> &lab);
		        template GC_DLL_EXPORT Float64 AlphaBetaSwap(const IGridEnergy<3,Float64,Uint8> &e, 
                    const System::Collection::IArrayMask<3> &mask,
                    Flow::IGridMaxFlow<3,Float64,Float64,Float64> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint8> &lab);

		        template GC_DLL_EXPORT Float32 AlphaBetaSwap(const IGridEnergy<2,Float32,Uint16> &e, 
                    const System::Collection::IArrayMask<2> &mask,
                    Flow::IGridMaxFlow<2,Float32,Float32,Float32> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint16> &lab);
		        template GC_DLL_EXPORT Float64 AlphaBetaSwap(const IGridEnergy<2,Float64,Uint16> &e, 
                    const System::Collection::IArrayMask<2> &mask,
                    Flow::IGridMaxFlow<2,Float64,Float64,Float64> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint16> &lab);
		        template GC_DLL_EXPORT Float32 AlphaBetaSwap(const IGridEnergy<3,Float32,Uint16> &e, 
                    const System::Collection::IArrayMask<3> &mask,
                    Flow::IGridMaxFlow<3,Float32,Float32,Float32> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint16> &lab);
		        template GC_DLL_EXPORT Float64 AlphaBetaSwap(const IGridEnergy<3,Float64,Uint16> &e, 
                    const System::Collection::IArrayMask<3> &mask,
                    Flow::IGridMaxFlow<3,Float64,Float64,Float64> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint16> &lab);
                /** @endcond */

                /***********************************************************************************/
            }
        }
	}
}

