/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
#include "Core/SetGetMacros.h"
#include "Core/Types.h"

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
	SetMacro(Offset, float*);
	GetMacro(Offset, float*);
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
	char* FileName;
	unsigned NumberOfSlices;
	unsigned TotalNumberOfSlices;
	unsigned Width;
	unsigned Height;
	int Compression;
	float* PixelSize;
	float* Offset;
	float** ImageSlices;
	float** WorkSlices;
	tissues_size_t** TissueSlices;
	std::vector<QString> MergeFileNames;

private:
	int InternalWrite(const char *filename, std::vector<QString> &mergefilenames, float **slicesbmp,float **sliceswork, tissues_size_t **slicestissue, unsigned nrslices, unsigned nrslicesTotal, unsigned width, unsigned height, float *pixelsize, float *offset, int compression);
	int ReadSource(XdmfImageReader *imageReader, const char *filename, std::vector<float> &bufferFloat, size_t &sliceoffset);
	int ReadTarget(XdmfImageReader *imageReader, const char *filename, std::vector<float> &bufferFloat, size_t &sliceoffset);
	int ReadTissues(XdmfImageReader *imageReader, const char *filename, std::vector<tissues_size_t> &bufferTissuesSizeT, size_t &sliceoffset);
};

#endif // XDMFIMAGEMERGER_H
