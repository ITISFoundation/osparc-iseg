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

#include "KMeans.h"

#include <cstdlib>
#include <float.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>

using namespace std;
using namespace iseg;

KMeans::KMeans()
{
	m = nullptr;
	centers = nullptr;
}

void KMeans::init(short unsigned w, short unsigned h, short nrclass,
				  short dimension, float** bit, float* weight)
{
	width = w;
	height = h;
	area = unsigned(w) * h;
	nrclasses = nrclass;
	dim = dimension;
	bits = bit;
	weights = weight;
	m = (short*)malloc(sizeof(unsigned) * area);
	centers = (float*)malloc(sizeof(float) * dim * nrclass);

	init_centers();

	return;
}

void KMeans::init(short unsigned w, short unsigned h, short nrclass,
				  short dimension, float** bit, float* weight, float* center)
{
	width = w;
	height = h;
	area = unsigned(w) * h;
	nrclasses = nrclass;
	dim = dimension;
	bits = bit;
	weights = weight;
	m = (short*)malloc(sizeof(unsigned) * area);
	centers = (float*)malloc(sizeof(float) * dim * nrclass);
	for (short i = 0; i < nrclass * dimension; i++)
		centers[i] = center[i];
	for (unsigned i = 0; i < area; i++)
		m[i] = -1;

	return;
}

unsigned KMeans::make_iter(unsigned maxiter, unsigned converged)
{
	/*
	vector<vector<float>> centersVc;
	vector<float> centersInit;
	centersInit.resize(nrclasses*dim,0);
	for(short i=0;i<dim;i++)
		for(short j=0;j<nrclasses;j++)
			centersInit[i+j*dim]=centers[i+j*dim];
	centersVc.push_back(centersInit);
	*/

	unsigned iter = 0;
	unsigned conv = area;
	while (iter++ < maxiter && conv > converged)
	{
		conv = recompute_membership();
		recompute_centers();

		/*
		vector<float> centersItVc;
		centersItVc.resize(nrclasses*dim,0);
		for(short i=0;i<dim;i++)
			for(short j=0;j<nrclasses;j++)
				centersItVc[i+j*dim]=centers[i+j*dim];
		centersVc.push_back(centersItVc);
		*/
	}

	/*	for(short i=0;i<nrclasses;i++)
		cout << centers[i] << " ";
	cout << endl;*/

	//	cout << iter << endl;

	return iter;
}

void KMeans::return_m(float* result_bits)
{
	for (unsigned i = 0; i < area; i++)
		result_bits[i] = 255.0f / (nrclasses - 1) * m[i];
	return;
}

void KMeans::apply_to(float** sources, float* result_bits)
{
	float dist, distmin;
	short unsigned cindex;
	short dummy;

	for (unsigned i = 0; i < area; i++)
	{
		dummy = 0;
		distmin = 0;
		cindex = 0;
		for (short n = 0; n < dim; n++)
		{
			distmin += (sources[n][i] - centers[cindex]) *
					   (sources[n][i] - centers[cindex]) * weights[n];
			cindex++;
		}
		for (short l = 1; l < nrclasses; l++)
		{
			dist = 0;
			for (short n = 0; n < dim; n++)
			{
				dist += (sources[n][i] - centers[cindex]) *
						(sources[n][i] - centers[cindex]) * weights[n];
				cindex++;
			}
			if (dist < distmin)
			{
				distmin = dist;
				dummy = l;
			}
		}
		result_bits[i] = 255.0f / (nrclasses - 1) * dummy;
	}

	return;
}

void KMeans::recompute_centers()
{
	vector<unsigned> count;
	vector<float> newcenters;
	count.resize(nrclasses, 0);
	newcenters.resize(nrclasses * dim, 0);

	unsigned dummy;
	for (unsigned j = 0; j < area; j++)
	{
		dummy = m[j];
		count[dummy]++;
		for (short i = 0; i < dim; i++)
		{
			newcenters[i + dummy * dim] += bits[i][j];
		}
	}
	for (short i = 0; i < dim; i++)
	{
		for (short j = 0; j < nrclasses; j++)
		{
			if (count[j] != 0)
				centers[i + j * dim] = newcenters[i + j * dim] / count[j];
		}
	}

	return;
}

