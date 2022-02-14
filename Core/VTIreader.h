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

#include "Data/Types.h"

#include <string>
#include <vector>

namespace iseg {

class VTIreader
{
public:
	static bool GetInfo(const std::string& filename, unsigned& width, unsigned& height, unsigned& nrslices, float* pixelsize, float* offset, std::vector<std::string>& arrayNames);
	static bool GetSlice(const std::string& filename, float* slice, unsigned slicenr, unsigned width, unsigned height);
	static float* GetSliceInfo(const std::string& filename, unsigned slicenr, unsigned& width, unsigned& height);
	static bool GetVolume(const std::string& filename, float** slices, unsigned nrslices, unsigned width, unsigned height, const std::string& arrayName);
	static bool GetVolume(const std::string& filename, float** slices, unsigned startslice, unsigned nrslices, unsigned width, unsigned height, const std::string& arrayName);
	static bool GetVolumeAll(const std::string& filename, float** slicesbmp, float** sliceswork, tissues_size_t** slicestissue, unsigned nrslices, unsigned width, unsigned height);
};

class VTIwriter
{
public:
	static bool WriteVolumeAll(const std::string& filename, float** slicesbmp, float** sliceswork, tissues_size_t** slicestissue, tissues_size_t nrtissues, unsigned nrslices, unsigned width, unsigned height, float* pixelsize, float* offset, bool binary, bool compress);
};

} // namespace iseg
