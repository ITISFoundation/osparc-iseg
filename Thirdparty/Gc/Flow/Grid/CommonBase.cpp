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
    Common base for many grid maximum flow algorithms.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include <limits>
#include "../../System/ArgumentException.h"
#include "../../System/OverflowException.h"
#include "../../System/InvalidOperationException.h"
#include "../../Data/BorderIterator.h"
#include "CommonBase.h"

namespace Gc {
namespace Flow {
namespace Grid {
/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP>
void CommonBase<N, TFLOW, TTCAP, TCAP>::ValidateParameters(const Math::Algebra::Vector<N, Size> & dim,
                                                           const Energy::Neighbourhood<N, Int32> & nb)
{
    // Check that graph is not empty
    if (System::Algo::Range::AnyEqualTo(dim.Begin(), dim.End(), Size(0)) || nb.IsEmpty())
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__, "Empty graph.");
    }

    // Check overflow
    Size max_elems = std::numeric_limits<Size>::max();

    for (Size i = 0; i < N; i++)
    {
        max_elems /= dim[i];
    }

    if (!max_elems)
    {
        throw System::OverflowException(__FUNCTION__, __LINE__, "The number of elements is too large "
                                                                "to be stored in the memory.");
    }

    // Check neighbourhood
    if (nb.Elements() + 2 > std::numeric_limits<NbIndexType>::max())
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__, "The neighbourhood is too large.");
    }

    if (nb.Elements() % 2 != 0)
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__, "The neighbourhood must have even number "
                                                                "of elements.");
    }

    // Check symmetry of the neighbourhood
    for (Size i = 0; i < nb.Elements() / 2; i++)
    {
        if (m_bw_idx.Index(dim, nb[i]) != -m_bw_idx.Index(dim, nb[nb.Elements() - i - 1]))
        {
            throw System::ArgumentException(__FUNCTION__, __LINE__, "The neighbourhood is not symmetric.");
        }
    }
}

/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP>
void CommonBase<N, TFLOW, TTCAP, TCAP>::InitBase(const Math::Algebra::Vector<N, Size> & dim,
                                                 const Energy::Neighbourhood<N, Int32> & nb)
{
    // Allocate memory for arcs
    if (m_arc_cap.IsEmpty())
    {
        throw System::InvalidOperationException(__FUNCTION__, __LINE__, "Memory for arc capacities has "
                                                                        "not been allocated.");
    }

    // Init neighbourhood
    m_nb = nb;

    // Calculate neighbour indexes
    m_bw_idx.Indexes(dim, m_nb, m_n_ofs);

    // Get neighbourhood extent
    Math::Algebra::Vector<N, Size> bleft, bright;
    nb.Extent(bleft, bright);

    // Setup data iterator
    Data::BorderIterator<N> iter(dim, bleft, bright);
    iter.Start(false);

    Math::Algebra::Vector<N, Size> ncur = iter.CurPos();
    Size bsize;
    bool border;

    // Init arc capacities - set -1 capacity to edges going out of the grid
    TCAP * edgep = m_arc_cap.Begin();
    while (!iter.IsFinished())
    {
        border = iter.NextBlock(bsize);

        while (bsize-- > 0)
        {
            for (Size i = 0; i < nb.Elements(); i++, edgep++)
            {
                if (border && !m_bw_idx.IsNeighbourInside(dim, ncur, m_nb[i]))
                {
                    *edgep = TCAP(-1); // Neighbour outside grid - invalid edge
                }
                else
                {
                    *edgep = 0;
                }
            }

            iter.NextPos(ncur);
        }
    }

    // Save dimensions info
    m_dim = dim;
}

/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP>
void CommonBase<N, TFLOW, TTCAP, TCAP>::InitBaseMasked(const Math::Algebra::Vector<N, Size> & dim,
                                                       const Energy::Neighbourhood<N, Int32> & nb, const System::Collection::IArrayMask<N> & mask)
{
    // Allocate memory for arcs
    if (m_arc_cap.IsEmpty())
    {
        throw System::InvalidOperationException(__FUNCTION__, __LINE__, "Memory for arc capacities has "
                                                                        "not been allocated.");
    }

    // Get grid -> node index conversion array
    m_bw_idx = mask.BackwardIndexes();

    // Init neighbourhood
    m_nb = nb;

    // Calculate neighbour indexes
    m_bw_idx.Indexes(m_nb, m_n_ofs);

    // Get neighbourhood extent
    Math::Algebra::Vector<N, Size> bleft, bright;
    nb.Extent(bleft, bright);

    // Setup data iterator
    Data::BorderIterator<N> iter(dim, bleft, bright);
    iter.Start(false);

    Math::Algebra::Vector<N, Size> ncur = iter.CurPos();
    Size bsize;
    bool border;

    // Init arc capacities - set -1 capacity to edges going out of the grid
    TCAP * edgep = m_arc_cap.Begin();
    Size grid_node_idx = 0;

    while (!iter.IsFinished())
    {
        border = iter.NextBlock(bsize);

        while (bsize-- > 0)
        {
            if (!mask.IsMasked(grid_node_idx))
            {
                for (Size i = 0; i < nb.Elements(); i++, edgep++)
                {
                    if (border && !m_bw_idx.IsNeighbourInside(ncur, m_nb[i]))
                    {
                        *edgep = -1; // Neighbour outside grid - invalid edge
                    }
                    else
                    {
                        // Check if neighbour is masked -> invalid edge
                        *edgep = mask.IsMasked(grid_node_idx + m_n_ofs[i]) ? TCAP(-1) : TCAP(0);
                    }
                }
            }

            iter.NextPos(ncur);
            grid_node_idx++;
        }
    }

    // Save dimensions info
    m_dim = dim;
}

/***********************************************************************************/

// Explicit instantiations
/** @cond */
template class GC_DLL_EXPORT CommonBase<2, Int32, Int32, Int32>;
template class GC_DLL_EXPORT CommonBase<2, Float32, Float32, Float32>;
template class GC_DLL_EXPORT CommonBase<2, Float64, Float64, Float64>;
template class GC_DLL_EXPORT CommonBase<3, Int32, Int32, Int32>;
template class GC_DLL_EXPORT CommonBase<3, Float32, Float32, Float32>;
template class GC_DLL_EXPORT CommonBase<3, Float64, Float64, Float64>;
/** @endcond */

/***********************************************************************************/
}
}
} // namespace Gc::Flow::Grid
