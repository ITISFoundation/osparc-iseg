/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "feature_extractor.h"
#include "Precompiled.h"

#include <algorithm>

using namespace std;

void feature_extractor::init(float *bit, Point p1, Point p2, short unsigned w,
														 short unsigned h)
{
	width = w;
	height = h;
	set_bits(bit);
	set_window(p1, p2);

	return;
}

void feature_extractor::set_bits(float *bit)
{
	bits = bit;
	return;
}

void feature_extractor::set_window(Point p1, Point p2)
{
	xmin = min(p1.px, p2.px);
	xmax = max(p1.px, p2.px);
	ymin = min(p1.py, p2.py);
	ymax = max(p1.py, p2.py);
	area = unsigned(ymax - ymin + 1) * (xmax - xmin + 1);
	valmin = valmax = bits[unsigned(ymin) * width + xmin];

	average = 0;
	for (short i = xmin; i <= xmax; i++)
	{
		for (short j = ymin; j <= ymax; j++)
		{
			average += bits[unsigned(j) * width + i];
			valmin = min(valmin, bits[unsigned(j) * width + i]);
			valmax = max(valmax, bits[unsigned(j) * width + i]);
		}
	}

	average /= area;

	return;
}

float feature_extractor::return_average() { return average; }

float feature_extractor::return_stddev()
{
	if (area != 1)
	{
		float stddev = 0;
		for (short i = xmin; i <= xmax; i++)
		{
			for (short j = ymin; j <= ymax; j++)
			{
				stddev += (bits[unsigned(j) * width + i] - average) *
									(bits[unsigned(j) * width + i] - average);
			}
		}
		return sqrt(stddev / (area - 1));
	}
	else
		return 1E10;
}

void feature_extractor::return_extrema(Pair *p)
{
	p->high = valmax;
	p->low = valmin;
	return;
}

class feature_extractor_mask
{
public:
	void init(float *bit, float *mask1, short unsigned w, short unsigned h,
						float f1);
	void set_bits(float *bit);
	void set_mask(float *mask1);
	void set_f(float f1);
	float return_average();
	float return_stddev();
	void return_extrema(Pair *p);

private:
	float f;
	float *bits;
	float valmin;
	float valmax;
	float average;
	short unsigned width;
	short unsigned height;
	unsigned area;
	float *mask;
};

void feature_extractor_mask::init(float *bit, float *mask1, short unsigned w,
																	short unsigned h, float f1)
{
	width = w;
	height = h;
	f = f1;
	set_bits(bit);
	set_mask(mask1);

	return;
}

void feature_extractor_mask::set_bits(float *bit)
{
	bits = bit;
	return;
}

void feature_extractor_mask::set_mask(float *mask1)
{
	area = 0;
	unsigned pos = 0;
	mask = mask1;
	while (mask[pos] == 0 && pos < unsigned(width) * height)
		pos++;
	if (pos < unsigned(width) * height)
	{
		valmin = valmax = bits[pos];
		average = 0;
		for (; pos < unsigned(width) * height; pos++)
		{
			if (mask[pos] != 0)
			{
				area++;
				average += bits[pos];
				valmin = min(valmin, bits[pos]);
				valmax = max(valmax, bits[pos]);
			}
		}

		average /= area;
	}
	else
	{
		average = 0;
		area = 0;
	}

	return;
}

void feature_extractor_mask::set_f(float f1)
{
	f = f1;
	set_mask(mask);

	return;
}

float feature_extractor_mask::return_average() { return average; }

float feature_extractor_mask::return_stddev()
{
	if (area > 1)
	{
		float stddev = 0;
		for (unsigned pos = 0; pos < unsigned(width) * height; pos++)
		{
			if (mask[pos] != 0)
			{
				stddev += (bits[pos] - average) * (bits[pos] - average);
			}
		}

		return sqrt(stddev / (area - 1));
	}
	else
		return 1E10;
}

void feature_extractor_mask::return_extrema(Pair *p)
{
	p->high = valmax;
	p->low = valmin;
	return;
}
