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

class iSegCore_API multidimgamma
{
public:
	multidimgamma();
	void init(short unsigned w, short unsigned h, short nrclass, short dimension,
						float **bit, float *weight, float **centers1, float *tol_f1,
						float *tol_d1, float dx1, float dy1);
	void execute();
	void return_image(float *result_bits);
	~multidimgamma();

private:
	short *m;
	short nrclasses;
	short dim;
	float **bits;
	float *weights;
	float **centers;
	float *tol_f;
	float *tol_d;
	float *minvals;
	float dx;
	float dy;
	short unsigned width;
	short unsigned height;
	unsigned area;
};
