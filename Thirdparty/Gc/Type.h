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

#include <cstddef>
#include <cstdint>

// 64bit architecture detection
#if (defined(_MSC_VER) && defined(_M_X64)) || (defined(__GNUC__) && defined(__x86_64__))
    #define GC_PLATFORM_X64
#endif

/** Global library namespace. */
namespace Gc {
/** Signed 8-bit decimal number. */
using Int8 = int8_t;
/** Unsigned 8-bit decimal number. */
using Uint8 = uint8_t;

/** Signed 16-bit decimal number. */
using Int16 = int16_t;
/** Unsigned 16-bit decimal number. */
using Uint16 = uint16_t;

/** Signed 32-bit decimal number. */
using Int32 = int32_t;
/** Unsigned 32-bit decimal number. */
using Uint32 = uint32_t;

/** Signed 64-bit decimal number. */
using Int64 = int64_t;
/** Unsigned 64-bit decimal number. */
using Uint64 = uint64_t;

/** Single precision floating point number. */
using Float32 = float;
/** Double precision floating point number. */
using Float64 = double;

#ifdef GC_PLATFORM_X64
/** Platform dependent integer. 
    
        32-bit on 32-bit platforms, 64-bit on 64-bit platforms.
    */
using Pint = Int64;

/** Platform dependent size type. 
    
        32-bit on 32-bit platforms, 64-bit on 64-bit platforms.
    */
using Size = Uint64;
#else
/** Platform dependent integer. 
    
        32-bit on 32-bit platforms, 64-bit on 64-bit platforms.
    */
using Pint = Int32;

/** Platform dependent size type. 
    
        32-bit on 32-bit platforms, 64-bit on 64-bit platforms.
    */
using Size = Uint32;
#endif
} // namespace Gc

#endif
