/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	void dostepsmooth(tissues_size_t* line);
	void init(float* mask1, unsigned short masklength1,
			unsigned short linelength1, tissues_size_t nrtissues1);
	void init(unsigned short masklength1, unsigned short linelength1,
			tissues_size_t nrtissues1);

private:
	float* mask;
	unsigned short masklength;
	unsigned short linelength;
	tissues_size_t nrtissues;
	float** weights;
	void generate_binommask();
	bool ownmask;
};

} // namespace iseg
