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
    Basic predicate functors.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_SYSTEM_ALGO_PREDICATE_H
#define GC_SYSTEM_ALGO_PREDICATE_H

#include "Function.h"

namespace Gc
{
    namespace System
    {
        namespace Algo
        {        
            /** Basic predicate functors. */
            namespace Predicate
            {
                /////////////////////////////////////////////////////////////
                // Unary predicates
                /////////////////////////////////////////////////////////////

                /** Equal to a fixed value predicate. 
            
                    @remarks The functor holds a copy of the value so it can be
                        safely deleted.
                */
                template <class T>
                struct EqualTo
                    : public UnaryFunction<T,bool>
                {
                    T m_val;

                    bool operator()(const T& val) const
                    {
                        return (val == m_val);
                    }

                    EqualTo(const T& val)
                        : m_val(val)
                    {}
                };

                /** Equal to a fixed value predicate. 
            
                    @warning Only reference to the value is taken. Don't use
                        with temporary values.
                */
                template <class T>
                struct EqualToRef
                    : public UnaryFunction<T,bool>
                {
                    const T& m_val;

                    bool operator()(const T& val) const
                    {
                        return (val == m_val);
                    }

                    EqualToRef(const T& val)
                        : m_val(val)
                    {}
                };

                /////////////////////////////////////////////////////////////
                // Binary predicates
                /////////////////////////////////////////////////////////////

                /** Equal to predicate. */
                template <class T>
                struct Equal
                    : public BinaryFunction<T,T,bool>
                {                
                    bool operator()(const T& v1, const T& v2) const
                    {
                        return (v1 == v2);
                    }
                };

                /** Not equal to predicate. */
                template <class T>
                struct NotEqual
                    : public BinaryFunction<T,T,bool>
                {                
                    bool operator()(const T& v1, const T& v2) const
                    {
                        return (v1 != v2);
                    }
                };

                /** Less than predicate. */
                template <class T>
                struct Less
                    : public BinaryFunction<T,T,bool>
                {                
                    bool operator()(const T& v1, const T& v2) const
                    {
                        return (v1 < v2);
                    }
                };

                /** Less than or equal to predicate. */
                template <class T>
                struct LessOrEqual
                    : public BinaryFunction<T,T,bool>
                {                
                    bool operator()(const T& v1, const T& v2) const
                    {
                        return (v1 <= v2);
                    }
                };

                /** Greater than predicate. */
                template <class T>
                struct Greater
                    : public BinaryFunction<T,T,bool>
                {                
                    bool operator()(const T& v1, const T& v2) const
                    {
                        return (v1 > v2);
                    }
                };

                /** Greater than or equal to predicate. */
                template <class T>
                struct GreaterOrEqual
                    : public BinaryFunction<T,T,bool>
                {                
                    bool operator()(const T& v1, const T& v2) const
                    {
                        return (v1 >= v2);
                    }
                };
            }
        }
    }
}

#endif
