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
    Push-Relabel maximum flow algorithm for general directed graphs with 
    FIFO selection rule.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#include "../../../Math/Basic.h"
#include "../../../System/Format.h"
#include "../../../System/IndexOutOfRangeException.h"
#include "../../../System/OverflowException.h"
#include "../../../System/InvalidOperationException.h"
#include "Fifo.h"

namespace Gc
{
	namespace Flow
	{
        namespace General
        {
            namespace PushRelabel
            {
		        /***********************************************************************************/

		        template <class TFLOW, class TCAP>
		        void Fifo<TFLOW,TCAP>::Init(Size nodes, Size max_arcs, Size src_arcs, Size snk_arcs)
		        {
			        Dispose();

                    if (!nodes || !max_arcs || !src_arcs || !snk_arcs)
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "Empty graph.");
                    }

                    // Allocate memory
                    m_nodes = nodes + 2; // +Sink and source
                    m_node_list.Resize(nodes);
                    m_node_queue.Resize(m_node_list.Elements());
                    m_arc_list.Resize(2 * max_arcs);

                    m_fifo_first = NULL;
                    m_fifo_last = NULL;        				
                    m_arcs = 0;
                    m_flow = 0;
                    m_stage = 1;

                    // Initialize node array
                    m_node_list.ZeroFillMemory();
		        }

		        /***********************************************************************************/

		        template <class TFLOW, class TCAP>
		        void Fifo<TFLOW,TCAP>::SetArcCap(Size n1, Size n2, TCAP cap, TCAP rcap)
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
		        void Fifo<TFLOW,TCAP>::SetTerminalArcCap(Size node, TFLOW csrc, TFLOW csnk)
                {
                    if (m_stage != 1)
                    {
                        throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                            "Call Init method before adding arcs.");
                    }

                    // This constraint can be probably safely removed to allow
                    // minimization of more general energies in AlphaBetaSwap, etc.
                    /*if (csrc < 0 || csnk < 0)
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__,
                            "Arcs with negative capacity are not supported.");
                    }*/

                    if (node >= m_node_list.Elements())
                    {
                        throw System::IndexOutOfRangeException(__FUNCTION__, __LINE__, 
                            System::Format("Invalid node index. Nodes={0}, node={1}.")
                            << m_node_list.Elements() << node);
                    }

                    m_node_list[node].m_excess = csrc - csnk;
                    m_flow += Math::Min(csrc, csnk);
                }

		        /***********************************************************************************/

		        template <class TFLOW, class TCAP>
		        TFLOW Fifo<TFLOW,TCAP>::FindMaxFlow()
		        {
                    if (m_stage != 1)
                    {
                        throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                            "Call Init method before computing maximum flow.");
                    }          

                    Size rl_count = 0, rl_threshold = (Size) (m_glob_rel_freq * m_nodes);
                    Size next_layer_rank, min_rank;
                    Node *node, *head;
                    Arc *arc, *min_arc = NULL;
                    TCAP bottleneck;

                    GlobalUpdate();

                    while (m_fifo_first != NULL)
                    {
                        // Update FIFO
                        node = m_fifo_first;
                        m_fifo_first = m_fifo_first->m_fifo_succ;
                        if (m_fifo_first == NULL)
                        {
                            m_fifo_last = NULL;
                        }

                        // Perform DISCHARGE operation with current node
                        while (node->m_rank < m_nodes)
                        {
                            // PUSH excess
                            next_layer_rank = node->m_rank - 1;

                            for (arc = node->m_current; arc != NULL; arc = arc->m_next)
                            {
                                if (arc->m_res_cap > 0)
                                {
                                    head = arc->m_head;

                                    if (head->m_rank == next_layer_rank)
                                    {
                                        bottleneck = Math::Min(node->m_excess, arc->m_res_cap);

                                        arc->m_res_cap -= bottleneck;
                                        arc->m_sister->m_res_cap += bottleneck;

                                        if (head->m_excess <= 0)
                                        {
                                            if (head->m_excess + bottleneck > 0)
                                            {
                                                // Node becomes active
                                                if (m_fifo_first != NULL)
                                                {
                                                    m_fifo_last->m_fifo_succ = head;
                                                }
                                                else
                                                {
                                                    m_fifo_first = head;
                                                }

                                                m_fifo_last = head;
                                                head->m_fifo_succ = NULL;
                                                m_flow -= head->m_excess;
                                            }
                                            else
                                            {
                                                m_flow += bottleneck;
                                            }
                                        }

                                        head->m_excess += bottleneck;
                                        node->m_excess -= bottleneck;

                                        if (node->m_excess == 0)
                                        {
                                            break;
                                        }
                                    }
                                }
                            }

                            if (arc != NULL)
                            {
                                // All excess pushed away, the node becomes inactive
                                node->m_current = arc;
                                break;
                            }

                            // Node is still active so RELABEL it
                            node->m_rank = min_rank = m_nodes;

                            for (arc = node->m_first; arc != NULL; arc = arc->m_next)
                            {
                                if (arc->m_res_cap > 0 && arc->m_head->m_rank < min_rank)
                                {
                                    min_rank = arc->m_head->m_rank;
                                    min_arc = arc;
                                }
                            }

                            if (++min_rank < m_nodes)
                            {
                                node->m_rank = min_rank;
                                node->m_current = min_arc;
                            }

                            rl_count++;
                        }

                        // Check if global relabeling should be done
                        if (rl_count > rl_threshold && m_fifo_first != NULL)
                        {
                            GlobalUpdate();
                            rl_count = 0;
                        }
                    }

