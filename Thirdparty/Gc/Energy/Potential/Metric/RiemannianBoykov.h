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
    Pairwise clique potentials approximating a general Riemannian metric
    (Y. Boykov approximation).

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_ENERGY_POTENTIAL_METRIC_RIEMANNIANBOYKOV_H
#define GC_ENERGY_POTENTIAL_METRIC_RIEMANNIANBOYKOV_H

#include "../../../Core.h"
#include "../../../Type.h"
#include "../../../Math/Algebra/SquareMatrix.h"
#include "../../Neighbourhood.h"

namespace Gc {
namespace Energy {
namespace Potential {
namespace Metric {
/** 2-clique potentials approximating a general Riemannian metric. 

                    This class implements calculation of edge weights approximating
                    a general Riemannian metric. 

                    It is based on the following paper:
                    - Y. Boykov and V. Kolmogorov. <em>Computing geodesics and minimal 
                     surfaces via graph cuts</em>. In ICCV '03: Proceedings of the Ninth 
                     IEEE International Conference on Computer Vision, page 26, 2003.

                    Assume we have a regular orthogonal grid of nodes and that all these
                    nodes have equivalent neighbourhood \f$ \mathcal{N} = (n_1, n_2, \dots, n_N) \f$.
                    Further, assume we are given a metric tensor (symmetric positive definite 
                    matrix) \c D describing the Riemannian space in one given node.
                    
                    The weight for \c i-th edge in the neighbourhood and given metric 
                    tensor \c D is calculated as follows:
                    \f[
                        w_i = e_i \cdot \frac{\mathrm{det} \, D}{(u^T_i \cdot D \cdot u_i)^k} 
                    \f]
                    where \f$ e_i \f$ is the weight approximating the Euclidean metric,
                    \f$ u_i \f$ is unit vector in the direction of the \c i-th edge and \c k is
                    \f$ \frac{3}{2} \f$ or \f$ 2 \f$ for \c N=2 and \c N=3, respectively.

                    To get \f$ e_i \f$ we use our improved approximation implemented in
                    RiemannianDanek instead of the original approach. These weights are
                    calculated only once in the constructor and then the weights can
                    be updated very quickly when metric tensor changes using the described
                    Boykov's method. On the other hand the approximation error may be 
                    much larger (see the papers referenced in RiemannianDanek).

                    @see RiemannianDanek, SetMetricTensor(), SetTransformationMatrix().

                    @tparam N Number of dimensions.
                    @tparam T Precision.
                */
template <Size N, class T>
class RiemannianBoykov
{
  protected:
    /** Edge weights. */
    System::Collection::Array<1, T> m_rew;
    /** Normalized neighbourhood. */
    Neighbourhood<N, T> m_nno;
    /** Weights for a standard Euclidean metric. */
    System::Collection::Array<1, T> m_em;

  public:
    /** Constructor. 
                    
                        @param[in] n %Neighbourhood used.
                    */
    RiemannianBoykov(const Neighbourhood<N, T> & n);

    /** Specify the transformation of the Riemannian space.

                        @param[in] mt Positive definite symmetric matrix specifying the 
                        transformation of the space. The space is stretched in the directions 
                        given by the eigenvectors of this matrix by the amount equal to the
                        corresponding eigenvalues.
                        
                        To approximate Euclidean metric use identity matrix as the transformation
                        matrix. To simulate an anisotropic grid with resolution 
                        \f$ \delta = (\delta_1, \delta_2, \dots) \f$ use the following transformation
                        matrix \f$ T = \mathrm{diag}(\delta)\f$.
                         
                        @warning Don't confuse the transformation matrix with the metric tensor. 
                        The metric tensor is a square of the transformation matrix or conversely 
                        the transformation matrix is the square root of the metric tensor, i.e., it 
                        has the same eigenvectors but eigenvalues are square roots of the metric 
                        tensor ones. 

                        @see SetMetricTensor().
                    */
    RiemannianBoykov<N, T> & SetTransformationMatrix(const Math::Algebra::SquareMatrix<N, T> & mt);

    /** Specify the transformation of the Riemannian space.

                        @param[in] mt Positive definite symmetric matrix specifying the 
                        transformation of the space. The space is stretched in the directions 
                        given by the eigenvectors of this matrix by the amount equal to the
                        square root of the corresponding eigenvalues.
                        
                        To approximate Euclidean metric use identity matrix as the metric tensor
                        To simulate an anisotropic grid with resolution 
                        \f$ \delta = (\delta_1, \delta_2, \dots) \f$ use the following metric tensor
                        \f$ D = \mathrm{diag}(\delta_1^2, \delta_2^2, \dots)\f$.
                         
                        @warning Don't confuse the transformation matrix with the metric tensor. 
                        The metric tensor is a square of the transformation matrix or conversely 
                        the transformation matrix is the square root of the metric tensor, i.e., it 
                        has the same eigenvectors but eigenvalues are square roots of the metric 
                        tensor ones.

                        @see SetTransformationMatrix().
                    */
    RiemannianBoykov<N, T> & SetMetricTensor(const Math::Algebra::SquareMatrix<N, T> & mt);

    /** Get edge weights calculated for the current metric tensor. */
    const System::Collection::Array<1, T> & EdgeWeights() const
    {
        return m_rew;
    }

    /** Get the edge weight for i-th neighbour (using the current metric tensor). */
    T operator[](Size i) const
    {
        return m_rew[i];
    }
};
}
}
}
} // namespace Gc::Energy::Potential::Metric

#endif
