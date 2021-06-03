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
    Tools related to segmentation algorithms.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#include "../Geometry/DistanceTransform.h"
#include "Mask.h"
#include "Tools.h"

namespace Gc {
namespace Algo {
namespace Segmentation {
namespace Tools {
/******************************************************************************/

template <Size N, class T, class DATA>
void NormalizeImage(const Data::Image<N, T, DATA> & im_in,
                    Data::Image<N, T, T> & im_out)
{
    if (im_in.IsEmpty())
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__,
                                        "Input image is empty.");
    }

    // Re-quantize to <0,1> interval
    DATA dmin, dmax;
    im_in.Range(dmin, dmax);

    if (dmin >= dmax)
    {
        throw System::ArgumentException(__FUNCTION__, __LINE__,
                                        "Input image has constant intensity.");
    }

    T * dout = im_out.Resize(im_in.Dimensions()).Begin();
    const DATA * din = im_in.Begin();
    dmax -= dmin;

    for (Size i = 0; i < im_in.Elements(); i++, din++, dout++)
    {
        *dout = T(*din - dmin) / T(dmax);
    }

    // Normalize the resolution of the image
    im_out.SetSpacing(im_in.Spacing() / im_in.Spacing().Min());
}

/******************************************************************************/

// Explicit instantiations
/** @cond */
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<2, Float32, Uint8> & im_in,
                                           Data::Image<2, Float32, Float32> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<2, Float64, Uint8> & im_in,
                                           Data::Image<2, Float64, Float64> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<2, Float32, Uint16> & im_in,
                                           Data::Image<2, Float32, Float32> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<2, Float64, Uint16> & im_in,
                                           Data::Image<2, Float64, Float64> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<2, Float32, Float32> & im_in,
                                           Data::Image<2, Float32, Float32> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<2, Float64, Float64> & im_in,
                                           Data::Image<2, Float64, Float64> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<3, Float32, Uint8> & im_in,
                                           Data::Image<3, Float32, Float32> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<3, Float64, Uint8> & im_in,
                                           Data::Image<3, Float64, Float64> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<3, Float32, Uint16> & im_in,
                                           Data::Image<3, Float32, Float32> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<3, Float64, Uint16> & im_in,
                                           Data::Image<3, Float64, Float64> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<3, Float32, Float32> & im_in,
                                           Data::Image<3, Float32, Float32> & im_out);
template void GC_DLL_EXPORT NormalizeImage(const Data::Image<3, Float64, Float64> & im_in,
                                           Data::Image<3, Float64, Float64> & im_out);
/** @endcond */

/******************************************************************************/

template <Size N>
void CreateBandMask(const System::Collection::Array<N, bool> & seg, Size band_size,
                    System::Collection::Array<N, Uint8> & mask)
{
    mask.Resize(seg.Dimensions());

    System::Collection::Array<N, Uint32> dt;
    Geometry::DistanceTransform::CityBlock(seg, true, dt);
    for (Size i = 0; i < dt.Elements(); i++)
    {
        if (dt[i] > band_size)
        {
            mask[i] = MaskSource;
        }
        else if (dt[i] > 0)
        {
            mask[i] = MaskUnknown;
        }
    }

    Geometry::DistanceTransform::CityBlock(seg, false, dt);
    for (Size i = 0; i < dt.Elements(); i++)
    {
        if (dt[i] > band_size)
        {
            mask[i] = MaskSink;
        }
        else if (dt[i] > 0)
        {
            mask[i] = MaskUnknown;
        }
    }
}

/******************************************************************************/

// Explicit instantiations
/** @cond */
template void GC_DLL_EXPORT CreateBandMask(const System::Collection::Array<2, bool> & seg,
                                           Size band_size, System::Collection::Array<2, Uint8> & mask);
template void GC_DLL_EXPORT CreateBandMask(const System::Collection::Array<3, bool> & seg,
                                           Size band_size, System::Collection::Array<3, Uint8> & mask);
/** @endcond */

/******************************************************************************/

template <Size N>
void CreateBandMask(const System::Collection::Array<N, bool> & seg,
                    const System::Collection::Array<N, Uint8> & init_mask, Size band_size,
                    System::Collection::Array<N, Uint8> & mask)
{
    mask = init_mask;

    System::Collection::Array<N, Uint32> dt;
    Geometry::DistanceTransform::CityBlock(seg, true, dt);
    for (Size i = 0; i < dt.Elements(); i++)
    {
        if (mask[i] == MaskUnknown && dt[i] > band_size)
        {
            mask[i] = MaskSource;
        }
    }

    Geometry::DistanceTransform::CityBlock(seg, false, dt);
    for (Size i = 0; i < dt.Elements(); i++)
    {
        if (mask[i] == MaskUnknown && dt[i] > band_size)
        {
            mask[i] = MaskSink;
        }
    }
}

/******************************************************************************/

// Explicit instantiations
/** @cond */
template void GC_DLL_EXPORT CreateBandMask(const System::Collection::Array<2, bool> & seg,
                                           const System::Collection::Array<2, Uint8> & init_mask, Size band_size,
                                           System::Collection::Array<2, Uint8> & mask);
template void GC_DLL_EXPORT CreateBandMask(const System::Collection::Array<3, bool> & seg,
                                           const System::Collection::Array<3, Uint8> & init_mask, Size band_size,
                                           System::Collection::Array<3, Uint8> & mask);
/** @endcond */

/******************************************************************************/
}
}
}
} // namespace Gc::Algo::Segmentation::Tools
