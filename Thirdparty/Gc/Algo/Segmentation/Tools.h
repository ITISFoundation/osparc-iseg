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
    %Tools related to segmentation algorithms.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_ALGO_SEGMENTATION_TOOLS_H
#define GC_ALGO_SEGMENTATION_TOOLS_H

#include "../../Core.h"
#include "../../Type.h"
#include "../../Data/Image.h"

namespace Gc
{
    namespace Algo
    {
        namespace Segmentation
        {
            /** %Tools related to segmentation algorithms. */
            namespace Tools
            {
                /** Normalize image and its resolution. 

                    This function scales the image intensities to [0,1] interval and divides
                    the resolution/spacing by its smallest element (e.g (2,2,4) will
                    be normalized to (1,1,2)).

                    @param[in] im_in Input image.
                    @param[out] im_out Normalized input image (can be the same reference as im_in)
                */
                template <Size N, class T, class DATA>
                void GC_DLL_EXPORT NormalizeImage(const Data::Image<N,T,DATA> &im_in, 
                    Data::Image<N,T,T> &im_out);

                /** Create mask for the two-stage algorithm.

                    Using the coarse segmentation from the first stage this method can be used
                    to create a mask of a band of given size along the segmentation boundary. 
                    This mask is then used in the second stage to reduce the computational domain.

                    @param[in] seg The initial segmentation.
                    @param[in] band_size Size of the band in voxels.
                    @param[out] mask The output mask.
                */
                template <Size N>
                void GC_DLL_EXPORT CreateBandMask(const System::Collection::Array<N,bool> &seg, 
                    Size band_size, System::Collection::Array<N,Uint8> &mask);

                /** Create mask for the two-stage algorithm.

                    Using the coarse segmentation from the first stage this method can be used
                    to create a mask of a band of given size along the segmentation boundary. 
                    This mask is then used in the second stage to reduce the computational domain.

                    @param[in] seg The initial segmentation.
                    @param[in] init_mask Initial mask to be taken into account. This is useful when
                        the first stage was computed in a mask as well. Hard constraints are copied
                        from this mask.
                    @param[in] band_size Size of the band in voxels.
                    @param[out] mask The output mask.
                */
                template <Size N>
                void GC_DLL_EXPORT CreateBandMask(const System::Collection::Array<N,bool> &seg, 
                    const System::Collection::Array<N,Uint8> &init_mask, Size band_size,
                    System::Collection::Array<N,Uint8> &mask);
            }
        }
    }
}

#endif
