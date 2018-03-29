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
    %Mask specification interface for segmentation algorithms.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_ALGO_SEGMENTATION_MASK_H
#define GC_ALGO_SEGMENTATION_MASK_H

#include "../../Type.h"
#include "../../System/Collection/Array.h"
#include "../../System/Collection/IArrayMask.h"

namespace Gc
{
    namespace Algo
    {
        namespace Segmentation
        {
            /** Voxel mask flag. */
            enum MaskFlag
            {
                /** Ignore the voxel (exclude it from the computation). */
                MaskIgnore = 0,
                /** The voxel is known to be background (hard constraint). */
                MaskSource = 1,
                /** The voxel is known to be foreground (hard constraint). */
                MaskSink = 2,
                /** Classify this voxel (include it in the computation). */
                MaskUnknown = 3
            };

            /** %Mask specification interface for segmentation algorithms. 
            
                The mask itself is specified using an array of MaskFlag values
                for each image element.
            */
            template <Size N>
            class Mask
                : public System::Collection::IArrayMask<N>
            {
            protected:
                /** Reference to the mask. */
                const System::Collection::Array<N,Uint8> &m_mask;
                /** Backward indexes. */
                System::Collection::Array<N,Size> m_back_idx;
                /** Number of unmasked elements. */
                Size m_unm_elems;

            public:
                /** Constructor. 

                    Calculates number of unmasked elements and creates backward indexes.
                */
                Mask(const System::Collection::Array<N,Uint8> &mask)
                    : m_mask(mask)
                {
                    m_unm_elems = 0;
                    m_back_idx.Resize(mask.Dimensions());

                    Size *di = m_back_idx.Begin();
                    for (const Uint8 *dm = mask.Begin(); dm != mask.End(); dm++, di++)
                    {
                        if (*dm == MaskUnknown)
                        {
                            *di = m_unm_elems;
                            m_unm_elems++;
                        }
                        else
                        {
                            *di = Size(-1);
                        }
                    }
                }

                /** Dimensions of the mask. */
                virtual const Math::Algebra::Vector<N,Size>& Dimensions() const
                {
                    return m_mask.Dimensions();
                }

                /** Number of elements. */
                virtual Size Elements() const
                {
                    return m_mask.Elements();
                }

                /** Number of unmasked elements. */
                virtual Size UnmaskedElements() const
                {
                    return m_unm_elems;
                }

                /** Returns whether node is masked or not. 

                    @param[in] nidx Node index.
                */
                virtual bool IsMasked(Size nidx) const
                {
                    return m_mask[nidx] != MaskUnknown;
                }

                /** Backward conversion indexes. 

                    Returns a reference to an array that for each element contains its
                    index to an array of unmasked elements. If the given element is
                    masked then the corresponding backward index is unspecified.
                */
                virtual const System::Collection::Array<N,Size>& BackwardIndexes() const
                {
                    return m_back_idx;
                }
            };
        }
    }
}

#endif
