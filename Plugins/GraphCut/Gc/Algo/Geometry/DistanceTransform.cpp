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

#include <limits>
#include "../../Energy/Neighbourhood.h"
#include "../../Data/BorderIterator.h"
#include "../../System/ArgumentException.h"
#include "DistanceTransform.h"

namespace Gc
{
    namespace Algo
    {
        namespace Geometry
        {
            namespace DistanceTransform
            {
                /******************************************************************************/

                template <Size N, class T>
                static void Scan(System::Collection::Array<N,T> &map, const Energy::Neighbourhood<N,Int32> &nb)
                {
                    // Calculate neighbour offsets
                    System::Collection::Array<1,Int32> ofs;
                    map.Indexes(nb, ofs);

                    // Check if we're doing forward or backward scan
                    bool forward = ofs[0] < 0;

                    // Get neighbourhood extent
                    Math::Algebra::Vector<N,Size> bleft, bright;
                    nb.Extent(bleft, bright);

                    // Create data iterator
                    Data::BorderIterator<N> iter(map.Dimensions(), bleft, bright);
                    iter.Start(!forward);                    

                    // Scan forward or backward
                    Math::Algebra::Vector<N,Size> ncur;
                    T *p = &map[forward ? 0 : (map.Elements() - 1)];
                    Int32 addp = forward ? 1 : -1;
                    Size bsize;
                    
                    while (!iter.IsFinished())
                    {
                        ncur = iter.CurPos();

                        if (iter.NextBlock(bsize)) // Border, we have to check boundaries
                        {                            
                            while (bsize-- > 0)
                            {
                                for (Size i = 0; i < nb.Elements(); i++)
                                {
                                    if (map.IsNeighbourInside(ncur, nb[i]))
                                    {
                                        T nd = p[ofs[i]] + 1;
                                        if (*p > nd)
                                        {
                                            *p = nd;
                                        }
                                    }
                                }

                                p += addp;
                                iter.NextPos(ncur);
                            }
                        }
                        else // Safe zone, no need to check boundaries
                        {                                
                            while (bsize-- > 0)
                            {
                                for (Size i = 0; i < nb.Elements(); i++)
                                {
                                    if (*p > p[ofs[i]] + 1)
                                    {
                                        *p = p[ofs[i]] + 1;
                                    }
                                }

                                p += addp;
                            }
                        }
                    }
                }

                /******************************************************************************/

                template <Size N, class DATA, class T>
                void CityBlock(const System::Collection::Array<N,DATA> &mask,
                    DATA zero_val, System::Collection::Array<N,T> &map)
                {
                    // Generate neighbourhood
                    Energy::Neighbourhood<N,Int32> nb;
                    nb.Elementary();

                    // Init map
                    map.Resize(mask.Dimensions());
                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        map[i] = (mask[i] == zero_val) ? 0 : (std::numeric_limits<T>::max() - 1);
                    }

                    // Forward and backward scan
                    if (!mask.IsEmpty())
                    {
                        Scan(map, nb.Negative());
                        Scan(map, nb.Positive());
                    }
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<2,bool> &mask,
                    bool zero_val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<2,bool> &mask,
                    bool zero_val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<2,bool> &mask,
                    bool zero_val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<3,bool> &mask,
                    bool zero_val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<3,bool> &mask,
                    bool zero_val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<3,bool> &mask,
                    bool zero_val, System::Collection::Array<3,Uint64> &map);

                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT CityBlock(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<3,Uint64> &map);
                /** @endcond */

                /******************************************************************************/

                template <Size N, class DATA, class T>
                void ChessBoard(const System::Collection::Array<N,DATA> &mask,
                    DATA zero_val, System::Collection::Array<N,T> &map)
                {
                    // Generate neighbourhood
                    Energy::Neighbourhood<N,Int32> nb;
                    nb.Box(Math::Algebra::Vector<N,Size>(1), false, false);

                    // Init map
                    map.Resize(mask.Dimensions());
                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        map[i] = (mask[i] == zero_val) ? 0 : (std::numeric_limits<T>::max() - 1);
                    }

                    // Forward and backward scan
                    if (!mask.IsEmpty())
                    {
                        Scan(map, nb.Negative());
                        Scan(map, nb.Positive());
                    }
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<2,bool> &mask,
                    bool zero_val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<2,bool> &mask,
                    bool zero_val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<2,bool> &mask,
                    bool zero_val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<3,bool> &mask,
                    bool zero_val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<3,bool> &mask,
                    bool zero_val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<3,bool> &mask,
                    bool zero_val, System::Collection::Array<3,Uint64> &map);

                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoard(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 zero_val, System::Collection::Array<3,Uint64> &map);
                /** @endcond */

