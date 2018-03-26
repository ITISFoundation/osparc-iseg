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
    Basic type definitions.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_TYPE_H
#define GC_TYPE_H

#include <stddef.h>
#include <stdint.h>

// 64bit architecture detection
#if (defined(_MSC_VER) && defined(_M_X64)) || (defined(__GNUC__) && defined(__x86_64__))
    #define GC_PLATFORM_X64
#endif

/** Global library namespace. */
namespace Gc
{
    /** Signed 8-bit decimal number. */
	typedef int8_t Int8;
    /** Unsigned 8-bit decimal number. */
	typedef uint8_t Uint8;

    /** Signed 16-bit decimal number. */
	typedef int16_t Int16;
    /** Unsigned 16-bit decimal number. */
	typedef uint16_t Uint16;

    /** Signed 32-bit decimal number. */
	typedef int32_t Int32;
    /** Unsigned 32-bit decimal number. */
	typedef uint32_t Uint32;

    /** Signed 64-bit decimal number. */
	typedef int64_t Int64;
    /** Unsigned 64-bit decimal number. */
	typedef uint64_t Uint64;

    /** Single precision floating point number. */
    typedef float Float32;
    /** Double precision floating point number. */
    typedef double Float64;

#ifdef GC_PLATFORM_X64
    /** Platform dependent integer. 
    
        32-bit on 32-bit platforms, 64-bit on 64-bit platforms.
    */
    typedef Int64 Pint;

    /** Platform dependent size type. 
    
        32-bit on 32-bit platforms, 64-bit on 64-bit platforms.
    */
	typedef Uint64 Size;
#else
    /** Platform dependent integer. 
    
        32-bit on 32-bit platforms, 64-bit on 64-bit platforms.
    */
    typedef Int32 Pint;

    /** Platform dependent size type. 
    
        32-bit on 32-bit platforms, 64-bit on 64-bit platforms.
    */
	typedef Uint32 Size;
#endif
}

#endif
