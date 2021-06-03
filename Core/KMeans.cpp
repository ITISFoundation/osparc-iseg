/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "KMeans.h"

#include <cfloat>

namespace iseg {

KMeans::KMeans()
{
	m_M = nullptr;
	m_Centers = nullptr;
}

KMeans::~KMeans()
{
	free(m_M);
	free(m_Centers);
}

void KMeans::Init(short unsigned w, short unsigned h, short nrclass, short dimension, float** bit, float* weight)
{
	m_Width = w;
	m_Height = h;
	m_Area = unsigned(w) * h;
	m_Nrclasses = nrclass;
	m_Dim = dimension;
	m_Bits = bit;
	m_Weights = weight;
	m_M = (short*)malloc(sizeof(unsigned) * m_Area);
	m_Centers = (float*)malloc(sizeof(float) * m_Dim * nrclass);

	InitCenters();
}

void KMeans::Init(short unsigned w, short unsigned h, short nrclass, short dimension, float** bit, float* weight, float* center)
{
	m_Width = w;
	m_Height = h;
	m_Area = unsigned(w) * h;
	m_Nrclasses = nrclass;
	m_Dim = dimension;
	m_Bits = bit;
	m_Weights = weight;
	m_M = (short*)malloc(sizeof(unsigned) * m_Area);
	m_Centers = (float*)malloc(sizeof(float) * m_Dim * nrclass);
	for (short i = 0; i < nrclass * dimension; i++)
		m_Centers[i] = center[i];
	for (unsigned i = 0; i < m_Area; i++)
		m_M[i] = -1;
}

unsigned KMeans::MakeIter(unsigned maxiter, unsigned converged)
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
	unsigned conv = m_Area;
	while (iter++ < maxiter && conv > converged)
	{
		conv = RecomputeMembership();
		RecomputeCenters();

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

void KMeans::ReturnM(float* result_bits)
{
	for (unsigned i = 0; i < m_Area; i++)
		result_bits[i] = 255.0f / (m_Nrclasses - 1) * m_M[i];
}

void KMeans::ApplyTo(float** sources, float* result_bits)
{
	float dist, distmin;
	short unsigned cindex;
	short dummy;

	for (unsigned i = 0; i < m_Area; i++)
	{
		dummy = 0;
		distmin = 0;
		cindex = 0;
		for (short n = 0; n < m_Dim; n++)
		{
			distmin += (sources[n][i] - m_Centers[cindex]) *
								 (sources[n][i] - m_Centers[cindex]) * m_Weights[n];
			cindex++;
		}
		for (short l = 1; l < m_Nrclasses; l++)
		{
			dist = 0;
			for (short n = 0; n < m_Dim; n++)
			{
				dist += (sources[n][i] - m_Centers[cindex]) *
								(sources[n][i] - m_Centers[cindex]) * m_Weights[n];
				cindex++;
			}
			if (dist < distmin)
			{
				distmin = dist;
				dummy = l;
			}
		}
		result_bits[i] = 255.0f / (m_Nrclasses - 1) * dummy;
	}
}

void KMeans::RecomputeCenters()
{
	std::vector<unsigned> count;
	std::vector<float> newcenters;
	count.resize(m_Nrclasses, 0);
	newcenters.resize(m_Nrclasses * m_Dim, 0);

	unsigned dummy;
	for (unsigned j = 0; j < m_Area; j++)
	{
		dummy = m_M[j];
		count[dummy]++;
		for (short i = 0; i < m_Dim; i++)
		{
			newcenters[i + dummy * m_Dim] += m_Bits[i][j];
		}
	}
	for (short i = 0; i < m_Dim; i++)
	{
		for (short j = 0; j < m_Nrclasses; j++)
		{
			if (count[j] != 0)
				m_Centers[i + j * m_Dim] = newcenters[i + j * m_Dim] / count[j];
		}
	}
}

unsigned KMeans::RecomputeMembership()
{
	unsigned count = 0;
	float dist, distmin;
	short unsigned cindex;
	short dummy;

	for (unsigned i = 0; i < m_Area; i++)
	{
		dummy = 0;
		distmin = 0;
		cindex = 0;
		for (short n = 0; n < m_Dim; n++)
		{
			distmin += (m_Bits[n][i] - m_Centers[cindex]) *
								 (m_Bits[n][i] - m_Centers[cindex]) * m_Weights[n];
			cindex++;
		}
		for (short l = 1; l < m_Nrclasses; l++)
		{
			dist = 0;
			for (short n = 0; n < m_Dim; n++)
			{
				dist += (m_Bits[n][i] - m_Centers[cindex]) *
								(m_Bits[n][i] - m_Centers[cindex]) * m_Weights[n];
				cindex++;
			}
			if (dist < distmin)
			{
				distmin = dist;
				dummy = l;
			}
		}
		if (m_M[i] != dummy)
		{
			count++;
			m_M[i] = dummy;
		}
	}

	return count;
}

void KMeans::InitCenters()
{
	// Get minimum and maximum values
	float* min_vals = (float*)malloc(sizeof(float) * m_Dim);
	float* max_vals = (float*)malloc(sizeof(float) * m_Dim);
	for (short i = 0; i < m_Dim; i++)
	{
		min_vals[i] = FLT_MAX;
		max_vals[i] = 0.0f;
	}
	for (unsigned j = 0; j < m_Area; j++)
	{
		for (short i = 0; i < m_Dim; i++)
		{
			if (min_vals[i] > m_Bits[i][j])
			{
				min_vals[i] = m_Bits[i][j];
			}
			if (max_vals[i] < m_Bits[i][j])
			{
				max_vals[i] = m_Bits[i][j];
			}
		}
	}

	if (m_Nrclasses == 27 && m_Dim == 3)
	{
		for (short l = 0; l < m_Nrclasses; l++)
		{ //equally spaced for 3D, 27 classes
			m_Centers[l * 3 + 0] =
					max_vals[0] - (l % 3 + .5f) * (max_vals[0] - min_vals[0]) / 3;
			m_Centers[l * 3 + 1] = max_vals[1] - ((l / 3) % 3 + .5f) *
																							 (max_vals[1] - min_vals[1]) / 3;
			m_Centers[l * 3 + 2] =
					max_vals[2] - (l / 9 + .5f) * (max_vals[2] - min_vals[2]) / 3;
		}
	}
	else
	{
		// Distribute centers uniformly within range
		for (short l = 0; l < m_Nrclasses; l++)
		{
			for (short i = 0; i < m_Dim; i++)
			{
				m_Centers[l * m_Dim + i] =
						max_vals[i] -
						(float)l / (m_Nrclasses - 1.0f) * (max_vals[i] - min_vals[i]);
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

	free(min_vals);
	free(max_vals);
}

void KMeans::InitCentersRand()
{
	unsigned j;

	for (short i = 0; i < m_Nrclasses; i++)
	{
		j = (unsigned)(rand() % m_Area);
		for (short k = 0; k < m_Dim; k++)
			m_Centers[k + i * m_Dim] = m_Bits[k][j];
	}

	for (unsigned i = 0; i < m_Area; i++)
		m_M[i] = -1;
}

void KMeans::InitCenters(float* center)
{
	for (short i = 0; i < m_Nrclasses * m_Dim; i++)
		m_Centers[i] = center[i];
}

bool KMeans::GetCentersFromFile(const std::string& fileName, float*& centers, int& dimensions, int& classes)
{
	std::vector<float> center_vector;
	int class_name = -1;
	dimensions = 0;
	classes = 0;

	FILE* init_file;
	char read_char[100];
	init_file = fopen(fileName.c_str(), "r");
	if (init_file == nullptr)
	{
		perror("ERROR: opening file");
		return false;
	}
	else
	{
		while (fgets(read_char, 100, init_file) != nullptr)
		{
			std::string str(read_char);
			std::stringstream linestream(str);
			std::string class_name_str;
			std::getline(linestream, class_name_str, '\t');
			int class_name_aux = -1;
			float value = -1;
			class_name_aux = stoi(class_name_str);
			linestream >> value;

			if ((class_name_aux == -1) || (value == -1))
				return false;

			center_vector.push_back(value);
			if (class_name_aux != class_name)
			{
				class_name = class_name_aux;
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

float* KMeans::ReturnCenters() { return m_Centers; }

} // namespace iseg
