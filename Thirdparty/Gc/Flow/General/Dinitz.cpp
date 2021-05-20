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

#include "../../Math/Basic.h"
#include "../../System/Format.h"
#include "../../System/IndexOutOfRangeException.h"
#include "../../System/OverflowException.h"
#include "../../System/InvalidOperationException.h"
#include "Dinitz.h"

namespace Gc {
namespace Flow {
namespace General {
/***********************************************************************************/

template <class TFLOW, class TCAP>
void Dinitz<TFLOW, TCAP>::Init(Size nodes, Size max_arcs, Size src_arcs, Size snk_arcs)
{
    Dispose();

    if (!nodes || !max_arcs || !src_arcs || !snk_arcs)
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__, "Empty graph.");
    }

    // Allocate memory
    m_node_list.Resize(nodes + 2); // +Source and sink
    m_node_heap.Resize(m_node_list.Elements());
    m_arc_list.Resize(2 * (max_arcs + src_arcs + snk_arcs));
    m_arcs = 0;

    m_flow = 0;
    m_stage = 1;

    // Initialize node array
    m_node_list.ZeroFillMemory();
}

/***********************************************************************************/

template <class TFLOW, class TCAP>
void Dinitz<TFLOW, TCAP>::AddArcInternal(Size n1, Size n2, TCAP cap, TCAP rcap)
{
    if (m_stage != 1)
    {
        throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                                                "Call Init method before adding arcs.");
    }

    if (!cap && !rcap)
    {
        return; // Zero capacity arcs can be ignored
    }

    if (cap < 0 || rcap < 0)
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__,
                                        "Arcs with negative capacity are not supported.");
    }

    if (n1 >= m_node_list.Elements() || n2 >= m_node_list.Elements())
    {
        throw System::IndexOutOfRangeException(__FUNCTION__, __LINE__,
                                               System::Format("Invalid node index. Nodes={0}, n1={1}, n2={2}.")
                                                   << m_node_list.Elements() << n1 << n2);
    }

    if (m_arcs >= m_arc_list.Elements())
    {
        throw System::OverflowException(__FUNCTION__, __LINE__,
                                        System::Format("Arc count overflow. Max arcs={0}.")
                                            << m_arc_list.Elements());
    }

    Arc *a1 = &m_arc_list[m_arcs++], *a2 = &m_arc_list[m_arcs++];
    Node *n1p = &m_node_list[n1], *n2p = &m_node_list[n2];

    a1->m_next = n1p->m_first;
    a1->m_sister = a2;
    a1->m_res_cap = cap;
    a1->m_head = n2p;

    a2->m_next = n2p->m_first;
    a2->m_sister = a1;
    a2->m_res_cap = rcap;
    a2->m_head = n1p;

    n1p->m_first = a1;
    n2p->m_first = a2;
}

/***********************************************************************************/

template <class TFLOW, class TCAP>
void Dinitz<TFLOW, TCAP>::SetTerminalArcCap(Size node, TCAP csrc, TCAP csnk)
{
    // Adjust the capacities - only the absolute difference is relevant
    // The smaller capacity can be directly added to the flow and subtracted from both
    TCAP min_cap = Math::Min(csrc, csnk);

    m_flow += min_cap;

    AddArcInternal(0, node + 2, csrc - min_cap, 0);
    AddArcInternal(1, node + 2, 0, csnk - min_cap);
}

/***********************************************************************************/

template <class TFLOW, class TCAP>
TFLOW Dinitz<TFLOW, TCAP>::FindMaxFlow()
{
    if (m_stage != 1)
    {
        throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                                                "Call Init method before computing maximum flow.");
    }

    Node *node, *bn_node, *source = &m_node_list[0], *sink = &m_node_list[1];
    Arc * arc;
    TCAP bottleneck;

    while (BuildNodeRanks())
    {
        // Run the layered depth first search phase
        bn_node = source;

        while (TraverseLayersUsingDFS(bn_node))
        {
            // Find bottleneck for current path
            arc = source->m_current;
            bottleneck = arc->m_res_cap;
            bn_node = source;
            node = arc->m_head;

            while (node != sink)
            {
                arc = node->m_current;
                if (arc->m_res_cap < bottleneck)
                {
                    bottleneck = arc->m_res_cap;
                    bn_node = node;
                }
                node = arc->m_head;
            }

            // Augment
            for (node = source; node != sink; node = arc->m_head)
            {
                arc = node->m_current;
                arc->m_res_cap -= bottleneck;
                arc->m_sister->m_res_cap += bottleneck;
            }

            m_flow += bottleneck;
        }
    }

    m_stage = 2;

    return m_flow;
}

