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

#include "Data/Point.h"

#include "Core/SliceProvider.h"

#include <vector>

namespace iseg {

class bmphandler;

class Levelset
{
public:
	Levelset();
	void init(unsigned short h, unsigned short w, Point p, float* kbit,
			  float* Pbit, float balloon, float epsilon1, float step_size);
	void init(unsigned short h, unsigned short w, float* initial, float f,
			  float* kbit, float* Pbit, float balloon, float epsilon1,
			  float step_size);
	void init(unsigned short h, unsigned short w, float* levlset, float* kbit,
			  float* Pbit, float balloon, float epsilon1, float step_size);
	void iterate(unsigned nrsteps, unsigned updatefreq);
	void set_k(float* kbit);
	void set_P(float* Pbit);
	void return_levelset(float* output);
	void return_zerolevelset(std::vector<std::vector<Point>>* v1,
							 std::vector<std::vector<Point>>* v2, int minsize);
	~Levelset();

private:
	bool loaded;
	bool ownlvlset;
	bmphandler* image;
	void reinitialize();
	void make_step();
	float stepsize;
	float epsilon;
	float balloon1;
	unsigned short width;
	unsigned short height;
	unsigned area;
	//		float *levset;
	float* devx;
	float* devy;
	float* devxx;
	float* devxy;
	float* devyy;
	float* kbits;
	float* Pbits;
	float* Px;
	float* Py;
	void diffx(float* input, float* output);
	void diffy(float* input, float* output);
	void diffxx(float* input, float* output);
	void diffxy(float* input, float* output);
	void diffyy(float* input, float* output);
	SliceProvider* sliceprovide;
	SliceProviderInstaller* sliceprovide_installer;
};

} // namespace iseg