                    // Find nodes that are connected to the sink after the last step.
                    // This is IMPORTANT, because it allows us to find which nodes are
                    // connected to the source and which to the sink.
                    GlobalUpdate();

                    m_stage = 2;

                    return m_flow;
		        }

		        /***********************************************************************************/

		        template <class TFLOW, class TCAP>
		        void Fifo<TFLOW,TCAP>::GlobalUpdate()
		        {
                    Node **q_read, **q_write, *node, *head;
                    Arc *arc;

                    // Initialize queue and active node fifo pointers
                    q_write = q_read = m_node_queue.Begin();
                    m_fifo_first = m_fifo_last = NULL;

                    // Set initial node ranks and add nodes connected to sink to the queue
                    for (node = m_node_list.Begin(); node != m_node_list.End(); ++node)
                    {
                        if (node->m_excess < 0)
                        {
                            node->m_rank = 1;
                            node->m_current = node->m_first;
                            *q_write = node;
                            q_write++;
                        }
                        else
                        {
                            node->m_rank = m_nodes;
                        }
                    }

                    // Breadth first search from the sink
                    while (q_read < q_write)
                    {
                        node = *q_read;
                        q_read++;

                        for (arc = node->m_first; arc != NULL; arc = arc->m_next)
                        {
                            head = arc->m_head;

                            if (head->m_rank == m_nodes && arc->m_sister->m_res_cap > 0)
                            {
                                head->m_rank = node->m_rank + 1;
                                head->m_current = head->m_first;

                                if (head->m_excess > 0)
                                {
                                    head->m_fifo_succ = m_fifo_first;
                                    if (m_fifo_first == NULL)
                                    {
                                        m_fifo_last = head;
                                    }
                                    m_fifo_first = head;
                                }

                                *q_write = head;
                                q_write++;
                            }
                        }
                    }
                }

		        /***********************************************************************************/

		        template <class TFLOW, class TCAP>
		        Origin Fifo<TFLOW,TCAP>::NodeOrigin(Size node) const
		        {
                    if (m_stage != 2)
                    {
                        throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                            "Compute the maximum flow first before calling this method.");
                    }                

                    if (node >= m_node_list.Elements())
                    {
                        throw System::IndexOutOfRangeException(__FUNCTION__, __LINE__, 
                            System::Format("Invalid node index. Nodes={0}, node={1}.")
                            << m_node_list.Elements() << node);
                    }

                    // Nodes connected to the sink are those with rank less than infinity (=m_nodes),
                    // i.e., those that were reached during the final global update.
                    return m_node_list[node].m_rank < m_nodes ? Sink : Source;
		        }

		        /***********************************************************************************/

		        template <class TFLOW, class TCAP>
		        void Fifo<TFLOW,TCAP>::Dispose()
		        {
			        m_node_list.Dispose();
                    m_node_queue.Dispose();
			        m_arc_list.Dispose();
			        m_nodes = 0;
			        m_arcs = 0;
                    m_fifo_first = NULL;
                    m_fifo_last = NULL;
                    m_stage = 0;
		        }

		        /***********************************************************************************/

		        // Explicit instantiations
                /** @cond */
		        template class GC_DLL_EXPORT Fifo<Int32,Int32>;
		        template class GC_DLL_EXPORT Fifo<Float32,Float32>;
		        template class GC_DLL_EXPORT Fifo<Float64,Float64>;
                /** @endcond */
            }
        }
	}
}
