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
    Simple image representation class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#include "../System/InvalidOperationException.h"
#include "BorderIterator.h"
#include "Image.h"

namespace Gc
{
    namespace Data
    {
        /************************************************************************/

        template <Size N, class PREC, class DATA>
        void Image<N,PREC,DATA>::Pad(const Math::Algebra::Vector<N,Size> &left,
            const Math::Algebra::Vector<N,Size> &right, const DATA &val,
            Image<N,PREC,DATA> &iout) const
        {
            // Resize the output image
            Math::Algebra::Vector<N,Size> new_dim = this->Dimensions() + left + right;
            iout.Resize(new_dim);
            iout.SetSpacing(Spacing());

            // Create iterator
            BorderIterator<N> iter(new_dim, left, right);
            iter.Start(false);

            // Start padding
            const DATA *data_in = this->Begin();
            DATA *data_out = iout.Begin();

            while (!iter.IsFinished())
            {
                Size num;
                
                if (iter.NextBlock(num))
                {
                    while (num-- > 0)
                    {
                        *data_out = val;
                        data_out++;
                    }
                }
                else
                {
                    while (num-- > 0)
                    {
                        *data_out = *data_in;
                        data_out++;
                        data_in++;
                    }
                }
            }
        }

        /************************************************************************/

        template <Size N, class PREC, class DATA>
        void Image<N,PREC,DATA>::Range(DATA &imin, DATA &imax) const
        {
            if (this->IsEmpty())
            {
                throw System::InvalidOperationException(__FUNCTION__, __LINE__,
                    "Intensity range of an empty image is undefined.");
            }

            imin = imax = m_data[0];
            const DATA *p = &m_data[1];
            for (Size i = 1; i < m_elems; i++, p++)
            {
                if (*p < imin)
                {
                    imin = *p;
                }
                else if (*p > imax)
                {
                    imax = *p;
                }
            }
        }

        /************************************************************************/

        // Explicit instantiations

        // 2D
        /** @cond */
        template GC_DLL_EXPORT class Image<2,Float32,bool>;
        template GC_DLL_EXPORT class Image<2,Float32,Uint8>;
        template GC_DLL_EXPORT class Image<2,Float32,Uint16>;
        template GC_DLL_EXPORT class Image<2,Float32,Uint32>;
        template GC_DLL_EXPORT class Image<2,Float32,Uint64>;
        template GC_DLL_EXPORT class Image<2,Float32,Float32>;
        template GC_DLL_EXPORT class Image<2,Float32,Float64>;

        template GC_DLL_EXPORT class Image<2,Float64,bool>;
        template GC_DLL_EXPORT class Image<2,Float64,Uint8>;
        template GC_DLL_EXPORT class Image<2,Float64,Uint16>;
        template GC_DLL_EXPORT class Image<2,Float64,Uint32>;
        template GC_DLL_EXPORT class Image<2,Float64,Uint64>;
        template GC_DLL_EXPORT class Image<2,Float64,Float32>;
        template GC_DLL_EXPORT class Image<2,Float64,Float64>;

        // 3D
        template GC_DLL_EXPORT class Image<3,Float32,bool>;
        template GC_DLL_EXPORT class Image<3,Float32,Uint8>;
        template GC_DLL_EXPORT class Image<3,Float32,Uint16>;
        template GC_DLL_EXPORT class Image<3,Float32,Uint32>;
        template GC_DLL_EXPORT class Image<3,Float32,Uint64>;
        template GC_DLL_EXPORT class Image<3,Float32,Float32>;
        template GC_DLL_EXPORT class Image<3,Float32,Float64>;

        template GC_DLL_EXPORT class Image<3,Float64,bool>;
        template GC_DLL_EXPORT class Image<3,Float64,Uint8>;
        template GC_DLL_EXPORT class Image<3,Float64,Uint16>;
        template GC_DLL_EXPORT class Image<3,Float64,Uint32>;
        template GC_DLL_EXPORT class Image<3,Float64,Uint64>;
        template GC_DLL_EXPORT class Image<3,Float64,Float32>;
        template GC_DLL_EXPORT class Image<3,Float64,Float64>;
        /** @endcond */

        /************************************************************************/
    }
}
