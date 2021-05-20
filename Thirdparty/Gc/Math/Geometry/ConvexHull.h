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
    Convex hull computation algorithms.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_MATH_GEOMETRY_CONVEXHULL_H
#define GC_MATH_GEOMETRY_CONVEXHULL_H

#include "../../Core.h"
#include "../../Type.h"
#include "../Algebra/Vector.h"
#include "../../System/Collection/Array.h"

namespace Gc {
namespace Math {
/** Geometrical algorithms. */
namespace Geometry {
/** Calculate convex hull of given set of points in 3D space. 
            
                @param[in] points Set of points in 3D space.
                @param[out] hull Array of triangles constituting the convex hull. Each triangle
                    consists of three indexes to \c points array.
            */
template <class T>
void GC_DLL_EXPORT ConvexHull(const System::Collection::Array<1, Algebra::Vector<3, T>> & points,
                              System::Collection::Array<1, System::Collection::Tuple<3, Size>> & hull);

/** Calculate convex hull of given set of points in 2D space. 
                        
                @param[in] points Set of points in 2D space.
                @param[out] hull Array of lines constituting the convex hull. Each line
                    consists of two indexes to \c points array.            
            */
template <class T>
void GC_DLL_EXPORT ConvexHull(const System::Collection::Array<1, Algebra::Vector<2, T>> & points,
                              System::Collection::Array<1, System::Collection::Tuple<2, Size>> & hull);
} // namespace Geometry
}
} // namespace Gc::Math

#endif
