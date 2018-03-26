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
    Dynamic multi-dimensional array of values.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_COLLECTION_ARRAY_H
#define GC_SYSTEM_COLLECTION_ARRAY_H

#include <limits>
#include <string.h>
#include "../../Type.h"
#include "../OutOfMemoryException.h"
#include "../OverflowException.h"
#include "../Algo/Sort/Heap.h"
#include "../../Math/Algebra/Vector.h"
#include "../../System/Algo/Range.h"

namespace Gc
{
    namespace System
    {
        namespace Collection
        {
            /** Dynamic multi-dimensional array of values.

                This class represents a dynamic multi-dimensional array of data. Its size
                can be dynamically changed. However, its intended use is for 
                data whose size is set once and then many operations are performed
                on them, eg. images, etc. It is not optimized for frequent size
                changes. During resize the old content is lost. For applications where
                the final size is unknown and may change frequently use ArrayBuilder instead.
                Advantage of this class is that there is no memory overhead (like when using
                stl::vector) which is crucial for graph cuts that are memory-intensive
                and also it is very fast.

                @warning During copy and resize first default constructor is called and 
                    subsequently the object is copied. This may cause performance problems
                    for some type of objects, but is very fast for classes with empty
                    default constructor (which is the intended use in Gc).

                @tparam N Number of dimensions.
                @tparam T %Data type.

                @see ArrayBuilder.
            */
            template <Size N, class T>
            class Array
            {
            public:
                typedef T * Iterator;
                typedef const T * ConstIterator;

            protected:
                /** Dimensions of the array. */
                Math::Algebra::Vector<N,Size> m_dim;
                /** %Data container. */
                T *m_data;

                /** Cache - number of elements required to move to the next position in given dimension. */
                Math::Algebra::Vector<N,Size> m_stride;
                /** Cache - total number of elements (to speed up calls to Elements()). */
                Size m_elems;

            public:
                /** Default constructor. */
                Array()
                    : m_dim(0), m_data(NULL), m_stride(0), m_elems(0)
                {}

                /** Constructor. 

                    Constructs and resizes the array. Values are initialized with their 
                    default constructor.
                
                    @param[in] dim Requested dimensions of the array.
                */
                explicit Array(const Math::Algebra::Vector<N,Size> &dim)
                    : m_dim(0), m_data(NULL), m_stride(0), m_elems(0)
                {
                    Resize(dim);
                }

                /** Constructor. 

                    Constructs and resizes the array. Element values are set to \c val.
                
                    @param[in] dim Requested dimensions of the array.
                    @param[in] val Value used to initialize the elements.
                */
                Array(const Math::Algebra::Vector<N,Size> &dim, const T& val)
                    : m_dim(0), m_data(NULL), m_stride(0), m_elems(0)
                {
                    Resize(dim, val);
                }

                /** Constructor applicable to one-dimensional arrays. 

                    Constructs and resizes the array. Values are initialized with their 
                    default constructor.
                
                    @param[in] sz Requested size of the array.
                */
                explicit Array(Size sz)
                    : m_dim(0), m_data(NULL), m_stride(0), m_elems(0)
                {
                    Resize(sz);
                }

                /** Constructor applicable to one-dimensional arrays.

                    Constructs and resizes the array. Element values are set to \c val.
                
                    @param[in] sz Requested size of the array.
                    @param[in] val Value used to initialize the elements.
                */
                Array(Size sz, const T& val)
                    : m_dim(0), m_data(NULL), m_stride(0), m_elems(0)
                {
                    Resize(sz, val);
                }

                /** Copy constructor. */
                Array(const Array<N,T> &v)
                    : m_dim(0), m_data(NULL), m_stride(0), m_elems(0)
                {
                    *this = v;
                }

                /** Destructor. */
                ~Array()
                {
                    Dispose();
                }

                /** Dispose memory occupied by the array. */
                Array<N,T>& Dispose()
                {
                    if (m_elems > 0)
                    {
                        delete [] m_data;
                        m_data = NULL;
                        m_dim.SetZero();
                        m_stride.SetZero();
                        m_elems = 0;
                    }
                    
                    return *this;
                }

                /** Assignment operator. 
                
                    @warning See the note in the documentation to this class.
                */
                Array<N,T>& operator=(const Array<N,T>& v)
                {
                    if (this != &v)
                    {                        
                        if (!v.IsEmpty())
                        {
                            // Resize
                            Resize(v.Dimensions());
                            
                            // Copy elements one after another
                            System::Algo::Range::Copy(v.Begin(), v.End(), Begin());
                        }
                        else
                        {
                            Dispose();
                        }
                    }

                    return *this;
                }

                /** Get total number of elements in the array. */
                Size Elements() const
                {
                    return m_elems;
                }

                /** Get array dimensions. */
                const Math::Algebra::Vector<N,Size>& Dimensions() const
                {
                    return m_dim;
                }

                /** Get element stride. */
                const Math::Algebra::Vector<N,Size>& Stride() const
                {
                    return m_stride;
                }

                /** Check if the array is empty. */
                bool IsEmpty() const
                {
                    return m_elems == 0;
                }

