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
    Mathematical constants.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_MATH_CONSTANT_H
#define GC_MATH_CONSTANT_H

#include "../Type.h"

namespace Gc {
namespace Math {
/** Mathematical constants. */
namespace Constant {
/** \f$ \pi \f$ number. */
const Float64 Pi = 3.14159265358979323846;
/** Euler number. */
const Float64 E = 2.7182818284590452354;
} // namespace Constant
}
} // namespace Gc::Math

#endif
