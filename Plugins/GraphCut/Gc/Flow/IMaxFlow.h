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
    Common interface of maximum flow algorithms for general 
    directed and undirected graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_IMAXFLOW_H
#define GC_FLOW_IMAXFLOW_H

#include "../Type.h"
#include "Type.h"

namespace Gc
{    
	namespace Flow
	{
        /** Common %interface of maximum flow algorithms on general directed and
            undirected graphs.

            Generally, the scenario should look like this:
            -# Initialize the algorithm by a call to Init().
            -# Specify the arc capacities by calling SetArcCap() and SetTerminalArcCap().
            -# Call FindMaxFlow() to compute the maxmimum flow/minimum cut.
            -# Find node origin using NodeOrigin().
            -# Optionally, for non-dynamic algorithms go to 1. For dynamic ones update the graph
            using SetArcCap() and SetTerminalArcCap() and go to 3.

            @tparam TFLOW %Data type used for the flow value.
            @tparam TCAP %Data type used for terminal arc capacity values.
            @tparam TCAP %Data type used for arc capacity values.
        */
	    template <class TFLOW, class TTCAP, class TCAP>
	    class IMaxFlow
	    {
	    public:
            /** Virtual destructor. */
            virtual ~IMaxFlow()
            {}

		    /** Initialize the flow network.

			    @param[in] nodes Number of network nodes.
			    @param[in] max_arcs Maximum number of non-terminal arcs.
                @param[in] max_src_arcs Maximum number of arcs going from the source.
                @param[in] max_snk_arcs Maximum number of arcs going to the sink.

                @remarks By an arc we mean the bidirectional arc created by a call
                to the SetArcCap() method, i.e., the number of arcs is equivalent to
                the number of calls of this method in the graph creating stage.

                @see SetArcCap, SetTerminalArcCap.
		    */
		    virtual void Init(Size nodes, Size max_arcs, Size max_src_arcs, Size max_snk_arcs) = 0;

		    /** Set the capacity of a bidirectional arc between two non-terminal graph
                nodes.

                Sets capacities of the arcs \c n1 -> \c n2 and \c n2 -> \c n1 to \c cap and
                \c rcap, respectively. It is usually assumed that you call this method only
                once for each connected pair of nodes for each call to FindMaxFlow().
                For algorithms supporting only undirected graphs the values of \c cap and 
                \c rcap should be the same.

			    @param[in] n1 First node index.
			    @param[in] n2 Second node index.
			    @param[in] cap Capacity of the arc \c n1 -> \c n2.
                @param[in] rcap Capacity of the arc \c n2 -> \c n1.

                @see Init.
		    */
		    virtual void SetArcCap(Size n1, Size n2, TCAP cap, TCAP rcap) = 0;

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

                @see Init.
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
