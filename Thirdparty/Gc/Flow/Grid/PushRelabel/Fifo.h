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
    Push-Relabel maximum flow algorithm for directed grid graphs 
    with FIFO selection rule.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_GRID_PUSHRELABEL_FIFO_H
#define GC_FLOW_GRID_PUSHRELABEL_FIFO_H

#include "../../../Core.h"
#include "../CommonBase.h"

namespace Gc
{
	namespace Flow
	{
        namespace Grid
        {
            /** Maximum flow computation methods from Push-relabel family. */
            namespace PushRelabel
            {
                /** Implementation of Push-Relabel maximum flow algorithm for directed grid graphs
                    with FIFO selection rule.

                    For description of this algorithm see General::PushRelabel::Fifo.

                    @tparam N Number of dimensions of the grid.
                    @tparam TFLOW %Data type used for the flow value. Must be signed 
                        (allow negative numbers).
                    @tparam TCAP %Data type used for arc capacity values.
                    @tparam MASK If this parameter is \c false then you won't be able to
                        specify a voxel mask (method InitMask() will throw an exception) but 
                        the computation will be faster and will require slightly 
                        less memory.

                    @see General::PushRelabel::Fifo.
                */
		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        class GC_DLL_EXPORT Fifo
                    : public CommonBase<N,TFLOW,TFLOW,TCAP>
		        {
                protected:
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::m_arc_cap;
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::m_nb;
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::m_n_ofs;
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::m_bw_idx;
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::Sister;

                private:
                    /** Structure used for storing nodes in the network */
			        struct Node
			        {
                        /** Forward index to the grid. */
                        Size m_grid_idx;
                        /** Node rank (distance to the sink). */
                        Size m_rank;
                        /** Next active node in the FIFO queue. */
                        Node *m_fifo_succ;
                        /** Node excess. */
                        TFLOW m_excess;
                        /** Currently processed arc. */
				        typename CommonBase<N,TFLOW,TFLOW,TCAP>::NbIndexType m_current;
                    };

                    /** Node count. */
			        Size m_nodes; 

                    /** The array of nodes. */
                    System::Collection::Array<1,Node> m_node_list;
                    /** Node queue used during global update of ranks. */
                    System::Collection::Array<1,Node *> m_node_queue;
                    /** Forward (node -> grid) indexes. */
                    System::Collection::Array<1,Size> m_fw_idx;

                    /** First active node in the FIFO queue. */
                    Node *m_fifo_first;
                    /** Last active node in the FIFO queue. */
                    Node *m_fifo_last;
                    /** Global update frequency. */
                    double m_glob_rel_freq;
                    /** Total flow */
                    TFLOW m_flow;

                    /** Current stage. Used to check correct method calling order. */
                    Uint8 m_stage;

		        public:
			        /** Constructor. */
			        Fifo()
				        : m_nodes(0), m_glob_rel_freq(0.4), m_stage(0)
			        {}

			        /** Destructor. */
			        virtual ~Fifo()
			        {
				        Dispose();
			        }

                    virtual void Init(const Math::Algebra::Vector<N,Size> &dim, 
                        const Energy::Neighbourhood<N,Int32> &nb);
    
                    virtual void InitMask(const Math::Algebra::Vector<N,Size> &dim, 
                        const Energy::Neighbourhood<N,Int32> &nb,
                        const System::Collection::IArrayMask<N> &mask);

		            virtual void SetArcCap(Size node, Size arc, TCAP cap);

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

                    /** Get arc capacity. */
                    TCAP& ArcCap(Node *n, Size arc)
                    {
                        return m_arc_cap[(n - m_node_list.Begin()) * m_nb.Elements() + arc];
                    }

                    /** Get head node of given arc. 
                    
                        @param[in] n Pointer to the node. Used only in non-mask verison.
                        @param[in] nidx Grid index of the node (cached). Used only in 
                            masked version.
                        @param[in] arc Arc index.
                    */
                    Node *Head(Node *n, Size nidx, Size arc)
                    {
                        if (MASK)
                        {
                            return m_node_list.Begin() + m_bw_idx[nidx + m_n_ofs[arc]];
                        }
                        else
                        {
                            return n + m_n_ofs[arc];
                        }
                    }
                };
            }
        }
	}
}

#endif
