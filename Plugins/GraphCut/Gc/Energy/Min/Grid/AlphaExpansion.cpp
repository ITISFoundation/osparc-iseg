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
    MRF energy minimization using the alpha-expansion algorithm for
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
#include "AlphaExpansion.h"

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
                static T ExpansionMove(const IGridEnergy<N,T,L> &e, 
                    const System::Collection::IArrayMask<N> &mask,
                    Flow::IGridMaxFlow<N,T,T,T> &mf,
                    System::Collection::Array<1,L> &lab, L alpha)
                {
                    // Create arrays for pre-computed T-links
                    System::Collection::Array<1,T> tsrc, tsnk;

                    tsrc.Resize(lab.Elements());
                    System::Algo::Range::Fill(tsrc.Begin(), tsrc.End(), T(0));
                    tsnk.Resize(lab.Elements());
                    System::Algo::Range::Fill(tsnk.Begin(), tsnk.End(), T(0));

                    // Backward indexes
                    const System::Collection::Array<N,Size> &bw_idx(mask.BackwardIndexes());

                    // Get neighbourhood system
                    const Neighbourhood<N,Int32> &nb = e.NbSystem();

                    // Get neighbour offsets
                    Math::Algebra::Vector<N,Size> gdim = e.Dimensions();
                    System::Collection::Array<1,Int32> ofs;
                    System::Collection::Array<N,L>::Indexes(gdim, nb, ofs);

                    // Get neighbourhood extent
                    Math::Algebra::Vector<N,Size> bleft, bright;
                    nb.Extent(bleft, bright);

                    // Create data iterator
                    Data::BorderIterator<N> iter(gdim, bleft, bright);
                    iter.Start(false);
                    
                    Math::Algebra::Vector<N,Size> ncur = iter.CurPos();
                    Size bsize;
                    bool border;

                    // Setup N-links and pre-compute T-link weights
                    Size n1 = 0, n2, n1u = 0, n2u;
                    T a, b, c, d;
                    
                    mf.InitMask(gdim, nb, mask);
                                        
                    while (!iter.IsFinished())
                    {
                        border = iter.NextBlock(bsize);

                        while (bsize-- > 0)
                        {
                            if (!mask.IsMasked(n1))
                            {
                                // Data costs
                                tsrc[n1u] += e.CliquePotential(n1, alpha);
                                tsnk[n1u] += e.CliquePotential(n1, lab[n1u]);

                                // Smoothness costs
                                for (Size i = 0; i < nb.Elements() / 2; i++)
                                {
                                    if (!border || System::Collection::Array<N,L>::IsNeighbourInside(gdim, ncur, nb[i]))
                                    {
                                        n2 = n1 + ofs[i];

                                        if (!mask.IsMasked(n2))
                                        {
                                            n2u = bw_idx[n2];

                                            // Setup arc capacities based on Vladimir Kolmogorov's paper
                                            a = e.CliquePotential(n1, n2, i, lab[n1u], lab[n2u]);
                                            b = e.CliquePotential(n1, n2, i, lab[n1u], alpha);
                                            c = e.CliquePotential(n1, n2, i, alpha, lab[n2u]);
                                            d = e.CliquePotential(n1, n2, i, alpha, alpha);

                                            tsrc[n1u] += d;
                                            tsnk[n1u] += a;

                                            b -= a;
                                            c -= d;

                                            if (b + c < 0)
                                            {
                                                throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                                                    System::Format("Given energy is not submodular for node pair "
                                                        "n1 = {0}, n2 = {1} and labels alpha = {2}, l1 = {3}, l2 = {4}.") 
                                                        << n1 << n2 << alpha << lab[n1u] << lab[n2u]);
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
                    for (Size i = 0; i < lab.Elements(); i++)
                    {
                        mf.SetTerminalArcCap(i, tsrc[i], tsnk[i]);
                    }

                    // Find maximum flow
                    T flow = mf.FindMaxFlow();

                    // Update labeling
                    for (Size i = 0; i < lab.Elements(); i++)
                    {
                        if (mf.NodeOrigin(i) == Flow::Sink)
                        {
                            lab[i] = alpha;
                        }
                    }

                    return flow;
                }

                /***********************************************************************************/

                template <Size N, class T, class L>
                T AlphaExpansion(const IGridEnergy<N,T,L> &e, const System::Collection::IArrayMask<N> &mask,
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

                    // Perform alpha-expansion moves while energy decreases
                    T min_energy = esmooth + edata, cur_energy = min_energy;
                    Size iter = 0;

                    while (iter < max_iter)
                    {
                        iter++;

                        // Do expansion move for all labels
                        for (L alpha = 0; Size(alpha) < glabs; alpha++)
                        {
                            ExpansionMove(e, mask, mf, lab, alpha);
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
		        template GC_DLL_EXPORT Float32 AlphaExpansion(const IGridEnergy<2,Float32,Uint8> &e, 
                    const System::Collection::IArrayMask<2> &mask,
                    Flow::IGridMaxFlow<2,Float32,Float32,Float32> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint8> &lab);
		        template GC_DLL_EXPORT Float64 AlphaExpansion(const IGridEnergy<2,Float64,Uint8> &e, 
                    const System::Collection::IArrayMask<2> &mask,
                    Flow::IGridMaxFlow<2,Float64,Float64,Float64> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint8> &lab);
		        template GC_DLL_EXPORT Float32 AlphaExpansion(const IGridEnergy<3,Float32,Uint8> &e, 
                    const System::Collection::IArrayMask<3> &mask,
                    Flow::IGridMaxFlow<3,Float32,Float32,Float32> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint8> &lab);
		        template GC_DLL_EXPORT Float64 AlphaExpansion(const IGridEnergy<3,Float64,Uint8> &e, 
                    const System::Collection::IArrayMask<3> &mask,
                    Flow::IGridMaxFlow<3,Float64,Float64,Float64> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint8> &lab);

		        template GC_DLL_EXPORT Float32 AlphaExpansion(const IGridEnergy<2,Float32,Uint16> &e, 
                    const System::Collection::IArrayMask<2> &mask,
                    Flow::IGridMaxFlow<2,Float32,Float32,Float32> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint16> &lab);
		        template GC_DLL_EXPORT Float64 AlphaExpansion(const IGridEnergy<2,Float64,Uint16> &e, 
                    const System::Collection::IArrayMask<2> &mask,
                    Flow::IGridMaxFlow<2,Float64,Float64,Float64> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint16> &lab);
		        template GC_DLL_EXPORT Float32 AlphaExpansion(const IGridEnergy<3,Float32,Uint16> &e, 
                    const System::Collection::IArrayMask<3> &mask,
                    Flow::IGridMaxFlow<3,Float32,Float32,Float32> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint16> &lab);
		        template GC_DLL_EXPORT Float64 AlphaExpansion(const IGridEnergy<3,Float64,Uint16> &e, 
                    const System::Collection::IArrayMask<3> &mask,
                    Flow::IGridMaxFlow<3,Float64,Float64,Float64> &mf,
                    Size &max_iter, System::Collection::Array<1,Uint16> &lab);
                /** @endcond */

                /***********************************************************************************/
            }
        }
	}
}

