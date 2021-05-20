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
    Label preserving maximum flow algorithm for general directed graphs.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include <set>
#include "../../Math/Basic.h"
#include "../../System/Format.h"
#include "../../System/IndexOutOfRangeException.h"
#include "../../System/InvalidOperationException.h"
#include "../../System/NotImplementedException.h"
#include "../../Algo/Geometry/DistanceTransform.h"
#include "DanekLabels.h"

namespace Gc {
namespace Flow {
namespace Grid {
/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
void DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::Init(const Math::Algebra::Vector<N, Size> & dim,
                                                         const Energy::Neighbourhood<N, Int32> & nb)
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

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
void DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::InitMask(const Math::Algebra::Vector<N, Size> & dim,
                                                             const Energy::Neighbourhood<N, Int32> & nb,
                                                             const System::Collection::IArrayMask<N> & mask)
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

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
void DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::SetArcCap(Size node, Size arc, TCAP cap)
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

    TCAP * arc_cap = m_arc_cap.Begin() + arc + node * m_nb.Elements();
    if (*arc_cap >= 0)
    {
        *arc_cap = cap;
    }
}

/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
void DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::SetTerminalArcCap(Size node, TTCAP csrc, TTCAP csnk)
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

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
void DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::SetInitialLabelingRef(const System::Collection::Array<N, LAB> & ilab)
{
    m_ilab = &ilab;

    if (!ilab.IsEmpty())
    {
        // Compute distance transforms for all labels
        m_dmap.Resize(ilab.Dimensions());
        m_dmap.ZeroFillMemory();

        for (Size i = 0; i < ilab.Elements(); i++)
        {
            if (!m_dmap[i]) // Label ilab[i] has not yet been processed
            {
                Algo::Geometry::DistanceTransform::CityBlockLocal(ilab, ilab[i], m_dmap);
            }
        }
    }
    else
    {
        m_dmap.Dispose();
    }
}

/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
TFLOW DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::FindMaxFlow()
{
    if (m_stage != 1)
    {
        throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                                                "Call Init method before computing maximum flow.");
    }

    if (m_ilab == nullptr || (m_ilab->Dimensions() != this->m_dim))
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
    m_q_orphan.m_first = m_q_orphan.m_last = nullptr;

    // Init distance map
    for (Size i = 0; i < m_node_list.Elements(); i++)
    {
        Size idx = MASK ? m_fw_idx[i] : i;
        m_node_list[i].m_dist = m_dmap[idx];
    }

    // Find nodes connected to the source and to the sink
    for (Size i = 0; i < m_node_list.Elements(); i++)
    {
        Node * node = &m_node_list[i];

        Size idx = MASK ? m_fw_idx[i] : i;
        node->m_label = (*m_ilab)[idx];
        node->m_tag = (node->m_label > 0) ? 1 : 0; // Background/foreground

        if (node->m_tr_cap > 0)
        {
            node->m_parent_arc = m_nb.Elements(); // No parent, connected to a terminal
            m_bp[node->m_tag].InsertNode(node);   // Active + Source + Fg/Bg
        }
        else if (node->m_tr_cap < 0)
        {
            node->m_parent_arc = m_nb.Elements(); // No parent, connected to a terminal
            node->m_tag += 2;                     // Active + Sink + Fg/Bg
            m_bp[node->m_tag].InsertNode(node);
        }
        else
        {
            node->m_tag += 8; // Free + Fg/Bg
        }
    }

    // Init priority queues
    for (Size i = 0; i < 4; i++)
    {
        m_bp[i].Init();
    }

    // Label preserving max-flow
    Node * n;
    while (!m_bp[0].IsEmpty() || !m_bp[1].IsEmpty() || !m_bp[2].IsEmpty() || !m_bp[3].IsEmpty())
    {
        // Get next active node
        n = m_bp[0].FrontNode();

        for (Size i = 1; i < 4; i++)
        {
            Node * mdn = m_bp[i].FrontNode();

            if (n == nullptr || (mdn != nullptr && n->m_dist < mdn->m_dist))
            {
                n = mdn;
            }
        }

        while (n->m_tag < 8) // Not free
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
                n->m_tag ^= 4;  // XOR -> flip active bit
                break;          // Became passive
            }
        }
    }

    m_stage = 2;

    return m_flow;
}

