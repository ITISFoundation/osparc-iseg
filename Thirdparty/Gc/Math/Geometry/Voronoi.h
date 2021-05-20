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
    Voronoi diagram computation algorithms.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_MATH_GEOMETRY_VORONOI_H
#define GC_MATH_GEOMETRY_VORONOI_H

#include "../../Core.h"
#include "../../Type.h"
#include "../Algebra/Vector.h"
#include "../../System/Collection/Array.h"

namespace Gc {
namespace Math {
namespace Geometry {
/** Calculate spherical Voronoi diagram. 

                Calculates the Voronoi diagram on 2-D unit sphere surface manifold.
            
                @param[in] points List of points lying on a unit sphere.
                @param[out] vd Area sizes of the spherical voronoi cell corresponding to given points.
            */
template <class T>
void GC_DLL_EXPORT HypersphereVoronoiDiagram(const System::Collection::Array<1, Algebra::Vector<3, T>> & points,
                                             System::Collection::Array<1, T> & vd);

/** Calculate circular Voronoi diagram.

                Calculates the Voronoi diagram on 1-D unit circle manifold.
            
                @param[in] points List of points lying on a unit circle.
                @param[out] vd Lengths of the circular voronoi cell corresponding to given points.
            */
template <class T>
void GC_DLL_EXPORT HypersphereVoronoiDiagram(const System::Collection::Array<1, Algebra::Vector<2, T>> & points,
                                             System::Collection::Array<1, T> & vd);
}
}
} // namespace Gc::Math::Geometry

#endif
