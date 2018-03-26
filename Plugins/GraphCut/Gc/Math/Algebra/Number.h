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
    Common functions from number theory.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_MATH_ALGEBRA_NUMBER_H
#define GC_MATH_ALGEBRA_NUMBER_H

#include "../../Type.h"

namespace Gc
{
    namespace Math
    {
        namespace Algebra
        {
            /** Greates common divisor. */
            template <class T>
            T Gcd (T a, T b)
            {
                a = Abs(a);
                b = Abs(b);

                while (b != 0)
                {
                    T t = b;
                    b = a % b;
                    a = t;
                }

                return a;
            }

            /** Greates common divisor. */
            template <class T>
            T Gcd (T a, T b, T c)
            {
                return Gcd(a, Gcd(b,c));
            }

            /** Greates common divisor. */
            template <class T>
            T Gcd (const T *v, Size sz)
            {
                T t = 0;

                for (Size i = 0; i < sz; i++)
                {
                    t = Gcd(t, v[i]);
                }

                return t;
            }

            /** Least common multiple. */
            template <class T>
            T Lcm (T a, T b)
            {
                return (a / Gcd(a,b)) * b;
            }

            /** Least common multiple. */
            template <class T>
            T Lcm (T a, T b, T c)
            {     
                return Lcm(a, Lcm(b,c));
            }

            /** Least common multiple. */
            template <class T>
            T Lcm (const T *v, Size sz)
            {
                T t = 1;

                for (Size i = 0; i < sz; i++)
                {
                    t = Lcm(t, v[i]);
                }

                return t;
            }
        }
    }
}

#endif
