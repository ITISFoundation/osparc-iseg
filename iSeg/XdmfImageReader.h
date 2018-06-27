/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef XDMFIMAGEREADER_H
#define XDMFIMAGEREADER_H

#include "Data/Transform.h"
#include "Data/Types.h"

#include "Core/SetGetMacros.h"

#include <QMap>
#include <QStringList>

#include <memory>

namespace iseg {

class ColorLookupTable;

class XdmfImageReader
{
public:
	XdmfImageReader();
	~XdmfImageReader();
	SetStringMacro(FileName);
	GetStringMacro(FileName);
	GetMacro(NumberOfSlices, unsigned);
	GetMacro(Width, unsigned);
	GetMacro(Height, unsigned);
	GetMacro(Compression, int);
	GetMacro(PixelSize, float*);
	GetMacro(ImageTransform, Transform);
	SetMacro(ImageSlices, float**);
	GetMacro(ImageSlices, float**);
	SetMacro(WorkSlices, float**);
	GetMacro(WorkSlices, float**);
	SetMacro(TissueSlices, tissues_size_t**);
	GetMacro(TissueSlices, tissues_size_t**);
	SetMacro(ReadContiguousMemory, bool);
	GetMacro(ReadContiguousMemory, bool);
	QStringList GetArrayNames() const { return this->ArrayNames; };
	QMap<QString, QString> GetMapArrayNames() const
	{
		return this->mapArrayNames;
	}
	int ParseXML();
	int Read();

	std::shared_ptr<ColorLookupTable> ReadColorLookup() const;

private:
	char* FileName;
	bool ReadContiguousMemory;
	unsigned NumberOfSlices;
	unsigned Width;
	unsigned Height;
	int Compression;
	float PixelSize[3];
	Transform ImageTransform;
	float** ImageSlices;
	float** WorkSlices;
	tissues_size_t** TissueSlices;
	QStringList ArrayNames;
	QMap<QString, QString> mapArrayNames;
};

class HDFImageReader
{
public:
	HDFImageReader();
	~HDFImageReader();
	SetStringMacro(FileName);
	GetStringMacro(FileName);
	GetMacro(NumberOfSlices, unsigned);
	GetMacro(Width, unsigned);
	GetMacro(Height, unsigned);
	GetMacro(Compression, int);
	GetMacro(PixelSize, float*);
	SetMacro(ReadContiguousMemory, bool);
	GetMacro(ReadContiguousMemory, bool);
	/// returns array as if it were a float[16] array
	/// transform is stored in row-major, i.e. column fastest
	float* GetImageTransform() { return ImageTransform[0]; }
	SetMacro(ImageSlices, float**);
	GetMacro(ImageSlices, float**);
	SetMacro(WorkSlices, float**);
	GetMacro(WorkSlices, float**);
	SetMacro(TissueSlices, tissues_size_t**);
	GetMacro(TissueSlices, tissues_size_t**);
	QStringList GetArrayNames() const { return this->ArrayNames; };
	QMap<QString, QString> GetMapArrayNames() const
	{
		return this->mapArrayNames;
	};
	int ParseHDF();
	int Read();

	std::shared_ptr<ColorLookupTable> ReadColorLookup() const;

private:
	char* FileName;
	bool ReadContiguousMemory;
	unsigned NumberOfSlices;
	unsigned Width;
	unsigned Height;
	int Compression;
	float PixelSize[3];
	Transform ImageTransform;
	float** ImageSlices;
	float** WorkSlices;
	tissues_size_t** TissueSlices;
	QStringList ArrayNames;
	QMap<QString, QString> mapArrayNames;
};

} // namespace iseg

#endif // XDMFIMAGEREADER_H
