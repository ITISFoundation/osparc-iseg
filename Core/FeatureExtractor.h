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

#include "Pair.h"
#include "Point.h"

namespace iseg {

class iSegCore_API FeatureExtractor
{
public:
	void init(float* bit, Point p1, Point p2, short unsigned w,
			  short unsigned h);
	void set_bits(float* bit);
	void set_window(Point p1, Point p2);
	float return_average();
	float return_stddev();
	void return_extrema(Pair* p);

private:
	float* bits;
	short xmin;
	short xmax;
	short ymin;
	short ymax;
	float valmin;
	float valmax;
	float average;
	short unsigned width;
	short unsigned height;
	unsigned area;
	float* mask;
};
} // namespace iseg
