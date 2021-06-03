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
    Tuple data type.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_SYSTEM_COLLECTION_TUPLE_H
#define GC_SYSTEM_COLLECTION_TUPLE_H

#include "../../Type.h"
#include "../InvalidOperationException.h"
#include "../Algo/Range.h"

namespace Gc {
namespace System {
/** Namespace encapsulating data container classes. */
namespace Collection {
/** Class representing n-tuple of elements of the same data type.

                The size of the tuple cannot be changed during its lifetime. 
                However, its possible to freely access the individual elements
                and change their values.

                @tparam N Number of elements (size of the %tuple).
                @tparam T Data type. 
            */
template <Size N, class T>
class Tuple
{
  public:
    using Iterator = T *;
    using ConstIterator = const T *;

  protected:
    /** Array of the tuple elements. */
    T m_data[N];

  public:
    /** Constructor. */
    Tuple() = default;

    /** Constructor. 

                    Initializes all elements with given value.
                */
    explicit Tuple(const T & val)
    {
        // Compiler will optimize this out
        if (N == 1)
        {
            *m_data = val;
        }
        else if (N == 2)
        {
            m_data[0] = val;
            m_data[1] = val;
        }
        else if (N == 3)
        {
            m_data[0] = val;
            m_data[1] = val;
            m_data[2] = val;
        }
        else if (N == 4)
        {
            m_data[0] = val;
            m_data[1] = val;
            m_data[2] = val;
            m_data[3] = val;
        }
        else
        {
            Algo::Range::Fill(Begin(), End(), val);
        }
    }

    /** Constructor applicable to 2-tuples. */
    Tuple(const T & v1, const T & v2)
    {
        Set(v1, v2);
    }

    /** Constructor applicable to 3-tuples. */
    Tuple(const T & v1, const T & v2, const T & v3)
    {
        Set(v1, v2, v3);
    }

    /** Constructor applicable to 4-tuples. */
    Tuple(const T & v1, const T & v2, const T & v3, const T & v4)
    {
        Set(v1, v2, v3, v4);
    }

    /** Copy constructor. */
    Tuple(const Tuple<N, T> & t)
    {
        *this = t;
    }

    /** Assignment operator. */
    Tuple<N, T> & operator=(const Tuple<N, T> & t)
    {
        if (this != &t)
        {
            // Compiler will optimize this out
            if (N == 1)
            {
                *m_data = *t.m_data;
            }
            else if (N == 2)
            {
                m_data[0] = t.m_data[0];
                m_data[1] = t.m_data[1];
            }
            else if (N == 3)
            {
                m_data[0] = t.m_data[0];
                m_data[1] = t.m_data[1];
                m_data[2] = t.m_data[2];
            }
            else if (N == 4)
            {
                m_data[0] = t.m_data[0];
                m_data[1] = t.m_data[1];
                m_data[2] = t.m_data[2];
                m_data[3] = t.m_data[3];
            }
            else
            {
                System::Algo::Range::Copy(t.Begin(), t.End(), Begin());
            }
        }

        return *this;
    }

    /** Get number of elements. */
    Size Elements() const
    {
        return N;
    }

    /** Element access operator. */
    const T & operator[](Size i) const
    {
        return m_data[i];
    }

    /** Element access operator. */
    T & operator[](Size i)
    {
        return m_data[i];
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
        return m_data + N;
    }

    /** %Iterator one after the last element. */
    ConstIterator End() const
    {
        return m_data + N;
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
        return m_data + (N - 1);
    }

    /** %Iterator to the last element. */
    ConstIterator Back() const
    {
        return m_data + (N - 1);
    }

    /** Set elements of a 2-tuple. 

                    @remarks Applicable to 2-tuples only. Otherwise throws an InvalidOperationException.
                */
    Tuple<N, T> & Set(const T & v1, const T & v2)
    {
        if (N != 2)
        {
            throw InvalidOperationException(__FUNCTION__, __LINE__, "Method applicable"
                                                                    " to 2-tuples only.");
        }
        else
        {
            m_data[0] = v1;
            m_data[1] = v2;
        }

        return *this;
    }

    /** Set elements of a 3-tuple. 

                    @remarks Applicable to 3-tuples only. Otherwise throws an InvalidOperationException.               
                */
    Tuple<N, T> & Set(const T & v1, const T & v2, const T & v3)
    {
        if (N != 3)
        {
            throw InvalidOperationException(__FUNCTION__, __LINE__, "Method applicable"
                                                                    " to 3-tuples only.");
        }
        else
        {
            m_data[0] = v1;
            m_data[1] = v2;
            m_data[2] = v3;
        }

        return *this;
    }

    /** Set elements of a 4-tuple. 

                    @remarks Applicable to 4-tuples only. Otherwise throws an InvalidOperationException.
                */
    Tuple<N, T> & Set(const T & v1, const T & v2, const T & v3, const T & v4)
    {
        if (N != 4)
        {
            throw InvalidOperationException(__FUNCTION__, __LINE__, "Method applicable"
                                                                    " to 4-tuples only.");
        }
        else
        {
            m_data[0] = v1;
            m_data[1] = v2;
            m_data[2] = v3;
            m_data[3] = v4;
        }

        return *this;
    }

    /** Equal to operator. */
    bool operator==(const Tuple<N, T> & t) const
    {
        // Compiler will optimize this out
        if (N == 1)
        {
            return (*m_data == *t.m_data);
        }
        else if (N == 2)
        {
            return (m_data[0] == t.m_data[0] && m_data[1] == t.m_data[1]);
        }
        else if (N == 3)
        {
            return (m_data[0] == t.m_data[0] && m_data[1] == t.m_data[1] &&
                    m_data[2] == t.m_data[2]);
        }
        else if (N == 4)
        {
            return (m_data[0] == t.m_data[0] && m_data[1] == t.m_data[1] &&
                    m_data[2] == t.m_data[2] && m_data[3] == t.m_data[3]);
        }

        return System::Algo::Range::IsEqual(Begin(), End(), t.Begin());
    }

    /** Not equal to operator. */
    bool operator!=(const Tuple<N, T> & v) const
    {
        return !(*this == v);
    }
};
} // namespace Collection
}
} // namespace Gc::System

#endif
