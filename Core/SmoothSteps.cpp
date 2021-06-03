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

#include "SmoothSteps.h"

#include <cstdio>
#include <cstdlib>

namespace iseg {

SmoothSteps::SmoothSteps()
{
	m_Mask = nullptr;
	m_Ownmask = false;
	m_Masklength = 0;
	m_Weights = nullptr;
}

SmoothSteps::~SmoothSteps()
{
	for (tissues_size_t i = 0; i < m_Nrtissues; i++)
	{
		delete[] m_Weights[i];
	}
	delete[] m_Weights;
	if (m_Ownmask)
		delete[] m_Mask;
}

void SmoothSteps::Dostepsmooth(tissues_size_t* line)
{
	if (m_Mask == nullptr || m_Weights == nullptr)
		return;
	if (m_Linelength < m_Masklength || m_Linelength < 2)
		return;

	/*int *counter=new int[linelength];
	for(unsigned short i=0;i<linelength;i++) counter[i]=0;
	float *counterfloat=new float[linelength];
	for(unsigned short i=0;i<linelength;i++) counterfloat[i]=0;*/

	unsigned short n1 = m_Masklength / 2;
	for (tissues_size_t c = 0; c < m_Nrtissues; c++)
	{
		for (unsigned short i = 0; i < m_Linelength; i++)
			m_Weights[c][i] = 0; //,counter[i]=0,counterfloat[i]=0;
	}
	unsigned short upper = m_Linelength + 1 - m_Masklength;
	for (unsigned short i = 0; i < upper; i++)
	{
		tissues_size_t index = line[i + n1];
		for (unsigned short j = 0; j < m_Masklength; j++)
		{
			m_Weights[index][i + j] +=
					m_Mask[j]; //,counter[i+j]++,counterfloat[i+j]+=mask[j];
		}
	}
	for (unsigned short i = 0; i < n1; i++)
	{
		tissues_size_t index = line[i];
		unsigned short k = 0;
		for (unsigned short j = n1 - i; j < m_Masklength; j++, k++)
		{
			m_Weights[index][k] +=
					m_Mask[j]; //,counter[k]++,counterfloat[k]+=mask[j];
		}
	}
	for (unsigned short i = m_Linelength - m_Masklength + n1 + 1; i < m_Linelength;
			 i++)
	{
		tissues_size_t index = line[i];
		unsigned short k = i - n1;
		for (unsigned short j = 0; k < m_Linelength; j++, k++)
		{
			m_Weights[index][k] +=
					m_Mask[j]; //,counter[k]++,counterfloat[k]+=mask[j];
		}
	}
	tissues_size_t index = line[0];
	upper = m_Masklength - n1 - 1;
	for (unsigned short i = 0; i < upper; i++)
	{
		for (unsigned short j = i + n1 + 1; j < m_Masklength; j++)
		{
			m_Weights[index][i] +=
					m_Mask[j]; //,counter[i]++,counterfloat[i]+=mask[j];
		}
	}
	index = line[m_Linelength - 1];
	for (unsigned short i = m_Linelength - n1, upper = 1; i < m_Linelength;
			 i++, upper++)
	{
		for (unsigned short j = 0; j < upper; j++)
		{
			m_Weights[index][i] +=
					m_Mask[j]; //,counter[i]++,counterfloat[i]+=mask[j];
		}
	}
	for (unsigned short i = 0; i < m_Linelength; i++)
	{
		float valmax = 0;
		for (tissues_size_t c = 0; c < m_Nrtissues; c++)
		{
			if (m_Weights[c][i] > valmax)
			{
				line[i] = c;
				valmax = m_Weights[c][i];
			}
		}
	}
	//FILE *fp3=fopen("C:\\test.txt","w");
	//for(unsigned short i=0;i<linelength;i++) fprintf(fp3,"%i %f \n",counter[i],counterfloat[i]);
	//fclose(fp3);
	//delete[] counter;
	//delete[] counterfloat;
}

void SmoothSteps::Init(float* mask1, unsigned short masklength1, unsigned short linelength1, tissues_size_t nrtissues1)
{
	if (m_Ownmask)
		delete[] m_Mask;
	m_Ownmask = false;
	m_Mask = mask1;
	m_Masklength = masklength1;
	m_Linelength = linelength1;
	if (m_Weights != nullptr)
	{
		for (tissues_size_t i = 0; i < m_Nrtissues; i++)
		{
			delete[] m_Weights[i];
		}
		delete[] m_Weights;
	}
	m_Nrtissues = nrtissues1;
	m_Weights = new float*[m_Nrtissues];
	for (tissues_size_t i = 0; i < m_Nrtissues; i++)
	{
		m_Weights[i] = new float[m_Linelength];
	}
}

void SmoothSteps::Init(unsigned short masklength1, unsigned short linelength1, tissues_size_t nrtissues1)
{
	if (m_Ownmask)
		delete[] m_Mask;
	m_Mask = new float[masklength1];
	m_Masklength = masklength1;
	GenerateBinommask();
	m_Ownmask = true;
	m_Linelength = linelength1;
	if (m_Weights != nullptr)
	{
		for (tissues_size_t i = 0; i < m_Nrtissues; i++)
		{
			delete[] m_Weights[i];
		}
		delete[] m_Weights;
	}
	m_Nrtissues = nrtissues1;
	m_Weights = new float*[m_Nrtissues];
	for (tissues_size_t i = 0; i < m_Nrtissues; i++)
	{
		m_Weights[i] = new float[m_Linelength];
	}
}

void SmoothSteps::GenerateBinommask()
{
	m_Mask[0] = 1.0f;
	for (unsigned short i = 1; i < m_Masklength; i++)
	{
		m_Mask[i] = 1.0;
		for (unsigned short j = i - 1; j > 0; j--)
		{
			m_Mask[j] += m_Mask[j - 1];
		}
	}
	m_Mask[m_Masklength / 2] += 0.1;
}

} // namespace iseg
