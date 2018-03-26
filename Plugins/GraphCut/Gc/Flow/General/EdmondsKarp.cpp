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

#include "../../System/InvalidOperationException.h"
#include "EdmondsKarp.h"

namespace Gc
{
	namespace Flow
	{
        namespace General
        {
		    /***********************************************************************************/

            template <class TFLOW, class TCAP>
            bool EdmondsKarp<TFLOW,TCAP>::FindAugmentingPath (
                typename FordFulkerson<TFLOW,TCAP>::Node *source, 
                typename FordFulkerson<TFLOW,TCAP>::Node *sink, Size timestamp, 
                TCAP scale, TCAP &maxrjcap)
            {
                // Shortest path
                if (m_strategy == ShortestPathMethod)
                {
                    return BreadthFirstSearch (source, sink, timestamp, scale, maxrjcap);
                }

                // Fattest path
                if (this->m_cap_scale != 0)
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Fattest path strategy and capacity scaling are incompatible.");
                }

                maxrjcap = 0;                    
                return FattestPathSearch (source, sink, timestamp);
            }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    bool EdmondsKarp<TFLOW,TCAP>::BreadthFirstSearch(
                typename FordFulkerson<TFLOW,TCAP>::Node *source, 
		        typename FordFulkerson<TFLOW,TCAP>::Node *sink, Size timestamp, 
                TCAP scale, TCAP &maxrjcap)
		    {
			    typename FordFulkerson<TFLOW,TCAP>::Node *head, **fifo_read, **fifo_write;
			    typename FordFulkerson<TFLOW,TCAP>::Arc *cur_arc;

                maxrjcap = 0;
                source->m_ts = timestamp;

                fifo_read = fifo_write = m_node_heap.Begin();
                *fifo_write = source;
                fifo_write++;

			    while (fifo_read < fifo_write)
			    {
				    // Investigate all arcs connected to this node
				    for (cur_arc = (*fifo_read)->m_first; cur_arc != NULL; cur_arc = cur_arc->m_next)
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

						        *fifo_write = head;
						        fifo_write++;
					        }
                            else if (cap > maxrjcap)
                            {
                                maxrjcap = cap;
                            }
                        }
				    }

                    fifo_read++;
			    }

			    return false;
		    }

		    /***********************************************************************************/

		    template <class TFLOW, class TCAP>
		    bool EdmondsKarp<TFLOW,TCAP>::FattestPathSearch(
                typename FordFulkerson<TFLOW,TCAP>::Node *source, 
                typename FordFulkerson<TFLOW,TCAP>::Node *sink, Size timestamp)
		    {
			    Size heap_first = 0, heap_size = 1, i, heap_last;
			    TCAP best_cap;
			    typename FordFulkerson<TFLOW,TCAP>::Node *tmp;
			    typename FordFulkerson<TFLOW,TCAP>::Arc *cur_arc, *best_arc;

			    m_node_heap[0] = source;

			    while (heap_size)
			    {
				    // Find the fattest arc from nodes that are alive
				    best_arc = NULL;
				    best_cap = 0;

				    i = heap_first;
				    heap_last = heap_first + heap_size;

				    while (i < heap_last)
				    {
					    // Check all arcs for given node
					    bool is_alive = false;

					    for (cur_arc = m_node_heap[i]->m_first; cur_arc != NULL; cur_arc = cur_arc->m_next)
					    {
						    if (cur_arc->m_res_cap > 0 && cur_arc->m_head->m_ts != timestamp)
						    {
							    if (cur_arc->m_res_cap > best_cap)
							    {
								    best_arc = cur_arc;
								    best_cap = cur_arc->m_res_cap;
							    }
							    else
							    {
								    is_alive = true;
							    }
						    }
					    }

					    // Make nodes from which no move is possible dead and remove them from the heap
					    if (!is_alive)
					    {
						    tmp = m_node_heap[heap_first];
						    m_node_heap[heap_first] = m_node_heap[i];
						    m_node_heap[i] = tmp;
						    heap_first++;
						    heap_size--;
					    }

					    i++;
				    }

				    // Follow the fattest possible arc
				    if (best_arc != NULL)
				    {
					    tmp = best_arc->m_head;

					    tmp->m_parent = best_arc->m_sister;
					    m_node_heap[heap_last] = tmp;

					    if (tmp == sink)
					    {
						    return true;
					    }

					    tmp->m_ts = timestamp;
					    heap_size++;
				    }
			    }

			    return false;
		    }

		    /***********************************************************************************/

		    // Explicit instantiations
            /** @cond */
		    template class GC_DLL_EXPORT EdmondsKarp<Int32,Int32>;
		    template class GC_DLL_EXPORT EdmondsKarp<Float32,Float32>;
		    template class GC_DLL_EXPORT EdmondsKarp<Float64,Float64>;
            /** @endcond */
        }
	}
}
