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
    Basic function types.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_SYSTEM_ALGO_FUNCTION_H
#define GC_SYSTEM_ALGO_FUNCTION_H

namespace Gc {
namespace System {
namespace Algo {
/** Generic unary function. */
template <class Arg, class Res>
struct UnaryFunction
{
    using ArgType = Arg;
    using ResType = Res;
};

/** Generic binary function. */
template <class Arg1, class Arg2, class Res>
struct BinaryFunction
{
    using Arg1Type = Arg1;
    using Arg2Type = Arg2;
    using ResType = Res;
};
}
}
} // namespace Gc::System::Algo

#endif
