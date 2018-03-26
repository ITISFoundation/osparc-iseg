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
    Distance transform algorithms.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_ALGO_GEOMETRY_DISTANCETRANSFORM_H
#define GC_ALGO_GEOMETRY_DISTANCETRANSFORM_H

#include "../../Core.h"
#include "../../Type.h"
#include "../../System/Collection/Array.h"

namespace Gc
{
    namespace Algo
    {
        /** Digital geometry algorithms. */
        namespace Geometry
        {
            /** Distance transform algorithms. */
            namespace DistanceTransform
            {
                /** Compute distance transform of given mask using the city-block metric.

                    The computed distance is measured in voxels, no resolution is taken
                    into account.

                    @param[in] mask Mask image.
                    @param[in] zero_val Intensity value in the mask image with zero distance. 
                        Distance to the nearest \c zero_val voxel will be computed for 
                        all other voxels.
                    @param[out] map The final distance map. The distance of \c zero_val 
                        voxels is set to zero.
                */
                template <Size N, class DATA, class T>
                void GC_DLL_EXPORT CityBlock(const System::Collection::Array<N,DATA> &mask,
                    DATA zero_val, System::Collection::Array<N,T> &map);

                /** Compute distance transform of given mask using the chess-board metric.

                    The computed distance is measured in voxels, no resolution is taken
                    into account.

                    @param[in] mask Mask image.
                    @param[in] zero_val Intensity value in the mask image with zero distance. 
                        Distance to the nearest \c zero_val voxel will be computed for 
                        all other voxels.
                    @param[out] map The final distance map. The distance of \c zero_val 
                        voxels is set to zero.
                */
                template <Size N, class DATA, class T>
                void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<N,DATA> &mask,
                    DATA zero_val, System::Collection::Array<N,T> &map);

                /** Compute local distance transform of given mask using the city-block metric.

                    The computed distance is measured in voxels, no resolution is taken
                    into account. 

                    @param[in] mask Mask image.
                    @param[in] val Intensity value in the mask image for which their distance
                        from the nearest voxel with intensity different from \c val is computed.
                    @param[out] map The final distance map. Its size must be set before computation.
                        The distance of voxels with intensity different from \c val is left unchanged.
                */
                template <Size N, class DATA, class T>
                void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<N,DATA> &mask,
                    DATA val, System::Collection::Array<N,T> &map);

                /** Compute local distance transform of given mask using the chess-board metric.

                    The computed distance is measured in voxels, no resolution is taken
                    into account. 

                    @param[in] mask Mask image.
                    @param[in] val Intensity value in the mask image for which their distance
                        from the nearest voxel with intensity different from \c val is computed.
                    @param[out] map The final distance map. Its size must be set before computation.
                        The distance of voxels with intensity different from \c val is left unchanged.
                */
                template <Size N, class DATA, class T>
                void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<N,DATA> &mask,
                    DATA val, System::Collection::Array<N,T> &map);

            }
        }
    }
}

#endif