                /** Resize the array.

                    If the new number of elements is different then the old content is lost during this operation and 
                    the values are initialized with their default constructor. Otherwise, only info about the dimensions
                    is updated.

                    @param[in] dim New array dimensions.

                    @remarks If you want to be sure, that the values are re-initialized use Resize(dim, T()).
                */
                Array<N,T>& Resize(const Math::Algebra::Vector<N,Size> &dim)
                {
                    if (!System::Algo::Range::AnyEqualTo(dim.Begin(), dim.End(), Size(0)))
                    {
                        CheckOverflow(dim);

                        if (dim.Product() != m_elems)
                        {                            
                            Dispose();

                            try
                            {
                                m_data = new T[dim.Product()];
                            }
                            catch (const std::bad_alloc &)
                            {
                                throw OutOfMemoryException(__FUNCTION__, __LINE__);
                            }
                        }

                        m_dim = dim;
                        UpdateCache();
                    }
                    else
                    {
                        throw ArgumentException(__FUNCTION__, __LINE__,
                            "The size of the array must be non-zero in all dimensions.");
                    }

                    return *this;
                }

                /** Resize the array.

                    The old content of the array is lost and values of new elements is set 
                    to \c val.

                    @param[in] dim New array dimensions.
                    @param[in] val Value of the new elements.
                */
                Array<N,T>& Resize(const Math::Algebra::Vector<N,Size> &dim, const T& val)
                {
                    Resize(dim);
                    System::Algo::Range::Fill(Begin(), End(), val);
                    return *this;
                }

                /** Resize applicable to one-dimensional arrays. */
                Array<N,T>& Resize(Size sz, const T& val)
                {
                    if (N == 1)
                    {
                        Resize(Math::Algebra::Vector<N,Size>(sz), val);
                    }
                    else
                    {
                        throw InvalidOperationException(__FUNCTION__,__LINE__,"Method applicable"
                            " to one dimensional arrays only.");
                    }

                    return *this;
                }

                /** Resize applicable to one-dimensional arrays. */
                Array<N,T>& Resize(Size sz)
                {
                    if (N == 1)
                    {
                        Resize(Math::Algebra::Vector<N,Size>(sz));
                    }
                    else
                    {
                        throw InvalidOperationException(__FUNCTION__,__LINE__,"Method applicable"
                            " to one dimensional arrays only.");
                    }

                    return *this;
                }

                /** Element access operator. */
                T& operator[](Size i)
                {
                    return m_data[i];
                }

                /** Element access operator. */
                const T& operator[](Size i) const
                {
                    return m_data[i];
                }

                /** Comparison operator. */
                bool operator==(const Array<N,T> &a) const
                {
                    if (m_dim != a.m_dim)
                    {
                        return false;
                    }

                    return System::Algo::Range::IsEqual(Begin(), End(), a.Begin());
                }

                /** Comparison operator. */
                bool operator!=(const Array<N,T> &a) const
                {
                    return !(*this == a);
                }

                /** Calculate index into the array corresponding to given vector offset. 
                
                    @see Coord.            
                */
                template <class U>
                U Index(const Math::Algebra::Vector<N,U> &ofs) const
                {
                    Math::Algebra::Vector<N,U> coef = m_stride.template Convert<N,U>(0);
                    return (ofs * coef).Sum();
                }

                /** Calculate index into the array corresponding to given vector offset. 
                
                    Static version of the method.

                    @param[in] dim Dimensions of the array.
                    @param[in] ofs Offset for which the index should be computed.
                */
                template <class U>
                static U Index(const Math::Algebra::Vector<N,Size> &dim, 
                    const Math::Algebra::Vector<N,U> &ofs)
                {
                    Size stride = 1;
                    U idx = 0;

                    for (Size i = 0; i < N; i++)
                    {
                        idx += ofs[i] * U(stride);
                        stride *= dim[i];
                    }
                    
                    return idx;
                }

                /** Calculate indexes into the array corresponding to given vector offsets. 
                
                    @param[in] ofs List of vector offsets.
                    @param[out] idx Element index for each vector in \c ofs.

                    @see Coords.
                */
                template <class U>
                void Indexes(const Collection::Array<1,Math::Algebra::Vector<N,U> > &ofs,
                    Collection::Array<1,U> &idx) const
                {
                    // Calculate offsets
                    idx.Resize(ofs.Elements());
                    Math::Algebra::Vector<N,U> coef = m_stride.template Convert<N,U>(0);
                    for (Size i = 0; i < ofs.Elements(); i++)
                    {
                        idx[i] = (ofs[i] * coef).Sum();
                    }
                }

                /** Calculate indexes into the array corresponding to given vector offsets. 

                    Static version of the method.
                
                    @param[in] dim Dimensions of the array.
                    @param[in] ofs List of vector offsets.
                    @param[out] idx Element index for each vector in \c ofs.

                    @see Coords.
                */
                template <class U>
                static void Indexes(const Math::Algebra::Vector<N,Size> &dim, 
                    const Collection::Array<1,Math::Algebra::Vector<N,U> > &ofs,
                    Collection::Array<1,U> &idx)
                {
                    Math::Algebra::Vector<N,U> stride;
                    stride[0] = 1;
                    for (Size i = 1; i < N; i++)
                    {
                        stride[i] = stride[i-1] * U(dim[i-1]);
                    }

                    // Calculate offsets
                    idx.Resize(ofs.Elements());
                    for (Size i = 0; i < ofs.Elements(); i++)
                    {
                        idx[i] = (ofs[i] * stride).Sum();
                    }
                }

