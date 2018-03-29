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

#include "../../Math/Basic.h"
#include "../../System/Format.h"
#include "../../System/IndexOutOfRangeException.h"
#include "../../System/OverflowException.h"
#include "../../System/InvalidOperationException.h"
#include "FordFulkerson.h"

namespace Gc
{
	namespace Flow
	{
        namespace General
        {
		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    void FordFulkerson<TFLOW,TCAP>::Init(Size nodes, Size arcs, Size src_arcs, Size snk_arcs)
		    {
			    Dispose();

                if (!nodes || !arcs || !src_arcs || !snk_arcs)
                {
                    throw System::ArgumentException(__FUNCTION__, __LINE__, "Empty graph.");
                }

                // Allocate memory
                m_node_list.Resize(nodes + 2); // +Source and sink
                m_node_heap.Resize(m_node_list.Elements());
                m_arc_list.Resize(2 * (arcs + src_arcs + snk_arcs));
                m_arcs = 0;
                m_timestamp = 0;
                m_flow = 0;
                m_stage = 1;
                m_max_arc_cap = 0;

                // Initialize node array
                m_node_list.ZeroFillMemory();
		    }

		    /***********************************************************************************/
                
		    template <class TFLOW, class TCAP>
            void FordFulkerson<TFLOW,TCAP>::SetCapacityScaling(double coef)
            {
                if (coef != 0 && coef <= 1)
                {
                    throw System::ArgumentException(__FUNCTION__, __LINE__,
                        "Invalid capacity scaling coefficient.");
                }

                m_cap_scale = coef;
            }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    void FordFulkerson<TFLOW,TCAP>::AddArcInternal(Size n1, Size n2, TCAP cap, TCAP rcap)
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

                m_max_arc_cap = Math::Max(m_max_arc_cap, cap, rcap);
		    }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    void FordFulkerson<TFLOW,TCAP>::SetTerminalArcCap(Size node, TCAP csrc, TCAP csnk)
            {
                // Adjust the capacities - only the absolute difference is relevant
                // The smaller capacity can be directly added to the flow and subtracted from both
                TCAP min_cap = Math::Min(csrc, csnk);
                            
                m_flow += min_cap;

                AddArcInternal (0, node + 2, csrc - min_cap, 0);
                AddArcInternal (1, node + 2, 0, csnk - min_cap);
            }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    bool FordFulkerson<TFLOW,TCAP>::DepthFirstSearch(Node *source, Node *sink, 
			    Size timestamp, TCAP scale, TCAP &maxrjcap)
		    {
			    Size stack_size = 1;
			    Node *head;
			    Arc *cur_arc;

                maxrjcap = 0;
			    m_node_heap[0] = source;

			    while (stack_size-- > 0)
			    {
				    // Investigate all arcs connected to this node
				    for (cur_arc = m_node_heap[stack_size]->m_first; cur_arc != NULL; cur_arc = cur_arc->m_next)
				    {
					    head = cur_arc->m_head;

					    // Can we augment across this arc?
					    if (head->m_ts < timestamp)
                        {
                            TCAP cap = cur_arc->m_res_cap;

                            if (cap > scale)
					        {
						        head->m_parent = cur_arc->m_sister;
                                head->m_ts = timestamp;

						        if (head == sink)
						        {
							        return true;
						        }

						        m_node_heap[stack_size++] = head;
					        }
                            else if (cap > maxrjcap)
                            {
                                maxrjcap = cap;
                            }
                        }
				    }
			    }

			    return false;
		    }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    void FordFulkerson<TFLOW,TCAP>::Augment(Node *source, Node *sink)
		    {
                // Find how much can we augment
                TCAP bottleneck = sink->m_parent->m_sister->m_res_cap;

                for (Arc *arc = sink->m_parent; arc != NULL; arc = arc->m_head->m_parent)
                {
                    if (arc->m_sister->m_res_cap < bottleneck)
                    {
                        bottleneck = arc->m_sister->m_res_cap;
                    }
                }

                // Augment
                for (Arc *arc = sink->m_parent; arc != NULL; arc = arc->m_head->m_parent)
                {
                    arc->m_res_cap += bottleneck;
                    arc->m_sister->m_res_cap -= bottleneck;
                }

                m_flow += bottleneck;
            }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    TFLOW FordFulkerson<TFLOW,TCAP>::FindMaxFlow()
		    {
                if (m_stage != 1)
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Call Init method before computing maximum flow.");
                }                

                TCAP maxrjcap;
                Node *source = &m_node_list[0], *sink = &m_node_list[1];
    			
			    m_timestamp = 0;

                if (!m_cap_scale)
                {
                    while (FindAugmentingPath(source, sink, ++m_timestamp, 0, maxrjcap))
                    {
                        Augment(source, sink);
                    }
                }
                else
                {
                    TCAP reqcap = (TCAP)(m_max_arc_cap / m_cap_scale);

                    while (true)
                    {
                        if (FindAugmentingPath(source, sink, ++m_timestamp, reqcap, maxrjcap))
                        {
                            Augment(source, sink);
                        }
                        else
                        {
                            if (!reqcap)
                            {
                                break;
                            }
                            else
                            {
                                reqcap = Math::Min (maxrjcap, (TCAP)(reqcap / m_cap_scale));
                            }
                        }
                    }
                }

                m_stage = 2;

			    return m_flow;
		    }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    Origin FordFulkerson<TFLOW,TCAP>::NodeOrigin(Size node) const
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

                // Nodes connected to the source can be easily recognized, because
                // their timestamp is the same as the global timestamp (they have
                // been visited during the last search)
			    return m_node_list[node + 2].m_ts == m_timestamp ? Source : Sink;
		    }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    void FordFulkerson<TFLOW,TCAP>::Dispose()
		    {
			    m_node_list.Dispose();
			    m_node_heap.Dispose();
			    m_arc_list.Dispose();
			    m_arcs = 0;
			    m_timestamp = 0;
                m_stage = 0;
		    }

		    /***********************************************************************************/

		    // Explicit instantiations
            /** @cond */
		    template class GC_DLL_EXPORT FordFulkerson<Int32,Int32>;
		    template class GC_DLL_EXPORT FordFulkerson<Float32,Float32>;
		    template class GC_DLL_EXPORT FordFulkerson<Float64,Float64>;
            /** @endcond */
        }
	}
}
