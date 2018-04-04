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
    Boykov/Kolmogorov maximum flow algorithm for general directed graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#include "../../Math/Basic.h"
#include "../../System/Format.h"
#include "../../System/IndexOutOfRangeException.h"
#include "../../System/OverflowException.h"
#include "../../System/InvalidOperationException.h"
#include "BoykovKolmogorov.h"

namespace Gc
{
	namespace Flow
	{
        namespace General
        {
		    /***********************************************************************************/

		    template <class TFLOW, class TTCAP, class TCAP>
		    void BoykovKolmogorov<TFLOW,TTCAP,TCAP>::Init(Size nodes, Size max_arcs, 
                Size src_arcs, Size snk_arcs)
		    {
			    Dispose();

                if (!nodes || !max_arcs || !src_arcs || !snk_arcs)
                {
                    throw System::ArgumentException(__FUNCTION__, __LINE__, "Empty graph.");
                }

                // Allocate memory
                m_node_list.Resize(nodes);
                m_arc_list.Resize(2 * max_arcs);

                m_arcs = 0;
                m_flow = 0;
                m_stage = 1;

                // Initialize the node array
                m_node_list.ZeroFillMemory();
		    }

		    /***********************************************************************************/

		    template <class TFLOW, class TTCAP, class TCAP>
		    void BoykovKolmogorov<TFLOW,TTCAP,TCAP>::SetArcCap(Size n1, Size n2, TCAP cap, TCAP rcap)
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
                        System::Format("Arc count overflow. Max arcs={0}.") << m_arc_list.Elements());
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

		    template <class TFLOW, class TTCAP, class TCAP>
		    void BoykovKolmogorov<TFLOW,TTCAP,TCAP>::SetTerminalArcCap(Size node, TTCAP csrc, TTCAP csnk)
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

                m_node_list[node].m_tr_cap = csrc - csnk;
                m_flow += Math::Min(csrc, csnk);
            }

		    /***********************************************************************************/

		    template <class TFLOW, class TTCAP, class TCAP>
		    TFLOW BoykovKolmogorov<TFLOW,TTCAP,TCAP>::FindMaxFlow()
		    {
                if (m_stage != 1)
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Call Init method before computing maximum flow.");
                }

                Node *node;
                Arc *arc;
                
                // Initialization
                m_time = 0;
                m_q_active.m_first = m_q_active.m_last = NULL;
                m_q_orphan.m_first = m_q_orphan.m_last = NULL;

                // Find nodes connected to the source and to the sink
                for (node = m_node_list.Begin(); node != m_node_list.End(); ++node)
                {
                    if (node->m_tr_cap > 0)
                    {
                        node->m_dist = 1;
                        node->m_origin = Source;
                        AddToActive(node);
                    }
                    else if (node->m_tr_cap < 0)
                    {
                        node->m_dist = 1;
                        node->m_origin = Sink;
                        AddToActive(node);
                    } 
                    else
                    {
                        node->m_origin = Free;
                    }
                }

                // Main loop
                while (m_q_active.m_first != NULL)
                {
                    // Grow trees and find augmenting path
                    arc = GrowTrees();
                    if (arc)
                    {
                        // Augment and adopth orphans
                        Augment(arc);
                        AdoptOrphans();
                    }
                }

                m_stage = 2;

