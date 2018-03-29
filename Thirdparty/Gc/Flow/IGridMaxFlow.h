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
    Common interface of maximum flow algorithms for grid graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_IGRIDMAXFLOW_H
#define GC_FLOW_IGRIDMAXFLOW_H

#include "../Type.h"
#include "../Energy/Neighbourhood.h"
#include "../System/Collection/Array.h"
#include "../System/Collection/IArrayMask.h"
#include "Type.h"

namespace Gc
{    
	namespace Flow
	{
        /** Common %interface of maximum flow algorithms on grid graphs.

            Grid graphs are graphs in which nodes are organized into a regular orthogonal
            N-dimensional grid and it is assumed that all nodes have a topologically equivalent
            neighbourhood. These assumptions often allow to significantly optimize memory
            consumption and speed of the max-flow algorithm.

            Generally, the scenario should look like this:
            -# Initialize the algorithm by a call to Init().
            -# Specify the arc capacities by calling SetArcCap() and SetTerminalArcCap().
            -# Call FindMaxFlow() to compute the maxmimum flow/minimum cut.
            -# Find final node origin using NodeOrigin().
            -# Optionally, for non-dynamic algorithms go to 1. For dynamic ones update the graph
            using SetArcCap() and SetTerminalArcCap() and go to 3.

            @tparam N Dimensionality of the grid.
            @tparam TFLOW %Data type used for the flow value.
            @tparam TCAP %Data type used for terminal arc capacity values.
            @tparam TCAP %Data type used for arc capacity values.
        */
	    template <Size N, class TFLOW, class TTCAP, class TCAP>
	    class IGridMaxFlow
	    {
	    public:
            /** Virtual destructor. */
            virtual ~IGridMaxFlow()
            {}

		    /** Initialize the grid flow network.

			    @param[in] dim Dimensions of the grid.
                @param[in] nb Node neighbourhood specification. This neighbourhood is same for all nodes.
		    */
            virtual void Init(const Math::Algebra::Vector<N,Size> &dim, 
                const Energy::Neighbourhood<N,Int32> &nb) = 0;

		    /** Initialize the grid flow network with masked nodes.

			    @param[in] dim Dimensions of the grid.
                @param[in] nb Node neighbourhood specification. This neighbourhood is same for all nodes.
                @param[in] mask Node mask predicate. This predicate is able to say which nodes are masked and should not be
                    included in the computation.
		    */
            virtual void InitMask(const Math::Algebra::Vector<N,Size> &dim, 
                const Energy::Neighbourhood<N,Int32> &nb,
                const System::Collection::IArrayMask<N> &mask) = 0;

		    /** Set arc capacity.

                @param[in] node Node index.
                @param[in] arc Arc index (index to the neighbourhood passed to the Init() method).
                @param[in] cap Arc capacity. Should be non-negative.

                @see Init.
		    */
		    virtual void SetArcCap(Size node, Size arc, TCAP cap) = 0;

		    /** Set the capacity of the arcs connecting a node and the terminals.

                Using this method \c source -> \c node and \c node -> \c sink
                arc capacity is set to \c cap_src and \c cap_sink, respectively.
                It is usually assumed that you call this method at most once
                for each node for each call to FindMaxFlow().

			    @param[in] node Node index.
			    @param[in] csrc \c source -> \c node arc capacity.
			    @param[in] csnk \c node -> \c sink arc capacity.

                @remarks Specifying the capacity of \c node -> \c source and
                \c sink -> \c node arcs is not possible and this capacity is
                without a loss of generality always zero.
		    */
		    virtual void SetTerminalArcCap(Size node, TTCAP csrc, TTCAP csnk) = 0;

		    /** Compute the maximum flow.                

                For non-dynamic algorithms this method can be called usually only once for
                each call to Init().

			    @return %Flow value.
		    */
		    virtual TFLOW FindMaxFlow() = 0;

		    /** Get node's origin after the flow computation.
				
			    @param[in] node Node index.
			    @return Associated terminal node (source, sink or free).
                @see Origin.
		    */
		    virtual Origin NodeOrigin(Size node) const = 0;

            /** Checks wether this algorithm supports dynamic change of capacities. */
            virtual bool IsDynamic() const
            {
                return false;
            }

		    /** Release the allocated memory. */
		    virtual void Dispose() = 0;
	    };
	}
}

#endif
