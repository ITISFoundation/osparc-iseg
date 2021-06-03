/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

	const QStringList& GetArrayNames() const { return this->m_ArrayNames; };
	const QMap<QString, QString>& GetMapArrayNames() const { return this->m_MapArrayNames; }

	int ParseXML();
	int Read();

	std::shared_ptr<ColorLookupTable> ReadColorLookup() const;

private:
	char* m_FileName;
	bool m_ReadContiguousMemory;
	unsigned m_NumberOfSlices;
	unsigned m_Width;
	unsigned m_Height;
	int m_Compression;
	float m_PixelSize[3];
	Transform m_ImageTransform;
	float** m_ImageSlices;
	float** m_WorkSlices;
	tissues_size_t** m_TissueSlices;
	QStringList m_ArrayNames;
	QMap<QString, QString> m_MapArrayNames;
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
	float* GetImageTransform() { return m_ImageTransform[0]; }
	SetMacro(ImageSlices, float**);
	GetMacro(ImageSlices, float**);
	SetMacro(WorkSlices, float**);
	GetMacro(WorkSlices, float**);
	SetMacro(TissueSlices, tissues_size_t**);
	GetMacro(TissueSlices, tissues_size_t**);
	QStringList GetArrayNames() const { return this->m_ArrayNames; };
	QMap<QString, QString> GetMapArrayNames() const
	{
		return this->m_MapArrayNames;
	};
	int ParseHDF();
	int Read();

	std::shared_ptr<ColorLookupTable> ReadColorLookup() const;

private:
	char* m_FileName;
	bool m_ReadContiguousMemory;
	unsigned m_NumberOfSlices;
	unsigned m_Width;
	unsigned m_Height;
	int m_Compression;
	float m_PixelSize[3];
	Transform m_ImageTransform;
	float** m_ImageSlices;
	float** m_WorkSlices;
	tissues_size_t** m_TissueSlices;
	QStringList m_ArrayNames;
	QMap<QString, QString> m_MapArrayNames;
};

} // namespace iseg

#endif // XDMFIMAGEREADER_H