			    return m_flow;
		    }

		    /***********************************************************************************/

		    template <class TFLOW, class TTCAP, class TCAP>
		    typename BoykovKolmogorov<TFLOW,TTCAP,TCAP>::Arc *BoykovKolmogorov<TFLOW,TTCAP,TCAP>::GrowTrees()
            {
                TCAP cap;
                Arc *arc;
                Node *node, *head;

                while (m_q_active.m_first != NULL)
                {
                    // Increase time
                    m_time++;

                    // Pop node from the queue of active nodes
                    node = m_q_active.m_first;

                    // Grow tree
                    if (node->m_origin != Free)
                    {
                        for (arc = node->m_first; arc != NULL; arc = arc->m_next)
                        {
                            cap = (node->m_origin == Source) ? arc->m_res_cap : arc->m_sister->m_res_cap;

                            if (cap > 0)
                            {
                                head = arc->m_head;
                                if (head->m_origin == Free)
                                {
                                    head->m_origin = node->m_origin;
                                    head->m_parent = arc->m_sister;
                                    head->m_dist = node->m_dist + 1;
                                    head->m_ts = node->m_ts;
                                    AddToActive(head);
                                }
                                else if (node->m_origin != head->m_origin)
                                {
                                    return (node->m_origin == Source) ? arc : arc->m_sister;
                                }
                                else if (head->m_ts <= node->m_ts && head->m_dist > node->m_dist + 1) 
                                {
                                    // Heuristic, trying to produce shorter paths.
                                    // Remark: > in m_dist comparison seems to perform better than >=
                                    head->m_parent = arc->m_sister;
                                    head->m_dist = node->m_dist + 1;
                                    head->m_ts = node->m_ts;
                                }
                            }
                        }                    
                    }

                    // Remove the node from the queue of active nodes
                    if (m_q_active.m_first == m_q_active.m_last)
                    {
                        m_q_active.m_first = NULL;
                        m_q_active.m_last = NULL;
                    }
                    else
                    {
                        m_q_active.m_first = node->m_next_active;
                    }

                    // Mark node as inactive
                    node->m_next_active = NULL;

                }

                return NULL;
            }

		    /***********************************************************************************/

		    template <class TFLOW, class TTCAP, class TCAP>
		    void BoykovKolmogorov<TFLOW,TTCAP,TCAP>::Augment(Arc *joint)
            {
                TCAP bottleneck = joint->m_res_cap;
                Arc *arc = NULL;
                Node *node;

                // Find bottleneck in the source tree
                for (node = joint->m_sister->m_head; node->m_dist > 1; node = arc->m_head)
                {
                    arc = node->m_parent;
                    if (arc->m_sister->m_res_cap < bottleneck)
                    {
                        bottleneck = arc->m_sister->m_res_cap;
                    }
                }
                if (node->m_tr_cap < bottleneck)
                {
                    bottleneck = node->m_tr_cap;
                }

                // Find bottleneck in the sink tree
                for (node = joint->m_head; node->m_dist > 1; node = arc->m_head)
                {
                    arc = node->m_parent;
                    if (arc->m_res_cap < bottleneck)
                    {
                        bottleneck = arc->m_res_cap;
                    }
                }
                if (-node->m_tr_cap < bottleneck)
                {
                    bottleneck = -node->m_tr_cap;
                }

                // Augmentation
                m_flow += bottleneck;

                joint->m_res_cap -= bottleneck;
                joint->m_sister->m_res_cap += bottleneck;

                // Augment in the source tree
                for (node = joint->m_sister->m_head; node->m_dist > 1; node = arc->m_head)
                {
                    arc = node->m_parent;
                    arc->m_res_cap += bottleneck;
                    if ((arc->m_sister->m_res_cap -= bottleneck) == 0)
                    {
                        AddToOrphans(node);
                    }
                }
                if ((node->m_tr_cap -= bottleneck) == 0)
                {
                    AddToOrphans(node);
                }

                // Augment in the sink tree
                for (node = joint->m_head; node->m_dist > 1; node = arc->m_head)
                {
                    arc = node->m_parent;
                    arc->m_sister->m_res_cap += bottleneck;
                    if ((arc->m_res_cap -= bottleneck) == 0)
                    {
                        AddToOrphans(node);
                    }
                }
                if ((node->m_tr_cap += bottleneck) == 0)
                {
                    AddToOrphans(node);                
                }
            }

		    /***********************************************************************************/

            template <class TFLOW, class TTCAP, class TCAP>
		    void BoykovKolmogorov<TFLOW,TTCAP,TCAP>::AdoptOrphans()
            {
                Node *node, *head;
                Size d, d_min = 0;
                Arc *arc, *arc_min;
                TCAP cap;

                // Increase time counter
                m_time++;

                while (m_q_orphan.m_first != NULL)
                {
                    // Pop orphan node
                    node = m_q_orphan.m_first;
                    if (m_q_orphan.m_first == m_q_orphan.m_last)
                    {
                        m_q_orphan.m_first = NULL;
                        m_q_orphan.m_last = NULL;
                    }
                    else
                    {
                        m_q_orphan.m_first = node->m_next_orphan;
                    }

                    // Adoption
                    arc_min = NULL;

                    for (arc = node->m_first; arc != NULL; arc = arc->m_next)
                    {
                        cap = (node->m_origin == Source) ? arc->m_sister->m_res_cap : arc->m_res_cap;

                        if (cap > 0)
                        {
                            head = arc->m_head;
                            if (node->m_origin == head->m_origin)
                            {
                                // Find the origin of given node
                                d = 1;
                                while (head->m_dist > 1 && head->m_next_orphan == NULL && head->m_ts < m_time)
                                {
                                    head = head->m_parent->m_head;
                                    d++;
                                }

                                if (head->m_next_orphan == NULL)
                                {
                                    // Ok, originates from the same tree
                                    d += head->m_dist;
                                    if (d < d_min || arc_min == NULL)
                                    {
                                        d_min = d;
                                        arc_min = arc;
                                    }

                                    // Set marks along the path
                                    for (head = arc->m_head; 
                                         head->m_ts != m_time && head->m_dist > 1; 
                                         head = head->m_parent->m_head)
                                    {
                                        d--;
                                        head->m_ts = m_time;
                                        head->m_dist = d;
                                    }
                                }
                            }
                        }
                    }

                    // Remove from orphans
                    node->m_next_orphan = NULL;

                    if (arc_min != NULL)
                    {
                        // Parent was found in the same tree
                        node->m_parent = arc_min;
                        node->m_dist = d_min;
                        node->m_ts = m_time;
                    }
                    else
                    {
                        // No parent found
                        for (arc = node->m_first; arc != NULL; arc = arc->m_next)
                        {
                            head = arc->m_head;
                            if (node->m_origin == head->m_origin)
                            {
                                if (head->m_dist > 1 && head->m_parent->m_head == node)
                                {
                                    AddToOrphans(head);
                                }
                                cap = (node->m_origin == Source) ? arc->m_sister->m_res_cap : arc->m_res_cap;
                                if (cap > 0)
                                {
                                    AddToActive(head);
                                }
                            }
                        }

                        // Add node to free nodes, leave only active flag if present
                        node->m_origin = Free;
                    }
                }
            }

		    /***********************************************************************************/

		    template <class TFLOW, class TTCAP, class TCAP>
		    Origin BoykovKolmogorov<TFLOW,TTCAP,TCAP>::NodeOrigin(Size node) const
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

                return (Origin)m_node_list[node].m_origin;
		    }

		    /***********************************************************************************/

		    template <class TFLOW, class TTCAP, class TCAP>
		    void BoykovKolmogorov<TFLOW,TTCAP,TCAP>::Dispose()
		    {
			    m_node_list.Dispose();
			    m_arc_list.Dispose();
			    m_arcs = 0;
                m_stage = 0;
		    }

		    /***********************************************************************************/

		    // Explicit instantiations
            /** @cond */
		    template class GC_DLL_EXPORT BoykovKolmogorov<Int32,Int32,Int32>;
		    template class GC_DLL_EXPORT BoykovKolmogorov<Float32,Float32,Float32>;
		    template class GC_DLL_EXPORT BoykovKolmogorov<Float64,Float64,Float64>;
            /** @endcond */
        }
	}
}
