/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "EM.h"
#include "Precompiled.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace std;

#define UNREFERENCED_PARAMETER(P) (P)

void EM::init(short unsigned wi, short unsigned h, short nrclass,
							short dimension, float **bit, float *weight)
{
	width = wi;
	height = h;
	area = unsigned(wi) * h;
	nrclasses = nrclass;
	dim = dimension;
	bits = bit;
	weights = weight;
	m = (short *)malloc(sizeof(short) * area * nrclasses);
	w = (float *)malloc(sizeof(float) * area * nrclasses);
	sw = (float *)malloc(sizeof(float) * nrclasses);
	centers = (float *)malloc(sizeof(float) * dim * nrclass);
	devs = (float *)malloc(sizeof(float) * nrclass);
	ampls = (float *)malloc(sizeof(float) * nrclass);

	init_centers();
	//	init_centers_rand();

	return;
}

void EM::init(short unsigned wi, short unsigned h, short nrclass,
							short dimension, float **bit, float *weight, float *center,
							float *dev, float *ampl)
{
	width = wi;
	height = h;
	area = unsigned(wi) * h;
	nrclasses = nrclass;
	dim = dimension;
	bits = bit;
	weights = weight;
	m = (short *)malloc(sizeof(short) * area * nrclasses);
	w = (float *)malloc(sizeof(float) * area * nrclasses);
	sw = (float *)malloc(sizeof(float) * nrclasses);
	centers = (float *)malloc(sizeof(float) * dim * nrclass);
	for (short i = 0; i < nrclass * dimension; i++)
		centers[i] = center[i];
	devs = (float *)malloc(sizeof(float) * nrclass);
	for (short i = 0; i < nrclass; i++)
		devs[i] = dev[i];
	ampls = (float *)malloc(sizeof(float) * nrclass);
	for (short i = 0; i < nrclass; i++)
		ampls[i] = ampl[i];
	for (unsigned i = 0; i < area; i++)
		m[i] = -1;

	return;
}

unsigned EM::make_iter(unsigned maxiter, unsigned converged)
{
	unsigned iter = 0;
	unsigned conv = area;
	while (iter++ < maxiter && conv > converged)
	{
		//		cout << iter<<":" <<conv << " ";
		conv = recompute_membership();
		//		cout << ":";
		recompute_centers();
		//		cout << ";";
		//		cout << conv << " ";
	}

	return iter;
}

void EM::classify(float *result_bits)
{
	for (unsigned i = 0; i < area; i++)
		result_bits[i] = 255.0f / (nrclasses - 1) * m[i];
	return;
}

void EM::apply_to(float **sources, float *result_bits)
{
	UNREFERENCED_PARAMETER(sources);
	float dist, wmax, wdummy;
	short unsigned cindex;
	short dummy;

	for (unsigned i = 0; i < area; i++)
	{
		dummy = 0;
		dist = 0;
		for (short n = 0; n < dim; n++)
		{
			dist +=
					(bits[n][i] - centers[n]) * (bits[n][i] - centers[n]) * weights[n];
		}
		wmax = exp(-dist / (2 * devs[0])) / sqrt(devs[0]);
		cindex = dim;
		for (short l = 1; l < nrclasses; l++)
		{
			dist = 0;
			for (short n = 0; n < dim; n++)
			{
				dist += (bits[n][i] - centers[cindex]) *
								(bits[n][i] - centers[cindex]) * weights[n];
				cindex++;
			}
			wdummy = exp(-dist / (2 * devs[l])) / sqrt(devs[l]);
			if (wdummy > wmax)
			{
				wmax = wdummy;
				dummy = l;
			}
		}
		result_bits[i] = 255.0f / (nrclasses - 1) * dummy;
	}

	return;
}

void EM::recompute_centers()
{
	float devold;
	for (short i = 0; i < nrclasses; i++)
	{
		ampls[i] = sw[i] / area;
		//		cout << "x"<<ampls[i];
	}
	//	cout << endl;

	for (short i = 0; i < nrclasses; i++)
	{
		if (sw[i] != 0)
		{
			for (short k = 0; k < dim; k++)
			{
				centers[k + i * dim] = 0;
				for (unsigned j = 0; j < area; j++)
				{
					centers[k + i * dim] += w[j + area * i] * bits[k][j];
				}
				centers[k + i * dim] /= sw[i];
				//				cout << "y"<<centers[k+i*dim];
			}

			devold = devs[i];
			devs[i] = 0;
			for (unsigned j = 0; j < area; j++)
			{
				for (short k = 0; k < dim; k++)
				{
					devs[i] += w[j + area * i] * (bits[k][j] - centers[k + i * dim]) *
										 (bits[k][j] - centers[k + i * dim]) * weights[k];
				}
			}
			devs[i] /= sw[i];
			if (devs[i] == 0)
				devs[i] = devold;
			//			cout << "z"<<devs[i]<<" ";
		}
	}
	//	cout << endl;
	//	cout << endl;

	return;
}

