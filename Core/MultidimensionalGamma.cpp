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

#include "MultidimensionalGamma.h"

namespace iseg {

MultidimensionalGamma::MultidimensionalGamma() { m_M = nullptr; }

void MultidimensionalGamma::Init(short unsigned w, short unsigned h, short nrclass, short dimension, float** bit, float* weight, float** centers1, float* tol_f1, float* tol_d1, float dx1, float dy1)
{
	m_Width = w;
	m_Height = h;
	m_Area = unsigned(w) * h;
	m_Nrclasses = nrclass;
	m_Dim = dimension;
	m_Bits = bit;
	m_Centers = centers1;
	m_Weights = weight;
	m_TolF = tol_f1;
	m_TolD = tol_d1;
	m_Dx = dx1;
	m_Dy = dy1;
	m_M = (short*)malloc(sizeof(unsigned) * m_Area);
	m_Minvals = (float*)malloc(sizeof(float) * m_Area);
}

void MultidimensionalGamma::ReturnImage(float* result_bits)
{
	for (unsigned i = 0; i < m_Area; i++)
		result_bits[i] = 255.0f / (m_Nrclasses - 1) * m_M[i];
}

MultidimensionalGamma::~MultidimensionalGamma()
{
	free(m_M);
	free(m_Minvals);
}

void MultidimensionalGamma::Execute()
{
	float val;
	for (unsigned i = 0; i < m_Area; i++)
	{
		m_Minvals[i] = 123E30f;
		m_M[i] = 0;
	}
	for (short classnr = 0; classnr < m_Nrclasses; classnr++)
	{
		for (unsigned i = 0; i < m_Area; i++)
		{
			val = (m_Bits[0][i] - m_Centers[classnr][0]) *
						(m_Bits[0][i] - m_Centers[classnr][0]) * m_Weights[0] * m_Weights[0] /
						(m_TolF[0] * m_TolF[0]);
			for (short dimnr = 1; dimnr < m_Dim; dimnr++)
			{
				float factor1 =
						m_TolF[dimnr] * m_TolF[dimnr] / (m_TolD[dimnr] * m_TolD[dimnr]);
				float minvaldim = (m_Bits[dimnr][i] - m_Centers[classnr][dimnr]) *
													(m_Bits[dimnr][i] - m_Centers[classnr][dimnr]);
				float dummy = (m_Bits[dimnr][i] - m_Centers[classnr][dimnr]) *
											m_TolD[dimnr] / m_TolF[dimnr];
				if (dummy < 0)
					dummy = -dummy;
				if (dummy > 3 * m_TolD[dimnr])
					dummy = 3 * m_TolD[dimnr];
				int nxsize = int(dummy / m_Dx);
				int nysize = int(dummy / m_Dy);
				int nxmin, nymin, nxmax, nymax;
				nxmin = -std::min(nxsize, (int)(i % m_Width));
				nxmax = std::min(nxsize, m_Width - (int)(i % m_Width));
				nymin = -std::min(nysize, (int)(i / m_Width));
				nymax = std::min(nysize, m_Height - (int)(i / m_Width));
				unsigned j = i + nxmin + nymin * m_Width;
				for (int ny = nymin; ny < nymax; ny++)
				{
					for (int nx = nxmin; nx < nxmax; nx++)
					{
						float valdim =
								(m_Bits[dimnr][j] - m_Centers[classnr][dimnr]) *
										(m_Bits[dimnr][j] - m_Centers[classnr][dimnr]) +
								factor1 * (nx * nx + ny * ny);
						if (valdim < minvaldim)
							minvaldim = valdim;
						j++;
					}
					j += m_Width + nxmin - nxmax;
				}
				val += minvaldim * m_Weights[dimnr] * m_Weights[dimnr] /
							 (m_TolF[dimnr] * m_TolF[dimnr]);
			}
			if (val < m_Minvals[i])
			{
				m_Minvals[i] = val;
				m_M[i] = classnr;
			}
		}
	}
}

} // namespace iseg
