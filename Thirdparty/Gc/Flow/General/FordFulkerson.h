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
    Ford/Fulkerson maximum flow algorithm for general directed graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_GENERAL_FORDFULKERSON_H
#define GC_FLOW_GENERAL_FORDFULKERSON_H

#include "../../Core.h"
#include "../IMaxFlow.h"
#include "../../System/Collection/Array.h"

namespace Gc {
namespace Flow {
/** Maximum flow algorithms for general directed graphs. */
namespace General {
/** Implementation of the Ford/Fulkerson maximum flow algorithm for general 
                directed graphs.

                Description of this algorithm can be found in:
                - L. R. Ford, D. R. Fulkerson: <em>Maximal flow through a network</em>, 
                Canadian Journal of Mathematics, vol. 8, pages 399-404, 1956

                In fact this algorithm is just a framework for maximum flow
                algorithms, in which an augmenting path is found and then a
                flow is send along this path saturating the edge with the smallest
                capacity. It does not state how to find the augmenting path.
                Thus, the complexity of the generic algorithm in integer case 
                is \f$ \mathcal{O}(mf^*) \f$. In case of irrational numbers,
                the algorithm is not guaranteed to stop. Depending on the strategy 
                for searching the augmenting paths better results can
                be obtained, as for example in the EdmondsKarp algorithm,
                which is nothing but a specialization of the FordFulkerson method. 
                
                To distinguish this implementation from the EdmondsKarp algorithm, 
                depth-first search is used to find the augmenting paths.
                Such strategy obviously leads to extremely slow computation
                and therefore is not recommended for use. Other strategies can
                be simply implemented by deriving from this class and
                overriding the FindAugmentingPath() method.

                @warning This implementation does store the arcs going to 
                terminals so its memory consumption can be much larger when
                compared to BoykovKolmogorov algorithm or PushRelabel::Fifo and
                PushRelabel::HighestLevel methods.

                @tparam TFLOW %Data type used for the flow value.
                @tparam TCAP %Data type used for arc capacity values.

                @see EdmondsKarp
		    */
template <class TFLOW, class TCAP>
class GC_DLL_EXPORT FordFulkerson
    : public IMaxFlow<TFLOW, TCAP, TCAP>
{
  protected:
    // Forward declaration
    struct Node;

    /** Structure used for storing arcs in the network. */
    struct Arc
    {
        /** Residual capacity. */
        TCAP m_res_cap;
        /** Pointer to the next arc with the same tail. */
        Arc * m_next;
        /** Pointer to the arc going in the opposite direction. */
        Arc * m_sister;
        /** Pointer to the head node. */
        Node * m_head;
    };

    /** Structure used for storing nodes in the network. */
    struct Node
    {
        /** Pointer to the first arc going from this node. */
        Arc * m_first;
        /** Pointer to the arc going to this node from its parent node. */
        Arc * m_parent;
        /** Timestamp. */
        Size m_ts;
    };

    /** Node array. */
    System::Collection::Array<1, Node> m_node_list;
    /** Node heap. It has the same size as m_node_list and can be used 
                    for storing temporary node structures like stack, queue or list 
                    without the need to allocate/deallocate memory. */
    System::Collection::Array<1, Node *> m_node_heap;
    /** Arc array. */
    System::Collection::Array<1, Arc> m_arc_list;
    /** Arc count. */
    Size m_arcs = 0;
    /** Global timestamp. */
    Size m_timestamp = 0;
    /** Total flow */
    TFLOW m_flow;

    /** Highest arc capacity. */
    TCAP m_max_arc_cap;
    /** Capacity scaling coefficient. */
    double m_cap_scale = 0;
    /** Current stage. Used to check correct method calling order. */
    Uint8 m_stage = 0;

  public:
    /** Constructor. */
    FordFulkerson() = default;

    /** Destructor. */
    ~FordFulkerson() override
    {
        Dispose();
    }

    void Init(Size nodes, Size max_arcs, Size src_arcs, Size snk_arcs) override;

    void SetArcCap(Size n1, Size n2, TCAP cap, TCAP rcap) override
    {
        AddArcInternal(n1 + 2, n2 + 2, cap, rcap);
    }

    void SetTerminalArcCap(Size node, TCAP csrc, TCAP csnk) override;

    TFLOW FindMaxFlow() override;

    Origin NodeOrigin(Size node) const override;

    void Dispose() override;

    /** Enable capacity scaling and set scaling coefficient. 
                
                    @param[in] coef Capacity scaling coefficient. Has to be 
                        bigger than 1. Set to zero to disable capacity scaling.
                */
    void SetCapacityScaling(double coef = 2.0f);

  private:
    /** Find augmenting path from the source to the sink using a 
                    particular method.

				    @param[in] source Pointer to the source node.
				    @param[in] sink Pointer to the sink node.
				    @param[in] timestamp Current timestamp.
				    @param[in] scale Minimal requested path bottleneck (capacity scaling threshold).
                    @param[out] maxrjcap Maximum capacity arc among those rejected due
                        to the capacity scaling threshold (this value helps to estabilish
                        a new threshold).
				    @return \c True if augmenting path exists, \c false otherwise.
			    */
    virtual bool FindAugmentingPath(Node * source, Node * sink,
                                    Size timestamp, TCAP scale, TCAP & maxrjcap)
    {
        return DepthFirstSearch(source, sink, timestamp, scale, maxrjcap);
    }

    /** Find augmenting path using depth-first search.

				    @param[in] source Pointer to the source node.
				    @param[in] sink Pointer to the sink node.
				    @param[in] timestamp Current timestamp.
				    @param[in] scale Minimal path bottleneck (capacity scaling threshold).
                    @param[out] maxrjcap Maximum capacity arc among those rejected due
                        to the capacity scaling threshold (this value helps to estabilish
                        a new threshold).
                    @return \c True if augmenting path exists, \c false otherwise. 
			    */
    bool DepthFirstSearch(Node * source, Node * sink, Size timestamp, TCAP scale, TCAP & maxrjcap);

    /** Add bidirectional arc between two nodes. 
                
                    Source is node 0, sink is node 1.

				    @param[in] n1 Node index.
				    @param[in] n2 Node index.
				    @param[in] cap \c n1 -> \c n2 arc capacity.
				    @param[in] rcap \c n2 -> \c n1 arc capacity.
			    */
    void AddArcInternal(Size n1, Size n2, TCAP cap, TCAP rcap);

    /** Augment accross the current path.

                    @param[in] source Source node.
                    @param[in] sink Sink node.
                */
    void Augment(Node * source, Node * sink);
};
} // namespace General
}
} // namespace Gc::Flow

#endif