unsigned KMeans::recompute_membership()
{
	unsigned count = 0;
	float dist, distmin;
	short unsigned cindex;
	short dummy;

	for (unsigned i = 0; i < area; i++)
	{
		dummy = 0;
		distmin = 0;
		cindex = 0;
		for (short n = 0; n < dim; n++)
		{
			distmin += (bits[n][i] - centers[cindex]) *
					   (bits[n][i] - centers[cindex]) * weights[n];
			cindex++;
		}
		for (short l = 1; l < nrclasses; l++)
		{
			dist = 0;
			for (short n = 0; n < dim; n++)
			{
				dist += (bits[n][i] - centers[cindex]) *
						(bits[n][i] - centers[cindex]) * weights[n];
				cindex++;
			}
			if (dist < distmin)
			{
				distmin = dist;
				dummy = l;
			}
		}
		if (m[i] != dummy)
		{
			count++;
			m[i] = dummy;
		}
	}

	return count;
}

void KMeans::init_centers()
{
	// Get minimum and maximum values
	float* minVals = (float*)malloc(sizeof(float) * dim);
	float* maxVals = (float*)malloc(sizeof(float) * dim);
	for (short i = 0; i < dim; i++)
	{
		minVals[i] = FLT_MAX;
		maxVals[i] = 0.0f;
	}
	for (unsigned j = 0; j < area; j++)
	{
		for (short i = 0; i < dim; i++)
		{
			if (minVals[i] > bits[i][j])
			{
				minVals[i] = bits[i][j];
			}
			if (maxVals[i] < bits[i][j])
			{
				maxVals[i] = bits[i][j];
			}
		}
	}

	if (nrclasses == 27 && dim == 3)
	{
		for (short l = 0; l < nrclasses; l++)
		{ //equally spaced for 3D, 27 classes
			centers[l * 3 + 0] =
				maxVals[0] - (l % 3 + .5f) * (maxVals[0] - minVals[0]) / 3;
			centers[l * 3 + 1] = maxVals[1] - ((l / 3) % 3 + .5f) *
												  (maxVals[1] - minVals[1]) / 3;
			centers[l * 3 + 2] =
				maxVals[2] - (l / 9 + .5f) * (maxVals[2] - minVals[2]) / 3;
		}
	}
	else
	{
		// Distribute centers uniformly within range
		for (short l = 0; l < nrclasses; l++)
		{
			for (short i = 0; i < dim; i++)
			{
				centers[l * dim + i] =
					maxVals[i] -
					(float)l / (nrclasses - 1.0f) * (maxVals[i] - minVals[i]);
			}
		}

		/*
		for(short l=0;l<nrclasses;l++){
			for(short i=0;i<dim;i++){
				centers[l*dim+i] = maxVals[i] -rand()%100*(maxVals[i] - minVals[i])/99;
			}
		}
		*/
	}

	free(minVals);
	free(maxVals);
	return;
}

void KMeans::init_centers_rand()
{
	unsigned j;

	for (short i = 0; i < nrclasses; i++)
	{
		j = (unsigned)(rand() % area);
		for (short k = 0; k < dim; k++)
			centers[k + i * dim] = bits[k][j];
	}

	for (unsigned i = 0; i < area; i++)
		m[i] = -1;

	return;
}

void KMeans::init_centers(float* center)
{
	for (short i = 0; i < nrclasses * dim; i++)
		centers[i] = center[i];
	return;
}

bool KMeans::get_centers_from_file(const std::string& fileName, float*& centers,
								   int& dimensions, int& classes)
{
	std::vector<float> center_vector;
	int className = -1;
	dimensions = 0;
	classes = 0;

	FILE* initFile;
	char readChar[100];
	initFile = fopen(fileName.c_str(), "r");
	if (initFile == nullptr)
	{
		perror("ERROR: opening file");
		return false;
	}
	else
	{
		while (fgets(readChar, 100, initFile) != nullptr)
		{
			string str(readChar);
			std::stringstream linestream(str);
			std::string classNameStr;
			std::getline(linestream, classNameStr, '\t');
			int classNameAux = -1;
			float value = -1;
			classNameAux = stoi(classNameStr);
			linestream >> value;

			if ((classNameAux == -1) || (value == -1))
				return false;

			center_vector.push_back(value);
			if (classNameAux != className)
			{
				className = classNameAux;
				classes++;
				dimensions = 1;
			}
			else
				dimensions++;
		}
	}

	centers = (float*)malloc(sizeof(float) * dimensions * classes);
	for (int i = 0; i < center_vector.size(); i++)
	{
		centers[i] = center_vector[i];
	}

	return true;
}

float* KMeans::return_centers() { return centers; }

KMeans::~KMeans()
{
	free(m);
	free(centers);
	return;
}
