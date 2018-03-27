/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef NIFTIIMAGEREADER_H
#define NIFTIIMAGEREADER_H

#include "common.h"
#include <QMap>
#include <QStringList>
#include <SetGetMacros.h>

//class QStringList;

class NiftiImageReader
{
public:
	NiftiImageReader();
	~NiftiImageReader();
	SetStringMacro(FileName);
	GetStringMacro(FileName);
	GetMacro(ReadSliceStart, unsigned);
	GetMacro(ReadSliceCount, unsigned);
	GetMacro(NumberOfSlices, unsigned);
	GetMacro(Width, unsigned);
	GetMacro(Height, unsigned);
	GetMacro(Compression, int);
	GetMacro(PixelSize, float *);
	GetMacro(Offset, float *);
	GetMacro(Quaternion, float *);
	SetMacro(ImageSlices, float **);
	GetMacro(ImageSlices, float **);
	SetMacro(WorkSlices, float **);
	GetMacro(WorkSlices, float **);
	SetMacro(TissueSlices, tissues_size_t **);
	GetMacro(TissueSlices, tissues_size_t **);
	QStringList GetArrayNames() const { return this->ArrayNames; };
	QMap<QString, QString> GetMapArrayNames() const
	{
		return this->mapArrayNames;
	};
	int Read();
	int Read(unsigned short startslice, unsigned short nrslices);
	int ReadHeader();

private:
	template<typename InType, typename OutType>
	void ReadData(const InType *in, OutType **out);

	void ReadDataRGB(const unsigned char *in, float **out);

private:
	bool HeaderNotRead;
	char *FileName;
	unsigned ReadSliceStart;
	unsigned ReadSliceCount;
	unsigned NumberOfSlices;
	unsigned Width;
	unsigned Height;
	int Compression;
	float *PixelSize;
	float *Offset;
	float *Quaternion;
	float **ImageSlices;
	float **WorkSlices;
	tissues_size_t **TissueSlices;
	QStringList ArrayNames;
	QMap<QString, QString> mapArrayNames;
};

#endif // NIFTIIMAGEREADER_H
