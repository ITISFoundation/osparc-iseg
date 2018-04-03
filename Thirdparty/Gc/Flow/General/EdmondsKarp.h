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
    Edmonds/Karp maximum flow algorithm for general directed graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_GENERAL_EDMONDSKARP_H
#define GC_FLOW_GENERAL_EDMONDSKARP_H

#include "../../Core.h"
#include "FordFulkerson.h"

namespace Gc
{
	namespace Flow
	{
        namespace General
        {
            /** Implementation of Edmonds/Karp maximum flow algorithm on general directed graphs.

                Description of this algorithm can be found in:
                - Jack Edmonds, Richard M. Karp: <em>Theoretical Improvements in 
                Algorithmic Efficiency for Network %Flow Problems</em>, 
                Journal of the Association for Computing Machinery (ACM), vol. 2,
                pages 248-264, April 1972

                Basically, this is just the FordFulkerson algorithm with a
                different strategy for finding the augmenting paths. One can
                choose between shortest path strategy which leads to
                \f$ \mathcal{O}(nm^2) \f$ time complexity and fattest path
                strategy resulting in \f$ \mathcal{O}(m^2 \log m \log f^*) \f$
                time complexity.

                @warning This implementation does store the arcs going to 
                terminals so its memory consumption can be much larger when
                compared to BoykovKolmogorov algorithm or PushRelabelFifo and
                PushRelabelHighestLevel methods.

                @tparam TFLOW %Data type used for the flow value.
                @tparam TCAP %Data type used for arc capacity values.
            */
		    template <class TFLOW, class TCAP>
		    class GC_DLL_EXPORT EdmondsKarp 
			    : public FordFulkerson<TFLOW,TCAP>
		    {
		    public:
                /** Strategy for finding the augmenting paths in the residual graph. */
			    enum Strategy
			    {
                    /** Shortest path method. */
				    ShortestPathMethod,
                    /** Fattest path method. */                    
				    FattestPathMethod
			    };

            protected:
                using FordFulkerson<TFLOW,TCAP>::m_node_heap;

                /** Current path finding strategy. */
			    Strategy m_strategy;

		    public:
			    /** Constructor. */
			    EdmondsKarp()
				    : FordFulkerson<TFLOW,TCAP>(), m_strategy(ShortestPathMethod)
			    {}

			    /** Destructor. */
			    virtual ~EdmondsKarp()
			    {}

			    /** Select path searching strategy.

                    @param[in] strategy Selected strategy.

				    @remarks It doesn't make sense to combine fattest path search
				    and capacity scaling. This combination will result into
				    performance degradation.
			    */
			    void SelectStrategy (Strategy strategy)
			    {
				    m_strategy = strategy;
			    }

                /** Get currently selected path searching strategy.
                    
                    @return Currently selected strategy.
                */
                Strategy CurrentStrategy() const
                {
                    return m_strategy;
                }

		    private:
			    virtual bool FindAugmentingPath (
                    typename FordFulkerson<TFLOW,TCAP>::Node *source, 
                    typename FordFulkerson<TFLOW,TCAP>::Node *sink, Size timestamp, 
                    TCAP scale, TCAP &maxrjcap);

			    /** Find the shortest path from the source to the sink.
                
                    Implemented using breadth first search.

				    @param[in] source Pointer to the source node.
				    @param[in] sink Pointer to the sink node.
				    @param[in] timestamp Current timestamp.
				    @param[in] scale Minimal requested path bottleneck (capacity scaling threshold).
                    @param[out] maxrjcap Maximum capacity arc among those rejected due
                        to the capacity scaling threshold (this value helps to estabilish
                        a new threshold).
				    @return \c True if augmenting path exists, \c false otherwise .
			    */
			    bool BreadthFirstSearch (typename FordFulkerson<TFLOW,TCAP>::Node *source, 
                    typename FordFulkerson<TFLOW,TCAP>::Node *sink, Size timestamp, 
                    TCAP scale, TCAP &maxrjcap);

			    /** Find the fattest path from source to sink.

                    This methods implements a variant of the Jarnik/Prim minimum 
                    spanning tree algorithm, starting with the source node and adding 
                    edges until the sink is reached.

				    @param[in] source Pointer to the source node.
				    @param[in] sink Pointer to the sink node.
				    @param[in] timestamp Current timestamp.
				    @return \c True if augmenting path exists, \c false otherwise.
			    */
			    bool FattestPathSearch (typename FordFulkerson<TFLOW,TCAP>::Node *source, 
                    typename FordFulkerson<TFLOW,TCAP>::Node *sink, Size timestamp);
		    };
        }
	}
}

#endif
