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
    Push-Relabel maximum flow algorithm for general directed graphs 
    with FIFO selection rule.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_GENERAL_PUSHRELABEL_FIFO_H
#define GC_FLOW_GENERAL_PUSHRELABEL_FIFO_H

#include "../../../Core.h"
#include "../../IMaxFlow.h"
#include "../../../System/Collection/Array.h"

namespace Gc
{
	namespace Flow
	{
        namespace General
        {
            /** Maximum flow computation methods from Push-relabel family. */
            namespace PushRelabel
            {
                /** Implementation of Push-Relabel maximum flow algorithm for general directed graphs
                    with FIFO selection rule.

                    This method is commonly referred to as <b>F_PRF</b> and its description can be found
                    in the following papers:
                    - Andrev V. Goldberg, Robert E. Tarjan: <em>A New Approach to the Maximum-Flow
                    Problem</em>, Journal of the Association for Computing Machinery (ACM), vol. 35,
                    no. 4, page 921-940, october 1988
                    - Boris V. Cherkassky, Andrew V. Goldberg: <em>On Implementing the Push-Relabel
                    Method for the Maximum %Flow Problem</em>, Algorithmica, vol. 19, no. 4. 
                    page 390-410, 1997, Springer

                    This variant of the <b>Push-Relabel</b> method uses first-in-first-out selection strategy
                    for the active nodes and global update heuristic. Gap relabeling heuristic is NOT
                    applied as it has been shown not to bring significant performance improvements 
                    (see FifoGap for the gap relabeling variant). The overall worst case
                    time complexity of this algorithm is \f$ \mathcal{O}(n^3) \f$.

                    @remarks This implementation loosely follows the implementation by Boris Cherkassky,
                    that can be downloaded from Andrew Goldberg's site: http://www.avglab.com/andrew/soft.html
                    (the PRF package). However, several changes have been made:
                    - Particularly, we don't store the terminal arcs in the memory and only initialize 
                    the excess of the incident node with the difference of the capacities. This have several 
                    positive effects (most notably much lower memory consumption and cancellation of 
                    redundant arc capacity). The only negative aspect of this approach is that
                    signed data type has to be used, because the excess can be negative (nodes with
                    negative excess are those directly connected to the sink).
                    - Secondly, we perform only the first half of the algorithm in which the flow
                    is pushed towards the sink. The second in which the excessive flow is returned
                    back to the source is not necessary, because we are interested only in the minimal
                    cut and the value of the maximum flow (not the flow itself) and these are already known
                    after the first half (refer to the first listed paper).
                    - Finally, we always perform the global update before the start of the algorithm 
                    to obtain the most accurate initial choice of the ranks. The frequency of the 
                    subsequent global updates can be chosen using SetGlobalUpdateFrequency().

                    @tparam TFLOW %Data type used for the flow value. Must be signed 
                        (allow negative numbers).
                    @tparam TCAP %Data type used for arc capacity values.

                    @see HighestLevel, FifoGap
                */
		        template <class TFLOW, class TCAP>
		        class GC_DLL_EXPORT Fifo 
                    : public IMaxFlow<TFLOW,TFLOW,TCAP>
		        {
                private:
                    // Forward declaration
			        struct Node;

                    /** Structure for storing arcs in the network. */
			        struct Arc
			        {
                        /** Residual capacity. */
				        TCAP m_res_cap;
                        /** Pointer to the next arc with the same tail. */
				        Arc *m_next;
                        /** Pointer to the arc going in the opposite direction. */
				        Arc *m_sister;
                        /** Pointer to the head node of this arc. */
				        Node *m_head;
			        };

                    /** Structure used for storing nodes in the network */
			        struct Node
			        {
                        /** Pointer to the first arc going from this node. */
				        Arc *m_first;
                        /** Currently processed arc. */
				        Arc *m_current;
                        /** Node excess. */
                        TFLOW m_excess;
                        /** Node rank (distance to the sink). */
                        Size m_rank;
                        /** Next active node in the FIFO queue. */
                        Node *m_fifo_succ;
			        };

                    /** The array of nodes. */
                    System::Collection::Array<1,Node> m_node_list;
                    /** Node queue used during global update of ranks. */
                    System::Collection::Array<1,Node *> m_node_queue;
                    /** First active node in the FIFO queue. */
                    Node *m_fifo_first;
                    /** Last active node in the FIFO queue. */
                    Node *m_fifo_last;
                    /** The array of arcs. */
                    System::Collection::Array<1,Arc> m_arc_list;
                    /** Node count. */
			        Size m_nodes;
                    /** Arc count. */
			        Size m_arcs;
                    /** Global update frequency. */
                    double m_glob_rel_freq;
                    /** Total flow */
                    TFLOW m_flow;

                    /** Current stage. Used to check correct method calling order. */
                    Uint8 m_stage;

		        public:
			        /** Constructor. */
			        Fifo()
				        : m_nodes(0), m_arcs(0), m_glob_rel_freq(0.4), m_stage(0)
			        {}

			        /** Destructor. */
			        virtual ~Fifo ()
			        {
				        Dispose();
			        }

    			    virtual void Init(Size nodes, Size max_arcs, Size src_arcs, Size snk_arcs);
    
	    		    virtual void SetArcCap(Size n1, Size n2, TCAP cap, TCAP rcap);

		    	    virtual void SetTerminalArcCap(Size node, TFLOW csrc, TFLOW csnk);

			        virtual TFLOW FindMaxFlow();

			        virtual Origin NodeOrigin(Size node) const;

			        virtual void Dispose();

                    /** Set the global update frequency.

                        Global update is performed whenever the number of relabelings
                        exceeds the number of nodes times the global update frequency.
                    
                        @param[in] new_freq New global update frequency.
                    */
                    void SetGlobalUpdateFrequency(double new_freq)
                    {
                        m_glob_rel_freq = new_freq;
                    }

                    /** Get global update frequency.
                    
                        @return Current global update frequency.
                    */
                    double GlobalUpdateFrequency()
                    {
                        return m_glob_rel_freq;
                    }

		        private:
                    /** Global update of node ranks. 
                    
                        Performed using breadth-first search from the sink. This method has
                        to be performed regularly as it has drastical impact on the running
                        time of the algorithm.
                    */
                    void GlobalUpdate();
		        };
            }
        }
	}
}

#endif
