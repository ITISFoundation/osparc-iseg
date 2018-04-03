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
    Auxiliary method for retrivieng the energy of a given grid labeling.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_ENERGY_MIN_GRID_LABELINGENERGY_H
#define GC_ENERGY_MIN_GRID_LABELINGENERGY_H

#include "../../../System/Collection/Array.h"
#include "../../../System/Collection/IArrayMask.h"
#include "../../../Data/BorderIterator.h"
#include "../../IGridEnergy.h"

namespace Gc
{    
	namespace Energy
	{
        namespace Min
        {
            namespace Grid
            {
                /***********************************************************************************/

                /** Calculate the labeling energy for a given IGridEnergy object and labeling.
                
                    @param[in] e %Grid energy specification. 
                    @param[in] mask %Grid mask.
                    @param[in] lab Labeling of the unmasked nodes.
                    @param[out] edata %Energy corresponding to the data terms (unary clique 
                        potential functions).
                    @param[out] esmooth %Energy corresponding to the smoothness terms (pairwise
                        clique potential functions).

                    @remark The energy neighbourhood system must be symmetric, i.e., it must
                        hold that <tt>nb[i] = -nb[nb.Elements() - i - 1]</tt>. This is easily achieved
                        by sorting the neighbourhood in advance.

                    @see IGridEnergy.
                */
                template <Size N, class T, class L>
                void LabelingEnergy(const IGridEnergy<N,T,L> &e, 
                    const System::Collection::IArrayMask<N> &mask,
                    System::Collection::Array<1,L> &lab, T &edata, T &esmooth)
                {
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

                    // Get labeling energy
                    Size n1 = 0, n2, n1u = 0, n2u;

                    edata = 0;
                    esmooth = 0;
                    
                    while (!iter.IsFinished())
                    {
                        border = iter.NextBlock(bsize);

                        while (bsize-- > 0)
                        {
                            if (!mask.IsMasked(n1))
                            {
                                // Data cost - unary clique potential
                                edata += e.CliquePotential(n1, lab[n1u]);

                                // Smoothness costs - pairwise clique potentials
                                for (Size i = 0; i < nb.Elements() / 2; i++)
                                {
                                    if (!border || System::Collection::Array<N,L>::IsNeighbourInside(gdim, ncur, nb[i]))
                                    {
                                        n2 = n1 + ofs[i];

                                        if (!mask.IsMasked(n2))
                                        {
                                            n2u = bw_idx[n2];
                                            esmooth += e.CliquePotential(n1, n2, i, lab[n1u], lab[n2u]);
                                        }
                                    }
                                }

                                n1u++;
                            }

                            iter.NextPos(ncur);
                            n1++;
                        }
                    }
                }

                /***********************************************************************************/
            }
        }
	}
}

#endif
