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
    Dinitz's maximum flow algorithm for general directed graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_GENERAL_DINITZ_H
#define GC_FLOW_GENERAL_DINITZ_H

#include "../../Core.h"
#include "../IMaxFlow.h"
#include "../../System/Collection/Array.h"

namespace Gc {
namespace Flow {
namespace General {
/** Implementation of Dinitz's maximum flow algorithm.

                Description of this algorithm can be found in:
                - Yefim %Dinitz: <em>Dinitz's Algorithm: The Original Version and 
                Even's Version </em>, Theoretical Computer Science, vol. 3895/2006,
                p. 218-240, March 2006, Springer

                Worst case time complexity of this algorithm is \f$ \mathcal{O}(n^2m) \f$.
                
                @remarks This implementation loosely follows the implementation 
                suggested by Cherkassky (BFS followed by DFS on a layered graph)
                that is briefly described in the aforementioned paper and
                can also be downloaded from Andrew Goldberg's site:
                http://www.avglab.com/andrew/soft.html (in the PRF package).

                @warning This implementation does store the arcs going to 
                terminals so its memory consumption can be much larger when
                compared to BoykovKolmogorov algorithm or PushRelabel methods.

                @tparam TFLOW %Data type used for the flow value.
                @tparam TCAP %Data type used for arc capacity values.
            */
template <class TFLOW, class TCAP>
class GC_DLL_EXPORT Dinitz
    : public IMaxFlow<TFLOW, TCAP, TCAP>
{
  private:
    // Forward declaration
    struct Node;

    /** Structure used for storing arcs in the network. */
    struct Arc
    {
        /** Residual capacity. */
        TCAP m_res_cap;
        /** Pointer to the next arc the same tail. */
        Arc * m_next;
        /** Pointer to the arc going in the opposite direction. */
        Arc * m_sister;
        /** Head node of this arc. */
        Node * m_head;
    };

    /** Structure used for storing nodes in the network. */
    struct Node
    {
        /** Pointer to the first arc going from this node. */
        Arc * m_first;
        /** Pointer to the next arc to be traversed in the DFS phase. */
        Arc * m_current;
        /** Parent node in the DFS tree. */
        Node * m_previous;
        /** Distance to the sink. */
        Size m_rank;
    };

    /** Node array. */
    System::Collection::Array<1, Node> m_node_list;
    /** Node heap. It has the same size as _node_list and can be used for 
                    storing temporary node structures like stacks or queues without \
                    the need to allocate/deallocate memory. */
    System::Collection::Array<1, Node *> m_node_heap;
    /** Arc array. */
    System::Collection::Array<1, Arc> m_arc_list;
    /** Arc count. */
    Size m_arcs = 0;
    /** Total flow */
    TFLOW m_flow;

    /** Current stage. Used to check correct method calling order. */
    Uint8 m_stage = 0;

  public:
    /** Constructor. */
    Dinitz() = default;

    /** Destructor. */
    ~Dinitz() override
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

  private:
    /** Add bidirectional arc between two nodes. 
                
                    Source is node 0, sink is node 1.

				    @param[in] n1 Node index.
				    @param[in] n2 Node index.
				    @param[in] cap \c n1 -> \c n2 arc capacity.
				    @param[in] rcap \c n2 -> \c n1 arc capacity.
			    */
    void AddArcInternal(Size n1, Size n2, TCAP cap, TCAP rcap);

    /** For each node find the shortest distance to the sink.

                    @return \c True if the source was reached, \c false otherwise.
                */
    bool BuildNodeRanks();

    /** Traverse the layered graph using depth first search.

                    @param[in] start The node where the traversing should start.
                    @return \c True if the sink was reached, \c false otherwise.
                */
    bool TraverseLayersUsingDFS(Node * start_node);
};
}
}
} // namespace Gc::Flow::General

#endif
