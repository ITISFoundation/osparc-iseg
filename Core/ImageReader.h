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

#include <string>
#include <vector>

namespace iseg {

class Transform;

/** \brief Image reader based on ITK image reader factory
	*/
class ISEG_CORE_API ImageReader
{
public:
	/// get image size, spacing and transform
	static bool getInfo(const char* filename, unsigned& width, unsigned& height,
						unsigned& nrslices, float spacing[3],
						Transform& transform);

	/// loads image into pre-allocated memory
	static bool getSlice(const char* filename, float* slice, unsigned slicenr,
						 unsigned width, unsigned height);

	/// \note allocates memory for slice using new
	static float* getSliceInfo(const char* filename, unsigned slicenr,
							   unsigned& width, unsigned& height);

	/// loads image into pre-allocated memory
	static bool getVolume(const char* filename, float** slices,
						  unsigned nrslices, unsigned width, unsigned height);
	static bool getVolume(const char* filename, float** slices,
						  unsigned startslice, unsigned nrslices,
						  unsigned width, unsigned height);
};

class ImageSeriesReader
{
public:
	/// get image size, spacing and transform
	static bool getInfo(const char* filename, unsigned& width, unsigned& height,
						unsigned& nrslices, float spacing[3],
						Transform& transform);

	/// loads image into pre-allocated memory
	static bool getSlice(const char* filename, float* slice, unsigned slicenr,
						 unsigned width, unsigned height);

	/// \note allocates memory for slice using new
	static float* getSliceInfo(const char* filename, unsigned slicenr,
							   unsigned& width, unsigned& height);

	/// loads image into pre-allocated memory
	static bool getVolume(const char* filename, float** slices,
						  unsigned nrslices, unsigned width, unsigned height);
	static bool getVolume(const char* filename, float** slices,
						  unsigned startslice, unsigned nrslices,
						  unsigned width, unsigned height);
};
} // namespace iseg
