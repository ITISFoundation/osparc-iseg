/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "addLine.h"

#include <cstdlib>

namespace iseg {

using namespace std;

void addLine(vector<Point>* vP, Point p1, Point p2)
{
	short deltax, deltay, xinc1, xinc2, yinc1, yinc2, den, num, numadd,
		numpixels;
	Point p;

	deltax = abs(p2.px - p1.px); // The difference between the x's
	deltay = abs(p2.py - p1.py); // The difference between the y's
	p.px = p1.px;				 // Start x off at the first pixel
	p.py = p1.py;				 // Start y off at the first pixel

	if (p2.px >= p1.px) // The x-values are increasing
	{
		xinc1 = 1;
		xinc2 = 1;
	}
	else // The x-values are decreasing
	{
		xinc1 = -1;
		xinc2 = -1;
	}

	if (p2.py >= p1.py) // The y-values are increasing
	{
		yinc1 = 1;
		yinc2 = 1;
	}
	else // The y-values are decreasing
	{
		yinc1 = -1;
		yinc2 = -1;
	}

	if (deltax >= deltay) // There is at least one x-value for every y-value
	{
		xinc1 = 0; // Don't change the x when numerator >= denominator
		yinc2 = 0; // Don't change the y for every iteration
		den = deltax;
		num = deltax / 2;
		numadd = deltay;
		numpixels = deltax; // There are more x-values than y-values
	}
	else // There is at least one y-value for every x-value
	{
		xinc2 = 0; // Don't change the x for every iteration
		yinc1 = 0; // Don't change the y when numerator >= denominator
		den = deltay;
		num = deltay / 2;
		numadd = deltax;
		numpixels = deltay; // There are more y-values than x-values
	}

	for (short curpixel = 0; curpixel <= numpixels; curpixel++)
	{
		vP->push_back(p); // Draw the current pixel
		num += numadd;	// Increase the numerator by the top of the fraction
		if (num >= den)   // Check if numerator >= denominator
		{
			num -= den;	// Calculate the new numerator value
			p.px += xinc1; // Change the x as appropriate
			p.py += yinc1; // Change the y as appropriate
		}
		p.px += xinc2; // Change the x as appropriate
		p.py += yinc2; // Change the y as appropriate
	}
}

} // namespace iseg
