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
    Useful mathematical functions and structures.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_MATH_BASIC_H
#define GC_MATH_BASIC_H

#include <math.h>
#include "../System/InvalidOperationException.h"
#include "../System/ArgumentException.h"
#include "../Type.h"

namespace Gc
{
	/** Mathematical functions and structures */
	namespace Math
	{
		/** Calculate \f$ x^2 \f$. */
		template <class T> 
        inline T Sqr(T x)
		{
			return x * x;
		}

		/** Calculate \f$ \sqrt{x} \f$. */
		template <class T> 
        inline T Sqrt(T x)
		{
            throw System::InvalidOperationException(__FUNCTION__, __LINE__, "Sqrt is defined for"
                " floating point numbers only.");
		}

        /** @cond */
        template <>
        inline Float32 Sqrt(Float32 x)
        {
            return sqrt(x);
        }

        template <> 
        inline Float64 Sqrt(Float64 x)
		{
			return sqrt(x);
		}
        /** @endcond */

        /** Calculate \f$ x^y \f$. */
        template <class T>
        inline T Pow(T x, T y)
        {
            if (y < 0)
            {
                throw System::ArgumentException(__FUNCTION__, __LINE__, "Function undefined for"
                    " negative exponents.");
            }

            T v = 1;
            while (y-- > 0)
            {
                v *= x;
            }
            return v;
        }

        /** @cond */
        template <>
        inline Float32 Pow(Float32 x, Float32 y)
        {
            return pow(x, y);
        }

        template <>
        inline Float64 Pow(Float64 x, Float64 y)
        {
            return pow(x, y);
        }
        /** @endcond */

		/** Calculate absolute value of a number. */
		template <class T> 
        inline T Abs(T x)
		{
			return x < 0 ? -x : x;
		}

		/** Get a maximum of two numbers. */
		template <class T> 
        inline T Max(T x, T y)
		{
			return x < y ? y : x;
		}

		/** Get a maximum of three numbers. */
		template <class T> 
        inline T Max(T x, T y, T z)
		{
			if (x < y)
			{
				return Max(y,z);
			}
			return Max(x,z);
		}

		/** Get a minimum of two numbers. */
		template <class T> 
        inline T Min(T x, T y)
		{
			return x < y ? x : y;
		}

		/** Get a minimum of three numbers. */
		template <class T> 
        inline T Min(T x, T y, T z)
		{
			if (x < y)
			{
				return Min(x,z);
			}
			return Min(y,z);
		}

        /** Sign function. 
        
            @return \c 1 for positive numbers, \c -1 for negative and \c 0 for zero.
        */
        template <class T>
        inline Int8 Sgn(T val)
        {
            return (val < 0) ? -1 : ((val > 0) ? 1 : 0);
        }

		/** Restrict the value of a number.
		 *
         * @param[in] x Number.
         * @param[in] low Left interval boundary.
         * @param[in] high Right interval boundary.
		 * @return A number from the interval [\c low, \c high] closest to \c x.
		 */
		template <class T> 
        inline T Restrict(T x, T low, T high) 
		{ 
			return x > high ? high : (x < low ? low : x); 
		}

        /** Round number. 
        
            @remarks This method does nothing for integer numbers.
        */
        template <class T>
        inline T Round(T v)
        {
            return v;            
        }

        /** @cond */
        template <>
        inline Float32 Round(Float32 v)
        {
            return (v >= 0) ? floor(v + 0.5f) : ceil(v - 0.5f);
        }

        template <>
        inline Float64 Round(Float64 v)
        {
            return (v >= 0) ? floor(v + 0.5) : ceil(v - 0.5);
        }    
        /** @endcond */
    }
}

#endif
