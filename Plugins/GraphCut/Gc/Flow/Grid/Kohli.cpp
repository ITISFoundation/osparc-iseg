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
    @date 2010
*/

#include "../../Math/Basic.h"
#include "../../System/Format.h"
#include "../../System/IndexOutOfRangeException.h"
#include "../../System/InvalidOperationException.h"
#include "../../System/NotImplementedException.h"
#include "Kohli.h"

namespace Gc
{
	namespace Flow
	{
        namespace Grid
        {
            /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::Init(const Math::Algebra::Vector<N,Size> &dim, 
                const Energy::Neighbourhood<N,Int32> &nb)
		    {
                this->ValidateParameters(dim, nb);

			    Dispose();                

                m_node_list.Resize(dim.Product());
                m_arc_cap.Resize(m_node_list.Elements() * nb.Elements());
                m_flow = 0;
                m_stage = 1;

                // Initialize the node array
                m_node_list.ZeroFillMemory();

                this->InitBase(dim, nb);

                if (MASK)
                {
                    // Set forward and backward indexes
                    m_bw_idx.Resize(dim);
                    m_fw_idx.Resize(m_node_list.Elements());

                    for (Size i = 0; i < m_bw_idx.Elements(); i++)
                    {
                        m_bw_idx[i] = i;
                        m_fw_idx[i] = i;
                    }
                }
		    }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::InitMask(const Math::Algebra::Vector<N,Size> &dim, 
                const Energy::Neighbourhood<N,Int32> &nb, 
                const System::Collection::IArrayMask<N> &mask)
		    {
                if (MASK)
                {
                    this->ValidateParameters(dim, nb);

                    Dispose();                

                    m_node_list.Resize(mask.UnmaskedElements());
                    m_arc_cap.Resize(m_node_list.Elements() * nb.Elements());
                    m_flow = 0;
                    m_stage = 1;

                    // Initialize the node array
                    m_node_list.ZeroFillMemory();

                    this->InitBaseMasked(dim, nb, mask);

                    // Set forward (node -> grid) indexes
                    Size cur_idx = 0;
                    m_fw_idx.Resize(mask.UnmaskedElements());
                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        if (!mask.IsMasked(i))
                        {
                            m_fw_idx[cur_idx] = i;
                            cur_idx++;
                        }
                    }
                }
                else
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__, "This algorithm "
                        "does not support mask specification.");
                }
		    }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::SetArcCap(Size node, Size arc, TCAP cap)
		    {
                if (m_stage < 1)
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Call Init method before adding arcs.");
                }

                if (m_stage > 1)
                {
                    throw System::NotImplementedException(__FUNCTION__, __LINE__,
                        "Arc capacity update support not yet implemented!");
                }

                if (cap < 0)
                {
                    throw System::ArgumentException(__FUNCTION__, __LINE__,
                        "Arcs with negative capacity are not supported.");
                }

			    if (node >= m_node_list.Elements())
			    {
                    throw System::IndexOutOfRangeException(__FUNCTION__, __LINE__, 
                        System::Format("Invalid node index. Nodes={0}, node={1}.")
                        << m_node_list.Elements() << node);
			    }

			    if (arc >= m_nb.Elements())
			    {
                    throw System::IndexOutOfRangeException(__FUNCTION__, __LINE__,
                        System::Format("Invalid arc index. Arcs={0}, arc={1}.") 
                        << m_nb.Elements() << arc);
			    }

                TCAP *arc_cap = m_arc_cap.Begin() + arc + node * m_nb.Elements();
                if (*arc_cap >= 0)
                {
                    *arc_cap = cap;
                }
		    }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::SetTerminalArcCap(Size node, TTCAP csrc, TTCAP csnk)
            {
                if (m_stage < 1)
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

                Node *pnode = &m_node_list[node];

                if (m_stage == 1)
                {
                    pnode->m_tr_cap = csrc - csnk;                
                    pnode->m_f_diff = 0;
                    m_flow += Math::Min(csrc, csnk);
                }
                else
                {
                    TTCAP old_tr = pnode->m_tr_cap;
                    pnode->m_tr_cap = csrc - csnk - pnode->m_f_diff;

                    if (Math::Sgn(old_tr) != Math::Sgn(pnode->m_tr_cap)) // Essential change
                    {
                        // Add to queue of marked nodes
                        if (m_q_marked.m_first == NULL)
                        {
                            m_q_marked.m_first = m_q_marked.m_last = pnode;
                            pnode->m_next_active = pnode; // Mark node!
                        }
                        else
                        {
                            pnode->m_next_active = m_q_marked.m_first; // Mark node!
                            m_q_marked.m_first = pnode;
                        }
                    }
                }
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::InitTrees()
            {
                m_time = 0;
                m_q_active.m_first = m_q_active.m_last = NULL;
                m_q_orphan.m_first = m_q_orphan.m_last = NULL;
                m_q_marked.m_first = m_q_marked.m_last = NULL;

                // Find nodes connected to the source and to the sink
                for (Node *node = m_node_list.Begin(); node != m_node_list.End(); ++node)
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
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::RecycleTrees()
            {
                Node *node, *head;

                m_time++;

                while (m_q_marked.m_first != NULL)
                {
                    node = m_q_marked.m_first;
                    if (m_q_marked.m_first == m_q_marked.m_last)
                    {
                        m_q_marked.m_first = m_q_marked.m_last = NULL;
                    }
                    else
                    {
                        m_q_marked.m_first = node->m_next_active;
                    }

                    node->m_next_active = NULL;
                    AddToActive(node);

                    if (!node->m_tr_cap)
                    {
                        AddToOrphans(node);
                        continue;
                    }

                    // Cached node grid index
                    Size nidx = MASK ? m_fw_idx[node - m_node_list.Begin()] : 0;

                    if (node->m_tr_cap > 0 && node->m_origin != Source)
                    {
                        node->m_origin = Source;

                        TCAP *arc_cap = &ArcCap(node, 0);
                        for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                        {                            
                            if (*arc_cap >= 0)
                            {
                                head = Head(node, nidx, i);
                                if (!head->m_tr_cap && ParentNode(head) == node) // Not marked
                                {
                                    AddToOrphans(head);
                                }
                                if (head->m_origin == Sink && *arc_cap > 0)
                                {
                                    AddToActive(head);
                                }
                            }
                        }
                    }
                    else if (node->m_tr_cap < 0 && node->m_origin != Sink)
                    {
                        node->m_origin = Sink;

                        TCAP *arc_cap = &ArcCap(node, 0);
                        for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                        {                            
                            if (*arc_cap >= 0)
                            {
                                head = Head(node, nidx, i);
                                if (!head->m_tr_cap && ParentNode(head) == node) // Not marked
                                {
                                    AddToOrphans(head);
                                }
                                if (head->m_origin == Source && *arc_cap > 0)
                                {
                                    AddToActive(head);
                                }
                            }
                        }
                    }

                    node->m_dist = 1;
                    node->m_ts = m_time;
                }

                AdoptOrphans();
            }

            /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    TFLOW Kohli<N,TFLOW,TTCAP,TCAP,MASK>::FindMaxFlow()
		    {
                if (m_stage < 1)
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Call Init method before computing maximum flow.");
                }
                
                // Initialization
                if (m_stage == 1)
                {
                    InitTrees();
                }
                else
                {
                    RecycleTrees();
                }

                // Main loop
                while (m_q_active.m_first != NULL)
                {
                    // Grow trees and find augmenting path
                    Node *n_src, *n_snk;
                    Size arc;
                    
                    if (GrowTrees(n_src, n_snk, arc))
                    {
                        // Augment and adopth orphans
                        Augment(n_src, n_snk, arc);
                        AdoptOrphans();
                    }
                }

                m_stage = 2;

			    return m_flow;
		    }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    bool Kohli<N,TFLOW,TTCAP,TCAP,MASK>::GrowTrees(Node *&n_src, Node *&n_snk, Size &arc)
            {
                TCAP cap;
                Node *node, *head;

                while (m_q_active.m_first != NULL)
                {
                    // Increase time
                    m_time++;

                    // Pop node from the queue of active nodes
                    node = m_q_active.m_first;

                    // Cached node grid index
                    Size nidx = MASK ? m_fw_idx[node - m_node_list.Begin()] : 0;

                    // Grow tree
                    if (node->m_origin != Free)
                    {
                        TCAP *arc_cap = &ArcCap(node, 0);
                        for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                        {
                            if (*arc_cap >= 0)
                            {
                                head = Head(node, nidx, i);
                                cap = (node->m_origin == Source) ? *arc_cap : ArcCap(head, Sister(i));

                                if (cap > 0)
                                {                                    
                                    if (head->m_origin == Free)
                                    {
                                        head->m_origin = node->m_origin;
                                        head->m_parent_arc = (typename CommonBase<N,TFLOW,TTCAP,TCAP>::NbIndexType)Sister(i);
                                        head->m_dist = node->m_dist + 1;
                                        head->m_ts = node->m_ts;
                                        AddToActive(head);
                                    }
                                    else if (node->m_origin != head->m_origin)
                                    {
                                        if (node->m_origin == Source)
                                        {
                                            n_src = node;
                                            n_snk = head;
                                            arc = i;
                                        }
                                        else
                                        {
                                            n_src = head;
                                            n_snk = node;
                                            arc = Sister(i);
                                        }
                                        return true;
                                    }
                                    else if (head->m_ts <= node->m_ts && head->m_dist > node->m_dist + 1) 
                                    {
                                        // Heuristic, trying to produce shorter paths.
                                        // Remark: > in m_dist comparison seems to perform better than >=
                                        head->m_parent_arc = (typename CommonBase<N,TFLOW,TTCAP,TCAP>::NbIndexType)Sister(i);
                                        head->m_dist = node->m_dist + 1;
                                        head->m_ts = node->m_ts;
                                    }
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

                return false;
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::Augment(Node *n_src, Node *n_snk, Size arc)
            {
                TCAP bottleneck = ArcCap(n_src, arc);
                Node *node;

                // Find bottleneck in the source tree
                for (node = n_src; node->m_dist > 1; )
                {
                    Node *parent = ParentNode(node);
                    TCAP cap = ArcCap(parent, Sister(node->m_parent_arc));
                    if (cap < bottleneck)
                    {
                        bottleneck = cap;
                    }
                    node = parent;
                }
                if (node->m_tr_cap < bottleneck)
                {
                    bottleneck = node->m_tr_cap;
                }

                // Find bottleneck in the sink tree
                for (node = n_snk; node->m_dist > 1; node = ParentNode(node))
                {
                    TCAP cap = ArcCap(node, node->m_parent_arc);
                    if (cap < bottleneck)
                    {
                        bottleneck = cap;
                    }
                }
                if (-node->m_tr_cap < bottleneck)
                {
                    bottleneck = -node->m_tr_cap;
                }

                // Augmentation
                m_flow += bottleneck;

                ArcCap(n_src, arc) -= bottleneck;
                ArcCap(n_snk, Sister(arc)) += bottleneck;

                // Augment in the source tree
                for (node = n_src; node->m_dist > 1; )
                {
                    Node *parent = ParentNode(node);
                    ArcCap(node, node->m_parent_arc) += bottleneck;
                    if ((ArcCap(parent, Sister(node->m_parent_arc)) -= bottleneck) == 0)
                    {
                        AddToOrphans(node);
                    }
                    node = parent;
                }
                if ((node->m_tr_cap -= bottleneck) == 0)
                {
                    AddToOrphans(node);
                }
                node->m_f_diff += bottleneck;

                // Augment in the sink tree
                for (node = n_snk; node->m_dist > 1; )
                {
                    Node *parent = ParentNode(node);
                    ArcCap(parent, Sister(node->m_parent_arc)) += bottleneck;
                    if ((ArcCap(node, node->m_parent_arc) -= bottleneck) == 0)
                    {
                        AddToOrphans(node);
                    }
                    node = parent;
                }
                if ((node->m_tr_cap += bottleneck) == 0)
                {
                    AddToOrphans(node);                
                }
                node->m_f_diff -= bottleneck;
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::AdoptOrphans()
            {
                Node *node, *head, *arc_head;
                Size d, d_min = 0;
                Size parent_arc;
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

                    // Cached node grid index
                    Size nidx = MASK ? m_fw_idx[node - m_node_list.Begin()] : 0;

                    // Adoption
                    parent_arc = m_nb.Elements();

                    TCAP *arc_cap = &ArcCap(node, 0);
                    for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                    {
                        if (*arc_cap >= 0)
                        {
                            arc_head = Head(node, nidx, i);
                            cap = (node->m_origin == Source) ? ArcCap(arc_head, Sister(i)) : *arc_cap;

                            if (cap > 0)
                            {                                
                                if (node->m_origin == arc_head->m_origin)
                                {
                                    // Find the origin of given node
                                    d = 1;
                                    head = arc_head;
                                    while (head->m_dist > 1 && head->m_next_orphan == NULL && head->m_ts < m_time)
                                    {
                                        head = ParentNode(head);
                                        d++;
                                    }

                                    if (head->m_next_orphan == NULL)
                                    {
                                        // Ok, originates from the same tree
                                        d += head->m_dist;
                                        if (d < d_min || parent_arc == m_nb.Elements())
                                        {
                                            d_min = d;
                                            parent_arc = i;
                                        }

                                        // Set marks along the path
                                        for (head = arc_head; 
                                            head->m_ts != m_time && head->m_dist > 1; 
                                            head = ParentNode(head))
                                        {
                                            d--;
                                            head->m_ts = m_time;
                                            head->m_dist = d;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Remove from orphans
                    node->m_next_orphan = NULL;

                    if (parent_arc < m_nb.Elements())
                    {
                        // Parent was found in the same tree
                        node->m_parent_arc = (typename CommonBase<N,TFLOW,TTCAP,TCAP>::NbIndexType)parent_arc;
                        node->m_dist = d_min;
                        node->m_ts = m_time;
                    }
                    else
                    {
                        // No parent found
                        TCAP *arc_cap = &ArcCap(node, 0);
                        for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                        {
                            if (*arc_cap >= 0)
                            {
                                head = Head(node, nidx, i);
                                if (node->m_origin == head->m_origin)
                                {
                                    if (head->m_dist > 1 && ParentNode(head) == node)
                                    {
                                        AddToOrphans(head);
                                    }
                                    cap = (node->m_origin == Source) ? ArcCap(head, Sister(i)) : *arc_cap;
                                    if (cap > 0)
                                    {
                                        AddToActive(head);
                                    }
                                }
                            }
                        }

                        // Add node to free nodes, leave only active flag if present
                        node->m_origin = Free;
                    }
                }
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    Origin Kohli<N,TFLOW,TTCAP,TCAP,MASK>::NodeOrigin(Size node) const
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

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void Kohli<N,TFLOW,TTCAP,TCAP,MASK>::Dispose()
		    {
			    m_node_list.Dispose();
                m_fw_idx.Dispose();
                m_stage = 0;
                this->DisposeBase();
		    }

		    /***********************************************************************************/

		    // Explicit instantiations
            /** @cond */
		    template class GC_DLL_EXPORT Kohli<2,Int32,Int32,Int32,false>;
		    template class GC_DLL_EXPORT Kohli<2,Float32,Float32,Float32,false>;
		    template class GC_DLL_EXPORT Kohli<2,Float64,Float64,Float64,false>;
		    template class GC_DLL_EXPORT Kohli<3,Int32,Int32,Int32,false>;
		    template class GC_DLL_EXPORT Kohli<3,Float32,Float32,Float32,false>;
		    template class GC_DLL_EXPORT Kohli<3,Float64,Float64,Float64,false>;
		    
            template class GC_DLL_EXPORT Kohli<2,Int32,Int32,Int32,true>;
		    template class GC_DLL_EXPORT Kohli<2,Float32,Float32,Float32,true>;
		    template class GC_DLL_EXPORT Kohli<2,Float64,Float64,Float64,true>;
		    template class GC_DLL_EXPORT Kohli<3,Int32,Int32,Int32,true>;
		    template class GC_DLL_EXPORT Kohli<3,Float32,Float32,Float32,true>;
		    template class GC_DLL_EXPORT Kohli<3,Float64,Float64,Float64,true>;
            /** @endcond */

            /***********************************************************************************/
        }
    }
}
