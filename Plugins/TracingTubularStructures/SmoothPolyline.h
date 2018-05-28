/*
* Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#pragma once

#include "Data/Vec3.h"

#include <Eigen/Core>
#include <Eigen/Sparse>

#include <iostream>

namespace iseg {

void ConstrainedLaplacian(std::vector<Vec3>& polyline, double penalty_weight)
{
	bool is_closed = !polyline.empty() && (polyline.front() == polyline.back());

	Eigen::MatrixXd points;
	points.resize(is_closed ? polyline.size() - 1 : polyline.size(), 3);
	for (int i = 0; i < points.rows(); ++i)
	{
		points(i, 0) = polyline[i][0];
		points(i, 1) = polyline[i][1];
		points(i, 2) = polyline[i][2];
	}
	std::vector<int> edges;
	for (int i = 0; i + 1 < points.rows(); ++i)
	{
		edges.push_back(i);
		edges.push_back(i + 1);
	}
	if (is_closed)
	{
		edges.push_back(edges.back());
		edges.push_back(0);
	}

	int num_dof = (int)points.rows();
	int num_edges = is_closed ? num_dof : num_dof - 1;
	int num_equations = num_dof + num_edges;
	assert(edges.size() == num_edges * 2);

	Eigen::MatrixXd b = Eigen::MatrixXd::Zero(num_equations, 3);
	Eigen::SparseMatrix<double> L;
	L.resize(num_equations, num_dof);

	typedef Eigen::Triplet<double, int> triplet_type;
	std::vector<triplet_type> triplets;
	for (int id = 0; id < num_edges; ++id)
	{
		int n1 = edges[id * 2];
		int n2 = edges[id * 2 + 1];

		auto p1 = polyline[n1];
		auto p2 = polyline[n2];

		triplets.push_back(triplet_type(n1, n1, 1.0));
		triplets.push_back(triplet_type(n2, n2, 1.0));

		triplets.push_back(triplet_type(n1, n2, -1.0));
		triplets.push_back(triplet_type(n2, n1, -1.0));

		triplets.push_back(triplet_type(num_dof + id, n1, penalty_weight));
		triplets.push_back(triplet_type(num_dof + id, n2, penalty_weight));

		b(num_dof + id, 0) = penalty_weight * (p1[0] + p2[0]);
		b(num_dof + id, 1) = penalty_weight * (p1[1] + p2[1]);
		b(num_dof + id, 2) = penalty_weight * (p1[2] + p2[2]);
	}
	L.setFromTriplets(triplets.begin(), triplets.end());

	if (!is_closed)
	{
		// set boundary conditions
		std::vector<int> is_fixed;
		is_fixed.push_back(0);
		is_fixed.push_back(num_dof - 1);

		for (auto id : is_fixed)
		{
			auto p = polyline[id];
			b(id, 0) = 1e5 * p[0];
			b(id, 1) = 1e5 * p[1];
			b(id, 2) = 1e5 * p[2];

			L.prune([id](int row, int, double) {
				return (row != id);
			});
			L.coeffRef(id, id) = 1e5;
		}
	}

	Eigen::ConjugateGradient<Eigen::SparseMatrix<double>> cg;
	Eigen::MatrixXd x = cg.compute(L.transpose() * L).solveWithGuess(L.transpose() * b, points);

	std::cerr << "Iterations: " << cg.iterations() << "\n";
	std::cerr << "Estimated error: " << cg.error() << "\n";
	if (cg.info() == Eigen::ComputationInfo::NoConvergence)
	{
		std::cerr << "ERROR: Conjugate gradient solver did not converge\n";
		return;
	}

	for (int i = 0; i < x.rows(); ++i)
	{
		polyline[i] = Vec3(x(i, 0), x(i, 1), x(i, 2));
	}

	if (is_closed)
	{
		polyline.back() = polyline.front();
	}
}

} // namespace iseg
