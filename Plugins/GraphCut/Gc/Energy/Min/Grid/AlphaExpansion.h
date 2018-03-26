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

#ifndef GC_ENERGY_MIN_GRID_ALPHAEXPANSION_H
#define GC_ENERGY_MIN_GRID_ALPHAEXPANSION_H

#include "../../../Core.h"
#include "../../../System/Collection/IArrayMask.h"
#include "../../IGridEnergy.h"
#include "../../../Flow/IGridMaxFlow.h"

namespace Gc
{    
	namespace Energy
	{
        namespace Min
        {
            namespace Grid
            {
                /** Minimize energy corresponding to a given grid labeling problem
                    using the \f$\alpha\f$-expansion method.                

                    This method is based on the following papers:
                    - Yuri Boykov, Olga Veksler, Ramin Zabih: <em>Efficient Approximate 
                    %Energy Minimization via Graph Cuts</em>, IEEE Transaction on Pattern 
                    Analysis and Machine Intelligence, vol. 20, no. 12, p. 1222-1239, 
                    November 2001.
                    - Vladimir Kolmogorov, Ramin Zabih: <em>What energy functions can be 
                    minimized via graph cuts?</em>, IEEE Transactions on Pattern Analysis 
                    and Machine Intelligence, 26(2):147-159, 2004.

                    And the following implementations:
                    - http://vision.csd.uwo.ca/code/: gco-v3.0

                    It is strongly recommended to cite the aforementioned papers if you
                    use this algorithm in your work. Be aware that there is a US patent
                    on the method!

                    @param[in] e %Energy specification. The energy must be submodular
                        (see Vladimir Kolmogorov's paper).
                    @param[in] mask %Grid mask.
                    @param[in] mf Maximum-flow algorithm for grid problems (must allow 
                        mask specification).
                    @param[in,out] max_iter Maximum number of iterations. At the end of
                        computation this parameter contains the total number of iterations 
                        performed.
                    @param[out] lab Final node labeling for the unmasked nodes.

                    @return %Energy of the final labeling.

                    @remark The energy neighbourhood system must be symmetric, i.e., it must
                        hold that <tt>nb[i] = -nb[nb.Elements() - i - 1]</tt>. This is easily achieved
                        by sorting the neighbourhood in advance.

                    @see IGridEnergy.
                */
                template <Size N, class T, class L>
                T GC_DLL_EXPORT AlphaExpansion(const IGridEnergy<N,T,L> &e, 
                    const System::Collection::IArrayMask<N> &mask,
                    Flow::IGridMaxFlow<N,T,T,T> &mf,
                    Size &max_iter, 
                    System::Collection::Array<1,L> &lab);
            }
        }
	}
}

#endif
