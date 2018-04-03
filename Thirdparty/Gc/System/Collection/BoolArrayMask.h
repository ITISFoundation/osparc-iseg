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
    Array mask class implementation using bool array.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_COLLECTION_BOOLARRAYMASK_H
#define GC_SYSTEM_COLLECTION_BOOLARRAYMASK_H

#include "IArrayMask.h"

namespace Gc
{
    namespace System
    {
        namespace Collection
        {
            /** Array mask implementation using bool array. 
            
                Allows to specify which elements of an array are masked. The
                number of unmasked elements and backward indexes are generated
                on-demand.
            */
            template <Size N>
            class BoolArrayMask
                : public IArrayMask<N>
            {
            protected:
                /** Reference to the bool array. */
                const Array<N,bool> &m_data;
                /** Backward indexes. */
                mutable Array<N,Size> m_bi;
                /** Number of unmasked elements. */
                mutable Size m_num_uel;
                /** Masked element flag. */
                bool m_mask_flag;
                /** Number of unmasked elements has been calculated. */
                mutable bool m_has_num_uel;
                /** Backward indexes have been generated. */
                mutable bool m_has_bi;

            public:
                /** Constructor. 
                
                    Takes reference to the \c bool array where masked
                    elements are those with mask_flag value.
                */
                BoolArrayMask(const Array<N,bool> &ba, bool mask_flag)
                    : m_data(ba), m_mask_flag(mask_flag), m_has_num_uel(false), 
                    m_has_bi(false)
                {}

                virtual const Math::Algebra::Vector<N,Size>& Dimensions() const
                {
                    return m_data.Dimensions();
                }

                virtual Size Elements() const
                {
                    return m_data.Elements();
                }

                virtual Size UnmaskedElements() const
                {
                    if (!m_has_num_uel)
                    {
                        m_has_num_uel = true;
                        m_num_uel = 0;

                        for (const bool *it = m_data.Begin(); it != m_data.End(); ++it)
                        {
                            if (*it != m_mask_flag)
                            {
                                m_num_uel++;
                            }
                        }
                    }

                    return m_num_uel;
                }

                virtual bool IsMasked(Size nidx) const
                {
                    return m_data[nidx] == m_mask_flag;
                }

                virtual const System::Collection::Array<N,Size>& BackwardIndexes() const
                {
                    if (!m_has_bi)
                    {
                        m_has_bi = true;
                        m_bi.Resize(m_data.Dimensions());

                        Size idx = 0;
                        for (const bool *it = m_data.Begin(); it != m_data.End(); ++it)
                        {
                            if (*it != m_mask_flag)
                            {
                                m_bi[it - m_data.Begin()] = idx;
                                idx++;
                            }
                        }
                    }

                    return m_bi;
                }
            };
        }
    }
}

#endif
