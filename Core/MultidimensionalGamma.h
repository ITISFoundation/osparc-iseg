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

class ISEG_CORE_API MultidimensionalGamma
{
public:
	MultidimensionalGamma();
	void Init(short unsigned w, short unsigned h, short nrclass, short dimension, float** bit, float* weight, float** centers1, float* tol_f1, float* tol_d1, float dx1, float dy1);
	void Execute();
	void ReturnImage(float* result_bits);
	~MultidimensionalGamma();

private:
	short* m_M;
	short m_Nrclasses;
	short m_Dim;
	float** m_Bits;
	float* m_Weights;
	float** m_Centers;
	float* m_TolF;
	float* m_TolD;
	float* m_Minvals;
	float m_Dx;
	float m_Dy;
	short unsigned m_Width;
	short unsigned m_Height;
	unsigned m_Area;
};

} // namespace iseg
