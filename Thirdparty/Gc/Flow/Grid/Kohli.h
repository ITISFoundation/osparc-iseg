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
    Kohli's dynamic maximum flow algorithm for directed grid graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_GRID_KOHLI_H
#define GC_FLOW_GRID_KOHLI_H

#include "../../Core.h"
#include "CommonBase.h"

namespace Gc
{
    namespace Flow
    {
        namespace Grid
        {
            /** Implementation of Kohli's dynamic maximum flow algorithm for grid graphs.

                For description of this algorithm see General::Kohli.

                @tparam N Number of dimensions of the grid.
                @tparam TFLOW %Data type used for the flow value.
                @tparam TTCAP %Data type used for terminal arc capacity values. This type 
                    must be signed (allow negative numbers).
                @tparam TCAP %Data type used for regular arc capacity values.
                @tparam MASK If this parameter is \c false then you won't be able to
                    specify a voxel mask (method InitMask() will throw an exception) but 
                    the computation will be faster and will require slightly 
                    less memory.

                @see General::Kohli.
                
                @todo Currently dynamic change of the capacity is supported only for the
                    terminal arcs. Also note that the flow value returned in subsequent
                    calls to FindMaxFlow() after dynamic changes of the graph is currently 
                    incorrect (i.e., it is not the same as what would be returned by a
                    non-dynamic algorithm).
            */
            template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
            class GC_DLL_EXPORT Kohli
                : public CommonBase<N,TFLOW,TTCAP,TCAP>
            {
            protected:
                using CommonBase<N,TFLOW,TTCAP,TCAP>::m_arc_cap;
                using CommonBase<N,TFLOW,TTCAP,TCAP>::m_nb;
                using CommonBase<N,TFLOW,TTCAP,TCAP>::m_n_ofs;
                using CommonBase<N,TFLOW,TTCAP,TCAP>::m_bw_idx;
                using CommonBase<N,TFLOW,TTCAP,TCAP>::Sister;

            private:    
                /** Structure used for storing nodes in the network. */
                struct Node
                {
                    /** Next active/marked node. When this is not NULL, then node is active/marked. */
                    Node *m_next_active;
                    /** Next orphan node. When this is not NULL, then node is orphan. */
                    Node *m_next_orphan;
                    /** Timestamp when distance was computed. */
                    Size m_ts;
                    /** Distance to the terminal */
                    Size m_dist;
                    /** Residual capacity of the terminal arcs */
                    TTCAP m_tr_cap;
                    /** Flow going through the node. */
                    TTCAP m_f_diff;
                    /** Index of arc going to the parent node. */
                    typename CommonBase<N,TFLOW,TTCAP,TCAP>::NbIndexType m_parent_arc;
                    /** Node origin - source, sink or free */
                    Uint8 m_origin;
                };

                /** First-in-first-out queue structure */
                struct Queue
                {
                    /** First node in the queue */
                    Node *m_first;
                    /** Last node in the queue */
                    Node *m_last;
                };

                /** Graph nodes. */
                System::Collection::Array<1,Node> m_node_list;
                /** Forward (node -> grid) indexes. */
                System::Collection::Array<1,Size> m_fw_idx;

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
                Uint8 m_stage;

		    public:
			    /** Constructor */
			    Kohli()
                    : m_stage(0)
                {}

			    /** Destructor */
			    virtual ~Kohli()
			    {}

                virtual void Init(const Math::Algebra::Vector<N,Size> &dim, 
                    const Energy::Neighbourhood<N,Int32> &nb);

                virtual void InitMask(const Math::Algebra::Vector<N,Size> &dim, 
                    const Energy::Neighbourhood<N,Int32> &nb,
                    const System::Collection::IArrayMask<N> &mask);

		        virtual void SetArcCap(Size node, Size arc, TCAP cap);

			    virtual void SetTerminalArcCap(Size node, TTCAP csrc, TTCAP csnk);

			    virtual TFLOW FindMaxFlow();

			    virtual Origin NodeOrigin(Size node) const;

                virtual bool IsDynamic() const
                {
                    return true;
                }

			    virtual void Dispose();

            private:
                /** Init source and sink trees for first use. */
                void InitTrees();

                /** Recycle trees from previous run. */
                void RecycleTrees();

                /** Grow source and sink trees until an augmenting path is found. */
                bool GrowTrees(Node *&n_src, Node *&n_snk, Size &arc);

                /** Augment flow across given path. */
                void Augment(Node *n_src, Node *n_snk, Size arc);

                /** Adopt orphan nodes. 
                
                    @param[in] timestamp Current timestamp.
                */
                void AdoptOrphans();

                /** Add node to the queue of active nodes.
                    
                    @param[in] node Node to be added.
                */
                void AddToActive(Node *node)
                {
                    if (node->m_next_active == NULL)
                    {
                        if (m_q_active.m_last != NULL)
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
                void AddToOrphans(Node *node)
                {
                    if (node->m_next_orphan == NULL)
                    {                    
                        if (m_q_orphan.m_last != NULL)
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

                /** Get arc capacity. */
                TCAP& ArcCap(Node *n, Size arc)
                {
                    return m_arc_cap[(n - m_node_list.Begin()) * m_nb.Elements() + arc];
                }

                /** Get parent node. */
                Node *ParentNode(Node *n)
                {
                    if (MASK)
                    {
                        return m_node_list.Begin() + m_bw_idx[m_fw_idx[n - m_node_list.Begin()] + 
                            m_n_ofs[n->m_parent_arc]];
                    }
                    else
                    {
                        return n + m_n_ofs[n->m_parent_arc];
                    }
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

#endif
