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
    Simple point inspection routines.

    @author Martin Maska <xmaska@fi.muni.cz>
    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_ALGO_GEOMETRY_SIMPLEPOINT_H
#define GC_ALGO_GEOMETRY_SIMPLEPOINT_H

#include "../../Core.h"
#include "../../Type.h"

namespace Gc {
namespace Algo {
namespace Geometry {
/** Simple point inspection in 2D.

                4-adjacency is considered for the background and 8-adjacency 
                for the foreground.

                @param[in] nb Number coding the 8-neighbourhood of the inspected grid
                    point. Starting with the top left corner and going to right and
                    down the first 8 bits code whether the given neighbour is labeled
                    as background = 0 or foreground = 1.
            */
bool GC_DLL_EXPORT IsSimplePoint2D(Uint32 nb);

/** Simple point inspection in 3D.

                6-adjacency is considered for the background and 26-adjacency 
                for the foreground.

                @param[in] nb Number coding the 26-neighbourhood of the inspected grid
                    point. Starting with the top left corner and going to right and
                    down the first 26 bits code whether the given neighbour is labeled
                    as background = 0 or foreground = 1.
            */
bool GC_DLL_EXPORT IsSimplePoint3D(Uint32 nb);
}
}
} // namespace Gc::Algo::Geometry

#endif