unsigned EM::recompute_membership()
{
	unsigned count = 0;
	float dist, wmax, wsum, wdummy;
	short unsigned cindex;
	short dummy;

	for (short l = 0; l < nrclasses; l++)
	{
		sw[l] = 0;
	}

	for (unsigned i = 0; i < area; i++)
	{
		dummy = 0;
		dist = 0;
		for (short n = 0; n < dim; n++)
		{
			dist +=
					(bits[n][i] - centers[n]) * (bits[n][i] - centers[n]) * weights[n];
		}
		//		wsum=wmax=w[i]=exp(-dist/(2*devs[0]))/sqrt(devs[0])*ampls[0];
		wmax = exp(-dist / (2 * devs[0])) / sqrt(devs[0]);
		wsum = w[i] = wmax * ampls[0];
		//		sw[0]+=wmin;
		cindex = dim;
		for (short l = 1; l < nrclasses; l++)
		{
			dist = 0;
			for (short n = 0; n < dim; n++)
			{
				dist += (bits[n][i] - centers[cindex]) *
								(bits[n][i] - centers[cindex]) * weights[n];
				cindex++;
			}
			wdummy = exp(-dist / (2 * devs[l])) / sqrt(devs[l]);
			w[i + area * l] = wdummy * ampls[l];
			//			w[i+area*l]=exp(-dist/(2*devs[l]))/sqrt(devs[l])*ampls[l];
			wsum += w[i + area * l];
			if (wdummy > wmax)
			{
				wmax = wdummy;
				dummy = l;
			}
			//			if(w[i+area*l]>wmax) {wmax=w[i+area*l]; dummy=l;}
		}
		if (m[i] != dummy)
		{
			count++;
			m[i] = dummy;
		}
		for (short l = 0; l < nrclasses; l++)
		{
			if (wsum == 0)
			{
				w[i + area * l] = 1.0f / nrclasses;
			}
			else
			{
				w[i + area * l] /= wsum;
			}
			//			sw[l]/=wsum;
		}
	}

	for (short l = 0; l < nrclasses; l++)
	{
		for (unsigned i = 0; i < area; i++)
		{
			sw[l] += w[i + l * area];
		}
	}

	//	count=10000;

	return count;
}

void EM::init_centers()
{
	for (short j = 0; j < nrclasses; j++)
		sw[j] = 0;
	for (short j = 0; j < nrclasses; j++)
	{
		for (unsigned i = 0; i < area; i++)
			w[i + area * j] = 0;
		for (unsigned i = j; i < area; i += nrclasses)
		{
			w[i + area * j] = 1;
			sw[j]++;
		}
	}

	recompute_centers();

	//	for(short i=0;i<nrclasses;i++) devs[i]=float(width)/nrclasses;
	for (short i = 0; i < nrclasses; i++)
		ampls[i] = 1.0f / nrclasses;

	return;
}

void EM::init_centers_rand()
{
	unsigned j;

	for (short i = 0; i < nrclasses; i++)
	{
		j = unsigned(area * float(rand()) / RAND_MAX);
		for (short k = 0; k < dim; k++)
		{
			centers[i + k * dim] = bits[k][j];
			//			cout << j << "-"<<centers[i+k*dim] << ":";
		}
	}

	float dist = 0;
	float minim, maxim;
	for (short i = 0; i < dim; i++)
	{
		minim = maxim = bits[i][0];
		for (j = 1; j < area; j++)
		{
			minim = std::min(bits[i][j], minim);
			maxim = std::max(bits[i][j], maxim);
		}
		dist += (maxim - minim) * (maxim - minim);
	}

	for (short i = 0; i < nrclasses; i++)
		devs[i] = dist / (nrclasses * nrclasses);

	for (short i = 0; i < nrclasses; i++)
		ampls[i] = 1.0f / nrclasses;

	return;
}

void EM::init_centers(float *center, float *dev, float *ampl)
{
	for (short i = 0; i < nrclasses * dim; i++)
		centers[i] = center[i];
	for (short i = 0; i < nrclasses; i++)
		devs[i] = dev[i];
	for (short i = 0; i < nrclasses; i++)
		ampls[i] = ampl[i];
	return;
}

float *EM::return_centers() { return centers; }

float *EM::return_devs() { return devs; }

float *EM::return_ampls() { return ampls; }

EM::~EM()
{
	free(w);
	free(sw);
	free(centers);
	free(devs);
	free(ampls);
	return;
}
