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

#include "Pair.h"

#include "Data/Point.h"

namespace iseg {

class ISEG_CORE_API FeatureExtractor
{
public:
	void Init(float* bit, Point p1, Point p2, short unsigned w, short unsigned h);
	void SetBits(float* bit);
	void SetWindow(Point p1, Point p2);
	float ReturnAverage() const;
	float ReturnStddev();
	void ReturnExtrema(Pair* p) const;

private:
	float* m_Bits;
	short m_Xmin;
	short m_Xmax;
	short m_Ymin;
	short m_Ymax;
	float m_Valmin;
	float m_Valmax;
	float m_Average;
	short unsigned m_Width;
	short unsigned m_Height;
	unsigned m_Area;
};
} // namespace iseg
