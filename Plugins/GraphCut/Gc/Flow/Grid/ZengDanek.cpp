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
    Topology preserving maximum flow algorithm for general directed graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include "../../Math/Basic.h"
#include "../../System/Format.h"
#include "../../System/IndexOutOfRangeException.h"
#include "../../System/InvalidOperationException.h"
#include "../../System/NotImplementedException.h"
#include "../../Algo/Geometry/DistanceTransform.h"
#include "../../Algo/Geometry/SimplePoint.h"
#include "../../Data/BorderIterator.h"
#include "ZengDanek.h"

namespace Gc
{
	namespace Flow
	{
        namespace Grid
        {
		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::Init(const Math::Algebra::Vector<N,Size> &dim, 
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
		    void ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::InitMask(const Math::Algebra::Vector<N,Size> &dim, 
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
		    void ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::SetArcCap(Size node, Size arc, TCAP cap)
		    {
                if (m_stage != 1)
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Call Init method before adding arcs.");
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
		    void ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::SetTerminalArcCap(Size node, TTCAP csrc, TTCAP csnk)
            {
                if (m_stage != 1)
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Call Init method before adding arcs.");
                }

                if (csrc < 0 || csnk < 0)
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

                m_node_list[node].m_tr_cap = csrc - csnk;
                m_flow += Math::Min(csrc, csnk);
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::SetInitialLabelingRef
                (const System::Collection::Array<N,bool> &ilab)
            {
                m_ilab = &ilab;

                if (!ilab.IsEmpty())
                {
                    // Compute distance transforms
                    Algo::Geometry::DistanceTransform::CityBlock(ilab, true, m_dmap);
                    Algo::Geometry::DistanceTransform::CityBlockLocal(ilab, true, m_dmap);
                }
                else
                {
                    m_dmap.Dispose();
                }
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    TFLOW ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::FindMaxFlow()
		    {
                if (m_stage != 1)
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Call Init method before computing maximum flow.");
                }

                if (m_ilab == NULL || (m_ilab->Dimensions() != this->m_dim))
                {
                    throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                        "Initial labeling is not available or has incompatible dimensions.");
                }

                // Init priority queues
                for (Size i = 0; i < 4; i++)
                {
                    m_bp[i].Dispose();
                }

                // Init orphan queue
                m_q_orphan.m_first = m_q_orphan.m_last = NULL;

                // Init distance map
                for (Node *node = m_node_list.Begin(); node != m_node_list.End(); ++node)
                {
                    node->m_dist = m_dmap[NodeFwIdx(node)];
                }

                // Copy initial labels
                m_clab.Resize(m_ilab->Dimensions());
                for (Size i = 0; i < m_clab.Elements(); i++)
                {
                    m_clab[i] = (*m_ilab)[i] ? 1 : 0;
                }

                // Find nodes connected to the source and to the sink
                for (Node *node = m_node_list.Begin(); node != m_node_list.End(); ++node)
                {
                    if ((*m_ilab)[NodeFwIdx(node)])
                    {
                        node->m_tag |= 1; // Foreground label (1), otherwise background (0)
                    }

                    if (node->m_tr_cap > 0)
                    {
                        node->m_parent_arc = m_nb.Elements(); // No parent, connected to a terminal
                        m_bp[node->m_tag & 3].InsertNode(node); // Active + Source + Label
                    }
                    else if (node->m_tr_cap < 0)
                    {
                        node->m_parent_arc = m_nb.Elements(); // No parent, connected to a terminal
                        node->m_tag |= 2; // Active + Sink + Label
                        m_bp[node->m_tag & 3].InsertNode(node);
                    } 
                    else
                    {                        
                        node->m_tag |= 8; // Free + Label
                    }
                }

                // Init priority queues
                for (Size i = 0; i < 4; i++)
                {
                    m_bp[i].Init();
                }

                // Topology preserving max-flow
                while (!m_bp[0].IsEmpty() || !m_bp[1].IsEmpty() || !m_bp[2].IsEmpty() || !m_bp[3].IsEmpty())
                {           
                    // Get next active node 
                    Node *n = m_bp[0].FrontNode();

                    for (Size i = 1; i < 4; i++)
                    {
                        Node *mdn = m_bp[i].FrontNode();

                        if (n == NULL || (mdn != NULL && n->m_dist < mdn->m_dist))
                        {
                            n = mdn;
                        }
                    }

                    while (!(n->m_tag & 8)) // Not free
                    {
                        Node *n_src, *n_snk;
                        Size arc;

                        if (GrowTrees(n, n_src, n_snk, arc))
                        {
                            // Augment and adopt orphans
                            Augment(n_src, n_snk, arc);
                            AdoptOrphans();
                        }
                        else
                        {
                            DequeueNode(n); // !!!!
                            n->m_tag ^= 4; // XOR -> flip active bit
                            break; // Became passive
                        }
                    }
                }

                m_stage = 2;

			    return m_flow;
		    }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    bool ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::GrowTrees(Node *n, Node *&n_src, Node *&n_snk, Size &arc)
            {
                // Cached node grid index
                Size nidx = MASK ? m_fw_idx[n - m_node_list.Begin()] : 0;

                TCAP *arc_cap = &ArcCap(n, 0);

                for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                {
                    if (*arc_cap >= 0)
                    {
                        Node *head = Head(n, nidx, i);
                        TCAP cap = ((n->m_tag & 2) == 0) ? *arc_cap : ArcCap(head, Sister(i));

                        if (cap > 0)
                        {
                            if ((head->m_tag & 8) != 0) // Free node
                            {
                                if ((n->m_tag & 1) == (head->m_tag & 1))
                                {
                                    head->m_parent_arc = Sister(i);
                                    head->m_tag = n->m_tag;
                                    EnqueueNode(head);
                                }
                                else if (IsSimple(head))
                                {
                                    head->m_dist = n->m_dist - 1;
                                    head->m_parent_arc = Sister(i);
                                    head->m_tag = n->m_tag & 15;
                                    EnqueueNode(head);

                                    // Label changed, update m_clab
                                    m_clab[NodeFwIdx(head)] = head->m_tag & 1;
                                }
                            }
                            else if ((n->m_tag & 2) == (head->m_tag & 2)) // Same tree
                            {
                                if ((n->m_tag & 1) != (head->m_tag & 1)) // Different labels
                                {
                                    // Head should belong to our tree and is simple
                                    if (((n->m_tag & 2) >> 1) == (n->m_tag & 1) && IsSimple(head))
                                    {
                                        // Head is active
                                        if (!(head->m_tag & 4))
                                        {
                                            DequeueNode(head);
                                        }
                                        head->m_dist = n->m_dist - 1;
                                        head->m_tag = n->m_tag;
                                        EnqueueNode(head);

                                        // Label changed, update m_clab
                                        m_clab[NodeFwIdx(head)] = head->m_tag & 1;
                                    }
                                }
                            }
                            else // Different tree -> augmenting path
                            {
                                if ((n->m_tag & 2) == 0)
                                {
                                    n_src = n;
                                    n_snk = head;
                                    arc = i;
                                }
                                else
                                {
                                    n_src = head;
                                    n_snk = n;
                                    arc = Sister(i);
                                }
                                return true;
                            }
                        }
                    }
                }

                return false;
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::Augment(Node *n_src, Node *n_snk, Size arc)
            {
                TCAP bottleneck = ArcCap(n_src, arc);
                Node *node;

                // Find bottleneck in the source tree
                for (node = n_src; node->m_parent_arc < m_nb.Elements(); )
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
                for (node = n_snk; node->m_parent_arc < m_nb.Elements(); node = ParentNode(node))
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
                for (node = n_src; node->m_parent_arc < m_nb.Elements(); )
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

                // Augment in the sink tree
                for (node = n_snk; node->m_parent_arc < m_nb.Elements(); )
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
            }

		    /***********************************************************************************/

            template <Size N,class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::AdoptOrphans()
            {
                Node *node, *head;
                Size dist, d_min = 0;
                Size parent_arc;
                TCAP cap;

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
                            head = Head(node, nidx, i);
                            cap = ((node->m_tag & 2) == 0) ? ArcCap(head, Sister(i)) : *arc_cap;

                            if (cap > 0)
                            { 
                                if (!(head->m_tag & 8) && (node->m_tag & 3) == (head->m_tag & 3)) // Same subtree
                                {
                                    // Check origin
                                    dist = 0;

                                    while (head->m_parent_arc < m_nb.Elements() && head->m_next_orphan == NULL)
                                    {
                                        head = ParentNode(head);
                                        dist++;
                                    }

                                    if (head->m_next_orphan == NULL) // Originates from terminal?
                                    {
                                        if (!dist)
                                        {
                                            parent_arc = i;
                                            break;
                                        }

                                        if (dist < d_min || parent_arc == m_nb.Elements())
                                        {
                                            d_min = dist;
                                            parent_arc = i;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Remove from orphans
                    node->m_next_orphan = NULL;
                    node->m_parent_arc = parent_arc; // New parent

                    if (parent_arc == m_nb.Elements())
                    {
                        // No parent found    
                        TCAP *arc_cap = &ArcCap(node, 0);
                        for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                        {
                            if (*arc_cap >= 0)
                            {
                                head = Head(node, nidx, i);
                                if (head->m_parent_arc < m_nb.Elements() && ParentNode(head) == node)
                                {
                                    AddToOrphans(head);
                                }
                                if (!(head->m_tag & 8) && (head->m_tag & 4) != 0) // Not free, passive -> active
                                {
                                    TCAP cap = ((head->m_tag & 2) == 0) ? ArcCap(head, Sister(i)) : *arc_cap;
                                    if (cap > 0)
                                    {
                                        head->m_tag ^= 4; // Flip active bit
                                        EnqueueNode(head);
                                    }
                                }
                            }
                        }

                        // Remove from priority queue (if active and not free)
                        if (!(node->m_tag & 12))
                        {
                            DequeueNode(node);
                        }

                        // Set as free node
                        node->m_tag |= 8;
                    }
                }
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    Origin ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::NodeOrigin(Size node) const
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

                return (!(m_node_list[node].m_tag & 1)) ? Source : Sink;
		    }

		    /***********************************************************************************/


		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    bool ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::IsSimple(Node *n) const
            {
                Size idx = NodeFwIdx(n);
                const Uint8 *lp = &m_clab[idx];
                
                const Math::Algebra::Vector<N,Size> &srd(m_clab.Stride());
                const Math::Algebra::Vector<N,Size> &sz(m_clab.Dimensions());
                
                Math::Algebra::Vector<N,Size> p;
                m_clab.Coord(idx, p);

                if (N == 2)
                {
                    Uint32 val = 0;

                    if (p[0] == 0 || p[0] == sz[0]-1 || p[1] == 0 || p[1] == sz[1]-1)
                    {
                        // Border node
                        if (p[1] > 0)
                        {
                            if (p[0] > 0) val += (*(lp - srd[1] - 1)) << 7;
                            val += (*(lp - srd[1])) << 6;
                            if (p[0] < sz[0]-1) val += (*(lp - srd[1] + 1)) << 5;
                        }
                        
                        if (p[0] > 0) val += (*(lp - 1)) << 4;
                        if (p[0] < sz[0]-1) val += (*(lp + 1)) << 3;

                        if (p[1] < sz[1]-1)
                        {
                            if (p[0] > 0) val += (*(lp + srd[1] - 1)) << 2;
                            val += (*(lp + srd[1])) << 1;
                            if (p[0] < sz[0]-1) val += *(lp + srd[1] + 1);
                        }
                    }
                    else
                    {
                        // Non-border node
                        val = ((*(lp - srd[1] - 1)) << 7) +
                              ((*(lp - srd[1]    )) << 6) +
                              ((*(lp - srd[1] + 1)) << 5) +
                              ((*(lp          - 1)) << 4) +
                              ((*(lp          + 1)) << 3) +
                              ((*(lp + srd[1] - 1)) << 2) +
                              ((*(lp + srd[1]    )) << 1) +
                               (*(lp + srd[1] + 1));
                    }

                    return Algo::Geometry::IsSimplePoint2D(val);
                }
                else if (N == 3)
                {
                    Uint32 val = 0;

                    if (!p[0] || !p[1] || !p[2] || p[0] == sz[0]-1 || p[1] == sz[1]-1 || p[2] == sz[2]-1)
                    {
                        // Border node
                        if (p[2] > 0)
                        {
                            if (p[1] > 0)
                            {
                                if (p[0] > 0) val += *(lp - srd[2] - srd[1] - 1);
                                val += (*(lp - srd[2] - srd[1])) << 1;
                                if (p[0] < sz[0]-1) val += (*(lp - srd[2] - srd[1] + 1)) << 2;
                            }

                            if (p[0] > 0) val += (*(lp - srd[2] - 1)) << 3;
                            val += (*(lp - srd[2])) << 4;
                            if (p[0] < sz[0]-1) val += (*(lp - srd[2] + 1)) << 5;

                            if (p[1] < sz[1]-1)
                            {
                                if (p[0] > 0) val += (*(lp - srd[2] + srd[1] - 1)) << 6;
                                val += (*(lp - srd[2] + srd[1])) << 7;
                                if (p[0] < sz[0]-1) val += (*(lp - srd[2] + srd[1] + 1)) << 8;
                            }
                        }

                        if (p[1] > 0)
                        {
                            if (p[0] > 0) val += (*(lp - srd[1] - 1)) << 9;
                            val += (*(lp - srd[1])) << 10;
                            if (p[0] < sz[0]-1) val += (*(lp - srd[1] + 1)) << 11;
                        }

                        if (p[0] > 0) val += (*(lp - 1)) << 12;
                        if (p[0] < sz[0]-1) val += (*(lp + 1)) << 13;

                        if (p[1] < sz[1]-1)
                        {
                            if (p[0] > 0) val += (*(lp + srd[1] - 1)) << 14;
                            val += (*(lp + srd[1])) << 15;
                            if (p[0] < sz[0]-1) val += (*(lp + srd[1] + 1)) << 16;
                        }

                        if (p[2] < sz[2]-1)
                        {
                            if (p[1] > 0)
                            {
                                if (p[0] > 0) val += (*(lp + srd[2] - srd[1] - 1)) << 17;
                                val += (*(lp + srd[2] - srd[1]    )) << 18;
                                if (p[0] < sz[0]-1) val += (*(lp + srd[2] - srd[1] + 1)) << 19;
                            }

                            if (p[0] > 0) val += (*(lp + srd[2] - 1)) << 20;
                            val += (*(lp + srd[2])) << 21;
                            if (p[0] < sz[0]-1) val += (*(lp + srd[2]      + 1)) << 22;

                            if (p[1] < sz[1]-1)
                            {
                                if (p[0] > 0) val += (*(lp + srd[2] + srd[1] - 1)) << 23;
                                val += (*(lp + srd[2] + srd[1])) << 24;
                                if (p[0] < sz[0]-1) val += (*(lp + srd[2] + srd[1] + 1)) << 25;
                            }
                        }
                    }
                    else
                    {
                        // Non-border node
                        val = 
                            ((*(lp - srd[2] - srd[1] - 1))      ) |
                            ((*(lp - srd[2] - srd[1]    )) << 1 ) |
                            ((*(lp - srd[2] - srd[1] + 1)) << 2 ) |
                            ((*(lp - srd[2]          - 1)) << 3 ) |
                            ((*(lp - srd[2]             )) << 4 ) |
                            ((*(lp - srd[2]          + 1)) << 5 ) |
                            ((*(lp - srd[2] + srd[1] - 1)) << 6 ) |
                            ((*(lp - srd[2] + srd[1]    )) << 7 ) |
                            ((*(lp - srd[2] + srd[1] + 1)) << 8 ) |
                            ((*(lp          - srd[1] - 1)) << 9 ) |
                            ((*(lp          - srd[1]    )) << 10) |
                            ((*(lp          - srd[1] + 1)) << 11) |
                            ((*(lp                   - 1)) << 12) |
                            ((*(lp                   + 1)) << 13) |
                            ((*(lp          + srd[1] - 1)) << 14) |
                            ((*(lp          + srd[1]    )) << 15) |
                            ((*(lp          + srd[1] + 1)) << 16) |
                            ((*(lp + srd[2] - srd[1] - 1)) << 17) |
                            ((*(lp + srd[2] - srd[1]    )) << 18) |
                            ((*(lp + srd[2] - srd[1] + 1)) << 19) |
                            ((*(lp + srd[2]          - 1)) << 20) |
                            ((*(lp + srd[2]             )) << 21) |
                            ((*(lp + srd[2]          + 1)) << 22) |
                            ((*(lp + srd[2] + srd[1] - 1)) << 23) |
                            ((*(lp + srd[2] + srd[1]    )) << 24) |
                            ((*(lp + srd[2] + srd[1] + 1)) << 25);
                    }

                    return Algo::Geometry::IsSimplePoint3D(val);
                }
                else
                {
                    throw System::NotImplementedException(__FUNCTION__, __LINE__, "Current implementation "
                        "supports only 2D and 3D grids.");
                }

                return false;
            }

		    /***********************************************************************************/

		    template <Size N, class TFLOW, class TTCAP, class TCAP, bool MASK>
		    void ZengDanek<N,TFLOW,TTCAP,TCAP,MASK>::Dispose()
		    {
			    m_node_list.Dispose();
                m_fw_idx.Dispose();
                m_clab.Dispose();
                m_stage = 0;

                for (Size i = 0; i < 4; i++)
                {
                    m_bp[i].Dispose();
                }

                this->DisposeBase();
            }

		    /***********************************************************************************/

		    // Explicit instantiations
            /** @cond */
		    template class GC_DLL_EXPORT ZengDanek<2,Int32,Int32,Int32,false>;
		    template class GC_DLL_EXPORT ZengDanek<2,Float32,Float32,Float32,false>;
		    template class GC_DLL_EXPORT ZengDanek<2,Float64,Float64,Float64,false>;
		    template class GC_DLL_EXPORT ZengDanek<3,Int32,Int32,Int32,false>;
		    template class GC_DLL_EXPORT ZengDanek<3,Float32,Float32,Float32,false>;
		    template class GC_DLL_EXPORT ZengDanek<3,Float64,Float64,Float64,false>;

		    template class GC_DLL_EXPORT ZengDanek<2,Int32,Int32,Int32,true>;
		    template class GC_DLL_EXPORT ZengDanek<2,Float32,Float32,Float32,true>;
		    template class GC_DLL_EXPORT ZengDanek<2,Float64,Float64,Float64,true>;
		    template class GC_DLL_EXPORT ZengDanek<3,Int32,Int32,Int32,true>;
		    template class GC_DLL_EXPORT ZengDanek<3,Float32,Float32,Float32,true>;
		    template class GC_DLL_EXPORT ZengDanek<3,Float64,Float64,Float64,true>;
            /** @endcond */
        }
	}
}
