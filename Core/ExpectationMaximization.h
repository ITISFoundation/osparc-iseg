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

#include "iSegCore.h"

namespace iseg {

// Warning: devs should really be matrices. But this would necessitate a matrix inversion...
class iSegCore_API ExpectationMaximization
{
public:
	~ExpectationMaximization();

	void init(short unsigned wi, short unsigned h, short nrclass,
			  short dimension, float** bit, float* weight);
	void init(short unsigned wi, short unsigned h, short nrclass,
			  short dimension, float** bit, float* weight, float* center,
			  float* dev, float* ampl);
	unsigned make_iter(unsigned maxiter, unsigned converged);
	void classify(float* result_bits);
	void apply_to(float** sources, float* result_bits);
	void init_centers(float* center, float* dev, float* ampl);
	void init_centers();
	void init_centers_rand();
	float* return_centers();
	float* return_devs();
	float* return_ampls();

private:
	void recompute_centers();
	unsigned recompute_membership();
	short* m = nullptr;
	float* w = nullptr;
	float* sw = nullptr;
	short nrclasses = 0;
	short dim = 0;
	float** bits = nullptr;
	float* weights = nullptr;
	float* centers = nullptr;
	float* devs = nullptr;
	float* ampls = nullptr;
	short unsigned width = 0;
	short unsigned height = 0;
	unsigned area = 0;
};

} // namespace iseg
