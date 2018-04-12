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

namespace iseg { namespace fillcontours {

ISEG_CORE_API bool pointinpoly(float* pt, unsigned int cnt, float* polypts);
ISEG_CORE_API int whichquad(float* pt, float* orig);
ISEG_CORE_API bool is_hole(float** points, unsigned int* nrpoints,
						  unsigned int nrcontours, unsigned int contournr);
ISEG_CORE_API void fill_contour(bool* array, unsigned short* pixel_extents,
							   float* origin, float* pixel_size,
							   float* direction_cosines, float** points,
							   unsigned int* nrpoints, unsigned int nrcontours,
							   bool clockwisefill);

}} // namespace iseg::fillcontours
