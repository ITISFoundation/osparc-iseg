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
    Array mask interface class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_COLLECTION_IARRAYMASK_H
#define GC_SYSTEM_COLLECTION_IARRAYMASK_H

#include "../../Type.h"
#include "../../Math/Algebra/Vector.h"
#include "Array.h"

namespace Gc
{
    namespace System
    {
        namespace Collection
        {
            /** Array mask interface class. 
            
                Allows to specify which elements of an array are masked.
            */
            template <Size N>
            class IArrayMask
            {
            public:
                /** Virtual destructor. */
                virtual ~IArrayMask() 
                {}

                /** Dimensions of the mask. */
                virtual const Math::Algebra::Vector<N,Size>& Dimensions() const = 0;

                /** Number of elements. */
                virtual Size Elements() const = 0;

                /** Number of unmasked elements. */
                virtual Size UnmaskedElements() const = 0;

                /** Returns whether node is masked or not. 

                    @param[in] nidx Node index.
                */
                virtual bool IsMasked(Size nidx) const = 0;

                /** Backward conversion indexes. 
                
                    Returns a reference to an array that for each element contains its
                    unmasked index (i.e., 0 for first unmasked element, 1 for second
                    unmasked element and so on). If the given element is masked then 
                    the corresponding backward index is unspecified.
                */
                virtual const System::Collection::Array<N,Size>& BackwardIndexes() const = 0;
            };
        }
    }
}

#endif
