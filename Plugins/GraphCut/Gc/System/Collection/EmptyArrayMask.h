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
    Array mask class where all elements are unmasked.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_COLLECTION_EMPTYARRAYMASK_H
#define GC_SYSTEM_COLLECTION_EMPTYARRAYMASK_H

#include "IArrayMask.h"

namespace Gc
{
    namespace System
    {
        namespace Collection
        {
            /** Array mask class where all elements are unmasked. */
            template <Size N>
            class EmptyArrayMask
                : public IArrayMask<N>
            {
            protected:
                /** Dimensions of the mask. */
                Math::Algebra::Vector<N,Size> m_dim;
                /** Number of unmasked elements. */
                Size m_num_uel;
                /** Backward indexes. */
                mutable Array<N,Size> m_bi;
                /** Backward indexes have been generated. */
                mutable bool m_has_bi;

            public:
                /** Constructor. 

                    @param[in] dim Dimensions of the mask.
                */
                EmptyArrayMask(const Math::Algebra::Vector<N,Size> &dim)
                    : m_dim(dim), m_has_bi(false)
                {
                    m_num_uel = dim.Product();
                }

                virtual const Math::Algebra::Vector<N,Size>& Dimensions() const
                {
                    return m_dim;
                }

                virtual Size Elements() const
                {
                    return m_num_uel;
                }

                virtual Size UnmaskedElements() const
                {
                    return m_num_uel;
                }

                virtual bool IsMasked(Size nidx) const
                {
                    return false;
                }

                virtual const System::Collection::Array<N,Size>& BackwardIndexes() const
                {
                    if (!m_has_bi)
                    {
                        m_has_bi = true;
                        m_bi.Resize(m_dim);

                        for (Size i = 0; i < m_bi.Elements(); i++)
                        {
                            m_bi[i] = i;
                        }
                    }

                    return m_bi;
                }
            };
        }
    }
}

#endif
