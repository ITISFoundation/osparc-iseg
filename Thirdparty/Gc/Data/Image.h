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

#ifndef GC_DATA_IMAGE_H
#define GC_DATA_IMAGE_H

#include "../Core.h"
#include "../System/Collection/Array.h"

namespace Gc {
namespace Data {
/** N-dimensional image representation class.
        
            @tparam N Dimensionality of the data.
            @tparam PREC %Data type used to store the image resolution.
            @tparam DATA %Data type of the voxels.
        */
template <Size N, class PREC, class DATA>
class GC_DLL_EXPORT Image
    : public System::Collection::Array<N, DATA>
{
  protected:
    using System::Collection::Array<N, DATA>::m_data;
    using System::Collection::Array<N, DATA>::m_dim;
    using System::Collection::Array<N, DATA>::m_elems;

    /** %Image voxel spacing/resolution. */
    Math::Algebra::Vector<N, PREC> m_spacing;

  public:
    /** Constructor. */
    Image()
        : System::Collection::Array<N, DATA>()
        , m_spacing(0)
    {}

    /** Copy constructor. */
    Image(const Image<N, PREC, DATA> & img)
        : System::Collection::Array<N, DATA>()
        , m_spacing(0)
    {
        *this = img;
    }

    /** Destructor. */
    ~Image()
    {
        this->Dispose();
    }

    /** Assignment operator. */
    Image<N, PREC, DATA> & operator=(const Image<N, PREC, DATA> & img)
    {
        if (this != &img)
        {
            System::Collection::Array<N, DATA>::operator=(img);
            m_spacing = img.m_spacing;
        }

        return *this;
    }

    /** Get voxel spacing of the image. */
    const Math::Algebra::Vector<N, PREC> & Spacing() const
    {
        return m_spacing;
    }

    /** Set image voxel spacing. */
    void SetSpacing(const Math::Algebra::Vector<N, PREC> & spc)
    {
        m_spacing = spc;
    }

    /** Pad image with given value.

                @param[in] left Size of the border to be added on the left 
                    side in each dimension.
                @param[in] right Size of the border to be added on the right
                    side in each dimension.
                @param[in] val The padded value.
                @param[in] iout Output image.

                @see Gc::Energy::Neighbourhood::Extent().
            */
    void Pad(const Math::Algebra::Vector<N, Size> & left,
             const Math::Algebra::Vector<N, Size> & right, const DATA & val,
             Image<N, PREC, DATA> & iout) const;

    /** Copy image size and resolution. */
    template <class DATA2>
    void CopyMetaData(const Image<N, PREC, DATA2> & img)
    {
        Resize(img.Dimensions());
        SetSpacing(img.Spacing());
    }

    /** Get voxel size. 
            
                @return Product of the resolution vector.
            */
    PREC VoxelSize() const
    {
        return m_spacing.Product();
    }

    /** Calculate the range of element values in the image. 
            
                @param[out] imin Lowest element value.
                @param[out] imax Highest element value.
            */
    void Range(DATA & imin, DATA & imax) const;
};
}
} // namespace Gc::Data

#endif