/***********************************************************************************/

template <class TFLOW, class TCAP>
bool Dinitz<TFLOW, TCAP>::BuildNodeRanks()
{
    Node *cur_node, *head;
    Node *source = &m_node_list[0], *sink = &m_node_list[1];
    Arc * arc;

    // Reset node ranks
    for (Size i = 0; i < m_node_list.Elements(); i++)
    {
        m_node_list[i].m_rank = m_node_list.Elements();
    }

    // Run BFS from the sink and build layers
    sink->m_rank = 0;
    m_node_heap[0] = sink;
    Node **fifo_read, **fifo_write = &m_node_heap[1];

    for (fifo_read = m_node_heap.Begin(); fifo_read < fifo_write; fifo_read++)
    {
        cur_node = *fifo_read;

        for (arc = cur_node->m_first; arc != nullptr; arc = arc->m_next)
        {
            head = arc->m_head;

            if (head->m_rank == m_node_list.Elements() && arc->m_sister->m_res_cap > 0)
            {
                head->m_rank = cur_node->m_rank + 1;
                head->m_current = head->m_first;

                if (head == source)
                {
                    return true;
                }

                *fifo_write = head;
                fifo_write++;
            }
        }
    }

    return false;
}

/***********************************************************************************/

template <class TFLOW, class TCAP>
bool Dinitz<TFLOW, TCAP>::TraverseLayersUsingDFS(Node * start_node)
{
    Node *node = start_node, *sink = &m_node_list[1];
    Arc * arc;
    Size s_rank;

    while (node != nullptr && node != sink)
    {
        s_rank = node->m_rank - 1;

        // Find arc to the underlying layer
        for (arc = node->m_current; arc != nullptr; arc = arc->m_next)
        {
            if (arc->m_head->m_rank == s_rank && arc->m_res_cap > 0)
            {
                break;
            }
        }

        node->m_current = arc;

        if (arc != nullptr)
        {
            // Step forward
            arc->m_head->m_previous = node;
            node = arc->m_head;
        }
        else
        {
            // Step back
            node = node->m_previous;

            if (node != nullptr)
            {
                node->m_current = node->m_current->m_next;
            }
        }
    }

    return (node == sink);
}

/***********************************************************************************/

template <class TFLOW, class TCAP>
Origin Dinitz<TFLOW, TCAP>::NodeOrigin(Size node) const
{
    if (m_stage != 2)
    {
        throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                                                "Compute the maximum flow first before calling this method.");
    }

    if (node + 2 >= m_node_list.Elements())
    {
        throw System::IndexOutOfRangeException(__FUNCTION__, __LINE__,
                                               System::Format("Invalid node index. Nodes={0}, node={1}.")
                                                   << m_node_list.Elements() << node + 2);
    }

    // Nodes connected to the sink can be easily recognized, because
    // their rank (distance to the sink) is less than infinity (=m_nodes)
    return m_node_list[node + 2].m_rank < m_node_list.Elements() ? Sink : Source;
}

/***********************************************************************************/

template <class TFLOW, class TCAP>
void Dinitz<TFLOW, TCAP>::Dispose()
{
    m_node_list.Dispose();
    m_node_heap.Dispose();
    m_arc_list.Dispose();
    m_arcs = 0;
    m_stage = 0;
}

/***********************************************************************************/

// Explicit instantiations
/** @cond */
template class GC_DLL_EXPORT Dinitz<Int32, Int32>;
template class GC_DLL_EXPORT Dinitz<Float32, Float32>;
template class GC_DLL_EXPORT Dinitz<Float64, Float64>;
/** @endcond */
}
}
} // namespace Gc::Flow::General
