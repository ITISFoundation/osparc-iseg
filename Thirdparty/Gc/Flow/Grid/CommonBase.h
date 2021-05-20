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

#ifndef GC_FLOW_GRID_COMMONBASE_H
#define GC_FLOW_GRID_COMMONBASE_H

#include "../../Core.h"
#include "../IGridMaxFlow.h"

namespace Gc {
namespace Flow {
/** Maximum flow algorithms for grid graphs. 
        
            This namespace contains maximum flow algorithms that are optimized for use with
            grid graphs. Assuming that the nodes of the graph form a regular orthogonal grid
            and that all nodes have topologically equivalent neighbourhood. These assumptions
            allow to make the algorithms faster and particularly decrease their memory
            consumption substantially.

            @see Grid::IGridMaxFlow, Grid::CommonBase.
        */
namespace Grid {
/** Common base for many grid maximum flow algorithms. 
            
                This class provides some base functionality for many other grid
                maximum-flow algorithms.

                @warning All algorithms derived from this class impose the following
                    constraints on the node neighbourhood: The maximum size of the
                    neighbourhood is 255 and the neighbourhood must be symmetric
                    (see ValidateParameters() for more info).
            */
template <Size N, class TFLOW, class TTCAP, class TCAP>
class GC_DLL_EXPORT CommonBase
    : public IGridMaxFlow<N, TFLOW, TTCAP, TCAP>
{
  protected:
    /** Neighbour index type. 
                
                    @remarks If you need more than 255 neighbours it is
                        safe to change this type to a larger one.
                */
    using NbIndexType = Uint8;

    /** Arc capacity array. */
    System::Collection::Array<1, TCAP> m_arc_cap;
    /** Neighbourhood. */
    Energy::Neighbourhood<N, Int32> m_nb;
    /** Neighbour offset. */
    System::Collection::Array<1, Int32> m_n_ofs;
    /** Grid dimensions. */
    Math::Algebra::Vector<N, Size> m_dim;
    /** Array containing grid -> node index conversion values (used only with mask). */
    System::Collection::Array<N, Size> m_bw_idx;

  protected:
    /** Free memory occupied by the object. */
    void DisposeBase()
    {
        m_arc_cap.Dispose();
        m_nb.Dispose();
        m_n_ofs.Dispose();
        m_dim.SetZero();
        m_bw_idx.Dispose();
    }

    /** Validates input parameters. 
                
                    Checks that \c dim and \c nb are non-empty and that they are not
                    too large. Also checks that \c nb is symmetric, which means that
                    for offset \c i opposite vector is at position <tt>nb.Elements() - 1 - i</tt>.

                    @see CommonBase.
                */
    void ValidateParameters(const Math::Algebra::Vector<N, Size> & dim,
                            const Energy::Neighbourhood<N, Int32> & nb);

    /** Init arc capacities and neighbour offsets.

                    Arcs that go out of the grid or to a masked node are marked with 
                    negative capacity value. Assumes that the arc capacities array has
                    already been allocated.
                */
    void InitBase(const Math::Algebra::Vector<N, Size> & dim,
                  const Energy::Neighbourhood<N, Int32> & nb);

    /** Init arc capacities, neighbour offsets and index conversion array.

                    Arcs that go out of the grid or to a masked node are marked with 
                    negative capacity value. Assumes that the arc capacities array has
                    already been allocated.
                */
    void InitBaseMasked(const Math::Algebra::Vector<N, Size> & dim,
                        const Energy::Neighbourhood<N, Int32> & nb,
                        const System::Collection::IArrayMask<N> & mask);

    /** Find sister arc index. */
    Size Sister(Size arc) const
    {
        return m_nb.Elements() - 1 - arc;
    }
};
} // namespace Grid
}
} // namespace Gc::Flow

#endif
