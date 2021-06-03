/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef XDMFIMAGEMERGER_H
#define XDMFIMAGEMERGER_H

#include "XdmfImageReader.h"

#include "Data/Transform.h"
#include "Data/Types.h"

#include "Core/SetGetMacros.h"

namespace iseg {

class XdmfImageMerger
{
public:
	XdmfImageMerger();
	~XdmfImageMerger();
	SetStringMacro(FileName);
	GetStringMacro(FileName);
	SetMacro(NumberOfSlices, unsigned);
	GetMacro(NumberOfSlices, unsigned);
	SetMacro(TotalNumberOfSlices, unsigned);
	GetMacro(TotalNumberOfSlices, unsigned);
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
	SetMacro(MergeFileNames, std::vector<QString>);
	GetMacro(MergeFileNames, std::vector<QString>);
	int Write();

protected:
	char* m_FileName;
	unsigned m_NumberOfSlices;
	unsigned m_TotalNumberOfSlices;
	unsigned m_Width;
	unsigned m_Height;
	int m_Compression;
	float* m_PixelSize;
	Transform m_ImageTransform;
	float** m_ImageSlices;
	float** m_WorkSlices;
	tissues_size_t** m_TissueSlices;
	std::vector<QString> m_MergeFileNames;

private:
	int InternalWrite(const char* filename, std::vector<QString>& mergefilenames, float** slicesbmp, float** sliceswork, tissues_size_t** slicestissue, unsigned nrslices, unsigned nrslicesTotal, unsigned width, unsigned height, float* pixelsize, const Transform& transform, int compression);
	int ReadSource(XdmfImageReader* imageReader, const char* filename, std::vector<float>& bufferFloat, size_t& sliceoffset) const;
	int ReadTarget(XdmfImageReader* imageReader, const char* filename, std::vector<float>& bufferFloat, size_t& sliceoffset) const;
	int ReadTissues(XdmfImageReader* imageReader, const char* filename, std::vector<tissues_size_t>& bufferTissuesSizeT, size_t& sliceoffset) const;
};

} // namespace iseg

#endif // XDMFIMAGEMERGER_H
