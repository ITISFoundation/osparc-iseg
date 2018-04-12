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

namespace iseg {

class ISEG_CORE_API RTDoseReader
{
public:
	RTDoseReader();
	~RTDoseReader();

	static bool ReadSizeData(const char* filename, unsigned int* dims,
							 double* spacing, double* origin, double* dc);
	static bool ReadPixelData(const char* filename, float** bits);
};

} // namespace iseg
