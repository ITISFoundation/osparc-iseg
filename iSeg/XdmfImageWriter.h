/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef XDMFIMAGEWRITER_H
#define XDMFIMAGEWRITER_H

#include "Data/Transform.h"
#include "Data/Types.h"

#include "Core/SetGetMacros.h"

namespace iseg {

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
	char* m_FileName;
	unsigned m_NumberOfSlices;
	unsigned m_Width;
	unsigned m_Height;
	int m_Compression;
	float* m_PixelSize;
	Transform m_ImageTransform;
	float** m_ImageSlices;
	float** m_WorkSlices;
	tissues_size_t** m_TissueSlices;
	bool m_CopyToContiguousMemory;

private:
	int InternalWrite(const char* filename, float** slicesbmp, float** sliceswork, tissues_size_t** slicestissue, unsigned nrslices, unsigned width, unsigned height, float* pixelsize, Transform& transform, int compression, bool naked);
};

} // namespace iseg

#endif // XDMFIMAGEWRITER_H
