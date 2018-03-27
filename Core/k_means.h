/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef KMEANS
#define KMEANS

#include "iSegCore.h"

#include <string>
#include <vector>

class iSegCore_API k_means
{
public:
	k_means();
	~k_means();

	void init(short unsigned w, short unsigned h, short nrclass, short dimension,
						float **bit, float *weight);
	void init(short unsigned w, short unsigned h, short nrclass, short dimension,
						float **bit, float *weight, float *center);
	unsigned make_iter(unsigned maxiter, unsigned converged);
	void return_m(float *result_bits);
	void apply_to(float **sources, float *result_bits);
	void init_centers(float *center);
	void init_centers();
	void init_centers_rand();
	/// BL TODO allocates centers using malloc -> probably never freed
	bool get_centers_from_file(const std::string &fileName, float *&centers,
														 int &dimensions, int &classes);
	float *return_centers();

private:
	void recompute_centers();
	unsigned recompute_membership();
	short *m;
	short nrclasses;
	short dim;
	float **bits;
	float *weights;
	float *centers;
	short unsigned width;
	short unsigned height;
	unsigned area;
};

#endif