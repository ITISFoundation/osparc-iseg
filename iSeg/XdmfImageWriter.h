/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef XDMFIMAGEWRITER_H
#define XDMFIMAGEWRITER_H

#include "Core/Types.h"
#include "Core/SetGetMacros.h"
#include "Core/Transform.h"

class ColorLookupTable;


class XdmfImageWriter
{
public:
	XdmfImageWriter();
	XdmfImageWriter(const char* filepath);
	~XdmfImageWriter();
	SetStringMacro(FileName);
	GetStringMacro(FileName);
	SetMacro(NumberOfSlices, unsigned);
	GetMacro(NumberOfSlices, unsigned);
	SetMacro(Width, unsigned);
	GetMacro(Width, unsigned);
	SetMacro(Height, unsigned);
	GetMacro(Height, unsigned);
	SetMacro(Compression, int);
	GetMacro(Compression, int);
	SetMacro(PixelSize, float*);
	GetMacro(PixelSize, float*);
	SetMacro(ImageTransform, Transform);
	GetMacro(ImageTransform, Transform);
	SetMacro(ImageSlices, float**);
	GetMacro(ImageSlices, float**);
	SetMacro(WorkSlices, float**);
	GetMacro(WorkSlices, float**);
	SetMacro(TissueSlices, tissues_size_t**);
	GetMacro(TissueSlices, tissues_size_t**);
	SetMacro(CopyToContiguousMemory, bool);
	GetMacro(CopyToContiguousMemory, bool);
	bool Write(bool naked = false);

	bool WriteColorLookup(const ColorLookupTable* lut, bool naked = false);

protected:
	char* FileName;
	unsigned NumberOfSlices;
	unsigned Width;
	unsigned Height;
	int Compression;
	float* PixelSize;
	Transform ImageTransform;
	float** ImageSlices;
	float** WorkSlices;
	tissues_size_t** TissueSlices;
	bool CopyToContiguousMemory;

private:
	int InternalWrite(const char *filename, float **slicesbmp, float **sliceswork, tissues_size_t **slicestissue, unsigned nrslices, unsigned width, unsigned height, float *pixelsize, Transform& transform, int compression, bool naked);
};

#endif // XDMFIMAGEWRITER_H
