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
    A pair of values which may be of different data type.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2012
*/

#ifndef GC_SYSTEM_COLLECTION_PAIR_H
#define GC_SYSTEM_COLLECTION_PAIR_H

namespace Gc {
namespace System {
namespace Collection {
/** A pair of values which may be of a different data type. */
template <class T1, class T2>
class Pair
{
  public:
    /** First element. */
    T1 m_first;
    /** Second element. */
    T2 m_second;

  public:
    /** Default constructor. */
    Pair() = default;

    /** Constructor. */
    Pair(const T1 & v1, const T2 & v2)
        : m_first(v1)
        , m_second(v2)
    {}

    /** Copy constructor. */
    Pair(const Pair<T1, T2> & p)
        : m_first(p.m_first)
        , m_second(p.m_second)
    {}

    /** Assignment operator. */
    Pair<T1, T2> & operator=(const Pair<T1, T2> & p)
    {
        if (this != &p)
        {
            m_first = p.m_first;
            m_second = p.m_second;
        }
        return *this;
    }
};
}
}
} // namespace Gc::System::Collection

#endif
