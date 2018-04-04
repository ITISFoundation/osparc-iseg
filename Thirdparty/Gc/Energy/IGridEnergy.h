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
    Discrete energy specification interface for problems on regular grids.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_ENERGY_IGRIDENERGY_H
#define GC_ENERGY_IGRIDENERGY_H

#include "../Type.h"
#include "../Math/Algebra/Vector.h"
#include "Neighbourhood.h"

namespace Gc
{    
	namespace Energy
	{
        /** Second order discrete energy class defined on a regular grid.
        
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

            It is assumed that the nodes form a regular grid and that the neighbourhood system is
            topologically equivalent for all nodes. For a general problem not satisfying these
            properties consider using IEnergy class instead. Similar type of energy often arises when 
            dealing with labeling problems (segmentation, restoration, etc.) in computer vision or 
            with problems specified via Markov Random Fields.

            @see IEnergy.
        */
        template <Size N, class T, class L>
	    class IGridEnergy
	    {
        public:
            /** Virtual destructor. */
            virtual ~IGridEnergy()
            {}

            /** Number of possible labels. */
            virtual Size Labels() const = 0;

            /** Grid dimensions. */
            virtual Math::Algebra::Vector<N,Size> Dimensions() const = 0;

            /** Get grid neighbourhood system. */
            virtual const Neighbourhood<N,Int32> &NbSystem() const = 0;

            /** Unary clique potentials, i.e., label penalties (data costs). 
            
                @param[in] node Node index.
                @param[in] label Label index.
                @return %Energy corresponding to the labeling of node \c node
                    with label \c label.
            */
            virtual T CliquePotential(Size node, L label) const = 0;

            /** Pairwise clique potentials, i.e., smoothness costs. 
            
                @param[in] n1 First node index.
                @param[in] n2 Second node index.
                @param[in] i Auxiliary parameter containing information which
                    neighbourhood vector generated the connection \c n1 -> \c n2.
                @param[in] l1 First label.
                @param[in] l2 Second label.
                @return %Energy corresponding to the labeling of nodes \c n1 and \c n2
                    with labels \c l1 and \c l2, respectively.
            */
            virtual T CliquePotential(Size n1, Size n2, Size i, L l1, L l2) const = 0;
	    };
	}
}

#endif
