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
    Kohli's dynamic maximum flow algorithm for general directed graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_GENERAL_KOHLI_H
#define GC_FLOW_GENERAL_KOHLI_H

#include "../../Core.h"
#include "../IMaxFlow.h"
#include "../../System/Collection/Array.h"

namespace Gc {
namespace Flow {
namespace General {
/** Implementation of Kohli's dynamic maximum flow algorithm for general directed graphs.

                Description of this algorithm can be found in:
                - Pushmeet %Kohli, Philip H. S. Torr: <em>Dynamic Graph Cuts for Efficient 
                Inference in Markov Random Fields</em>, ICCV '05: Proceedings of the Tenth IEEE 
                International Conference on Computer Vision, p. 922-929, 2005

                It is strongly recommended to cite the aforementioned paper if you
                use this algorithm in your work. Theoretical worst case time complexity of
                this method is \f$ \mathcal{O}(mn^2f^*) \f$. However, it is usually
                significantly faster.
                
                This algorithm is basically the BoykovKolmogorov algorithm with the ability to 
                dynamically change the T-link and N-link capacities of the graph and reuse the 
                search trees from the previous computations. So when only a small part of the graph
                is changed the re-computation is extremely fast. As with the other algorithms
                our implementation should be exception safe. All memory is allocated in the Init()
                method and no other memory allocations are performed during the computation
                or tree recyclation.

                @tparam TFLOW %Data type used for the flow value.
                @tparam TTCAP %Data type used for terminal arc capacity values. This type 
                    must be signed (allow negative numbers).
                @tparam TCAP %Data type used for regular arc capacity values.

                @see BoykovKolmogorov
                
                @todo Currently dynamic change of the capacity is supported only for the
                    terminal arcs. Also note that the flow value returned in subsequent
                    calls to FindMaxFlow() after dynamic changes of the graph is currently 
                    incorrect (i.e., it is not the same as what would be returned by a
                    non-dynamic algorithm).
            */
template <class TFLOW, class TTCAP, class TCAP>
class GC_DLL_EXPORT Kohli
    : public IMaxFlow<TFLOW, TTCAP, TCAP>
{
  private:
    // Forward declaration
    struct Node;

    /** First-in-first-out queue structure */
    struct Queue
    {
        /** First node in the queue */
        Node * m_first;
        /** Last node in the queue */
        Node * m_last;
    };

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
        /** Next active/marked node. When this is not NULL, then node is active/marked. */
        Node * m_next_active;
        /** Next orphan node. When this is not NULL, then node is orphan. */
        Node * m_next_orphan;
        /** Residual capacity of the terminal arcs */
        TTCAP m_tr_cap;
        /** Timestamp when distance was computed. */
        Size m_ts;
        /** Distance to the terminal */
        Size m_dist;
        /** Node origin - source, sink or free */
        Uint8 m_origin;
        /** Flow going through the node. */
        TTCAP m_f_diff;
    };

    /** Node array. */
    System::Collection::Array<1, Node> m_node_list;
    /** Arc array. */
    System::Collection::Array<1, Arc> m_arc_list;
    /** Arc count. */
    Size m_arcs = 0;

    /** Total flow */
    TFLOW m_flow;
    /** Global time counter */
    Size m_time;

    /** Source tree active nodes. */
    Queue m_q_active;
    /** Queue of orphans. */
    Queue m_q_orphan;
    /** Queue of marked nodes. */
    Queue m_q_marked;

    /** Current stage. Used to check correct method calling order. */
    Uint8 m_stage = 0;

  public:
    /** Constructor */
    Kohli() = default;

    /** Destructor */
    ~Kohli() override = default;

    void Init(Size nodes, Size max_arcs, Size src_arcs, Size snk_arcs) override;

    void SetArcCap(Size n1, Size n2, TCAP cap, TCAP rcap) override;

    void SetTerminalArcCap(Size node, TTCAP csrc, TTCAP csnk) override;

    TFLOW FindMaxFlow() override;

    Origin NodeOrigin(Size node) const override;

    bool IsDynamic() const override
    {
        return true;
    }

    void Dispose() override;

  private:
    /** Init source and sink trees for first use. */
    void InitTrees();

    /** Recycle trees from previous run. */
    void RecycleTrees();

    /** Grow source and sink trees until an augmenting path is found.

                    @return Arc joining the source and sink trees.
                */
    Arc * GrowTrees();

    /** Augment flow across given path.

                    @param[in] joint Arc joining the source and sink trees.
                */
    void Augment(Arc * joint);

    /** Adopt orphan nodes. 
                
                    @param[in] timestamp Current timestamp.
                */
    void AdoptOrphans();

    /** Add node to the queue of active nodes.
                    
                    @param[in] node Node to be added.
                */
    void AddToActive(Node * node)
    {
        if (node->m_next_active == nullptr)
        {
            if (m_q_active.m_last != nullptr)
            {
                m_q_active.m_last->m_next_active = node;
            }
            else
            {
                m_q_active.m_first = node;
            }
            m_q_active.m_last = node;
            node->m_next_active = node; // Mark as active
        }
    }

    /** Add node to the queue of orphan nodes.
                    
                    @param[in] node Node to be added.
                */
    void AddToOrphans(Node * node)
    {
        if (node->m_next_orphan == nullptr)
        {
            if (m_q_orphan.m_last != nullptr)
            {
                m_q_orphan.m_last->m_next_orphan = node;
            }
            else
            {
                m_q_orphan.m_first = node;
            }
            m_q_orphan.m_last = node;
            node->m_next_orphan = node; // Mark as orphan
        }
    }
};
}
}
} // namespace Gc::Flow::General

#endif
