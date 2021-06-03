/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "bmp_read_1.h"
#include "config.h"

#include "Data/addLine.h"

#include "Core/ExpectationMaximization.h"
#include "Core/ImageForestingTransform.h"
#include "Core/ImageReader.h"
#include "Core/KMeans.h"
#include "Core/MultidimensionalGamma.h"
#include "Core/SliceProvider.h"

#define cimg_display 0
#include "AvwReader.h"
#include "CImg.h"
#include "ChannelExtractor.h"
#include "DicomReader.h"
#include "Levelset.h"
#include "TissueInfos.h"

#include <vtkBMPWriter.h>
#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <QColor>
#include <QImage>
#include <QMessageBox>

#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <list>
#include <queue>
#include <stack>
#include <vector>

namespace iseg {

#ifndef M_PI
#	define M_PI 3.14159
#endif

#ifndef ID_BMP
#	define ID_BMP 0x4D42 /* "BM" */
#endif
#ifndef WIN32
struct BITMAPFILEHEADER /**** BMP file header structure ****/
{
	unsigned short bfType;			/* Magic number for file */
	unsigned int bfSize;				/* Size of file */
	unsigned short bfReserved1; /* Reserved */
	unsigned short bfReserved2; /* ... */
	unsigned int bfOffBits;			/* Offset to bitmap data */
};

#

struct BITMAPINFOHEADER /**** BMP file info structure ****/
{
	unsigned int biSize;				 /* Size of info header */
	int biWidth;								 /* Width of image */
	int biHeight;								 /* Height of image */
	unsigned short biPlanes;		 /* Number of color planes */
	unsigned short biBitCount;	 /* Number of bits per pixel */
	unsigned int biCompression;	 /* Type of compression to use */
	unsigned int biSizeImage;		 /* Size of image data */
	int biXPelsPerMeter;				 /* X pixels per meter */
	int biYPelsPerMeter;				 /* Y pixels per meter */
	unsigned int biClrUsed;			 /* Number of colors used */
	unsigned int biClrImportant; /* Number of important colors */
};

#	define BI_RGB 0			 /* No compression - straight BGR data */
#	define BI_RLE8 1			 /* 8-bit run-length compression */
#	define BI_RLE4 2			 /* 4-bit run-length compression */
#	define BI_BITFIELDS 3 /* RGB bitmap with RGB masks */

struct RGBQUAD /**** Colormap entry structure ****/
{
	unsigned char m_RgbBlue;		 /* Blue value */
	unsigned char m_RgbGreen;		 /* Green value */
	unsigned char m_RgbRed;			 /* Red value */
	unsigned char m_RgbReserved; /* Reserved */
};

struct BITMAPINFO /**** Bitmap information structure ****/
{
	BITMAPINFOHEADER bmiHeader; /* Image header */
	RGBQUAD bmiColors[256];			/* Image colormap */
};
#endif /* !WIN32 */

template<typename T>
inline void swap_maps(T const*& Tp1, T const*& Tp2)
{
	T const* dummy;
	dummy = Tp1;
	Tp1 = Tp2;
	Tp2 = dummy;
}

std::list<unsigned> Bmphandler::stackindex;
unsigned Bmphandler::stackcounter;
std::list<float*> Bmphandler::bits_stack;
std::list<unsigned char> Bmphandler::mode_stack;
//bool Bmphandler::lockedtissues[TISSUES_SIZE_MAX+1];

Bmphandler::Bmphandler()
{
	m_Area = 0;
	m_Loaded = false;
	m_Ownsliceprovider = false;
	m_SliceprovideInstaller = SliceProviderInstaller::Getinst();
	stackcounter = 1;
	m_Mode1 = m_Mode2 = 1;
}

Bmphandler::Bmphandler(const Bmphandler&)
{
	m_Area = 0;
	m_Loaded = false;
	m_Ownsliceprovider = false;
	m_SliceprovideInstaller = SliceProviderInstaller::Getinst();
	stackcounter = 1;
	m_Mode1 = m_Mode2 = 1;

	m_RedFactor = 0.299;
	m_GreenFactor = 0.587;
	m_BlueFactor = 0.114;
}

Bmphandler::~Bmphandler()
{
	if (m_Loaded)
	{
		ClearStack();
		m_Sliceprovide->TakeBack(m_BmpBits);
		m_Sliceprovide->TakeBack(m_WorkBits);
		m_Sliceprovide->TakeBack(m_HelpBits);
		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			free(m_Tissuelayers[idx]);
		}
		m_Tissuelayers.clear();
		//		if(ownsliceprovider)
		m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		//		free(bmpinfo);
	}
	m_SliceprovideInstaller->ReturnInstance();
	if (m_SliceprovideInstaller->Unused())
		delete m_SliceprovideInstaller;
}

void Bmphandler::ClearStack()
{
	for (auto& b : bits_stack)
		m_Sliceprovide->TakeBack(b);
	bits_stack.clear();
	stackindex.clear();
	mode_stack.clear();
	stackcounter = 1;
}

float* Bmphandler::ReturnBmp() { return m_BmpBits; }

const float* Bmphandler::ReturnBmp() const { return m_BmpBits; }

float* Bmphandler::ReturnWork() { return m_WorkBits; }

const float* Bmphandler::ReturnWork() const { return m_WorkBits; }

tissues_size_t* Bmphandler::ReturnTissues(tissuelayers_size_t idx)
{
	if (idx < m_Tissuelayers.size())
		return m_Tissuelayers[idx];
	else
		return nullptr;
}

const tissues_size_t* Bmphandler::ReturnTissues(tissuelayers_size_t idx) const
{
	if (idx < m_Tissuelayers.size())
		return m_Tissuelayers[idx];
	else
		return nullptr;
}

float* Bmphandler::ReturnHelp() { return m_HelpBits; }

float** Bmphandler::ReturnBmpfield() { return &m_BmpBits; }

float** Bmphandler::ReturnWorkfield() { return &m_WorkBits; }

tissues_size_t** Bmphandler::ReturnTissuefield(tissuelayers_size_t idx)
{
	return &m_Tissuelayers[idx];
}

std::vector<Mark>* Bmphandler::ReturnMarks() { return &m_Marks; }

void Bmphandler::Copy2marks(std::vector<Mark>* marks1) { m_Marks = *marks1; }

void Bmphandler::GetLabels(std::vector<Mark>* labels)
{
	labels->clear();
	GetAddLabels(labels);
}

void Bmphandler::GetAddLabels(std::vector<Mark>* labels)
{
	for (size_t i = 0; i < m_Marks.size(); i++)
	{
		if (m_Marks[i].name != std::string(""))
			labels->push_back(m_Marks[i]);
	}
}

void Bmphandler::SetBmp(float* bits, unsigned char mode)
{
	if (m_Loaded)
	{
		if (m_BmpBits != bits)
		{
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_BmpBits = bits;
			m_Mode1 = mode;
		}
	}
}
void Bmphandler::SetWork(float* bits, unsigned char mode)
{
	if (m_Loaded)
	{
		if (m_WorkBits != bits)
		{
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_WorkBits = bits;
			m_Mode2 = mode;
		}
	}
}

void Bmphandler::SetTissue(tissuelayers_size_t idx, tissues_size_t* bits)
{
	if (m_Loaded)
	{
		if (m_Tissuelayers[idx] != bits)
		{
			free(m_Tissuelayers[idx]);
			m_Tissuelayers[idx] = bits;
		}
	}
}

float* Bmphandler::SwapBmpPointer(float* bits)
{
	float* tmp = m_BmpBits;
	m_BmpBits = bits;
	return tmp;
}

float* Bmphandler::SwapWorkPointer(float* bits)
{
	float* tmp = m_WorkBits;
	m_WorkBits = bits;
	return tmp;
}

tissues_size_t* Bmphandler::SwapTissuesPointer(tissuelayers_size_t idx, tissues_size_t* bits)
{
	tissues_size_t* tmp = m_Tissuelayers[idx];
	m_Tissuelayers[idx] = bits;
	return tmp;
}

void Bmphandler::Copy2bmp(float* bits, unsigned char mode)
{
	if (m_Loaded)
	{
		for (unsigned i = 0; i < m_Area; i++)
			m_BmpBits[i] = bits[i];
		m_Mode1 = mode;
	}
}

void Bmphandler::Copy2work(float* bits, unsigned char mode)
{
	if (m_Loaded)
	{
		for (unsigned i = 0; i < m_Area; i++)
			m_WorkBits[i] = bits[i];
		m_Mode2 = mode;
	}
}

void Bmphandler::Copy2work(float* bits, bool* mask, unsigned char mode)
{
	if (m_Loaded)
	{
		for (unsigned i = 0; i < m_Area; i++)
		{
			if (mask[i])
				m_WorkBits[i] = bits[i];
		}
		if (mode == 1)
			m_Mode2 = 1;
	}
}

void Bmphandler::Copy2tissue(tissuelayers_size_t idx, tissues_size_t* bits, bool* mask)
{
	if (m_Loaded)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned i = 0; i < m_Area; i++)
		{
			if (mask[i] && (!TissueInfos::GetTissueLocked(tissues[i])))
				tissues[i] = bits[i];
		}
	}
}

void Bmphandler::Copy2tissue(tissuelayers_size_t idx, tissues_size_t* bits)
{
	if (m_Loaded)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned i = 0; i < m_Area; i++)
			tissues[i] = bits[i];
	}
}

void Bmphandler::Copyfrombmp(float* bits)
{
	if (m_Loaded)
	{
		for (unsigned i = 0; i < m_Area; i++)
			bits[i] = m_BmpBits[i];
	}
}

void Bmphandler::Copyfromwork(float* bits)
{
	if (m_Loaded)
	{
		for (unsigned i = 0; i < m_Area; i++)
			bits[i] = m_WorkBits[i];
	}
}

void Bmphandler::Copyfromtissue(tissuelayers_size_t idx, tissues_size_t* bits)
{
	if (m_Loaded)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned i = 0; i < m_Area; i++)
			bits[i] = tissues[i];
	}
}

#ifdef TISSUES_SIZE_TYPEDEF
void Bmphandler::Copyfromtissue(tissuelayers_size_t idx, unsigned char* bits)
{
	if (m_Loaded)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned i = 0; i < m_Area; i++)
			bits[i] = (unsigned char)tissues[i];
	}
}
#endif // TISSUES_SIZE_TYPEDEF

void Bmphandler::Copyfromtissuepadded(tissuelayers_size_t idx, tissues_size_t* bits, unsigned short padding)
{
	if (m_Loaded)
	{
		unsigned int pos1 = 0;
		unsigned int pos2 = 0;
		for (; pos1 < (unsigned int)(m_Width + 2 * padding) * padding + padding;
				 pos1++)
			bits[pos1] = 0;
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned short j = 0; j < m_Height; j++)
		{
			for (unsigned short i = 0; i < m_Width; i++, pos1++, pos2++)
			{
				bits[pos1] = tissues[pos2];
			}
			for (unsigned short i = 0; i < 2 * padding; i++, pos1++)
				bits[pos1] = 0;
		}
		unsigned int maxval = m_Area + 2 * padding * m_Width +
													2 * padding * m_Height + padding * padding;
		for (; pos1 < maxval; pos1++)
			bits[pos1] = 0;
	}
}

void Bmphandler::ClearBmp()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_BmpBits[i] = 0;

	delete m_BmpBits;
}

void Bmphandler::ClearWork()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = 0;
}

inline unsigned Bmphandler::Pt2coord(Point p) const
{
	return p.px + (p.py * (unsigned)m_Width);
}

void Bmphandler::BmpAbs()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = abs(m_WorkBits[i]);
}

void Bmphandler::BmpNeg()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = 255 - m_WorkBits[i];
}

void Bmphandler::BmpSum()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = m_WorkBits[i] + m_BmpBits[i];
}

void Bmphandler::BmpDiff()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = m_WorkBits[i] - m_BmpBits[i];
}

void Bmphandler::BmpAdd(float f)
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = m_WorkBits[i] + f;
}

void Bmphandler::BmpMult()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] *= m_BmpBits[i];
}

void Bmphandler::BmpMult(float f)
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = f * m_WorkBits[i];
}

void Bmphandler::BmpOverlay(float alpha)
{
	float tmp = 1.0f - alpha;
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = alpha * m_BmpBits[i] + tmp * m_WorkBits[i];
	m_Mode2 = 2;
}

void Bmphandler::TransparentAdd(float* pict2)
{
	for (unsigned int i = 0; i < m_Area; i++)
		if (m_WorkBits[i] == 0)
			m_WorkBits[i] = pict2[i];
}

float* Bmphandler::CopyWork()
{
	float* results = m_Sliceprovide->GiveMe();
	for (unsigned i = 0; i < m_Area; i++)
		results[i] = m_WorkBits[i];

	return results;
}

float* Bmphandler::CopyBmp()
{
	float* results = m_Sliceprovide->GiveMe();
	for (unsigned i = 0; i < m_Area; i++)
		results[i] = m_BmpBits[i];

	return results;
}

tissues_size_t* Bmphandler::CopyTissue(tissuelayers_size_t idx)
{
	tissues_size_t* results =
			(tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area);
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned i = 0; i < m_Area; i++)
		results[i] = tissues[i];

	return results;
}

void Bmphandler::CopyWork(float* output)
{
	for (unsigned i = 0; i < m_Area; i++)
		output[i] = m_WorkBits[i];
}

void Bmphandler::CopyBmp(float* output)
{
	for (unsigned i = 0; i < m_Area; i++)
		output[i] = m_BmpBits[i];
}

void Bmphandler::CopyTissue(tissuelayers_size_t idx, tissues_size_t* output)
{
	if (m_Tissuelayers.size() <= idx)
		return;

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned i = 0; i < m_Area; i++)
		output[i] = tissues[i];
}

void Bmphandler::Newbmp(unsigned short width1, unsigned short height1, bool init)
{
	unsigned areanew = unsigned(width1) * height1;
	m_Width = width1;
	m_Height = height1;
	if (m_Area != areanew)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}
		m_Area = areanew;
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);
		m_BmpBits = m_Sliceprovide->GiveMe();
		m_WorkBits = m_Sliceprovide->GiveMe();
		m_HelpBits = m_Sliceprovide->GiveMe();
		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}
	else
	{
		if (!m_Loaded)
		{
			m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);
			m_BmpBits = m_Sliceprovide->GiveMe();
			m_WorkBits = m_Sliceprovide->GiveMe();
			m_HelpBits = m_Sliceprovide->GiveMe();
			m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
			ClearTissue(0);
		}
	}

	tissues_size_t* tissues = m_Tissuelayers[0];

	if (init)
	{
		std::fill(m_BmpBits, m_BmpBits + m_Area, 0.f);
		std::fill(m_WorkBits, m_WorkBits + m_Area, 0.f);
		std::fill(m_HelpBits, m_HelpBits + m_Area, 0.f);
		std::fill(tissues, tissues + m_Area, 0);
	}

	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();
}

void Bmphandler::Newbmp(unsigned short width1, unsigned short height1, float* bits)
{
	unsigned areanew = unsigned(width1) * height1;
	m_Width = width1;
	m_Height = height1;
	if (m_Area != areanew)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}
		m_Area = areanew;
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);
		m_BmpBits = bits;
		m_WorkBits = m_Sliceprovide->GiveMe();
		m_HelpBits = m_Sliceprovide->GiveMe();
		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}
	else
	{
		if (!m_Loaded)
		{
			m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);
			m_BmpBits = bits;
			m_WorkBits = m_Sliceprovide->GiveMe();
			m_HelpBits = m_Sliceprovide->GiveMe();
			m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
			ClearTissue(0);
		}
	}

	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();
}

void Bmphandler::Freebmp()
{
	if (m_Loaded)
	{
		ClearStack();
		m_Sliceprovide->TakeBack(m_BmpBits);
		m_Sliceprovide->TakeBack(m_WorkBits);
		m_Sliceprovide->TakeBack(m_HelpBits);
		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			free(m_Tissuelayers[idx]);
		}
		m_Tissuelayers.clear();
		m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
	}

	m_Area = 0;
	m_Loaded = false;
	ClearMarks();
	ClearVvm();
	ClearLimits();
}

int Bmphandler::CheckBMPDepth(const char* filename)
{
	FILE* fp;
	BITMAPFILEHEADER header;

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	if (fread(&header, sizeof(BITMAPFILEHEADER), 1, fp) < 1)
	{
		fclose(fp);
		return 0;
	}

	if (header.bfType != ID_BMP)
	{
		fclose(fp);
		return 0;
	}

	int infosize = header.bfOffBits - sizeof(BITMAPFILEHEADER);

	BITMAPINFO* bmpinfo;
	if ((bmpinfo = (BITMAPINFO*)malloc(infosize)) == nullptr)
	{
		fclose(fp);
		return 0;
	}

	if (fread(bmpinfo, 1, infosize, fp) < (unsigned int)infosize)
	{
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

	return bmpinfo->bmiHeader.biBitCount;
}

void Bmphandler::SetConverterFactors(int newRedFactor, int newGreenFactor, int newBlueFactor)
{
	m_RedFactor = newRedFactor / 100.00;
	m_GreenFactor = newGreenFactor / 100.00;
	m_BlueFactor = newBlueFactor / 100.00;
}

int Bmphandler::LoadDIBitmap(const char* filename) /* I - File to load */
{
	FILE* fp; /* Open file pointer */
	unsigned char* bits_tmp;
	unsigned int bitsize;		 /* Size of bitmap */
	BITMAPFILEHEADER header; /* File header */

	/* Try opening the file; use "rb" mode to read this *binary* file. */
	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	/* Read the file header and any following bitmap information... */
	if (fread(&header, sizeof(BITMAPFILEHEADER), 1, fp) < 1)
	{
		/* Couldn't read the file header - return nullptr... */
		fclose(fp);
		return 0;
	}

	if (header.bfType != ID_BMP) /* Check for BM reversed... */
	{
		/* Not a bitmap file - return nullptr... */
		fclose(fp);
		return 0;
	}

	int infosize = header.bfOffBits - sizeof(BITMAPFILEHEADER);

	BITMAPINFO* bmpinfo;
	if ((bmpinfo = (BITMAPINFO*)malloc(infosize)) == nullptr)
	//	if ((bmpinfo = (BITMAPINFO *)malloc(40)) == nullptr)
	{
		/* Couldn't allocate memory for bitmap info - return nullptr... */
		fclose(fp);
		return 0;
	}

	if (fread(bmpinfo, 1, infosize, fp) < (unsigned int)infosize)

	//	if(fread(bmpinfo, 1, 40, fp)<40)

	//    if (fread(&bmpinfo, 1, infosize, fp) < infosize)
	{
		/* Couldn't read the bitmap header - return nullptr... */
		//       free(*info);
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

	/* Now that we have all the header info read in, allocate memory for *
     * the bitmap and read *it* in...                                    */
	//if (bmpinfo->bmiHeader.biBitCount!=8||bmpinfo->bmiHeader.biCompression!=0){
	if (bmpinfo->bmiHeader.biCompression != 0)
	{
		//		free(*info);
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

	m_Width = (short unsigned)bmpinfo->bmiHeader.biWidth;
	m_Height = (short unsigned)abs(bmpinfo->bmiHeader.biHeight);
	unsigned newarea = m_Height * (unsigned int)m_Width;

	if ((bitsize = bmpinfo->bmiHeader.biSizeImage) == 0)
	{
		int padding = m_Width % 4;
		if (padding != 0)
			padding = 4 - padding;
		bitsize = (unsigned int)(m_Width + padding) * m_Height;
	}

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}

	ClearTissue(0);

	if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
	{
		/* Couldn't allocate memory - return nullptr! */
		//       free(*info);
		//		free(bits);
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

	//int result = fseek(fp,header.bfOffBits - sizeof(BITMAPFILEHEADER) - 40, SEEK_CUR);

	if (bmpinfo->bmiHeader.biBitCount == 24 ||
			bmpinfo->bmiHeader.biBitCount == 32)
	{
		int result = ConvertImageTo8BitBMP(filename, bits_tmp);
		if (result == 0)
		{
			free(bits_tmp);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}
		bitsize /= (bmpinfo->bmiHeader.biBitCount / 8);
	}
	else
	{
		if (fread(bits_tmp, 1, bitsize, fp) < (unsigned int)bitsize)
		{
			/* Couldn't read bitmap - free memory and return nullptr! */
			//       free(*info);
			//       free(bits);
			free(bits_tmp);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}
	}

	if (bitsize == newarea)
	{
		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}
	}
	else
	{
		unsigned long i = 0;
		unsigned long j = 0;
		unsigned incr = 4 - (m_Width % 4);
		if (incr == 4)
			incr = 0;
		for (unsigned i1 = 0; i1 < m_Height; i1++)
		{
			for (unsigned j1 = 0; j1 < m_Width; j1++)
			{
				m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[j];
				i++;
				j++;
			}
			j += incr;
		}
	}

	free(bits_tmp);
	free(bmpinfo);

	/* OK, everything went fine - return the allocated bitmap... */
	fclose(fp);
	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();
	m_Mode1 = m_Mode2 = 1;
	return 1;
}

int Bmphandler::LoadDIBitmap(const char* filename, Point p, unsigned short dx, unsigned short dy) /* I - File to load */
{
	FILE* fp; /* Open file pointer */
	unsigned char* bits_tmp;
	unsigned int bitsize;		 /* Size of bitmap */
	BITMAPFILEHEADER header; /* File header */

	m_Width = dx;
	m_Height = dy;
	unsigned newarea = unsigned(dx) * dy;
	unsigned short w, h;

	/* Try opening the file; use "rb" mode to read this *binary* file. */
	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	/* Read the file header and any following bitmap information... */
	if (fread(&header, sizeof(BITMAPFILEHEADER), 1, fp) < 1)
	{
		/* Couldn't read the file header - return nullptr... */
		fclose(fp);
		return 0;
	}

	if (header.bfType != ID_BMP) /* Check for BM reversed... */
	{
		/* Not a bitmap file - return nullptr... */
		fclose(fp);
		return 0;
	}

	int infosize = header.bfOffBits - sizeof(BITMAPFILEHEADER);

	BITMAPINFO* bmpinfo;
	if ((bmpinfo = (BITMAPINFO*)malloc(infosize)) == nullptr)
	//    if ((bmpinfo = (BITMAPINFO *)malloc(infosize)) == nullptr)
	//	if ((bmpinfo = (BITMAPINFO *)malloc(40)) == nullptr)
	{
		/* Couldn't allocate memory for bitmap info - return nullptr... */
		fclose(fp);
		return 0;
	}

	if (fread(bmpinfo, 1, infosize, fp) < (unsigned int)infosize)
	//	if(fread(bmpinfo, 1, 40, fp)<40)

	//    if (fread(&bmpinfo, 1, infosize, fp) < infosize)
	{
		/* Couldn't read the bitmap header - return nullptr... */
		//       free(*info);
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

	/* Now that we have all the header info read in, allocate memory for *
     * the bitmap and read *it* in...                                    */

	if (bmpinfo->bmiHeader.biBitCount != 8 ||
			bmpinfo->bmiHeader.biCompression != 0)
	{
		//		free(*info);
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

	w = (short unsigned)bmpinfo->bmiHeader.biWidth;
	//	bmpinfo->bmiHeader.biWidth=(int)dx;
	h = (short unsigned)abs(bmpinfo->bmiHeader.biHeight);
	//	bmpinfo->bmiHeader.biHeight=(int)dy;

	bitsize = newarea;

	unsigned incr = 4 - w % 4;
	if (incr == 4)
		incr = 0;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}

	ClearTissue(0);

	if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
	{
		/* Couldn't allocate memory - return nullptr! */
		//       free(*info);
		//		free(bits);
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

#ifdef _MSC_VER
	int result = _fseeki64(fp, (__int64)(w + incr) * p.py + p.px, SEEK_CUR);
#else
	int result = fseek(fp, (size_t)(w + incr) * p.py + p.px, SEEK_CUR);
#endif
	if (result)
	{
		m_Sliceprovide->TakeBack(m_BmpBits);
		m_Sliceprovide->TakeBack(m_WorkBits);
		m_Sliceprovide->TakeBack(m_HelpBits);
		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			free(m_Tissuelayers[idx]);
		}
		m_Tissuelayers.clear();
		free(bits_tmp);
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

	for (unsigned short n = 0; n < dy; n++)
	{
		if ((unsigned short)fread(bits_tmp + n * dx, 1, dx, fp) < dx)
		{
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			free(bits_tmp);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}
		if (n + 1 != dy)
		{
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)w + incr - dx, SEEK_CUR);
#else
			result = fseek(fp, (size_t)w + incr - dx, SEEK_CUR);
#endif
			if (result)
			{
				m_Sliceprovide->TakeBack(m_BmpBits);
				m_Sliceprovide->TakeBack(m_WorkBits);
				m_Sliceprovide->TakeBack(m_HelpBits);
				for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size();
						 ++idx)
				{
					free(m_Tissuelayers[idx]);
				}
				m_Tissuelayers.clear();
				free(bits_tmp);
				fclose(fp);
				free(bmpinfo);
				return 0;
			}
		}
	}

	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
	}

	free(bits_tmp);
	free(bmpinfo);

	/* OK, everything went fine - return the allocated bitmap... */
	fclose(fp);
	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();
	m_Mode1 = m_Mode2 = 1;
	return 1;
}

void Bmphandler::SetRGBtoGrayScaleFactors(double newRedFactor, double newGreenFactor, double newBlueFactor)
{
	m_RedFactor = newRedFactor;
	m_GreenFactor = newGreenFactor;
	m_BlueFactor = newBlueFactor;
}

int Bmphandler::ConvertImageTo8BitBMP(const char* filename, unsigned char*& bits_tmp) const
{
	// construct image from reading an image file.
	cimg_library::CImg<float> src_no_norm(filename);
	cimg_library::CImg<unsigned char> src = src_no_norm.get_normalize(0, 255);

	int width = src.width();
	int height = src.height();

	// convert RGB image to gray scale image
	double r, g, b;
	int counter = 0;
	for (int j = height - 1; j > 0; j--) // flipped ?
	{
		for (int i = 0; i < width; i++, counter++)
		{
			r = src(i, j, 0, 0); // First channel RED
			g = src(i, j, 0, 1); // Second channel GREEN
			b = src(i, j, 0, 2); // Third channel BLUE

			bits_tmp[counter] = (unsigned char)(m_RedFactor * r + m_GreenFactor * g + m_BlueFactor * b);
		}
	}

	return 1;
}

int Bmphandler::ConvertPNGImageTo8BitBMP(const char* filename, unsigned char*& bits_tmp) const
{
	QImage source_image(filename);

	unsigned int counter = 0;
	QColor old_color;
	for (int y = source_image.height() - 1; y >= 0; y--)
	{
		for (int x = 0; x < source_image.width(); x++)
		{
			old_color = QColor(source_image.pixel(x, y));
			bits_tmp[counter] = (unsigned char)(m_RedFactor * old_color.red() +
																					m_GreenFactor * old_color.green() +
																					m_BlueFactor * old_color.blue());
			counter++;
		}
	}

	return 1;
}

int Bmphandler::ReloadDIBitmap(const char* filename) /* I - File to load */
{
	if (!m_Loaded)
		return 0;
	FILE* fp; /* Open file pointer */
	unsigned char* bits_tmp;
	unsigned int bitsize;		 /* Size of bitmap */
	BITMAPFILEHEADER header; /* File header */

	/* Try opening the file; use "rb" mode to read this *binary* file. */
	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	/* Read the file header and any following bitmap information... */
	if (fread(&header, sizeof(BITMAPFILEHEADER), 1, fp) < 1)
	{
		/* Couldn't read the file header - return nullptr... */
		fclose(fp);
		return 0;
	}

	if (header.bfType != ID_BMP) /* Check for BM reversed... */
	{
		/* Not a bitmap file - return nullptr... */
		fclose(fp);
		return 0;
	}

	/*	if(infosize != header.bfOffBits - sizeof(BITMAPFILEHEADER)){
		fclose(fp);
        return 0;
	}*/

	int infosize = header.bfOffBits - sizeof(BITMAPFILEHEADER);

	BITMAPINFO* bmpinfo;
	if ((bmpinfo = (BITMAPINFO*)malloc(infosize)) == nullptr)
	{
		fclose(fp);
		return 0;
	}
	//	BITMAPINFO *bmpinfo=(BITMAPINFO* )malloc(40);
	if (fread(bmpinfo, 1, infosize, fp) < (unsigned int)infosize)
	//	if(fread(bmpinfo, 1, 40, fp)<40)

	//    if (fread(&bmpinfo, 1, infosize, fp) < infosize)
	{
		/* Couldn't read the bitmap header - return nullptr... */
		//       free(*info);
		free(bmpinfo);
		fclose(fp);
		return 0;
	}

	/* Now that we have all the header info read in, allocate memory for *
     * the bitmap and read *it* in...                                    */

	if (bmpinfo->bmiHeader.biBitCount != 8 ||
			bmpinfo->bmiHeader.biCompression != 0)
	{
		//		free(*info);
		free(bmpinfo);
		fclose(fp);
		return 0;
	}

	if (m_Width != (short unsigned)bmpinfo->bmiHeader.biWidth ||
			m_Height != (short unsigned)abs(bmpinfo->bmiHeader.biHeight))
	{
		free(bmpinfo);
		fclose(fp);
		return 0;
	}

	if ((bitsize = bmpinfo->bmiHeader.biSizeImage) == 0)
	{
		int padding = m_Width % 4;
		if (padding != 4)
			padding = 4 - padding;
		bitsize = (unsigned int)(m_Width + padding) * m_Height;
	}

	if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
	{
		/* Couldn't allocate memory - return nullptr! */
		//       free(*info);
		//		free(bits);
		free(bmpinfo);
		fclose(fp);
		return 0;
	}

	//int result = fseek(fp,header.bfOffBits - sizeof(BITMAPFILEHEADER) - 40, SEEK_CUR);

	if (fread(bits_tmp, 1, bitsize, fp) < (unsigned int)bitsize)
	{
		/* Couldn't read bitmap - free memory and return nullptr! */
		//       free(*info);
		//       free(bits);
		free(bmpinfo);
		free(bits_tmp);
		fclose(fp);
		return 0;
	}

	if (bitsize == m_Area)
	{
		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_BmpBits[i] = (float)bits_tmp[i];
		}
	}
	else
	{
		unsigned long i = 0;
		unsigned long j = 0;
		unsigned incr = 4 - (m_Width % 4);
		if (incr == 4)
			incr = 0;
		for (unsigned i1 = 0; i1 < m_Height; i1++)
		{
			for (unsigned j1 = 0; j1 < m_Width; j1++)
			{
				m_BmpBits[i] = (float)bits_tmp[j];
				i++;
				j++;
			}
			j += incr;
		}
	}

	free(bits_tmp);
	free(bmpinfo);

	/* OK, everything went fine - return the allocated bitmap... */
	fclose(fp);
	m_Mode1 = 1;

	return 1;
}

int Bmphandler::ReloadDIBitmap(const char* filename, Point p)
{
	if (!m_Loaded)
		return 0;
	FILE* fp; /* Open file pointer */
	unsigned char* bits_tmp;
	unsigned int bitsize;		 /* Size of bitmap */
	BITMAPFILEHEADER header; /* File header */
	unsigned short w, h;

	/* Try opening the file; use "rb" mode to read this *binary* file. */
	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	/* Read the file header and any following bitmap information... */
	if (fread(&header, sizeof(BITMAPFILEHEADER), 1, fp) < 1)
	{
		/* Couldn't read the file header - return nullptr... */
		fclose(fp);
		return 0;
	}

	if (header.bfType != ID_BMP) /* Check for BM reversed... */
	{
		/* Not a bitmap file - return nullptr... */
		fclose(fp);
		return 0;
	}

	/*	if(infosize != header.bfOffBits - sizeof(BITMAPFILEHEADER)){
		fclose(fp);
        return 0;
	}*/
	int infosize = header.bfOffBits - sizeof(BITMAPFILEHEADER);

	BITMAPINFO* bmpinfo;
	if ((bmpinfo = (BITMAPINFO*)malloc(infosize)) == nullptr)
	{
		fclose(fp);
		return 0;
	}
	//	BITMAPINFO *bmpinfo=(BITMAPINFO *)malloc(40);
	if (fread(bmpinfo, 1, infosize, fp) < (unsigned int)infosize)
	//	if(fread(bmpinfo, 1, 40, fp)<(unsigned int)40)

	//    if (fread(&bmpinfo, 1, infosize, fp) < infosize)
	{
		/* Couldn't read the bitmap header - return nullptr... */
		//       free(*info);
		free(bmpinfo);
		fclose(fp);
		return 0;
	}

	h = (unsigned short)bmpinfo->bmiHeader.biHeight;
	//	bmpinfo->bmiHeader.biHeight = height;
	w = (unsigned short)bmpinfo->bmiHeader.biWidth;
	//	bmpinfo->bmiHeader.biWidth = width;

	/* Now that we have all the header info read in, allocate memory for *
     * the bitmap and read *it* in...                                    */

	if (bmpinfo->bmiHeader.biBitCount != 8 ||
			bmpinfo->bmiHeader.biCompression != 0)
	{
		//		free(*info);
		free(bmpinfo);
		fclose(fp);
		return 0;
	}

	bitsize = m_Area;

	if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
	{
		/* Couldn't allocate memory - return nullptr! */
		//       free(*info);
		//		free(bits);
		free(bmpinfo);
		fclose(fp);
		return 0;
	}

	unsigned incr = 4 - w % 4;
	if (incr == 4)
		incr = 0;

#ifdef _MSC_VER
	int result = _fseeki64(fp, (__int64)(w + incr) * p.py + p.px + header.bfOffBits, SEEK_CUR);
#else
	int result = fseek(fp, (size_t)(w + incr) * p.py + p.px + header.bfOffBits, SEEK_CUR);
#endif
	if (result)
	{
		/*		free(bmp_bits);
		free(work_bits);
		free(help_bits);*/
		free(bmpinfo);
		free(bits_tmp);
		fclose(fp);
		return 0;
	}

	for (unsigned short n = 0; n < m_Height; n++)
	{
		if ((unsigned short)fread(bits_tmp + n * m_Width, 1, m_Width, fp) < m_Width)
		{
			/*			free(bmp_bits);
			free(work_bits);
			free(help_bits);*/
			free(bmpinfo);
			free(bits_tmp);
			fclose(fp);
			return 0;
		}
		if (n + 1 != m_Height)
		{
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)w + incr - m_Width, SEEK_CUR);
#else
			result = fseek(fp, (size_t)w + incr - m_Width, SEEK_CUR);
#endif
			if (result)
			{
				/*			free(bmp_bits);
				free(work_bits);
				free(help_bits);*/
				free(bmpinfo);
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}
	}

	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_BmpBits[i] = (float)bits_tmp[i];
	}

	free(bits_tmp);
	free(bmpinfo);

	fclose(fp);
	m_Mode1 = 1;

	return 1;
}

int Bmphandler::CheckPNGDepth(const char* filename)
{
	QImage image(filename);
	return image.depth();
}

int Bmphandler::LoadPNGBitmap(const char* filename)
{
	unsigned char* bits_tmp;
	unsigned int bitsize; /* Size of bitmap */

	//Check if the file exists
	QImage image(filename);
	if (image.isNull())
	{
		return 0;
	}
	m_Width = (short unsigned)image.width();
	m_Height = (short unsigned)image.height();
	unsigned newarea = m_Height * (unsigned int)m_Width;
	bitsize = (unsigned int)(m_Width * m_Height);

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}

	ClearTissue(0);

	if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
	{
		/* Couldn't allocate memory - return nullptr! */
		return 0;
	}

	int result = ConvertPNGImageTo8BitBMP(filename, bits_tmp);
	if (result == 0)
	{
		free(bits_tmp);
		return 0;
	}

	if (bitsize == newarea)
	{
		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}
	}
	else
	{
		unsigned long i = 0;
		unsigned long j = 0;
		unsigned incr = 4 - (m_Width % 4);
		if (incr == 4)
			incr = 0;
		for (unsigned i1 = 0; i1 < m_Height; i1++)
		{
			for (unsigned j1 = 0; j1 < m_Width; j1++)
			{
				m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[j];
				i++;
				j++;
			}
			j += incr;
		}
	}

	free(bits_tmp);

	/* OK, everything went fine - return the allocated bitmap... */
	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();
	m_Mode1 = m_Mode2 = 1;
	return 1;
}

bool Bmphandler::LoadArray(float* bits, unsigned short w1, unsigned short h1)
{
	m_Width = w1;
	m_Height = h1;

	unsigned newarea = m_Height * (unsigned int)m_Width;

	if (newarea == 0)
		return false;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}

	ClearTissue(0);

	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = m_BmpBits[i] = bits[i];
	}

	/* OK, everything went fine - return the allocated bitmap... */
	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();

	m_Mode1 = m_Mode2 = 1;

	return true;
}

bool Bmphandler::LoadArray(float* bits, unsigned short w, unsigned short h, Point p, unsigned short dx, unsigned short dy)
{
	if (p.px > w)
	{
		p.px = 0;
		dx = 0;
	}
	if (p.py > h)
	{
		p.py = 0;
		dy = 0;
	}
	if (dx + p.px > w)
	{
		dx = w - p.px;
	}
	if (dy + p.py > h)
	{
		dy = h - p.py;
	}

	m_Width = dx;
	m_Height = dy;
	unsigned newarea = dx * (unsigned int)dy;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return false;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}

	ClearTissue(0);

	unsigned int pos1;
	unsigned int pos2 = 0;
	for (unsigned short j = 0; j < dy; j++)
	{
		pos1 = (p.py + j) * (unsigned int)(w) + p.px;
		for (unsigned short i = 0; i < dx; i++, pos1++, pos2++)
		{
			m_WorkBits[pos2] = m_BmpBits[pos2] = bits[pos1];
		}
	}

	/* OK, everything went fine - return the allocated bitmap... */
	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();

	m_Mode1 = m_Mode2 = 1;

	return true;
}

bool Bmphandler::ReloadArray(float* bits)
{
	if (!m_Loaded)
		return false;

	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_BmpBits[i] = bits[i];
	}

	m_Mode1 = 1;
	return true;
}

bool Bmphandler::ReloadArray(float* bits, unsigned short w1, unsigned short h1, Point p)
{
	if (!m_Loaded)
		return false;

	if (m_Width + p.px > w1 || m_Height + p.py > h1)
	{
		return false;
	}

	unsigned int pos1;
	unsigned int pos2 = 0;
	for (unsigned short j = 0; j < m_Height; j++)
	{
		pos1 = (p.py + j) * (unsigned int)(w1) + p.px;
		for (unsigned short i = 0; i < m_Width; i++, pos1++, pos2++)
		{
			m_WorkBits[pos2] = m_BmpBits[pos2] = bits[pos1];
		}
	}

	/* OK, everything went fine - return the allocated bitmap... */
	m_Mode1 = 1;

	return true;
}

bool Bmphandler::LoadDICOM(const char* filename)
{
	DicomReader dcmread;

	if (!dcmread.Opendicom(filename))
		return false;

	m_Width = dcmread.GetWidth();
	m_Height = dcmread.GetHeight();

	unsigned newarea = m_Height * (unsigned int)m_Width;

	if (newarea == 0)
		return false;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}

	ClearTissue(0);

	if (!dcmread.LoadPicture(m_BmpBits))
	{
		if (!dcmread.LoadPictureGdcm(filename, m_BmpBits))
		{
			dcmread.Closedicom();
			return false;
		}
	}

	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = m_BmpBits[i];
	}

	/* OK, everything went fine - return the allocated bitmap... */
	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();

	dcmread.Closedicom();
	m_Mode1 = m_Mode2 = 1;

	return true;
}

bool Bmphandler::LoadDICOM(const char* filename, Point p, unsigned short dx, unsigned short dy)
{
	DicomReader dcmread;
	dcmread.Opendicom(filename);

	unsigned short w = dcmread.GetWidth();
	unsigned short h = dcmread.GetHeight();
	if (p.px > w)
	{
		p.px = 0;
		dx = 0;
	}
	if (p.py > h)
	{
		p.py = 0;
		dy = 0;
	}
	if (dx + p.px > w)
	{
		dx = w - p.px;
	}
	if (dy + p.py > h)
	{
		dy = h - p.py;
	}

	m_Width = dx;
	m_Height = dy;
	unsigned newarea = dx * (unsigned int)dy;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.Closedicom();
			return false;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
	}

	ClearTissue(0);

	dcmread.LoadPicture(m_BmpBits, p, dx, dy);

	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = m_BmpBits[i];
	}

	/* OK, everything went fine - return the allocated bitmap... */
	m_Loaded = true;
	ClearMarks();
	ClearVvm();
	ClearLimits();

	dcmread.Closedicom();
	m_Mode1 = m_Mode2 = 1;

	return true;
}

bool Bmphandler::ReloadDICOM(const char* filename)
{
	if (!m_Loaded)
		return false;
	//FILE             *fp;          /* Open file pointer */
	DicomReader dcmread;

	dcmread.Opendicom(filename);

	if (m_Width != dcmread.GetWidth() || m_Height != dcmread.GetHeight())
	{
		dcmread.Closedicom();
		return false;
	}

	dcmread.LoadPicture(m_BmpBits);

	/*for(int i=0;i<area;i++){
		work_bits[i]=bmp_bits[i];
	} */

	/* OK, everything went fine - return the allocated bitmap... */
	dcmread.Closedicom();
	m_Mode1 = 1;
	return true;
}

bool Bmphandler::ReloadDICOM(const char* filename, Point p)
{
	if (!m_Loaded)
		return false;
	//FILE             *fp;          /* Open file pointer */
	DicomReader dcmread;

	dcmread.Opendicom(filename);

	if (m_Width + p.px > dcmread.GetWidth() ||
			m_Height + p.py > dcmread.GetHeight())
	{
		dcmread.Closedicom();
		return false;
	}

	dcmread.LoadPicture(m_BmpBits, p, m_Width, m_Height);

	/*for(int i=0;i<area;i++){
		work_bits[i]=bmp_bits[i];
	} */

	/* OK, everything went fine - return the allocated bitmap... */
	dcmread.Closedicom();
	m_Mode1 = 1;

	return true;
}

/*
 * 'SaveDIBitmap()' - Save a DIB/BMP file to disk.
 *
 * Returns 0 on success or -1 on failure...
 */

//int                                /* O - 0 = success, -1 = failure */
//Bmphandler::SaveDIBitmap(const char *filename,float *p_bits) /* I - File to load */
//    {
//    FILE             *fp;          /* Open file pointer */
//    int              size,         /* Size of file */
//                     bitsize;      /* Size of bitmap pixels */
//    BITMAPFILEHEADER header;       /* File header */
//	unsigned char *	 bits_tmp;
//
//	bits_tmp=(unsigned char *)malloc(area);
//	if(bits_tmp==nullptr)
//		return -1;
//
//	for(unsigned int i=0;i<area;i++){
//		bits_tmp[i]=unsigned char(min(255,max(0,p_bits[i]+0.5)));
//	}
//
//    /* Try opening the file; use "wb" mode to write this *binary* file. */
//    if ((fp = fopen(filename, "wb")) == nullptr)
//        return (-1);
//
//	if (bmpinfo->bmiHeader.biBitCount!=8||bmpinfo->bmiHeader.biCompression!=0||bmpinfo->bmiHeader.biPlanes>1)
//		return -1;
//
//    /* Figure out the bitmap size */
//    if (bmpinfo->bmiHeader.biSizeImage == 0)
//	bitsize = (int) area;
//    else
//	bitsize = bmpinfo->bmiHeader.biSizeImage = (int) area;
//
//    /* Figure out the header size */
////    infosize = sizeof(BITMAPINFOHEADER);
//
//    size = sizeof(BITMAPFILEHEADER) + infosize + bitsize;
//
//    /* Write the file header, bitmap information, and bitmap pixel data... */
//    header.bfType      = 'MB'; /* Non-portable... sigh */
//    header.bfSize      = size;
//    header.bfReserved1 = 0;
//    header.bfReserved2 = 0;
//    header.bfOffBits   = sizeof(BITMAPFILEHEADER) + infosize;
//
//    if (fwrite(&header, 1, sizeof(BITMAPFILEHEADER), fp) < sizeof(BITMAPFILEHEADER)) {
//        /* Couldn't write the file header - return... */
//        fclose(fp);
//        return (-1);
//    }
//
//	bmpinfo->bmiHeader.biHeight=height;
//	bmpinfo->bmiHeader.biWidth=width;
//	bmpinfo->bmiHeader.biClrUsed=0;
//	bmpinfo->bmiHeader.biClrImportant=0;
//
//    if (fwrite(bmpinfo, 1, infosize, fp) < (unsigned int)infosize)
//        {
//        /* Couldn't write the bitmap header - return... */
//        fclose(fp);
//        return (-1);
//        }
//
//    if (fwrite(bits_tmp, 1, bitsize, fp) < (unsigned int)bitsize)
//        {
//        /* Couldn't write the bitmap - return... */
//        fclose(fp);
//        return (-1);
//        }
//
//    /* OK, everything went fine - return... */
//
//	free(bits_tmp);
//
//    fclose(fp);
//    return (0);
//}

FILE* Bmphandler::SaveProj(FILE* fp, bool inclpics)
{
	if (m_Loaded)
	{
		fwrite(&m_Width, 1, sizeof(unsigned short), fp);
		fwrite(&m_Height, 1, sizeof(unsigned short), fp);
		if (inclpics)
		{
			fwrite(m_BmpBits, 1, m_Area * sizeof(float), fp);
			fwrite(m_WorkBits, 1, m_Area * sizeof(float), fp);
			fwrite(m_Tissuelayers[0], 1, m_Area * sizeof(tissues_size_t), fp); // TODO
		}
		int size = -1 - int(m_Marks.size());
		fwrite(&size, 1, sizeof(int), fp);
		int marks_version = 2;
		fwrite(&marks_version, 1, sizeof(int), fp);
		for (auto& m : m_Marks)
		{
			fwrite(&(m.mark), 1, sizeof(unsigned), fp);
			fwrite(&(m.p.px), 1, sizeof(unsigned short), fp);
			fwrite(&(m.p.py), 1, sizeof(unsigned short), fp);
			if (marks_version >= 2)
			{
				int size1 = m.name.length();
				fwrite(&size1, 1, sizeof(int), fp);
				fwrite(m.name.c_str(), 1, sizeof(char) * size1, fp);
			}
		}
		size = int(m_Vvm.size());
		fwrite(&size, 1, sizeof(int), fp);

		for (auto& it1 : m_Vvm)
		{
			size = int(it1.size());
			fwrite(&size, 1, sizeof(int), fp);
			for (auto& m : it1)
			{
				fwrite(&(m.mark), 1, sizeof(unsigned), fp);
				fwrite(&(m.p.px), 1, sizeof(unsigned short), fp);
				fwrite(&(m.p.py), 1, sizeof(unsigned short), fp);
			}
		}
		size = int(m_Limits.size());
		fwrite(&size, 1, sizeof(int), fp);
		for (auto& it1 : m_Limits)
		{
			size = int(it1.size());
			fwrite(&size, 1, sizeof(int), fp);
			for (auto& it : it1)
			{
				fwrite(&(it.px), 1, sizeof(unsigned short), fp);
				fwrite(&(it.py), 1, sizeof(unsigned short), fp);
			}
		}

		fwrite(&m_Mode1, 1, sizeof(unsigned char), fp);
		fwrite(&m_Mode2, 1, sizeof(unsigned char), fp);
	}
	return fp;
}

FILE* Bmphandler::SaveStack(FILE* fp) const
{
	if (m_Loaded)
	{
		fwrite(&stackcounter, 1, sizeof(unsigned), fp);

		int size = -int(stackindex.size()) - 1;
		fwrite(&size, 1, sizeof(int), fp);
		int stack_version = 1;
		fwrite(&stack_version, 1, sizeof(int), fp);
		for (std::list<unsigned>::iterator it = stackindex.begin();
				 it != stackindex.end(); it++)
		{
			fwrite(&(*it), 1, sizeof(unsigned), fp);
		}

		size = int(bits_stack.size());
		fwrite(&size, 1, sizeof(int), fp);
		for (std::list<float*>::iterator it = bits_stack.begin();
				 it != bits_stack.end(); it++)
		{
			fwrite(*it, 1, sizeof(float) * m_Area, fp);
		}

		size = int(mode_stack.size());
		fwrite(&size, 1, sizeof(int), fp);
		for (std::list<unsigned char>::iterator it = mode_stack.begin();
				 it != mode_stack.end(); it++)
		{
			fwrite(&(*it), 1, sizeof(unsigned char), fp);
		}
	}
	return fp;
}

FILE* Bmphandler::LoadProj(FILE* fp, int tissuesVersion, bool inclpics, bool init)
{
	unsigned short width1, height1;
	fread(&width1, sizeof(unsigned short), 1, fp);
	fread(&height1, sizeof(unsigned short), 1, fp);

	Newbmp(width1, height1, init);

	if (inclpics)
	{
		fread(m_BmpBits, m_Area * sizeof(float), 1, fp);
		fread(m_WorkBits, m_Area * sizeof(float), 1, fp);
		tissues_size_t* tissues = m_Tissuelayers[0]; // TODO
		if (tissuesVersion > 0)
		{
			fread(tissues, m_Area * sizeof(tissues_size_t), 1, fp);
		}
		else
		{
			unsigned char* uchar_buffer = (unsigned char*)malloc(m_Area);
			fread(uchar_buffer, m_Area, 1, fp);
			for (unsigned int i = 0; i < m_Area; ++i)
			{
				tissues[i] = (tissues_size_t)uchar_buffer[i];
			}
			free(uchar_buffer);
		}
	}

	int size, size1;
	fread(&size, sizeof(int), 1, fp);

	int marks_version = 0;
	if (size < 0)
	{
		size = -1 - size;
		fread(&marks_version, sizeof(int), 1, fp);
	}

	Mark m;
	m_Marks.clear();
	char name[100];
	for (int j = 0; j < size; j++)
	{
		fread(&(m.mark), sizeof(unsigned), 1, fp);
		fread(&(m.p.px), sizeof(unsigned short), 1, fp);
		fread(&(m.p.py), sizeof(unsigned short), 1, fp);
		if (marks_version >= 2)
		{
			int size1;
			fread(&size1, sizeof(int), 1, fp);
			fread(name, sizeof(char) * size1, 1, fp);
			name[size1] = '\0';
			m.name = std::string(name);
		}
		m_Marks.push_back(m);
	}

	ClearVvm();
	fread(&size1, sizeof(int), 1, fp);

	m_Vvm.resize(size1);
	for (int j1 = 0; j1 < size1; j1++)
	{
		fread(&size, sizeof(int), 1, fp);
		for (int j = 0; j < size; j++)
		{
			fread(&(m.mark), sizeof(unsigned), 1, fp);
			fread(&(m.p.px), sizeof(unsigned short), 1, fp);
			fread(&(m.p.py), sizeof(unsigned short), 1, fp);
			m_Vvm[j1].push_back(m);
		}
		if (size > 0)
		{
			m_MaximStore = std::max(m_MaximStore, m_Vvm[j1].begin()->mark);
		}
	}

	Point p1;
	ClearLimits();
	fread(&size1, sizeof(int), 1, fp);

	m_Limits.resize(size1);
	for (int j1 = 0; j1 < size1; j1++)
	{
		fread(&size, sizeof(int), 1, fp);
		for (int j = 0; j < size; j++)
		{
			fread(&(p1.px), sizeof(unsigned short), 1, fp);
			fread(&(p1.py), sizeof(unsigned short), 1, fp);
			m_Limits[j1].push_back(p1);
		}
	}

	if (marks_version > 0)
	{
		fread(&m_Mode1, sizeof(unsigned char), 1, fp);
		fread(&m_Mode2, sizeof(unsigned char), 1, fp);
	}
	else
	{
		m_Mode1 = 1;
		m_Mode2 = 2;
	}

	return fp;
}

FILE* Bmphandler::LoadStack(FILE* fp)
{
	fread(&stackcounter, sizeof(unsigned), 1, fp);

	int size1;
	fread(&size1, sizeof(int), 1, fp);
	int stack_version = 0;
	if (size1 < 0)
	{
		size1 = -1 - size1;
		fread(&stack_version, sizeof(int), 1, fp);
		//		if(stackVersion<1) fseek(fp,-1, SEEK_CUR);
	}

	stackindex.clear();
	unsigned dummy;
	for (int i = 0; i < size1; i++)
	{
		fread(&dummy, sizeof(unsigned), 1, fp);
		stackindex.push_back(dummy);
	}

	int size;
	fread(&size, sizeof(int), 1, fp);
	bits_stack.clear();
	float* f;
	for (int i = 0; i < size; i++)
	{
		f = m_Sliceprovide->GiveMe();
		fread(f, sizeof(float) * m_Area, 1, fp);
		bits_stack.push_back(f);
	}

	if (stack_version > 0)
	{
		fread(&size, sizeof(int), 1, fp);
		mode_stack.clear();
		unsigned char dummymode;
		for (int i = 0; i < size; i++)
		{
			fread(&dummymode, sizeof(unsigned char), 1, fp);
			mode_stack.push_back(dummymode);
		}
	}

	return fp;
}

//the code below works, however, it produces indexed bmp that can not be read by iSeg instead of greyscale ones
//int                                /* O - 0 = success, -1 = failure */
//Bmphandler::SaveDIBitmap(const char *filename,float *p_bits) /* I - File to load */
//    {
//    FILE             *fp;          /* Open file pointer */
//    int              size,         /* Size of file */
//                     bitsize;      /* Size of bitmap pixels */
//    BITMAPFILEHEADER header;       /* File header */
//	unsigned char *	 bits_tmp;
//
//	unsigned char info1[1068];
//	BITMAPINFO *bmpinfo1=(BITMAPINFO *) info1;
//	bmpinfo1->bmiHeader.biSize=40;
//	bmpinfo1->bmiHeader.biBitCount=8;
//	bmpinfo1->bmiHeader.biCompression=0;
//	bmpinfo1->bmiHeader.biPlanes=1;
//	bmpinfo1->bmiHeader.biSizeImage=(int) area;
//	bmpinfo1->bmiHeader.biClrImportant=0;
//	bmpinfo1->bmiHeader.biClrUsed=0;
//	bmpinfo1->bmiHeader.biHeight=(int)height;
//	bmpinfo1->bmiHeader.biWidth=(int)width;
//	bmpinfo1->bmiHeader.biXPelsPerMeter=0;
//	bmpinfo1->bmiHeader.biYPelsPerMeter=0;
//
//	bits_tmp=(unsigned char *)malloc(area);
//	if(bits_tmp==nullptr)
//		return -1;
//
//	for(unsigned int i=0;i<area;i++){
//		bits_tmp[i]=unsigned char(min(255,max(0,p_bits[i]+0.5)));
//	}
//
//    /* Try opening the file; use "wb" mode to write this *binary* file. */
//    if ((fp = fopen(filename, "wb")) == nullptr)
//        return (-1);
//
////	if (bmpinfo->bmiHeader.biBitCount!=8||bmpinfo->bmiHeader.biCompression!=0||bmpinfo->bmiHeader.biPlanes>1)
////		return -1;
//
//    /* Figure out the bitmap size */
////    if (bmpinfo->bmiHeader.biSizeImage == 0)
//	bitsize = (int) area;
////    else
////	bitsize = bmpinfo->bmiHeader.biSizeImage = (int) area;
//
//    /* Figure out the header size */
////    infosize = sizeof(BITMAPINFOHEADER);
//
//    size = sizeof(BITMAPFILEHEADER) + 1064 + bitsize;
//
//    /* Write the file header, bitmap information, and bitmap pixel data... */
//    header.bfType      = 'MB'; /* Non-portable... sigh */
//    header.bfSize      = size;
//    header.bfReserved1 = 0;
//    header.bfReserved2 = 0;
//    header.bfOffBits   = sizeof(BITMAPFILEHEADER) + 1064;
//
//    if (fwrite(&header, 1, sizeof(BITMAPFILEHEADER), fp) < sizeof(BITMAPFILEHEADER)) {
//        /* Couldn't write the file header - return... */
//        fclose(fp);
//        return (-1);
//    }
//
///*	bmpinfo->bmiHeader.biHeight=height;
//	bmpinfo->bmiHeader.biWidth=width;
//	bmpinfo->bmiHeader.biClrUsed=0;
//	bmpinfo->bmiHeader.biClrImportant=0;*/
//
//	unsigned char *cp=(unsigned char *)bmpinfo1;
//
//	for(unsigned i1=0;i1<256;i1++) {
//
//		cp[i1*4+40]=cp[i1*4+41]=cp[i1*4+42]=(unsigned char)i1;
//		cp[i1*4+43]=0;
//	}
//	for(unsigned i1=1;i1<255;i1++) {
//		cp[i1*4+40]=cp[i1*4+41]=cp[i1*4+42]=(unsigned char)(i1+1);
//		cp[i1*4+43]=0;
//	}
//
//	if (fwrite(bmpinfo1, 1, 1064, fp) < 1064)
//        {
//        /* Couldn't write the bitmap header - return... */
//        fclose(fp);
//        return (-1);
//        }
//
//    if (fwrite(bits_tmp, 1, bitsize, fp) < (unsigned int)bitsize)
//        {
//        /* Couldn't write the bitmap - return... */
//        fclose(fp);
//        return (-1);
//        }
//
//
//    /* OK, everything went fine - return... */
//
//	free(bits_tmp);
//	free(info1);
//
//    fclose(fp);
//    return (0);
//}
int																																/* O - 0 = success, -1 = failure */
		Bmphandler::SaveDIBitmap(const char* filename, float* p_bits) const /* I - File to load */
{
	FILE* fp;								 /* Open file pointer */
	unsigned int size;			 /* Size of file */
	unsigned int bitsize;		 /* Size of bitmap pixels */
	BITMAPFILEHEADER header; /* File header */
	unsigned char* bits_tmp;

	unsigned char info1[1068];
	BITMAPINFO* bmpinfo1 = (BITMAPINFO*)info1;
	bmpinfo1->bmiHeader.biSize = 40;
	bmpinfo1->bmiHeader.biBitCount = 8;
	bmpinfo1->bmiHeader.biCompression = 0;
	bmpinfo1->bmiHeader.biPlanes = 1;
	bmpinfo1->bmiHeader.biSizeImage = (int)0;
	bmpinfo1->bmiHeader.biClrImportant = 0;
	bmpinfo1->bmiHeader.biClrUsed = 0;
	bmpinfo1->bmiHeader.biHeight = (int)m_Height;
	bmpinfo1->bmiHeader.biWidth = (int)m_Width;
	bmpinfo1->bmiHeader.biXPelsPerMeter = 0;
	bmpinfo1->bmiHeader.biYPelsPerMeter = 0;

	int padding = m_Width % 4;
	if (padding != 0)
		padding = 4 - padding;
	bitsize = m_Area + padding * m_Height;

	bits_tmp = (unsigned char*)malloc(bitsize);
	if (bits_tmp == nullptr)
		return -1;

	unsigned long pos = 0;
	unsigned long pos2 = 0;
	for (unsigned short j = 0; j < m_Height; j++)
	{
		for (unsigned short i = 0; i < m_Width; i++, pos++, pos2++)
		{
			bits_tmp[pos] =
					(unsigned char)(std::min(255.0, std::max(0.0, p_bits[pos2] + 0.5)));
		}
		for (unsigned int i = 0; i < padding; i++, pos++)
		{
			bits_tmp[pos] = 0;
		}
	}

	/* Try opening the file; use "wb" mode to write this *binary* file. */
	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	//	if (bmpinfo->bmiHeader.biBitCount!=8||bmpinfo->bmiHeader.biCompression!=0||bmpinfo->bmiHeader.biPlanes>1)
	//		return -1;

	/* Figure out the bitmap size */
	//    if (bmpinfo->bmiHeader.biSizeImage == 0)
	//    else
	//	bitsize = bmpinfo->bmiHeader.biSizeImage = (int) area;

	/* Figure out the header size */
	//    infosize = sizeof(BITMAPINFOHEADER);

	size = sizeof(BITMAPFILEHEADER) + 1064 + bitsize + 2;

	/* Write the file header, bitmap information, and bitmap pixel data... */
	header.bfType = ID_BMP; /* Non-portable... sigh */
	header.bfSize = size;
	header.bfReserved1 = 0;
	header.bfReserved2 = 0;
	header.bfOffBits = sizeof(BITMAPFILEHEADER) + 1064;

	if (fwrite(&header, 1, sizeof(BITMAPFILEHEADER), fp) <
			sizeof(BITMAPFILEHEADER))
	{
		/* Couldn't write the file header - return... */
		fclose(fp);
		return (-1);
	}

	/*	bmpinfo->bmiHeader.biHeight=height;
	bmpinfo->bmiHeader.biWidth=width;
	bmpinfo->bmiHeader.biClrUsed=0;
	bmpinfo->bmiHeader.biClrImportant=0;*/

	unsigned char* cp = (unsigned char*)bmpinfo1;

	for (unsigned i1 = 0; i1 < 256; i1++)
	{
		cp[i1 * 4 + 40] = cp[i1 * 4 + 41] = cp[i1 * 4 + 42] = (unsigned char)i1;
		cp[i1 * 4 + 43] = 0;
	}
	for (unsigned i1 = 1; i1 < 255; i1++)
	{
		cp[i1 * 4 + 40] = cp[i1 * 4 + 41] = cp[i1 * 4 + 42] =
				(unsigned char)(i1 + 1);
		cp[i1 * 4 + 43] = 0;
	}

	if (fwrite(bmpinfo1, 1, 1064, fp) < 1064)
	{
		/* Couldn't write the bitmap header - return... */
		fclose(fp);
		return (-1);
	}

	if (fwrite(bits_tmp, 1, bitsize, fp) < (unsigned int)bitsize)
	{
		/* Couldn't write the bitmap - return... */
		fclose(fp);
		return (-1);
	}

	/* OK, everything went fine - return... */
	free(bits_tmp);

	fclose(fp);
	return (0);
}

#if 0 // Old version: Greyscale bmp

int Bmphandler::SaveTissueBitmap(tissuelayers_size_t idx, const char *filename)
{
    FILE             *fp;          /* Open file pointer */
    int              size,         /* Size of file */
                     bitsize,      /* Size of bitmap pixels */
					 bmpInfoSize;  /* Size of bitmap info */
    BITMAPFILEHEADER header;       /* File header */

	int padding=width%4;
	if(padding!=0) padding=4-padding;
	bitsize = (int) area + padding*height;

	if (sizeof(tissues_size_t) == 1) // 1-byte tissue indices: Add color palette to bitmap
	{
		bmpInfoSize = 1064;
		//unsigned char info1[1068]; // TODO: Why +4?
		unsigned char *info1 = (unsigned char*) malloc(bmpInfoSize);
		BITMAPINFO *bmpinfo1=(BITMAPINFO *) info1;
		bmpinfo1->bmiHeader.biSize=40;
		bmpinfo1->bmiHeader.biBitCount=8;
		bmpinfo1->bmiHeader.biCompression=0;
		bmpinfo1->bmiHeader.biPlanes=1;
	//	bmpinfo1->bmiHeader.biSizeImage=(int) area;
		bmpinfo1->bmiHeader.biSizeImage=0;
		bmpinfo1->bmiHeader.biClrImportant=0;
		bmpinfo1->bmiHeader.biClrUsed=0;
		bmpinfo1->bmiHeader.biHeight=(int)height;
		bmpinfo1->bmiHeader.biWidth=(int)width;
		bmpinfo1->bmiHeader.biXPelsPerMeter=0;
		bmpinfo1->bmiHeader.biYPelsPerMeter=0;
		bmpinfo1->bmiHeader.biSizeImage=(int) bitsize;

		/* Try opening the file; use "wb" mode to write this *binary* file. */
		if ((fp = fopen(filename, "wb")) == nullptr)
			return (-1);

	//	if (bmpinfo->bmiHeader.biBitCount!=8||bmpinfo->bmiHeader.biCompression!=0||bmpinfo->bmiHeader.biPlanes>1)
	//		return -1;

		/* Figure out the bitmap size */
	//    if (bmpinfo->bmiHeader.biSizeImage == 0)
	//    else
	//	bitsize = bmpinfo->bmiHeader.biSizeImage = (int) area;

		/* Figure out the header size */
	//    infosize = sizeof(BITMAPINFOHEADER);

		// File size = Header + Bitmap info + Pixel data + Tail
		size = sizeof(BITMAPFILEHEADER) + bmpInfoSize + bitsize + 2;

		/* Write the file header, bitmap information, and bitmap pixel data... */
		header.bfType      = 'MB'; /* Non-portable... sigh */
		header.bfSize      = size;
		header.bfReserved1 = 0;
		header.bfReserved2 = 0;
		header.bfOffBits   = sizeof(BITMAPFILEHEADER) + bmpInfoSize;

		if (fwrite(&header, 1, sizeof(BITMAPFILEHEADER), fp) < sizeof(BITMAPFILEHEADER)) {
			/* Couldn't write the file header - return... */
			fclose(fp);
			return (-1);
		}

	/*	bmpinfo->bmiHeader.biHeight=height;
		bmpinfo->bmiHeader.biWidth=width;
		bmpinfo->bmiHeader.biClrUsed=0;
		bmpinfo->bmiHeader.biClrImportant=0;*/

		unsigned char *cp=(unsigned char *)bmpinfo1;

#	if 1 // 256 Greyscale
		for(unsigned i1=0;i1<256;i1++) {
			cp[i1*4+40]=cp[i1*4+41]=cp[i1*4+42]=(unsigned char)i1;
			cp[i1*4+43]=0;
		}
#	else // Color palette
		for(unsigned i1=0;i1<256;i1++) {
			cp[i1*4+40]=colorPaletteRGBA256[i1*4];
			cp[i1*4+41]=colorPaletteRGBA256[i1*4+1];
			cp[i1*4+42]=colorPaletteRGBA256[i1*4+2];
			cp[i1*4+43]=0;
		}
#	endif

		/*for(unsigned i1=1;i1<255;i1++) {
			cp[i1*4+40]=cp[i1*4+41]=cp[i1*4+42]=(unsigned char)(i1+1);
			cp[i1*4+43]=0;
		}*/

		if (fwrite(bmpinfo1, 1, bmpInfoSize, fp) < bmpInfoSize)
		{
			/* Couldn't write the bitmap header - return... */
			fclose(fp);
			return (-1);
		}

		if(padding==0) {
			if (fwrite(tissuelayers[idx], 1, bitsize, fp) < (unsigned int)bitsize)
			{
			/* Couldn't write the bitmap - return... */
			fclose(fp);
			return (-1);
			}
		} else {
			tissues_size_t pad[4];
			pad[0]=pad[1]=pad[2]=pad[3]=0;
			for(unsigned short i=0;i<height;i++) {
				if (fwrite(&(tissuelayers[idx][int(i)*width]), 1, width, fp) < (unsigned int)width)
				{
					/* Couldn't write the bitmap - return... */
					fclose(fp);
					return (-1);
				}
				if (fwrite(pad, 1, padding, fp) < (unsigned int)padding)
				{
					/* Couldn't write the bitmap - return... */
					fclose(fp);
					return (-1);
				}
			}
		}
		free(info1);
	}
	else // Tissue indices exceed 1 byte: Do not add color palette to bitmap
	{
		bmpInfoSize = 44;
		unsigned char *info1 = (unsigned char*) malloc(bmpInfoSize);
		BITMAPINFO *bmpinfo1=(BITMAPINFO *) info1;
		bmpinfo1->bmiHeader.biSize=40;
		bmpinfo1->bmiHeader.biBitCount=8*sizeof(tissues_size_t);
		bmpinfo1->bmiHeader.biCompression=BI_RGB; // TODO: ifndef win32
		bmpinfo1->bmiHeader.biPlanes=1;
		bmpinfo1->bmiHeader.biSizeImage=0;
		bmpinfo1->bmiHeader.biClrImportant=0;
		bmpinfo1->bmiHeader.biClrUsed=0;
		bmpinfo1->bmiHeader.biHeight=(int)height;
		bmpinfo1->bmiHeader.biWidth=(int)width;
		bmpinfo1->bmiHeader.biXPelsPerMeter=0;
		bmpinfo1->bmiHeader.biYPelsPerMeter=0;
		bmpinfo1->bmiHeader.biSizeImage=(int) bitsize;

		/* Try opening the file; use "wb" mode to write this *binary* file. */
		if ((fp = fopen(filename, "wb")) == nullptr)
			return (-1);

		// File size = Header + Bitmap info + Pixel data + Tail
		size = sizeof(BITMAPFILEHEADER) + bmpInfoSize + bitsize*sizeof(tissues_size_t) + 2;

		/* Write the file header, bitmap information, and bitmap pixel data... */
		header.bfType      = 'MB'; /* Non-portable... sigh */
		header.bfSize      = size;
		header.bfReserved1 = 0;
		header.bfReserved2 = 0;
		header.bfOffBits   = sizeof(BITMAPFILEHEADER) + bmpInfoSize;

		if (fwrite(&header, 1, sizeof(BITMAPFILEHEADER), fp) < sizeof(BITMAPFILEHEADER)) {
			/* Couldn't write the file header - return... */
			fclose(fp);
			return (-1);
		}

		if (fwrite(bmpinfo1, 1, bmpInfoSize, fp) < bmpInfoSize)
		{
			/* Couldn't write the bitmap header - return... */
			fclose(fp);
			return (-1);
		}

		if(padding==0) {
			if (fwrite(tissuelayers[idx], sizeof(tissues_size_t), bitsize, fp) < (unsigned int)bitsize)
			{
				/* Couldn't write the bitmap - return... */
				fclose(fp);
				return (-1);
			}
		} else {
			tissues_size_t pad[4];
			pad[0]=pad[1]=pad[2]=pad[3]=0;
			for(unsigned short i=0;i<height;i++) {
				if (fwrite(&(tissuelayers[idx][int(i)*width]), sizeof(tissues_size_t), width, fp) < (unsigned int)width)
				{
					/* Couldn't write the bitmap - return... */
					fclose(fp);
					return (-1);
				}
				if (fwrite(pad, sizeof(tissues_size_t), padding, fp) < (unsigned int)padding)
				{
					/* Couldn't write the bitmap - return... */
					fclose(fp);
					return (-1);
				}
			}
		}
		free(info1);
	}

	unsigned char tail[2];
	tail[0]=tail[1]=0;
	if (fwrite(tail, 1, 2, fp) < (unsigned int)2)
    {
		/* Couldn't write the bitmap - return... */
		fclose(fp);
		return (-1);
    }

    fclose(fp);
    return (0);
}

#else // New version: 32bpp RGBA Tissue colors

int Bmphandler::SaveTissueBitmap(tissuelayers_size_t idx, const char* filename)
{
	vtkSmartPointer<vtkImageData> image_source =
			vtkSmartPointer<vtkImageData>::New();
	image_source->SetExtent(0, m_Width - 1, 0, m_Height - 1, 0, 0);
	image_source->AllocateScalars(VTK_UNSIGNED_CHAR, 4); // 32bpp RGBA
	unsigned char* field =
			(unsigned char*)image_source->GetScalarPointer(0, 0, 0);

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < (unsigned int)m_Width * m_Height; ++i)
	{
		auto tissue_color = TissueInfos::GetTissueColor(tissues[i]);
		field[i * 4] = (tissue_color[0]) * 255;
		field[i * 4 + 1] = (tissue_color[1]) * 255;
		field[i * 4 + 2] = (tissue_color[2]) * 255;
		field[i * 4 + 3] = 0;
	}

	auto bmp_writer = vtkSmartPointer<vtkBMPWriter>::New();
	bmp_writer->SetFileName(filename);
	bmp_writer->SetInputData(image_source);
	bmp_writer->Write();

	return (0);
}

#endif

int Bmphandler::SaveDIBitmap(const char* filename)
{
	return SaveDIBitmap(filename, m_BmpBits);
}

int Bmphandler::SaveWorkBitmap(const char* filename)
{
	return SaveDIBitmap(filename, m_WorkBits);
}

int Bmphandler::ReadAvw(const char* filename, short unsigned slicenr)
{
	unsigned int bitsize; /* Size of bitmap */

	unsigned short w, h;
	avw::eDataType type;

	void* data = avw::ReadData(filename, slicenr, w, h, type);
	if (data == nullptr)
	{
		return 0;
	}

	m_Width = w;
	m_Height = h;
	unsigned newarea = m_Height * (unsigned int)m_Width;

	bitsize = newarea;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			free(data);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			free(data);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			free(data);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			free(data);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			free(data);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			free(data);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}

	m_Loaded = true;

	if (type == avw::schar)
	{
		char* bits_tmp = (char*)data;

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(data);
	}
	else if (type == avw::uchar)
	{
		unsigned char* bits_tmp = (unsigned char*)data;

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(data);
	}
	else if (type == avw::ushort)
	{
		unsigned short* bits_tmp = (unsigned short*)data;

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(data);
	}
	else if (type == avw::sshort)
	{
		short* bits_tmp = (short*)data;

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(data);
	}
	else
	{
		return 0;
	}

	ClearMarks();
	ClearVvm();
	ClearLimits();
	m_Mode1 = m_Mode2 = 2;
	return 1;
}

int Bmphandler::ReadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr)
{
	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	m_Width = w;
	m_Height = h;
	unsigned newarea = m_Height * (unsigned int)m_Width;

	bitsize = newarea;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		if (!m_Tissuelayers[0])
		{
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}
		ClearTissue(0);
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		if (!m_Tissuelayers[0])
		{
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}
		ClearTissue(0);
	}

	m_Loaded = true;

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;

		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		// int result = fseek(fp, (size_t)(bitsize)*slicenr, SEEK_SET);
		// const fpos_t pos = (fpos_t)(bitsize)*slicenr;
		// int result = fsetpos(fp, &pos);
#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(bitsize)*slicenr, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*slicenr, SEEK_SET);
#endif
		if (result)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "Bmphandler::ReadRaw() : error, file operation failed"
								<< endl;
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, bitsize, fp) < m_Area)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "Bmphandler::ReadRaw() : error, file operation failed"
								<< endl;
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2)
	{
		unsigned short* bits_tmp;

		if ((bits_tmp = (unsigned short*)malloc(bitsize * 2)) == nullptr)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "Bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		// int result = fseek(fp, (size_t)(bitsize)*2*slicenr, SEEK_SET);
		// const fpos_t pos = (fpos_t)(bitsize)*2*slicenr;
		// int result = fsetpos(fp, &pos);
#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(bitsize)*2 * slicenr, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*2 * slicenr, SEEK_SET);
#endif
		if (result)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "Bmphandler::ReadRaw() : error, file operation failed"
								<< endl;
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, (size_t)(bitsize)*2, fp) < m_Area * 2)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "Bmphandler::ReadRaw() : error, file operation failed"
								<< endl;
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		/*		sliceprovide->take_back(bmp_bits);
		sliceprovide->take_back(work_bits);
		sliceprovide->take_back(help_bits);*/
		std::cerr << "Bmphandler::ReadRaw() : error, unsupported depth..." << endl;
		fclose(fp);
		return 0;
	}

	fclose(fp);
	ClearMarks();
	ClearVvm();
	ClearLimits();
	m_Mode1 = m_Mode2 = 1;
	return 1;
}

int Bmphandler::ReadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, Point p, unsigned short dx, unsigned short dy)
{
	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	m_Width = dx;
	m_Height = dy;
	unsigned newarea = m_Height * (unsigned int)m_Width;
	unsigned int area2 = h * (unsigned int)w;

	bitsize = newarea;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}

	m_Loaded = true;

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;

		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
		{
			/*			free(bmp_bits);
			free(work_bits);
			free(help_bits);*/
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px, SEEK_SET);
#endif
		if (result)
		{
			/*			free(bmp_bits);
			free(work_bits);
			free(help_bits);*/
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned short n = 0; n < dy; n++)
		{
			if ((unsigned short)fread(bits_tmp + n * dx, 1, dx, fp) < dx)
			{
				/*				free(bmp_bits);
				free(work_bits);
				free(help_bits);*/
				free(bits_tmp);
				fclose(fp);

				return 0;
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)w - dx, SEEK_CUR);
#else
			result = fseek(fp, (size_t)w - dx, SEEK_CUR);
#endif
			if (result)
			{
				/*				free(bmp_bits);
				free(work_bits);
				free(help_bits);*/
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2)
	{
		unsigned short* bits_tmp;

		if ((bits_tmp = (unsigned short*)malloc(bitsize * 2)) == nullptr)
		{
			/*			free(bmp_bits);
			free(work_bits);
			free(help_bits);*/
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, ((__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px) * 2, SEEK_SET);
#else
		int result =
				fseek(fp, ((size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px) * 2, SEEK_SET);
#endif
		if (result)
		{
			/*			free(bmp_bits);
			free(work_bits);
			free(help_bits);*/
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned short n = 0; n < dy; n++)
		{
			if ((unsigned short)fread(bits_tmp + n * dx, 1, dx * 2, fp) <
					2 * dx)
			{
				/*				free(bmp_bits);
				free(work_bits);
				free(help_bits);*/
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)(w - dx) * 2, SEEK_CUR);
#else
			result = fseek(fp, (size_t)(w - dx) * 2, SEEK_CUR);
#endif
			if (result)
			{
				/*				free(bmp_bits);
				free(work_bits);
				free(help_bits);*/
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_WorkBits[i] = m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		/*		free(bmp_bits);
		free(work_bits);
		free(help_bits);*/
		fclose(fp);
		return 0;
	}

	fclose(fp);
	ClearMarks();
	ClearVvm();
	ClearLimits();
	m_Mode1 = m_Mode2 = 1;
	return 1;
}

int Bmphandler::ReadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr)
{
	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	m_Width = w;
	m_Height = h;
	unsigned newarea = m_Height * (unsigned int)m_Width;

	bitsize = newarea;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}

	m_Loaded = true;

#ifdef _MSC_VER
	int result =
			_fseeki64(fp, (__int64)(bitsize) * sizeof(float) * slicenr, SEEK_SET);
#else
	int result =
			fseek(fp, (size_t)(bitsize) * sizeof(float) * slicenr, SEEK_SET);
#endif
	if (result)
	{
		/*			sliceprovide->take_back(bmp_bits);
		sliceprovide->take_back(work_bits);
		sliceprovide->take_back(help_bits);*/
		fclose(fp);
		return 0;
	}

	if (fread(m_BmpBits, 1, bitsize * sizeof(float), fp) < m_Area * sizeof(float))
	{
		/*			sliceprovide->take_back(bmp_bits);
		sliceprovide->take_back(work_bits);
		sliceprovide->take_back(help_bits);*/
		fclose(fp);
		return 0;
	}

	for (unsigned int i = 0; i < bitsize; i++)
	{
		m_WorkBits[i] = m_BmpBits[i];
	}

	fclose(fp);
	ClearMarks();
	ClearVvm();
	ClearLimits();
	m_Mode1 = m_Mode2 = 1;
	return 1;
}

int Bmphandler::ReadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr, Point p, unsigned short dx, unsigned short dy)
{
	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	m_Width = dx;
	m_Height = dy;
	unsigned newarea = m_Height * (unsigned int)m_Width;
	unsigned int area2 = h * (unsigned int)w;

	bitsize = newarea;

	if (m_Area != newarea)
	{
		if (m_Loaded)
		{
			ClearStack();
			m_Sliceprovide->TakeBack(m_BmpBits);
			m_Sliceprovide->TakeBack(m_WorkBits);
			m_Sliceprovide->TakeBack(m_HelpBits);
			for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
			{
				free(m_Tissuelayers[idx]);
			}
			m_Tissuelayers.clear();
			m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		}

		m_Area = newarea;

		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}
	else if (!m_Loaded)
	{
		m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

		if ((m_BmpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_WorkBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((m_HelpBits = m_Sliceprovide->GiveMe()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		m_Tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area));
		ClearTissue(0);
	}

	m_Loaded = true;

#ifdef _MSC_VER
	int result = _fseeki64(fp, ((__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px) * sizeof(float), SEEK_SET);
#else
	int result = fseek(fp, ((size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px) * sizeof(float), SEEK_SET);
#endif
	if (result)
	{
		/*			free(bmp_bits);
		free(work_bits);
		free(help_bits);*/
		fclose(fp);
		return 0;
	}

	for (unsigned short n = 0; n < dy; n++)
	{
		if ((unsigned short)fread(m_BmpBits + n * dx, 1, dx * sizeof(float), fp) < sizeof(float) * dx)
		{
			/*				free(bmp_bits);
			free(work_bits);
			free(help_bits);*/
			fclose(fp);
			return 0;
		}
#ifdef _MSC_VER
		result = _fseeki64(fp, (__int64)(w - dx) * sizeof(float), SEEK_CUR);
#else
		result = fseek(fp, (size_t)(w - dx) * sizeof(float), SEEK_CUR);
#endif
		if (result)
		{
			/*				free(bmp_bits);
			free(work_bits);
			free(help_bits);*/
			fclose(fp);
			return 0;
		}
	}

	for (unsigned int i = 0; i < bitsize; i++)
	{
		m_WorkBits[i] = m_BmpBits[i];
	}

	fclose(fp);
	ClearMarks();
	ClearVvm();
	ClearLimits();
	m_Mode1 = m_Mode2 = 1;
	return 1;
}

int Bmphandler::ReloadRaw(const char* filename, unsigned bitdepth, unsigned slicenr)
{
	if (!m_Loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = m_Area;

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;

		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
		{
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(bitsize)*slicenr, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*slicenr, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, bitsize, fp) < m_Area)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2)
	{
		unsigned short* bits_tmp;

		if ((bits_tmp = (unsigned short*)malloc(bitsize * 2)) == nullptr)
		{
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(bitsize)*2 * slicenr, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*2 * slicenr, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, bitsize * 2, fp) < m_Area * 2)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	m_Mode1 = 1;
	return 1;
}

int Bmphandler::ReloadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned slicenr, Point p)
{
	if (!m_Loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = m_Area;
	unsigned long area2 = (unsigned long)(w)*h;

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;

		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
		{
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned short n = 0; n < m_Height; n++)
		{
			if ((unsigned short)fread(bits_tmp + n * m_Width, 1, m_Width, fp) <
					m_Width)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)w - m_Width, SEEK_CUR);
#else
			result = fseek(fp, (size_t)w - m_Width, SEEK_CUR);
#endif
			if (result)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2)
	{
		unsigned short* bits_tmp;

		if ((bits_tmp = (unsigned short*)malloc(bitsize * 2)) == nullptr)
		{
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, ((__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px) * 2, SEEK_SET);
#else
		int result = fseek(fp, ((size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px) * 2, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned short n = 0; n < m_Height; n++)
		{
			if ((unsigned short)fread(bits_tmp + n * m_Width, 1, m_Width * 2, fp) <
					2 * m_Width)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, ((__int64)(w)-m_Width) * 2, SEEK_CUR);
#else
			result = fseek(fp, ((size_t)(w)-m_Width) * 2, SEEK_CUR);
#endif
			if (result)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			m_BmpBits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	m_Mode1 = 1;
	return 1;
}

int Bmphandler::ReloadRawFloat(const char* filename, unsigned slicenr)
{
	if (!m_Loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = m_Area;

#ifdef _MSC_VER
	int result =
			_fseeki64(fp, (__int64)(bitsize) * sizeof(float) * slicenr, SEEK_SET);
#else
	int result =
			fseek(fp, (size_t)(bitsize) * sizeof(float) * slicenr, SEEK_SET);
#endif
	if (result)
	{
		fclose(fp);
		return 0;
	}

	if (fread(m_BmpBits, 1, bitsize * sizeof(float), fp) < m_Area * sizeof(float))
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	m_Mode1 = 1;
	return 1;
}

float* Bmphandler::ReadRawFloat(const char* filename, unsigned slicenr, unsigned int area)
{
	FILE* fp; /* Open file pointer */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return nullptr;

#ifdef _MSC_VER
	int result =
			_fseeki64(fp, (__int64)(area) * sizeof(float) * slicenr, SEEK_SET);
#else
	int result = fseek(fp, (size_t)(area) * sizeof(float) * slicenr, SEEK_SET);
#endif
	if (result)
	{
		fclose(fp);
		return nullptr;
	}

	float* bits_red;

	if ((bits_red = (float*)malloc(sizeof(float) * area)) == nullptr)
	{
		fclose(fp);
		return nullptr;
	}

	if (fread(bits_red, 1, area * sizeof(float), fp) < area * sizeof(float))
	{
		fclose(fp);
		return nullptr;
	}

	fclose(fp);
	return bits_red;
}

int Bmphandler::ReloadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned slicenr, Point p)
{
	if (!m_Loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = m_Area;
	unsigned long area2 = (unsigned long)(w)*h;

#ifdef _MSC_VER
	int result = _fseeki64(fp, ((__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px) * sizeof(float), SEEK_SET);
#else
	int result = fseek(fp, ((size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px) * sizeof(float), SEEK_SET);
#endif
	if (result)
	{
		fclose(fp);
		return 0;
	}

	for (unsigned short n = 0; n < m_Height; n++)
	{
		if ((unsigned short)fread(m_BmpBits + n * m_Width, 1, m_Width * sizeof(float), fp) < sizeof(float) * m_Width)
		{
			fclose(fp);
			return 0;
		}
#ifdef _MSC_VER
		result = _fseeki64(fp, (__int64)(w - m_Width) * sizeof(float), SEEK_CUR);
#else
		result = fseek(fp, (size_t)(w - m_Width) * sizeof(float), SEEK_CUR);
#endif
		if (result)
		{
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	m_Mode1 = 1;
	return 1;
}

int Bmphandler::ReloadRawTissues(const char* filename, unsigned bitdepth, unsigned slicenr)
{
	if (!m_Loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap stack */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = m_Area * static_cast<unsigned>(m_Tissuelayers.size());

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;

		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
		{
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(bitsize)*slicenr, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*slicenr, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, bitsize, fp) < m_Area)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = m_Tissuelayers[idx];
			for (unsigned int i = 0; i < m_Area; i++)
			{
				tissues[i] = (tissues_size_t)bits_tmp[i + idx * m_Area];
			}
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2)
	{
		unsigned short* bits_tmp;

		if ((bits_tmp = (unsigned short*)malloc(bitsize * 2)) == nullptr)
		{
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(bitsize)*2 * slicenr, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*2 * slicenr, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, bitsize * 2, fp) < m_Area * 2)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = m_Tissuelayers[idx];
			for (unsigned int i = 0; i < m_Area; i++)
			{
				tissues[i] = (tissues_size_t)bits_tmp[i + idx * m_Area];
			}
		}

		free(bits_tmp);
	}
	else
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 1;
}

int Bmphandler::ReloadRawTissues(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned slicenr, Point p)
{
	if (!m_Loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap stack */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = m_Area * static_cast<unsigned>(m_Tissuelayers.size());
	unsigned long area2 = (unsigned long)(w)*h;

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;

		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
		{
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(area2)*slicenr * m_Tissuelayers.size() + (__int64)(w)*p.py + p.px, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(area2)*slicenr * m_Tissuelayers.size() + (size_t)(w)*p.py + p.px, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			for (unsigned short n = 0; n < m_Height; n++)
			{
				if ((unsigned short)fread(bits_tmp + m_Area * idx + n * m_Width, 1, m_Width, fp) < m_Width)
				{
					free(bits_tmp);
					fclose(fp);
					return 0;
				}
#ifdef _MSC_VER
				result = _fseeki64(fp, (__int64)(w - m_Width), SEEK_CUR);
#else
				result = fseek(fp, (size_t)(w - m_Width), SEEK_CUR);
#endif
				if (result)
				{
					free(bits_tmp);
					fclose(fp);
					return 0;
				}
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)(h - m_Height) * w, SEEK_CUR);
#else
			result = fseek(fp, (size_t)(h - m_Height) * w, SEEK_CUR);
#endif
			if (result)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}

		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = m_Tissuelayers[idx];
			for (unsigned int i = 0; i < m_Area; i++)
			{
				tissues[i] = (tissues_size_t)bits_tmp[i + idx * m_Area];
			}
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2)
	{
		unsigned short* bits_tmp;

		if ((bits_tmp = (unsigned short*)malloc(bitsize * 2)) == nullptr)
		{
			fclose(fp);
			return 0;
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, ((__int64)(area2)*slicenr * m_Tissuelayers.size() + (__int64)(w)*p.py + p.px) * 2, SEEK_SET);
#else
		int result = fseek(fp, ((size_t)(area2)*slicenr * m_Tissuelayers.size() + (size_t)(w)*p.py + p.px) * 2, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			for (unsigned short n = 0; n < m_Height; n++)
			{
				if ((unsigned short)fread(bits_tmp + m_Area * idx + n * m_Width, 1, m_Width * 2, fp) < 2 * m_Width)
				{
					free(bits_tmp);
					fclose(fp);
					return 0;
				}
#ifdef _MSC_VER
				result = _fseeki64(fp, (__int64)(w - m_Width) * 2, SEEK_CUR);
#else
				result = fseek(fp, (size_t)(w - m_Width) * 2, SEEK_CUR);
#endif
				if (result)
				{
					free(bits_tmp);
					fclose(fp);
					return 0;
				}
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)(h - m_Height) * w * 2, SEEK_CUR);
#else
			result = fseek(fp, (size_t)(h - m_Height) * w * 2, SEEK_CUR);
#endif
			if (result)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}

		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = m_Tissuelayers[idx];
			for (unsigned int i = 0; i < m_Area; i++)
			{
				tissues[i] = (tissues_size_t)bits_tmp[i + idx * m_Area];
			}
		}

		free(bits_tmp);
	}
	else
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	return 1;
}

int Bmphandler::SaveBmpRaw(const char* filename)
{
	return SaveRaw(filename, m_BmpBits);
}

int Bmphandler::SaveWorkRaw(const char* filename)
{
	return SaveRaw(filename, m_WorkBits);
}

int Bmphandler::SaveRaw(const char* filename, float* p_bits) const
{
	FILE* fp;
	unsigned char* bits_tmp;

	bits_tmp = (unsigned char*)malloc(m_Area);
	if (bits_tmp == nullptr)
		return -1;

	for (unsigned int i = 0; i < m_Area; i++)
	{
		bits_tmp[i] = (unsigned char)(std::min(255.0, std::max(0.0, p_bits[i] + 0.5)));
	}

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = m_Width * (unsigned)m_Height;

	if (fwrite(bits_tmp, 1, bitsize, fp) < bitsize)
	{
		fclose(fp);
		return (-1);
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int Bmphandler::SaveTissueRaw(tissuelayers_size_t idx, const char* filename)
{
	FILE* fp;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = m_Width * (unsigned)m_Height;

	if ((TissueInfos::GetTissueCount() <= 255) &&
			(sizeof(tissues_size_t) > sizeof(unsigned char)))
	{
		unsigned char* uchar_buffer = new unsigned char[bitsize];
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned int i = 0; i < bitsize; ++i)
		{
			uchar_buffer[i] = (unsigned char)tissues[i];
		}
		if (fwrite(uchar_buffer, sizeof(unsigned char), bitsize, fp) < bitsize)
		{
			fclose(fp);
			delete[] uchar_buffer;
			return (-1);
		}
		delete[] uchar_buffer;
	}
	else
	{
		if (fwrite(m_Tissuelayers[idx], sizeof(tissues_size_t), bitsize, fp) <
				bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	fclose(fp);
	return 0;
}

void Bmphandler::SetWorkPt(Point p, float f)
{
	m_WorkBits[m_Width * p.py + p.px] = f;
}

void Bmphandler::SetBmpPt(Point p, float f)
{
	m_BmpBits[m_Width * p.py + p.px] = f;
}

void Bmphandler::SetTissuePt(tissuelayers_size_t idx, Point p, tissues_size_t f)
{
	m_Tissuelayers[idx][m_Width * p.py + p.px] = f;
}

float Bmphandler::BmpPt(Point p) { return m_BmpBits[m_Width * p.py + p.px]; }

float Bmphandler::WorkPt(Point p) { return m_WorkBits[m_Width * p.py + p.px]; }

tissues_size_t Bmphandler::TissuesPt(tissuelayers_size_t idx, Point p)
{
	return Bmphandler::m_Tissuelayers[idx][m_Width * p.py + p.px];
}

void Bmphandler::PrintInfo() const
{
	/*	cout << "SizeInfoHeader " << bmpinfo->bmiHeader.biSize <<endl;
	cout << "Width " << width <<endl;
	cout << "Height " << bmpinfo->bmiHeader.biHeight << endl;
	cout << "Planes " << bmpinfo->bmiHeader.biPlanes << endl;
	cout << "BitCount " << bmpinfo->bmiHeader.biBitCount << endl;
	cout << "Compression " << bmpinfo->bmiHeader.biCompression << endl;
	cout << "SizeImage " << bmpinfo->bmiHeader.biCompression << endl;
	cout << "NrPix/m x " << bmpinfo->bmiHeader.biXPelsPerMeter << endl;
	cout << "NrPix/m y " << bmpinfo->bmiHeader.biYPelsPerMeter << endl;
	cout << "NrColors " << bmpinfo->bmiHeader.biClrUsed << endl;
	cout << "NrImpColors " << bmpinfo->bmiHeader.biClrImportant << endl;*/

	std::cout << "Width " << m_Width << std::endl;
	std::cout << "Height " << m_Height << std::endl;

	/*if(bmpinfo->bmiHeader.biSize>40){
		for(unsigned int i=0;i<bmpinfo->bmiHeader.biClrUsed;i++)
			cout << "Color " << i << ": " << bmpinfo->bmiColors[i].rgbBlue <<" "<< bmpinfo->bmiColors[i].rgbGreen<< " " << bmpinfo->bmiColors[i].rgbRed<<endl;
	}*/
}

unsigned int Bmphandler::ReturnArea() const { return m_Area; }

unsigned int Bmphandler::ReturnWidth() const { return m_Width; }

unsigned int Bmphandler::ReturnHeight() const { return m_Height; }

unsigned int Bmphandler::MakeHistogram(bool includeoutofrange)
{
	unsigned int j = 0;
	float k;
	for (int i = 0; i < 256; i++)
		m_Histogram[i] = 0;
	for (unsigned int i = 0; i < m_Area; i++)
	{
		k = m_WorkBits[i];
		if (k < 0 || k >= 256)
		{
			if (includeoutofrange && k < 0)
				m_Histogram[0]++;
			if (includeoutofrange && k >= 256)
				m_Histogram[255]++;
			j++;
		}
		else
			m_Histogram[(int)k]++;
	}
	return j;
}

unsigned int Bmphandler::MakeHistogram(float* mask, float f, bool includeoutofrange)
{
	unsigned int j = 0;
	float k;
	for (int i = 0; i < 256; i++)
		m_Histogram[i] = 0;
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (mask[i] == f)
		{
			k = m_WorkBits[i];
			if (k < 0 || k >= 256)
			{
				if (includeoutofrange && k < 0)
					m_Histogram[0]++;
				if (includeoutofrange && k >= 256)
					m_Histogram[255]++;
				j++;
			}
			else
				m_Histogram[(int)k]++;
		}
	}
	return j;
}

unsigned int Bmphandler::MakeHistogram(Point p, unsigned short dx, unsigned short dy, bool includeoutofrange)
{
	unsigned int i, l;
	l = 0;
	float m;
	for (i = 0; i < 256; i++)
		m_Histogram[i] = 0;

	dx = std::min(int(dx), m_Width - p.px);
	dy = std::min(int(dy), m_Height - p.py);

	i = Pt2coord(p);
	for (int j = 0; j < dy; j++)
	{
		for (int k = 0; k < dx; k++)
		{
			m = m_WorkBits[i];
			if (m < 0 || m >= 256)
			{
				if (includeoutofrange && k < 0)
					m_Histogram[0]++;
				if (includeoutofrange && k >= 256)
					m_Histogram[255]++;
				l++;
			}
			else
				m_Histogram[(int)m]++;
			i++;
		}
		i = i + m_Width - dx;
	}

	return l;
}

unsigned int* Bmphandler::ReturnHistogram() { return m_Histogram; }

void Bmphandler::PrintHistogram()
{
	for (int i = 0; i < 256; i++)
		std::cout << m_Histogram[i] << " ";
	std::cout << std::endl;
}

void Bmphandler::Threshold(float* thresholds)
{
	const short unsigned n = (short unsigned)thresholds[0];

	if (n > 0)
	{
		const float leveldiff = 255.0f / n;

		short unsigned j;

		for (unsigned int i = 0; i < m_Area; i++)
		{
			j = 0;
			while (j < n && m_BmpBits[i] > thresholds[j + 1])
				j++;
			m_WorkBits[i] = j * leveldiff;
		}
	}

	m_Mode2 = 2;
}

void Bmphandler::Threshold(float* thresholds, Point p, unsigned short dx, unsigned short dy)
{
	dx = std::min(int(dx), m_Width - p.px);
	dy = std::min(int(dy), m_Width - p.py);

	short unsigned n = (short unsigned)thresholds[0];
	if (n > 0)
	{
		const float leveldiff = 255.0f / n;
		short unsigned l;
		unsigned int i;
		i = Pt2coord(p);

		for (int j = 0; j < dy; j++)
		{
			for (int k = 0; k < dx; k++)
			{
				l = 0;
				while (m_BmpBits[i] > thresholds[l + 1] && l < n)
					l++;
				m_WorkBits[i] = l * leveldiff;
				i++;
			}
			i = i + m_Width - dx;
		}
	}

	m_Mode2 = 2;
}

void Bmphandler::Work2bmp()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_BmpBits[i] = m_WorkBits[i];

	m_Mode1 = m_Mode2;
}

void Bmphandler::Bmp2work()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = m_BmpBits[i];

	m_Mode2 = m_Mode1;
}

void Bmphandler::Work2tissue(tissuelayers_size_t idx)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (m_WorkBits[i] < 0.0f)
			tissues[i] = 0;
		else if (m_WorkBits[i] > (float)TISSUES_SIZE_MAX)
			tissues[i] = TISSUES_SIZE_MAX;
		else
			tissues[i] = (tissues_size_t)floor(m_WorkBits[i] + 0.5f);
	}
}

void Bmphandler::Mergetissue(tissues_size_t tissuetype, tissuelayers_size_t idx)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (m_WorkBits[i] > 0.0f)
			tissues[i] = tissuetype;
	}
}

void Bmphandler::SwapBmpwork()
{
	std::swap(m_BmpBits, m_WorkBits);
	std::swap(m_Mode1, m_Mode2);
}

void Bmphandler::SwapBmphelp()
{
	std::swap(m_HelpBits, m_BmpBits);
}

void Bmphandler::SwapWorkhelp()
{
	std::swap(m_HelpBits, m_WorkBits);
}

float* Bmphandler::MakeGaussfilter(float sigma, int n)
{
	float* filter;
	if (n % 2 == 0)
		return nullptr;
	else
	{
		filter = (float*)malloc((n + 1) * sizeof(float));
		for (int i = -n / 2; i <= n / 2; i++)
			filter[i + n / 2 + 1] = exp(-float(i * i) / (2 * sigma * sigma));
		filter[n / 2 + 1] = exp(-1 / (16 * sigma * sigma));

		float summe = 0;
		for (int i = 1; i <= n; i++)
			summe += filter[i];
		for (int i = 1; i <= n; i++)
			filter[i] = filter[i] / summe;

		filter[0] = (float)n;

		return filter;
	}
}

float* Bmphandler::MakeLaplacianfilter()
{
	float* filter = (float*)malloc(4 * sizeof(float));

	filter[0] = 3;
	filter[1] = 1;
	filter[2] = -2;
	filter[3] = 1;

	return filter;
}

void Bmphandler::Convolute(float* mask, unsigned short direction)
{
	unsigned i, n;
	float dummy;
	switch (direction)
	{
	case 0:
		n = (unsigned)mask[0];
		i = 0;
		for (unsigned j = 0; j < m_Height; j++)
		{
			for (unsigned k = 0; k <= m_Width - n; k++)
			{
				dummy = 0;
				for (unsigned l = 0; l < n; l++)
					dummy += mask[l + 1] * m_BmpBits[i + l];
				m_WorkBits[i + n / 2] = dummy;
				i++;
			}
			i += n - 1;
		}

		i = 0;
		for (unsigned j = 0; j < n / 2; j++)
		{
			m_WorkBits[i] = 0;
			i++;
		}
		i += m_Width - n + 1;
		for (unsigned k = 0; k + 1 < m_Height; k++)
		{
			for (unsigned j = 0; j < n - 1; j++)
			{
				m_WorkBits[i] = 0;
				i++;
			}
			i += m_Width - n + 1;
		}
		for (; i < m_Area; i++)
			m_WorkBits[i] = 0;
		break;
	case 1:
		n = (unsigned)mask[0];
		i = 0;
		for (unsigned j = 0; j < m_Width * (m_Height - n + 1); j++)
		{
			dummy = 0;
			for (unsigned l = 0; l < n; l++)
				dummy += mask[l + 1] * m_BmpBits[i + l * m_Width];
			m_WorkBits[i + (n / 2) * m_Width] = dummy;
			i++;
		}

		for (i = 0; i < (n / 2) * m_Width; i++)
			m_WorkBits[i] = 0;
		for (i = m_Area - (n / 2) * m_Width; i < m_Area; i++)
			m_WorkBits[i] = 0;
		break;
	case 2:
		n = (unsigned)mask[0];
		unsigned m = (unsigned)mask[1];
		i = 0;
		for (unsigned j = 0; j <= m_Height - m; j++)
		{
			for (unsigned k = 0; k <= m_Width - n; k++)
			{
				dummy = 0;
				for (unsigned l = 0; l < n; l++)
				{
					for (unsigned o = 0; o < m; o++)
					{
						dummy +=
								mask[l + n * o + 2] * m_BmpBits[i + l + o * m_Width];
					}
				}
				m_WorkBits[i + n / 2 + (m / 2) * m_Width] = dummy;
				i++;
			}
			i += n - 1;
		}

		i = 0;
		for (unsigned j = 0; j < n / 2 + (m / 2) * m_Width; j++)
		{
			m_WorkBits[i] = 0;
			i++;
		}
		i += m_Width - n + 1;
		for (unsigned k = 0; k + m < m_Height; k++)
		{
			for (unsigned j = 0; j < n - 1; j++)
			{
				m_WorkBits[i] = 0;
				i++;
			}
			i += m_Width - n + 1;
		}
		for (; i < m_Area; i++)
			m_WorkBits[i] = 0;
		break;
	}
}

void Bmphandler::ConvoluteHist(float* mask)
{
	float histo[256];
	int n = (int)mask[0];
	float dummy;

	for (int k = 0; k <= 256 - n; k++)
	{
		histo[k + n / 2] = 0;
		for (int l = 0; l < n; l++)
			histo[k + n / 2] += mask[l + 1] * m_Histogram[k + l];
	}

	for (int k = 0; k < n / 2; k++)
	{
		histo[k] = 0;
		dummy = 0;
		for (int i = n / 2 - k; i < n; i++)
		{
			dummy += mask[i + 1];
			histo[k] += mask[i + 1] * m_Histogram[k - n / 2 + i];
		}
		histo[k] = histo[k] / dummy;
	}

	for (int k = 256 - n / 2; k < 256; k++)
	{
		histo[k] = 0;
		dummy = 0;
		for (int i = 0; i < 256 - k + n / 2; i++)
		{
			dummy += mask[i + 1];
			histo[k] += mask[i + 1] * m_Histogram[k - n / 2 + i];
		}
		histo[k] = histo[k] / dummy;
	}

	for (int k = 0; k < 256; k++)
		m_Histogram[k] = (unsigned int)(histo[k] + 0.5);
}

void Bmphandler::GaussianHist(float sigma)
{
	int n = int(3 * sigma);
	if (n % 2 == 0)
		n++;

	float* dummy;
	ConvoluteHist(dummy = MakeGaussfilter(sigma, n));

	free(dummy);
}

void Bmphandler::GetRange(Pair* pp)
{
	if (m_Area > 0)
	{
		auto range = std::minmax_element(m_WorkBits, m_WorkBits + m_Area);
		pp->low = *range.first;
		pp->high = *range.second;
	}
}

void Bmphandler::GetRangetissue(tissuelayers_size_t idx, tissues_size_t* pp)
{
	if (m_Area > 0)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		*pp = *std::max_element(tissues, tissues + m_Area);
	}
}

void Bmphandler::GetBmprange(Pair* pp)
{
	if (m_Area > 0)
	{
		auto range = std::minmax_element(m_BmpBits, m_BmpBits + m_Area);
		pp->low = *range.first;
		pp->high = *range.second;
	}
}

void Bmphandler::ScaleColors(Pair p)
{
	const float step = 255.0f / (p.high - p.low);

	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = (m_WorkBits[i] - p.low) * step;
}

void Bmphandler::CropColors()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = std::min(std::max(m_WorkBits[i], 0.0f), 255.0f);
}

void Bmphandler::Gaussian(float sigma)
{
	unsigned char dummymode1 = m_Mode1;
	if (sigma > 0.66f)
	{
		int n = int(3 * sigma);
		if (n % 2 == 0)
			n++;
		float* dummy;
		float* dummy1;
		Convolute(dummy = MakeGaussfilter(sigma, n), 0);

		dummy1 = m_BmpBits;
		m_BmpBits = m_Sliceprovide->GiveMe();
		SwapBmpwork();
		Convolute(dummy, 1);
		m_Sliceprovide->TakeBack(m_BmpBits);
		m_BmpBits = dummy1;

		free(dummy);
	}
	else
	{
		Bmp2work();
	}

	m_Mode1 = dummymode1;
	m_Mode2 = 1;
}

void Bmphandler::Average(unsigned short n)
{
	unsigned char dummymode1 = m_Mode1;
	float* dummy1;

	if (n % 2 == 0)
		n++;
	float* filter = (float*)malloc((n + 1) * sizeof(float));

	filter[0] = n;
	for (short unsigned int i = 1; i <= n; i++)
		filter[i] = 1.0f / n;

	Convolute(filter, 0);
	dummy1 = m_BmpBits;
	m_BmpBits = m_Sliceprovide->GiveMe();
	SwapBmpwork();
	Convolute(filter, 1);
	m_Sliceprovide->TakeBack(m_BmpBits);
	m_BmpBits = dummy1;

	free(filter);

	m_Mode1 = dummymode1;
	m_Mode2 = 1;
}

void Bmphandler::Laplacian()
{
	unsigned char dummymode1 = m_Mode1;
	float* dummy;
	float* dummy1;

	Convolute(dummy = MakeLaplacianfilter(), 0);
	dummy1 = m_BmpBits;
	m_BmpBits = m_Sliceprovide->GiveMe();
	SwapBmpwork();
	Convolute(dummy, 1);
	m_Sliceprovide->TakeBack(m_BmpBits);
	m_BmpBits = dummy1;
	//	bmp_abs();

	free(dummy);

	m_Mode1 = dummymode1;
	m_Mode2 = 1;
}

void Bmphandler::Laplacian1()
{
	unsigned char dummymode1 = m_Mode1;
	float laplacianfilter[12];
	laplacianfilter[0] = 11;
	laplacianfilter[1] = laplacianfilter[2] = 3;
	laplacianfilter[3] = laplacianfilter[5] = laplacianfilter[9] =
			laplacianfilter[11] = -1.0f / 12;
	laplacianfilter[4] = laplacianfilter[6] = laplacianfilter[8] =
			laplacianfilter[10] = -2.0f / 12;
	laplacianfilter[7] = 12.0f / 12;

	Convolute(laplacianfilter, 2);

	m_Mode1 = dummymode1;
	m_Mode2 = 1;
}

void Bmphandler::GaussSharpen(float sigma)
{
	unsigned char dummymode1 = m_Mode1;

	GaussLine(sigma);
	BmpSum();

	m_Mode1 = dummymode1;
	m_Mode2 = 1;
}

void Bmphandler::GaussLine(float sigma)
{
	unsigned char dummymode1 = m_Mode1;

	Gaussian(sigma);
	BmpDiff();

	m_Mode1 = dummymode1;
	m_Mode2 = 1;
}

void Bmphandler::MomentLine()
{
	unsigned char dummymode1 = m_Mode1;

	NMoment(3, 2);

	m_Mode1 = dummymode1;
	m_Mode2 = 2;
}

float* Bmphandler::DirectionMap(float* sobelx, float* sobely)
{
	float* direct_map = m_Sliceprovide->GiveMe();

	int i = m_Width + 1;
	for (int j = 1; j < m_Height - 1; j++)
	{
		for (int k = 1; k < m_Width - 1; k++)
		{
			if (sobelx[i] == 0)
			{
				if (sobely[i] == 0)
					direct_map[i] = 0;
				else
					direct_map[i] = 90;
			}
			else if ((sobelx[i] < 0 && sobely[i] > 0) ||
							 (sobelx[i] > 0 && sobely[i] < 0))
			{
				direct_map[i] = 180.0f - (180.0f / 3.141592f *
																		 atan(-sobely[i] / sobelx[i]));
			}
			else
				direct_map[i] =
						180.0f / 3.141592f * atan(sobely[i] / sobelx[i]);

			i++;
		}

		i += 2;
	}

	return direct_map;
}

void Bmphandler::NonmaximumSupr(float* direct_map)
{
	float* results = m_Sliceprovide->GiveMe();
	float left_bit, right_bit;
	int i = m_Width + 1;
	for (int j = 1; j < m_Height - 1; j++)
	{
		for (int k = 1; k < m_Width - 1; k++)
		{
			if (direct_map[i] < 22.5 || direct_map[i] >= 157.5)
			{
				left_bit = m_WorkBits[i - 1];
				right_bit = m_WorkBits[i + 1];
			}
			else if (direct_map[i] < 67.5)
			{
				left_bit = m_WorkBits[i - 1 - m_Width];
				right_bit = m_WorkBits[i + 1 + m_Width];
			}
			else if (direct_map[i] < 112.5)
			{
				left_bit = m_WorkBits[i + m_Width];
				right_bit = m_WorkBits[i - m_Width];
			}
			else if (direct_map[i] < 157.5)
			{
				left_bit = m_WorkBits[i + m_Width - 1];
				right_bit = m_WorkBits[i - m_Width + 1];
			}

			if (m_WorkBits[i] < left_bit || m_WorkBits[i] < right_bit)
				results[i] = 0;
			else
				results[i] = m_WorkBits[i];
			i++;
		}

		i += 2;
	}

	m_Sliceprovide->TakeBack(m_WorkBits);
	m_WorkBits = results;
}

void Bmphandler::CannyLine(float sigma, float thresh_low, float thresh_high)
{
	unsigned char dummymode1 = m_Mode1;
	float* sobelx = m_Sliceprovide->GiveMe();
	float* sobely = m_Sliceprovide->GiveMe();
	float* tmp = m_Sliceprovide->GiveMe();
	float* dummy;

	Gaussian(sigma);
	//	swap_bmphelp();
	dummy = m_BmpBits;
	m_BmpBits = tmp;
	tmp = dummy;
	SwapBmpwork();
	Sobelxy(&sobelx, &sobely);

	dummy = DirectionMap(sobelx, sobely);
	NonmaximumSupr(dummy);
	SwapBmpwork();
	Hysteretic(thresh_low, thresh_high, true, 255);

	m_Sliceprovide->TakeBack(dummy);
	m_Sliceprovide->TakeBack(sobelx);
	m_Sliceprovide->TakeBack(sobely);

	m_Sliceprovide->TakeBack(m_BmpBits);
	m_BmpBits = tmp;

	m_Mode1 = dummymode1;
	m_Mode2 = 2;
}

void Bmphandler::Hysteretic(float thresh_low, float thresh_high, bool connectivity, float set_to)
{
	unsigned char dummymode = m_Mode1;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			//			if(work_bits[i1]!=0){
			if (m_BmpBits[i1] >= thresh_high)
			{
				results[i] = set_to;
				s.push_back(i);
			}
			else if (m_BmpBits[i1] < thresh_low)
				results[i] = 0;
			else
			{
				results[i] = -1;
			}
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::Hysteretic(float thresh_low, float thresh_high, bool connectivity, float* mask, float f, float set_to)
{
	unsigned char dummymode = m_Mode1;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i1] != 0)
				results[i] = m_WorkBits[i1];
			else
			{
				if (m_BmpBits[i1] >= thresh_high)
				{
					results[i] = set_to;
					s.push_back(i);
				}
				else if (m_BmpBits[i1] >= thresh_low)
				{
					if (mask[i1] >= f)
					{
						results[i] = set_to;
						s.push_back(i);
					}
					else
						results[i] = -1;
				}
				else
					results[i] = 0;
			}

			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::DoubleHysteretic(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, float set_to)
{
	unsigned char dummymode = m_Mode1;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_BmpBits[i1] > thresh_high_h)
				results[i] = 0;
			else if (m_BmpBits[i1] > thresh_high_l)
				results[i] = -1;
			else if (m_BmpBits[i1] >= thresh_low_h)
			{
				results[i] = set_to;
				s.push_back(i);
			}
			else if (m_BmpBits[i1] >= thresh_low_l)
				results[i] = -1;
			else
				results[i] = 0;

			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::DoubleHysteretic(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, float* mask, float f, float set_to)
{
	unsigned char dummymode = m_Mode1;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i1] != 0)
				results[i] = m_WorkBits[i1];
			else
			{
				if (m_BmpBits[i1] > thresh_high_h)
					results[i] = 0;
				else if (m_BmpBits[i1] > thresh_high_l)
				{
					if (mask[i1] >= f)
					{
						results[i] = set_to;
						s.push_back(i);
					}
					else
						results[i] = -1;
				}
				else if (m_BmpBits[i1] >= thresh_low_h)
				{
					results[i] = set_to;
					s.push_back(i);
				}
				else if (m_BmpBits[i1] >= thresh_low_l)
				{
					if (mask[i1] >= f)
					{
						results[i] = set_to;
						s.push_back(i);
					}
					else
						results[i] = -1;
				}
				else
					results[i] = 0;
			}

			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::ThresholdedGrowing(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to)
{
	unsigned char dummymode = m_Mode1;
	unsigned position = Pt2coord(p);
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_BmpBits[i1] > thresh_high || m_BmpBits[i1] < thresh_low)
				results[i] = 0;
			else
				results[i] = -1;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	s.push_back(position % m_Width + 1 + (position / m_Width + 1) * (m_Width + 2));
	results[s.back()] = set_to;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	for (unsigned int i1 = 0; i1 < m_Area + 2 * m_Width + 2 * m_Height + 4; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::ThresholdedGrowing(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to, std::vector<Point>* limits1)
{
	unsigned char dummymode = m_Mode1;
	unsigned position = Pt2coord(p);
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_BmpBits[i1] > thresh_high || m_BmpBits[i1] < thresh_low)
				results[i] = 0;
			else
				results[i] = -1;
			i++;
			i1++;
		}

		i += 2;
	}

	unsigned w = (unsigned)m_Width + 2;
	for (const auto& it : *limits1)
		results[(it.py + 1) * w + it.px + 1] = 0;

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + w * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)w) * (m_Height + 1); j += w)
		results[j] = results[j + m_Width + 1] = 0;

	s.push_back(position % m_Width + 1 + (position / m_Width + 1) * (m_Width + 2));
	results[s.back()] = set_to;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	for (unsigned int i1 = 0; i1 < m_Area + 2 * m_Width + 2 * m_Height + 4; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::ThresholdedGrowinglimit(Point p, float thresh_low, float thresh_high, bool connectivity, float set_to)
{
	unsigned char dummymode = m_Mode1;
	unsigned position = Pt2coord(p);
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_BmpBits[i1] > thresh_high || m_BmpBits[i1] < thresh_low)
				results[i] = 0;
			else
				results[i] = -1;
			i++;
			i1++;
		}

		i += 2;
	}

	unsigned w = (unsigned)m_Width + 2;
	for (std::vector<std::vector<Point>>::iterator vit = m_Limits.begin();
			 vit != m_Limits.end(); vit++)
	{
		for (std::vector<Point>::iterator it = vit->begin(); it != vit->end(); it++)
			results[(it->py + 1) * w + it->px + 1] = 0;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + w * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)w) * (m_Height + 1); j += w)
		results[j] = results[j + m_Width + 1] = 0;

	s.push_back(position % m_Width + 1 + (position / m_Width + 1) * (m_Width + 2));
	results[s.back()] = set_to;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	for (unsigned int i1 = 0; i1 < m_Area + 2 * m_Width + 2 * m_Height + 4; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::ThresholdedGrowing(Point p, float threshfactor_low, float threshfactor_high, bool connectivity, float set_to, Pair* tp)
{
	unsigned char dummymode = m_Mode1;
	unsigned position = Pt2coord(p);
	float value = m_BmpBits[position];

	float thresh_low = threshfactor_low * value;
	float thresh_high = threshfactor_high * value;

	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_BmpBits[i1] > thresh_high || m_BmpBits[i1] < thresh_low)
				results[i] = 0;
			else
				results[i] = -1;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	s.push_back(position % m_Width + 1 + (position / m_Width + 1) * (m_Width + 2));
	results[s.back()] = set_to;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	for (unsigned int i1 = 0; i1 < m_Area + 2 * m_Width + 2 * m_Height + 4; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	tp->high = thresh_high;
	tp->low = thresh_low;
	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::ThresholdedGrowing(float thresh_low, float thresh_high, bool connectivity, float* mask, float f, float set_to)
{
	unsigned char dummymode = m_Mode1;
	f = f - f_tol;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i1] > 0)
				results[i] = m_WorkBits[i1];
			else
			{
				if (m_BmpBits[i1] <= thresh_high && m_BmpBits[i1] >= thresh_low)
				{
					if (mask[i1] >= f)
					{
						results[i] = set_to;
						s.push_back(i);
					}
					else
						results[i] = -1;
				}
				else
					results[i] = 0;
			}

			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, connectivity, set_to);

	//	for(unsigned int i1=0;i1<area+2*width+2*height+4;i1++) if(results[i1]==-1) results[i1]=0;

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			m_WorkBits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::HystereticGrowth(float* results, std::vector<int>* s, unsigned short w, unsigned short h, bool connectivity, float set_to)
{
	unsigned char dummymode = m_Mode1;
	int i;

	while (!(*s).empty())
	{
		i = (*s).back();
		(*s).pop_back();
		if (results[i - 1] == -1)
		{
			results[i - 1] = set_to;
			(*s).push_back(i - 1);
		}
		if (results[i + 1] == -1)
		{
			results[i + 1] = set_to;
			(*s).push_back(i + 1);
		}
		if (results[i - w] == -1)
		{
			results[i - w] = set_to;
			(*s).push_back(i - w);
		}
		if (results[i + w] == -1)
		{
			results[i + w] = set_to;
			(*s).push_back(i + w);
		}
		if (connectivity)
		{
			if (results[i - w - 1] == -1)
			{
				results[i - w - 1] = set_to;
				(*s).push_back(i - w - 1);
			}
			if (results[i + w + 1] == -1)
			{
				results[i + w + 1] = set_to;
				(*s).push_back(i + w + 1);
			}
			if (results[i - w + 1] == -1)
			{
				results[i - w + 1] = set_to;
				(*s).push_back(i - w + 1);
			}
			if (results[i + w - 1] == -1)
			{
				results[i + w - 1] = set_to;
				(*s).push_back(i + w - 1);
			}
		}
	}

	for (unsigned i1 = 0; i1 < unsigned(w) * h; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::HystereticGrowth(float* results, std::vector<int>* s, unsigned short w, unsigned short h, bool connectivity, float set_to, int nr)
{
	std::vector<int> sta;
	std::vector<int>* s1 = &sta;
	unsigned char dummymode = m_Mode1;
	int i;

	for (int i1 = 0; i1 < nr; i1++)
	{
		while (!(*s).empty())
		{
			i = (*s).back();
			(*s).pop_back();
			if (results[i - 1] == -1)
			{
				results[i - 1] = set_to;
				(*s1).push_back(i - 1);
			}
			if (results[i + 1] == -1)
			{
				results[i + 1] = set_to;
				(*s1).push_back(i + 1);
			}
			if (results[i - w] == -1)
			{
				results[i - w] = set_to;
				(*s1).push_back(i - w);
			}
			if (results[i + w] == -1)
			{
				results[i + w] = set_to;
				(*s1).push_back(i + w);
			}
			if (connectivity)
			{
				if (results[i - w - 1] == -1)
				{
					results[i - w - 1] = set_to;
					(*s1).push_back(i - w - 1);
				}
				if (results[i + w + 1] == -1)
				{
					results[i + w + 1] = set_to;
					(*s1).push_back(i + w + 1);
				}
				if (results[i - w + 1] == -1)
				{
					results[i - w + 1] = set_to;
					(*s1).push_back(i - w + 1);
				}
				if (results[i + w - 1] == -1)
				{
					results[i + w - 1] = set_to;
					(*s1).push_back(i + w - 1);
				}
			}
		}
		std::vector<int>* sdummy = s1;
		s1 = s;
		s = sdummy;
	}

	while (!(*s).empty())
	{
		(*s).pop_back();
	}

	for (unsigned i1 = 0; i1 < unsigned(w) * h; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::Sobel()
{
	unsigned char dummymode = m_Mode1;
	float* tmp = m_Sliceprovide->GiveMe();
	float* dummy;
	float mask1[4];
	float mask2[4];
	mask1[0] = mask2[0] = 3;
	mask1[1] = mask1[3] = mask2[3] = 1;
	mask1[2] = 2;
	mask2[2] = 0;
	mask2[1] = -1;

	Convolute(mask1, 1);
	dummy = tmp;
	tmp = m_BmpBits;
	m_BmpBits = dummy;
	SwapBmpwork();
	Convolute(mask2, 0);
	BmpAbs();
	m_WorkBits = m_BmpBits; // work_bits is already saved in dummy
	m_BmpBits = tmp;
	Convolute(mask1, 0);
	tmp = m_BmpBits;
	m_BmpBits = m_WorkBits;
	m_WorkBits = m_Sliceprovide->GiveMe();
	Convolute(mask2, 1);
	BmpAbs();
	m_Sliceprovide->TakeBack(m_BmpBits);
	m_BmpBits = dummy;
	BmpSum();
	m_Sliceprovide->TakeBack(m_BmpBits);
	m_BmpBits = tmp;

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::SobelFiner()
{
	unsigned char dummymode = m_Mode1;
	float* tmp = m_Sliceprovide->GiveMe();
	float* dummy;
	float mask1[4];
	float mask2[4];
	mask1[0] = mask2[0] = 3;
	mask1[1] = mask1[3] = mask2[3] = 1;
	mask1[2] = 2;
	mask2[2] = 0;
	mask2[1] = -1;

	Convolute(mask1, 1);
	dummy = tmp;
	tmp = m_BmpBits;
	m_BmpBits = dummy;
	SwapBmpwork();
	Convolute(mask2, 0);
	BmpAbs();
	m_WorkBits = m_BmpBits; // work_bits is already saved in dummy
	m_BmpBits = tmp;
	Convolute(mask1, 0);
	tmp = m_BmpBits;
	m_BmpBits = m_WorkBits;
	m_WorkBits = m_Sliceprovide->GiveMe();
	Convolute(mask2, 1);
	BmpAbs();
	m_Sliceprovide->TakeBack(m_BmpBits);
	m_BmpBits = dummy;
	for (unsigned i = 0; i < m_Area; i++)
		m_WorkBits[i] =
				sqrt(m_WorkBits[i] * m_WorkBits[i] + m_BmpBits[i] * m_BmpBits[i]);
	m_Sliceprovide->TakeBack(m_BmpBits);
	m_BmpBits = tmp;

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::MedianInterquartile(bool median)
{
	unsigned char dummymode = m_Mode1;
	std::vector<float> fvec;

	unsigned k = 0;

	for (unsigned short i = 0; i < m_Height - 2; i++)
	{
		for (unsigned short j = 0; j < m_Width - 2; j++)
		{
			fvec.clear();
			for (unsigned short l = 0; l < 3; l++)
			{
				for (unsigned short m = 0; m < 3; m++)
				{
					fvec.push_back(m_BmpBits[k + l + m * m_Width]);
				}
			}

			std::sort(fvec.begin(), fvec.end());
			if (median)
				m_WorkBits[k + 1 + m_Width] = fvec[4];
			else
				m_WorkBits[k + 1 + m_Width] = fvec[7] - fvec[1];
			k++;
		}
		k += 2;
	}

	if (median)
	{
		for (unsigned k = 0; k < m_Width; k++)
			m_WorkBits[k] = m_BmpBits[k];
		for (unsigned k = m_Area - m_Width; k < m_Area; k++)
			m_WorkBits[k] = m_BmpBits[k];
		for (unsigned k = 0; k < m_Area; k += m_Width)
			m_WorkBits[k] = m_BmpBits[k];
		for (unsigned k = m_Width - 1; k < m_Area; k += m_Width)
			m_WorkBits[k] = m_BmpBits[k];
	}
	else
	{
		for (unsigned k = 0; k < m_Width; k++)
			m_WorkBits[k] = 0;
		for (unsigned k = m_Area - m_Width; k < m_Area; k++)
			m_WorkBits[k] = 0;
		for (unsigned k = 0; k < m_Area; k += m_Width)
			m_WorkBits[k] = 0;
		for (unsigned k = m_Width - 1; k < m_Area; k += m_Width)
			m_WorkBits[k] = 0;
	}

	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

void Bmphandler::MedianInterquartile(float* median, float* iq)
{
	unsigned char dummymode = m_Mode1;
	std::vector<float> fvec;

	unsigned k = 0;

	for (unsigned short i = 0; i < m_Height - 2; i++)
	{
		for (unsigned short j = 0; j < m_Width - 2; j++)
		{
			fvec.clear();
			for (unsigned short l = 0; l < 3; l++)
			{
				for (unsigned short m = 0; m < 3; m++)
				{
					fvec.push_back(m_BmpBits[k + l + m * m_Width]);
				}
			}

			std::sort(fvec.begin(), fvec.end());
			median[k + 1 + m_Width] = fvec[4];
			iq[k + 1 + m_Width] = fvec[7] - fvec[1];
			k++;
		}
		k += 2;
	}

	for (unsigned k = 0; k < m_Width; k++)
		median[k] = m_BmpBits[k];
	for (unsigned k = m_Area - m_Width; k < m_Area; k++)
		median[k] = m_BmpBits[k];
	for (unsigned k = 0; k < m_Area; k += m_Width)
		median[k] = m_BmpBits[k];
	for (unsigned k = m_Width - 1; k < m_Area; k += m_Width)
		median[k] = m_BmpBits[k];
	for (unsigned k = 0; k < m_Width; k++)
		iq[k] = 0;
	for (unsigned k = m_Area - m_Width; k < m_Area; k++)
		iq[k] = 0;
	for (unsigned k = 0; k < m_Area; k += m_Width)
		iq[k] = 0;
	for (unsigned k = m_Width - 1; k < m_Area; k += m_Width)
		iq[k] = 0;

	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

void Bmphandler::Sigmafilter(float sigma, unsigned short nx, unsigned short ny)
{
	unsigned char dummymode = m_Mode1;
	if (nx % 2 == 0)
		nx++;
	if (ny % 2 == 0)
		ny++;

	unsigned short counter;
	float summa;
	float dummy;

	unsigned i = 0;

	for (int j = 0; j <= m_Height - ny; j++)
	{
		for (int k = 0; k <= m_Width - nx; k++)
		{
			summa = 0;
			counter = 0;
			dummy = m_BmpBits[i + nx / 2 + (ny / 2) * m_Width];
			for (int l = 0; l < nx; l++)
			{
				for (int o = 0; o < ny; o++)
				{
					if (m_BmpBits[i + l + o * m_Width] < dummy + sigma &&
							m_BmpBits[i + l + o * m_Width] > dummy - sigma)
					{
						summa += m_BmpBits[i + l + o * m_Width];
						counter++;
					}
				}
			}
			m_WorkBits[i + nx / 2 + (ny / 2) * m_Width] = summa / counter;
			i++;
		}
		i = i + nx - 1;
	}

	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

void Bmphandler::Sobelxy(float** sobelx, float** sobely)
{
	float mask1[4];
	float mask2[4];
	mask1[0] = mask2[0] = 3;
	mask1[1] = mask1[3] = mask2[3] = 1;
	mask1[2] = 2;
	mask2[2] = 0;
	mask2[1] = -1;

	Convolute(mask1, 1);
	float* tmp = m_BmpBits;
	m_BmpBits = m_WorkBits;
	m_WorkBits = *sobelx;
	Convolute(mask2, 0);
	m_WorkBits = m_BmpBits;
	m_BmpBits = tmp;
	Convolute(mask1, 0);
	tmp = m_BmpBits;
	m_BmpBits = m_WorkBits;
	m_WorkBits = *sobely;
	Convolute(mask2, 1);
	m_WorkBits = m_BmpBits;
	m_BmpBits = tmp;

	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] = abs((*sobelx)[i]) + abs((*sobely)[i]);
}

void Bmphandler::Compacthist()
{
	float histmap[256];
	histmap[0] = 0;
	float dummy = 0;

	for (int i = 1; i < 255; i++)
	{
		if (m_Histogram[i] == 0 && m_Histogram[i - 1] != 0 && m_Histogram[i + 1] != 0)
			dummy += 1;
		else
			histmap[i] = dummy;
	}
	histmap[255] = dummy;

	for (unsigned int i = 0; i < m_Area; i++)
		m_WorkBits[i] -= histmap[std::min((int)m_WorkBits[i], 255)];
}

float* Bmphandler::FindModal(unsigned int thresh1, float thresh2)
{
	int n = 0;

	std::list<int> threshes;

	unsigned int temp_hist;
	bool pushable = false;

	int lastmin;
	unsigned int lastmax_h, lastmin_h;
	lastmin = 0;
	lastmax_h = lastmin_h = m_Histogram[0];
	for (int i = 1; i < 256; i++)
	{
		temp_hist = m_Histogram[i];

		if (pushable)
		{
			if (temp_hist < lastmin_h)
			{
				lastmin_h = temp_hist;
				lastmin = i;
			}
			if ((float)temp_hist > float(lastmin_h) / thresh2 &&
					temp_hist >= thresh1)
			{
				threshes.push_back(lastmin);
				n++;
				lastmax_h = temp_hist;
				pushable = false;
			}
		}
		else
		{
			if (temp_hist > lastmax_h)
			{
				lastmax_h = temp_hist;
			}
			if ((float)temp_hist < lastmax_h * thresh2)
			{
				lastmin_h = temp_hist;
				lastmin = i;
				pushable = true;
			}
		}
	}

	n++;
	float* thresholds = (float*)malloc(n * sizeof(float));
	thresholds[0] = float(n - 1);

	std::list<int>::iterator it = threshes.begin();
	for (int i = 1; i < n; i++)
	{
		thresholds[i] = (float)*it;
		it++;
	}

	return thresholds;
}

void Bmphandler::Subthreshold(int n1, int n2, unsigned int thresh1, float thresh2, float sigma)
{
	unsigned char dummymode = m_Mode1;
	int dx = (m_Width + n1 - 1) / n1;
	int dy = (m_Height + n2 - 1) / n2;
	//int dx1,dy1;

	float* f_p;

	Point p;

	sigma = std::max(0.1f, sigma);

	for (int i = 0; i < n1; i++)
	{
		for (int j = 0; j < n2; j++)
		{
			p.px = i * dx;
			p.py = j * dy;

			SwapBmpwork();
			MakeHistogram(p, dx, dy, true);
			SwapBmpwork();
			GaussianHist(sigma);
			Threshold(f_p = FindModal(thresh1, thresh2), p, dx, dy);
		}
	}

	free(f_p);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::Erosion1(int n, bool connectivity) // true for 8-, false for 4-connectivity
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	float* results = m_Sliceprovide->GiveMe();
	float* dummy;

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < m_Area; i++)
			results[i] = m_WorkBits[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < m_Width; j++)
			{
				if (m_WorkBits[i1] != m_WorkBits[i1 + m_Width])
				{
					if (m_WorkBits[i1] == 0)
						results[i1 + m_Width] = 0;
					else if (m_WorkBits[i1 + m_Width] == 0)
						results[i1] = 0;
				}

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
			{
				if (m_WorkBits[i1] != m_WorkBits[i1 + 1])
				{
					if (m_WorkBits[i1] == 0)
						results[i1 + 1] = 0;
					else if (m_WorkBits[i1 + 1] == 0)
						results[i1] = 0;
				}

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (m_Height - 1); i++)
			{
				for (unsigned short j = 0; j < (m_Width - 1); j++)
				{
					if (m_WorkBits[i1] != m_WorkBits[i1 + m_Width + 1])
					{
						if (m_WorkBits[i1] == 0)
							results[i1 + 1 + m_Width] = 0;
						else if (m_WorkBits[i1 + 1 + m_Width] == 0)
							results[i1] = 0;
					}
					if (m_WorkBits[i1 + 1] != m_WorkBits[i1 + m_Width])
					{
						if (m_WorkBits[i1 + 1] == 0)
							results[i1 + m_Width] = 0;
						else if (m_WorkBits[i1 + m_Width] == 0)
							results[i1 + 1] = 0;
					}

					i1++;
				}
				i1++;
			}
		}

		dummy = results;
		results = m_WorkBits;
		m_WorkBits = dummy;
	}

	m_Sliceprovide->TakeBack(results);

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

void Bmphandler::Erosion(int n, bool connectivity) // true for 8-, false for 4-connectivity
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;
	float* results = m_Sliceprovide->GiveMe();
	float* dummy;

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < m_Area; i++)
			results[i] = m_WorkBits[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < m_Width; j++)
			{
				if (m_WorkBits[i1] != m_WorkBits[i1 + m_Width])
					results[i1] = results[i1 + m_Width] = 0;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
			{
				if (m_WorkBits[i1] != m_WorkBits[i1 + 1])
					results[i1] = results[i1 + 1] = 0;

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (m_Height - 1); i++)
			{
				for (unsigned short j = 0; j < (m_Width - 1); j++)
				{
					if (m_WorkBits[i1] != m_WorkBits[i1 + m_Width + 1])
						results[i1] = results[i1 + m_Width + 1] = 0;
					if (m_WorkBits[i1 + 1] != m_WorkBits[i1 + m_Width])
						results[i1 + 1] = results[i1 + m_Width] = 0;

					i1++;
				}
				i1++;
			}
		}

		dummy = results;
		results = m_WorkBits;
		m_WorkBits = dummy;
	}

	m_Sliceprovide->TakeBack(results);

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

void Bmphandler::Dilation(int n, bool connectivity) // true for 8-, false for 4-connectivity
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	float* results = m_Sliceprovide->GiveMe();
	float* dummy;

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < m_Area; i++)
			results[i] = m_WorkBits[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < m_Width; j++)
			{
				if (m_WorkBits[i1] != m_WorkBits[i1 + m_Width])
				{
					if (m_WorkBits[i1] == 0)
						results[i1] = results[i1 + m_Width];
					if (m_WorkBits[i1 + m_Width] == 0)
						results[i1 + m_Width] = results[i1];
				}

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
			{
				if (m_WorkBits[i1] != m_WorkBits[i1 + 1])
				{
					if (m_WorkBits[i1] == 0)
						results[i1] = results[i1 + 1];
					if (m_WorkBits[i1 + 1] == 0)
						results[i1 + 1] = results[i1];
				}

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (m_Height - 1); i++)
			{
				for (unsigned short j = 0; j < (m_Width - 1); j++)
				{
					if (m_WorkBits[i1] != m_WorkBits[i1 + m_Width + 1])
					{
						if (m_WorkBits[i1] == 0)
							results[i1] = results[i1 + m_Width + 1];
						if (m_WorkBits[i1 + m_Width + 1] == 0)
							results[i1 + m_Width + 1] = results[i1];
					}
					if (m_WorkBits[i1 + 1] != m_WorkBits[i1 + m_Width])
					{
						if (m_WorkBits[i1 + 1] == 0)
							results[i1 + 1] = results[i1 + m_Width];
						if (m_WorkBits[i1 + m_Width] == 0)
							results[i1 + m_Width] = results[i1 + 1];
					}

					i1++;
				}
				i1++;
			}
		}

		dummy = results;
		results = m_WorkBits;
		m_WorkBits = dummy;
	}

	m_Sliceprovide->TakeBack(results);

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

void Bmphandler::Closure(int n, bool connectivity)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	Dilation(n, connectivity);
	Erosion1(n, connectivity);

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

void Bmphandler::Open(int n, bool connectivity)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	Erosion(n, connectivity);
	Dilation(n, connectivity);

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

void Bmphandler::MarkBorder(bool connectivity) // true for 8-, false for 4-connectivity
{
	float* results = m_Sliceprovide->GiveMe();

	for (unsigned int i = 0; i < m_Area; i++)
		results[i] = m_WorkBits[i];

	int i1 = 0;

	for (unsigned short i = 0; i < (m_Height - 1); i++)
	{
		for (unsigned short j = 0; j < m_Width; j++)
		{
			if (m_WorkBits[i1] > m_WorkBits[i1 + m_Width])
				results[i1] = -1;
			else if (m_WorkBits[i1] < m_WorkBits[i1 + m_Width])
				results[i1 + m_Width] = -1;

			i1++;
		}
	}

	i1 = 0;

	for (unsigned short i = 0; i < m_Height; i++)
	{
		for (unsigned short j = 0; j < (m_Width - 1); j++)
		{
			if (m_WorkBits[i1] > m_WorkBits[i1 + 1])
				results[i1] = -1;
			else if (m_WorkBits[i1] < m_WorkBits[i1 + 1])
				results[i1 + 1] = -1;

			i1++;
		}
		i1++;
	}

	if (connectivity)
	{
		i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
			{
				if (m_WorkBits[i1] > m_WorkBits[i1 + m_Width + 1])
					results[i1] = -1;
				else if (m_WorkBits[i1] < m_WorkBits[i1 + m_Width + 1])
					results[i1 + m_Width + 1] = -1;
				if (m_WorkBits[i1 + 1] > m_WorkBits[i1 + m_Width])
					results[i1 + 1] = -1;
				else if (m_WorkBits[i1 + 1] < m_WorkBits[i1 + m_Width])
					results[i1 + m_Width] = -1;

				i1++;
			}
			i1++;
		}
	}

	m_Sliceprovide->TakeBack(m_WorkBits);
	m_WorkBits = results;
}

void Bmphandler::ZeroCrossings(bool connectivity) // true for 8-, false for 4-connectivity
{
	float* results = m_Sliceprovide->GiveMe();

	for (unsigned int i = 0; i < m_Area; i++)
		results[i] = 255;

	int i1 = 0;

	for (unsigned short i = 0; i < (m_Height - 1); i++)
	{
		for (unsigned short j = 0; j < m_Width; j++)
		{
			if (m_WorkBits[i1] * m_WorkBits[i1 + m_Width] <= 0)
			{
				if (m_WorkBits[i1] > 0)
					results[i1 + m_Width] = -1;
				else if (m_WorkBits[i1 + m_Width] > 0)
					results[i1] = -1;
			}
			i1++;
		}
	}

	i1 = 0;

	for (unsigned short i = 0; i < m_Height; i++)
	{
		for (unsigned short j = 0; j < (m_Width - 1); j++)
		{
			if (m_WorkBits[i1] * m_WorkBits[i1 + 1] <= 0)
			{
				if (m_WorkBits[i1] > 0)
					results[i1 + 1] = -1;
				else if (m_WorkBits[i1 + 1] > 0)
					results[i1] = -1;
			}
			i1++;
		}
		i1++;
	}

	if (connectivity)
	{
		i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
			{
				if (m_WorkBits[i1] * m_WorkBits[i1 + m_Width + 1] <= 0)
				{
					if (m_WorkBits[i1] > 0)
						results[i1 + m_Width + 1] = -1;
					else if (m_WorkBits[i1 + m_Width + 1] > 0)
						results[i1] = -1;
				}
				if (m_WorkBits[i1 + 1] * m_WorkBits[i1 + m_Width] <= 0)
				{
					if (m_WorkBits[i1 + 1] > 0)
						results[i1 + m_Width] = -1;
					else if (m_WorkBits[i1 + m_Width] > 0)
						results[i1 + 1] = -1;
				}
				i1++;
			}
			i1++;
		}
	}

	m_Sliceprovide->TakeBack(m_WorkBits);
	m_WorkBits = results;

	m_Mode2 = 2;
}

void Bmphandler::ZeroCrossings(float thresh, bool connectivity) // true for 8-, false for 4-connectivity
{
	float* results = m_Sliceprovide->GiveMe();

	for (unsigned int i = 0; i < m_Area; i++)
		results[i] = 255;

	int i1 = 0;

	for (unsigned short i = 0; i < (m_Height - 1); i++)
	{
		for (unsigned short j = 0; j < m_Width; j++)
		{
			if (m_WorkBits[i1] * m_WorkBits[i1 + m_Width] < 0 &&
					abs(m_WorkBits[i1] - m_WorkBits[i1 + m_Width]) > thresh)
			{
				if (m_WorkBits[i1] > 0)
					results[i1] = -1;
				else
					results[i1 + m_Width] = -1;
			}
			i1++;
		}
	}

	i1 = 0;

	for (unsigned short i = 0; i < m_Height; i++)
	{
		for (unsigned short j = 0; j < (m_Width - 1); j++)
		{
			if (m_WorkBits[i1] * m_WorkBits[i1 + 1] < 0 &&
					abs(m_WorkBits[i1] - m_WorkBits[i1 + m_Width]) > thresh)
			{
				if (m_WorkBits[i1] > 0)
					results[i1] = -1;
				else
					results[i1 + 1] = -1;
			}
			i1++;
		}
		i1++;
	}

	if (connectivity)
	{
		i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
			{
				if (m_WorkBits[i1] * m_WorkBits[i1 + m_Width + 1] < 0 &&
						abs(m_WorkBits[i1] - m_WorkBits[i1 + m_Width]) > thresh)
				{
					if (m_WorkBits[i1] > 0)
						results[i1] = -1;
					else
						results[i1 + m_Width + 1] = -1;
				}
				if (m_WorkBits[i1 + 1] * m_WorkBits[i1 + m_Width] < 0 &&
						abs(m_WorkBits[i1] - m_WorkBits[i1 + m_Width]) > thresh)
				{
					if (m_WorkBits[i1 + 1] > 0)
						results[i1 + 1] = -1;
					else
						results[i1 + m_Width] = -1;
				}
				i1++;
			}
			i1++;
		}
	}

	m_Sliceprovide->TakeBack(m_WorkBits);
	m_WorkBits = results;

	m_Mode2 = 2;
}

void Bmphandler::LaplacianZero(float sigma, float thresh, bool connectivity)
{
	unsigned char dummymode = m_Mode1;

	float* tmp1;
	float* tmp2;

	Gaussian(sigma);
	tmp1 = m_BmpBits;
	m_BmpBits = m_WorkBits;
	m_WorkBits = m_Sliceprovide->GiveMe();
	//	swap_bmpwork();
	Sobel();
	tmp2 = m_WorkBits;
	m_WorkBits = m_Sliceprovide->GiveMe();
	//	swap_workhelp();
	Laplacian1();

	//	for(unsigned i=0;i<area;i++) tmp2[i]-=thresh*work_bits[i];

	ZeroCrossings(connectivity);

	for (unsigned i = 0; i < m_Area; i++)
	{
		if (m_WorkBits[i] == -1 && tmp2[i] < thresh)
			m_WorkBits[i] = 255;
		//		work_bits[i]=tmp2[i];
	}

	m_Sliceprovide->TakeBack(m_BmpBits);
	m_BmpBits = tmp1;
	m_Sliceprovide->TakeBack(tmp2);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::NMoment(short unsigned n, short unsigned p)
{
	unsigned char dummymode = m_Mode1;

	float* results = m_Sliceprovide->GiveMe();
	if (n % 2 == 0)
		n++;

	Average(n);

	for (unsigned int i = 0; i < m_Area; i++)
		results[i] = 0;

	for (unsigned short i = 0; i <= m_Height - n; i++)
	{
		for (unsigned short j = 0; j <= m_Width - n; j++)
		{
			for (unsigned short k = 0; k < n; k++)
			{
				for (unsigned short l = 0; l < n; l++)
				{
					results[(i + n / 2) * m_Width + j + n / 2] +=
							pow(abs(m_BmpBits[(i + k) * m_Width + j + l] -
											m_WorkBits[(i + n / 2) * m_Width + j + n / 2]),
									p);
				}
			}
			results[(i + n / 2) * m_Width + j + n / 2] =
					results[(i + n / 2) * m_Width + j + n / 2] / (n * n);
		}
	}

	m_Sliceprovide->TakeBack(m_WorkBits);
	m_WorkBits = results;

	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

void Bmphandler::NMomentSigma(short unsigned n, short unsigned p, float sigma)
{
	unsigned char dummymode = m_Mode1;
	float* results = m_Sliceprovide->GiveMe();
	if (n % 2 == 0)
		n++;

	Sigmafilter(sigma, n, n);

	unsigned count;
	float dummy;

	for (unsigned int i = 0; i < m_Area; i++)
		results[i] = 0;

	for (unsigned short i = 0; i <= m_Height - n; i++)
	{
		for (unsigned short j = 0; j <= m_Width - n; j++)
		{
			count = 0;
			for (unsigned short k = 0; k < n; k++)
			{
				for (unsigned short l = 0; l < n; l++)
				{
					dummy = abs(m_BmpBits[(i + k) * m_Width + j + l] -
											m_WorkBits[(i + n / 2) * m_Width + j + n / 2]);
					if (dummy < sigma)
					{
						count++;
						results[(i + n / 2) * m_Width + j + n / 2] +=
								pow(dummy, p);
					}
				}
			}
			results[(i + n / 2) * m_Width + j + n / 2] =
					results[(i + n / 2) * m_Width + j + n / 2] / (count);
		}
	}

	m_Sliceprovide->TakeBack(m_WorkBits);
	m_WorkBits = results;

	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

void Bmphandler::AnisoDiff(float dt, int n, float (*f)(float, float), float k, float restraint)
{
	unsigned char dummymode = m_Mode1;
	Bmp2work();
	ContAnisodiff(dt, n, f, k, restraint);
	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

void Bmphandler::ContAnisodiff(float dt, int n, float (*f)(float, float), float k, float restraint)
{
	float* flowx = m_Sliceprovide->GiveMe(); // only area-height of it is used
	float* flowy = m_Sliceprovide->GiveMe(); // only area-width of it is used

	float dummy;

	for (int l = 0; l < n; l++)
	{
		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < m_Width - 1; j++)
			{
				dummy =
						(m_WorkBits[i * m_Width + j + 1] - m_WorkBits[i * m_Width + j]);
				flowx[i * (m_Width - 1) + j] = f(dummy, k) * dummy;
			}
		}

		for (unsigned short i = 0; i < m_Height - 1; i++)
		{
			for (unsigned short j = 0; j < m_Width; j++)
			{
				dummy =
						(m_WorkBits[(i + 1) * m_Width + j] - m_WorkBits[i * m_Width + j]);
				flowy[i * m_Width + j] = f(dummy, k) * dummy;
			}
		}

		for (unsigned int i = 0; i < m_Area; i++)
			m_WorkBits[i] += dt * restraint * (m_BmpBits[i] - m_WorkBits[i]);

		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < m_Width - 1; j++)
			{
				m_WorkBits[i * m_Width + j] += dt * flowx[i * (m_Width - 1) + j];
				m_WorkBits[i * m_Width + j + 1] -= dt * flowx[i * (m_Width - 1) + j];
			}
		}

		for (unsigned short i = 0; i < m_Height - 1; i++)
		{
			for (unsigned short j = 0; j < m_Width; j++)
			{
				m_WorkBits[i * m_Width + j] += dt * flowy[i * m_Width + j];
				m_WorkBits[(i + 1) * m_Width + j] -= dt * flowy[i * m_Width + j];
			}
		}
	}

	m_Sliceprovide->TakeBack(flowx);
	m_Sliceprovide->TakeBack(flowy);

	m_Mode2 = 1;
}

void Bmphandler::ToBmpgrey(float* p_bits) const
{
	for (unsigned int i = 0; i < m_Area; i++)
	{
		p_bits[i] = floor(std::min(255.0f, std::max(0.0f, p_bits[i] + 0.5f)));
	}
	}
void Bmphandler::Bucketsort(std::vector<unsigned int>* sorted, float* p_bits) const
{
	for (unsigned int i = 0; i < m_Area; i++)
	{
		sorted[(unsigned char)floor(p_bits[i] + 0.5f)].push_back(i);
	}
}

unsigned* Bmphandler::Watershed(bool connectivity)
{
	m_Wshedobj.m_B.clear();
	m_Wshedobj.m_M.clear();
	m_Wshedobj.m_Marks.clear();

	unsigned p, minbase, minbase_nr;
	unsigned basin_nr = 0;
	unsigned* y = (unsigned*)malloc(sizeof(unsigned) * m_Area);
	for (unsigned i = 0; i < m_Area; i++)
		y[i] = unvisited;

	std::vector<unsigned> sorted[256];
	std::vector<unsigned>::iterator it, it1;
	std::vector<unsigned> bp;

	Basin b;
	b.m_L = 0;
	MergeEvent m;

	m_BmpIsGrey = false; //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	if (!m_BmpIsGrey)
	{
		SwapBmpwork();
		Pair p1;
		GetRange(&p1);
		ScaleColors(p1);
		SwapBmpwork();
	}

	Bucketsort(sorted, m_BmpBits);

	for (unsigned i = 0; i < 256; i++)
	{
		for (it = sorted[i].begin(); it != sorted[i].end(); it++)
		{
			p = *it;
			bp.clear();

			if (p % m_Width != 0 && y[p - 1] != unvisited)
				bp.push_back(y[p - 1]);
			if ((p + 1) % m_Width != 0 && y[p + 1] != unvisited)
				bp.push_back(y[p + 1]);
			if (p >= m_Width && y[p - m_Width] != unvisited)
				bp.push_back(y[p - m_Width]);
			if ((p + m_Width) < m_Area && y[p + m_Width] != unvisited)
				bp.push_back(y[p + m_Width]);
			if (connectivity)
			{
				if (p % m_Width != 0 && p >= m_Width &&
						y[p - 1 - m_Width] != unvisited)
					bp.push_back(y[p - 1 - m_Width]);
				if (p % m_Width != 0 && (p + m_Width) < m_Area &&
						y[p - 1 + m_Width] != unvisited)
					bp.push_back(y[p + m_Width - 1]);
				if ((p + 1) % m_Width != 0 && p >= m_Width &&
						y[p - m_Width + 1] != unvisited)
					bp.push_back(y[p - m_Width + 1]);
				if ((p + 1) % m_Width != 0 && (p + m_Width) < m_Area &&
						y[p + m_Width + 1] != unvisited)
					bp.push_back(y[p + m_Width + 1]);
			}

			if (bp.empty())
			{
				b.m_G = i;
				b.m_R = basin_nr;
				m_Wshedobj.m_B.push_back(b);
				y[p] = basin_nr;
				basin_nr++;
			}
			else
			{
				minbase_nr = bp[0];
				minbase = DeepestConBas(bp[0]);
				for (unsigned j = 1; j < bp.size(); j++)
					if (DeepestConBas(bp[j]) < minbase)
						minbase_nr = bp[j];
				y[p] = minbase_nr;

				for (it1 = bp.begin(); it1 != bp.end(); it1++)
				{
					if (DeepestConBas(*it1) != DeepestConBas(minbase_nr))
					{
						m_Wshedobj.m_B[DeepestConBas(*it1)].m_R =
								DeepestConBas(minbase_nr);
						m.m_K = *it1;
						m.m_A = minbase_nr;
						m.m_G = i;
						m_Wshedobj.m_M.push_back(m);
					}
				}
			}
		}
	}

	return y;
}

unsigned* Bmphandler::WatershedSobel(bool connectivity)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	unsigned i = PushstackWork();
	Sobel();
	SwapBmpwork();
	unsigned* usp = Watershed(connectivity);
	SwapBmpwork();
	GetstackWork(i);
	Removestack(i);
	//	popstack_work();

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;

	return usp;
}

void Bmphandler::ConstructRegions(unsigned h, unsigned* wshed)
{
	unsigned char dummymode1 = m_Mode1;
	//unsigned char dummymode2=mode2;

	for (unsigned i = 0; i < m_Wshedobj.m_B.size(); i++)
	{
		m_Wshedobj.m_B[i].m_L = 0;
		m_Wshedobj.m_B[i].m_R = i;
	}

	//	unsigned i1=1;
	//	for(std::vector<mark>::iterator it=marks.begin();it!=marks.end();it++) {
	//		set_marker(*it,i1,wshed);
	SetMarker(wshed);
	//		i1++;
	//	}

	//	cout << wshed[126167]<<" "<<wshedobj.B[wshed[126167]].l<<endl;

	for (unsigned i = 0; i < m_Wshedobj.m_M.size(); i++)
	{
		CondMerge(i, h);
	}

	Labels2work(wshed, (unsigned)m_Marks.size());

	m_Mode1 = dummymode1;
	m_Mode2 = 2;
}

void Bmphandler::SetMarker(unsigned* wshed)
{
	for (std::vector<Mark>::iterator it = m_Marks.begin(); it != m_Marks.end(); it++)
	{
		m_Wshedobj.m_B[wshed[Pt2coord((*it).p)]].m_L = (*it).mark;
	}
}

void Bmphandler::AddMark(Point p, unsigned label, std::string str)
{
	Mark m;
	m.p.px = p.px;
	m.p.py = p.py;
	m.mark = label;
	m.name = str;

	m_Marks.push_back(m);
}

bool Bmphandler::RemoveMark(Point p, unsigned radius)
{
	radius = radius * radius;

	std::vector<Mark>::iterator it = m_Marks.begin();
	while (it != m_Marks.end() &&
				 (unsigned int)((it->p.px - p.px) * (it->p.px - p.px) +
												(it->p.py - p.py) * (it->p.py - p.py)) > radius)
		it++;

	if (it != m_Marks.end())
	{
		m_Marks.erase(it);
		return true;
	}
	else
		return false;
}

void Bmphandler::ClearMarks()
{
	m_Marks.clear();
}

unsigned Bmphandler::DeepestConBas(unsigned k)
{
	if (m_Wshedobj.m_B[k].m_R != k)
		return DeepestConBas(m_Wshedobj.m_B[k].m_R);
	else
		return k;
}

void Bmphandler::CondMerge(unsigned m, unsigned h)
{
	unsigned k, a;
	k = DeepestConBas(m_Wshedobj.m_M[m].m_K);
	a = DeepestConBas(m_Wshedobj.m_M[m].m_A);
	if (h >= (m_Wshedobj.m_M[m].m_G - m_Wshedobj.m_B[k].m_G))
	{
		if (m_Wshedobj.m_B[a].m_L == 0 || m_Wshedobj.m_B[k].m_L == 0 ||
				m_Wshedobj.m_B[a].m_L == m_Wshedobj.m_B[k].m_L)
		{
			m_Wshedobj.m_B[k].m_R = a;
		}
		if (m_Wshedobj.m_B[a].m_L == 0)
		{
			m_Wshedobj.m_B[a].m_L = m_Wshedobj.m_B[k].m_L;
		}
	}
}

void Bmphandler::Wshed2work(unsigned* Y)
{
	float d = 255.0f / m_Wshedobj.m_B.size();
	for (unsigned i = 0; i < m_Area; i++)
		m_WorkBits[i] = Y[i] * d;

	m_Mode2 = 2;
}

void Bmphandler::Labels2work(unsigned* Y, unsigned /* lnr */)
{
	unsigned int maxim = 1;
	for (std::vector<Mark>::iterator it = m_Marks.begin(); it != m_Marks.end(); it++)
	{
		maxim = std::max(maxim, it->mark);
	}

	float d = 255.0f / maxim;
	for (unsigned i = 0; i < m_Area; i++)
		m_WorkBits[i] = d * LabelLookup(i, Y);

	m_Mode2 = 2;
}

unsigned Bmphandler::LabelLookup(unsigned i, unsigned* wshed)
{
	unsigned k = wshed[i];
	if (m_Wshedobj.m_B[k].m_L == 0 && m_Wshedobj.m_B[k].m_R != k)
		m_Wshedobj.m_B[k].m_L = m_Wshedobj.m_B[DeepestConBas(k)].m_L;
	return m_Wshedobj.m_B[k].m_L;
}

void Bmphandler::LoadLine(std::vector<Point>* vec_pt)
{
	m_Contour.Clear();
	m_Contour.AddPoints(vec_pt);
}

void Bmphandler::PresimplifyLine(float d)
{
	m_Contour.Presimplify(d, true);
}

void Bmphandler::DougpeuckLine(float epsilon)
{
	m_Contour.DougPeuck(epsilon, true);
}

void Bmphandler::PlotLine() // very temporary solution...
{
	std::vector<Point> p_vec;

	//	contour.doug_peuck(3,true);

	m_Contour.ReturnContour(&p_vec);
	m_Contour.PrintContour();

	unsigned dx, dy;
	Point p;

	unsigned n = (unsigned)m_Contour.ReturnN();

	for (unsigned i = 0; i < n - 1; i++)
	{
		dx = std::max(p_vec[i].px, p_vec[i + 1].px) -
				 std::min(p_vec[i].px, p_vec[i + 1].px);
		dy = std::max(p_vec[i].py, p_vec[i + 1].py) -
				 std::min(p_vec[i].py, p_vec[i + 1].py);
		p.px = p_vec[i].px;
		p.py = p_vec[i].py;
		m_WorkBits[Pt2coord(p)] = 0;
		if (dx > dy)
		{
			for (unsigned j = 1; j < dx; j++)
			{
				p.px = short(0.5 +
										 ((float)p_vec[i + 1].px - (float)p_vec[i].px) * j /
												 dx +
										 (float)p_vec[i].px);
				p.py = short(0.5 +
										 ((float)p_vec[i + 1].py - (float)p_vec[i].py) * j /
												 dx +
										 (float)p_vec[i].py);
				m_WorkBits[Pt2coord(p)] = 255;
			}
		}
		else
		{
			for (unsigned j = 1; j < dy; j++)
			{
				p.px = short(0.5 +
										 ((float)p_vec[i + 1].px - (float)p_vec[i].px) * j /
												 dy +
										 (float)p_vec[i].px);
				p.py = short(0.5 +
										 ((float)p_vec[i + 1].py - (float)p_vec[i].py) * j /
												 dy +
										 (float)p_vec[i].py);
				m_WorkBits[Pt2coord(p)] = 255;
			}
		}
	}

	dx = std::max(p_vec[n - 1].px, p_vec[0].px) - std::min(p_vec[n - 1].px, p_vec[0].px);
	dy = std::max(p_vec[n - 1].py, p_vec[0].py) - std::min(p_vec[n - 1].py, p_vec[0].py);
	p.px = p_vec[n - 1].px;
	p.py = p_vec[n - 1].py;
	m_WorkBits[Pt2coord(p)] = 0;
	if (dx > dy)
	{
		for (unsigned j = 1; j < dx; j++)
		{
			p.px = short(0.5 + ((float)p_vec[0].px - (float)p_vec[n - 1].px) * j / dx +
									 (float)p_vec[n - 1].px);
			p.py = short(0.5 + ((float)p_vec[0].py - (float)p_vec[n - 1].py) * j / dx +
									 (float)p_vec[n - 1].py);
			m_WorkBits[Pt2coord(p)] = 255;
		}
	}
	else
	{
		for (unsigned j = 1; j < dy; j++)
		{
			p.px = short(0.5 + ((float)p_vec[0].px - (float)p_vec[n - 1].px) * j / dy +
									 (float)p_vec[n - 1].px);
			p.py = short(0.5 + ((float)p_vec[0].py - (float)p_vec[n - 1].py) * j / dy +
									 (float)p_vec[n - 1].py);
			m_WorkBits[Pt2coord(p)] = 255;
		}
	}

	}

void Bmphandler::ConnectedComponents(bool connectivity)
{
	unsigned char dummymode = m_Mode1;

	float* temp = (float*)malloc(sizeof(float) * (m_Area + m_Width + m_Height + 1));
	std::vector<float> maps;
	float newest = 0.1f;

	for (unsigned i = 0; i < m_Area; i++)
		temp[i + i / m_Width + m_Width + 2] = m_BmpBits[i];
	for (unsigned i = m_Width + 1; i <= m_Area + m_Height; i += m_Width + 1)
		temp[i] = -1e10f;
	for (unsigned i = 0; i <= m_Width; i++)
		temp[i] = -2e10f;

	unsigned i1 = m_Width + 2;
	unsigned i2 = 0;

	for (short j = 0; j < m_Height; j++)
	{
		for (short k = 0; k < m_Width; k++)
		{
			if (temp[i1] == temp[i1 - 1])
			{
				m_WorkBits[i2] = BaseConnection(m_WorkBits[i2 - 1], &maps);
				if (temp[i1] == temp[i1 - m_Width - 1])
				{
					maps[(int)BaseConnection(m_WorkBits[i2 - m_Width], &maps)] =
							m_WorkBits[i2];
				}
			}
			else
			{
				if (temp[i1] == temp[i1 - m_Width - 1])
					m_WorkBits[i2] =
							BaseConnection(m_WorkBits[i2 - m_Width], &maps);
				else if (connectivity && temp[i1] == temp[i1 - m_Width - 2])
					m_WorkBits[i2] =
							BaseConnection(m_WorkBits[i2 - m_Width - 1], &maps);
				else
				{
					maps.push_back(newest);
					m_WorkBits[i2] = newest;
					newest++;
				}
			}

			if (connectivity && temp[i1 - 1] == temp[i1 - m_Width - 1])
				maps[(int)BaseConnection(m_WorkBits[i2 - 1], &maps)] =
						BaseConnection(m_WorkBits[i2 - m_Width], &maps);

			i1++;
			i2++;
		}
		i1++;
	}

	for (unsigned i = 0; i < m_Area; i++)
		m_WorkBits[i] = BaseConnection(m_WorkBits[i], &maps);

	newest = 0.0f;
	for (i1 = 0; i1 < (unsigned)maps.size(); i1++)
		if ((unsigned)maps[i1] == i1)
		{
			maps[i1] = newest;
			newest++;
		}

	for (unsigned i = 0; i < m_Area; i++)
		m_WorkBits[i] = maps[(int)m_WorkBits[i]];

	free(temp);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::ConnectedComponents(bool connectivity, std::set<float>& components)
{
	unsigned char dummymode = m_Mode1;

	float* temp = (float*)malloc(sizeof(float) * (m_Area + m_Width + m_Height + 1));
	std::vector<float> maps;
	float newest = 0.1f;

	for (unsigned i = 0; i < m_Area; i++)
		temp[i + i / m_Width + m_Width + 2] = m_BmpBits[i];
	for (unsigned i = m_Width + 1; i <= m_Area + m_Height; i += m_Width + 1)
		temp[i] = -1e10f;
	for (unsigned i = 0; i <= m_Width; i++)
		temp[i] = -2e10f;

	unsigned i1 = m_Width + 2;
	unsigned i2 = 0;

	for (short j = 0; j < m_Height; j++)
	{
		for (short k = 0; k < m_Width; k++)
		{
			if (temp[i1] == temp[i1 - 1])
			{
				m_WorkBits[i2] = BaseConnection(m_WorkBits[i2 - 1], &maps);
				if (temp[i1] == temp[i1 - m_Width - 1])
				{
					maps[(int)BaseConnection(m_WorkBits[i2 - m_Width], &maps)] =
							m_WorkBits[i2];
				}
			}
			else
			{
				if (temp[i1] == temp[i1 - m_Width - 1])
					m_WorkBits[i2] =
							BaseConnection(m_WorkBits[i2 - m_Width], &maps);
				else if (connectivity && temp[i1] == temp[i1 - m_Width - 2])
					m_WorkBits[i2] =
							BaseConnection(m_WorkBits[i2 - m_Width - 1], &maps);
				else
				{
					maps.push_back(newest);
					m_WorkBits[i2] = newest;
					newest++;
				}
			}

			if (connectivity && temp[i1 - 1] == temp[i1 - m_Width - 1])
				maps[(int)BaseConnection(m_WorkBits[i2 - 1], &maps)] =
						BaseConnection(m_WorkBits[i2 - m_Width], &maps);

			i1++;
			i2++;
		}
		i1++;
	}

	for (unsigned i = 0; i < m_Area; i++)
		m_WorkBits[i] = BaseConnection(m_WorkBits[i], &maps);

	newest = 0.0f;
	for (i1 = 0; i1 < (unsigned)maps.size(); i1++)
		if ((unsigned)maps[i1] == i1)
		{
			maps[i1] = newest;
			newest++;
		}

	for (unsigned i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = maps[(int)m_WorkBits[i]];
		components.insert(m_WorkBits[i]);
	}

	free(temp);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

float Bmphandler::BaseConnection(float c, std::vector<float>* maps)
{
	if (c == (*maps)[(int)c])
		return c;
	else
		return (*maps)[(int)c] = BaseConnection((*maps)[(int)c], maps);
}

void Bmphandler::FillGaps(short unsigned n, bool connectivity)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	float* tmp1;
	float* tmp2 = m_Sliceprovide->GiveMe();
	float* dummy;
	bool* dummybool;
	float* bmpstore;
	bool* isinterface = (bool*)malloc(sizeof(bool) * m_Area);
	bool* isinterfaceold = (bool*)malloc(sizeof(bool) * m_Area);

	for (unsigned int i = 0; i < m_Area; i++)
	{
		tmp2[i] = m_WorkBits[i];
		isinterfaceold[i] = false;
	}
	Closure(int(ceil(float(n) / 2)), connectivity);
	tmp1 = m_WorkBits;
	bmpstore = m_BmpBits;
	m_BmpBits = tmp2;
	tmp2 = m_Sliceprovide->GiveMe();
	m_WorkBits = m_Sliceprovide->GiveMe();
	ConnectedComponents(connectivity);

	float f = -1;
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (m_BmpBits[i] == 0)
			m_WorkBits[i] = -1;
	}

	for (int l = 0; l < (int)std::ceil(float(n + 1) / 2); l++)
	{
		for (unsigned int i = 0; i < m_Area; i++)
		{
			tmp2[i] = m_WorkBits[i];
			isinterface[i] = isinterfaceold[i];
		}

		int i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < m_Width; j++)
			{
				if (m_WorkBits[i1] != m_WorkBits[i1 + m_Width])
				{
					if (m_WorkBits[i1] == f)
					{
						if (tmp2[i1] == f)
							tmp2[i1] = tmp2[i1 + m_Width];
						else if (tmp2[i1] != tmp2[i1 + m_Width])
							isinterface[i1] = true;
					}
					else if (m_WorkBits[i1 + m_Width] == f)
					{
						if (tmp2[i1 + m_Width] == f)
							tmp2[i1 + m_Width] = tmp2[i1];
						else if (tmp2[i1 + m_Width] != tmp2[i1])
							isinterface[i1 + m_Width] = true;
					}
					else
					{
						if (m_BmpBits[i1] == 0)
							isinterface[i1] = true;
						if (m_BmpBits[i1 + m_Width] == 0)
							isinterface[i1 + m_Width] = true;
					}
				}
				if (isinterfaceold[i1] && (m_BmpBits[i1 + m_Width] == 0))
					isinterface[i1 + m_Width] = true;
				else if (isinterfaceold[i1 + m_Width] && (m_BmpBits[i1] == 0))
					isinterface[i1] = true;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
			{
				if (m_WorkBits[i1] != m_WorkBits[i1 + 1])
				{
					if (m_WorkBits[i1] == f)
					{
						if (tmp2[i1] == f)
							tmp2[i1] = tmp2[i1 + 1];
						else if (tmp2[i1] != tmp2[i1 + 1])
							isinterface[i1] = true;
					}
					else if (m_WorkBits[i1 + 1] == f)
					{
						if (tmp2[i1 + 1] == f)
							tmp2[i1 + 1] = tmp2[i1];
						else if (tmp2[i1 + 1] != tmp2[i1])
							isinterface[i1 + 1] = true;
					}
					else
					{
						if (m_BmpBits[i1] == 0)
							isinterface[i1] = true;
						if (m_BmpBits[i1 + 1] == 0)
							isinterface[i1 + 1] = true;
					}
				}
				if (isinterfaceold[i1] && (m_BmpBits[i1 + 1] == 0))
					isinterface[i1 + 1] = true;
				else if (isinterfaceold[i1 + 1] && (m_BmpBits[i1] == 0))
					isinterface[i1] = true;

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (m_Height - 1); i++)
			{
				for (unsigned short j = 0; j < (m_Width - 1); j++)
				{
					if (m_WorkBits[i1] != m_WorkBits[i1 + m_Width + 1])
					{
						if (m_WorkBits[i1] == f)
						{
							if (tmp2[i1] == f)
								tmp2[i1] = tmp2[i1 + m_Width + 1];
							else if (tmp2[i1] != tmp2[i1 + m_Width + 1])
								isinterface[i1] = true;
						}
						else if (m_WorkBits[i1 + m_Width + 1] == f)
						{
							if (tmp2[i1 + m_Width + 1] == f)
								tmp2[i1 + m_Width + 1] = tmp2[i1];
							else if (tmp2[i1 + m_Width + 1] != tmp2[i1])
								isinterface[i1 + m_Width + 1] = true;
						}
						else
						{
							if (m_BmpBits[i1] == 0)
								isinterface[i1] = true;
							if (m_BmpBits[i1 + m_Width + 1] == 0)
								isinterface[i1 + m_Width + 1] = true;
						}
					}
					if (isinterfaceold[i1] && (m_BmpBits[i1 + m_Width + 1] == 0))
						isinterface[i1 + m_Width + 1] = true;
					else if (isinterfaceold[i1 + m_Width + 1] &&
									 (m_BmpBits[i1] == 0))
						isinterface[i1] = true;

					if (m_WorkBits[i1 + 1] != m_WorkBits[i1 + m_Width])
					{
						if (m_WorkBits[i1 + 1] == f)
						{
							if (tmp2[i1 + 1] == f)
								tmp2[i1 + 1] = tmp2[i1 + m_Width];
							else if (tmp2[i1 + 1] != tmp2[i1 + m_Width])
								isinterface[i1 + 1] = true;
						}
						else if (m_WorkBits[i1 + m_Width] == f)
						{
							if (tmp2[i1 + m_Width] == f)
								tmp2[i1 + m_Width] = tmp2[i1 + 1];
							else if (tmp2[i1 + m_Width] != tmp2[i1 + 1])
								isinterface[i1 + m_Width] = true;
						}
						else
						{
							if (m_BmpBits[i1 + 1] == 0)
								isinterface[i1 + 1] = true;
							if (m_BmpBits[i1 + m_Width] == 0)
								isinterface[i1 + m_Width] = true;
						}
					}
					if (isinterfaceold[i1 + 1] && (m_BmpBits[i1 + m_Width] == 0))
						isinterface[i1 + m_Width] = true;
					else if (isinterfaceold[i1 + m_Width] &&
									 (m_BmpBits[i1 + 1] == 0))
						isinterface[i1 + 1] = true;

					i1++;
				}
				i1++;
			}
		}

		dummy = tmp2;
		tmp2 = m_WorkBits;
		m_WorkBits = dummy;

		dummybool = isinterface;
		isinterface = isinterfaceold;
		isinterfaceold = dummybool;
	}

	for (int l = 0; l < (int)std::floor(float(n - 1) / 2); l++)
	{
		for (unsigned int i = 0; i < m_Area; i++)
		{
			isinterface[i] = isinterfaceold[i];
		}

		int i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < m_Width; j++)
			{
				if (isinterfaceold[i1] && (m_BmpBits[i1 + m_Width] == 0))
					isinterface[i1 + m_Width] = true;
				else if (isinterfaceold[i1 + m_Width] && (m_BmpBits[i1] == 0))
					isinterface[i1] = true;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
			{
				if (isinterfaceold[i1] && (m_BmpBits[i1 + 1] == 0))
					isinterface[i1 + 1] = true;
				else if (isinterfaceold[i1 + 1] && (m_BmpBits[i1] == 0))
					isinterface[i1] = true;

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (m_Height - 1); i++)
			{
				for (unsigned short j = 0; j < (m_Width - 1); j++)
				{
					if (isinterfaceold[i1] && (m_BmpBits[i1 + m_Width + 1] == 0))
						isinterface[i1 + m_Width + 1] = true;
					else if (isinterfaceold[i1 + m_Width + 1] &&
									 (m_BmpBits[i1] == 0))
						isinterface[i1] = true;

					if (isinterfaceold[i1 + 1] && (m_BmpBits[i1 + m_Width] == 0))
						isinterface[i1 + m_Width] = true;
					else if (isinterfaceold[i1 + m_Width] &&
									 (m_BmpBits[i1 + 1] == 0))
						isinterface[i1 + 1] = true;

					i1++;
				}
				i1++;
			}
		}

		dummy = tmp2;
		tmp2 = m_WorkBits;
		m_WorkBits = dummy;

		dummybool = isinterface;
		isinterface = isinterfaceold;
		isinterfaceold = dummybool;
	}

	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (m_BmpBits[i] == 0 && isinterfaceold[i])
			m_BmpBits[i] = tmp1[i];
		//		if(isinterfaceold[i])
		//			bmp_bits[i]=255;
		//		else bmp_bits[i]=0;
		//		bmp_bits[i]=tmp2[i];
	}

	m_Sliceprovide->TakeBack(tmp2);
	m_Sliceprovide->TakeBack(m_WorkBits);
	m_Sliceprovide->TakeBack(tmp1);
	m_WorkBits = m_BmpBits;
	m_BmpBits = bmpstore;
	free(isinterface);
	free(isinterfaceold);

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

void Bmphandler::FillGapstissue(tissuelayers_size_t idx, short unsigned n, bool connectivity)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	float* workstore = m_WorkBits;
	m_WorkBits = m_Sliceprovide->GiveMe();
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = (float)tissues[i];
	}

	FillGaps(n, connectivity);

	for (unsigned int i = 0; i < m_Area; i++)
	{
		tissues[i] = (tissues_size_t)m_WorkBits[i];
	}
	m_Sliceprovide->TakeBack(m_WorkBits);
	m_WorkBits = workstore;

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

void Bmphandler::GetTissuecontours(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize)
{
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	tissues_size_t* tmp_bits = (tissues_size_t*)malloc(sizeof(tissues_size_t) * (m_Width + 2) * (m_Height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (m_Width + 2) * (m_Height + 2));
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		visited[i] = false;

	unsigned pos = m_Width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner;									 //1 for outer, 7 for inner border
	short direction, directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, m_Width + 3, m_Width + 2, m_Width + 1, -1, -m_Width - 3, -m_Width - 2, -m_Width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned i = 0; i < m_Height; i++)
	{
		for (unsigned j = 0; j < m_Width; j++)
		{
			tmp_bits[pos] = tissues[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(m_Width + 2); i++)
		tmp_bits[i] = TISSUES_SIZE_MAX;
	for (unsigned i = unsigned(m_Width + 2) * (m_Height + 1);
			 i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		tmp_bits[i] = TISSUES_SIZE_MAX;
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = TISSUES_SIZE_MAX;
	for (unsigned i = m_Width + 1; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = TISSUES_SIZE_MAX;

	pos = m_Width + 2;
	while (pos < unsigned(m_Width + 2) * (m_Height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(m_Width + 2) * (m_Height + 1))
			pos++;

		if (pos < unsigned(m_Width + 2) * (m_Height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (m_Width + 2) - 1);
			p.py = short(pos / (m_Width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + m_Width + 3] != f &&
					tmp_bits[pos + m_Width + 2] != f &&
					tmp_bits[pos + m_Width + 1] != f &&
					tmp_bits[pos - m_Width - 1] != f &&
					tmp_bits[pos - m_Width - 2] != f &&
					tmp_bits[pos - m_Width - 3] != f)
			{
				vec_pt.push_back(p);
				if (1 >= minsize)
					(*outer_line).push_back(vec_pt);
				visited[pos] = true;
			}
			else
			{
				if (tmp_bits[pos - m_Width - 3] == f)
				{						 // tricky criteria
					inner = 7; // inner line
					directionold = direction = 1;
				}
				else
				{
					/*					if(tmp_bits[pos-width-2]==f){
						inner=7; // inner line
						direction=2;
					}
					else{*/
					inner = 1; // outer line
					directionold = direction = 7;
					//					}
				}

				bubble_size = 0;
				linelength = 0;

				direction = (direction + 5 * inner) % 8;
				done = false;
				directionchange = 5 * inner;
				while (!done)
				{
					directionchange += inner;
					direction = (direction + inner) % 8;
					pos2 = pos1 + offset[direction];
					if (tmp_bits[pos2] == f)
						done = true;
				}

				possecond = pos2;

				do
				{
					vec_pt.push_back(p);
					//					if(inner==1)
					//						bmp_bits[p.px+(unsigned)width*p.py]=255-bmp_bits[p.px+(unsigned)width*p.py];//xxxxxxxxxxxxxxxxxx
					//					if(direction<5||inner==1||tmp_bits[pos1-1]==f||directionold<4)
					if (tmp_bits[pos1 - 1] == f ||
							(inner == 7 &&
									!(((direction + 6) % 8 > 2 && directionold > 3 &&
												(directionchange + 5) % 8 > 2) ||
											(direction == 5 && directionold == 3))) ||
							(inner == 1 && !(((direction + 4) % 8 > 2 &&
																	 (directionold + 7) % 8 < 4 &&
																	 (directionchange + 5) % 8 > 2) ||
																 (direction == 3 && directionold == 5)))
							//					 (inner==1&&!(||(direction==3&&directionold==5)))
					)
						visited[pos1] = true;
					pos1 = pos2;
					p.px = short(pos1 % (m_Width + 2) - 1);
					p.py = short(pos1 / (m_Width + 2) - 1);
					//					vec_pt.push_back(p);
					bubble_size += dy[direction] * (2 * p.px - dx[direction]);
					bubble_size -= bordervolume[directionchange % 8];
					linelength += 2;
					//					work_bits[p.px+p.py*width]=255;
					directionold = direction;
					direction = (direction + 5 * inner) % 8;
					done = false;
					directionchange = 5 * inner;
					while (!done)
					{
						directionchange += inner;
						direction = (direction + inner) % 8;
						pos2 = pos1 + offset[direction];
						if (tmp_bits[pos2] == f)
							done = true;
					}
				}
				//				while(pos1!=pos||(inner==1&&pos2!=possecond));
				while (pos1 != pos || pos2 != possecond);
				//				while(pos1!=pos||(inner==1&&!visited[pos2]));
				//				while(pos1!=pos);
				//				vec_pt.pop_back();

				if (inner == 1)
				{
					if ((bubble_size + linelength) >= (float)minsize)
						(*outer_line).push_back(vec_pt);
					if (bubble_size + linelength < 0)
					{
						//						cout << bubble_size+linelength <<", ";
						for (std::vector<Point>::iterator it1 = vec_pt.begin();
								 it1 != vec_pt.end(); it1++)
						{
							m_BmpBits[it1->px + (unsigned)m_Width * it1->py] = 255;
							//							cout << it1->px << ":" << it1->py << "."<<it1->px+(unsigned)width*it1->py <<" ";
						}
						//						cout << endl;
					}
				}
				else
				{
					if (bubble_size >= (float)minsize)
						(*inner_line).push_back(vec_pt);
					if (bubble_size < 0)
					{
						//						cout << bubble_size <<"; ";
						for (std::vector<Point>::iterator it1 = vec_pt.begin();
								 it1 != vec_pt.end(); it1++)
						{
							m_BmpBits[it1->px + (unsigned)m_Width * it1->py] = 255;
							//							cout << it1->px << ":" << it1->py << "."<<it1->px+(unsigned)width*it1->py <<" ";
						}
						//						cout << endl;
					}
				}
			}
			pos++;
		}
	}

	free(tmp_bits);
	free(visited);
}

void Bmphandler::GetTissuecontours2Xmirrored(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize)
{
	int movpos[4];
	movpos[0] = 1;
	movpos[1] = m_Width + 2;
	movpos[2] = -1;
	movpos[3] = -int(m_Width + 2);

	int pos = m_Width + 3;
	int pos1 = 0;
	int pos2;

	unsigned setto = TISSUES_SIZE_MAX + 1;
	unsigned* tmp_bits = (unsigned*)malloc(sizeof(unsigned) * (m_Width + 2) * (m_Height + 2));
	unsigned char* nrlines = (unsigned char*)malloc(sizeof(unsigned char) * (m_Width + 2) * (m_Height + 2));

	unsigned f1 = (unsigned)f;

	std::vector<Point> vec_pt;
	float vol;

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (short unsigned i = 0; i < m_Height; i++)
	{
		for (short unsigned j = 0; j < m_Width; j++)
		{
			tmp_bits[pos] = (unsigned)tissues[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(m_Width + 2); i++)
		tmp_bits[i] = setto;
	for (unsigned i = unsigned(m_Width + 2) * (m_Height + 1); i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		tmp_bits[i] = setto;
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i += (m_Width + 2))
		tmp_bits[i] = setto;
	for (unsigned i = m_Width + 1; i < unsigned(m_Width + 2) * (m_Height + 2); i += (m_Width + 2))
		tmp_bits[i] = setto;
	tmp_bits[1] = tmp_bits[m_Width] = tmp_bits[unsigned(m_Width + 2) * (m_Height + 1) + 1] = tmp_bits[unsigned(m_Width + 2) * (m_Height + 2) - 2] = setto + 1;
	tmp_bits[m_Width + 2] = tmp_bits[2 * m_Width + 3] = tmp_bits[unsigned(m_Width + 2) * (m_Height)] = tmp_bits[unsigned(m_Width + 2) * (m_Height + 1) - 1] = setto + 2;

	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		nrlines[i] = 0;

	pos = 0;
	for (unsigned short i = 0; i < m_Height + 1; i++)
	{
		for (unsigned short j = 0; j < m_Width + 1; j++)
		{
			if ((tmp_bits[pos] != tmp_bits[pos + 1]) && (tmp_bits[pos] == f1 || tmp_bits[pos + 1] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos] != tmp_bits[pos + m_Width + 2]) && (tmp_bits[pos] == f1 || tmp_bits[pos + m_Width + 2] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos + m_Width + 2] != tmp_bits[pos + m_Width + 3]) && (tmp_bits[pos + m_Width + 2] == f1 || tmp_bits[pos + m_Width + 3] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 3]) && (tmp_bits[pos + 1] == f1 || tmp_bits[pos + m_Width + 3] == f1))
				nrlines[pos]++;
			pos++;
		}
		pos++;
	}

	unsigned short direction;

	pos = 0;

	Point p;
	Point p2;
	bool inner;
	unsigned char casenr;

	while (pos < int(m_Width + 2) * (m_Height + 2))
	{
		while (pos < int(m_Width + 2) * (m_Height + 2) && nrlines[pos] == 0)
			pos++;

		if (pos != int(m_Width + 2) * (m_Height + 2))
		{
			vec_pt.clear();
			inner = (tmp_bits[pos + m_Width + 3] != f1);
			pos2 = pos;
			direction = 3;
			p.px = p2.px = 2 * (m_Width + 1 - (pos % (m_Width + 2)));
			p.py = p2.py = 2 * (pos / (m_Width + 2)) + 1;

			vol = 0;
			unsigned count = 0;

			do
			{
				count++;
				casenr = 0;
				if (tmp_bits[pos] == f1)
					casenr += 1;
				if (tmp_bits[pos + 1] == f1)
					casenr += 2;
				if (tmp_bits[pos + m_Width + 3] == f1)
					casenr += 4;
				if (tmp_bits[pos + m_Width + 2] == f1)
					casenr += 8;
				nrlines[pos] -= 2;
				switch (casenr)
				{
				case 1:
					if (direction == 0)
					{
						p.px--;
						if (tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 2])
						{
							vec_pt.push_back(p);
							vol += p.py;
						}
						else
						{
							vol += (p.py - .5f);
						}
						direction = 3;
						p.py--;
					}
					else
					{
						direction = 2;
						p.py++;
						if (tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 2])
						{
							vec_pt.push_back(p);
							vol -= p.py;
						}
						else
						{
							vol -= (p.py - .5f);
						}
						p.px++;
					}
					break;
				case 2:
					if (direction == 1)
					{
						direction = 0;
						p.py++;
						if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3])
						{
							vec_pt.push_back(p);
							vol += p.py;
						}
						else
						{
							vol += (p.py - .5f);
						}
						p.px--;
					}
					else
					{
						direction = 3;
						p.px++;
						if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3])
						{
							vec_pt.push_back(p);
							vol -= p.py;
						}
						else
						{
							vol -= (p.py - .5f);
						}
						p.py--;
					}
					break;
				case 3:
					if (direction == 0)
					{
						p.px -= 2;
						vol += (p.py * 2.0f);
					}
					else
					{
						p.px += 2;
						vol -= (p.py * 2.0f);
					}
					break;
				case 4:
					if (direction == 3)
					{
						direction = 0;
						p.py--;
						if (tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 2])
						{
							vec_pt.push_back(p);
							vol += p.py;
						}
						else
						{
							vol += (p.py + 0.5f);
						}
						p.px--;
					}
					else
					{
						direction = 1;
						p.px++;
						if (tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 2])
						{
							vec_pt.push_back(p);
							vol -= p.py;
						}
						else
						{
							vol -= (p.py + 0.5f);
						}
						p.py++;
					}
					break;
				case 5:
					if (tmp_bits[pos + m_Width + 2] != tmp_bits[pos + 1] ||
							tmp_bits[pos + 1] > f1)
					{
						if (direction == 0)
						{
							direction = 1;
							p.px--;
							p.py++;
							vol += (p.py - 0.5f);
						}
						else if (direction == 1)
						{
							direction = 0;
							p.px--;
							p.py++;
							vol += (p.py - 0.5f);
						}
						else if (direction == 2)
						{
							direction = 3;
							p.px++;
							p.py--;
							vol -= (p.py + 0.5f);
						}
						else if (direction == 3)
						{
							direction = 2;
							p.px++;
							p.py--;
							vol -= (p.py + 0.5f);
						}
					}
					else
					{
						if (direction == 0)
						{
							direction = 3;
							p.px--;
							p.py--;
							vol += (p.py + 0.5f);
						}
						else if (direction == 1)
						{
							direction = 2;
							p.px++;
							p.py++;
							vol -= (p.py - 0.5f);
						}
						else if (direction == 2)
						{
							direction = 1;
							p.px++;
							p.py++;
							vol -= (p.py - 0.5f);
						}
						else if (direction == 3)
						{
							direction = 0;
							p.px--;
							p.py--;
							vol += (p.py + 0.5f);
						}
					}
					break;
				case 6:
					if (direction == 3)
					{
						p.py -= 2;
					}
					else
					{
						p.py += 2;
					}
					break;
				case 7:
					if (direction == 0)
					{
						direction = 1;
						p.px--;
						p.py++;
						vol += (p.py - 0.5f);
					}
					else
					{
						direction = 2;
						p.px++;
						p.py--;
						vol -= (p.py + 0.5f);
					}
					break;
				case 8:
					if (direction == 0)
					{
						direction = 1;
						p.px--;
						if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3])
						{
							vec_pt.push_back(p);
							vol += p.py;
						}
						else
						{
							vol += (p.py + 0.5f);
						}
						p.py++;
					}
					else
					{
						direction = 2;
						p.py--;
						if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3])
						{
							vec_pt.push_back(p);
							vol -= p.py;
						}
						else
						{
							vol -= (p.py + 0.5f);
						}
						p.px++;
					}
					break;
				case 9:
					if (direction == 3)
					{
						p.py -= 2;
					}
					else
					{
						p.py += 2;
					}
					break;
				case 10:
					if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3] ||
							tmp_bits[pos] > f1)
					{
						if (direction == 0)
						{
							direction = 3;
							p.px--;
							p.py--;
							vol += (p.py + 0.5f);
						}
						else if (direction == 1)
						{
							direction = 2;
							p.px++;
							p.py++;
							vol -= (p.py - 0.5f);
						}
						else if (direction == 2)
						{
							direction = 1;
							p.px++;
							p.py++;
							vol -= (p.py - 0.5f);
						}
						else if (direction == 3)
						{
							direction = 0;
							p.px--;
							p.py--;
							vol += (p.py + 0.5f);
						}
					}
					else
					{
						if (direction == 0)
						{
							direction = 1;
							p.px--;
							p.py++;
							vol += (p.py - 0.5f);
						}
						else if (direction == 1)
						{
							direction = 0;
							p.px--;
							p.py++;
							vol += (p.py - 0.5f);
						}
						else if (direction == 2)
						{
							direction = 3;
							p.px++;
							p.py--;
							vol -= (p.py + 0.5f);
						}
						else if (direction == 3)
						{
							direction = 2;
							p.px++;
							p.py--;
							vol -= (p.py + 0.5f);
						}
					}
					break;
				case 11:
					if (direction == 3)
					{
						direction = 0;
						p.px--;
						p.py--;
						vol += (p.py + 0.5f);
					}
					else
					{
						direction = 1;
						p.px++;
						p.py++;
						vol -= (p.py - 0.5f);
					}
					break;
				case 12:
					if (direction == 0)
					{
						p.px -= 2;
						vol += (p.py * 2.0f);
					}
					else
					{
						p.px += 2;
						vol -= (p.py * 2.0f);
					}
					break;
				case 13:
					if (direction == 1)
					{
						direction = 0;
						p.px--;
						p.py++;
						vol += (p.py - .5f);
					}
					else
					{
						direction = 3;
						p.px++;
						p.py--;
						vol -= (p.py + .5f);
					}
					break;
				case 14:
					if (direction == 0)
					{
						direction = 3;
						p.px--;
						p.py--;
						vol += (p.py + .5f);
					}
					else
					{
						direction = 2;
						p.px++;
						p.py++;
						vol -= (p.py - .5f);
					}
					break;
				}

				pos += movpos[direction];
				vec_pt.push_back(p);
			} while (pos != pos2 || p.px != p2.px || p.py != p2.py);

			if (std::abs(vol / 4) >= (float)minsize - 0.6f)
			{
				if (inner)
					(*inner_line).push_back(vec_pt);
				else
					(*outer_line).push_back(vec_pt);
			}
		}
	}

	free(tmp_bits);
	free(nrlines);
}

void Bmphandler::GetTissuecontours2Xmirrored(tissuelayers_size_t idx, tissues_size_t f, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize, float disttol)
{
	int movpos[4];
	movpos[0] = 1;
	movpos[1] = m_Width + 2;
	movpos[2] = -1;
	movpos[3] = -int(m_Width + 2);

	int pos = m_Width + 3;
	int pos1 = 0;
	int pos2;

	unsigned setto = TISSUES_SIZE_MAX + 1;
	unsigned* tmp_bits = (unsigned*)malloc(sizeof(unsigned) * (m_Width + 2) * (m_Height + 2));
	unsigned f1 = (unsigned)f;
	unsigned char* nrlines = (unsigned char*)malloc(sizeof(unsigned char) * (m_Width + 2) * (m_Height + 2));

	std::vector<Point> vec_pt;
	std::vector<unsigned> vec_meetings;
	float vol;

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (short unsigned i = 0; i < m_Height; i++)
	{
		for (short unsigned j = 0; j < m_Width; j++)
		{
			tmp_bits[pos] = (unsigned)tissues[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(m_Width + 2); i++)
		tmp_bits[i] = setto;
	for (unsigned i = unsigned(m_Width + 2) * (m_Height + 1); i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		tmp_bits[i] = setto;
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i += (m_Width + 2))
		tmp_bits[i] = setto;
	for (unsigned i = m_Width + 1; i < unsigned(m_Width + 2) * (m_Height + 2); i += (m_Width + 2))
		tmp_bits[i] = setto;
	tmp_bits[1] = tmp_bits[m_Width] = tmp_bits[unsigned(m_Width + 2) * (m_Height + 1) + 1] = tmp_bits[unsigned(m_Width + 2) * (m_Height + 2) - 2] = setto + 1;
	tmp_bits[m_Width + 2] = tmp_bits[2 * m_Width + 3] = tmp_bits[unsigned(m_Width + 2) * (m_Height)] = tmp_bits[unsigned(m_Width + 2) * (m_Height + 1) - 1] = setto + 2;

	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		nrlines[i] = 0;

	pos = 0;
	for (unsigned short i = 0; i < m_Height + 1; i++)
	{
		for (unsigned short j = 0; j < m_Width + 1; j++)
		{
			if ((tmp_bits[pos] != tmp_bits[pos + 1]) && (tmp_bits[pos] == f1 || tmp_bits[pos + 1] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos] != tmp_bits[pos + m_Width + 2]) && (tmp_bits[pos] == f1 || tmp_bits[pos + m_Width + 2] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos + m_Width + 2] != tmp_bits[pos + m_Width + 3]) && (tmp_bits[pos + m_Width + 2] == f1 || tmp_bits[pos + m_Width + 3] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 3]) && (tmp_bits[pos + 1] == f1 || tmp_bits[pos + m_Width + 3] == f1))
				nrlines[pos]++;
			pos++;
		}
		pos++;
	}

	unsigned short direction;

	pos = 0;

	Point p;
	Point p2;
	bool inner;
	unsigned char casenr;

	while (pos < int(m_Width + 2) * (m_Height + 2))
	{
		while (pos < int(m_Width + 2) * (m_Height + 2) && nrlines[pos] == 0)
			pos++;

		if (pos != int(m_Width + 2) * (m_Height + 2))
		{
			vec_pt.clear();
			vec_meetings.clear();
			inner = (tmp_bits[pos + m_Width + 3] != f1);
			pos2 = pos;
			direction = 3;
			p.px = p2.px = 2 * (m_Width + 1 - (pos % (m_Width + 2)));
			p.py = p2.py = 2 * (pos / (m_Width + 2)) + 1;

			vol = 0;
			unsigned count = 0;

			do
			{
				count++;
				casenr = 0;
				if (tmp_bits[pos] == f1)
					casenr += 1;
				if (tmp_bits[pos + 1] == f1)
					casenr += 2;
				if (tmp_bits[pos + m_Width + 3] == f1)
					casenr += 4;
				if (tmp_bits[pos + m_Width + 2] == f1)
					casenr += 8;
				nrlines[pos] -= 2;
				switch (casenr)
				{
				case 1:
					if (direction == 0)
					{
						p.px--;
						if (tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 2])
						{
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							vol += p.py;
						}
						else
						{
							vol += (p.py - .5f);
						}
						direction = 3;
						p.py--;
					}
					else
					{
						direction = 2;
						p.py++;
						if (tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 2])
						{
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							vol -= p.py;
						}
						else
						{
							vol -= (p.py - .5f);
						}
						p.px++;
					}
					break;
				case 2:
					if (direction == 1)
					{
						direction = 0;
						p.py++;
						if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3])
						{
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							vol += p.py;
						}
						else
						{
							vol += (p.py - .5f);
						}
						p.px--;
					}
					else
					{
						direction = 3;
						p.px++;
						if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3])
						{
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							vol -= p.py;
						}
						else
						{
							vol -= (p.py - .5f);
						}
						p.py--;
					}
					break;
				case 3:
					if (direction == 0)
					{
						if (tmp_bits[pos + m_Width + 2] !=
								tmp_bits[pos + m_Width + 3])
						{
							p.px--;
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							p.px--;
						}
						else
						{
							p.px -= 2;
						}
						vol += (p.py * 2.0f);
					}
					else
					{
						if (tmp_bits[pos + m_Width + 2] !=
								tmp_bits[pos + m_Width + 3])
						{
							p.px++;
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							p.px++;
						}
						else
						{
							p.px += 2;
						}
						vol -= (p.py * 2.0f);
					}
					break;
				case 4:
					if (direction == 3)
					{
						direction = 0;
						p.py--;
						if (tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 2])
						{
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							vol += p.py;
						}
						else
						{
							vol += (p.py + 0.5f);
						}
						p.px--;
					}
					else
					{
						direction = 1;
						p.px++;
						if (tmp_bits[pos + 1] != tmp_bits[pos + m_Width + 2])
						{
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							vol -= p.py;
						}
						else
						{
							vol -= (p.py + 0.5f);
						}
						p.py++;
					}
					break;
				case 5:
					if (tmp_bits[pos + m_Width + 2] != tmp_bits[pos + 1] ||
							tmp_bits[pos + 1] > f1)
					{
						if (direction == 0)
						{
							direction = 1;
							p.px--;
							p.py++;
							vol += (p.py - 0.5f);
						}
						else if (direction == 1)
						{
							direction = 0;
							p.px--;
							p.py++;
							vol += (p.py - 0.5f);
						}
						else if (direction == 2)
						{
							direction = 3;
							p.px++;
							p.py--;
							vol -= (p.py + 0.5f);
						}
						else if (direction == 3)
						{
							direction = 2;
							p.px++;
							p.py--;
							vol -= (p.py + 0.5f);
						}
					}
					else
					{
						if (direction == 0)
						{
							direction = 3;
							p.px--;
							p.py--;
							vol += (p.py + 0.5f);
						}
						else if (direction == 1)
						{
							direction = 2;
							p.px++;
							p.py++;
							vol -= (p.py - 0.5f);
						}
						else if (direction == 2)
						{
							direction = 1;
							p.px++;
							p.py++;
							vol -= (p.py - 0.5f);
						}
						else if (direction == 3)
						{
							direction = 0;
							p.px--;
							p.py--;
							vol += (p.py + 0.5f);
						}
					}
					break;
				case 6:
					if (direction == 3)
					{
						if (tmp_bits[pos + m_Width + 2] != tmp_bits[pos])
						{
							p.py--;
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							p.py--;
						}
						else
						{
							p.py -= 2;
						}
					}
					else
					{
						if (tmp_bits[pos + m_Width + 2] != tmp_bits[pos])
						{
							p.py++;
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							p.py++;
						}
						else
						{
							p.py += 2;
						}
					}
					break;
				case 7:
					if (direction == 0)
					{
						direction = 1;
						p.px--;
						p.py++;
						vol += (p.py - 0.5f);
					}
					else
					{
						direction = 2;
						p.px++;
						p.py--;
						vol -= (p.py + 0.5f);
					}
					break;
				case 8:
					if (direction == 0)
					{
						direction = 1;
						p.px--;
						if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3])
						{
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							vol += p.py;
						}
						else
						{
							vol += (p.py + 0.5f);
						}
						p.py++;
					}
					else
					{
						direction = 2;
						p.py--;
						if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3])
						{
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							vol -= p.py;
						}
						else
						{
							vol -= (p.py + 0.5f);
						}
						p.px++;
					}
					break;
				case 9:
					if (direction == 3)
					{
						if (tmp_bits[pos + m_Width + 3] != tmp_bits[pos + 1])
						{
							p.py--;
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							p.py--;
						}
						else
						{
							p.py -= 2;
						}
					}
					else
					{
						if (tmp_bits[pos + m_Width + 3] != tmp_bits[pos + 1])
						{
							p.py++;
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							p.py++;
						}
						else
						{
							p.py += 2;
						}
					}
					break;
				case 10:
					if (tmp_bits[pos] != tmp_bits[pos + m_Width + 3] ||
							tmp_bits[pos] > f1)
					{
						if (direction == 0)
						{
							direction = 3;
							p.px--;
							p.py--;
							vol += (p.py + 0.5f);
						}
						else if (direction == 1)
						{
							direction = 2;
							p.px++;
							p.py++;
							vol -= (p.py - 0.5f);
						}
						else if (direction == 2)
						{
							direction = 1;
							p.px++;
							p.py++;
							vol -= (p.py - 0.5f);
						}
						else if (direction == 3)
						{
							direction = 0;
							p.px--;
							p.py--;
							vol += (p.py + 0.5f);
						}
					}
					else
					{
						if (direction == 0)
						{
							direction = 1;
							p.px--;
							p.py++;
							vol += (p.py - 0.5f);
						}
						else if (direction == 1)
						{
							direction = 0;
							p.px--;
							p.py++;
							vol += (p.py - 0.5f);
						}
						else if (direction == 2)
						{
							direction = 3;
							p.px++;
							p.py--;
							vol -= (p.py + 0.5f);
						}
						else if (direction == 3)
						{
							direction = 2;
							p.px++;
							p.py--;
							vol -= (p.py + 0.5f);
						}
					}
					break;
				case 11:
					if (direction == 3)
					{
						direction = 0;
						p.px--;
						p.py--;
						vol += (p.py + 0.5f);
					}
					else
					{
						direction = 1;
						p.px++;
						p.py++;
						vol -= (p.py - 0.5f);
					}
					break;
				case 12:
					if (direction == 0)
					{
						if (tmp_bits[pos] != tmp_bits[pos + 1])
						{
							p.px--;
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							p.px--;
						}
						else
						{
							p.px -= 2;
						}
						vol += (p.py * 2.0f);
					}
					else
					{
						if (tmp_bits[pos] != tmp_bits[pos + 1])
						{
							p.px++;
							vec_meetings.push_back(vec_pt.size());
							vec_pt.push_back(p);
							p.px++;
						}
						else
						{
							p.px += 2;
						}
						vol -= (p.py * 2.0f);
					}
					break;
				case 13:
					if (direction == 1)
					{
						direction = 0;
						p.px--;
						p.py++;
						vol += (p.py - .5f);
					}
					else
					{
						direction = 3;
						p.px++;
						p.py--;
						vol -= (p.py + .5f);
					}
					break;
				case 14:
					if (direction == 0)
					{
						direction = 3;
						p.px--;
						p.py--;
						vol += (p.py + .5f);
					}
					else
					{
						direction = 2;
						p.px++;
						p.py++;
						vol -= (p.py - .5f);
					}
					break;
				}
				pos += movpos[direction];
				vec_pt.push_back(p);
			} while (pos != pos2 || p.px != p2.px || p.py != p2.py);

			if (std::abs(vol / 4) >= (float)minsize - 0.6f)
			{
				if (vec_meetings.empty())
				{
					vec_meetings.push_back(0);
				}
				Contour2 cc2;
				std::vector<Point> vec_simp;
				cc2.DougPeuck(disttol * 2, &vec_pt, &vec_meetings, &vec_simp);
				if (vec_simp.size() > 2)
				{
					if (inner)
						(*inner_line).push_back(vec_simp);
					else
						(*outer_line).push_back(vec_simp);
				}
			}
		}
	}

	free(tmp_bits);
	free(nrlines);
}

void Bmphandler::GetContours(float f, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize)
{
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	float* tmp_bits = (float*)malloc(sizeof(float) * (m_Width + 2) * (m_Height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (m_Width + 2) * (m_Height + 2));
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		visited[i] = false;

	unsigned pos = m_Width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner;									 //1 for outer, 7 for inner border
	short direction, directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, m_Width + 3, m_Width + 2, m_Width + 1, -1, -m_Width - 3, -m_Width - 2, -m_Width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	for (unsigned i = 0; i < m_Height; i++)
	{
		for (unsigned j = 0; j < m_Width; j++)
		{
			tmp_bits[pos] = m_WorkBits[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(m_Width + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = unsigned(m_Width + 2) * (m_Height + 1);
			 i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvisited;
	for (unsigned i = m_Width + 1; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvisited;

	pos = m_Width + 2;
	while (pos < unsigned(m_Width + 2) * (m_Height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(m_Width + 2) * (m_Height + 1))
			pos++;

		if (pos < unsigned(m_Width + 2) * (m_Height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (m_Width + 2) - 1);
			p.py = short(pos / (m_Width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + m_Width + 3] != f &&
					tmp_bits[pos + m_Width + 2] != f &&
					tmp_bits[pos + m_Width + 1] != f &&
					tmp_bits[pos - m_Width - 1] != f &&
					tmp_bits[pos - m_Width - 2] != f &&
					tmp_bits[pos - m_Width - 3] != f)
			{
				vec_pt.push_back(p);
				if (1 >= minsize)
					(*outer_line).push_back(vec_pt);
				visited[pos] = true;
			}
			else
			{
				if (tmp_bits[pos - m_Width - 3] == f)
				{						 // tricky criteria
					inner = 7; // inner line
					directionold = direction = 1;
				}
				else
				{
					/*					if(tmp_bits[pos-width-2]==f){
						inner=7; // inner line
						direction=2;
					}
					else{*/
					inner = 1; // outer line
					directionold = direction = 7;
					//					}
				}

				bubble_size = 0;
				linelength = 0;

				direction = (direction + 5 * inner) % 8;
				done = false;
				directionchange = 5 * inner;
				while (!done)
				{
					directionchange += inner;
					direction = (direction + inner) % 8;
					pos2 = pos1 + offset[direction];
					if (tmp_bits[pos2] == f)
						done = true;
				}

				possecond = pos2;

				do
				{
					vec_pt.push_back(p);
					//					if(inner==1)
					//						bmp_bits[p.px+(unsigned)width*p.py]=255-bmp_bits[p.px+(unsigned)width*p.py];//xxxxxxxxxxxxxxxxxx
					//					if(direction<5||inner==1||tmp_bits[pos1-1]==f||directionold<4)
					if (tmp_bits[pos1 - 1] == f ||
							(inner == 7 &&
									!(((direction + 6) % 8 > 2 && directionold > 3 &&
												(directionchange + 5) % 8 > 2) ||
											(direction == 5 && directionold == 3))) ||
							(inner == 1 && !(((direction + 4) % 8 > 2 &&
																	 (directionold + 7) % 8 < 4 &&
																	 (directionchange + 5) % 8 > 2) ||
																 (direction == 3 && directionold == 5)))
							//					 (inner==1&&!(||(direction==3&&directionold==5)))
					)
						visited[pos1] = true;
					pos1 = pos2;
					p.px = short(pos1 % (m_Width + 2) - 1);
					p.py = short(pos1 / (m_Width + 2) - 1);
					//					vec_pt.push_back(p);
					bubble_size += dy[direction] * (2 * p.px - dx[direction]);
					bubble_size -= bordervolume[directionchange % 8];
					linelength += 2;
					//					work_bits[p.px+p.py*width]=255;
					directionold = direction;
					direction = (direction + 5 * inner) % 8;
					done = false;
					directionchange = 5 * inner;
					while (!done)
					{
						directionchange += inner;
						direction = (direction + inner) % 8;
						pos2 = pos1 + offset[direction];
						if (tmp_bits[pos2] == f)
							done = true;
					}
				} while (pos1 != pos || pos2 != possecond);

				if (inner == 1)
				{
					if ((bubble_size + linelength) >= (float)minsize)
						(*outer_line).push_back(vec_pt);
					if (bubble_size + linelength < 0)
					{
						//						cout << bubble_size+linelength <<", ";
						for (std::vector<Point>::iterator it1 = vec_pt.begin();
								 it1 != vec_pt.end(); it1++)
						{
							m_BmpBits[it1->px + (unsigned)m_Width * it1->py] = 255;
							//							cout << it1->px << ":" << it1->py << "."<<it1->px+(unsigned)width*it1->py <<" ";
						}
						//						cout << endl;
					}
				}
				else
				{
					if (bubble_size >= (float)minsize)
						(*inner_line).push_back(vec_pt);
					if (bubble_size < 0)
					{
						//						cout << bubble_size <<"; ";
						for (std::vector<Point>::iterator it1 = vec_pt.begin();
								 it1 != vec_pt.end(); it1++)
						{
							m_BmpBits[it1->px + (unsigned)m_Width * it1->py] = 255;
							//							cout << it1->px << ":" << it1->py << "."<<it1->px+(unsigned)width*it1->py <<" ";
						}
						//						cout << endl;
					}
				}
			}
			pos++;
		}
	}

	free(tmp_bits);
	free(visited);
}

void Bmphandler::GetContours(Point p, std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line, int minsize)
{
	GetContours(WorkPt(p), outer_line, inner_line, minsize);
}

void Bmphandler::DistanceMap(bool connectivity)
{
	unsigned char dummymode = m_Mode1;

	std::vector<unsigned> sorted[256];
	std::vector<unsigned>::iterator it;

	m_BmpIsGrey = false; //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	if (!m_BmpIsGrey)
	{
		SwapBmpwork();
		Pair p1;
		GetRange(&p1);
		ScaleColors(p1);
		SwapBmpwork();
	}

	Bucketsort(sorted, m_BmpBits);

	float wbp;
	unsigned p;
	for (unsigned i = 0; i < m_Area; i++)
		m_WorkBits[i] = 0;
	for (it = sorted[255].begin(); it != sorted[255].end(); it++)
	{
		m_WorkBits[*it] = m_BmpBits[*it];
	}

	for (unsigned i = 255; i > 0; i--)
	{
		for (it = sorted[i].begin(); it != sorted[i].end(); it++)
		{
			p = *it;
			wbp = m_WorkBits[p];

			if ((unsigned)floor(wbp + 0.5f) == i)
			{
				if (p % m_Width != 0 && m_WorkBits[p - 1] < wbp - 1)
				{
					m_WorkBits[p - 1] = wbp - 1;
					sorted[i - 1].push_back(p - 1);
				}
				if ((p + 1) % m_Width != 0 && m_WorkBits[p + 1] < wbp - 1)
				{
					m_WorkBits[p + 1] = wbp - 1;
					sorted[i - 1].push_back(p + 1);
				}
				if (p >= m_Width && m_WorkBits[p - m_Width] < wbp - 1)
				{
					m_WorkBits[p - m_Width] = wbp - 1;
					sorted[i - 1].push_back(p - m_Width);
				}
				if ((p + m_Width) < m_Area && m_WorkBits[p + m_Width] < wbp - 1)
				{
					m_WorkBits[p + m_Width] = wbp - 1;
					sorted[i - 1].push_back(p + m_Width);
				}
				if (connectivity)
				{
					if (p % m_Width != 0 && p >= m_Width &&
							m_WorkBits[p - 1 - m_Width] < wbp - 1)
					{
						m_WorkBits[p - 1 - m_Width] = wbp - 1;
						sorted[i - 1].push_back(p - 1 - m_Width);
					}
					if (p % m_Width != 0 && (p + m_Width) < m_Area &&
							m_WorkBits[p - 1 + m_Width] < wbp - 1)
					{
						m_WorkBits[p - 1 + m_Width] = wbp - 1;
						sorted[i - 1].push_back(p - 1 + m_Width);
					}
					if ((p + 1) % m_Width != 0 && p >= m_Width &&
							m_WorkBits[p - m_Width + 1] < wbp - 1)
					{
						m_WorkBits[p - m_Width + 1] = wbp - 1;
						sorted[i - 1].push_back(p - m_Width + 1);
					}
					if ((p + 1) % m_Width != 0 && (p + m_Width) < m_Area &&
							m_WorkBits[p + m_Width + 1] < wbp - 1)
					{
						m_WorkBits[p + m_Width + 1] = wbp - 1;
						sorted[i - 1].push_back(p + m_Width + 1);
					}
				}
			}
		}
	}

	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

void Bmphandler::DistanceMap(bool connectivity, float f, short unsigned levlset)
{
	unsigned char dummymode = m_Mode1;
	std::vector<unsigned> v1, v2;
	std::vector<unsigned>*vp1, *vp2, *vpdummy;
	const float background = f - m_Width - m_Height;

	std::vector<std::vector<Point>> vo, vi;
	std::vector<unsigned>::iterator vuit;
	std::vector<Point>::iterator vpit;

	SwapBmpwork();
	GetContours(f, &vo, &vi, 0);
	SwapBmpwork();

	for (unsigned i = 0; i < (unsigned)vo.size(); i++)
		for (vpit = vo[i].begin(); vpit != vo[i].end(); vpit++)
			v1.push_back(Pt2coord(*vpit));
	for (unsigned i = 0; i < (unsigned)vi.size(); i++)
		for (vpit = vi[i].begin(); vpit != vi[i].end(); vpit++)
			v1.push_back(Pt2coord(*vpit));

	for (unsigned i = 0; i < m_Area; i++)
		m_WorkBits[i] = background;
	for (vuit = v1.begin(); vuit != v1.end(); vuit++)
		m_WorkBits[*vuit] = f;

	if (levlset == 0)
		for (unsigned i = 0; i < m_Area; i++)
			if (m_BmpBits[i] == f)
				m_WorkBits[i] = f;

	if (levlset == 1)
		for (unsigned i = 0; i < m_Area; i++)
			if (m_BmpBits[i] != f)
				m_WorkBits[i] = f;

	vp1 = &v1;
	vp2 = &v2;

	float wbp;
	unsigned p;

	while (!(*vp1).empty())
	{
		for (vuit = (*vp1).begin(); vuit != (*vp1).end(); vuit++)
		{
			p = *vuit;
			wbp = m_WorkBits[p];

			if (p % m_Width != 0 && m_WorkBits[p - 1] == background)
			{
				if (m_BmpBits[p - 1] == f)
				{
					m_WorkBits[p - 1] = wbp + 1;
					(*vp2).push_back(p - 1);
				}
				else
				{
					m_WorkBits[p - 1] = wbp - 1;
					(*vp2).push_back(p - 1);
				}
			}

			if ((p + 1) % m_Width != 0 && m_WorkBits[p + 1] == background)
			{
				if (m_BmpBits[p + 1] == f)
				{
					m_WorkBits[p + 1] = wbp + 1;
					(*vp2).push_back(p + 1);
				}
				else
				{
					m_WorkBits[p + 1] = wbp - 1;
					(*vp2).push_back(p + 1);
				}
			}

			if (p >= m_Width && m_WorkBits[p - m_Width] == background)
			{
				if (m_BmpBits[p - m_Width] == f)
				{
					m_WorkBits[p - m_Width] = wbp + 1;
					(*vp2).push_back(p - m_Width);
				}
				else
				{
					m_WorkBits[p - m_Width] = wbp - 1;
					(*vp2).push_back(p - m_Width);
				}
			}

			if ((p + m_Width) < m_Area != 0 && m_WorkBits[p + m_Width] == background)
			{
				if (m_BmpBits[p + m_Width] == f)
				{
					m_WorkBits[p + m_Width] = wbp + 1;
					(*vp2).push_back(p + m_Width);
				}
				else
				{
					m_WorkBits[p + m_Width] = wbp - 1;
					(*vp2).push_back(p + m_Width);
				}
			}

			if (connectivity)
			{
				if (p % m_Width != 0 && p >= m_Width &&
						m_WorkBits[p - 1 - m_Width] == background)
				{
					if (m_BmpBits[p - 1 - m_Width] == f)
					{
						m_WorkBits[p - 1 - m_Width] = wbp + 1;
						(*vp2).push_back(p - 1 - m_Width);
					}
					else
					{
						m_WorkBits[p - 1 - m_Width] = wbp - 1;
						(*vp2).push_back(p - 1 - m_Width);
					}
				}

				if (p % m_Width != 0 && (p + m_Width) < m_Area &&
						m_WorkBits[p - 1 + m_Width] == background)
				{
					if (m_BmpBits[p - 1 + m_Width] == f)
					{
						m_WorkBits[p - 1 + m_Width] = wbp + 1;
						(*vp2).push_back(p - 1 + m_Width);
					}
					else
					{
						m_WorkBits[p - 1 + m_Width] = wbp - 1;
						(*vp2).push_back(p - 1 + m_Width);
					}
				}

				if ((p + 1) % m_Width != 0 && p >= m_Width &&
						m_WorkBits[p - m_Width + 1] == background)
				{
					if (m_BmpBits[p - m_Width + 1] == f)
					{
						m_WorkBits[p - m_Width + 1] = wbp + 1;
						(*vp2).push_back(p - m_Width + 1);
					}
					else
					{
						m_WorkBits[p - m_Width + 1] = wbp - 1;
						(*vp2).push_back(p - m_Width + 1);
					}
				}

				if ((p + 1) % m_Width != 0 && (p + m_Width) < m_Area &&
						m_WorkBits[p + m_Width + 1] == background)
				{
					if (m_BmpBits[p + m_Width + 1] == f)
					{
						m_WorkBits[p + m_Width + 1] = wbp + 1;
						(*vp2).push_back(p + m_Width + 1);
					}
					else
					{
						m_WorkBits[p + m_Width + 1] = wbp - 1;
						(*vp2).push_back(p + m_Width + 1);
					}
				}
			}
		}

		(*vp1).clear();
		vpdummy = vp1;
		vp1 = vp2;
		vp2 = vpdummy;
	}

	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

unsigned* Bmphandler::DeadReckoning(float f)
{
	unsigned char dummymode = m_Mode1;
	unsigned* p = (unsigned*)malloc(m_Area * sizeof(unsigned));

	for (unsigned i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = float((m_Width + m_Height) * (m_Width + m_Height));
		p[i] = m_Area;
	}

	std::vector<std::vector<Point>> vo, vi;
	std::vector<unsigned>::iterator vuit;
	std::vector<Point>::iterator vpit;

	SwapBmpwork();
	GetContours(f, &vo, &vi, 0);
	SwapBmpwork();

	for (unsigned i = 0; i < (unsigned)vo.size(); i++)
	{
		for (vpit = vo[i].begin(); vpit != vo[i].end(); vpit++)
		{
			m_WorkBits[Pt2coord(*vpit)] = 0;
			p[Pt2coord(*vpit)] = Pt2coord(*vpit);
		}
		//		cout << ";"<<vo[i].size();
	}
	for (unsigned i = 0; i < (unsigned)vi.size(); i++)
	{
		for (vpit = vi[i].begin(); vpit != vi[i].end(); vpit++)
		{
			m_WorkBits[Pt2coord(*vpit)] = 0;
			p[Pt2coord(*vpit)] = Pt2coord(*vpit);
		}
		//		cout << "."<<vi[i].size();
	}

	unsigned j = 0;

	float d1 = 1;
	float d2 = sqrt(2.0f);

	for (unsigned short k = 0; k < m_Height; k++)
	{
		for (unsigned short l = 0; l < m_Width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && m_WorkBits[j - 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j - m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			if (l > 0 && m_WorkBits[j - 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j - 1];
				m_WorkBits[j] =
						sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
											 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			j++;
		}
	}

	j = m_Area - 1;

	for (int k = m_Height - 1; k >= 0; k--)
	{
		for (int l = m_Width - 1; l >= 0; l--)
		{
			if ((l + 1) != m_Width && m_WorkBits[j + 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j + 1];
				m_WorkBits[j] =
						sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
											 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			if ((k + 1) != m_Height)
			{
				if (l > 0 && m_WorkBits[j - 1 + m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 + m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j + m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j + m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 + m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 + m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			j--;
		}
	}

	j = 0;

	for (unsigned short k = 0; k < m_Height; k++)
	{
		for (unsigned short l = 0; l < m_Width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && m_WorkBits[j - 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j - m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			if (l > 0 && m_WorkBits[j - 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j - 1];
				m_WorkBits[j] =
						sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
											 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			j++;
		}
	}

	for (unsigned i = 0; i < m_Area; i++)
		if (m_BmpBits[i] != f)
			m_WorkBits[i] = -m_WorkBits[i];

	/*	for(unsigned i=0;i<(unsigned)vo.size();i++)
		for(vpit=vo[i].begin();vpit!=vo[i].end();vpit++) {
			work_bits[pt2coord(*vpit)]=255;
			P[pt2coord(*vpit)]=pt2coord(*vpit);
		}
	for(unsigned i=0;i<(unsigned)vi.size();i++)
		for(vpit=vi[i].begin();vpit!=vi[i].end();vpit++) {
			work_bits[pt2coord(*vpit)]=255;
			P[pt2coord(*vpit)]=pt2coord(*vpit);
		}*/

	m_Mode1 = dummymode;
	m_Mode2 = 1;

	return p;
}

void Bmphandler::DeadReckoning()
{
	unsigned char dummymode = m_Mode1;
	unsigned* p = (unsigned*)malloc(m_Area * sizeof(unsigned));

	for (unsigned i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = float((m_Width + m_Height) * (m_Width + m_Height));
		p[i] = m_Area;
	}

	unsigned i1 = 0;
	for (unsigned short h = 0; h < m_Height - 1; h++)
	{
		for (unsigned short w = 0; w < m_Width; w++)
		{
			if (m_BmpBits[i1] != m_BmpBits[i1 + m_Width])
			{
				m_WorkBits[i1] = m_WorkBits[i1 + m_Width] = 0;
				p[i1] = i1;
				p[i1 + m_Width] = i1 + m_Width;
			}
			i1++;
		}
	}

	i1 = 0;
	for (unsigned short h = 0; h < m_Height; h++)
	{
		for (unsigned short w = 0; w < m_Width - 1; w++)
		{
			if (m_BmpBits[i1] != m_BmpBits[i1 + 1])
			{
				m_WorkBits[i1] = m_WorkBits[i1 + 1] = 0;
				p[i1] = i1;
				p[i1 + 1] = i1 + 1;
			}
			i1++;
		}
		i1++;
	}

	unsigned j = 0;

	float d1 = 1;
	float d2 = sqrt(2.0f);

	for (unsigned short k = 0; k < m_Height; k++)
	{
		for (unsigned short l = 0; l < m_Width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && m_WorkBits[j - 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j - m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			if (l > 0 && m_WorkBits[j - 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j - 1];
				m_WorkBits[j] =
						sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
											 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			j++;
		}
	}

	j = m_Area - 1;

	for (int k = m_Height - 1; k >= 0; k--)
	{
		for (int l = m_Width - 1; l >= 0; l--)
		{
			if ((l + 1) != m_Width && m_WorkBits[j + 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j + 1];
				m_WorkBits[j] =
						sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
											 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			if ((k + 1) != m_Height)
			{
				if (l > 0 && m_WorkBits[j - 1 + m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 + m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j + m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j + m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 + m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 + m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			j--;
		}
	}

	j = 0;

	for (unsigned short k = 0; k < m_Height; k++)
	{
		for (unsigned short l = 0; l < m_Width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && m_WorkBits[j - 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j - m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 - m_Width];
					m_WorkBits[j] =
							sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
												 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			if (l > 0 && m_WorkBits[j - 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j - 1];
				m_WorkBits[j] =
						sqrt(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
											 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			j++;
		}
	}

	//	for(unsigned i=0;i<area;i++)
	//		if(bmp_bits[i]==0) work_bits[i]=-work_bits[i];

	m_Mode1 = dummymode;
	m_Mode2 = 1;

	free(p);
}

unsigned* Bmphandler::DeadReckoningSquared(float f)
{
	unsigned char dummymode = m_Mode1;
	unsigned* p = (unsigned*)malloc(m_Area * sizeof(unsigned));

	for (unsigned i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = float((m_Width + m_Height) * (m_Width + m_Height));
		p[i] = m_Area;
	}

	std::vector<std::vector<Point>> vo, vi;
	std::vector<Point>::iterator vpit;

	SwapBmpwork();
	GetContours(f, &vo, &vi, 0);
	SwapBmpwork();

	for (unsigned i = 0; i < (unsigned)vo.size(); i++)
	{
		for (vpit = vo[i].begin(); vpit != vo[i].end(); vpit++)
		{
			m_WorkBits[Pt2coord(*vpit)] = 0;
			p[Pt2coord(*vpit)] = Pt2coord(*vpit);
		}
	}
	for (unsigned i = 0; i < (unsigned)vi.size(); i++)
	{
		for (vpit = vi[i].begin(); vpit != vi[i].end(); vpit++)
		{
			m_WorkBits[Pt2coord(*vpit)] = 0;
			p[Pt2coord(*vpit)] = Pt2coord(*vpit);
		}
	}

	unsigned j = 0;

	float d1 = 1;
	float d2 = sqrt(2.0f);

	for (unsigned short k = 0; k < m_Height; k++)
	{
		for (unsigned short l = 0; l < m_Width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && m_WorkBits[j - 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 - m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j - m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j - m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 - m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			if (l > 0 && m_WorkBits[j - 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j - 1];
				m_WorkBits[j] = (float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
															 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			j++;
		}
	}

	j = m_Area - 1;

	for (int k = m_Height - 1; k >= 0; k--)
	{
		for (int l = m_Width - 1; l >= 0; l--)
		{
			if ((l + 1) != m_Width && m_WorkBits[j + 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j + 1];
				m_WorkBits[j] = (float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
															 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			if ((k + 1) != m_Height)
			{
				if (l > 0 && m_WorkBits[j - 1 + m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 + m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j + m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j + m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 + m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 + m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			j--;
		}
	}

	j = 0;

	for (unsigned short k = 0; k < m_Height; k++)
	{
		for (unsigned short l = 0; l < m_Width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && m_WorkBits[j - 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j - 1 - m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if (m_WorkBits[j - m_Width] + d1 < m_WorkBits[j])
				{
					p[j] = p[j - m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}

				if ((l + 1) != m_Width &&
						m_WorkBits[j + 1 - m_Width] + d2 < m_WorkBits[j])
				{
					p[j] = p[j + 1 - m_Width];
					m_WorkBits[j] =
							(float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
										 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
				}
			}

			if (l > 0 && m_WorkBits[j - 1] + d1 < m_WorkBits[j])
			{
				p[j] = p[j - 1];
				m_WorkBits[j] = (float((l - p[j] % m_Width) * (l - p[j] % m_Width) +
															 (k - p[j] / m_Width) * (k - p[j] / m_Width)));
			}

			j++;
		}
	}

	for (unsigned i = 0; i < m_Area; i++)
		if (m_BmpBits[i] != f)
			m_WorkBits[i] = -m_WorkBits[i];

	/*	for(unsigned i=0;i<(unsigned)vo.size();i++)
		for(vpit=vo[i].begin();vpit!=vo[i].end();vpit++) {
			work_bits[pt2coord(*vpit)]=255;
			P[pt2coord(*vpit)]=pt2coord(*vpit);
		}
	for(unsigned i=0;i<(unsigned)vi.size();i++)
		for(vpit=vi[i].begin();vpit!=vi[i].end();vpit++) {
			work_bits[pt2coord(*vpit)]=255;
			P[pt2coord(*vpit)]=pt2coord(*vpit);
		}*/

	m_Mode1 = dummymode;
	m_Mode2 = 1;

	return p;
}

void Bmphandler::IftDistance1(float f)
{
	unsigned char dummymode = m_Mode1;
	ImageForestingTransformDistance if_tdist;
	if_tdist.DistanceInit(m_Width, m_Height, f, m_BmpBits);
	float* f1 = if_tdist.ReturnPf();
	for (unsigned i = 0; i < m_Area; i++)
	{
		if (m_BmpBits[i] == f)
			m_WorkBits[i] = f1[i];
		else
			m_WorkBits[i] = -f1[i];
	}
	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

void Bmphandler::RgIft(float* lb_map, float thresh)
{
	unsigned char dummymode = m_Mode1;
	ImageForestingTransformRegionGrowing if_trg;

	//	sobel();

	//	IFTrg.rg_init(width,height,work_bits,lb_map);
	if_trg.RgInit(m_Width, m_Height, m_BmpBits, lb_map);

	float* f1 = if_trg.ReturnLb();
	float* f2 = if_trg.ReturnPf();

	for (unsigned i = 0; i < m_Area; i++)
	{
		if (f2[i] < thresh)
			m_WorkBits[i] = f1[i];
		else
			m_WorkBits[i] = 0;
	}

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

ImageForestingTransformRegionGrowing* Bmphandler::IfTrgInit(float* lb_map)
{
	ImageForestingTransformRegionGrowing* if_trg =
			new ImageForestingTransformRegionGrowing;

	//	float *tmp=work_bits;
	//	work_bits=sliceprovide->give_me();

	//	sobel();

	//	IFTrg->rg_init(width,height,work_bits,lb_map);
	if_trg->RgInit(m_Width, m_Height, m_BmpBits, lb_map);

	//	sliceprovide->take_back(work_bits);

	//	work_bits=tmp;

	return if_trg;
}

ImageForestingTransformLivewire* Bmphandler::Livewireinit(Point pt)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	float* sobelx = m_Sliceprovide->GiveMe();
	float* sobely = m_Sliceprovide->GiveMe();
	float* tmp = m_Sliceprovide->GiveMe();
	float* dummy;
	float* dummy1;
	float* grad = m_WorkBits;
	m_WorkBits = m_Sliceprovide->GiveMe();

	Gaussian(1);
	dummy = m_BmpBits;
	m_BmpBits = tmp;
	tmp = dummy;
	SwapBmpwork();
	Sobelxy(&sobelx, &sobely);

	dummy = DirectionMap(sobelx, sobely);

	Pair p;
	GetRange(&p);

	//	sliceprovide->take_back(sobely);
	//	sobely=grad;

	//	grad=work_bits;
	//	work_bits=sobelx;
	dummy1 = sobely;
	sobely = m_WorkBits;
	m_WorkBits = dummy1;

	LaplacianZero(2.0f, 30, false);

	if (p.high != 0)
		for (unsigned i = 0; i < m_Area; i++)
			sobelx[i] = (0.43f * (1 - sobely[i] / p.high) +
									 0.43f * ((m_WorkBits[i] + 1) / 256));
	else
		for (unsigned i = 0; i < m_Area; i++)
			sobelx[i] = 0.43f * ((m_WorkBits[i] + 1) / 256);

	ImageForestingTransformLivewire* lw = new ImageForestingTransformLivewire;
	lw->LwInit(m_Width, m_Height, sobelx, dummy, pt);

	m_Sliceprovide->TakeBack(sobelx);
	m_Sliceprovide->TakeBack(sobely);
	m_Sliceprovide->TakeBack(m_BmpBits);
	m_Sliceprovide->TakeBack(m_WorkBits);
	m_Sliceprovide->TakeBack(dummy);
	m_BmpBits = tmp;
	m_WorkBits = grad;

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;

	return lw;
}

void Bmphandler::FillContour(std::vector<Point>* vp, bool continuous)
{
	unsigned char dummymode = m_Mode1;

	if (continuous)
	{
		std::vector<int> s;
		float* results =
				(float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

		int i = m_Width + 3;
		int i1 = 0;
		for (int j = 0; j < m_Height; j++)
		{
			for (int k = 0; k < m_Width; k++)
			{
				results[i] = -1;
				i++;
				i1++;
			}

			i += 2;
		}

		for (std::vector<Point>::iterator it = vp->begin(); it != vp->end(); it++)
		{
			results[it->px + 1 + (it->py + 1) * (m_Width + 2)] = 0;
		}

		for (int j = 0; j < m_Width + 2; j++)
			results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
		for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
			results[j] = results[j + m_Width + 1] = 0;

		for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
		{
			if (results[j] == -1)
			{
				results[j] = 255.0f;
				s.push_back(j);
			}
		}
		for (unsigned int j = m_Area + 2 * m_Height + 1;
				 j < m_Area + m_Width + 2 * m_Height + 1; j++)
		{
			if (results[j] == -1)
			{
				results[j] = 255.0f;
				s.push_back(j);
			}
		}
		for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
				 j += m_Width + 2)
		{
			if (results[j] == -1)
			{
				results[j] = 255.0f;
				s.push_back(j);
			}
		}
		for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
				 j += m_Width + 2)
		{
			if (results[j] == -1)
			{
				results[j] = 255.0f;
				s.push_back(j);
			}
		}

		HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, false, 255.0f);

		i = m_Width + 3;
		int i2 = 0;
		for (int j = 0; j < m_Height; j++)
		{
			for (int k = 0; k < m_Width; k++)
			{
				m_WorkBits[i2] = 255.0f - results[i];
				//				if(results[i]==0) 0;
				//				work_bits[i2]=results[i];
				i++;
				i2++;
			}

			i += 2;
		}

		free(results);
	}
	else
	{
		std::vector<Point> vp1;
		std::vector<Point>::iterator it = vp->begin();
		Point p = *it;
		it++;

		for (; it != vp->end(); it++)
		{
			addLine(&vp1, p, *it);
			p = *it;
		}
		addLine(&vp1, p, *(vp->begin()));

		FillContour(&vp1, true);
	}

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::AddSkin(unsigned i4, float setto)
{
	unsigned i = 0, pos, y, x, j;

	//Create a binary std::vector noTissue/Tissue
	std::vector<int> s;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i] == 0)
				s.push_back(-1);
			else
				s.push_back(0);
			i++;
		}
	}

	// i4 itetations through  y, -y, x, -x converting, each time a tissue beginning is find, one tissue pixel into skin
	//!! It is assumed that ix and iy are the same
	bool convert_skin = true;
	for (i = 1; i < i4 + 1; i++)
	{
		for (y = 0; y < m_Height; y++)
		{
			pos = y * m_Width;
			while (pos < (y + 1) * m_Width)
			{
				if (s[pos] == 0)
				{
					if (convert_skin)
						s[pos] = 256;
					convert_skin = false;
				}
				else
					convert_skin = true;

				pos++;
			}

			pos = (y + 1) * m_Width - 1;
			while (pos > y * m_Width)
			{
				if (s[pos] == 0)
				{
					if (convert_skin)
						s[pos] = 256;
					convert_skin = false;
				}
				else
					convert_skin = true;

				pos--;
			}
		}

		for (x = 0; x < m_Width; x++)
		{
			pos = x;
			while (pos < m_Height * m_Width)
			{
				if (s[pos] == 0)
				{
					if (convert_skin)
						s[pos] = 256;
					convert_skin = false;
				}
				else
					convert_skin = true;

				pos += m_Width;
			}

			pos = m_Width * (m_Height - 1) + x;
			while (pos > m_Width)
			{
				if (s[pos] == 0)
				{
					if (convert_skin)
						s[pos] = 256;
					convert_skin = false;
				}
				else
					convert_skin = true;

				pos -= m_Width;
			}
		}
	}

	//go over the std::vector and set the skin pixel at the source pointer
	i = 0;
	for (j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (s[i] == 256)
				m_WorkBits[i] = setto;
			i++;
		}
	}
}

void Bmphandler::AddSkinOutside(unsigned i4, float setto)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;
	std::vector<int> s;
	std::vector<int> s1;
	float* results =
			(float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i3 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i3] == 0)
				results[i] = -1;
			else
			{
				results[i] = 0;
				s1.push_back(i);
			}
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	//hysteretic_growth(results,&s,width+2,height+2,false,255.0f);
	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, true, 255.0f);

	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 2); j++)
		if (results[j] == 255.0f)
			results[j] = -1;

	//hysteretic_growth(results,&s1,width+2,height+2,false,255.0f,i4);
	HystereticGrowth(results, &s1, m_Width + 2, m_Height + 2, true, 255.0f, i4);

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 255.0f)
				m_WorkBits[i2] = setto;
			//			work_bits[i2]=results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;

	free(results);
}

void Bmphandler::AddSkintissue(tissuelayers_size_t idx, unsigned i4, tissues_size_t setto)
{
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i3 = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tissues[i3] == 0)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, false, 255.0f);

	unsigned short x, y;
	unsigned pos;
	unsigned i1;
	unsigned w = (unsigned)(m_Width + 2);

	for (y = 1; y < m_Height + 1; y++)
	{
		pos = y * w;
		i1 = 0;
		while (pos < (y + 1) * w)
		{
			if (results[pos] == 255.0f)
				i1 = i4;
			else
			{
				if (i1 > 0)
				{
					results[pos] = 256.0f;
					i1--;
				}
			}
			pos++;
		}
	}

	for (y = 1; y < m_Height + 1; y++)
	{
		pos = (y + 1) * w - 1;
		i1 = 0;
		while (pos > y * w)
		{
			if (results[pos] == 255.0f)
				i1 = i4;
			else
			{
				if (i1 > 0)
				{
					results[pos] = 256.0f;
					i1--;
				}
			}
			pos--;
		}
	}

	for (x = 1; x < m_Width + 1; x++)
	{
		pos = x;
		i1 = 0;
		while (pos < (m_Height + 1) * w)
		{
			if (results[pos] == 255.0f)
				i1 = i4;
			else
			{
				if (i1 > 0)
				{
					results[pos] = 256.0f;
					i1--;
				}
			}
			pos += w;
		}
	}

	for (x = 1; x < m_Width + 1; x++)
	{
		pos = w * (m_Height + 1) + x;
		i1 = 0;
		while (pos > w)
		{
			if (results[pos] == 255.0f)
				i1 = i4;
			else
			{
				if (i1 > 0)
				{
					results[pos] = 256.0f;
					i1--;
				}
			}
			pos -= w;
		}
	}

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 256.0f &&
					(!TissueInfos::GetTissueLocked(tissues[i2])))
				tissues[i2] = setto;
			//			work_bits[i2]=results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);
}

void Bmphandler::AddSkintissueOutside(tissuelayers_size_t idx, unsigned i4, tissues_size_t setto)
{
	std::vector<int> s;
	std::vector<int> s1;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i3 = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tissues[i3] == 0)
				results[i] = -1;
			else
			{
				results[i] = 0;
				s1.push_back(i);
			}
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, false, 255.0f);

	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 2); j++)
		if (results[j] == 255.0f)
			results[j] = -1;

	HystereticGrowth(results, &s1, m_Width + 2, m_Height + 2, false, 255.0f, i4);

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 255.0f)
				tissues[i2] = setto;
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);
}

bool Bmphandler::ValueAtBoundary(float value)
{
	// Top
	float* tmp = &m_WorkBits[0];
	for (unsigned pos = 0; pos < m_Width; pos++, tmp++)
	{
		if (*tmp == value)
			return true;
	}

	// Left & right
	for (unsigned pos = 1; pos < (m_Height - 1); pos++)
	{
		if (m_WorkBits[pos * m_Width] == value)
			return true;
		else if (m_WorkBits[(pos + 1) * m_Width - 1] == value)
			return true;
	}

	// Bottom
	tmp = &m_WorkBits[(m_Height - 1) * m_Width];
	for (unsigned pos = 0; pos < m_Width; pos++, tmp++)
	{
		if (*tmp == value)
			return true;
	}
	return false;
}

bool Bmphandler::TissuevalueAtBoundary(tissuelayers_size_t idx, tissues_size_t value)
{
	// Top
	tissues_size_t* tissues = m_Tissuelayers[idx];
	tissues_size_t* tmp = &(tissues[0]);
	for (unsigned pos = 0; pos < m_Width; pos++, tmp++)
	{
		if (*tmp == value)
			return true;
	}

	// Left & right
	for (unsigned pos = 1; pos < (m_Height - 1); pos++)
	{
		if (tissues[pos * m_Width] == value)
			return true;
		else if (tissues[(pos + 1) * m_Width - 1] == value)
			return true;
	}

	// Bottom
	tmp = &(tissues[(m_Height - 1) * m_Width]);
	for (unsigned pos = 0; pos < m_Width; pos++, tmp++)
	{
		if (*tmp == value)
			return true;
	}
	return false;
}

float Bmphandler::AddSkin(unsigned i)
{
	Pair p;
	GetRange(&p);
	float setto;
	if (p.high <= 254.0f)
		setto = 255.0f;
	else if (p.low >= 1.0f)
		setto = 0.0f;
	else
	{
		setto = p.low;
		for (unsigned pos = 0; pos < m_Area; pos++)
		{
			if (m_WorkBits[pos] != p.high)
				setto = std::max(setto, m_WorkBits[pos]);
		}
		setto = (setto + p.high) / 2;
	}

	AddSkin(i, setto);
	return setto;
}

float Bmphandler::AddSkinOutside(unsigned i)
{
	Pair p;
	GetRange(&p);
	float setto;
	if (p.high <= 254.0f)
		setto = 255.0f;
	else if (p.low >= 1.0f)
		setto = 0.0f;
	else
	{
		setto = p.low;
		for (unsigned pos = 0; pos < m_Area; pos++)
		{
			if (m_WorkBits[pos] != p.high)
				setto = std::max(setto, m_WorkBits[pos]);
		}
		setto = (setto + p.high) / 2;
	}

	AddSkinOutside(i, setto);
	return setto;
}

void Bmphandler::FillSkin(int thicknessX, int thicknessY, tissues_size_t backgroundID, tissues_size_t skinID)
{
	//BL recommendation
	int skin_thick = thicknessX;

	std::vector<int> dims;
	dims.push_back(m_Width);
	dims.push_back(m_Height);

	//int backgroundID = (bmp_bits[0]==bmp_bits[area-1]) ? bmp_bits[0] : 0;

	double max_d = skin_thick == 1 ? 1.5 * skin_thick : 1.2 * skin_thick;
	std::vector<int> offsets;

	//Create the relative neighborhood
	for (int j = -skin_thick; j <= skin_thick; j++)
	{
		for (int i = -skin_thick; i <= skin_thick; i++)
		{
			if (sqrt(i * i + j * j) < max_d)
			{
				int shift = i + j * dims[0];
				if (shift != 0)
				{
					offsets.push_back(shift);
				}
			}
		}
	}

	tissues_size_t* tissues;
	if (!m_Tissuelayers.empty())
		tissues = m_Tissuelayers[0];

	bool preview_way = true;

	if (preview_way)
	{
		float* bmp1 = this->ReturnBmp();

		tissues_size_t* tissue1 = this->ReturnTissues(0);
		this->PushstackBmp();

		for (unsigned int i = 0; i < m_Area; i++)
		{
			bmp1[i] = (float)tissue1[i];
		}
		this->DeadReckoning((float)0);
		bmp1 = this->ReturnWork();

		for (int j = skin_thick; j + skin_thick < dims[1]; j++)
		{
			for (int i = skin_thick; i + skin_thick < dims[0]; i++)
			{
				int pos = i + j * dims[0];
				if (tissues[pos] == backgroundID)
				{
					//iterate through neighbors of pixel
					//if any of these are not
					for (int l = 0; l < offsets.size(); l++)
					{
						int idx = pos + offsets[l];
						assert(idx >= 0 && idx < m_Area);
						/*
						if( tissues[idx] != backgroundID && 
							tissues[idx] != skinID )
						{
							if( tissues[idx] != backgroundID && 
								tissues[idx] != skinID )
								work_bits[pos]=255.0f;
							else
								work_bits[pos]=0.0f;
						}
						*/
						if (tissues[idx] != backgroundID &&
								tissues[idx] != skinID)
							m_WorkBits[pos] = 255.0f;
					}
				}
			}
		}
		for (unsigned i = 0; i < m_Area; i++)
		{
			if (bmp1[i] < 0)
				bmp1[i] = 0;
			else
				bmp1[i] = 255.0f;
		}

		this->SetMode(2, false);
		this->PopstackBmp();
	}

	else
	{
		for (int j = skin_thick; j + skin_thick < dims[1]; j++)
		{
			for (int i = skin_thick; i + skin_thick < dims[0]; i++)
			{
				int pos = i + j * dims[0];
				if (tissues[pos] == backgroundID)
				{
					//iterate through neighbors of pixel
					//if any of these are not
					for (int l = 0; l < offsets.size(); l++)
					{
						int idx = pos + offsets[l];
						assert(idx >= 0 && idx < m_Area);

						if (tissues[idx] != backgroundID &&
								tissues[idx] != skinID)
						{
							tissues[pos] = skinID;
							break;
						}
					}
				}
			}
		}
	}
}

void Bmphandler::FloodExterior(float setto)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i3 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i3] == 0)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, false, 255.0f);

	i = m_Width + 3;
	i3 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 255.0f)
				m_WorkBits[i3] = setto;
			i++;
			i3++;
		}

		i += 2;
	}

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;

	free(results);
}

void Bmphandler::FloodExteriortissue(tissuelayers_size_t idx, tissues_size_t setto)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i3 = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tissues[i3] == 0)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, false, 255.0f);

	i = m_Width + 3;
	i3 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 255.0f)
				tissues[i3] = setto;
			i++;
			i3++;
		}

		i += 2;
	}

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;

	free(results);
}

void Bmphandler::FillUnassigned()
{
	Pair p;
	GetRange(&p);
	float setto;
	if (p.high <= 254.0f)
		setto = 255.0f;
	else if (p.low >= 1.0f)
		setto = 0.0f;
	else
	{
		setto = p.low;
		for (unsigned pos = 0; pos < m_Area; pos++)
		{
			if (m_WorkBits[pos] != p.high)
				setto = std::max(setto, m_WorkBits[pos]);
		}
		setto = (setto + p.high) / 2;
	}

	FillUnassigned(setto);
}

void Bmphandler::FillUnassigned(float setto)
{
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i3 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i3] == 0)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, false, 255.0f);

	unsigned pos = m_Width + 3;
	unsigned long pos1 = 0;

	for (unsigned short py = 0; py < m_Height; py++)
	{
		for (unsigned short px = 0; px < m_Width; px++)
		{
			if (results[pos] != 255.0f && m_WorkBits[pos1] == 0)
				m_WorkBits[pos1] = setto;
			pos++;
			pos1++;
		}
		pos += 2;
	}

	free(results);
}

void Bmphandler::FillUnassignedtissue(tissuelayers_size_t idx, tissues_size_t setto)
{
	std::vector<int> s;
	float* results =
			(float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i3 = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tissues[i3] == 0)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	HystereticGrowth(results, &s, m_Width + 2, m_Height + 2, false, 255.0f);

	unsigned pos = m_Width + 3;
	unsigned long pos1 = 0;

	for (unsigned short py = 0; py < m_Height; py++)
	{
		for (unsigned short px = 0; px < m_Width; px++)
		{
			if (results[pos] != 255.0f && tissues[pos1] == 0)
				tissues[pos1] = setto;
			pos++;
			pos1++;
		}
		pos += 2;
	}

	free(results);
}

void Bmphandler::AdaptiveFuzzy(Point p, float m1, float s1, float s2, float /* thresh */)
{
	ImageForestingTransformAdaptFuzzy af;
	af.FuzzyInit(m_Width, m_Height, m_BmpBits, p, m1, s1, s2);
	float* pf = af.ReturnPf();
	for (unsigned i = 0; i < m_Area; i++)
	{
		/*		if(pf[i]<1-thresh) work_bits[i]=255;
		else work_bits[i]=0;*/
		m_WorkBits[i] = pf[i] * 255;
	}
	m_Mode2 = 1;
}

void Bmphandler::FastMarching(Point p, float sigma, float thresh)
{
	unsigned char dummymode = m_Mode1;
	ImageForestingTransformFastMarching fm;

	float* dummy;
	float* lbl = m_Sliceprovide->GiveMe();
	Gaussian(sigma);
	dummy = lbl;
	lbl = m_BmpBits;
	m_BmpBits = dummy;
	SwapBmpwork();
	Sobel();
	dummy = lbl;
	lbl = m_BmpBits;
	m_BmpBits = dummy;

	for (unsigned i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = exp(-m_WorkBits[i] / thresh);
		lbl[i] = 0;
	}
	lbl[Pt2coord(p)] = 1;

	fm.FastmarchInit(m_Width, m_Height, m_WorkBits, lbl);
	float* pf = fm.ReturnPf();
	for (unsigned i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = pf[i];
	}
	Pair p1;
	GetRange(&p1);
	//	cout << p1.high << " " << p1.low << endl;
	//	scale_colors(p1);

	m_Sliceprovide->TakeBack(lbl);

	m_Mode1 = dummymode;
	m_Mode2 = 1;
}

ImageForestingTransformFastMarching*
		Bmphandler::FastmarchingInit(Point p, float sigma, float thresh)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;
	ImageForestingTransformFastMarching* fm =
			new ImageForestingTransformFastMarching;

	float* dummy;
	float* work_store = m_WorkBits;
	m_WorkBits = m_Sliceprovide->GiveMe();
	Gaussian(sigma);
	float* lbl = m_BmpBits;
	m_BmpBits = m_Sliceprovide->GiveMe();
	SwapBmpwork();
	Sobel();
	dummy = lbl;
	lbl = m_BmpBits;
	m_BmpBits = dummy;

	for (unsigned i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = exp(-m_WorkBits[i] / thresh);
		lbl[i] = 0;
	}
	lbl[Pt2coord(p)] = 1;

	fm->FastmarchInit(m_Width, m_Height, m_WorkBits, lbl);

	m_Sliceprovide->TakeBack(lbl);
	m_Sliceprovide->TakeBack(m_WorkBits);
	m_WorkBits = work_store;

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;

	return fm;
}

float Bmphandler::ExtractFeature(Point p1, Point p2)
{
	m_Fextract.Init(m_BmpBits, p1, p2, m_Width, m_Height);
	return m_Fextract.ReturnAverage();
}

float Bmphandler::ExtractFeaturework(Point p1, Point p2)
{
	m_Fextract.Init(m_WorkBits, p1, p2, m_Width, m_Height);
	return m_Fextract.ReturnAverage();
}

float Bmphandler::ReturnStdev()
{
	return m_Fextract.ReturnStddev();
	;
}

Pair Bmphandler::ReturnExtrema()
{
	Pair p;
	m_Fextract.ReturnExtrema(&p);
	return p;
}

void Bmphandler::Classify(short nrclasses, short dim, float** bits, float* weights, float* centers, float maxdist)
{
	short k;
	float dist, distmin;
	maxdist = maxdist * maxdist;
	short unsigned cindex;

	for (unsigned i = 0; i < m_Area; i++)
	{
		k = 0;
		distmin = 0;
		cindex = 0;
		for (short m = 0; m < dim; m++)
		{
			distmin += (bits[m][i] - centers[cindex]) *
								 (bits[m][i] - centers[cindex]) * weights[m];
			cindex++;
		}
		for (short l = 1; l < nrclasses; l++)
		{
			dist = 0;
			for (short m = 0; m < dim; m++)
			{
				dist += (bits[m][i] - centers[cindex]) *
								(bits[m][i] - centers[cindex]) * weights[m];
				cindex++;
			}
			if (dist < distmin)
			{
				distmin = dist;
				k = l;
			}
		}
		if (distmin < maxdist)
			m_WorkBits[i] = 255.0f * (k + 1) / nrclasses;
		else
			m_WorkBits[i] = 0;
	}

	m_Mode2 = 2;
}

void Bmphandler::Kmeans(short nrtissues, short dim, float** bits, float* weights, unsigned int iternr, unsigned int converge)
{
	float w = 0;
	for (short i = 0; i < dim; i++)
		w += weights[i];
	float* weightsnew = (float*)malloc(sizeof(float) * dim);
	if (w == 0)
		for (short i = 0; i < dim; i++)
			weightsnew[i] = 1.0f / dim;
	else
		for (short i = 0; i < dim; i++)
			weightsnew[i] = weights[i] / w;
	KMeans kmeans;
	kmeans.Init(m_Width, m_Height, nrtissues, dim, bits, weightsnew);
	kmeans.MakeIter(iternr, converge);
	kmeans.ReturnM(m_WorkBits);
	free(weightsnew);
	m_Mode2 = 2;
}

void Bmphandler::KmeansMhd(short nrtissues, short dim, std::vector<std::string> mhdfiles, unsigned short slicenr, float* weights, unsigned int iternr, unsigned int converge)
{
	float** bits = new float*[dim];
	for (unsigned short j = 0; j + 1 < dim; j++)
	{
		bits[j + 1] = m_Sliceprovide->GiveMe();
	}

	bits[0] = m_BmpBits;
	for (unsigned short i = 0; i + 1 < dim; i++)
	{
		if (!ImageReader::GetSlice(mhdfiles[i].c_str(), bits[i + 1], slicenr, m_Width, m_Height))
		{
			for (unsigned short j = 1; j < dim; j++)
				m_Sliceprovide->TakeBack(bits[j]);
			delete[] bits;
			return;
		}
	}

	float w = 0;
	for (short i = 0; i < dim; i++)
		w += weights[i];
	float* weightsnew = (float*)malloc(sizeof(float) * dim);
	if (w == 0)
		for (short i = 0; i < dim; i++)
			weightsnew[i] = 1.0f / dim;
	else
		for (short i = 0; i < dim; i++)
			weightsnew[i] = weights[i] / w;
	KMeans kmeans;
	kmeans.Init(m_Width, m_Height, nrtissues, dim, bits, weightsnew);
	kmeans.MakeIter(iternr, converge);
	kmeans.ReturnM(m_WorkBits);
	free(weightsnew);

	for (unsigned short j = 1; j < dim; j++)
		m_Sliceprovide->TakeBack(bits[j]);
	delete[] bits;

	m_Mode2 = 2;
}

void Bmphandler::KmeansPng(short nrtissues, short dim, std::vector<std::string> pngfiles, std::vector<int> exctractChannel, unsigned short slicenr, float* weights, unsigned int iternr, unsigned int converge, const std::string initCentersFile)
{
	float** bits = new float*[dim];
	for (unsigned short j = 0; j + 1 < dim; j++)
	{
		bits[j + 1] = m_Sliceprovide->GiveMe();
	}

	bits[0] = m_BmpBits;
	for (unsigned short i = 0; i + 1 < dim; i++)
	{
		if (!ChannelExtractor::getSlice(pngfiles[0].c_str(), bits[i + 1], exctractChannel[i], slicenr, m_Width, m_Height))
		{
			for (unsigned short j = 1; j < dim; j++)
				m_Sliceprovide->TakeBack(bits[j]);
			delete[] bits;
			return;
		}
	}

	float w = 0;
	for (short i = 0; i < dim; i++)
		w += weights[i];
	float* weightsnew = (float*)malloc(sizeof(float) * dim);
	if (w == 0)
		for (short i = 0; i < dim; i++)
			weightsnew[i] = 1.0f / dim;
	else
		for (short i = 0; i < dim; i++)
			weightsnew[i] = weights[i] / w;

	KMeans kmeans;
	if (!initCentersFile.empty())
	{
		float* centers = nullptr;
		int dimensions;
		int nr_classes;
		if (kmeans.GetCentersFromFile(initCentersFile, centers, dimensions, nr_classes))
		{
			dim = dimensions;
			nrtissues = nr_classes;
			kmeans.Init(m_Width, m_Height, nrtissues, dim, bits, weightsnew, centers);
		}
		else
		{
			QMessageBox msg_box;
			msg_box.setText("ERROR: reading centers initialization file.");
			msg_box.exec();
			return;
		}
	}
	else
	{
		kmeans.Init(m_Width, m_Height, nrtissues, dim, bits, weightsnew);
	}
	kmeans.MakeIter(iternr, converge);
	kmeans.ReturnM(m_WorkBits);
	free(weightsnew);

	for (unsigned short j = 1; j < dim; j++)
		m_Sliceprovide->TakeBack(bits[j]);
	delete[] bits;

	m_Mode2 = 2;
}

void Bmphandler::GammaMhd(short nrtissues, short dim, std::vector<std::string> mhdfiles, unsigned short slicenr, float* weights, float** centers, float* tol_f, float* tol_d, Pair pixelsize)
{
	float** bits = new float*[dim];
	for (unsigned short j = 0; j + 1 < dim; j++)
	{
		bits[j + 1] = m_Sliceprovide->GiveMe();
	}

	bits[0] = m_BmpBits;
	for (unsigned short i = 0; i + 1 < dim; i++)
	{
		if (!ImageReader::GetSlice(mhdfiles[i].c_str(), bits[i + 1], slicenr, m_Width, m_Height))
		{
			for (unsigned short j = 1; j < dim; j++)
				m_Sliceprovide->TakeBack(bits[j]);
			delete[] bits;
			return;
		}
	}

	MultidimensionalGamma mdg;
	mdg.Init(m_Width, m_Height, nrtissues, dim, bits, weights, centers, tol_f, tol_d, pixelsize.high, pixelsize.low);
	mdg.Execute();
	mdg.ReturnImage(m_WorkBits);

	for (unsigned short j = 1; j < dim; j++)
		m_Sliceprovide->TakeBack(bits[j]);
	delete[] bits;

	m_Mode2 = 2;
}

void Bmphandler::Em(short nrtissues, short dim, float** bits, float* weights, unsigned int iternr, unsigned int converge)
{
	ExpectationMaximization em;
	em.Init(m_Width, m_Height, nrtissues, dim, bits, weights);
	em.MakeIter(iternr, converge);
	em.Classify(m_WorkBits);
	m_Mode2 = 2;
}

void Bmphandler::Cannylevelset(float* initlev, float f, float sigma, float thresh_low, float thresh_high, float epsilon, float stepsize, unsigned nrsteps, unsigned reinitfreq)
{
	unsigned char dummymode = m_Mode1;
	float* tmp = m_Sliceprovide->GiveMe();
	float* dummy;

	CannyLine(sigma, thresh_low, thresh_high);
	dummy = tmp;
	tmp = m_BmpBits;
	m_BmpBits = dummy;
	SwapBmpwork();
	//	dead_reckoning_squared(255.0f);
	//	IFT_distance1(255.0f);
	DeadReckoning(255.0f);
	dummy = tmp;
	tmp = m_BmpBits;
	m_BmpBits = dummy;
	//	for(unsigned i=0;i<area;i++) work_bits[i]=work_bits[i];
	Pair p;
	GetRange(&p);
	//	cout << p.low << " " << p.high << endl;
	p.low = p.low * 10;
	p.high = p.high * 10;
	//	p.low=p.high*10;
	ScaleColors(p);
	GetRange(&p);
	//	cout <<"+"<< p.low << " " << p.high << endl;

	//	for(unsigned i=0;i<area;i++) work_bits[i]=0.0f;
	for (unsigned i = 0; i < m_Area; i++)
		tmp[i] = 1.0f;

	Levelset levset;
	levset.Init(m_Height, m_Width, initlev, f, tmp, m_WorkBits, 0.0f, epsilon, stepsize);
	levset.Iterate(nrsteps, reinitfreq);
	levset.ReturnLevelset(m_WorkBits);
	//	SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testdump1.bmp");

	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 0;
	//	threshold(thresh);
	for (unsigned i = 0; i < m_Area; i++)
	{
		if (m_WorkBits[i] < 0)
			//			work_bits[i]=tmp[i];
			m_WorkBits[i] = 0;
		else
			//			work_bits[i]=256-tmp[i];
			m_WorkBits[i] = 256;
	}
	m_Sliceprovide->TakeBack(tmp);

	m_Mode1 = dummymode;
	m_Mode2 = 2;
}

void Bmphandler::Threshlevelset(float thresh_low, float thresh_high, float epsilon, float stepsize, unsigned nrsteps, unsigned reinitfreq)
{
	float mean = (thresh_high + thresh_low) / 2;
	float halfdiff = (thresh_high - thresh_low) / 2;
	for (unsigned i = 0; i < m_Area; ++i)
		m_WorkBits[i] = 1 - abs(m_BmpBits[i] - mean) / halfdiff;

	Levelset levset;
	Point pt;
	pt.px = 376;
	pt.py = 177;
	/*	Pt.px=215;
	Pt.py=266;*/
	float* tmp = m_Sliceprovide->GiveMe();
	for (unsigned i = 0; i < m_Area; i++)
		tmp[i] = 0;
	levset.Init(m_Height, m_Width, pt, m_WorkBits, tmp, 1.0f, epsilon, stepsize);
	levset.Iterate(nrsteps, reinitfreq);
	levset.ReturnLevelset(m_WorkBits);
	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 0;
	for (unsigned i = 0; i < m_Area; i++)
	{
		if (m_WorkBits[i] < 0)
			//			work_bits[i]=tmp[i];
			m_WorkBits[i] = 0.0f;
		else
			//			work_bits[i]=256-tmp[i];
			m_WorkBits[i] = 256.0f;
	}

	m_Sliceprovide->TakeBack(tmp);

	m_Mode2 = 2;
}

unsigned Bmphandler::PushstackBmp()
{
	float* bits = m_Sliceprovide->GiveMe();

	for (unsigned i = 0; i < m_Area; ++i)
	{
		bits[i] = m_BmpBits[i];
	}

	bits_stack.push_back(bits);
	stackindex.push_back(stackcounter);
	mode_stack.push_back(m_Mode1);

	return stackcounter++;
}

unsigned Bmphandler::PushstackWork()
{
	float* bits = m_Sliceprovide->GiveMe();

	for (unsigned i = 0; i < m_Area; ++i)
	{
		bits[i] = m_WorkBits[i];
	}

	bits_stack.push_back(bits);
	stackindex.push_back(stackcounter);
	mode_stack.push_back(m_Mode2);

	return stackcounter++;
}

bool Bmphandler::Savestack(unsigned i, const char* filename)
{
	std::list<float*>::iterator it = bits_stack.begin();
	std::list<unsigned>::iterator it1 = stackindex.begin();
	std::list<unsigned char>::iterator it2 = mode_stack.begin();
	while (it != bits_stack.end() && (*it1) != i)
	{
		it++;
		it1++;
		it2++;
	}
	if (it != bits_stack.end())
	{
		FILE* fp = fopen(filename, "wb");
		if (fp == nullptr)
			return false;
		if (fwrite(&m_Width, 1, sizeof(short unsigned), fp) <
				sizeof(short unsigned))
		{
			fclose(fp);
			return false;
		}
		if (fwrite(&m_Height, 1, sizeof(short unsigned), fp) <
				sizeof(short unsigned))
		{
			fclose(fp);
			return false;
		}
		unsigned int bitsize = m_Width * (unsigned)m_Height * sizeof(float);
		if (fwrite(*it, 1, bitsize, fp) < bitsize)
		{
			fclose(fp);
			return false;
		}
		if (fwrite(&(*it2), 1, sizeof(unsigned char), fp) <
				sizeof(unsigned char))
		{
			fclose(fp);
			return false;
		}
		fclose(fp);
		return true;
	}

	return false;
}

unsigned Bmphandler::Loadstack(const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if (fp == nullptr)
		return 123456;

	short unsigned width1, height1;
	if (fread(&width1, 1, sizeof(short unsigned), fp) <
					sizeof(short unsigned) ||
			width1 != m_Width)
	{
		fclose(fp);
		return 123456;
	}
	if (fread(&height1, 1, sizeof(short unsigned), fp) <
					sizeof(short unsigned) ||
			height1 != m_Height)
	{
		fclose(fp);
		return 123456;
	}
	unsigned int bitsize = m_Width * (unsigned)m_Height * sizeof(float);
	float* bits = m_Sliceprovide->GiveMe();
	if (fread(bits, 1, bitsize, fp) < bitsize)
	{
		fclose(fp);
		return 123456;
	}
	unsigned char mode1;
	if (fread(&mode1, 1, sizeof(unsigned char), fp) < sizeof(unsigned char))
	{
		fclose(fp);
		return 123456;
	}

	bits_stack.push_back(bits);
	stackindex.push_back(stackcounter);
	mode_stack.push_back(mode1);

	fclose(fp);
	return stackcounter++;
}

unsigned Bmphandler::PushstackTissue(tissuelayers_size_t idx, tissues_size_t tissuenr)
{
	float* bits = m_Sliceprovide->GiveMe();

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned i = 0; i < m_Area; ++i)
	{
		if (tissues[i] == tissuenr)
			bits[i] = 255.0;
		else
			bits[i] = 0;
	}

	bits_stack.push_back(bits);
	stackindex.push_back(stackcounter);
	mode_stack.push_back(2);

	return stackcounter++;
}

unsigned Bmphandler::PushstackHelp()
{
	float* bits = m_Sliceprovide->GiveMe();

	for (unsigned i = 0; i < m_Area; ++i)
	{
		bits[i] = m_HelpBits[i];
	}

	bits_stack.push_back(bits);
	stackindex.push_back(stackcounter);
	mode_stack.push_back(0);

	return stackcounter++;
}

void Bmphandler::Removestack(unsigned i)
{
	/*	if(i<bits_stack.size()){
		std::list<float *>::iterator it=bits_stack.begin();
		std::list<unsigned>::iterator it1=stackindex.begin();
		while(i>0) {it++; it1++; i--;}
		bits_stack.erase(it);
		stackindex.erase(it1);
	}*/

	std::list<float*>::iterator it = bits_stack.begin();
	std::list<unsigned>::iterator it1 = stackindex.begin();
	std::list<unsigned char>::iterator it2 = mode_stack.begin();
	while (it != bits_stack.end() && (*it1) != i)
	{
		it++;
		it1++;
		it2++;
	}
	if (it != bits_stack.end())
	{
		m_Sliceprovide->TakeBack(*it);
		bits_stack.erase(it);
		stackindex.erase(it1);
		mode_stack.erase(it2);
	}
}

void Bmphandler::GetstackBmp(unsigned i)
{
	//	sliceprovide->take_back(bmp_bits);

	std::list<float*>::iterator it = bits_stack.begin();
	std::list<unsigned>::iterator it1 = stackindex.begin();
	std::list<unsigned char>::iterator it2 = mode_stack.begin();

	while (it != bits_stack.end() && (*it1) != i)
	{
		it++;
		it1++;
		it2++;
	}
	if (it != bits_stack.end())
	{
		//		bmp_bits=*it;
		for (unsigned i = 0; i < m_Area; i++)
			m_BmpBits[i] = (*it)[i];
	}

	m_Mode1 = (*it2);
}

void Bmphandler::GetstackWork(unsigned i)
{
	std::list<float*>::iterator it = bits_stack.begin();
	std::list<unsigned>::iterator it1 = stackindex.begin();
	std::list<unsigned char>::iterator it2 = mode_stack.begin();

	while (it != bits_stack.end() && (*it1) != i)
	{
		it++;
		it1++;
		it2++;
	}
	if (it != bits_stack.end())
	{
		//		work_bits=*it;
		for (unsigned i = 0; i < m_Area; i++)
			m_WorkBits[i] = (*it)[i];
	}

	m_Mode2 = (*it2);
}

void Bmphandler::GetstackTissue(tissuelayers_size_t idx, unsigned i, tissues_size_t tissuenr, bool override)
{
	//	sliceprovide->take_back(work_bits);

	std::list<float*>::iterator it = bits_stack.begin();
	std::list<unsigned>::iterator it1 = stackindex.begin();
	std::list<unsigned char>::iterator it2 = mode_stack.begin();

	while (it != bits_stack.end() && (*it1) != i)
	{
		it++;
		it1++;
		it2++;
	}
	tissues_size_t* tissues = m_Tissuelayers[idx];
	if (it != bits_stack.end())
	{
		if (override)
		{
			for (unsigned i = 0; i < m_Area; i++)
			{
				if (((*it)[i] != 0) &&
						(!TissueInfos::GetTissueLocked(tissues[i])))
					tissues[i] = tissuenr;
			}
		}
		else
		{
			for (unsigned i = 0; i < m_Area; i++)
			{
				if (((*it)[i] != 0) && (tissues[i] == 0))
					tissues[i] = tissuenr;
			}
		}
	}
}

void Bmphandler::GetstackHelp(unsigned i)
{
	//	sliceprovide->take_back(help_bits);

	std::list<float*>::iterator it = bits_stack.begin();
	std::list<unsigned>::iterator it1 = stackindex.begin();

	while (it != bits_stack.end() && (*it1) != i)
	{
		it++;
		it1++;
	}
	if (it != bits_stack.end())
	{
		//		help_bits=*it;
		for (unsigned i = 0; i < m_Area; i++)
			m_HelpBits[i] = (*it)[i];
	}
}

float* Bmphandler::Getstack(unsigned i, unsigned char& mode)
{
	std::list<float*>::iterator it = bits_stack.begin();
	std::list<unsigned>::iterator it1 = stackindex.begin();
	std::list<unsigned char>::iterator it2 = mode_stack.begin();

	while (it != bits_stack.end() && (*it1) != i)
	{
		it++;
		it1++;
		it2++;
	}

	if (it != bits_stack.end())
	{
		mode = *it2;
		return *it;
	}
	else
	{
		return nullptr;
	}
}

void Bmphandler::PopstackBmp()
{
	if (!bits_stack.empty())
	{
		m_Sliceprovide->TakeBack(m_BmpBits);
		m_BmpBits = bits_stack.back();
		m_Mode1 = mode_stack.back();
		bits_stack.pop_back();
		stackindex.pop_back();
		mode_stack.pop_back();
	}
}

void Bmphandler::PopstackWork()
{
	if (!bits_stack.empty())
	{
		m_Sliceprovide->TakeBack(m_WorkBits);
		m_WorkBits = bits_stack.back();
		m_Mode2 = mode_stack.back();
		bits_stack.pop_back();
		stackindex.pop_back();
		mode_stack.pop_back();
	}
}

void Bmphandler::PopstackHelp()
{
	if (!bits_stack.empty())
	{
		m_Sliceprovide->TakeBack(m_HelpBits);
		m_HelpBits = bits_stack.back();
		bits_stack.pop_back();
		stackindex.pop_back();
		mode_stack.pop_back();
	}
}

bool Bmphandler::Isloaded() const { return m_Loaded; }

void Bmphandler::ClearTissue(tissuelayers_size_t idx)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	std::fill(tissues, tissues + m_Area, 0);
}

bool Bmphandler::HasTissue(tissuelayers_size_t idx, tissues_size_t tissuetype)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (tissues[i] == tissuetype)
		{
			return true;
		}
	}
	return false;
}

void Bmphandler::Add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, float f, bool override)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	if (override)
	{
		for (unsigned int i = 0; i < m_Area; i++)
			if (m_WorkBits[i] == f &&
					TissueInfos::GetTissueLocked(tissues[i]) == false)
			{
				tissues[i] = tissuetype;
			}
	}
	else
	{
		for (unsigned int i = 0; i < m_Area; i++)
			if (m_WorkBits[i] == f && tissues[i] == 0)
			{
				tissues[i] = tissuetype;
			}
		//for(unsigned int i=0;i<area;i++) if(work_bits[i]==f&&tissuelocked[tissues[i]]==false) {tissues[i]=tissuetype;}
	}
}

void Bmphandler::Add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, bool* mask, bool override)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	if (override)
	{
		for (unsigned int i = 0; i < m_Area; i++)
			if (mask[i] && TissueInfos::GetTissueLocked(tissues[i]) == false)
			{
				tissues[i] = tissuetype;
			}
	}
	else
	{
		for (unsigned int i = 0; i < m_Area; i++)
			if (mask[i] && tissues[i] == 0)
			{
				tissues[i] = tissuetype;
			}
		//for(unsigned int i=0;i<area;i++) if(work_bits[i]==f&&tissuelocked[tissues[i]]==false) {tissues[i]=tissuetype;}
	}
}

void Bmphandler::Add2tissueConnected(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p, bool override)
{
	unsigned position = Pt2coord(p);
	float f = m_WorkBits[position];
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i1] == f && (tissues[i1] == 0 || (override && !TissueInfos::GetTissueLocked(tissues[i1]))))
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	std::vector<int> s;
	s.push_back(position % m_Width + 1 + (position / m_Width + 1) * (m_Width + 2));
	if (results[s.back()] == -1)
		results[s.back()] = 255.0f;

	int w = m_Width + 2;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();
		if (results[i - 1] == -1)
		{
			results[i - 1] = 255.0f;
			s.push_back(i - 1);
		}
		if (results[i + 1] == -1)
		{
			results[i + 1] = 255.0f;
			s.push_back(i + 1);
		}
		if (results[i - w] == -1)
		{
			results[i - w] = 255.0f;
			s.push_back(i - w);
		}
		if (results[i + w] == -1)
		{
			results[i + w] = 255.0f;
			s.push_back(i + w);
		}
	}

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 255.0f)
				tissues[i2] = tissuetype;

			i++;
			i2++;
		}

		i += 2;
	}

	free(results);
}

void Bmphandler::Add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p, bool override)
{
	float f = WorkPt(p);
	tissues_size_t* tissues = m_Tissuelayers[idx];
	if (override)
	{
		for (unsigned int i = 0; i < m_Area; i++)
			if (m_WorkBits[i] == f && !TissueInfos::GetTissueLocked(tissues[i]))
				tissues[i] = tissuetype;
	}
	else
	{
		for (unsigned int i = 0; i < m_Area; i++)
			if (m_WorkBits[i] == f && tissues[i] == 0)
				tissues[i] = tissuetype;
	}
}

void Bmphandler::Add2tissueThresh(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p)
{
	float f = WorkPt(p);
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
		if (m_WorkBits[i] >= f)
			tissues[i] = tissuetype;
}

void Bmphandler::SubtractTissue(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p)
{
	float f = WorkPt(p);
	SubtractTissue(idx, tissuetype, f);
}

void Bmphandler::SubtractTissueConnected(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p)
{
	unsigned position = Pt2coord(p);
	std::vector<int> s;

	float f = m_WorkBits[position];
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i1] == f && tissues[i1] == tissuetype)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	s.push_back(position % m_Width + 1 + (position / m_Width + 1) * (m_Width + 2));
	if (results[s.back()] == -1)
		results[s.back()] = 255.0f;

	int w = m_Width + 2;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();
		if (results[i - 1] == -1)
		{
			results[i - 1] = 255.0f;
			s.push_back(i - 1);
		}
		if (results[i + 1] == -1)
		{
			results[i + 1] = 255.0f;
			s.push_back(i + 1);
		}
		if (results[i - w] == -1)
		{
			results[i - w] = 255.0f;
			s.push_back(i - w);
		}
		if (results[i + w] == -1)
		{
			results[i + w] = 255.0f;
			s.push_back(i + w);
		}
	}

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 255.0f)
				tissues[i2] = 0;

			i++;
			i2++;
		}

		i += 2;
	}

	free(results);
}

void Bmphandler::SubtractTissue(tissuelayers_size_t idx, tissues_size_t tissuetype, float f)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
		if (m_WorkBits[i] == f && tissues[i] == tissuetype)
			tissues[i] = 0;
}

void Bmphandler::Change2maskConnectedwork(bool* mask, Point p, bool addorsub)
{
	unsigned position = Pt2coord(p);
	std::vector<int> s;

	float f = m_WorkBits[position];
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[i1] == f)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	s.push_back(position % m_Width + 1 + (position / m_Width + 1) * (m_Width + 2));
	if (results[s.back()] == -1)
		results[s.back()] = 255.0f;

	int w = m_Width + 2;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();
		if (results[i - 1] == -1)
		{
			results[i - 1] = 255.0f;
			s.push_back(i - 1);
		}
		if (results[i + 1] == -1)
		{
			results[i + 1] = 255.0f;
			s.push_back(i + 1);
		}
		if (results[i - w] == -1)
		{
			results[i - w] = 255.0f;
			s.push_back(i - w);
		}
		if (results[i + w] == -1)
		{
			results[i + w] = 255.0f;
			s.push_back(i + w);
		}
	}

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 255.0f)
				mask[i2] = addorsub;

			i++;
			i2++;
		}

		i += 2;
	}

	free(results);
}

void Bmphandler::Change2maskConnectedtissue(tissuelayers_size_t idx, bool* mask, Point p, bool addorsub)
{
	unsigned position = Pt2coord(p);
	std::vector<int> s;

	tissues_size_t* tissues = m_Tissuelayers[idx];
	tissues_size_t f = tissues[position];
	float* results = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tissues[i1] == f)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		results[j] = results[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		results[j] = results[j + m_Width + 1] = 0;

	s.push_back(position % m_Width + 1 + (position / m_Width + 1) * (m_Width + 2));
	if (results[s.back()] == -1)
		results[s.back()] = 255.0f;

	int w = m_Width + 2;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();
		if (results[i - 1] == -1)
		{
			results[i - 1] = 255.0f;
			s.push_back(i - 1);
		}
		if (results[i + 1] == -1)
		{
			results[i + 1] = 255.0f;
			s.push_back(i + 1);
		}
		if (results[i - w] == -1)
		{
			results[i - w] = 255.0f;
			s.push_back(i - w);
		}
		if (results[i + w] == -1)
		{
			results[i + w] = 255.0f;
			s.push_back(i + w);
		}
	}

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (results[i] == 255.0f)
				mask[i2] = addorsub;

			i++;
			i2++;
		}

		i += 2;
	}

	free(results);
}

void Bmphandler::Tissue2work(tissuelayers_size_t idx, const std::vector<float>& mask)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = mask.at(tissues[i]);
	}

	m_Mode2 = 2;
}

void Bmphandler::Tissue2work(tissuelayers_size_t idx)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		m_WorkBits[i] = (float)tissues[i];
	}

	m_Mode2 = 2;
}

void Bmphandler::Cleartissue(tissuelayers_size_t idx, tissues_size_t tissuetype)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (tissues[i] == tissuetype)
			tissues[i] = 0;
	}
}

void Bmphandler::CapTissue(tissues_size_t maxval)
{
	for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned int i = 0; i < m_Area; i++)
		{
			if (tissues[i] > maxval)
				tissues[i] = 0;
		}
	}
}

void Bmphandler::Cleartissues(tissuelayers_size_t idx)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		tissues[i] = 0;
	}
}

void Bmphandler::Cleartissuesall()
{
	for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned int i = 0; i < m_Area; i++)
		{
			tissues[i] = 0;
		}
	}
}

void Bmphandler::Erasework(bool* mask)
{
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (mask[i])
			m_WorkBits[i] = 0;
	}
}

void Bmphandler::Erasetissue(tissuelayers_size_t idx, bool* mask)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (mask[i] && (!TissueInfos::GetTissueLocked(tissues[i])))
			tissues[i] = 0;
	}
}

void Bmphandler::Floodwork(bool* mask)
{
	unsigned position;
	std::queue<unsigned int> s;

	float* values = (float*)malloc(sizeof(float) * (m_Area + 2 * m_Width + 2 * m_Height + 4));
	bool* bigmask = (bool*)malloc(sizeof(bool) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			values[i] = m_WorkBits[i1];
			bigmask[i] = mask[i1];
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		bigmask[j] = bigmask[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = false;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		bigmask[j] = bigmask[j + m_Width + 1] = false;

	int w = m_Width + 2;

	position = m_Width + 3;
	for (int j = 1; j < m_Height; j++)
	{
		for (int k = 1; k < m_Width; k++)
		{
			if (bigmask[position] != bigmask[position + 1])
			{
				if (bigmask[position])
				{
					s.push(position + 1);
				}
				else
				{
					s.push(position);
				}
			}
			if (bigmask[position] != bigmask[position + w])
			{
				if (bigmask[position])
				{
					s.push(position + w);
				}
				else
				{
					s.push(position);
				}
			}
			position++;
		}
		position += 3;
	}

	while (!s.empty())
	{
		i = s.front();
		s.pop();
		if (bigmask[i - 1])
		{
			bigmask[i - 1] = false;
			values[i - 1] = values[i];
			s.push(i - 1);
		}
		if (bigmask[i + 1])
		{
			bigmask[i + 1] = false;
			values[i + 1] = values[i];
			s.push(i + 1);
		}
		if (bigmask[i - w])
		{
			bigmask[i - w] = false;
			values[i - w] = values[i];
			s.push(i - w);
		}
		if (bigmask[i + w])
		{
			bigmask[i + w] = false;
			values[i + w] = values[i];
			s.push(i + w);
		}
	}

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (mask[i2])
				m_WorkBits[i2] = values[i];
			i++;
			i2++;
		}
		i += 2;
	}

	free(values);
	free(bigmask);
}

void Bmphandler::Floodtissue(tissuelayers_size_t idx, bool* mask)
{
	unsigned position;
	std::queue<unsigned int> s;

	tissues_size_t* values = (tissues_size_t*)malloc(sizeof(tissues_size_t) * (m_Area + 2 * m_Width + 2 * m_Height + 4));
	bool* bigmask = (bool*)malloc(sizeof(bool) * (m_Area + 2 * m_Width + 2 * m_Height + 4));

	int i = m_Width + 3;
	int i1 = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			values[i] = tissues[i1];
			bigmask[i] = mask[i1];
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < m_Width + 2; j++)
		bigmask[j] = bigmask[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = false;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		bigmask[j] = bigmask[j + m_Width + 1] = false;

	int w = m_Width + 2;

	position = m_Width + 3;
	for (int j = 1; j < m_Height; j++)
	{
		for (int k = 1; k < m_Width; k++)
		{
			if (bigmask[position] != bigmask[position + 1])
			{
				if (bigmask[position])
				{
					s.push(position + 1);
				}
				else
				{
					s.push(position);
				}
			}
			if (bigmask[position] != bigmask[position + w])
			{
				if (bigmask[position])
				{
					s.push(position + w);
				}
				else
				{
					s.push(position);
				}
			}
			position++;
		}
		position += 3;
	}

	while (!s.empty())
	{
		i = s.front();
		s.pop();
		if (bigmask[i - 1])
		{
			bigmask[i - 1] = false;
			values[i - 1] = values[i];
			s.push(i - 1);
		}
		if (bigmask[i + 1])
		{
			bigmask[i + 1] = false;
			values[i + 1] = values[i];
			s.push(i + 1);
		}
		if (bigmask[i - w])
		{
			bigmask[i - w] = false;
			values[i - w] = values[i];
			s.push(i - w);
		}
		if (bigmask[i + w])
		{
			bigmask[i + w] = false;
			values[i + w] = values[i];
			s.push(i + w);
		}
	}

	i = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (mask[i2])
				tissues[i2] = values[i];
			i++;
			i2++;
		}
		i += 2;
	}

	free(values);
	free(bigmask);
}

void Bmphandler::CorrectOutline(float f, std::vector<Point>* newline)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;

	std::vector<std::vector<Point>> vv_pouter, vv_pinner;
	std::vector<Point> limit1, limit2;
	std::vector<Point>::iterator it, itold, start_p, end_p;
	std::vector<std::vector<Point>>::iterator vv_pit;

	GetContours(f, &vv_pouter, &vv_pinner, 1);

	vv_pouter.insert(vv_pouter.end(), vv_pinner.begin(), vv_pinner.end());

	ImageForestingTransformDistance if_tdist;
	if_tdist.DistanceInit(m_Width, m_Height, f, m_WorkBits);
	if_tdist.ReturnPath(*(newline->begin()), &limit1);

	it = newline->end();
	it--;
	if_tdist.ReturnPath(*it, &limit2);

	int counter1, counter2;

	Point p1, p2;
	it = limit1.end();
	it--;
	p1 = *it;
	it = limit2.end();
	it--;
	p2 = *it;

	bool found = false;
	vv_pit = vv_pouter.begin();
	while (!found && vv_pit != vv_pouter.end())
	{
		it = vv_pit->begin();
		counter1 = 0;
		while (!found && it != vv_pit->end())
		{
			//			if(it->px==(limit1.begin())->px&&it->py==(limit1.begin())->py){
			if (p1.px == it->px && p1.py == it->py)
			{
				found = true;
				start_p = it;
				counter1--;
			}
			it++;
			counter1++;
		}
		vv_pit++;
	}

	if (found)
	{
		vv_pit--;
		found = false;
		it = vv_pit->begin();
		counter2 = 0;
		while (!found && it != vv_pit->end())
		{
			//			if(it->px==(limit2.begin())->px&&it->py==(limit2.begin())->py){
			if (p2.px == it->px && p2.py == it->py)
			{
				end_p = it;
				found = true;
				counter2--;
			}
			it++;
			counter2++;
		}

		if (found)
		{
			bool order = true;

			if (counter2 < counter1)
			{
				int dummy = counter1;
				counter1 = counter2;
				counter2 = dummy;
				it = start_p;
				start_p = end_p;
				end_p = it;
				order = false;
			}

			std::vector<Point> oldline, oldline1;
			if (counter2 - counter1 + 1 <
					(int)vv_pit->size() - counter2 + counter1)
			{
				oldline.insert(oldline.begin(), start_p, ++end_p);
				oldline1.insert(oldline1.begin(), start_p, end_p);
			}
			else
			{
				order = !order;
				oldline.insert(oldline.begin(), end_p, vv_pit->end());
				oldline.insert(oldline.end(), vv_pit->begin(), ++start_p);
				oldline1.insert(oldline1.begin(), end_p, vv_pit->end());
				oldline1.insert(oldline1.end(), vv_pit->begin(), start_p);
			}

			oldline.insert(oldline.end(), newline->begin(), newline->end());
			oldline.insert(oldline.end(), limit1.begin(), limit1.end());
			oldline.insert(oldline.end(), limit2.begin(), limit2.end());

			std::vector<Point> change_pts;
			Point p;
			bool in, in1;
			it = newline->begin();
			in = (m_WorkBits[unsigned(m_Width) * it->py + it->px] == f);
			in1 = !in;
			itold = it;
			it++;

			//			FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");

			while (it != newline->end())
			{
				if ((m_WorkBits[unsigned(m_Width) * it->py + it->px] == f) != in)
				{
					if (in)
					{
						change_pts.push_back(*itold);
						//						fprintf(fp3,"*%i %i\n",(int)itold->px,(int)itold->py);
					}
					else
					{
						change_pts.push_back(*it);
						//						fprintf(fp3,"*%i %i\n",(int)it->px,(int)it->py);
					}

					/*					if(work_bits[unsigned(width)*itold->py+it->px]==f){
						p.px=it->px;
						p.py=itold->py;
					} else {
						p.px=itold->px;
						p.py=it->py;
					}
					changePts.push_back(p);*/

					p.px = it->px;
					p.py = itold->py;
					change_pts.push_back(p);
					p.px = itold->px;
					p.py = it->py;
					change_pts.push_back(p);

					in = !in;
				}
				itold++;
				it++;
			}
			p.px = m_Width;
			p.py = m_Height;
			change_pts.push_back(p);
			change_pts.push_back(p);
			change_pts.push_back(p);

			float* bkp = m_WorkBits;
			m_WorkBits = m_Sliceprovide->GiveMe();

			FillContour(&oldline, true);

			for (unsigned i = 0; i < m_Area; i++)
			{
				if (m_WorkBits[i] == 255.0f)
				{
					if (bkp[i] == f)
						bkp[i] = 0.0;
					else
						bkp[i] = f;
				}
			}

			for (it = newline->begin(); it != newline->end(); it++)
				bkp[unsigned(m_Width) * it->py + it->px] = f;

			//			it=newline->begin();
			//			in=(work_bits[unsigned(width)*it->py+it->px]!=f);
			Point p1, p2, p3;
			it = change_pts.begin();
			p1 = *it;
			it++;
			p2 = *it;
			it++;
			p3 = *it;
			it++;
			bool dontdraw = false;
			if (order)
			{
				for (std::vector<Point>::iterator it1 = oldline1.begin();
						 it1 != oldline1.end(); it1++)
				{
					if (it1->px == p1.px && it1->py == p1.py)
					{
						//						fprintf(fp3,"a%i %i\n",(int)it1->px,(int)it1->py);
						in1 = !in1;
						p1 = *it;
						it++;
						p2 = *it;
						it++;
						p3 = *it;
						it++;
						it1--;
						dontdraw = true;
					}
					else if (!in1 && ((it1->px == p2.px && it1->py == p2.py) ||
															 (it1->px == p3.px && it1->py == p3.py)))
					{
						it1++;
						if (it1 == oldline1.end() || it1->px != p1.px ||
								it1->py != p1.py)
						{
							//							fprintf(fp3,"b%i %i\n",(int)it1->px,(int)it1->py);
							in1 = true;
							p1 = *it;
							it++;
							p2 = *it;
							it++;
							p3 = *it;
							it++;
						}
						it1--;
						dontdraw = true;
					}
					else if (in1 && ((it1->px == p2.px && it1->py == p2.py) ||
															(it1->px == p3.px && it1->py == p3.py)))
					{
						bkp[unsigned(m_Width) * it1->py + it1->px] = f;
						//						fprintf(fp3,"c%i %i\n",(int)it1->px,(int)it1->py);
						in1 = false;
						p1 = *it;
						it++;
						p2 = *it;
						it++;
						p3 = *it;
						it++;
					}

					if (dontdraw)
						dontdraw = false;
					else if (in1)
						bkp[unsigned(m_Width) * it1->py + it1->px] = f;
				}
			}
			else
			{
				for (std::vector<Point>::reverse_iterator it1 = oldline1.rbegin();
						 it1 != oldline1.rend(); it1++)
				{
					if (it1->px == p1.px && it1->py == p1.py)
					{
						//						fprintf(fp3,"d%i %i\n",(int)it1->px,(int)it1->py);
						in1 = !in1;
						p1 = *it;
						it++;
						p2 = *it;
						it++;
						p3 = *it;
						it++;
						it1--;
						dontdraw = true;
					}
					else if (!in1 && ((it1->px == p2.px && it1->py == p2.py) ||
															 (it1->px == p3.px && it1->py == p3.py)))
					{
						it1++;
						if (it1 == oldline1.rend() || it1->px != p1.px ||
								it1->py != p1.py)
						{
							//							fprintf(fp3,"e%i %i\n",(int)it1->px,(int)it1->py);
							in1 = true;
							p1 = *it;
							it++;
							p2 = *it;
							it++;
							p3 = *it;
							it++;
						}
						it1--;
						dontdraw = true;
					}
					else if (in1 && ((it1->px == p2.px && it1->py == p2.py) ||
															(it1->px == p3.px && it1->py == p3.py)))
					{
						bkp[unsigned(m_Width) * it1->py + it1->px] = f;
						//						fprintf(fp3,"f%i %i\n",(int)it1->px,(int)it1->py);
						in1 = false;
						p1 = *it;
						it++;
						p2 = *it;
						it++;
						p3 = *it;
						it++;
					}

					if (dontdraw)
						dontdraw = false;
					else if (in1)
						bkp[unsigned(m_Width) * it1->py + it1->px] = f;
				}
			}

			//			fclose(fp3);

			m_Sliceprovide->TakeBack(m_WorkBits);
			m_WorkBits = bkp;
		}
	}

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

void Bmphandler::CorrectOutlinetissue(tissuelayers_size_t idx, tissues_size_t f1, std::vector<Point>* newline)
{
	unsigned char dummymode1 = m_Mode1;
	unsigned char dummymode2 = m_Mode2;
	float f = float(f1);

	PushstackWork();
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned ineu = 0; ineu < m_Area; ineu++)
		m_WorkBits[ineu] = (float)tissues[ineu];

	std::vector<std::vector<Point>> vv_pouter, vv_pinner;
	std::vector<Point> limit1, limit2;
	std::vector<Point>::iterator it, itold, start_p, end_p;
	std::vector<std::vector<Point>>::iterator vv_pit;

	GetContours(f, &vv_pouter, &vv_pinner, 1);

	vv_pouter.insert(vv_pouter.end(), vv_pinner.begin(), vv_pinner.end());

	ImageForestingTransformDistance if_tdist;
	if_tdist.DistanceInit(m_Width, m_Height, f, m_WorkBits);
	// BL here I think we get closest connection from start/end point
	// to contour of selected tissue 'f'.
	if_tdist.ReturnPath(newline->front(), &limit1);
	if_tdist.ReturnPath(newline->back(), &limit2);

	int counter1, counter2;

	Point p1 = limit1.back();
	Point p2 = limit2.back();

	bool found = false;
	vv_pit = vv_pouter.begin();
	while (!found && vv_pit != vv_pouter.end())
	{
		it = vv_pit->begin();
		counter1 = 0;
		while (!found && it != vv_pit->end())
		{
			if (p1.px == it->px && p1.py == it->py)
			{
				found = true;
				start_p = it;
				counter1--;
			}
			it++;
			counter1++;
		}
		vv_pit++;
	}

	if (found)
	{
		vv_pit--;
		found = false;
		it = vv_pit->begin();
		counter2 = 0;
		while (!found && it != vv_pit->end())
		{
			if (p2.px == it->px && p2.py == it->py)
			{
				end_p = it;
				found = true;
				counter2--;
			}
			it++;
			counter2++;
		}

		if (found)
		{
			bool order = true;

			if (counter2 < counter1)
			{
				int dummy = counter1;
				counter1 = counter2;
				counter2 = dummy;
				it = start_p;
				start_p = end_p;
				end_p = it;
				order = false;
			}

			std::vector<Point> oldline, oldline1;
			if (counter2 - counter1 + 1 < (int)vv_pit->size() - counter2 + counter1)
			{
				oldline.insert(oldline.begin(), start_p, ++end_p);
				oldline1.insert(oldline1.begin(), start_p, end_p);
			}
			else
			{
				order = !order;
				oldline.insert(oldline.begin(), end_p, vv_pit->end());
				oldline.insert(oldline.end(), vv_pit->begin(), ++start_p);
				oldline1.insert(oldline1.begin(), end_p, vv_pit->end());
				oldline1.insert(oldline1.end(), vv_pit->begin(), start_p);
			}

			oldline.insert(oldline.end(), newline->begin(), newline->end());
			oldline.insert(oldline.end(), limit1.begin(), limit1.end());
			oldline.insert(oldline.end(), limit2.begin(), limit2.end());

			std::vector<Point> change_pts;
			Point p;
			bool in, in1;
			it = newline->begin();
			in = (m_WorkBits[unsigned(m_Width) * it->py + it->px] == f);
			in1 = !in;
			itold = it;
			it++;

			while (it != newline->end())
			{
				if ((m_WorkBits[unsigned(m_Width) * it->py + it->px] == f) != in)
				{
					if (in)
					{
						change_pts.push_back(*itold);
					}
					else
					{
						change_pts.push_back(*it);
					}

					p.px = it->px;
					p.py = itold->py;
					change_pts.push_back(p);
					p.px = itold->px;
					p.py = it->py;
					change_pts.push_back(p);

					in = !in;
				}
				itold++;
				it++;
			}
			p.px = m_Width;
			p.py = m_Height;
			change_pts.push_back(p);
			change_pts.push_back(p);
			change_pts.push_back(p);

			float* bkp = m_WorkBits;
			m_WorkBits = m_Sliceprovide->GiveMe();

			FillContour(&oldline, true);

			for (unsigned i = 0; i < m_Area; i++)
			{
				if (m_WorkBits[i] == 255.0f)
				{
					if (bkp[i] == f)
						bkp[i] = 0.0;
					else
						bkp[i] = f;
				}
			}

			for (it = newline->begin(); it != newline->end(); it++)
				bkp[unsigned(m_Width) * it->py + it->px] = f;

			Point p1, p2, p3;
			it = change_pts.begin();
			p1 = *it;
			it++;
			p2 = *it;
			it++;
			p3 = *it;
			it++;
			bool dontdraw = false;
			if (order)
			{
				for (std::vector<Point>::iterator it1 = oldline1.begin();
						 it1 != oldline1.end(); it1++)
				{
					if (it1->px == p1.px && it1->py == p1.py)
					{
						in1 = !in1;
						p1 = *it;
						it++;
						p2 = *it;
						it++;
						p3 = *it;
						it++;
						it1--;
						dontdraw = true;
					}
					else if (!in1 && ((it1->px == p2.px && it1->py == p2.py) ||
															 (it1->px == p3.px && it1->py == p3.py)))
					{
						it1++;
						if (it1 == oldline1.end() || it1->px != p1.px ||
								it1->py != p1.py)
						{
							in1 = true;
							p1 = *it;
							it++;
							p2 = *it;
							it++;
							p3 = *it;
							it++;
						}
						it1--;
						dontdraw = true;
					}
					else if (in1 && ((it1->px == p2.px && it1->py == p2.py) ||
															(it1->px == p3.px && it1->py == p3.py)))
					{
						bkp[unsigned(m_Width) * it1->py + it1->px] = f;
						in1 = false;
						p1 = *it;
						it++;
						p2 = *it;
						it++;
						p3 = *it;
						it++;
					}

					if (dontdraw)
						dontdraw = false;
					else if (in1)
						bkp[unsigned(m_Width) * it1->py + it1->px] = f;
				}
			}
			else
			{
				for (std::vector<Point>::reverse_iterator it1 = oldline1.rbegin();
						 it1 != oldline1.rend(); it1++)
				{
					if (it1->px == p1.px && it1->py == p1.py)
					{
						in1 = !in1;
						p1 = *it;
						it++;
						p2 = *it;
						it++;
						p3 = *it;
						it++;
						it1--;
						dontdraw = true;
					}
					else if (!in1 && ((it1->px == p2.px && it1->py == p2.py) ||
															 (it1->px == p3.px && it1->py == p3.py)))
					{
						it1++;
						if (it1 == oldline1.rend() || it1->px != p1.px ||
								it1->py != p1.py)
						{
							in1 = true;
							p1 = *it;
							it++;
							p2 = *it;
							it++;
							p3 = *it;
							it++;
						}
						it1--;
						dontdraw = true;
					}
					else if (in1 && ((it1->px == p2.px && it1->py == p2.py) ||
															(it1->px == p3.px && it1->py == p3.py)))
					{
						bkp[unsigned(m_Width) * it1->py + it1->px] = f;
						in1 = false;
						p1 = *it;
						it++;
						p2 = *it;
						it++;
						p3 = *it;
						it++;
					}

					if (dontdraw)
						dontdraw = false;
					else if (in1)
						bkp[unsigned(m_Width) * it1->py + it1->px] = f;
				}
			}

			m_Sliceprovide->TakeBack(m_WorkBits);
			m_WorkBits = bkp;
		}
	}

	// copy temporary 'work' image to tissues
	for (unsigned ineu = 0; ineu < m_Area; ineu++)
		tissues[ineu] = (tissues_size_t)(m_WorkBits[ineu] + 0.1f);

	PopstackWork();

	m_Mode1 = dummymode1;
	m_Mode2 = dummymode2;
}

template<typename T, typename F>
void Bmphandler::Brush(T* data, T f, Point p, int radius, bool draw, T f1, F is_locked)
{
	unsigned short dist = radius * radius;

	int xmin, xmax, ymin, ymax, d;
	if (p.px < radius)
		xmin = 0;
	else
		xmin = int(p.px - radius);
	if (p.px + radius >= m_Width)
		xmax = int(m_Width - 1);
	else
		xmax = int(p.px + radius);

	for (int x = xmin; x <= xmax; x++)
	{
		d = int(floor(sqrt(float(dist - (x - p.px) * (x - p.px)))));
		ymin = std::max(0, int(p.py) - d);
		ymax = std::min(int(m_Height - 1), d + p.py);

		for (int y = ymin; y <= ymax; y++)
		{
			// don't modify locked pixels
			if (is_locked(data[y * unsigned(m_Width) + x]))
				continue;

			if (draw)
			{
				data[y * unsigned(m_Width) + x] = f;
			}
			else
			{
				if (data[y * unsigned(m_Width) + x] == f)
					data[y * unsigned(m_Width) + x] = f1;
			}
		}
	}
}

template<typename T, typename F>
void Bmphandler::Brush(T* data, T f, Point p, float const radius, float dx, float dy, bool draw, T f1, F is_locked)
{
	float const radius_corrected = dx > dy
																		 ? std::floor(radius / dx + 0.5f) * dx
																		 : std::floor(radius / dy + 0.5f) * dy;
	float const radius_corrected2 = radius_corrected * radius_corrected;

	int const xradius = std::ceil(radius_corrected / dx);
	int const yradius = std::ceil(radius_corrected / dy);
	for (int x = std::max(0, p.px - xradius); x <= std::min(static_cast<int>(m_Width) - 1, p.px + xradius); x++)
	{
		for (int y = std::max(0, p.py - yradius); y <= std::min(static_cast<int>(m_Height) - 1, p.py + yradius); y++)
		{
			// don't modify locked pixels
			if (is_locked(data[y * unsigned(m_Width) + x]))
				continue;

			if (std::pow(dx * (p.px - x), 2.f) + std::pow(dy * (p.py - y), 2.f) <= radius_corrected2)
			{
				if (draw)
				{
					data[y * unsigned(m_Width) + x] = f;
				}
				else
				{
					if (data[y * unsigned(m_Width) + x] == f)
						data[y * unsigned(m_Width) + x] = f1;
				}
			}
		}
	}
}

void Bmphandler::Brush(float f, Point p, int radius, bool draw)
{
	Brush(m_WorkBits, f, p, radius, draw, 0.f, [](float v) { return false; });
}

void Bmphandler::Brush(float f, Point p, float radius, float dx, float dy, bool draw)
{
	Brush(m_WorkBits, f, p, radius, dx, dy, draw, 0.f, [](float v) { return false; });
}

void Bmphandler::Brushtissue(tissuelayers_size_t idx, tissues_size_t f, Point p, int radius, bool draw, tissues_size_t f1)
{
	Brush(m_Tissuelayers[idx], f, p, radius, draw, f1, [](tissues_size_t v) { return TissueInfos::GetTissueLocked(v); });
}

void Bmphandler::Brushtissue(tissuelayers_size_t idx, tissues_size_t f, Point p, float radius, float dx, float dy, bool draw, tissues_size_t f1)
{
	Brush(m_Tissuelayers[idx], f, p, radius, dx, dy, draw, f1, [](tissues_size_t v) { return TissueInfos::GetTissueLocked(v); });
}

void Bmphandler::FillHoles(float f, int minsize)
{
	std::vector<std::vector<Point>> inner_line;
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	float* tmp_bits =
			(float*)malloc(sizeof(float) * (m_Width + 2) * (m_Height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (m_Width + 2) * (m_Height + 2));
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		visited[i] = false;

	unsigned pos = m_Width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner;									 //1 for outer, 7 for inner border
	short direction, directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, m_Width + 3, m_Width + 2, m_Width + 1, -1, -m_Width - 3, -m_Width - 2, -m_Width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	for (unsigned i = 0; i < m_Height; i++)
	{
		for (unsigned j = 0; j < m_Width; j++)
		{
			tmp_bits[pos] = m_WorkBits[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(m_Width + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = unsigned(m_Width + 2) * (m_Height + 1);
			 i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvisited;
	for (unsigned i = m_Width + 1; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvisited;

	pos = m_Width + 2;
	while (pos < unsigned(m_Width + 2) * (m_Height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(m_Width + 2) * (m_Height + 1))
			pos++;

		if (pos < unsigned(m_Width + 2) * (m_Height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (m_Width + 2) - 1);
			p.py = short(pos / (m_Width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + m_Width + 3] != f &&
					tmp_bits[pos + m_Width + 2] != f &&
					tmp_bits[pos + m_Width + 1] != f &&
					tmp_bits[pos - m_Width - 1] != f &&
					tmp_bits[pos - m_Width - 2] != f &&
					tmp_bits[pos - m_Width - 3] != f)
			{
				visited[pos] = true;
			}
			else
			{
				if (tmp_bits[pos - m_Width - 3] == f)
				{						 // tricky criteria
					inner = 7; // inner line
					directionold = direction = 1;
				}
				else
				{
					inner = 1; // outer line
					directionold = direction = 7;
				}

				bubble_size = 0;
				linelength = 0;

				direction = (direction + 5 * inner) % 8;
				done = false;
				directionchange = 5 * inner;
				while (!done)
				{
					directionchange += inner;
					direction = (direction + inner) % 8;
					pos2 = pos1 + offset[direction];
					if (tmp_bits[pos2] == f)
						done = true;
				}

				possecond = pos2;

				do
				{
					vec_pt.push_back(p);
					if (tmp_bits[pos1 - 1] == f ||
							(inner == 7 &&
									!(((direction + 6) % 8 > 2 && directionold > 3 &&
												(directionchange + 5) % 8 > 2) ||
											(direction == 5 && directionold == 3))) ||
							(inner == 1 && !(((direction + 4) % 8 > 2 &&
																	 (directionold + 7) % 8 < 4 &&
																	 (directionchange + 5) % 8 > 2) ||
																 (direction == 3 && directionold == 5)))
							//					 (inner==1&&!(||(direction==3&&directionold==5)))
					)
						visited[pos1] = true;
					pos1 = pos2;
					p.px = short(pos1 % (m_Width + 2) - 1);
					p.py = short(pos1 / (m_Width + 2) - 1);
					//					vec_pt.push_back(p);
					bubble_size += dy[direction] * (2 * p.px - dx[direction]);
					bubble_size -= bordervolume[directionchange % 8];
					linelength += 2;
					//					work_bits[p.px+p.py*width]=255;
					directionold = direction;
					direction = (direction + 5 * inner) % 8;
					done = false;
					directionchange = 5 * inner;
					while (!done)
					{
						directionchange += inner;
						direction = (direction + inner) % 8;
						pos2 = pos1 + offset[direction];
						if (tmp_bits[pos2] == f)
							done = true;
					}
				} while (pos1 != pos || pos2 != possecond); // end do while

				if (inner == 7)
				{
					if (bubble_size < (float)minsize)
						inner_line.push_back(vec_pt);
				}
			}
			pos++;
		}
	}

	free(visited);

	std::vector<int> s;

	int i4 = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			tmp_bits[i4] = -1;
			i4++;
			i1++;
		}

		i4 += 2;
	}

	for (auto& it1 : inner_line)
	{
		for (const Point& it : it1)
		{
			tmp_bits[it.px + 1 + (it.py + 1) * (m_Width + 2)] = 1;
		}
	}

	for (int j = 0; j < m_Width + 2; j++)
		tmp_bits[j] = tmp_bits[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		tmp_bits[j] = tmp_bits[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}

	HystereticGrowth(tmp_bits, &s, m_Width + 2, m_Height + 2, false, 255.0f);

	i4 = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tmp_bits[i4] == 0)
				m_WorkBits[i2] = f;
			//			work_bits[i2]=tmp_bits[i4];
			i4++;
			i2++;
		}

		i4 += 2;
	}

	free(tmp_bits);
}

void Bmphandler::FillHolestissue(tissuelayers_size_t idx, tissues_size_t f, int minsize)
{
	std::vector<std::vector<Point>> inner_line;
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	tissues_size_t* tmp_bits = (tissues_size_t*)malloc(sizeof(tissues_size_t) * (m_Width + 2) * (m_Height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (m_Width + 2) * (m_Height + 2));
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		visited[i] = false;

	unsigned pos = m_Width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner;									 //1 for outer, 7 for inner border
	short direction, directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, m_Width + 3, m_Width + 2, m_Width + 1, -1, -m_Width - 3, -m_Width - 2, -m_Width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned i = 0; i < m_Height; i++)
	{
		for (unsigned j = 0; j < m_Width; j++)
		{
			tmp_bits[pos] = tissues[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	tissues_size_t unvis = f + 1;
	if (f == TISSUES_SIZE_MAX)
		unvis = 0;
	for (unsigned i = 0; i < unsigned(m_Width + 2); i++)
		tmp_bits[i] = unvis;
	for (unsigned i = unsigned(m_Width + 2) * (m_Height + 1);
			 i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		tmp_bits[i] = unvis;
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvis;
	for (unsigned i = m_Width + 1; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvis;

	pos = m_Width + 2;
	while (pos < unsigned(m_Width + 2) * (m_Height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(m_Width + 2) * (m_Height + 1))
			pos++;

		if (pos < unsigned(m_Width + 2) * (m_Height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (m_Width + 2) - 1);
			p.py = short(pos / (m_Width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + m_Width + 3] != f &&
					tmp_bits[pos + m_Width + 2] != f &&
					tmp_bits[pos + m_Width + 1] != f &&
					tmp_bits[pos - m_Width - 1] != f &&
					tmp_bits[pos - m_Width - 2] != f &&
					tmp_bits[pos - m_Width - 3] != f)
			{
				visited[pos] = true;
			}
			else
			{
				if (tmp_bits[pos - m_Width - 3] == f)
				{						 // tricky criteria
					inner = 7; // inner line
					directionold = direction = 1;
				}
				else
				{
					/*					if(tmp_bits[pos-width-2]==f){
						inner=7; // inner line
						direction=2;
					}
					else{*/
					inner = 1; // outer line
					directionold = direction = 7;
					//					}
				}

				bubble_size = 0;
				linelength = 0;

				direction = (direction + 5 * inner) % 8;
				done = false;
				directionchange = 5 * inner;
				while (!done)
				{
					directionchange += inner;
					direction = (direction + inner) % 8;
					pos2 = pos1 + offset[direction];
					if (tmp_bits[pos2] == f)
						done = true;
				}

				possecond = pos2;

				do
				{
					vec_pt.push_back(p);
					//					if(inner==1)
					//						bmp_bits[p.px+(unsigned)width*p.py]=255-bmp_bits[p.px+(unsigned)width*p.py];//xxxxxxxxxxxxxxxxxx
					//					if(direction<5||inner==1||tmp_bits[pos1-1]==f||directionold<4)
					if (tmp_bits[pos1 - 1] == f ||
							(inner == 7 &&
									!(((direction + 6) % 8 > 2 && directionold > 3 &&
												(directionchange + 5) % 8 > 2) ||
											(direction == 5 && directionold == 3))) ||
							(inner == 1 && !(((direction + 4) % 8 > 2 &&
																	 (directionold + 7) % 8 < 4 &&
																	 (directionchange + 5) % 8 > 2) ||
																 (direction == 3 && directionold == 5)))
							//					 (inner==1&&!(||(direction==3&&directionold==5)))
					)
						visited[pos1] = true;
					pos1 = pos2;
					p.px = short(pos1 % (m_Width + 2) - 1);
					p.py = short(pos1 / (m_Width + 2) - 1);
					//					vec_pt.push_back(p);
					bubble_size += dy[direction] * (2 * p.px - dx[direction]);
					bubble_size -= bordervolume[directionchange % 8];
					linelength += 2;
					//					work_bits[p.px+p.py*width]=255;
					directionold = direction;
					direction = (direction + 5 * inner) % 8;
					done = false;
					directionchange = 5 * inner;
					while (!done)
					{
						directionchange += inner;
						direction = (direction + inner) % 8;
						pos2 = pos1 + offset[direction];
						if (tmp_bits[pos2] == f)
							done = true;
					}
				}
				//				while(pos1!=pos||(inner==1&&pos2!=possecond));
				while (pos1 != pos || pos2 != possecond);
				//				while(pos1!=pos||(inner==1&&!visited[pos2]));
				//				while(pos1!=pos);
				//				vec_pt.pop_back();

				if (inner == 7)
				{
					if (bubble_size < (float)minsize)
						inner_line.push_back(vec_pt);
				}
			}
			pos++;
		}
	}

	free(visited);

	std::vector<int> s;

	int i4 = m_Width + 3;
	//int i1=0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			tmp_bits[i4] = 2;
			i4++;
		}

		i4 += 2;
	}

	for (std::vector<std::vector<Point>>::iterator it1 = inner_line.begin();
			 it1 != inner_line.end(); it1++)
	{
		for (std::vector<Point>::iterator it = it1->begin(); it != it1->end(); it++)
		{
			tmp_bits[it->px + 1 + (it->py + 1) * (m_Width + 2)] = 1;
		}
	}

	for (int j = 0; j < m_Width + 2; j++)
		tmp_bits[j] = tmp_bits[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		tmp_bits[j] = tmp_bits[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}

	while (!s.empty())
	{
		i4 = s.back();
		s.pop_back();
		if (tmp_bits[i4 - 1] == 2)
		{
			tmp_bits[i4 - 1] = TISSUES_SIZE_MAX;
			s.push_back(i4 - 1);
		}
		if (tmp_bits[i4 + 1] == 2)
		{
			tmp_bits[i4 + 1] = TISSUES_SIZE_MAX;
			s.push_back(i4 + 1);
		}
		if (tmp_bits[i4 - m_Width - 2] == 2)
		{
			tmp_bits[i4 - m_Width - 2] = TISSUES_SIZE_MAX;
			s.push_back(i4 - m_Width - 2);
		}
		if (tmp_bits[i4 + m_Width + 2] == 2)
		{
			tmp_bits[i4 + m_Width + 2] = TISSUES_SIZE_MAX;
			s.push_back(i4 + m_Width + 2);
		}
	}

	i4 = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tmp_bits[i4] == 2)
				tissues[i2] = f;
			i4++;
			i2++;
		}

		i4 += 2;
	}

	free(tmp_bits);
}

void Bmphandler::RemoveIslands(float f, int minsize)
{
	std::vector<std::vector<Point>> outer_line;
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	float* tmp_bits =
			(float*)malloc(sizeof(float) * (m_Width + 2) * (m_Height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (m_Width + 2) * (m_Height + 2));
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		visited[i] = false;

	unsigned pos = m_Width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner;									 //1 for outer, 7 for inner border
	short direction, directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, m_Width + 3, m_Width + 2, m_Width + 1, -1, -m_Width - 3, -m_Width - 2, -m_Width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	for (unsigned i = 0; i < m_Height; i++)
	{
		for (unsigned j = 0; j < m_Width; j++)
		{
			tmp_bits[pos] = m_WorkBits[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(m_Width + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = unsigned(m_Width + 2) * (m_Height + 1);
			 i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvisited;
	for (unsigned i = m_Width + 1; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvisited;

	pos = m_Width + 2;
	while (pos < unsigned(m_Width + 2) * (m_Height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(m_Width + 2) * (m_Height + 1))
			pos++;

		if (pos < unsigned(m_Width + 2) * (m_Height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (m_Width + 2) - 1);
			p.py = short(pos / (m_Width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + m_Width + 3] != f &&
					tmp_bits[pos + m_Width + 2] != f &&
					tmp_bits[pos + m_Width + 1] != f &&
					tmp_bits[pos - m_Width - 1] != f &&
					tmp_bits[pos - m_Width - 2] != f &&
					tmp_bits[pos - m_Width - 3] != f)
			{
				visited[pos] = true;
				vec_pt.push_back(p);
				if (1 < minsize)
					outer_line.push_back(vec_pt);
			}
			else
			{
				if (tmp_bits[pos - m_Width - 3] == f)
				{						 // tricky criteria
					inner = 7; // inner line
					directionold = direction = 1;
				}
				else
				{
					/*					if(tmp_bits[pos-width-2]==f){
						inner=7; // inner line
						direction=2;
					}
					else{*/
					inner = 1; // outer line
					directionold = direction = 7;
					//					}
				}

				bubble_size = 0;
				linelength = 0;

				direction = (direction + 5 * inner) % 8;
				done = false;
				directionchange = 5 * inner;
				while (!done)
				{
					directionchange += inner;
					direction = (direction + inner) % 8;
					pos2 = pos1 + offset[direction];
					if (tmp_bits[pos2] == f)
						done = true;
				}

				possecond = pos2;

				do
				{
					vec_pt.push_back(p);
					//					if(inner==1)
					//						bmp_bits[p.px+(unsigned)width*p.py]=255-bmp_bits[p.px+(unsigned)width*p.py];//xxxxxxxxxxxxxxxxxx
					//					if(direction<5||inner==1||tmp_bits[pos1-1]==f||directionold<4)
					if (tmp_bits[pos1 - 1] == f ||
							(inner == 7 &&
									!(((direction + 6) % 8 > 2 && directionold > 3 &&
												(directionchange + 5) % 8 > 2) ||
											(direction == 5 && directionold == 3))) ||
							(inner == 1 && !(((direction + 4) % 8 > 2 &&
																	 (directionold + 7) % 8 < 4 &&
																	 (directionchange + 5) % 8 > 2) ||
																 (direction == 3 && directionold == 5)))
							//					 (inner==1&&!(||(direction==3&&directionold==5)))
					)
						visited[pos1] = true;
					pos1 = pos2;
					p.px = short(pos1 % (m_Width + 2) - 1);
					p.py = short(pos1 / (m_Width + 2) - 1);
					//					vec_pt.push_back(p);
					bubble_size += dy[direction] * (2 * p.px - dx[direction]);
					bubble_size -= bordervolume[directionchange % 8];
					linelength += 2;
					//					work_bits[p.px+p.py*width]=255;
					directionold = direction;
					direction = (direction + 5 * inner) % 8;
					done = false;
					directionchange = 5 * inner;
					while (!done)
					{
						directionchange += inner;
						direction = (direction + inner) % 8;
						pos2 = pos1 + offset[direction];
						if (tmp_bits[pos2] == f)
							done = true;
					}
				}
				//				while(pos1!=pos||(inner==1&&pos2!=possecond));
				while (pos1 != pos || pos2 != possecond);
				//				while(pos1!=pos||(inner==1&&!visited[pos2]));
				//				while(pos1!=pos);
				//				vec_pt.pop_back();

				if (inner == 1)
				{
					if (bubble_size + linelength < (float)minsize)
						outer_line.push_back(vec_pt);
				}
			}
			pos++;
		}
	}

	free(visited);

	std::vector<int> s;

	int i4 = m_Width + 3;
	int i1 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			tmp_bits[i4] = -1;
			i4++;
			i1++;
		}

		i4 += 2;
	}

	for (std::vector<std::vector<Point>>::iterator it1 = outer_line.begin();
			 it1 != outer_line.end(); it1++)
	{
		for (std::vector<Point>::iterator it = it1->begin(); it != it1->end(); it++)
		{
			tmp_bits[it->px + 1 + (it->py + 1) * (m_Width + 2)] = 1;
		}
	}

	for (int j = 0; j < m_Width + 2; j++)
		tmp_bits[j] = tmp_bits[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		tmp_bits[j] = tmp_bits[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}

	HystereticGrowth(tmp_bits, &s, m_Width + 2, m_Height + 2, false, 255.0f);

	i4 = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tmp_bits[i4] == 0)
				m_WorkBits[i2] = 0;
			//			work_bits[i2]=tmp_bits[i4];
			i4++;
			i2++;
		}

		i4 += 2;
	}
	for (std::vector<std::vector<Point>>::iterator it1 = outer_line.begin();
			 it1 != outer_line.end(); it1++)
	{
		for (std::vector<Point>::iterator it = it1->begin(); it != it1->end(); it++)
		{
			m_WorkBits[it->px + it->py * m_Width] = 0;
		}
	}

	free(tmp_bits);
}

void Bmphandler::RemoveIslandstissue(tissuelayers_size_t idx, tissues_size_t f, int minsize)
{
	std::vector<std::vector<Point>> outer_line;
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	tissues_size_t* tmp_bits = (tissues_size_t*)malloc(sizeof(tissues_size_t) * (m_Width + 2) * (m_Height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (m_Width + 2) * (m_Height + 2));
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		visited[i] = false;

	unsigned pos = m_Width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner;									 //1 for outer, 7 for inner border
	short direction, directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, m_Width + 3, m_Width + 2, m_Width + 1, -1, -m_Width - 3, -m_Width - 2, -m_Width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned i = 0; i < m_Height; i++)
	{
		for (unsigned j = 0; j < m_Width; j++)
		{
			tmp_bits[pos] = tissues[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	tissues_size_t unvis = f + 1;
	if (f == TISSUES_SIZE_MAX)
		unvis = 0;
	for (unsigned i = 0; i < unsigned(m_Width + 2); i++)
		tmp_bits[i] = unvis;
	for (unsigned i = unsigned(m_Width + 2) * (m_Height + 1);
			 i < unsigned(m_Width + 2) * (m_Height + 2); i++)
		tmp_bits[i] = unvis;
	for (unsigned i = 0; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvis;
	for (unsigned i = m_Width + 1; i < unsigned(m_Width + 2) * (m_Height + 2);
			 i += (m_Width + 2))
		tmp_bits[i] = unvis;

	pos = m_Width + 2;
	while (pos < unsigned(m_Width + 2) * (m_Height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(m_Width + 2) * (m_Height + 1))
			pos++;

		if (pos < unsigned(m_Width + 2) * (m_Height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (m_Width + 2) - 1);
			p.py = short(pos / (m_Width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + m_Width + 3] != f &&
					tmp_bits[pos + m_Width + 2] != f &&
					tmp_bits[pos + m_Width + 1] != f &&
					tmp_bits[pos - m_Width - 1] != f &&
					tmp_bits[pos - m_Width - 2] != f &&
					tmp_bits[pos - m_Width - 3] != f)
			{
				visited[pos] = true;
				vec_pt.push_back(p);
				if (1 < minsize)
					outer_line.push_back(vec_pt);
			}
			else
			{
				if (tmp_bits[pos - m_Width - 3] == f)
				{						 // tricky criteria
					inner = 7; // inner line
					directionold = direction = 1;
				}
				else
				{
					/*					if(tmp_bits[pos-width-2]==f){
						inner=7; // inner line
						direction=2;
					}
					else{*/
					inner = 1; // outer line
					directionold = direction = 7;
					//					}
				}

				bubble_size = 0;
				linelength = 0;

				direction = (direction + 5 * inner) % 8;
				done = false;
				directionchange = 5 * inner;
				while (!done)
				{
					directionchange += inner;
					direction = (direction + inner) % 8;
					pos2 = pos1 + offset[direction];
					if (tmp_bits[pos2] == f)
						done = true;
				}

				possecond = pos2;

				do
				{
					vec_pt.push_back(p);
					//					if(inner==1)
					//						bmp_bits[p.px+(unsigned)width*p.py]=255-bmp_bits[p.px+(unsigned)width*p.py];//xxxxxxxxxxxxxxxxxx
					//					if(direction<5||inner==1||tmp_bits[pos1-1]==f||directionold<4)
					if (tmp_bits[pos1 - 1] == f ||
							(inner == 7 &&
									!(((direction + 6) % 8 > 2 && directionold > 3 &&
												(directionchange + 5) % 8 > 2) ||
											(direction == 5 && directionold == 3))) ||
							(inner == 1 && !(((direction + 4) % 8 > 2 &&
																	 (directionold + 7) % 8 < 4 &&
																	 (directionchange + 5) % 8 > 2) ||
																 (direction == 3 && directionold == 5)))
							//					 (inner==1&&!(||(direction==3&&directionold==5)))
					)
						visited[pos1] = true;
					pos1 = pos2;
					p.px = short(pos1 % (m_Width + 2) - 1);
					p.py = short(pos1 / (m_Width + 2) - 1);
					//					vec_pt.push_back(p);
					bubble_size += dy[direction] * (2 * p.px - dx[direction]);
					bubble_size -= bordervolume[directionchange % 8];
					linelength += 2;
					//					work_bits[p.px+p.py*width]=255;
					directionold = direction;
					direction = (direction + 5 * inner) % 8;
					done = false;
					directionchange = 5 * inner;
					while (!done)
					{
						directionchange += inner;
						direction = (direction + inner) % 8;
						pos2 = pos1 + offset[direction];
						if (tmp_bits[pos2] == f)
							done = true;
					}
				}
				//				while(pos1!=pos||(inner==1&&pos2!=possecond));
				while (pos1 != pos || pos2 != possecond);
				//				while(pos1!=pos||(inner==1&&!visited[pos2]));
				//				while(pos1!=pos);
				//				vec_pt.pop_back();

				if (inner == 1)
				{
					if (bubble_size + linelength < (float)minsize)
						outer_line.push_back(vec_pt);
				}
			}
			pos++;
		}
	}

	free(visited);

	std::vector<int> s;

	int i4 = m_Width + 3;
	//int i1=0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			tmp_bits[i4] = 2;
			i4++;
		}

		i4 += 2;
	}

	for (std::vector<std::vector<Point>>::iterator it1 = outer_line.begin();
			 it1 != outer_line.end(); it1++)
	{
		for (std::vector<Point>::iterator it = it1->begin(); it != it1->end(); it++)
		{
			tmp_bits[it->px + 1 + (it->py + 1) * (m_Width + 2)] = 1;
		}
	}

	for (int j = 0; j < m_Width + 2; j++)
		tmp_bits[j] = tmp_bits[j + ((unsigned)m_Width + 2) * (m_Height + 1)] = 0;
	for (int j = 0; j <= ((int)m_Width + 2) * (m_Height + 1); j += m_Width + 2)
		tmp_bits[j] = tmp_bits[j + m_Width + 1] = 0;

	for (int j = m_Width + 3; j < 2 * m_Width + 3; j++)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = m_Area + 2 * m_Height + 1;
			 j < m_Area + m_Width + 2 * m_Height + 1; j++)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * m_Width + 5; j <= m_Area + 2 * m_Height + 1;
			 j += m_Width + 2)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * m_Width + 4; j <= m_Area + m_Width + 2 * m_Height;
			 j += m_Width + 2)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}

	while (!s.empty())
	{
		i4 = s.back();
		s.pop_back();
		if (tmp_bits[i4 - 1] == 2)
		{
			tmp_bits[i4 - 1] = TISSUES_SIZE_MAX;
			s.push_back(i4 - 1);
		}
		if (tmp_bits[i4 + 1] == 2)
		{
			tmp_bits[i4 + 1] = TISSUES_SIZE_MAX;
			s.push_back(i4 + 1);
		}
		if (tmp_bits[i4 - m_Width - 2] == 2)
		{
			tmp_bits[i4 - m_Width - 2] = TISSUES_SIZE_MAX;
			s.push_back(i4 - m_Width - 2);
		}
		if (tmp_bits[i4 + m_Width + 2] == 2)
		{
			tmp_bits[i4 + m_Width + 2] = TISSUES_SIZE_MAX;
			s.push_back(i4 + m_Width + 2);
		}
	}

	i4 = m_Width + 3;
	int i2 = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tmp_bits[i4] == 2)
				tissues[i2] = 0;
			i4++;
			i2++;
		}

		i4 += 2;
	}
	for (std::vector<std::vector<Point>>::iterator it1 = outer_line.begin();
			 it1 != outer_line.end(); it1++)
	{
		for (std::vector<Point>::iterator it = it1->begin(); it != it1->end(); it++)
		{
			tissues[it->px + it->py * m_Width] = 0;
		}
	}

	free(tmp_bits);
}

/*void Bmphandler::add_skin(unsigned i)
{
	unsigned short x,y;
	unsigned pos;
	unsigned i1;
	unsigned w=(unsigned)width;

	Pair p;
	get_range(&p);
	float setto;
	if(p.high<=254.0f) setto=255.0f;
	else if(p.low>=1.0f) setto=0.0f;
	else {
		setto=p.low;
		for(pos=0;pos<area;pos++){
			if(work_bits[pos]!=p.high)
				setto=max(setto,work_bits[pos]);
		}
		setto=(setto+p.high)/2;
	}

	for(y=0;y<height;y++){
		pos=y*w;
		while(pos<(y+1)*w&&work_bits[pos]==0) pos++;
		i1=0;
		while(pos<(y+1)*w&&work_bits[pos]!=0&&i1<i){
			work_bits[pos]=setto;
			pos++;
			i1++;
		}
	}

	pos=w-1;
	while(pos>0&&work_bits[pos]==0) pos--;
//	if(work_bits[pos]!=0){
		i1=0;
		while(pos>0&&work_bits[pos]!=0&&i1<i){
			work_bits[pos]=setto;
			pos--;
			i1++;
		}
		if(work_bits[pos]!=0&&i1<i) work_bits[pos]=setto;
//	}

	for(y=1;y<height;y++){
		pos=(y+1)*w-1;
		while(pos>=y*w&&work_bits[pos]==0) pos--;
		i1=0;
		while(pos>=y*w&&work_bits[pos]!=0&&i1<i){
			work_bits[pos]=setto;
			pos--;
			i1++;
		}
	}

	for(x=0;x<width;x++){
		pos=x;
		while(pos<area&&work_bits[pos]==0) pos+=w;
		i1=0;
		while(pos<area&&work_bits[pos]!=0&&i1<i){
			work_bits[pos]=setto;
			pos+=w;
			i1++;
		}
	}

	for(x=0;x<width;x++){
		pos=area-width+x;
		while(pos>=w&&work_bits[pos]==0) pos-=w;
		i1=0;
		while(pos>=w&&work_bits[pos]!=0&&i1<i){
			work_bits[pos]=setto;
			pos-=w;
			i1++;
		}
		if(work_bits[pos]!=0&&i1<i) work_bits[pos]=setto;
	}

}*/

void Bmphandler::AddVm(std::vector<Mark>* vm1)
{
	m_Vvm.push_back(*vm1);
	m_MaximStore = std::max(m_MaximStore, vm1->begin()->mark);
}

void Bmphandler::ClearVvm()
{
	m_Vvm.clear();
	m_MaximStore = 1;
}

bool Bmphandler::DelVm(Point p, short radius)
{
	short radius2 = radius * radius;
	std::vector<Mark>::iterator it;
	std::vector<std::vector<Mark>>::iterator vit;
	vit = m_Vvm.begin();
	bool found = false;

	while (!found && vit != m_Vvm.end())
	{
		it = vit->begin();
		while (it != vit->end() &&
					 (it->p.px - p.px) * (it->p.px - p.px) +
									 (it->p.py - p.py) * (it->p.py - p.py) >
							 radius2)
			it++;
		if (it != vit->end() && (it->p.px - p.px) * (it->p.px - p.px) +
																		(it->p.py - p.py) * (it->p.py - p.py) <=
																radius2)
			found = true;
		else
			vit++;
	}

	if (found)
	{
		if (m_Vvm.size() == 1)
		{
			ClearVvm();
		}
		else
		{
			unsigned maxim1 = vit->begin()->mark;
			std::vector<std::vector<Mark>>::iterator vit1;
			vit1 = m_Vvm.begin();
			while (vit1 != m_Vvm.end())
			{
				vit1++;
			}
			m_Vvm.erase(vit);

			if (m_MaximStore == maxim1 && !m_Vvm.empty())
			{
				m_MaximStore = 1;
				for (vit = m_Vvm.begin(); vit != m_Vvm.end(); vit++)
					m_MaximStore = std::max(m_MaximStore, vit->begin()->mark);
			}
		}
		return true;
	}
	else
		return false;
}

std::vector<std::vector<Mark>>* Bmphandler::ReturnVvm() { return &m_Vvm; }

unsigned Bmphandler::ReturnVvmmaxim() const { return m_MaximStore; }

void Bmphandler::Copy2vvm(std::vector<std::vector<Mark>>* vvm1)
{
	m_Vvm = *vvm1;
}

void Bmphandler::AddLimit(std::vector<Point>* vp1)
{
	m_Limits.push_back(*vp1);
}

void Bmphandler::ClearLimits()
{
	m_Limits.clear();
}

bool Bmphandler::DelLimit(Point p, short radius)
{
	short radius2 = radius * radius;
	std::vector<Point>::iterator it;
	std::vector<std::vector<Point>>::iterator vit;
	vit = m_Limits.begin();
	bool found = false;

	while (!found && vit != m_Limits.end())
	{
		it = vit->begin();
		while (it != vit->end() && (it->px - p.px) * (it->px - p.px) +
																			 (it->py - p.py) * (it->py - p.py) >
																	 radius2)
			it++;
		if (it != vit->end() && (it->px - p.px) * (it->px - p.px) +
																		(it->py - p.py) * (it->py - p.py) <=
																radius2)
			found = true;
		else
			vit++;
	}

	if (found)
	{
		if (m_Limits.size() == 1)
		{
			ClearLimits();
		}
		else
		{
			std::vector<std::vector<Point>>::iterator vit1;
			vit1 = m_Limits.begin();
			while (vit1 != m_Limits.end())
			{
				vit1++;
			}
			m_Limits.erase(vit);
		}
		return true;
	}
	else
		return false;
}

std::vector<std::vector<Point>>* Bmphandler::ReturnLimits() { return &m_Limits; }

void Bmphandler::Copy2limits(std::vector<std::vector<Point>>* limits1)
{
	m_Limits = *limits1;
}

void Bmphandler::MapTissueIndices(const std::vector<tissues_size_t>& indexMap)
{
	for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned int i = 0; i < m_Area; ++i)
		{
			tissues[i] = indexMap[tissues[i]];
		}
	}
}

void Bmphandler::RemoveTissue(tissues_size_t tissuenr)
{
	for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
	{
		tissues_size_t* tissues = m_Tissuelayers[idx];
		for (unsigned int i = 0; i < m_Area; ++i)
		{
			if (tissues[i] > tissuenr)
			{
				tissues[i]--;
			}
			else if (tissues[i] == tissuenr)
			{
				tissues[i] = 0;
			}
		}
	}
}

void Bmphandler::GroupTissues(tissuelayers_size_t idx, std::vector<tissues_size_t>& olds, std::vector<tissues_size_t>& news)
{
	tissues_size_t crossref[TISSUES_SIZE_MAX + 1];
	for (int i = 0; i < TISSUES_SIZE_MAX + 1; i++)
		crossref[i] = (tissues_size_t)i;
	unsigned count = std::min(olds.size(), news.size());

	for (unsigned int i = 0; i < count; i++)
		crossref[olds[i]] = news[i];

	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned int j = 0; j < m_Area; j++)
	{
		tissues[j] = crossref[tissues[j]];
	}
}

unsigned char Bmphandler::ReturnMode(bool bmporwork) const
{
	if (bmporwork)
		return m_Mode1;
	else
		return m_Mode2;
}

void Bmphandler::SetMode(unsigned char mode, bool bmporwork)
{
	if (bmporwork)
		m_Mode1 = mode;
	else
		m_Mode2 = mode;
}

bool Bmphandler::PrintAmasciiSlice(tissuelayers_size_t idx, std::ofstream& streamname)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned i = 0; i < m_Area; i++)
	{
		streamname << (int)tissues[i] << " " << std::endl;
	}
	return true;
}

bool Bmphandler::PrintVtkasciiSlice(tissuelayers_size_t idx, std::ofstream& streamname)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (unsigned i = 0; i < m_Area; i++)
	{
		streamname << (int)tissues[i] << " ";
	}
	streamname << std::endl;
	return true;
}

bool Bmphandler::PrintVtkbinarySlice(tissuelayers_size_t idx, std::ofstream& streamname)
{
	tissues_size_t* tissues = m_Tissuelayers[idx];
	if (TissueInfos::GetTissueCount() <= 255)
	{
		if (sizeof(tissues_size_t) == sizeof(unsigned char))
		{
			streamname.write((char*)(tissues), m_Area);
		}
		else
		{
			unsigned char* uchar_buffer = new unsigned char[m_Area];
			for (unsigned int i = 0; i < m_Area; i++)
			{
				uchar_buffer[i] = (unsigned char)tissues[i];
			}
			streamname.write((char*)(uchar_buffer), m_Area);
			delete[] uchar_buffer;
		}
	}
	else
	{
		streamname.write((char*)(tissues), sizeof(tissues_size_t) * m_Area);
	}
	return true;
}

void Bmphandler::Shifttissue()
{
	int x, y;

	FILE* fp;
	fp = fopen("C:\\move.txt", "r");
	int counter = fscanf(fp, "%i %i", &x, &y);
	fclose(fp);
	if (x == 0 && y == 0)
		return;
	if (counter == 2)
	{
		long offset = (long)m_Width * y + x;
		for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = m_Tissuelayers[idx];
			if (x >= 0 && y >= 0)
			{
				unsigned pos = m_Area;
				for (int j = m_Height; j > 0;)
				{
					j--;
					for (int k = m_Width; k > 0;)
					{
						k--;
						pos--;
						if (k + x < m_Width && j + y < m_Height)
							tissues[pos + offset] = tissues[pos];
						tissues[pos] = 0;
					}
				}
			}
			else if (x <= 0 && y <= 0)
			{
				unsigned pos = 0;
				for (int j = 0; j < m_Height; j++)
				{
					for (int k = 0; k < m_Width; k++)
					{
						if (k + x >= 0 && j + y >= 0)
							tissues[pos + offset] = tissues[pos];
						tissues[pos] = 0;
						pos++;
					}
				}
			}
			else if (x > 0 && y < 0)
			{
				unsigned pos = 0;
				for (int j = 0; j < m_Height; j++)
				{
					for (int k = 0; k < m_Width; k++)
					{
						if (k + x < m_Width && j + y >= 0)
							tissues[pos + offset] = tissues[pos];
						tissues[pos] = 0;
						pos++;
					}
				}
			}
			else if (x < 0 && y > 0)
			{
				unsigned pos = m_Area;
				for (int j = m_Height; j > 0;)
				{
					j--;
					for (int k = m_Width; k > 0;)
					{
						k--;
						pos--;
						if (k + x >= 0 && j + y < m_Height)
							tissues[pos + offset] = tissues[pos];
						tissues[pos] = 0;
					}
				}
			}
		}
	}
}

void Bmphandler::Shiftbmp()
{
	int x, y;

	FILE* fp;
	fp = fopen("C:\\move.txt", "r");
	int counter = fscanf(fp, "%i %i", &x, &y);
	fclose(fp);
	if (x == 0 && y == 0)
		return;
	if (counter == 2)
	{
		long offset = (long)m_Width * y + x;
		if (x >= 0 && y >= 0)
		{
			unsigned pos = m_Area;
			for (int j = m_Height; j > 0;)
			{
				j--;
				for (int k = m_Width; k > 0;)
				{
					k--;
					pos--;
					if (k + x < m_Width && j + y < m_Height)
						m_BmpBits[pos + offset] = m_BmpBits[pos];
					m_BmpBits[pos] = 0;
				}
			}
		}
		else if (x <= 0 && y <= 0)
		{
			unsigned pos = 0;
			for (int j = 0; j < m_Height; j++)
			{
				for (int k = 0; k < m_Width; k++)
				{
					if (k + x >= 0 && j + y >= 0)
						m_BmpBits[pos + offset] = m_BmpBits[pos];
					m_BmpBits[pos] = 0;
					pos++;
				}
			}
		}
		else if (x > 0 && y < 0)
		{
			unsigned pos = 0;
			for (int j = 0; j < m_Height; j++)
			{
				for (int k = 0; k < m_Width; k++)
				{
					if (k + x < m_Width && j + y >= 0)
						m_BmpBits[pos + offset] = m_BmpBits[pos];
					m_BmpBits[pos] = 0;
					pos++;
				}
			}
		}
		else if (x < 0 && y > 0)
		{
			unsigned pos = m_Area;
			for (int j = m_Height; j > 0;)
			{
				j--;
				for (int k = m_Width; k > 0;)
				{
					k--;
					pos--;
					if (k + x >= 0 && j + y < m_Height)
						m_BmpBits[pos + offset] = m_BmpBits[pos];
					m_BmpBits[pos] = 0;
				}
			}
		}
	}
}

unsigned long Bmphandler::ReturnWorkpixelcount(float f)
{
	unsigned long pos = 0;
	unsigned long counter = 0;
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (m_WorkBits[pos] == f)
				counter++;
			pos++;
		}
	}
	return counter;
}

unsigned long Bmphandler::ReturnTissuepixelcount(tissuelayers_size_t idx, tissues_size_t c)
{
	unsigned long pos = 0;
	unsigned long counter = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	for (int j = 0; j < m_Height; j++)
	{
		for (int k = 0; k < m_Width; k++)
		{
			if (tissues[pos] == c)
				counter++;
			pos++;
		}
	}
	return counter;
}

bool Bmphandler::GetExtent(tissuelayers_size_t idx, tissues_size_t tissuenr, unsigned short extent[2][2])
{
	if (m_Area == 0)
		return false;

	bool found = false;
	unsigned long pos = 0;
	tissues_size_t* tissues = m_Tissuelayers[idx];
	while (!found && pos < m_Area)
	{
		if (tissues[pos] == tissuenr)
			found = true;
		pos++;
	}
	if (!found)
		return false;
	else
	{
		pos--;
		extent[1][0] = pos / m_Width;
		extent[0][0] = pos % m_Width;
	}
	while (pos < (unsigned long)(extent[1][0] + 1) * m_Width)
	{
		if (tissues[pos] == tissuenr)
			extent[0][1] = pos % m_Width;
		pos++;
	}
	found = false;
	pos = m_Area;
	while (!found && pos > 0)
	{
		pos--;
		if (tissues[pos] == tissuenr)
			found = true;
	}
	extent[1][1] = pos / m_Width;
	for (unsigned short i = extent[1][0] + 1; i <= extent[1][1]; i++)
	{
		pos = (unsigned long)(i)*m_Width;
		for (unsigned short j = 0; j < extent[0][0]; j++, pos++)
		{
			if (tissues[pos] == tissuenr)
				extent[0][0] = j;
		}
		pos = (unsigned long)(i + 1) * m_Width - 1;
		for (unsigned short j = m_Width - 1; j > extent[0][1]; j--, pos--)
		{
			if (tissues[pos] == tissuenr)
				extent[0][1] = j;
		}
	}

	return true;
}

void Bmphandler::Swap(Bmphandler& bmph)
{
	Contour contourd;
	contourd = m_Contour;
	m_Contour = bmph.m_Contour;
	bmph.m_Contour = contourd;
	std::vector<Mark> marksd;
	marksd = m_Marks;
	m_Marks = bmph.m_Marks;
	bmph.m_Marks = marksd;
	short unsigned widthd;
	widthd = m_Width;
	m_Width = bmph.m_Width;
	bmph.m_Width = widthd;
	short unsigned heightd;
	heightd = m_Height;
	m_Height = bmph.m_Height;
	bmph.m_Height = heightd;
	unsigned int aread;
	aread = m_Area;
	m_Area = bmph.m_Area;
	bmph.m_Area = aread;
	float* bmp_bitsd;
	bmp_bitsd = m_BmpBits;
	m_BmpBits = bmph.m_BmpBits;
	bmph.m_BmpBits = bmp_bitsd;
	float* work_bitsd;
	work_bitsd = m_WorkBits;
	m_WorkBits = bmph.m_WorkBits;
	bmph.m_WorkBits = work_bitsd;
	float* help_bitsd;
	help_bitsd = m_HelpBits;
	m_HelpBits = bmph.m_HelpBits;
	bmph.m_HelpBits = help_bitsd;
	tissues_size_t* tissuesd;
	for (tissuelayers_size_t idx = 0; idx < m_Tissuelayers.size(); ++idx)
	{
		tissuesd = m_Tissuelayers[idx];
		m_Tissuelayers[idx] = bmph.m_Tissuelayers[idx];
		bmph.m_Tissuelayers[idx] = tissuesd;
	}
	WshedObj wshedobjd;
	wshedobjd = m_Wshedobj;
	m_Wshedobj = bmph.m_Wshedobj;
	bmph.m_Wshedobj = wshedobjd;
	bool bmp_is_greyd;
	bmp_is_greyd = m_BmpIsGrey;
	m_BmpIsGrey = bmph.m_BmpIsGrey;
	bmph.m_BmpIsGrey = bmp_is_greyd;
	bool work_is_greyd;
	work_is_greyd = m_WorkIsGrey;
	m_WorkIsGrey = bmph.m_WorkIsGrey;
	bmph.m_WorkIsGrey = work_is_greyd;
	bool loadedd;
	loadedd = m_Loaded;
	m_Loaded = bmph.m_Loaded;
	bmph.m_Loaded = loadedd;
	bool ownsliceproviderd;
	ownsliceproviderd = m_Ownsliceprovider;
	m_Ownsliceprovider = bmph.m_Ownsliceprovider;
	bmph.m_Ownsliceprovider = ownsliceproviderd;
	FeatureExtractor fextractd;
	fextractd = m_Fextract;
	m_Fextract = bmph.m_Fextract;
	bmph.m_Fextract = fextractd;
	std::list<float*> bits_stackd;
	bits_stackd = bits_stack;
	bits_stack = bmph.bits_stack;
	bmph.bits_stack = bits_stackd;
	std::list<unsigned char> mode_stackd;
	mode_stackd = mode_stack;
	mode_stack = bmph.mode_stack;
	bmph.mode_stack = mode_stackd;
	SliceProvider* sliceprovided;
	sliceprovided = m_Sliceprovide;
	m_Sliceprovide = bmph.m_Sliceprovide;
	bmph.m_Sliceprovide = sliceprovided;
	std::vector<std::vector<Mark>> vvmd;
	vvmd = m_Vvm;
	m_Vvm = bmph.m_Vvm;
	bmph.m_Vvm = vvmd;
	unsigned maxim_stored;
	maxim_stored = m_MaximStore;
	m_MaximStore = bmph.m_MaximStore;
	bmph.m_MaximStore = maxim_stored;
	std::vector<std::vector<Point>> limitsd;
	limitsd = m_Limits;
	m_Limits = bmph.m_Limits;
	bmph.m_Limits = limitsd;
	unsigned char mode1d;
	mode1d = m_Mode1;
	m_Mode1 = bmph.m_Mode1;
	bmph.m_Mode1 = mode1d;
	unsigned char mode2d;
	mode2d = m_Mode2;
	m_Mode2 = bmph.m_Mode2;
	bmph.m_Mode2 = mode2d;
}

bool Bmphandler::Unwrap(float jumpratio, float range, float shift)
{
	if (range == 0)
	{
		Pair p;
		GetBmprange(&p);
		range = p.high - p.low;
	}
	float jumpabs = jumpratio * range;
	unsigned int pos = 0;
	float shiftbegin = shift;
	for (unsigned short i = 0; i < m_Height; i++)
	{
		if (i != 0)
		{
			if (m_BmpBits[pos - m_Width] - m_BmpBits[pos] > jumpabs)
				shiftbegin += range;
			else if (m_BmpBits[pos] - m_BmpBits[pos - m_Width] > jumpabs)
				shiftbegin -= range;
		}
		shift = shiftbegin;
		m_WorkBits[pos] = m_BmpBits[pos] + shift;
		for (unsigned short j = 1; j < m_Width; j++)
		{
			if (m_BmpBits[pos] - m_BmpBits[pos + 1] > jumpabs)
				shift += range;
			else if (m_BmpBits[pos + 1] - m_BmpBits[pos] > jumpabs)
				shift -= range;
			pos++;
			m_WorkBits[pos] = m_BmpBits[pos] + shift;
		}
		pos++;
	}
	m_Mode2 = m_Mode1;
	for (pos = m_Width - 1; pos + m_Width < m_Area; pos += m_Width)
	{
		if (m_WorkBits[pos + m_Width] - m_WorkBits[pos] > jumpabs)
			return false;
		if (m_WorkBits[pos] - m_WorkBits[pos + m_Width] > jumpabs)
			return false;
	}
	return true;
}

} // namespace iseg
