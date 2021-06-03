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

#include <functional>
#include <string>
#include <vector>

namespace iseg {

class Transform;

/** \brief Image reader based on ITK image reader factory
	*/
class ISEG_CORE_API ImageReader
{
public:
	static bool GetInfo2D(const char* filename, unsigned& width, unsigned& height);

	/// loads 2D image into pre-allocated memory
	static bool GetImageStack(const std::vector<const char*>& filenames, float** img_stack, unsigned width, unsigned height, const std::function<float(unsigned char, unsigned char, unsigned char)>& color2grey);

	/// get image size, spacing and transform
	static bool GetInfo(const char* filename, unsigned& width, unsigned& height, unsigned& nrslices, float spacing[3], Transform& transform);

	/// loads image into pre-allocated memory
	static bool GetSlice(const char* filename, float* slice, unsigned slicenr, unsigned width, unsigned height);

	/// \note allocates memory for slice using new
	static float* GetSliceInfo(const char* filename, unsigned slicenr, unsigned& width, unsigned& height);

	/// loads image into pre-allocated memory
	static bool GetVolume(const char* filename, float** slices, unsigned nrslices, unsigned width, unsigned height);
	static bool GetVolume(const char* filename, float** slices, unsigned startslice, unsigned nrslices, unsigned width, unsigned height);
};

} // namespace iseg
