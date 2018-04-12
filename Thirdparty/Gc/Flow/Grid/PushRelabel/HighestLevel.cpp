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
    @date 2010
*/

#include "../../../Math/Basic.h"
#include "../../../System/Format.h"
#include "../../../System/IndexOutOfRangeException.h"
#include "../../../System/InvalidOperationException.h"
#include "HighestLevel.h"

namespace Gc
{
	namespace Flow
	{
        namespace Grid
        {
            namespace PushRelabel
            {
		        /***********************************************************************************/

		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        void HighestLevel<N,TFLOW,TCAP,MASK>::Init(const Math::Algebra::Vector<N,Size> &dim, 
                    const Energy::Neighbourhood<N,Int32> &nb)
		        {
                    this->ValidateParameters(dim, nb);

			        Dispose();

                    m_node_list.Resize(dim.Product());
                    m_nodes = m_node_list.Elements() + 2; // +Sink and source
                    m_layer_list.Resize(m_nodes);
                    m_arc_cap.Resize(m_node_list.Elements() * nb.Elements());

                    m_max_rank = 0;
                    m_flow = 0;
                    m_stage = 1;

                    // Initialize node array
                    m_node_list.ZeroFillMemory();
                    m_layer_list.ZeroFillMemory();

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

		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        void HighestLevel<N,TFLOW,TCAP,MASK>::InitMask(const Math::Algebra::Vector<N,Size> &dim, 
                    const Energy::Neighbourhood<N,Int32> &nb, 
                    const System::Collection::IArrayMask<N> &mask)
		        {
                    if (MASK)
                    {
                        this->ValidateParameters(dim, nb);

                        Dispose();

                        m_node_list.Resize(mask.UnmaskedElements());
                        m_nodes = m_node_list.Elements() + 2; // +Sink and source
                        m_layer_list.Resize(m_nodes);
                        m_arc_cap.Resize(m_node_list.Elements() * nb.Elements());

                        m_max_rank = 0;
                        m_flow = 0;
                        m_stage = 1;

                        // Initialize node array
                        m_node_list.ZeroFillMemory();
                        m_layer_list.ZeroFillMemory();

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

		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        void HighestLevel<N,TFLOW,TCAP,MASK>::SetArcCap(Size node, Size arc, TCAP cap)
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

		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        void HighestLevel<N,TFLOW,TCAP,MASK>::SetTerminalArcCap(Size node, TFLOW csrc, TFLOW csnk)
                {
                    if (m_stage != 1)
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

                    m_node_list[node].m_excess = csrc - csnk;
                    m_flow += Math::Min(csrc, csnk);
                }

		        /***********************************************************************************/

		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        TFLOW HighestLevel<N,TFLOW,TCAP,MASK>::FindMaxFlow()
		        {
                    if (m_stage != 1)
                    {
                        throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                            "Call Init method before computing maximum flow.");
                    }                

                    Size rl_count = 0, rl_threshold = (Size) (m_glob_rel_freq * m_nodes);
                    Size next_layer_rank, min_rank, arc_idx, min_arc = 0;
                    Node *node, *head;
                    Layer *lay;
                    TCAP bottleneck, *arc_cap;

                    GlobalUpdate();

                    while (m_max_active_rank >= m_min_active_rank)
                    {
                        // Pop node from the top layer
                        node = m_layer_list[m_max_active_rank].m_first_active;                        
                        if (node == NULL)
                        {
                            m_max_active_rank--;
                            continue;
                        }

                        m_layer_list[m_max_active_rank].m_first_active = node->m_lay_next;
                        Size nidx = MASK ? m_fw_idx[node - m_node_list.Begin()] : 0;

                        // DISCHARGE node
                        while (node->m_rank < m_nodes)
                        {
                            lay = &m_layer_list[node->m_rank];
                            next_layer_rank = node->m_rank - 1;

                            // PUSH excess
                            arc_cap = &ArcCap(node, node->m_current);
                            arc_idx = node->m_current;

                            while (arc_idx < m_nb.Elements())
                            {
                                if (*arc_cap > 0)
                                {
                                    head = Head(node, nidx, arc_idx);

                                    if (head->m_rank == next_layer_rank)
                                    {
                                        bottleneck = Math::Min(node->m_excess, *arc_cap);

                                        *arc_cap -= bottleneck;
                                        ArcCap(head, Sister(arc_idx)) += bottleneck;

                                        // Zero excess and not a sink, inactive -> active
                                        if (head->m_excess <= 0)
                                        {
                                            if (head->m_excess + bottleneck > 0)
                                            {
                                                m_flow -= head->m_excess;
                                                RemoveFromInactive(lay - 1, head);
                                                AddToActive(lay - 1, head);
                                            }
                                            else
                                            {
                                                m_flow += bottleneck;
                                            }
                                        }

                                        head->m_excess += bottleneck;
                                        node->m_excess -= bottleneck;

                                        if (node->m_excess == 0)
                                        {
                                            break;
                                        }
                                    }
                                }

                                arc_idx++;
                                arc_cap++;
                            }

                            if (arc_idx < m_nb.Elements())
                            {
                                // All excess pushed away, the node becomes inactive
                                node->m_current = (typename CommonBase<N,TFLOW,TFLOW,TCAP>::NbIndexType)arc_idx;
                                AddToInactive(lay, node);
                                break;
                            }

                            // RELABEL node
                            node->m_rank = min_rank = m_nodes;
                            
                            if (lay->m_first_active == NULL && lay->m_first_inactive == NULL)
                            {
                                // Gap relabeling
                                GapRelabeling(lay, next_layer_rank);
                            }
                            else
                            {
                                // Node relabeling
                                arc_cap = &ArcCap(node, 0);
                                for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                                {
                                    if (*arc_cap > 0)
                                    {
                                        head = Head(node, nidx, i);

                                        if (head->m_rank < min_rank)
                                        {
                                            min_rank = head->m_rank;
                                            min_arc = i;
                                        }
                                    }
                                }

                                rl_count++;

                                if (++min_rank < m_nodes)
                                {
                                    node->m_rank = min_rank;
                                    node->m_current = (typename CommonBase<N,TFLOW,TFLOW,TCAP>::NbIndexType)min_arc;

                                    if (min_rank > m_max_rank)
                                    {
                                        m_max_rank = min_rank;
                                    }
                                }
                            }
                        }

                        // Check if global relabeling should be done
                        if (rl_count > rl_threshold && m_max_active_rank >= m_min_active_rank)
                        {
                            GlobalUpdate();
                            rl_count = 0;
                        }
                    }

                    // Find nodes that are connected to the sink after the last step.
                    // This is IMPORTANT, because it allows us to find which nodes are
                    // connected to the source and which to the sink.
                    GlobalUpdate();

                    m_stage = 2;

                    return m_flow;
		        }

		        /***********************************************************************************/

		        template <Size N, class TFLOW, class TCAP, bool MASK>
                void HighestLevel<N,TFLOW,TCAP,MASK>::GapRelabeling(Layer *empty_lay, Size next_layer_rank)
                {
                    Node *node;
                    Layer *lay;

                    for (lay = empty_lay + 1; lay <= m_layer_list.Begin() + m_max_rank; lay++)
                    {
                        for (node = lay->m_first_inactive; node != NULL; node = node->m_lay_next)
                        {
                            node->m_rank = m_nodes;
                        }
                        
                        lay->m_first_inactive = NULL;
                    }

                    m_max_rank = next_layer_rank;
                    m_max_active_rank = next_layer_rank;
                }

		        /***********************************************************************************/

		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        void HighestLevel<N,TFLOW,TCAP,MASK>::GlobalUpdate()
		        {
                    Node *node, *head;
                    Layer *lay;

                    // Reset layers
                    for (Size i = 0; i <= m_max_rank; i++)
                    {
                        m_layer_list[i].m_first_active = NULL;
                        m_layer_list[i].m_first_inactive = NULL;
                    }

                    m_max_rank = 0;
                    m_max_active_rank = 0;
                    m_min_active_rank = m_nodes;

                    // Initialize ranks, put nodes connected to the sink to the first layer
                    lay = &m_layer_list[1];

                    for (node = m_node_list.Begin(); node != m_node_list.End(); ++node)
                    {
                        if (node->m_excess < 0)
                        {
                            node->m_rank = 1;
                            node->m_current = 0;
                            AddToInactive(lay, node);
                        }
                        else
                        {
                            node->m_rank = m_nodes;
                        }
                    }            

                    // Breadth first search
                    for ( ; lay->m_first_active != NULL || lay->m_first_inactive != NULL; lay++)
                    {
                        m_max_rank++;

                        // Check list of active nodes on this layer
                        for (node = lay->m_first_active; node != NULL; node = node->m_lay_next)
                        {
                            TCAP *arc_cap = &ArcCap(node, 0);
                            Size nidx = MASK ? m_fw_idx[node - m_node_list.Begin()] : 0;

                            for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                            {
                                if (*arc_cap >= 0)
                                {
                                    head = Head(node, nidx, i);

                                    if (ArcCap(head, Sister(i)) > 0 && head->m_rank == m_nodes)
                                    {
                                        head->m_rank = node->m_rank + 1;
                                        head->m_current = 0;

                                        if (head->m_excess > 0)
                                        {
                                            AddToActive(lay + 1, head);
                                        }
                                        else
                                        {
                                            AddToInactive(lay + 1, head);
                                        }
                                    }
                                }
                            }
                        }

                        // Check list of inactive nodes on this layer
                        for (node = lay->m_first_inactive; node != NULL; node = node->m_lay_next)
                        {
                            TCAP *arc_cap = &ArcCap(node, 0);
                            Size nidx = MASK ? m_fw_idx[node - m_node_list.Begin()] : 0;

                            for (Size i = 0; i < m_nb.Elements(); i++, arc_cap++)
                            {
                                if (*arc_cap >= 0)
                                {
                                    head = Head(node, nidx, i);

                                    if (ArcCap(head, Sister(i)) > 0 && head->m_rank == m_nodes)
                                    {
                                        head->m_rank = node->m_rank + 1;
                                        head->m_current = 0;

                                        if (head->m_excess > 0)
                                        {
                                            AddToActive(lay + 1, head);
                                        }
                                        else
                                        {
                                            AddToInactive(lay + 1, head);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

		        /***********************************************************************************/

		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        Origin HighestLevel<N,TFLOW,TCAP,MASK>::NodeOrigin(Size node) const
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

                    // Nodes connected to the sink are those with rank less than infinity (=_nodes),
                    // i.e., those that were reached during the final global update.
                    return m_node_list[node].m_rank < m_nodes ? Sink : Source;
		        }

		        /***********************************************************************************/

		        template <Size N, class TFLOW, class TCAP, bool MASK>
		        void HighestLevel<N,TFLOW,TCAP,MASK>::Dispose()
		        {			        
			        m_node_list.Dispose();
                    m_layer_list.Dispose();
                    m_fw_idx.Dispose();
			        m_nodes = 0;
                    m_stage = 0;

                    this->DisposeBase();
		        }

		        /***********************************************************************************/

		        // Explicit instantiations
                /** @cond */
		        template class GC_DLL_EXPORT HighestLevel<2,Int32,Int32,false>;
		        template class GC_DLL_EXPORT HighestLevel<2,Float32,Float32,false>;
		        template class GC_DLL_EXPORT HighestLevel<2,Float64,Float64,false>;
		        template class GC_DLL_EXPORT HighestLevel<3,Int32,Int32,false>;
		        template class GC_DLL_EXPORT HighestLevel<3,Float32,Float32,false>;
		        template class GC_DLL_EXPORT HighestLevel<3,Float64,Float64,false>;

		        template class GC_DLL_EXPORT HighestLevel<2,Int32,Int32,true>;
		        template class GC_DLL_EXPORT HighestLevel<2,Float32,Float32,true>;
		        template class GC_DLL_EXPORT HighestLevel<2,Float64,Float64,true>;
		        template class GC_DLL_EXPORT HighestLevel<3,Int32,Int32,true>;
		        template class GC_DLL_EXPORT HighestLevel<3,Float32,Float32,true>;
		        template class GC_DLL_EXPORT HighestLevel<3,Float64,Float64,true>;
                /** @endcond */
            }
        }
	}
}
