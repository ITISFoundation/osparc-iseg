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

#include "Types.h"

#include <string>
#include <vector>

class VTIreader
{
public:
	static bool getInfo(const char *filename, unsigned &width, unsigned &height, unsigned &nrslices, float *pixelsize, float *offset, std::vector<std::string>& arrayNames);
	static bool getSlice(const char *filename, float *slice, unsigned slicenr, unsigned width, unsigned height);
	static float *getSliceInfo(const char *filename, unsigned slicenr, unsigned &width, unsigned &height);
	static bool getVolume(const char *filename, float **slices, unsigned nrslices, unsigned width, unsigned height, const std::string& arrayName);
	static bool getVolume(const char *filename, float **slices, unsigned startslice, unsigned nrslices, unsigned width, unsigned height, const std::string& arrayName);
	static bool getVolumeAll(const char *filename, float **slicesbmp, float **sliceswork, tissues_size_t **slicestissue, unsigned nrslices, unsigned width, unsigned height);
};

class VTIwriter
{
public:
	static bool writeVolumeAll(const char *filename, float **slicesbmp,float **sliceswork,tissues_size_t **slicestissue, tissues_size_t nrtissues, unsigned nrslices, unsigned width, unsigned height,float *pixelsize,float *offset,bool binary,bool compress);
};
