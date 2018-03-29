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
    Topology preserving maximum flow algorithm for directed grid graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_FLOW_GRID_ZENGDANEK_H
#define GC_FLOW_GRID_ZENGDANEK_H

#include <map>
#include "../../Core.h"
#include "CommonBase.h"

namespace Gc
{
	namespace Flow
	{
        namespace Grid
        {
            /** Topology preserving maximum flow algorithm for directed grid graphs.

                This algorithm is modified version of the General::BoykovKolmogorov algorithm.
                It allows to specify an initial labeling of the nodes and guarantees that the
                computed maximum flow will conform with this initialization in the sense that
                it will preserve its topology.

                The main idea is taken from the paper:
                - Y. Zeng, D. Samaras, W. Chen, and Q. Peng. <em>Topology cuts: A novel 
                min-cut/max-flow algorithm for topology preserving segmentation in n-d images.</em>
                Computer Vision and Image Understanding, 112(1):81-90, 2008.

                Even though some ideas are taken from the paper this implementation is
                rather different. First of all a simplification is used - all four
                trees grow at once and augmenting paths are found irrespective of the
                labels. This replaces the overly complex intra- and inter-label maximum
                flows and seems to give the same results. 
                - O. Danek, M. Maska. <em>A Simple Topology Preserving Max-Flow Algorithm for 
                Graph Cut Based Image Segmentation</em>. 6th Doctoral Workshop on Mathematical 
                and Engineering Methods in Computer Science. Brno : NOVPRESS, 2010, pp. 45-52
                
                Another modification is how active nodes are stored. In the original paper
                a bucket priority queue with constant complexity is used. However, this
                method seems to be flawed. The paper states that the distance to boundary cannot
                overflow/underflow given interval, but the reasoning is poor and in practice
                this often happens causing a crash of the algorithm. For this purpose we
                use a associative array storing queues of active nodes at given distance
                level allowing us to quickly find smallest distance nodes and insert/remove
                nodes. The theoretical complexity of this approach is logarithmics, however,
                the speed difference was less than 1% compared to the original method in
                our experiments and the method is guaranteed to work in all cases. 
                This modification is not published.

                The algorithm works for both 2D and 3D grids. It has been tested with the
                Chan-Vese and geodesic segmentation models. While the former one gives
                good results in most of the situations the latter one is more problematic
                and tends to yield unexpected results under certain conditions.

                @warning In some rare cases may throw std::bad_alloc when the computation 
                    runs out of memory.

                @tparam N Number of dimensions of the grid.
                @tparam TFLOW %Data type used for the flow value.
                @tparam TTCAP %Data type used for terminal arc capacity values. This type 
                    must be signed (allow negative numbers).
                @tparam TCAP %Data type used for regular arc capacity values.
                @tparam MASK If this parameter is \c false then you won't be able to
                    specify a voxel mask (method InitMask() will throw an exception) but 
                    the computation will be faster and will require slightly 
                    less memory.
            */
		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    class GC_DLL_EXPORT ZengDanek
                : public CommonBase<N,TFLOW,TTCAP,TCAP>
		    {
            protected:
                using CommonBase<N,TFLOW,TFLOW,TCAP>::m_arc_cap;
                using CommonBase<N,TFLOW,TFLOW,TCAP>::m_nb;
                using CommonBase<N,TFLOW,TFLOW,TCAP>::m_n_ofs;
                using CommonBase<N,TFLOW,TFLOW,TCAP>::m_bw_idx;
                using CommonBase<N,TFLOW,TFLOW,TCAP>::Sister;

            private:
                /** Structure used for storing nodes in the network. */
			    class Node
			    {
                public:
                    /** Next node in the queue of active nodes. */
                    Node *m_qnext;
                    /** Previous node in the queue of active nodes. */
                    Node *m_qprev;
                    /** Next orphan node. When this is not NULL, then node is orphan. */
                    Node *m_next_orphan;
                    /** Residual capacity of the terminal arcs */
                    TTCAP m_tr_cap;
                    /** Node distance to the contour. */
                    Int32 m_dist;
                    /** Index of arc going to the parent node. */
				    Size m_parent_arc;
                    /** Node tag. 
                    
                        It stores information about 4 binary variables:
                        - bit 1 - 0 = background, 1 = foreground
                        - bit 2 - 0 = source tree, 1 = sink tree
                        - bit 3 - 0 = active node, 1 = passive node
                        - bit 4 - 0 = non-free node, 1 = free node
                    */
                    Uint8 m_tag;
                };

                /** First-in-first-out queue structure */
                struct Queue
                {
                    /** First node in the queue */
                    Node *m_first;
                    /** Last node in the queue */
                    Node *m_last;
                };

            private:
                /** Queue of active nodes. Nodes closer to the boundary are 
                    processed first.
                */
                class ActiveNodeQueue
                {
                private:
                    /** Queue of nodes on currently processed distance level. */
                    Queue m_cur_lev;
                    /** Queues of remaining distance levels. */
                    std::map<Int32, Queue> m_map;

                public:
                    /** Dispose memory. */
                    void Dispose()
                    {                        
                        m_map.clear();
                    }

                    /** Insert node to the queue. 
                    
                        Use this method only during the initial filling of the queue.
                        During the computation use PushNode method instead.
                    */
                    void InsertNode(Node *n)
                    {
                        typename std::map<Int32, Queue>::iterator iter = m_map.find(n->m_dist);

                        if (iter == m_map.end())
                        {
                            Queue q;

                            q.m_first = q.m_last = n;
                            m_map[n->m_dist] = q;
                        }
                        else
                        {
                            iter->second.m_last->m_qnext = n;
                            n->m_qprev = iter->second.m_last;
                            iter->second.m_last = n;
                        }
                    }

                    /** Init queue. Nodes closest to the boundary are set as active. */
                    void Init()
                    {
                        if (m_map.empty())
                        {
                            m_cur_lev.m_first = m_cur_lev.m_last = NULL;
                        }
                        else
                        {
                            typename std::map<Int32, Queue>::iterator iter = m_map.begin();

                            m_cur_lev = iter->second;
                            m_map.erase(iter);
                        }
                    }

                    /** Get next active from the currently active distance level. */
                    Node *FrontNode()
                    {
                        return m_cur_lev.m_first;
                    }

                    /** Insert node into the queue. */
                    void PushNode(Node *n)
                    {
                        if (m_cur_lev.m_first == NULL)
                        {
                            m_cur_lev.m_first = m_cur_lev.m_last = n;
                        }
                        else
                        {
                            if (m_cur_lev.m_first->m_dist == n->m_dist)
                            {
                                m_cur_lev.m_last->m_qnext = n;
                                n->m_qprev = m_cur_lev.m_last;
                                m_cur_lev.m_last = n;
                            }
                            else
                            {
                                InsertNode(n);
                            }
                        }
                    }

                    /** Remove node from the queue. */
                    void RemoveNode(Node *n)
                    {
                        Queue *q;

                        // Find queue containing the node and solve special cases
                        if (n->m_dist == m_cur_lev.m_first->m_dist)
                        {
                            if (m_cur_lev.m_first == m_cur_lev.m_last)
                            {
                                if (m_map.empty())
                                {
                                    m_cur_lev.m_first = m_cur_lev.m_last = NULL;
                                }
                                else
                                {
                                    typename std::map<Int32, Queue>::iterator iter = m_map.begin();

                                    m_cur_lev = iter->second;
                                    m_map.erase(iter);
                                }

                                return;
                            }

                            q = &m_cur_lev;
                        }
                        else
                        {
                            typename std::map<Int32, Queue>::iterator iter = m_map.find(n->m_dist);
                            q = &iter->second;

                            if (q->m_first == q->m_last)
                            {
                                m_map.erase(iter);
                                return;
                            }
                        }

                        // Remove node from the queue
                        if (n == q->m_first)
                        {
                            q->m_first = q->m_first->m_qnext;
                        }
                        else
                        {
                            if (n == q->m_last)
                            {
                                q->m_last = q->m_last->m_qprev;
                            }
                            else
                            {
                                n->m_qprev->m_qnext = n->m_qnext;
                                n->m_qnext->m_qprev = n->m_qprev;
                            }
                        }
                    }

                    /** Check whether the queue is empty. */
                    bool IsEmpty() const
                    {
                        return (m_cur_lev.m_first == NULL);
                    }
                };

            private:
                /** Node array. */
                System::Collection::Array<1,Node> m_node_list;
                /** Initial labelling. */
                const System::Collection::Array<N,bool> *m_ilab;
                /** Initial distance map. */
                System::Collection::Array<N,Uint32> m_dmap;
                
                /** Forward (node -> grid) indexes. Used only in masked version. */
                System::Collection::Array<1,Size> m_fw_idx;
                /** Node labels (copy of node->m_tag & 1). Required for IsSimple lookup
                    in the masked version but used also in the unmasked one. */
                System::Collection::Array<N,Uint8> m_clab;

                /** Total flow */
                TFLOW m_flow;
                /** Current stage. Used to check correct method calling order. */
                Uint8 m_stage;

                /** Active nodes for SB, SF, TB and TF trees. */
                ActiveNodeQueue m_bp[4];
                /** Queue of orphans */
                Queue m_q_orphan;

		    public:
			    /** Constructor */
			    ZengDanek()
                    : m_ilab(NULL), m_stage(0)
			    {}

			    /** Destructor */
			    virtual ~ZengDanek()
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

			    virtual void Dispose();

                /** Set initial labelling. 
                
                    @warning Only pointer to the object is taken, so this object
                        should not be deleted before or during the computation!
                */
                void SetInitialLabelingRef(const System::Collection::Array<N,bool> &ilab);

            private:
                /** Grow source and sink trees until an augmenting path is found. */
                bool GrowTrees(Node *n, Node *&n_src, Node *&n_snk, Size &arc);

                /** Augment flow across given path. */
                void Augment(Node *n_src, Node *n_snk, Size arc);

                /** Adopt orphan nodes. 
                
                    @param[in] timestamp Current timestamp.
                */
                void AdoptOrphans();

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

                /** Add node to its bucket priority queue. */
                void EnqueueNode(Node *n)
                {
                    m_bp[n->m_tag & 3].PushNode(n);
                }

                /** Remove node from its bucket priority queue. */
                void DequeueNode(Node *n)
                {
                    m_bp[n->m_tag & 3].RemoveNode(n);
                }

                /** Get forward index. */
                Size NodeFwIdx(Node *n) const
                {
                    return MASK ? m_fw_idx[n - m_node_list.Begin()] : Size(n - m_node_list.Begin());
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
                        return m_node_list.Begin() + m_bw_idx[NodeFwIdx(n) + m_n_ofs[n->m_parent_arc]];
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

                /** Check if given node constitutes a simple point. 
                
                    Simple point is a point that can be ralebeled from foreground to background
                    or vice versa without changing the topology.
                */
                bool IsSimple(Node *n) const;
            };
        }
	}
}

#endif
