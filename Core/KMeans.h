/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "iSegCore.h"

#include <string>
#include <vector>

namespace iseg {

class ISEG_CORE_API KMeans
{
public:
	KMeans();
	~KMeans();

	void Init(short unsigned w, short unsigned h, short nrclass, short dimension, float** bit, float* weight);
	void Init(short unsigned w, short unsigned h, short nrclass, short dimension, float** bit, float* weight, float* center);
	unsigned MakeIter(unsigned maxiter, unsigned converged);
	void ReturnM(float* result_bits);
	void ApplyTo(float** sources, float* result_bits);
	void InitCenters(float* center);
	void InitCenters();
	void InitCentersRand();
	/// BL TODO allocates centers using malloc -> probably never freed
	bool GetCentersFromFile(const std::string& fileName, float*& centers, int& dimensions, int& classes);
	float* ReturnCenters();

private:
	void RecomputeCenters();
	unsigned RecomputeMembership();
	short* m_M;
	short m_Nrclasses;
	short m_Dim;
	float** m_Bits;
	float* m_Weights;
	float* m_Centers;
	short unsigned m_Width;
	short unsigned m_Height;
	unsigned m_Area;
};

} // namespace iseg
