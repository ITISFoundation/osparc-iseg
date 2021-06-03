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
    KMeans clustering algorithm.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_ALGO_CLUTERING_KMEANS_H
#define GC_ALGO_CLUTERING_KMEANS_H

#include "../../Core.h"
#include "../../System/Collection/Array.h"
#include "../../System/Collection/IArrayMask.h"

namespace Gc {
namespace Algo {
/** %Clustering algorithms. */
namespace Clustering {
/** Namespace containing different %KMeans clustering implementations. */
namespace KMeans {
/** This method computes a weighted %KMeans clustering of the intensities in 
                    the input image using Lloyd's algorithm.

                    @remarks It has been shown, that %KMeans clustering provides very good initial guess
                    for the piecewise constant Mumford-Shah models (including the Chan-Vese model). We 
                    recommend to use this approach for the initialization.

                    @param[in] im Input image. 
                    @param[in] k Number of partitions.
                    @param[in] lambda Weights of the partitions.
                    @param[in] conv_crit Convergence criterion. When \f$\sum_{i = 1}^k |\Delta c_i|\f$ is
                        less than or equal to this value the computation ends.
                    @param[in] max_iter Maximal number of iterations.
                    @param[out] mean The mean values \f$ c_i \f$

                    @return Number of iterations performed.

                    @see ChanVese, MumfordShah.

                    @tparam N Number of dimensions.
                    @tparam T Type of the input data and also precision used during the computation.
                */
template <Size N, class T>
Size Lloyd(const System::Collection::Array<N, T> & im, Size k,
           const System::Collection::Array<1, T> & lambda, T conv_crit,
           Size max_iter, System::Collection::Array<1, T> & mean);

/** This method computes weighted %KMeans clustering of the intensities in 
                    a masked input image using Lloyd's algorithm.

                    @see Lloyd().
                */
template <Size N, class T>
Size LloydMasked(const System::Collection::Array<N, T> & im,
                 const System::Collection::IArrayMask<N> & mask, Size k,
                 const System::Collection::Array<1, T> & lambda, T conv_crit,
                 Size max_iter, System::Collection::Array<1, T> & mean);
} // namespace KMeans
} // namespace Clustering
}
} // namespace Gc::Algo

#endif
