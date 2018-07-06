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

#include "MultidimensionalGamma.h"

#include <algorithm>
#include <cstdlib>
#include <vector>

using namespace std;
using namespace iseg;

MultidimensionalGamma::MultidimensionalGamma() { m = nullptr; }

void MultidimensionalGamma::init(short unsigned w, short unsigned h,
								 short nrclass, short dimension, float** bit,
								 float* weight, float** centers1, float* tol_f1,
								 float* tol_d1, float dx1, float dy1)
{
	width = w;
	height = h;
	area = unsigned(w) * h;
	nrclasses = nrclass;
	dim = dimension;
	bits = bit;
	centers = centers1;
	weights = weight;
	tol_f = tol_f1;
	tol_d = tol_d1;
	dx = dx1;
	dy = dy1;
	m = (short*)malloc(sizeof(unsigned) * area);
	minvals = (float*)malloc(sizeof(float) * area);

	return;
}

void MultidimensionalGamma::return_image(float* result_bits)
{
	for (unsigned i = 0; i < area; i++)
		result_bits[i] = 255.0f / (nrclasses - 1) * m[i];
	return;
}

MultidimensionalGamma::~MultidimensionalGamma()
{
	free(m);
	free(minvals);
	return;
}

void MultidimensionalGamma::execute()
{
	float val;
	for (unsigned i = 0; i < area; i++)
	{
		minvals[i] = 123E30f;
		m[i] = 0;
	}
	for (short classnr = 0; classnr < nrclasses; classnr++)
	{
		for (unsigned i = 0; i < area; i++)
		{
			val = (bits[0][i] - centers[classnr][0]) *
				  (bits[0][i] - centers[classnr][0]) * weights[0] * weights[0] /
				  (tol_f[0] * tol_f[0]);
			for (short dimnr = 1; dimnr < dim; dimnr++)
			{
				float factor1 =
					tol_f[dimnr] * tol_f[dimnr] / (tol_d[dimnr] * tol_d[dimnr]);
				float minvaldim = (bits[dimnr][i] - centers[classnr][dimnr]) *
								  (bits[dimnr][i] - centers[classnr][dimnr]);
				float dummy = (bits[dimnr][i] - centers[classnr][dimnr]) *
							  tol_d[dimnr] / tol_f[dimnr];
				if (dummy < 0)
					dummy = -dummy;
				if (dummy > 3 * tol_d[dimnr])
					dummy = 3 * tol_d[dimnr];
				int nxsize = int(dummy / dx);
				int nysize = int(dummy / dy);
				int nxmin, nymin, nxmax, nymax;
				nxmin = -std::min(nxsize, (int)(i % width));
				nxmax = std::min(nxsize, width - (int)(i % width));
				nymin = -std::min(nysize, (int)(i / width));
				nymax = std::min(nysize, height - (int)(i / width));
				unsigned j = i + nxmin + nymin * width;
				for (int ny = nymin; ny < nymax; ny++)
				{
					for (int nx = nxmin; nx < nxmax; nx++)
					{
						float valdim =
							(bits[dimnr][j] - centers[classnr][dimnr]) *
								(bits[dimnr][j] - centers[classnr][dimnr]) +
							factor1 * (nx * nx + ny * ny);
						if (valdim < minvaldim)
							minvaldim = valdim;
						j++;
					}
					j += width + nxmin - nxmax;
				}
				val += minvaldim * weights[dimnr] * weights[dimnr] /
					   (tol_f[dimnr] * tol_f[dimnr]);
			}
			if (val < minvals[i])
			{
				minvals[i] = val;
				m[i] = classnr;
			}
		}
	}
}
