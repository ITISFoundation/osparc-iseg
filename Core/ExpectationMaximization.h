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

namespace iseg {

// Warning: devs should really be matrices. But this would necessitate a matrix inversion...
class ISEG_CORE_API ExpectationMaximization
{
public:
	~ExpectationMaximization();

	void Init(short unsigned wi, short unsigned h, short nrclass, short dimension, float** bit, float* weight);
	void Init(short unsigned wi, short unsigned h, short nrclass, short dimension, float** bit, float* weight, float* center, float* dev, float* ampl);
	unsigned MakeIter(unsigned maxiter, unsigned converged);
	void Classify(float* result_bits);
	void ApplyTo(float** sources, float* result_bits);
	void InitCenters(float* center, float* dev, float* ampl);
	void InitCenters();
	void InitCentersRand();
	float* ReturnCenters();
	float* ReturnDevs();
	float* ReturnAmpls();

private:
	void RecomputeCenters();
	unsigned RecomputeMembership();
	short* m_M = nullptr;
	float* m_W = nullptr;
	float* m_Sw = nullptr;
	short m_Nrclasses = 0;
	short m_Dim = 0;
	float** m_Bits = nullptr;
	float* m_Weights = nullptr;
	float* m_Centers = nullptr;
	float* m_Devs = nullptr;
	float* m_Ampls = nullptr;
	short unsigned m_Width = 0;
	short unsigned m_Height = 0;
	unsigned m_Area = 0;
};

} // namespace iseg
