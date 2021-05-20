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
    Discrete energy specification interface.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_ENERGY_IENERGY_H
#define GC_ENERGY_IENERGY_H

#include "../Type.h"

namespace Gc {
/** Classes related to discrete energy functionals and their minimization. */
namespace Energy {
/** General second order discrete energy class.
        
            This class represents a discrete second order energy of the following type:
            \f[
                E(x) = \sum_{i \in \{1, \dots, n\}} E_i(x_i) + \sum_{\{i, j\} \in \mathcal{N}} E_{ij}(x_i, x_j) \, ,
            \f]
            where \f$ n \f$ is the number of nodes, \f$ x = (x_1, \dots, x_n) \f$ is a labeling of the nodes
            with \f$ x_i \in \{0, \dots, k-1\} \f$, where \f$ k \f$ is the number of possible labels,
            \f$ \mathcal{N} \subseteq \{1, \dots, n\}^2 \f$ is a neighbourhood system containing all unordered
            pairs of neighbouring nodes and \f$ E_i \f$ and \f$ E_{ij} \f$ are unary and binary functions
            on the labels, respectively, often called unary and pairwise clique potential functions or 
            data and smoothness costs.

            No special toplogy is assumed for the nodes. In case of a grid structure consider using
            IGridEnergy class instead. Similar type of energy often arises when dealing with labeling
            problems (segmentation, restoration, etc.) in computer vision or with problems specified 
            via Markov Random Fields.

            @see IGridEnergy.
        */
class IEnergy
{
  public:
    /** Virtual destructor. */
    virtual ~IEnergy()
    {}
};
} // namespace Energy
} // namespace Gc

#endif
