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
    Push-Relabel maximum flow algorithm for directed grid graphs with highest-level 
    selection rule.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_GRID_PUSHRELABEL_HIGHESTLEVEL_H
#define GC_FLOW_GRID_PUSHRELABEL_HIGHESTLEVEL_H

#include "../../../Core.h"
#include "../CommonBase.h"

namespace Gc
{
	namespace Flow
	{
        namespace Grid
        {
            namespace PushRelabel
            {
                /** Implementation of Push-Relabel maximum flow algorithm on directed graphs
                    with highest level selection strategy.

                    For description of this algorithm see General::PushRelabel::HighestLevel.

                    @tparam N Number of dimensions of the grid.
                    @tparam TFLOW %Data type used for the flow value. Must be signed 
                        (allow negative numbers).
                    @tparam TCAP %Data type used for arc capacity values.
                    @tparam MASK If this parameter is \c false then you won't be able to
                        specify a voxel mask (method InitMask() will throw an exception) but 
                        the computation will be faster and will require slightly 
                        less memory.

                    @see General::PushRelabel::HighestLevel.
                */
		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        class GC_DLL_EXPORT HighestLevel
                    : public CommonBase<N,TFLOW,TFLOW,TCAP>
		        {
                protected:
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::m_arc_cap;
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::m_nb;
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::m_n_ofs;
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::m_bw_idx;
                    using CommonBase<N,TFLOW,TFLOW,TCAP>::Sister;

                private:
                    /** Structure for storing nodes in the network. */
			        struct Node
			        {
                        /** Forward index to the grid. */
                        Size m_grid_idx;
                        /** Node rank (distance to the sink). */
                        Size m_rank;
                        /** Next node (active or inactive) on the same layer */
                        Node *m_lay_next;
                        /** Previous node (inactive only) on the same layer */
                        Node *m_lay_prev;
                        /** Node excess. */
                        TFLOW m_excess;
                        /** Currently processed arc. */
				        typename CommonBase<N,TFLOW,TFLOW,TCAP>::NbIndexType m_current;
                    };

                    /** Structure for storing rank layers. */
                    struct Layer
                    {
                        /** First active node on the layer. Active nodes form singly-linked 
                            list with deleting possible only from the front. */
                        Node *m_first_active;
                        /** First inactive node on the layer. Inactive nodes form doubly-linked
                            list with deleting possible from arbitraty position. */
                        Node *m_first_inactive;
                    };

                    /** Node count (including sink and source). */
			        Size m_nodes;

                    /** The array of graph nodes. */
                    System::Collection::Array<1,Node> m_node_list;
                    /** Forward (node -> grid) indexes. */
                    System::Collection::Array<1,Size> m_fw_idx;

                    /** Global update frequency. */
                    double m_glob_rel_freq;
                    /** Total flow */
                    TFLOW m_flow;

                    /** Array of layers. */
                    System::Collection::Array<1,Layer> m_layer_list;
                    /** Highest rank. */
                    Size m_max_rank;
                    /** Highest active rank. */
                    Size m_max_active_rank;
                    /** Lowest active rank. */
                    Size m_min_active_rank;

                    /** Current stage. Used to check correct method calling order. */
                    Uint8 m_stage;

		        public:
			        /** Constructor. */
			        HighestLevel()
				        : m_nodes(0), m_glob_rel_freq(1.5), m_stage(0)
                    {}

			        /** Destructor. */
			        virtual ~HighestLevel()
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

                    /** Gap relabeling.

                        Whenever a gap emerges in the node ranks all nodes with a rank above
                        this gap can be marked as unreachable from the sink.
                    
                        @param[in] empty_lay The layer that has become empty.
                        @param[in] next_layer_rank Rank of the layer below the empty layer.
                    */
                    void GapRelabeling(Layer *empty_lay, Size next_layer_rank);

                    /** Add node to the list of active nodes on given layer.

                        @param[in] lay Layer pointer
                        @param[in] node Node to be added.
                    */
                    void AddToActive(Layer *lay, Node *node)
                    {
                        node->m_lay_next = lay->m_first_active;
                        lay->m_first_active = node;
                        Size rank = node->m_rank;
                        if (rank < m_min_active_rank)
                        {
                            m_min_active_rank = rank;
                        }
                        if (rank > m_max_active_rank)
                        {
                            m_max_active_rank = rank;
                        }
                    }

                    /** Add node to the list of inactive nodes on given layer.

                        @param[in] lay Layer pointer.
                        @param[in] node Node to be added.
                    */
                    void AddToInactive(Layer *lay, Node *node)
                    {
                        Node *first = lay->m_first_inactive;

                        if (first != NULL)
                        {
                            first->m_lay_prev = node;
                        }
                        node->m_lay_next = first;
                        node->m_lay_prev = NULL;
                        lay->m_first_inactive = node;
                    }        

                    /** Remove node from the list of inactive nodes on given layer.

                        @param[in] lay Layer pointer.
                        @param[in] node Node to be removed.
                    */
                    void RemoveFromInactive(Layer *lay, Node *node)
                    {
                        Node *prev = node->m_lay_prev, *next = node->m_lay_next;

                        if (prev != NULL)
                        {
                            prev->m_lay_next = next;
                        }
                        else
                        {
                            lay->m_first_inactive = next;
                        }

                        if (next != NULL)
                        {
                            next->m_lay_prev = prev;
                        }
                    }        

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
