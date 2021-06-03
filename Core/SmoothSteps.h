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

#include "Data/Types.h"

namespace iseg {

class ISEG_CORE_API SmoothSteps
{
public:
	SmoothSteps();
	~SmoothSteps();
	void Dostepsmooth(tissues_size_t* line);
	void Init(float* mask1, unsigned short masklength1, unsigned short linelength1, tissues_size_t nrtissues1);
	void Init(unsigned short masklength1, unsigned short linelength1, tissues_size_t nrtissues1);

private:
	float* m_Mask;
	unsigned short m_Masklength;
	unsigned short m_Linelength;
	tissues_size_t m_Nrtissues;
	float** m_Weights;
	void GenerateBinommask();
	bool m_Ownmask;
};

} // namespace iseg
