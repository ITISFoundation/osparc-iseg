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
    Dynamic one dimensional array of values.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_COLLECTION_ARRAYBUILDER_H
#define GC_SYSTEM_COLLECTION_ARRAYBUILDER_H

#include <stdlib.h>
#include "../../Type.h"
#include "../OutOfMemoryException.h"
#include "../OverflowException.h"
#include "../../Math/Basic.h"
#include "Array.h"

namespace Gc
{
    namespace System
    {
        namespace Collection
        {
            /** Array building class.

                This class represents a one-dimensional array of data which size can be
                dynamically changed. The class is optimized for frequent changes and the
                number of reallocations should be minimized. It is recommended to use
                this class only in cases in which the final size of the array is unknown
                and then to copy the result to a standard Collection::Array class.

                @warning Current implementation supports only generic data types,
                    i.e., types compatible with \c malloc and \c memcpy. Particularly,
                    objects that require deep copy will cause a crash. This is for
                    speed purposes.
            */
            template <class T>
            class ArrayBuilder
            {
            private:
                /** Minimal capacity of the array. Setting this higher reduces memory reallocations but may
                    waste some memory.
                */
                static const Size MinArraySize = 10;

            protected:
                /** Current size. */
                Size m_elems;
                /** Allocated capacity. */
                Size m_cap;
                /** %Data container.*/
                T *m_data;

            public:
                /** Constructor. */
                ArrayBuilder()
                    : m_elems(0), m_cap(0), m_data(NULL)
                {}

                /** Constructor. 
                
                    @param[in] sz Initial capacity of the array.
                */
                explicit ArrayBuilder(Size sz)
                    : m_elems(0), m_cap(0), m_data(NULL)
                {
                    Enlarge(sz);
                }

                /** Copy constructor. */
                ArrayBuilder(const ArrayBuilder<T> &a)
                    : m_elems(0), m_cap(0), m_data(NULL)
                {
                    *this = a;
                }

                /** Assignment operator. */
                ArrayBuilder<T>& operator= (const ArrayBuilder<T> &a)
                {
                    if (&a != this)
                    {
                        Dispose();

                        if (!a.IsEmpty())
                        {
                            m_data = (T *)malloc(sizeof(T) * a.m_elems);
                            if (m_data == NULL)
                            {
                                throw OutOfMemoryException(__FUNCTION__, __LINE__);
                            }

                            m_elems = m_cap = a.m_elems;
                            memcpy(m_data, a.m_data, sizeof(T) * a.m_elems);
                        }
                    }

                    return *this;
                }

                /** Destructor. */
                ~ArrayBuilder()
                {
                    Dispose();
                }

                /** Dispose memory. */
                ArrayBuilder<T>& Dispose()
                {
                    if (m_cap > 0)
                    {
                        free(m_data);
                        m_data = NULL;
                        m_cap = 0;
                        m_elems = 0;
                    }

                    return *this;
                }

                /** Resize the array. 
                
                    @param[in] new_sz New size of the array.
                    
                    @remarks The memory usage is optimized, i.e., more memory than requested
                        may be allocated and also no memory is freed when the new size is
                        smaller than the old one.
                */
                ArrayBuilder<T>& Resize(Size new_sz)
                {
                    if (new_sz > m_cap)
                    {
                        Enlarge(new_sz);
                    }

                    m_elems = new_sz;

                    return *this;
                }

                /** Number of elements. */
                Size Elements() const
                {
                    return m_elems;
                }

                /** Is array empty. */
                bool IsEmpty() const
                {
                    return m_elems == 0;
                }

                /** Element access operator. */
                T& operator[] (Size i)
                {
                    return m_data[i];
                }

                /** Element access operator. */
                const T& operator[] (Size i) const
                {
                    return m_data[i];
                }

                /** Add new element to the end of the array. */
                ArrayBuilder<T>& Push(const T& val)
                {
                    Resize(m_elems + 1);
                    m_data[m_elems - 1] = val;
                    return *this;
                }

                /** Remove the last item in the array. */
                ArrayBuilder<T>& Pop()
                {
                    if (m_elems > 0)
                    {
                        m_elems--;
                    }
                    return *this;
                }

                /** Get the very first element in the array. */
                T& First()
                {
                    return m_data[0];
                }

                /** Get the very first element in the array. */
                const T& First() const
                {
                    return m_data[0];
                }

                /** Get the very first element in the array. */
                T& Last()
                {
                    return m_data[m_elems - 1];
                }

                /** Get the very first element in the array. */
                const T& Last() const
                {
                    return m_data[m_elems - 1];
                }

                /** Remove element at given position. */
                ArrayBuilder<T>& Remove(Size i)
                {
                    if (m_elems > 0)
                    {
                        if (i < m_elems - 1)
                        {
                            memmove(m_data + i, m_data + i + 1, sizeof(T) * ((m_elems - 1) - i));
                        }
                        m_elems--;
                    }

                    return *this;
                }

                /** Copy result to a standard array. */
                void CopyToArray(Array<1,T> &a) const
                {
                    if (m_elems)
                    {
                        a.Resize(m_elems);
                        for (Size i = 0; i < m_elems; i++)
                        {
                            a[i] = m_data[i];
                        }
                    }
                    else
                    {
                        a.Dispose();
                    }
                }

            protected:
                /** Enlarge the allocated memory so it can fit the requested number of elements.

                    @param[in] req_sz The requested new size.

                    @remarks More memory than requested may be allocated to optimize for later operations.
                    @see OptimalCapacityIncrease
                */
                void Enlarge(Size req_sz)
                {
                    Size new_cap = OptimalCapacityIncrease(req_sz);

                    T *new_data = (T *) realloc(m_data, sizeof(T) * new_cap);

                    if (new_data == NULL)
                    {
                        throw OutOfMemoryException(__FUNCTION__, __LINE__);                            
                    }

                    m_data = new_data;
                    m_cap = new_cap;
                }

                /** Returns how much the capacity should be increased in order to be able to fit the
                    requested number of elements and to reduce later memory reallocations.

                    @param[in] req_sz The requested new size.
                */
                Size OptimalCapacityIncrease(Size req_sz)
                {
                    Size max_elems = std::numeric_limits<Size>::max() / sizeof(T);

                    if (req_sz > max_elems)
                    {
                        throw OverflowException(__FUNCTION__, __LINE__, "Requested number of elements "
                            "exceeds the maximum possible.");
                    }

                    // Ideally grow by half of the old capacity
                    Size new_cap = (max_elems - m_cap < m_cap / 2) ? max_elems : m_cap + m_cap / 2;
                    
                    return Math::Max(MinArraySize, req_sz, new_cap);
                }
            };
        }
    }
}

#endif
