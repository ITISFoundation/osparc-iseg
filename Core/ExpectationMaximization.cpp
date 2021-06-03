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

#include "ExpectationMaximization.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

namespace iseg {

void ExpectationMaximization::Init(short unsigned wi, short unsigned h, short nrclass, short dimension, float** bit, float* weight)
{
	m_Width = wi;
	m_Height = h;
	m_Area = unsigned(wi) * h;
	m_Nrclasses = nrclass;
	m_Dim = dimension;
	m_Bits = bit;
	m_Weights = weight;
	m_M = (short*)malloc(sizeof(short) * m_Area * m_Nrclasses);
	m_W = (float*)malloc(sizeof(float) * m_Area * m_Nrclasses);
	m_Sw = (float*)malloc(sizeof(float) * m_Nrclasses);
	m_Centers = (float*)malloc(sizeof(float) * m_Dim * nrclass);
	m_Devs = (float*)malloc(sizeof(float) * nrclass);
	m_Ampls = (float*)malloc(sizeof(float) * nrclass);

	InitCenters();
}

void ExpectationMaximization::Init(short unsigned wi, short unsigned h, short nrclass, short dimension, float** bit, float* weight, float* center, float* dev, float* ampl)
{
	m_Width = wi;
	m_Height = h;
	m_Area = unsigned(wi) * h;
	m_Nrclasses = nrclass;
	m_Dim = dimension;
	m_Bits = bit;
	m_Weights = weight;
	m_M = (short*)malloc(sizeof(short) * m_Area * m_Nrclasses);
	m_W = (float*)malloc(sizeof(float) * m_Area * m_Nrclasses);
	m_Sw = (float*)malloc(sizeof(float) * m_Nrclasses);
	m_Centers = (float*)malloc(sizeof(float) * m_Dim * nrclass);
	for (short i = 0; i < nrclass * dimension; i++)
		m_Centers[i] = center[i];
	m_Devs = (float*)malloc(sizeof(float) * nrclass);
	for (short i = 0; i < nrclass; i++)
		m_Devs[i] = dev[i];
	m_Ampls = (float*)malloc(sizeof(float) * nrclass);
	for (short i = 0; i < nrclass; i++)
		m_Ampls[i] = ampl[i];
	for (unsigned i = 0; i < m_Area; i++)
		m_M[i] = -1;
}

unsigned ExpectationMaximization::MakeIter(unsigned maxiter, unsigned converged)
{
	unsigned iter = 0;
	unsigned conv = m_Area;
	while (iter++ < maxiter && conv > converged)
	{
		//		cout << iter<<":" <<conv << " ";
		conv = RecomputeMembership();
		//		cout << ":";
		RecomputeCenters();
		//		cout << ";";
		//		cout << conv << " ";
	}

	return iter;
}

void ExpectationMaximization::Classify(float* result_bits)
{
	for (unsigned i = 0; i < m_Area; i++)
		result_bits[i] = 255.0f / (m_Nrclasses - 1) * m_M[i];
}

void ExpectationMaximization::ApplyTo(float** /* sources */, float* result_bits)
{
	float dist, wmax, wdummy;
	short unsigned cindex;
	short dummy;

	for (unsigned i = 0; i < m_Area; i++)
	{
		dummy = 0;
		dist = 0;
		for (short n = 0; n < m_Dim; n++)
		{
			dist += (m_Bits[n][i] - m_Centers[n]) * (m_Bits[n][i] - m_Centers[n]) *
							m_Weights[n];
		}
		wmax = exp(-dist / (2 * m_Devs[0])) / sqrt(m_Devs[0]);
		cindex = m_Dim;
		for (short l = 1; l < m_Nrclasses; l++)
		{
			dist = 0;
			for (short n = 0; n < m_Dim; n++)
			{
				dist += (m_Bits[n][i] - m_Centers[cindex]) *
								(m_Bits[n][i] - m_Centers[cindex]) * m_Weights[n];
				cindex++;
			}
			wdummy = exp(-dist / (2 * m_Devs[l])) / sqrt(m_Devs[l]);
			if (wdummy > wmax)
			{
				wmax = wdummy;
				dummy = l;
			}
		}
		result_bits[i] = 255.0f / (m_Nrclasses - 1) * dummy;
	}
}

void ExpectationMaximization::RecomputeCenters()
{
	float devold;
	for (short i = 0; i < m_Nrclasses; i++)
	{
		m_Ampls[i] = m_Sw[i] / m_Area;
		//		cout << "x"<<ampls[i];
	}
	//	cout << endl;

	for (short i = 0; i < m_Nrclasses; i++)
	{
		if (m_Sw[i] != 0)
		{
			for (short k = 0; k < m_Dim; k++)
			{
				m_Centers[k + i * m_Dim] = 0;
				for (unsigned j = 0; j < m_Area; j++)
				{
					m_Centers[k + i * m_Dim] += m_W[j + m_Area * i] * m_Bits[k][j];
				}
				m_Centers[k + i * m_Dim] /= m_Sw[i];
				//				cout << "y"<<centers[k+i*dim];
			}

			devold = m_Devs[i];
			m_Devs[i] = 0;
			for (unsigned j = 0; j < m_Area; j++)
			{
				for (short k = 0; k < m_Dim; k++)
				{
					m_Devs[i] += m_W[j + m_Area * i] *
											 (m_Bits[k][j] - m_Centers[k + i * m_Dim]) *
											 (m_Bits[k][j] - m_Centers[k + i * m_Dim]) * m_Weights[k];
				}
			}
			m_Devs[i] /= m_Sw[i];
			if (m_Devs[i] == 0)
				m_Devs[i] = devold;
			//			cout << "z"<<devs[i]<<" ";
		}
	}
	//	cout << endl;
	//	cout << endl;
}

unsigned ExpectationMaximization::RecomputeMembership()
{
	unsigned count = 0;
	float dist, wmax, wsum, wdummy;
	short unsigned cindex;
	short dummy;

	for (short l = 0; l < m_Nrclasses; l++)
	{
		m_Sw[l] = 0;
	}

	for (unsigned i = 0; i < m_Area; i++)
	{
		dummy = 0;
		dist = 0;
		for (short n = 0; n < m_Dim; n++)
		{
			dist += (m_Bits[n][i] - m_Centers[n]) * (m_Bits[n][i] - m_Centers[n]) *
							m_Weights[n];
		}
		//		wsum=wmax=w[i]=exp(-dist/(2*devs[0]))/sqrt(devs[0])*ampls[0];
		wmax = exp(-dist / (2 * m_Devs[0])) / sqrt(m_Devs[0]);
		wsum = m_W[i] = wmax * m_Ampls[0];
		//		sw[0]+=wmin;
		cindex = m_Dim;
		for (short l = 1; l < m_Nrclasses; l++)
		{
			dist = 0;
			for (short n = 0; n < m_Dim; n++)
			{
				dist += (m_Bits[n][i] - m_Centers[cindex]) *
								(m_Bits[n][i] - m_Centers[cindex]) * m_Weights[n];
				cindex++;
			}
			wdummy = exp(-dist / (2 * m_Devs[l])) / sqrt(m_Devs[l]);
			m_W[i + m_Area * l] = wdummy * m_Ampls[l];
			//			w[i+area*l]=exp(-dist/(2*devs[l]))/sqrt(devs[l])*ampls[l];
			wsum += m_W[i + m_Area * l];
			if (wdummy > wmax)
			{
				wmax = wdummy;
				dummy = l;
			}
			//			if(w[i+area*l]>wmax) {wmax=w[i+area*l]; dummy=l;}
		}
		if (m_M[i] != dummy)
		{
			count++;
			m_M[i] = dummy;
		}
		for (short l = 0; l < m_Nrclasses; l++)
		{
			if (wsum == 0)
			{
				m_W[i + m_Area * l] = 1.0f / m_Nrclasses;
			}
			else
			{
				m_W[i + m_Area * l] /= wsum;
			}
			//			sw[l]/=wsum;
		}
	}

	for (short l = 0; l < m_Nrclasses; l++)
	{
		for (unsigned i = 0; i < m_Area; i++)
		{
			m_Sw[l] += m_W[i + l * m_Area];
		}
	}

	//	count=10000;

	return count;
}

void ExpectationMaximization::InitCenters()
{
	for (short j = 0; j < m_Nrclasses; j++)
		m_Sw[j] = 0;
	for (short j = 0; j < m_Nrclasses; j++)
	{
		for (unsigned i = 0; i < m_Area; i++)
			m_W[i + m_Area * j] = 0;
		for (unsigned i = j; i < m_Area; i += m_Nrclasses)
		{
			m_W[i + m_Area * j] = 1;
			m_Sw[j]++;
		}
	}

	RecomputeCenters();

	for (short i = 0; i < m_Nrclasses; i++)
		m_Ampls[i] = 1.0f / m_Nrclasses;
}

void ExpectationMaximization::InitCentersRand()
{
	unsigned j;

	for (short i = 0; i < m_Nrclasses; i++)
	{
		j = unsigned(m_Area * float(rand()) / RAND_MAX);
		for (short k = 0; k < m_Dim; k++)
		{
			m_Centers[i + k * m_Dim] = m_Bits[k][j];
		}
	}

	float dist = 0;
	float minim, maxim;
	for (short i = 0; i < m_Dim; i++)
	{
		minim = maxim = m_Bits[i][0];
		for (j = 1; j < m_Area; j++)
		{
			minim = std::min(m_Bits[i][j], minim);
			maxim = std::max(m_Bits[i][j], maxim);
		}
		dist += (maxim - minim) * (maxim - minim);
	}

	for (short i = 0; i < m_Nrclasses; i++)
		m_Devs[i] = dist / (m_Nrclasses * m_Nrclasses);

	for (short i = 0; i < m_Nrclasses; i++)
		m_Ampls[i] = 1.0f / m_Nrclasses;
}

void ExpectationMaximization::InitCenters(float* center, float* dev, float* ampl)
{
	for (short i = 0; i < m_Nrclasses * m_Dim; i++)
		m_Centers[i] = center[i];
	for (short i = 0; i < m_Nrclasses; i++)
		m_Devs[i] = dev[i];
	for (short i = 0; i < m_Nrclasses; i++)
		m_Ampls[i] = ampl[i];
}

float* ExpectationMaximization::ReturnCenters() { return m_Centers; }

float* ExpectationMaximization::ReturnDevs() { return m_Devs; }

float* ExpectationMaximization::ReturnAmpls() { return m_Ampls; }

ExpectationMaximization::~ExpectationMaximization()
{
	free(m_W);
	free(m_Sw);
	free(m_Centers);
	free(m_Devs);
	free(m_Ampls);
}

} // namespace iseg