                /** Calculate the coordinates of element with given index.

                    @see Index.
                */
                void Coord(Size idx, Math::Algebra::Vector<N,Size> &coord) const
                {
                    for (Size i = N; i-- > 0; )
                    {
                        coord[i] = idx / m_stride[i];
                        idx = idx % m_stride[i];
                    }
                }

                /** Calculate the coordinates of elements with given indexes.

                    @param[in] iv List of indexes.
                    @param[out] cv Coordinates of elements at indexes in \c nv.

                    @see Indexes.
                */
                void Coords(const Collection::Array<1,Size> &iv,
                    Collection::Array<1,Math::Algebra::Vector<N,Size> > &cv) const
                {
                    cv.Resize(iv.Elements());
                    for (Size i = 0; i < cv.Elements(); i++)
                    {
                        Size idx = iv[i];

                        for (Size j = N; j-- > 0; )
                        {
                            cv[i][j] = idx / m_stride[j];
                            idx -= idx % m_stride[j];
                        }
                    }
                }

                /** Check if neighbour of element at given position is in the array or out of bounds.
                
                    @param[in] pos Element coordinates. This element must be inside the array.
                    @param[in] ofs Offset of the neighbour to be checked.
                */
                template <class U>
                bool IsNeighbourInside (const Math::Algebra::Vector<N,Size> &pos, 
                    const Math::Algebra::Vector<N,U> &ofs) const
                {
                    for (Size i = 0; i < N; i++)
                    {
                        if (ofs[i] < 0)
                        {
                            if (Size(-ofs[i]) > pos[i])
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if (Size(ofs[i]) >= m_dim[i] - pos[i])
                            {
                                return false;
                            }
                        }
                    }

                    return true;
                }

                /** Check if neighbour of element at given position is in the array or out of bounds.
                
                    Static version of the method.

                    @param[in] dim Dimensions of the array.
                    @param[in] pos Element coordinates. This element must be inside the array.
                    @param[in] ofs Offset of the neighbour to be checked.
                */
                template <class U>
                static bool IsNeighbourInside (const Math::Algebra::Vector<N,Size> &dim,
                    const Math::Algebra::Vector<N,Size> &pos, 
                    const Math::Algebra::Vector<N,U> &ofs)
                {
                    for (Size i = 0; i < N; i++)
                    {
                        if (ofs[i] < 0)
                        {
                            if (Size(-ofs[i]) > pos[i])
                            {
                                return false;
                            }
                        }
                        else
                        {
                            if (Size(ofs[i]) >= dim[i] - pos[i])
                            {
                                return false;
                            }
                        }
                    }

                    return true;
                }

                /** Fill memory allocated for the array with zeros.

                    @warning Potentially may cause problems for complex objects (with
                        non-trivial constructors and destructors.). Use with caution.
                */
                void ZeroFillMemory()
                {
                    if (m_elems > 0)
                    {
                        memset(m_data, 0, m_elems * sizeof(T));
                    }
                }

                /** %Iterator to the first element. */
                Iterator Begin()
                {
                    return m_data;
                }

                /** %Iterator to the first element. */
                ConstIterator Begin() const
                {
                    return m_data;
                }

                /** %Iterator one after the last element. */
                Iterator End()
                {
                    return m_data + m_elems;
                }

                /** %Iterator one after the last element. */
                ConstIterator End() const
                {
                    return m_data + m_elems;
                }

                /** %Iterator to the first element. */
                Iterator Front()
                {
                    return m_data;
                }

                /** %Iterator to the first element. */
                ConstIterator Front() const
                {
                    return m_data;
                }

                /** %Iterator to the last element. */
                Iterator Back()
                {
                    return m_data + (m_elems - 1);
                }

                /** %Iterator to the last element. */
                ConstIterator Back() const
                {
                    return m_data + (m_elems - 1);
                }

            protected:
                /** Check if it is possible to store data with given dimensions in the memory. */
                void CheckOverflow(const Math::Algebra::Vector<N,Size> &dim)
                {
                    Size max_elems = std::numeric_limits<Size>::max() / sizeof(T);

                    for (Size i = 0; i < N; i++)
                    {
                        max_elems /= dim[i];
                    }

                    if (!max_elems)
                    {
                        throw OverflowException(__FUNCTION__, __LINE__, "The number of elements is too large "
                            "to be stored in the memory.");
                    }
                }

                /** Recompute cached values. */
                void UpdateCache()
                {
                    // Update number of elements
                    m_elems = m_dim.Product();

                    // Compute stride
                    m_stride[0] = 1;
                    for (Size i = 1; i < N; i++)
                    {
                        m_stride[i] = m_stride[i-1] * m_dim[i-1];
                    }
                }
            };
        }
    }
}

#endif