/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
bool DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::GrowTrees(Node * n, Node *& n_src, Node *& n_snk, Size & arc)
{
    // Cached node grid index
    Size nidx = MASK ? m_fw_idx[n - m_node_list.Begin()] : 0;

    TCAP * arc_cap = &ArcCap(n, 0);

    for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
    {
        if (*arc_cap >= 0)
        {
            Node * head = Head(n, nidx, i);
            TCAP cap = ((n->m_tag & 2) == 0) ? *arc_cap : ArcCap(head, Sister(i));

            if (cap > 0)
            {
                if (head->m_tag >= 8) // Free node
                {
                    if (n->m_label != head->m_label)
                    {
                        head->m_dist = n->m_dist - 1;
                    }

                    head->m_parent_arc = Sister(i);
                    head->m_tag = n->m_tag;
                    head->m_label = n->m_label;

                    EnqueueNode(head);
                }
                else if ((n->m_tag & 2) == (head->m_tag & 2)) // Same tree
                {
                    if ((n->m_tag & 1) != (head->m_tag & 1) &&   // Fg/bg mismatch
                        ((n->m_tag & 2) >> 1) == (n->m_tag & 1)) // Should belong to our tree
                    {
                        if (head->m_tag < 4)
                        {
                            DequeueNode(head);
                        }
                        head->m_dist = n->m_dist - 1;
                        head->m_tag = n->m_tag;
                        head->m_label = n->m_label;
                        EnqueueNode(head);
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

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
void DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::Augment(Node * n_src, Node * n_snk, Size arc)
{
    TCAP bottleneck = ArcCap(n_src, arc);
    Node * node;

    // Find bottleneck in the source tree
    for (node = n_src; node->m_parent_arc < m_nb.Elements();)
    {
        Node * parent = ParentNode(node);
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
    for (node = n_src; node->m_parent_arc < m_nb.Elements();)
    {
        Node * parent = ParentNode(node);
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
    for (node = n_snk; node->m_parent_arc < m_nb.Elements();)
    {
        Node * parent = ParentNode(node);
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

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
void DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::AdoptOrphans()
{
    Node *node, *head;
    Size dist, d_min = 0;
    Size parent_arc;
    TCAP cap;

    while (m_q_orphan.m_first != nullptr)
    {
        // Pop orphan node
        node = m_q_orphan.m_first;
        if (m_q_orphan.m_first == m_q_orphan.m_last)
        {
            m_q_orphan.m_first = nullptr;
            m_q_orphan.m_last = nullptr;
        }
        else
        {
            m_q_orphan.m_first = node->m_next_orphan;
        }

        // Cached node grid index
        Size nidx = MASK ? m_fw_idx[node - m_node_list.Begin()] : 0;

        // Adoption
        parent_arc = m_nb.Elements();

        TCAP * arc_cap = &ArcCap(node, 0);
        for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
        {
            if (*arc_cap >= 0)
            {
                head = Head(node, nidx, i);
                cap = ((node->m_tag & 2) == 0) ? ArcCap(head, Sister(i)) : *arc_cap;

                if (cap > 0)
                {
                    if (head->m_tag < 8 &&                        // Not free
                        (node->m_tag & 3) == (head->m_tag & 3) && // Same subtree
                        node->m_label == head->m_label)           // Same label
                    {
                        // Check origin
                        dist = 0;

                        while (head->m_parent_arc < m_nb.Elements() && head->m_next_orphan == nullptr)
                        {
                            head = ParentNode(head);
                            dist++;
                        }

                        if (head->m_next_orphan == nullptr) // Originates from terminal?
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
        node->m_next_orphan = nullptr;
        node->m_parent_arc = parent_arc; // New parent

        if (parent_arc == m_nb.Elements())
        {
            // No parent found
            TCAP * arc_cap = &ArcCap(node, 0);
            for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
            {
                if (*arc_cap >= 0)
                {
                    head = Head(node, nidx, i);
                    if (head->m_parent_arc < m_nb.Elements() && ParentNode(head) == node)
                    {
                        AddToOrphans(head);
                    }
                    if (head->m_tag < 8 && (head->m_tag & 4) != 0) // Not free, passive -> active
                    {
                        TCAP cap = ((head->m_tag & 2) == 0) ? ArcCap(head, Sister(i)) : *arc_cap;
                        if (cap > 0)
                        {
                            head->m_tag ^= 4;
                            EnqueueNode(head);
                        }
                    }
                }
            }

            // Remove from priority queue
            if (node->m_tag < 4)
            {
                DequeueNode(node);
            }

            // Set as free node
            node->m_tag |= 8;
        }
    }
}

/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
Origin DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::NodeOrigin(Size node) const
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

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
LAB DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::NodeLabel(Size node) const
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

    return m_node_list[node].m_label;
}

/***********************************************************************************/

template <Size N, class TFLOW, class TTCAP, class TCAP, class LAB, bool MASK>
void DanekLabels<N, TFLOW, TTCAP, TCAP, LAB, MASK>::Dispose()
{
    m_node_list.Dispose();
    m_fw_idx.Dispose();
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
template class GC_DLL_EXPORT DanekLabels<2, Int32, Int32, Int32, Uint8, false>;
template class GC_DLL_EXPORT DanekLabels<2, Float32, Float32, Float32, Uint8, false>;
template class GC_DLL_EXPORT DanekLabels<2, Float64, Float64, Float64, Uint8, false>;
template class GC_DLL_EXPORT DanekLabels<3, Int32, Int32, Int32, Uint8, false>;
template class GC_DLL_EXPORT DanekLabels<3, Float32, Float32, Float32, Uint8, false>;
template class GC_DLL_EXPORT DanekLabels<3, Float64, Float64, Float64, Uint8, false>;

template class GC_DLL_EXPORT DanekLabels<2, Int32, Int32, Int32, Uint8, true>;
template class GC_DLL_EXPORT DanekLabels<2, Float32, Float32, Float32, Uint8, true>;
template class GC_DLL_EXPORT DanekLabels<2, Float64, Float64, Float64, Uint8, true>;
template class GC_DLL_EXPORT DanekLabels<3, Int32, Int32, Int32, Uint8, true>;
template class GC_DLL_EXPORT DanekLabels<3, Float32, Float32, Float32, Uint8, true>;
template class GC_DLL_EXPORT DanekLabels<3, Float64, Float64, Float64, Uint8, true>;

template class GC_DLL_EXPORT DanekLabels<2, Int32, Int32, Int32, Uint16, false>;
template class GC_DLL_EXPORT DanekLabels<2, Float32, Float32, Float32, Uint16, false>;
template class GC_DLL_EXPORT DanekLabels<2, Float64, Float64, Float64, Uint16, false>;
template class GC_DLL_EXPORT DanekLabels<3, Int32, Int32, Int32, Uint16, false>;
template class GC_DLL_EXPORT DanekLabels<3, Float32, Float32, Float32, Uint16, false>;
template class GC_DLL_EXPORT DanekLabels<3, Float64, Float64, Float64, Uint16, false>;

template class GC_DLL_EXPORT DanekLabels<2, Int32, Int32, Int32, Uint16, true>;
template class GC_DLL_EXPORT DanekLabels<2, Float32, Float32, Float32, Uint16, true>;
template class GC_DLL_EXPORT DanekLabels<2, Float64, Float64, Float64, Uint16, true>;
template class GC_DLL_EXPORT DanekLabels<3, Int32, Int32, Int32, Uint16, true>;
template class GC_DLL_EXPORT DanekLabels<3, Float32, Float32, Float32, Uint16, true>;
template class GC_DLL_EXPORT DanekLabels<3, Float64, Float64, Float64, Uint16, true>;
/** @endcond */
}
}
} // namespace Gc::Flow::Grid