                /******************************************************************************/

                template <Size N, class DATA, class T>
                void CityBlockLocal(const System::Collection::Array<N,DATA> &mask,
                    DATA val, System::Collection::Array<N,T> &map)
                {
                    if (mask.Dimensions() != map.Dimensions())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "Mask and map dimensions "
                            "do not match.");
                    }

                    // Generate neighbourhood
                    Energy::Neighbourhood<N,Int32> nb;
                    nb.Elementary();

                    // Init temporary map
                    System::Collection::Array<N,T> temp_map;
                    temp_map.Resize(mask.Dimensions());
                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        temp_map[i] = (mask[i] != val) ? 0 : (std::numeric_limits<T>::max() - 1);
                    }

                    // Forward and backward scan
                    if (!mask.IsEmpty())
                    {
                        Scan(temp_map, nb.Negative());
                        Scan(temp_map, nb.Positive());
                    }

                    // Copy distances to output
                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        if (mask[i] == val)
                        {
                            map[i] = temp_map[i];
                        }
                    }
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,bool> &mask,
                    bool val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,bool> &mask,
                    bool val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,bool> &mask,
                    bool val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,bool> &mask,
                    bool val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,bool> &mask,
                    bool val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,bool> &mask,
                    bool val, System::Collection::Array<3,Uint64> &map);

                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 val, System::Collection::Array<3,Uint64> &map);

                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,Uint16> &mask,
                    Uint16 val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,Uint16> &mask,
                    Uint16 val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<2,Uint16> &mask,
                    Uint16 val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,Uint16> &mask,
                    Uint16 val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,Uint16> &mask,
                    Uint16 val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT CityBlockLocal(const System::Collection::Array<3,Uint16> &mask,
                    Uint16 val, System::Collection::Array<3,Uint64> &map);
                /** @endcond */

                /******************************************************************************/

                template <Size N, class DATA, class T>
                void ChessBoardLocal(const System::Collection::Array<N,DATA> &mask,
                    DATA val, System::Collection::Array<N,T> &map)
                {
                    if (mask.Dimensions() != map.Dimensions())
                    {
                        throw System::ArgumentException(__FUNCTION__, __LINE__, "Mask and map dimensions "
                            "do not match.");
                    }

                    // Generate neighbourhood
                    Energy::Neighbourhood<N,Int32> nb;
                    nb.Box(Math::Algebra::Vector<N,Size>(1), false, false);

                    // Init temporary map
                    System::Collection::Array<N,T> temp_map;
                    temp_map.Resize(mask.Dimensions());
                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        temp_map[i] = (mask[i] != val) ? 0 : (std::numeric_limits<T>::max() - 1);
                    }

                    // Forward and backward scan
                    if (!mask.IsEmpty())
                    {
                        Scan(temp_map, nb.Negative());
                        Scan(temp_map, nb.Positive());
                    }

                    // Copy distances to output
                    for (Size i = 0; i < mask.Elements(); i++)
                    {
                        if (mask[i] == val)
                        {
                            map[i] = temp_map[i];
                        }
                    }
                }

                /******************************************************************************/

                // Explicit instantiations
                /** @cond */
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,bool> &mask,
                    bool val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,bool> &mask,
                    bool val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,bool> &mask,
                    bool val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,bool> &mask,
                    bool val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,bool> &mask,
                    bool val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,bool> &mask,
                    bool val, System::Collection::Array<3,Uint64> &map);

                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,Uint8> &mask,
                    Uint8 val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,Uint8> &mask,
                    Uint8 val, System::Collection::Array<3,Uint64> &map);

                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,Uint16> &mask,
                    Uint16 val, System::Collection::Array<2,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,Uint16> &mask,
                    Uint16 val, System::Collection::Array<2,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<2,Uint16> &mask,
                    Uint16 val, System::Collection::Array<2,Uint64> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,Uint16> &mask,
                    Uint16 val, System::Collection::Array<3,Uint16> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,Uint16> &mask,
                    Uint16 val, System::Collection::Array<3,Uint32> &map);
                template void GC_DLL_EXPORT ChessBoardLocal(const System::Collection::Array<3,Uint16> &mask,
                    Uint16 val, System::Collection::Array<3,Uint64> &map);
                /** @endcond */

                /******************************************************************************/
            }
        }
    }
}
