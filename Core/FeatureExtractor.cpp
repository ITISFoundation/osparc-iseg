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

#include "FeatureExtractor.h"

#include <algorithm>

namespace iseg {

void FeatureExtractor::Init(float* bit, Point p1, Point p2, short unsigned w, short unsigned h)
{
	m_Width = w;
	m_Height = h;
	SetBits(bit);
	SetWindow(p1, p2);
}

void FeatureExtractor::SetBits(float* bit)
{
	m_Bits = bit;
}

void FeatureExtractor::SetWindow(Point p1, Point p2)
{
	m_Xmin = std::min(p1.px, p2.px);
	m_Xmax = std::max(p1.px, p2.px);
	m_Ymin = std::min(p1.py, p2.py);
	m_Ymax = std::max(p1.py, p2.py);
	m_Area = unsigned(m_Ymax - m_Ymin + 1) * (m_Xmax - m_Xmin + 1);
	m_Valmin = m_Valmax = m_Bits[unsigned(m_Ymin) * m_Width + m_Xmin];

	m_Average = 0;
	for (short i = m_Xmin; i <= m_Xmax; i++)
	{
		for (short j = m_Ymin; j <= m_Ymax; j++)
		{
			m_Average += m_Bits[unsigned(j) * m_Width + i];
			m_Valmin = std::min(m_Valmin, m_Bits[unsigned(j) * m_Width + i]);
			m_Valmax = std::max(m_Valmax, m_Bits[unsigned(j) * m_Width + i]);
		}
	}

	m_Average /= m_Area;
}

float FeatureExtractor::ReturnAverage() const { return m_Average; }

float FeatureExtractor::ReturnStddev()
{
	if (m_Area != 1)
	{
		float stddev = 0;
		for (short i = m_Xmin; i <= m_Xmax; i++)
		{
			for (short j = m_Ymin; j <= m_Ymax; j++)
			{
				stddev += (m_Bits[unsigned(j) * m_Width + i] - m_Average) *
									(m_Bits[unsigned(j) * m_Width + i] - m_Average);
			}
		}
		return sqrt(stddev / (m_Area - 1));
	}
	else
		return 1E10;
}

void FeatureExtractor::ReturnExtrema(Pair* p) const
{
	p->high = m_Valmax;
	p->low = m_Valmin;
}

class FeatureExtractorMask
{
public:
	void Init(float* bit, float* mask1, short unsigned w, short unsigned h, float f1);
	void SetBits(float* bit);
	void SetMask(float* mask1);
	void SetF(float f1);
	float ReturnAverage() const;
	float ReturnStddev();
	void ReturnExtrema(Pair* p) const;

private:
	float m_F;
	float* m_Bits;
	float m_Valmin;
	float m_Valmax;
	float m_Average;
	short unsigned m_Width;
	short unsigned m_Height;
	unsigned m_Area;
	float* m_Mask;
};

void FeatureExtractorMask::Init(float* bit, float* mask1, short unsigned w, short unsigned h, float f1)
{
	m_Width = w;
	m_Height = h;
	m_F = f1;
	SetBits(bit);
	SetMask(mask1);
}

void FeatureExtractorMask::SetBits(float* bit)
{
	m_Bits = bit;
}

void FeatureExtractorMask::SetMask(float* mask1)
{
	m_Area = 0;
	unsigned pos = 0;
	m_Mask = mask1;
	while (m_Mask[pos] == 0 && pos < unsigned(m_Width) * m_Height)
		pos++;
	if (pos < unsigned(m_Width) * m_Height)
	{
		m_Valmin = m_Valmax = m_Bits[pos];
		m_Average = 0;
		for (; pos < unsigned(m_Width) * m_Height; pos++)
		{
			if (m_Mask[pos] != 0)
			{
				m_Area++;
				m_Average += m_Bits[pos];
				m_Valmin = std::min(m_Valmin, m_Bits[pos]);
				m_Valmax = std::max(m_Valmax, m_Bits[pos]);
			}
		}

		m_Average /= m_Area;
	}
	else
	{
		m_Average = 0;
		m_Area = 0;
	}
}

void FeatureExtractorMask::SetF(float f1)
{
	m_F = f1;
	SetMask(m_Mask);
}

float FeatureExtractorMask::ReturnAverage() const { return m_Average; }

float FeatureExtractorMask::ReturnStddev()
{
	if (m_Area > 1)
	{
		float stddev = 0;
		for (unsigned pos = 0; pos < unsigned(m_Width) * m_Height; pos++)
		{
			if (m_Mask[pos] != 0)
			{
				stddev += (m_Bits[pos] - m_Average) * (m_Bits[pos] - m_Average);
			}
		}

		return sqrt(stddev / (m_Area - 1));
	}
	else
		return 1E10;
}

void FeatureExtractorMask::ReturnExtrema(Pair* p) const
{
	p->high = m_Valmax;
	p->low = m_Valmin;
}

} // namespace iseg
