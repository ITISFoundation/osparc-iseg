/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

#include <qcolor.h>
#include <qimage.h>
#include <qmessagebox.h>

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

//using namespace std;
using namespace iseg;

#define UNREFERENCED_PARAMETER(P) (P)

#ifndef M_PI
#	define M_PI 3.14159
#endif

#ifndef ID_BMP
#	define ID_BMP 0x4D42 /* "BM" */
#endif
#ifndef WIN32
typedef struct /**** BMP file header structure ****/
{
	unsigned short bfType;			/* Magic number for file */
	unsigned int bfSize;				/* Size of file */
	unsigned short bfReserved1; /* Reserved */
	unsigned short bfReserved2; /* ... */
	unsigned int bfOffBits;			/* Offset to bitmap data */
} BITMAPFILEHEADER;

#

typedef struct /**** BMP file info structure ****/
{
	unsigned int biSize;				 /* Size of info header */
	int biWidth;								 /* Width of image */
	int biHeight;								 /* Height of image */
	unsigned short biPlanes;		 /* Number of color planes */
	unsigned short biBitCount;	 /* Number of bits per pixel */
	unsigned int biCompression;	/* Type of compression to use */
	unsigned int biSizeImage;		 /* Size of image data */
	int biXPelsPerMeter;				 /* X pixels per meter */
	int biYPelsPerMeter;				 /* Y pixels per meter */
	unsigned int biClrUsed;			 /* Number of colors used */
	unsigned int biClrImportant; /* Number of important colors */
} BITMAPINFOHEADER;

#	define BI_RGB 0			 /* No compression - straight BGR data */
#	define BI_RLE8 1			 /* 8-bit run-length compression */
#	define BI_RLE4 2			 /* 4-bit run-length compression */
#	define BI_BITFIELDS 3 /* RGB bitmap with RGB masks */

typedef struct /**** Colormap entry structure ****/
{
	unsigned char rgbBlue;		 /* Blue value */
	unsigned char rgbGreen;		 /* Green value */
	unsigned char rgbRed;			 /* Red value */
	unsigned char rgbReserved; /* Reserved */
} RGBQUAD;

typedef struct /**** Bitmap information structure ****/
{
	BITMAPINFOHEADER bmiHeader; /* Image header */
	RGBQUAD bmiColors[256];			/* Image colormap */
} BITMAPINFO;
#endif /* !WIN32 */

template<typename T>
inline void swap_maps(T const*& Tp1, T const*& Tp2)
{
	T const* dummy;
	dummy = Tp1;
	Tp1 = Tp2;
	Tp2 = dummy;
	return;
}

float iseg::f1(float dI, float k) { return exp(-pow(dI / k, 2)); }

float iseg::f2(float dI, float k) { return 1 / (1 + pow(dI / k, 2)); }

std::list<unsigned> bmphandler::stackindex;
unsigned bmphandler::stackcounter;
std::list<float*> bmphandler::bits_stack;
std::list<unsigned char> bmphandler::mode_stack;
//bool bmphandler::lockedtissues[TISSUES_SIZE_MAX+1];

bmphandler::bmphandler()
{
	area = 0;
	loaded = false;
	ownsliceprovider = false;
	sliceprovide_installer = SliceProviderInstaller::getinst();
	stackcounter = 1;
	mode1 = mode2 = 1;
	return; // ?

	redFactor = 0.299;
	greenFactor = 0.587;
	blueFactor = 0.114;
}

bmphandler::bmphandler(const bmphandler&)
{
	area = 0;
	loaded = false;
	ownsliceprovider = false;
	sliceprovide_installer = SliceProviderInstaller::getinst();
	stackcounter = 1;
	mode1 = mode2 = 1;

	redFactor = 0.299;
	greenFactor = 0.587;
	blueFactor = 0.114;
}

bmphandler::~bmphandler()
{
	if (loaded)
	{
		clear_stack();
		sliceprovide->take_back(bmp_bits);
		sliceprovide->take_back(work_bits);
		sliceprovide->take_back(help_bits);
		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			free(tissuelayers[idx]);
		}
		tissuelayers.clear();
		//		if(ownsliceprovider)
		sliceprovide_installer->uninstall(sliceprovide);
		//		free(bmpinfo);
	}
	sliceprovide_installer->return_instance();
	if (sliceprovide_installer->unused())
		delete sliceprovide_installer;
}

void bmphandler::clear_stack()
{
	for (auto& b : bits_stack)
		sliceprovide->take_back(b);
	bits_stack.clear();
	stackindex.clear();
	mode_stack.clear();
	stackcounter = 1;
}

float* bmphandler::return_bmp() { return bmp_bits; }

const float* bmphandler::return_bmp() const { return bmp_bits; }

float* bmphandler::return_work() { return work_bits; }

const float* bmphandler::return_work() const { return work_bits; }

tissues_size_t* bmphandler::return_tissues(tissuelayers_size_t idx)
{
	if (idx < tissuelayers.size())
		return tissuelayers[idx];
	else
		return nullptr;
}

const tissues_size_t* bmphandler::return_tissues(tissuelayers_size_t idx) const
{
	if (idx < tissuelayers.size())
		return tissuelayers[idx];
	else
		return nullptr;
}

float* bmphandler::return_help() { return help_bits; }

float** bmphandler::return_bmpfield() { return &bmp_bits; }

float** bmphandler::return_workfield() { return &work_bits; }

tissues_size_t** bmphandler::return_tissuefield(tissuelayers_size_t idx)
{
	return &tissuelayers[idx];
}

std::vector<Mark>* bmphandler::return_marks() { return &marks; }

void bmphandler::copy2marks(std::vector<Mark>* marks1) { marks = *marks1; }

void bmphandler::get_labels(std::vector<Mark>* labels)
{
	labels->clear();
	get_add_labels(labels);
}

void bmphandler::get_add_labels(std::vector<Mark>* labels)
{
	for (size_t i = 0; i < marks.size(); i++)
	{
		if (marks[i].name != std::string(""))
			labels->push_back(marks[i]);
	}
}

void bmphandler::set_bmp(float* bits, unsigned char mode)
{
	if (loaded)
	{
		if (bmp_bits != bits)
		{
			sliceprovide->take_back(bmp_bits);
			bmp_bits = bits;
			mode1 = mode;
		}
	}
}
void bmphandler::set_work(float* bits, unsigned char mode)
{
	if (loaded)
	{
		if (work_bits != bits)
		{
			sliceprovide->take_back(work_bits);
			work_bits = bits;
			mode2 = mode;
		}
	}
}

void bmphandler::set_tissue(tissuelayers_size_t idx, tissues_size_t* bits)
{
	if (loaded)
	{
		if (tissuelayers[idx] != bits)
		{
			free(tissuelayers[idx]);
			tissuelayers[idx] = bits;
		}
	}
}

float* bmphandler::swap_bmp_pointer(float* bits)
{
	float* tmp = bmp_bits;
	bmp_bits = bits;
	return tmp;
}

float* bmphandler::swap_work_pointer(float* bits)
{
	float* tmp = work_bits;
	work_bits = bits;
	return tmp;
}

tissues_size_t* bmphandler::swap_tissues_pointer(tissuelayers_size_t idx, tissues_size_t* bits)
{
	tissues_size_t* tmp = tissuelayers[idx];
	tissuelayers[idx] = bits;
	return tmp;
}

void bmphandler::copy2bmp(float* bits, unsigned char mode)
{
	if (loaded)
	{
		for (unsigned i = 0; i < area; i++)
			bmp_bits[i] = bits[i];
		mode1 = mode;
	}
}

void bmphandler::copy2work(float* bits, unsigned char mode)
{
	if (loaded)
	{
		for (unsigned i = 0; i < area; i++)
			work_bits[i] = bits[i];
		mode2 = mode;
	}
}

void bmphandler::copy2work(float* bits, bool* mask, unsigned char mode)
{
	if (loaded)
	{
		for (unsigned i = 0; i < area; i++)
		{
			if (mask[i])
				work_bits[i] = bits[i];
		}
		if (mode == 1)
			mode2 = 1;
	}
}

void bmphandler::copy2tissue(tissuelayers_size_t idx, tissues_size_t* bits,
		bool* mask)
{
	if (loaded)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned i = 0; i < area; i++)
		{
			if (mask[i] && (!TissueInfos::GetTissueLocked(tissues[i])))
				tissues[i] = bits[i];
		}
	}
}

void bmphandler::copy2tissue(tissuelayers_size_t idx, tissues_size_t* bits)
{
	if (loaded)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned i = 0; i < area; i++)
			tissues[i] = bits[i];
	}
}

void bmphandler::copyfrombmp(float* bits)
{
	if (loaded)
	{
		for (unsigned i = 0; i < area; i++)
			bits[i] = bmp_bits[i];
	}
}

void bmphandler::copyfromwork(float* bits)
{
	if (loaded)
	{
		for (unsigned i = 0; i < area; i++)
			bits[i] = work_bits[i];
	}
}

void bmphandler::copyfromtissue(tissuelayers_size_t idx, tissues_size_t* bits)
{
	if (loaded)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned i = 0; i < area; i++)
			bits[i] = tissues[i];
	}
}

#ifdef TISSUES_SIZE_TYPEDEF
void bmphandler::copyfromtissue(tissuelayers_size_t idx, unsigned char* bits)
{
	if (loaded)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned i = 0; i < area; i++)
			bits[i] = (unsigned char)tissues[i];
	}
}
#endif // TISSUES_SIZE_TYPEDEF

void bmphandler::copyfromtissuepadded(tissuelayers_size_t idx,
		tissues_size_t* bits,
		unsigned short padding)
{
	if (loaded)
	{
		unsigned int pos1 = 0;
		unsigned int pos2 = 0;
		for (; pos1 < (unsigned int)(width + 2 * padding) * padding + padding;
				 pos1++)
			bits[pos1] = 0;
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned short j = 0; j < height; j++)
		{
			for (unsigned short i = 0; i < width; i++, pos1++, pos2++)
			{
				bits[pos1] = tissues[pos2];
			}
			for (unsigned short i = 0; i < 2 * padding; i++, pos1++)
				bits[pos1] = 0;
		}
		unsigned int maxval = area + 2 * padding * width +
													2 * padding * height + padding * padding;
		for (; pos1 < maxval; pos1++)
			bits[pos1] = 0;
	}
}

void bmphandler::clear_bmp()
{
	for (unsigned int i = 0; i < area; i++)
		bmp_bits[i] = 0;

	delete bmp_bits;
}

void bmphandler::clear_work()
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = 0;
}

inline unsigned bmphandler::pt2coord(Point p)
{
	return p.px + (p.py * (unsigned)width);
}

void bmphandler::bmp_abs()
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = abs(work_bits[i]);
}

void bmphandler::bmp_neg()
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = 255 - work_bits[i];
}

void bmphandler::bmp_sum()
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = work_bits[i] + bmp_bits[i];
}

void bmphandler::bmp_diff()
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = work_bits[i] - bmp_bits[i];
}

void bmphandler::bmp_add(float f)
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = work_bits[i] + f;
}

void bmphandler::bmp_mult()
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] *= bmp_bits[i];
}

void bmphandler::bmp_mult(float f)
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = f * work_bits[i];
}

void bmphandler::bmp_overlay(float alpha)
{
	float tmp = 1.0f - alpha;
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = alpha * bmp_bits[i] + tmp * work_bits[i];
	mode2 = 2;
}

void bmphandler::transparent_add(float* pict2)
{
	for (unsigned int i = 0; i < area; i++)
		if (work_bits[i] == 0)
			work_bits[i] = pict2[i];
}

float* bmphandler::copy_work()
{
	float* results = sliceprovide->give_me();
	for (unsigned i = 0; i < area; i++)
		results[i] = work_bits[i];

	return results;
}

float* bmphandler::copy_bmp()
{
	float* results = sliceprovide->give_me();
	for (unsigned i = 0; i < area; i++)
		results[i] = bmp_bits[i];

	return results;
}

tissues_size_t* bmphandler::copy_tissue(tissuelayers_size_t idx)
{
	tissues_size_t* results =
			(tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned i = 0; i < area; i++)
		results[i] = tissues[i];

	return results;
}

void bmphandler::copy_work(float* output)
{
	for (unsigned i = 0; i < area; i++)
		output[i] = work_bits[i];
	return;
}

void bmphandler::copy_bmp(float* output)
{
	for (unsigned i = 0; i < area; i++)
		output[i] = bmp_bits[i];
	return;
}

void bmphandler::copy_tissue(tissuelayers_size_t idx, tissues_size_t* output)
{
	if (tissuelayers.size() <= idx)
		return;

	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned i = 0; i < area; i++)
		output[i] = tissues[i];
	return;
}

void bmphandler::newbmp(unsigned short width1, unsigned short height1, bool init)
{
	unsigned areanew = unsigned(width1) * height1;
	width = width1;
	height = height1;
	if (area != areanew)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}
		area = areanew;
		sliceprovide = sliceprovide_installer->install(area);
		bmp_bits = sliceprovide->give_me();
		work_bits = sliceprovide->give_me();
		help_bits = sliceprovide->give_me();
		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}
	else
	{
		if (!loaded)
		{
			sliceprovide = sliceprovide_installer->install(area);
			bmp_bits = sliceprovide->give_me();
			work_bits = sliceprovide->give_me();
			help_bits = sliceprovide->give_me();
			tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
			clear_tissue(0);
		}
	}

	tissues_size_t* tissues = tissuelayers[0];

	if (init)
	{
		std::fill(bmp_bits, bmp_bits + area, 0.f);
		std::fill(work_bits, work_bits + area, 0.f);
		std::fill(help_bits, help_bits + area, 0.f);
		std::fill(tissues, tissues + area, 0);
	}

	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();
}

void bmphandler::newbmp(unsigned short width1, unsigned short height1,
		float* bits)
{
	unsigned areanew = unsigned(width1) * height1;
	width = width1;
	height = height1;
	if (area != areanew)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}
		area = areanew;
		sliceprovide = sliceprovide_installer->install(area);
		bmp_bits = bits;
		work_bits = sliceprovide->give_me();
		help_bits = sliceprovide->give_me();
		tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}
	else
	{
		if (!loaded)
		{
			sliceprovide = sliceprovide_installer->install(area);
			bmp_bits = bits;
			work_bits = sliceprovide->give_me();
			help_bits = sliceprovide->give_me();
			tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
			clear_tissue(0);
		}
	}

	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();
}

void bmphandler::freebmp()
{
	if (loaded)
	{
		clear_stack();
		sliceprovide->take_back(bmp_bits);
		sliceprovide->take_back(work_bits);
		sliceprovide->take_back(help_bits);
		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			free(tissuelayers[idx]);
		}
		tissuelayers.clear();
		sliceprovide_installer->uninstall(sliceprovide);
	}

	area = 0;
	loaded = false;
	clear_marks();
	clear_vvm();
	clear_limits();

	return;
}

int bmphandler::CheckBMPDepth(const char* filename)
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

void bmphandler::SetConverterFactors(int newRedFactor, int newGreenFactor,
		int newBlueFactor)
{
	redFactor = newRedFactor / 100.00;
	greenFactor = newGreenFactor / 100.00;
	blueFactor = newBlueFactor / 100.00;
}

int bmphandler::LoadDIBitmap(const char* filename) /* I - File to load */
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

	width = (short unsigned)bmpinfo->bmiHeader.biWidth;
	height = (short unsigned)abs(bmpinfo->bmiHeader.biHeight);
	unsigned newarea = height * (unsigned int)width;

	if ((bitsize = bmpinfo->bmiHeader.biSizeImage) == 0)
	{
		int padding = width % 4;
		if (padding != 0)
			padding = 4 - padding;
		bitsize = (unsigned int)(width + padding) * height;
	}

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}

	clear_tissue(0);

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
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}
	}
	else
	{
		unsigned long i = 0;
		unsigned long j = 0;
		unsigned incr = 4 - (width % 4);
		if (incr == 4)
			incr = 0;
		for (unsigned i1 = 0; i1 < height; i1++)
		{
			for (unsigned j1 = 0; j1 < width; j1++)
			{
				work_bits[i] = bmp_bits[i] = (float)bits_tmp[j];
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
	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();
	mode1 = mode2 = 1;
	return 1;
}

int bmphandler::LoadDIBitmap(const char* filename, Point p, unsigned short dx,
		unsigned short dy) /* I - File to load */
{
	FILE* fp; /* Open file pointer */
	unsigned char* bits_tmp;
	unsigned int bitsize;		 /* Size of bitmap */
	BITMAPFILEHEADER header; /* File header */

	width = dx;
	height = dy;
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

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			free(bmpinfo);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}

	clear_tissue(0);

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
		sliceprovide->take_back(bmp_bits);
		sliceprovide->take_back(work_bits);
		sliceprovide->take_back(help_bits);
		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			free(tissuelayers[idx]);
		}
		tissuelayers.clear();
		free(bits_tmp);
		fclose(fp);
		free(bmpinfo);
		return 0;
	}

	for (unsigned short n = 0; n < dy; n++)
	{
		if ((unsigned short)fread(bits_tmp + n * dx, 1, dx, fp) < dx)
		{
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
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
				sliceprovide->take_back(bmp_bits);
				sliceprovide->take_back(work_bits);
				sliceprovide->take_back(help_bits);
				for (tissuelayers_size_t idx = 0; idx < tissuelayers.size();
						 ++idx)
				{
					free(tissuelayers[idx]);
				}
				tissuelayers.clear();
				free(bits_tmp);
				fclose(fp);
				free(bmpinfo);
				return 0;
			}
		}
	}

	for (unsigned int i = 0; i < area; i++)
	{
		work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
	}

	free(bits_tmp);
	free(bmpinfo);

	/* OK, everything went fine - return the allocated bitmap... */
	fclose(fp);
	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();
	mode1 = mode2 = 1;
	return 1;
}

void bmphandler::SetRGBtoGrayScaleFactors(double newRedFactor,
		double newGreenFactor,
		double newBlueFactor)
{
	redFactor = newRedFactor;
	greenFactor = newGreenFactor;
	blueFactor = newBlueFactor;
}

int bmphandler::ConvertImageTo8BitBMP(const char* filename,
		unsigned char*& bits_tmp)
{
	// construct image from reading an image file.
	cimg_library::CImg<float> srcNoNorm(filename);
	cimg_library::CImg<unsigned char> src = srcNoNorm.get_normalize(0, 255);

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

			bits_tmp[counter] = (unsigned char)(redFactor * r + greenFactor * g + blueFactor * b);
		}
	}

	return 1;
}

int bmphandler::ConvertPNGImageTo8BitBMP(const char* filename,
		unsigned char*& bits_tmp)
{
	QImage sourceImage(filename);

	unsigned int counter = 0;
	QColor oldColor;
	for (int y = sourceImage.height() - 1; y >= 0; y--)
	{
		for (int x = 0; x < sourceImage.width(); x++)
		{
			oldColor = QColor(sourceImage.pixel(x, y));
			bits_tmp[counter] = (unsigned char)(redFactor * oldColor.red() +
																					greenFactor * oldColor.green() +
																					blueFactor * oldColor.blue());
			counter++;
		}
	}

	return 1;
}

int bmphandler::ReloadDIBitmap(const char* filename) /* I - File to load */
{
	if (!loaded)
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

	if (width != (short unsigned)bmpinfo->bmiHeader.biWidth ||
			height != (short unsigned)abs(bmpinfo->bmiHeader.biHeight))
	{
		free(bmpinfo);
		fclose(fp);
		return 0;
	}

	if ((bitsize = bmpinfo->bmiHeader.biSizeImage) == 0)
	{
		int padding = width % 4;
		if (padding != 4)
			padding = 4 - padding;
		bitsize = (unsigned int)(width + padding) * height;
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

	if (bitsize == area)
	{
		for (unsigned int i = 0; i < bitsize; i++)
		{
			bmp_bits[i] = (float)bits_tmp[i];
		}
	}
	else
	{
		unsigned long i = 0;
		unsigned long j = 0;
		unsigned incr = 4 - (width % 4);
		if (incr == 4)
			incr = 0;
		for (unsigned i1 = 0; i1 < height; i1++)
		{
			for (unsigned j1 = 0; j1 < width; j1++)
			{
				bmp_bits[i] = (float)bits_tmp[j];
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
	mode1 = 1;

	return 1;
}

int bmphandler::ReloadDIBitmap(const char* filename, Point p)
{
	if (!loaded)
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

	bitsize = area;

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
	int result = _fseeki64(
			fp, (__int64)(w + incr) * p.py + p.px + header.bfOffBits, SEEK_CUR);
#else
	int result = fseek(fp, (size_t)(w + incr) * p.py + p.px + header.bfOffBits,
			SEEK_CUR);
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

	for (unsigned short n = 0; n < height; n++)
	{
		if ((unsigned short)fread(bits_tmp + n * width, 1, width, fp) < width)
		{
			/*			free(bmp_bits);
			free(work_bits);
			free(help_bits);*/
			free(bmpinfo);
			free(bits_tmp);
			fclose(fp);
			return 0;
		}
		if (n + 1 != height)
		{
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)w + incr - width, SEEK_CUR);
#else
			result = fseek(fp, (size_t)w + incr - width, SEEK_CUR);
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

	for (unsigned int i = 0; i < area; i++)
	{
		bmp_bits[i] = (float)bits_tmp[i];
	}

	free(bits_tmp);
	free(bmpinfo);

	fclose(fp);
	mode1 = 1;

	return 1;
}

int bmphandler::CheckPNGDepth(const char* filename)
{
	QImage image(filename);
	return image.depth();
}

int bmphandler::LoadPNGBitmap(const char* filename)
{
	unsigned char* bits_tmp;
	unsigned int bitsize; /* Size of bitmap */

	//Check if the file exists
	QImage image(filename);
	if (image.isNull())
	{
		return 0;
	}
	width = (short unsigned)image.width();
	height = (short unsigned)image.height();
	unsigned newarea = height * (unsigned int)width;
	bitsize = (unsigned int)(width * height);

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			return 0;
		}

		tissuelayers.push_back((tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}

	clear_tissue(0);

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
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}
	}
	else
	{
		unsigned long i = 0;
		unsigned long j = 0;
		unsigned incr = 4 - (width % 4);
		if (incr == 4)
			incr = 0;
		for (unsigned i1 = 0; i1 < height; i1++)
		{
			for (unsigned j1 = 0; j1 < width; j1++)
			{
				work_bits[i] = bmp_bits[i] = (float)bits_tmp[j];
				i++;
				j++;
			}
			j += incr;
		}
	}

	free(bits_tmp);

	/* OK, everything went fine - return the allocated bitmap... */
	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();
	mode1 = mode2 = 1;
	return 1;
}

bool bmphandler::LoadArray(float* bits, unsigned short w1, unsigned short h1)
{
	width = w1;
	height = h1;

	unsigned newarea = height * (unsigned int)width;

	if (newarea == 0)
		return 0;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}

	clear_tissue(0);

	for (unsigned int i = 0; i < area; i++)
	{
		work_bits[i] = bmp_bits[i] = bits[i];
	}

	/* OK, everything went fine - return the allocated bitmap... */
	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();

	mode1 = mode2 = 1;

	return 1;
}

bool bmphandler::LoadArray(float* bits, unsigned short w, unsigned short h,
		Point p, unsigned short dx, unsigned short dy)
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

	width = dx;
	height = dy;
	unsigned newarea = dx * (unsigned int)dy;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}

	clear_tissue(0);

	unsigned int pos1;
	unsigned int pos2 = 0;
	for (unsigned short j = 0; j < dy; j++)
	{
		pos1 = (p.py + j) * (unsigned int)(w) + p.px;
		for (unsigned short i = 0; i < dx; i++, pos1++, pos2++)
		{
			work_bits[pos2] = bmp_bits[pos2] = bits[pos1];
		}
	}

	/* OK, everything went fine - return the allocated bitmap... */
	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();

	mode1 = mode2 = 1;

	return 1;
}

bool bmphandler::ReloadArray(float* bits)
{
	if (!loaded)
		return 0;

	for (unsigned int i = 0; i < area; i++)
	{
		bmp_bits[i] = bits[i];
	}

	mode1 = 1;
	return 1;
}

bool bmphandler::ReloadArray(float* bits, unsigned short w1, unsigned short h1,
		Point p)
{
	if (!loaded)
		return 0;

	if (width + p.px > w1 || height + p.py > h1)
	{
		return 0;
	}

	unsigned int pos1;
	unsigned int pos2 = 0;
	for (unsigned short j = 0; j < height; j++)
	{
		pos1 = (p.py + j) * (unsigned int)(w1) + p.px;
		for (unsigned short i = 0; i < width; i++, pos1++, pos2++)
		{
			work_bits[pos2] = bmp_bits[pos2] = bits[pos1];
		}
	}

	/* OK, everything went fine - return the allocated bitmap... */
	mode1 = 1;

	return 1;
}

bool bmphandler::LoadDICOM(const char* filename)
{
	DicomReader dcmread;

	if (!dcmread.opendicom(filename))
		return 0;

	width = dcmread.get_width();
	height = dcmread.get_height();

	unsigned newarea = height * (unsigned int)width;

	if (newarea == 0)
		return 0;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}

	clear_tissue(0);

	if (!dcmread.load_picture(bmp_bits))
	{
		if (!dcmread.load_pictureGDCM(filename, bmp_bits))
		{
			dcmread.closedicom();
			return 0;
		}
	}

	for (unsigned int i = 0; i < area; i++)
	{
		work_bits[i] = bmp_bits[i];
	}

	/* OK, everything went fine - return the allocated bitmap... */
	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();

	dcmread.closedicom();
	mode1 = mode2 = 1;

	return 1;
}

bool bmphandler::LoadDICOM(const char* filename, Point p, unsigned short dx,
		unsigned short dy)
{
	DicomReader dcmread;
	dcmread.opendicom(filename);

	unsigned short w = dcmread.get_width();
	unsigned short h = dcmread.get_height();
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

	width = dx;
	height = dy;
	unsigned newarea = dx * (unsigned int)dy;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			dcmread.closedicom();
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
	}

	clear_tissue(0);

	dcmread.load_picture(bmp_bits, p, dx, dy);

	for (unsigned int i = 0; i < area; i++)
	{
		work_bits[i] = bmp_bits[i];
	}

	/* OK, everything went fine - return the allocated bitmap... */
	loaded = true;
	clear_marks();
	clear_vvm();
	clear_limits();

	dcmread.closedicom();
	mode1 = mode2 = 1;

	return 1;
}

bool bmphandler::ReloadDICOM(const char* filename)
{
	if (!loaded)
		return 0;
	//FILE             *fp;          /* Open file pointer */
	DicomReader dcmread;

	dcmread.opendicom(filename);

	if (width != dcmread.get_width() || height != dcmread.get_height())
	{
		dcmread.closedicom();
		return 0;
	}

	dcmread.load_picture(bmp_bits);

	/*for(int i=0;i<area;i++){
		work_bits[i]=bmp_bits[i];
	} */

	/* OK, everything went fine - return the allocated bitmap... */
	dcmread.closedicom();
	mode1 = 1;
	return 1;
}

bool bmphandler::ReloadDICOM(const char* filename, Point p)
{
	if (!loaded)
		return 0;
	//FILE             *fp;          /* Open file pointer */
	DicomReader dcmread;

	dcmread.opendicom(filename);

	if (width + p.px > dcmread.get_width() ||
			height + p.py > dcmread.get_height())
	{
		dcmread.closedicom();
		return 0;
	}

	dcmread.load_picture(bmp_bits, p, width, height);

	/*for(int i=0;i<area;i++){
		work_bits[i]=bmp_bits[i];
	} */

	/* OK, everything went fine - return the allocated bitmap... */
	dcmread.closedicom();
	mode1 = 1;

	return 1;
}

/*
 * 'SaveDIBitmap()' - Save a DIB/BMP file to disk.
 *
 * Returns 0 on success or -1 on failure...
 */

//int                                /* O - 0 = success, -1 = failure */
//bmphandler::SaveDIBitmap(const char *filename,float *p_bits) /* I - File to load */
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

FILE* bmphandler::save_proj(FILE* fp, bool inclpics)
{
	if (loaded)
	{
		fwrite(&width, 1, sizeof(unsigned short), fp);
		fwrite(&height, 1, sizeof(unsigned short), fp);
		if (inclpics)
		{
			fwrite(bmp_bits, 1, area * sizeof(float), fp);
			fwrite(work_bits, 1, area * sizeof(float), fp);
			fwrite(tissuelayers[0], 1, area * sizeof(tissues_size_t),
					fp); // TODO
		}
		int size = -1 - int(marks.size());
		fwrite(&size, 1, sizeof(int), fp);
		int marksVersion = 2;
		fwrite(&marksVersion, 1, sizeof(int), fp);
		for (auto& m : marks)
		{
			fwrite(&(m.mark), 1, sizeof(unsigned), fp);
			fwrite(&(m.p.px), 1, sizeof(unsigned short), fp);
			fwrite(&(m.p.py), 1, sizeof(unsigned short), fp);
			if (marksVersion >= 2)
			{
				int size1 = m.name.length();
				fwrite(&size1, 1, sizeof(int), fp);
				fwrite(m.name.c_str(), 1, sizeof(char) * size1, fp);
			}
		}
		size = int(vvm.size());
		fwrite(&size, 1, sizeof(int), fp);

		for (auto& it1 : vvm)
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
		size = int(limits.size());
		fwrite(&size, 1, sizeof(int), fp);
		for (auto& it1 : limits)
		{
			size = int(it1.size());
			fwrite(&size, 1, sizeof(int), fp);
			for (auto& it : it1)
			{
				fwrite(&(it.px), 1, sizeof(unsigned short), fp);
				fwrite(&(it.py), 1, sizeof(unsigned short), fp);
			}
		}

		fwrite(&mode1, 1, sizeof(unsigned char), fp);
		fwrite(&mode2, 1, sizeof(unsigned char), fp);
	}
	return fp;
}

FILE* bmphandler::save_stack(FILE* fp)
{
	if (loaded)
	{
		fwrite(&stackcounter, 1, sizeof(unsigned), fp);

		int size = -int(stackindex.size()) - 1;
		fwrite(&size, 1, sizeof(int), fp);
		int stackVersion = 1;
		fwrite(&stackVersion, 1, sizeof(int), fp);
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
			fwrite(*it, 1, sizeof(float) * area, fp);
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

FILE* bmphandler::load_proj(FILE* fp, int tissuesVersion, bool inclpics, bool init)
{
	unsigned short width1, height1;
	fread(&width1, sizeof(unsigned short), 1, fp);
	fread(&height1, sizeof(unsigned short), 1, fp);

	newbmp(width1, height1, init);

	if (inclpics)
	{
		fread(bmp_bits, area * sizeof(float), 1, fp);
		fread(work_bits, area * sizeof(float), 1, fp);
		tissues_size_t* tissues = tissuelayers[0]; // TODO
		if (tissuesVersion > 0)
		{
			fread(tissues, area * sizeof(tissues_size_t), 1, fp);
		}
		else
		{
			unsigned char* ucharBuffer = (unsigned char*)malloc(area);
			fread(ucharBuffer, area, 1, fp);
			for (unsigned int i = 0; i < area; ++i)
			{
				tissues[i] = (tissues_size_t)ucharBuffer[i];
			}
			free(ucharBuffer);
		}
	}

	int size, size1;
	fread(&size, sizeof(int), 1, fp);

	int marksVersion = 0;
	if (size < 0)
	{
		size = -1 - size;
		fread(&marksVersion, sizeof(int), 1, fp);
	}

	Mark m;
	marks.clear();
	char name[100];
	for (int j = 0; j < size; j++)
	{
		fread(&(m.mark), sizeof(unsigned), 1, fp);
		fread(&(m.p.px), sizeof(unsigned short), 1, fp);
		fread(&(m.p.py), sizeof(unsigned short), 1, fp);
		if (marksVersion >= 2)
		{
			int size1;
			fread(&size1, sizeof(int), 1, fp);
			fread(name, sizeof(char) * size1, 1, fp);
			name[size1] = '\0';
			m.name = std::string(name);
		}
		marks.push_back(m);
	}

	clear_vvm();
	fread(&size1, sizeof(int), 1, fp);

	vvm.resize(size1);
	for (int j1 = 0; j1 < size1; j1++)
	{
		fread(&size, sizeof(int), 1, fp);
		for (int j = 0; j < size; j++)
		{
			fread(&(m.mark), sizeof(unsigned), 1, fp);
			fread(&(m.p.px), sizeof(unsigned short), 1, fp);
			fread(&(m.p.py), sizeof(unsigned short), 1, fp);
			vvm[j1].push_back(m);
		}
		if (size > 0)
		{
			maxim_store = std::max(maxim_store, vvm[j1].begin()->mark);
		}
	}

	Point p1;
	clear_limits();
	fread(&size1, sizeof(int), 1, fp);

	limits.resize(size1);
	for (int j1 = 0; j1 < size1; j1++)
	{
		fread(&size, sizeof(int), 1, fp);
		for (int j = 0; j < size; j++)
		{
			fread(&(p1.px), sizeof(unsigned short), 1, fp);
			fread(&(p1.py), sizeof(unsigned short), 1, fp);
			limits[j1].push_back(p1);
		}
	}

	if (marksVersion > 0)
	{
		fread(&mode1, sizeof(unsigned char), 1, fp);
		fread(&mode2, sizeof(unsigned char), 1, fp);
	}
	else
	{
		mode1 = 1;
		mode2 = 2;
	}

	return fp;
}

FILE* bmphandler::load_stack(FILE* fp)
{
	fread(&stackcounter, sizeof(unsigned), 1, fp);

	int size1;
	fread(&size1, sizeof(int), 1, fp);
	int stackVersion = 0;
	if (size1 < 0)
	{
		size1 = -1 - size1;
		fread(&stackVersion, sizeof(int), 1, fp);
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
		f = sliceprovide->give_me();
		fread(f, sizeof(float) * area, 1, fp);
		bits_stack.push_back(f);
	}

	if (stackVersion > 0)
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
//bmphandler::SaveDIBitmap(const char *filename,float *p_bits) /* I - File to load */
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
int /* O - 0 = success, -1 = failure */
		bmphandler::SaveDIBitmap(const char* filename,
				float* p_bits) /* I - File to load */
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
	bmpinfo1->bmiHeader.biHeight = (int)height;
	bmpinfo1->bmiHeader.biWidth = (int)width;
	bmpinfo1->bmiHeader.biXPelsPerMeter = 0;
	bmpinfo1->bmiHeader.biYPelsPerMeter = 0;

	int padding = width % 4;
	if (padding != 0)
		padding = 4 - padding;
	bitsize = area + padding * height;

	bits_tmp = (unsigned char*)malloc(bitsize);
	if (bits_tmp == nullptr)
		return -1;

	unsigned long pos = 0;
	unsigned long pos2 = 0;
	for (unsigned short j = 0; j < height; j++)
	{
		for (unsigned short i = 0; i < width; i++, pos++, pos2++)
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

int bmphandler::SaveTissueBitmap(tissuelayers_size_t idx, const char *filename)
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

int bmphandler::SaveTissueBitmap(tissuelayers_size_t idx, const char* filename)
{
	vtkSmartPointer<vtkImageData> imageSource =
			vtkSmartPointer<vtkImageData>::New();
	imageSource->SetExtent(0, width - 1, 0, height - 1, 0, 0);
	imageSource->AllocateScalars(VTK_UNSIGNED_CHAR, 4); // 32bpp RGBA
	unsigned char* field =
			(unsigned char*)imageSource->GetScalarPointer(0, 0, 0);

	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < (unsigned int)width * height; ++i)
	{
		auto tissueColor = TissueInfos::GetTissueColor(tissues[i]);
		field[i * 4] = (tissueColor[0]) * 255;
		field[i * 4 + 1] = (tissueColor[1]) * 255;
		field[i * 4 + 2] = (tissueColor[2]) * 255;
		field[i * 4 + 3] = 0;
	}

	auto bmpWriter = vtkSmartPointer<vtkBMPWriter>::New();
	bmpWriter->SetFileName(filename);
	bmpWriter->SetInputData(imageSource);
	bmpWriter->Write();

	return (0);
}

#endif

int bmphandler::SaveDIBitmap(const char* filename)
{
	return SaveDIBitmap(filename, bmp_bits);
}

int bmphandler::SaveWorkBitmap(const char* filename)
{
	return SaveDIBitmap(filename, work_bits);
}

int bmphandler::ReadAvw(const char* filename, short unsigned slicenr)
{
	unsigned int bitsize; /* Size of bitmap */

	unsigned short w, h;
	avw::datatype type;

	void* data = avw::ReadData(filename, slicenr, w, h, type);
	if (data == nullptr)
	{
		return 0;
	}

	width = w;
	height = h;
	unsigned newarea = height * (unsigned int)width;

	bitsize = newarea;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			free(data);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			free(data);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			free(data);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			free(data);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			free(data);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			free(data);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}

	loaded = true;

	if (type == avw::schar)
	{
		char* bits_tmp = (char*)data;

		for (unsigned int i = 0; i < bitsize; i++)
		{
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}

		free(data);
	}
	else if (type == avw::uchar)
	{
		unsigned char* bits_tmp = (unsigned char*)data;

		for (unsigned int i = 0; i < bitsize; i++)
		{
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}

		free(data);
	}
	else if (type == avw::ushort)
	{
		unsigned short* bits_tmp = (unsigned short*)data;

		for (unsigned int i = 0; i < bitsize; i++)
		{
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}

		free(data);
	}
	else if (type == avw::sshort)
	{
		short* bits_tmp = (short*)data;

		for (unsigned int i = 0; i < bitsize; i++)
		{
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}

		free(data);
	}
	else
	{
		return 0;
	}

	clear_marks();
	clear_vvm();
	clear_limits();
	mode1 = mode2 = 2;
	return 1;
}

int bmphandler::ReadRaw(const char* filename, short unsigned w,
		short unsigned h, unsigned bitdepth,
		unsigned short slicenr)
{
	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	width = w;
	height = h;
	unsigned newarea = height * (unsigned int)width;

	bitsize = newarea;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		if (!tissuelayers[0])
		{
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}
		clear_tissue(0);
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		if (!tissuelayers[0])
		{
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return 0;
		}
		clear_tissue(0);
	}

	loaded = true;

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;

		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
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
			std::cerr << "bmphandler::ReadRaw() : error, file operation failed"
								<< endl;
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, bitsize, fp) < area)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "bmphandler::ReadRaw() : error, file operation failed"
								<< endl;
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
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
			std::cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
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
			std::cerr << "bmphandler::ReadRaw() : error, file operation failed"
								<< endl;
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, (size_t)(bitsize)*2, fp) < area * 2)
		{
			/*			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);*/
			std::cerr << "bmphandler::ReadRaw() : error, file operation failed"
								<< endl;
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		/*		sliceprovide->take_back(bmp_bits);
		sliceprovide->take_back(work_bits);
		sliceprovide->take_back(help_bits);*/
		std::cerr << "bmphandler::ReadRaw() : error, unsupported depth..." << endl;
		fclose(fp);
		return 0;
	}

	fclose(fp);
	clear_marks();
	clear_vvm();
	clear_limits();
	mode1 = mode2 = 1;
	return 1;
}

int bmphandler::ReadRaw(const char* filename, short unsigned w,
		short unsigned h, unsigned bitdepth,
		unsigned short slicenr, Point p, unsigned short dx,
		unsigned short dy)
{
	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	width = dx;
	height = dy;
	unsigned newarea = height * (unsigned int)width;
	unsigned int area2 = h * (unsigned int)w;

	bitsize = newarea;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}

	loaded = true;

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
		int result = _fseeki64(
				fp, (__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px, SEEK_SET);
#else
		int result = fseek(
				fp, (size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px, SEEK_SET);
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
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
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
		int result = _fseeki64(
				fp, ((__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px) * 2,
				SEEK_SET);
#else
		int result =
				fseek(fp, ((size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px) * 2,
						SEEK_SET);
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
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
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
	clear_marks();
	clear_vvm();
	clear_limits();
	mode1 = mode2 = 1;
	return 1;
}

int bmphandler::ReadRawFloat(const char* filename, short unsigned w,
		short unsigned h, unsigned short slicenr)
{
	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	width = w;
	height = h;
	unsigned newarea = height * (unsigned int)width;

	bitsize = newarea;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}

	loaded = true;

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

	if (fread(bmp_bits, 1, bitsize * sizeof(float), fp) < area * sizeof(float))
	{
		/*			sliceprovide->take_back(bmp_bits);
		sliceprovide->take_back(work_bits);
		sliceprovide->take_back(help_bits);*/
		fclose(fp);
		return 0;
	}

	for (unsigned int i = 0; i < bitsize; i++)
	{
		work_bits[i] = bmp_bits[i];
	}

	fclose(fp);
	clear_marks();
	clear_vvm();
	clear_limits();
	mode1 = mode2 = 1;
	return 1;
}

int bmphandler::ReadRawFloat(const char* filename, short unsigned w,
		short unsigned h, unsigned short slicenr, Point p,
		unsigned short dx, unsigned short dy)
{
	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	width = dx;
	height = dy;
	unsigned newarea = height * (unsigned int)width;
	unsigned int area2 = h * (unsigned int)w;

	bitsize = newarea;

	if (area != newarea)
	{
		if (loaded)
		{
			clear_stack();
			sliceprovide->take_back(bmp_bits);
			sliceprovide->take_back(work_bits);
			sliceprovide->take_back(help_bits);
			for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
			{
				free(tissuelayers[idx]);
			}
			tissuelayers.clear();
			sliceprovide_installer->uninstall(sliceprovide);
		}

		area = newarea;

		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}
	else if (!loaded)
	{
		sliceprovide = sliceprovide_installer->install(area);

		if ((bmp_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((work_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		if ((help_bits = sliceprovide->give_me()) == nullptr)
		{
			/* Couldn't allocate memory - return nullptr! */
			//       free(*info);
			fclose(fp);
			return 0;
		}

		tissuelayers.push_back(
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area));
		clear_tissue(0);
	}

	loaded = true;

#ifdef _MSC_VER
	int result = _fseeki64(
			fp,
			((__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px) * sizeof(float),
			SEEK_SET);
#else
	int result = fseek(
			fp, ((size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px) * sizeof(float),
			SEEK_SET);
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
		if ((unsigned short)fread(bmp_bits + n * dx, 1, dx * sizeof(float),
						fp) < sizeof(float) * dx)
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
		work_bits[i] = bmp_bits[i];
	}

	fclose(fp);
	clear_marks();
	clear_vvm();
	clear_limits();
	mode1 = mode2 = 1;
	return 1;
}

int bmphandler::ReloadRaw(const char* filename, unsigned bitdepth,
		unsigned slicenr)
{
	if (!loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = area;

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

		if (fread(bits_tmp, 1, bitsize, fp) < area)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			bmp_bits[i] = (float)bits_tmp[i];
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

		if (fread(bits_tmp, 1, bitsize * 2, fp) < area * 2)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned int i = 0; i < bitsize; i++)
		{
			bmp_bits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	mode1 = 1;
	return 1;
}

int bmphandler::ReloadRaw(const char* filename, short unsigned w,
		short unsigned h, unsigned bitdepth, unsigned slicenr,
		Point p)
{
	if (!loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = area;
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
		int result = _fseeki64(
				fp, (__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px, SEEK_SET);
#else
		int result = fseek(
				fp, (size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px, SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned short n = 0; n < height; n++)
		{
			if ((unsigned short)fread(bits_tmp + n * width, 1, width, fp) <
					width)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)w - width, SEEK_CUR);
#else
			result = fseek(fp, (size_t)w - width, SEEK_CUR);
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
			bmp_bits[i] = (float)bits_tmp[i];
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
		int result = _fseeki64(
				fp, ((__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px) * 2,
				SEEK_SET);
#else
		int result = fseek(fp, ((size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px) * 2,
				SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (unsigned short n = 0; n < height; n++)
		{
			if ((unsigned short)fread(bits_tmp + n * width, 1, width * 2, fp) <
					2 * width)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, ((__int64)(w)-width) * 2, SEEK_CUR);
#else
			result = fseek(fp, ((size_t)(w)-width) * 2, SEEK_CUR);
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
			bmp_bits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	mode1 = 1;
	return 1;
}

int bmphandler::ReloadRawFloat(const char* filename, unsigned slicenr)
{
	if (!loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = area;

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

	if (fread(bmp_bits, 1, bitsize * sizeof(float), fp) < area * sizeof(float))
	{
		fclose(fp);
		return 0;
	}

	fclose(fp);
	mode1 = 1;
	return 1;
}

float* bmphandler::ReadRawFloat(const char* filename, unsigned slicenr,
		unsigned int area)
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

int bmphandler::ReloadRawFloat(const char* filename, short unsigned w,
		short unsigned h, unsigned slicenr, Point p)
{
	if (!loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = area;
	unsigned long area2 = (unsigned long)(w)*h;

#ifdef _MSC_VER
	int result = _fseeki64(
			fp,
			((__int64)(area2)*slicenr + (__int64)(w)*p.py + p.px) * sizeof(float),
			SEEK_SET);
#else
	int result = fseek(
			fp, ((size_t)(area2)*slicenr + (size_t)(w)*p.py + p.px) * sizeof(float),
			SEEK_SET);
#endif
	if (result)
	{
		fclose(fp);
		return 0;
	}

	for (unsigned short n = 0; n < height; n++)
	{
		if ((unsigned short)fread(bmp_bits + n * width, 1,
						width * sizeof(float),
						fp) < sizeof(float) * width)
		{
			fclose(fp);
			return 0;
		}
#ifdef _MSC_VER
		result = _fseeki64(fp, (__int64)(w - width) * sizeof(float), SEEK_CUR);
#else
		result = fseek(fp, (size_t)(w - width) * sizeof(float), SEEK_CUR);
#endif
		if (result)
		{
			fclose(fp);
			return 0;
		}
	}

	fclose(fp);
	mode1 = 1;
	return 1;
}

int bmphandler::ReloadRawTissues(const char* filename, unsigned bitdepth, unsigned slicenr)
{
	if (!loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap stack */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = area * static_cast<unsigned>(tissuelayers.size());

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

		if (fread(bits_tmp, 1, bitsize, fp) < area)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = tissuelayers[idx];
			for (unsigned int i = 0; i < area; i++)
			{
				tissues[i] = (tissues_size_t)bits_tmp[i + idx * area];
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

		if (fread(bits_tmp, 1, bitsize * 2, fp) < area * 2)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = tissuelayers[idx];
			for (unsigned int i = 0; i < area; i++)
			{
				tissues[i] = (tissues_size_t)bits_tmp[i + idx * area];
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

int bmphandler::ReloadRawTissues(const char* filename, short unsigned w,
		short unsigned h, unsigned bitdepth,
		unsigned slicenr, Point p)
{
	if (!loaded)
		return 0;

	FILE* fp;							/* Open file pointer */
	unsigned int bitsize; /* Size of bitmap stack */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	bitsize = area * static_cast<unsigned>(tissuelayers.size());
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
		int result = _fseeki64(fp,
				(__int64)(area2)*slicenr * tissuelayers.size() +
						(__int64)(w)*p.py + p.px,
				SEEK_SET);
#else
		int result = fseek(fp,
				(size_t)(area2)*slicenr * tissuelayers.size() +
						(size_t)(w)*p.py + p.px,
				SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			for (unsigned short n = 0; n < height; n++)
			{
				if ((unsigned short)fread(bits_tmp + area * idx + n * width, 1,
								width, fp) < width)
				{
					free(bits_tmp);
					fclose(fp);
					return 0;
				}
#ifdef _MSC_VER
				result = _fseeki64(fp, (__int64)(w - width), SEEK_CUR);
#else
				result = fseek(fp, (size_t)(w - width), SEEK_CUR);
#endif
				if (result)
				{
					free(bits_tmp);
					fclose(fp);
					return 0;
				}
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)(h - height) * w, SEEK_CUR);
#else
			result = fseek(fp, (size_t)(h - height) * w, SEEK_CUR);
#endif
			if (result)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}

		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = tissuelayers[idx];
			for (unsigned int i = 0; i < area; i++)
			{
				tissues[i] = (tissues_size_t)bits_tmp[i + idx * area];
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
		int result = _fseeki64(fp,
				((__int64)(area2)*slicenr * tissuelayers.size() +
						(__int64)(w)*p.py + p.px) *
						2,
				SEEK_SET);
#else
		int result = fseek(fp,
				((size_t)(area2)*slicenr * tissuelayers.size() +
						(size_t)(w)*p.py + p.px) *
						2,
				SEEK_SET);
#endif
		if (result)
		{
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			for (unsigned short n = 0; n < height; n++)
			{
				if ((unsigned short)fread(bits_tmp + area * idx + n * width, 1,
								width * 2, fp) < 2 * width)
				{
					free(bits_tmp);
					fclose(fp);
					return 0;
				}
#ifdef _MSC_VER
				result = _fseeki64(fp, (__int64)(w - width) * 2, SEEK_CUR);
#else
				result = fseek(fp, (size_t)(w - width) * 2, SEEK_CUR);
#endif
				if (result)
				{
					free(bits_tmp);
					fclose(fp);
					return 0;
				}
			}
#ifdef _MSC_VER
			result = _fseeki64(fp, (__int64)(h - height) * w * 2, SEEK_CUR);
#else
			result = fseek(fp, (size_t)(h - height) * w * 2, SEEK_CUR);
#endif
			if (result)
			{
				free(bits_tmp);
				fclose(fp);
				return 0;
			}
		}

		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = tissuelayers[idx];
			for (unsigned int i = 0; i < area; i++)
			{
				tissues[i] = (tissues_size_t)bits_tmp[i + idx * area];
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

int bmphandler::SaveBmpRaw(const char* filename)
{
	return SaveRaw(filename, bmp_bits);
}

int bmphandler::SaveWorkRaw(const char* filename)
{
	return SaveRaw(filename, work_bits);
}

int bmphandler::SaveRaw(const char* filename, float* p_bits)
{
	FILE* fp;
	unsigned char* bits_tmp;

	bits_tmp = (unsigned char*)malloc(area);
	if (bits_tmp == nullptr)
		return -1;

	for (unsigned int i = 0; i < area; i++)
	{
		bits_tmp[i] = (unsigned char)(std::min(255.0, std::max(0.0, p_bits[i] + 0.5)));
	}

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = width * (unsigned)height;

	if (fwrite(bits_tmp, 1, bitsize, fp) < bitsize)
	{
		fclose(fp);
		return (-1);
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int bmphandler::SaveTissueRaw(tissuelayers_size_t idx, const char* filename)
{
	FILE* fp;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = width * (unsigned)height;

	if ((TissueInfos::GetTissueCount() <= 255) &&
			(sizeof(tissues_size_t) > sizeof(unsigned char)))
	{
		unsigned char* ucharBuffer = new unsigned char[bitsize];
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned int i = 0; i < bitsize; ++i)
		{
			ucharBuffer[i] = (unsigned char)tissues[i];
		}
		if (fwrite(ucharBuffer, sizeof(unsigned char), bitsize, fp) < bitsize)
		{
			fclose(fp);
			delete[] ucharBuffer;
			return (-1);
		}
		delete[] ucharBuffer;
	}
	else
	{
		if (fwrite(tissuelayers[idx], sizeof(tissues_size_t), bitsize, fp) <
				bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	fclose(fp);
	return 0;
}

void bmphandler::set_work_pt(Point p, float f)
{
	work_bits[width * p.py + p.px] = f;
}

void bmphandler::set_bmp_pt(Point p, float f)
{
	bmp_bits[width * p.py + p.px] = f;
}

void bmphandler::set_tissue_pt(tissuelayers_size_t idx, Point p, tissues_size_t f)
{
	tissuelayers[idx][width * p.py + p.px] = f;
}

float bmphandler::bmp_pt(Point p) { return bmp_bits[width * p.py + p.px]; }

float bmphandler::work_pt(Point p) { return work_bits[width * p.py + p.px]; }

tissues_size_t bmphandler::tissues_pt(tissuelayers_size_t idx, Point p)
{
	return bmphandler::tissuelayers[idx][width * p.py + p.px];
}

void bmphandler::print_info()
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

	std::cout << "Width " << width << std::endl;
	std::cout << "Height " << height << std::endl;

	/*if(bmpinfo->bmiHeader.biSize>40){
		for(unsigned int i=0;i<bmpinfo->bmiHeader.biClrUsed;i++)
			cout << "Color " << i << ": " << bmpinfo->bmiColors[i].rgbBlue <<" "<< bmpinfo->bmiColors[i].rgbGreen<< " " << bmpinfo->bmiColors[i].rgbRed<<endl;
	}*/
}

unsigned int bmphandler::return_area() { return area; }

unsigned int bmphandler::return_width() { return width; }

unsigned int bmphandler::return_height() { return height; }

unsigned int bmphandler::make_histogram(bool includeoutofrange)
{
	unsigned int j = 0;
	float k;
	for (int i = 0; i < 256; i++)
		histogram[i] = 0;
	for (unsigned int i = 0; i < area; i++)
	{
		k = work_bits[i];
		if (k < 0 || k >= 256)
		{
			if (includeoutofrange && k < 0)
				histogram[0]++;
			if (includeoutofrange && k >= 256)
				histogram[255]++;
			j++;
		}
		else
			histogram[(int)k]++;
	}
	return j;
}

unsigned int bmphandler::make_histogram(float* mask, float f,
		bool includeoutofrange)
{
	unsigned int j = 0;
	float k;
	for (int i = 0; i < 256; i++)
		histogram[i] = 0;
	for (unsigned int i = 0; i < area; i++)
	{
		if (mask[i] == f)
		{
			k = work_bits[i];
			if (k < 0 || k >= 256)
			{
				if (includeoutofrange && k < 0)
					histogram[0]++;
				if (includeoutofrange && k >= 256)
					histogram[255]++;
				j++;
			}
			else
				histogram[(int)k]++;
		}
	}
	return j;
}

unsigned int bmphandler::make_histogram(Point p, unsigned short dx,
		unsigned short dy,
		bool includeoutofrange)
{
	unsigned int i, l;
	l = 0;
	float m;
	for (i = 0; i < 256; i++)
		histogram[i] = 0;

	dx = std::min(int(dx), width - p.px);
	dy = std::min(int(dy), height - p.py);

	i = pt2coord(p);
	for (int j = 0; j < dy; j++)
	{
		for (int k = 0; k < dx; k++)
		{
			m = work_bits[i];
			if (m < 0 || m >= 256)
			{
				if (includeoutofrange && k < 0)
					histogram[0]++;
				if (includeoutofrange && k >= 256)
					histogram[255]++;
				l++;
			}
			else
				histogram[(int)m]++;
			i++;
		}
		i = i + width - dx;
	}

	return l;
}

unsigned int* bmphandler::return_histogram() { return histogram; }

void bmphandler::print_histogram()
{
	for (int i = 0; i < 256; i++)
		std::cout << histogram[i] << " ";
	std::cout << std::endl;
}

void bmphandler::threshold(float* thresholds)
{
	const short unsigned n = (short unsigned)thresholds[0];

	if (n > 0)
	{
		const float leveldiff = 255.0f / n;

		short unsigned j;

		for (unsigned int i = 0; i < area; i++)
		{
			j = 0;
			while (j < n && bmp_bits[i] > thresholds[j + 1])
				j++;
			work_bits[i] = j * leveldiff;
		}
	}

	mode2 = 2;
}

void bmphandler::threshold(float* thresholds, Point p, unsigned short dx,
		unsigned short dy)
{
	dx = std::min(int(dx), width - p.px);
	dy = std::min(int(dy), width - p.py);

	short unsigned n = (short unsigned)thresholds[0];
	if (n > 0)
	{
		const float leveldiff = 255.0f / n;
		short unsigned l;
		unsigned int i;
		i = pt2coord(p);

		for (int j = 0; j < dy; j++)
		{
			for (int k = 0; k < dx; k++)
			{
				l = 0;
				while (bmp_bits[i] > thresholds[l + 1] && l < n)
					l++;
				work_bits[i] = l * leveldiff;
				i++;
			}
			i = i + width - dx;
		}
	}

	mode2 = 2;
}

void bmphandler::work2bmp()
{
	for (unsigned int i = 0; i < area; i++)
		bmp_bits[i] = work_bits[i];

	mode1 = mode2;
}

void bmphandler::bmp2work()
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = bmp_bits[i];

	mode2 = mode1;
}

void bmphandler::work2tissue(tissuelayers_size_t idx)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		if (work_bits[i] < 0.0f)
			tissues[i] = 0;
		else if (work_bits[i] > (float)TISSUES_SIZE_MAX)
			tissues[i] = TISSUES_SIZE_MAX;
		else
			tissues[i] = (tissues_size_t)floor(work_bits[i] + 0.5f);
	}
}

void bmphandler::mergetissue(tissues_size_t tissuetype, tissuelayers_size_t idx)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		if (work_bits[i] > 0.0f)
			tissues[i] = tissuetype;
	}
}

void bmphandler::swap_bmpwork()
{
	std::swap(bmp_bits, work_bits);
	std::swap(mode1, mode2);
}

void bmphandler::swap_bmphelp()
{
	std::swap(help_bits, bmp_bits);
}

void bmphandler::swap_workhelp()
{
	std::swap(help_bits, work_bits);
}

float* bmphandler::make_gaussfilter(float sigma, int n)
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

float* bmphandler::make_laplacianfilter()
{
	float* filter = (float*)malloc(4 * sizeof(float));

	filter[0] = 3;
	filter[1] = 1;
	filter[2] = -2;
	filter[3] = 1;

	return filter;
}

void bmphandler::convolute(float* mask, unsigned short direction)
{
	unsigned i, n;
	float dummy;
	switch (direction)
	{
	case 0:
		n = (unsigned)mask[0];
		i = 0;
		for (unsigned j = 0; j < height; j++)
		{
			for (unsigned k = 0; k <= width - n; k++)
			{
				dummy = 0;
				for (unsigned l = 0; l < n; l++)
					dummy += mask[l + 1] * bmp_bits[i + l];
				work_bits[i + n / 2] = dummy;
				i++;
			}
			i += n - 1;
		}

		i = 0;
		for (unsigned j = 0; j < n / 2; j++)
		{
			work_bits[i] = 0;
			i++;
		}
		i += width - n + 1;
		for (unsigned k = 0; k + 1 < height; k++)
		{
			for (unsigned j = 0; j < n - 1; j++)
			{
				work_bits[i] = 0;
				i++;
			}
			i += width - n + 1;
		}
		for (; i < area; i++)
			work_bits[i] = 0;
		break;
	case 1:
		n = (unsigned)mask[0];
		i = 0;
		for (unsigned j = 0; j < width * (height - n + 1); j++)
		{
			dummy = 0;
			for (unsigned l = 0; l < n; l++)
				dummy += mask[l + 1] * bmp_bits[i + l * width];
			work_bits[i + (n / 2) * width] = dummy;
			i++;
		}

		for (i = 0; i < (n / 2) * width; i++)
			work_bits[i] = 0;
		for (i = area - (n / 2) * width; i < area; i++)
			work_bits[i] = 0;
		break;
	case 2:
		n = (unsigned)mask[0];
		unsigned m = (unsigned)mask[1];
		i = 0;
		for (unsigned j = 0; j <= height - m; j++)
		{
			for (unsigned k = 0; k <= width - n; k++)
			{
				dummy = 0;
				for (unsigned l = 0; l < n; l++)
				{
					for (unsigned o = 0; o < m; o++)
					{
						dummy +=
								mask[l + n * o + 2] * bmp_bits[i + l + o * width];
					}
				}
				work_bits[i + n / 2 + (m / 2) * width] = dummy;
				i++;
			}
			i += n - 1;
		}

		i = 0;
		for (unsigned j = 0; j < n / 2 + (m / 2) * width; j++)
		{
			work_bits[i] = 0;
			i++;
		}
		i += width - n + 1;
		for (unsigned k = 0; k + m < height; k++)
		{
			for (unsigned j = 0; j < n - 1; j++)
			{
				work_bits[i] = 0;
				i++;
			}
			i += width - n + 1;
		}
		for (; i < area; i++)
			work_bits[i] = 0;
		break;
	}
}

void bmphandler::convolute_hist(float* mask)
{
	float histo[256];
	int n = (int)mask[0];
	float dummy;

	for (int k = 0; k <= 256 - n; k++)
	{
		histo[k + n / 2] = 0;
		for (int l = 0; l < n; l++)
			histo[k + n / 2] += mask[l + 1] * histogram[k + l];
	}

	for (int k = 0; k < n / 2; k++)
	{
		histo[k] = 0;
		dummy = 0;
		for (int i = n / 2 - k; i < n; i++)
		{
			dummy += mask[i + 1];
			histo[k] += mask[i + 1] * histogram[k - n / 2 + i];
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
			histo[k] += mask[i + 1] * histogram[k - n / 2 + i];
		}
		histo[k] = histo[k] / dummy;
	}

	for (int k = 0; k < 256; k++)
		histogram[k] = (unsigned int)(histo[k] + 0.5);
}

void bmphandler::gaussian_hist(float sigma)
{
	int n = int(3 * sigma);
	if (n % 2 == 0)
		n++;

	float* dummy;
	convolute_hist(dummy = make_gaussfilter(sigma, n));

	free(dummy);
}

void bmphandler::get_range(Pair* pp)
{
	if (area > 0)
	{
		auto range = std::minmax_element(work_bits, work_bits + area);
		pp->low = *range.first;
		pp->high = *range.second;
	}
}

void bmphandler::get_rangetissue(tissuelayers_size_t idx, tissues_size_t* pp)
{
	if (area > 0)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		*pp = *std::max_element(tissues, tissues + area);
	}
}

void bmphandler::get_bmprange(Pair* pp)
{
	if (area > 0)
	{
		auto range = std::minmax_element(bmp_bits, bmp_bits + area);
		pp->low = *range.first;
		pp->high = *range.second;
	}
}

void bmphandler::scale_colors(Pair p)
{
	const float step = 255.0f / (p.high - p.low);

	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = (work_bits[i] - p.low) * step;
}

void bmphandler::crop_colors()
{
	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = std::min(std::max(work_bits[i], 0.0f), 255.0f);
}

void bmphandler::gaussian(float sigma)
{
	unsigned char dummymode1 = mode1;
	if (sigma > 0.66f)
	{
		int n = int(3 * sigma);
		if (n % 2 == 0)
			n++;
		float* dummy;
		float* dummy1;
		convolute(dummy = make_gaussfilter(sigma, n), 0);

		dummy1 = bmp_bits;
		bmp_bits = sliceprovide->give_me();
		swap_bmpwork();
		convolute(dummy, 1);
		sliceprovide->take_back(bmp_bits);
		bmp_bits = dummy1;

		free(dummy);
	}
	else
	{
		bmp2work();
	}

	mode1 = dummymode1;
	mode2 = 1;
}

void bmphandler::average(unsigned short n)
{
	unsigned char dummymode1 = mode1;
	float* dummy1;

	if (n % 2 == 0)
		n++;
	float* filter = (float*)malloc((n + 1) * sizeof(float));

	filter[0] = n;
	for (short unsigned int i = 1; i <= n; i++)
		filter[i] = 1.0f / n;

	convolute(filter, 0);
	dummy1 = bmp_bits;
	bmp_bits = sliceprovide->give_me();
	swap_bmpwork();
	convolute(filter, 1);
	sliceprovide->take_back(bmp_bits);
	bmp_bits = dummy1;

	free(filter);

	mode1 = dummymode1;
	mode2 = 1;

	return;
}

void bmphandler::laplacian()
{
	unsigned char dummymode1 = mode1;
	float* dummy;
	float* dummy1;

	convolute(dummy = make_laplacianfilter(), 0);
	dummy1 = bmp_bits;
	bmp_bits = sliceprovide->give_me();
	swap_bmpwork();
	convolute(dummy, 1);
	sliceprovide->take_back(bmp_bits);
	bmp_bits = dummy1;
	//	bmp_abs();

	free(dummy);

	mode1 = dummymode1;
	mode2 = 1;
}

void bmphandler::laplacian1()
{
	unsigned char dummymode1 = mode1;
	float laplacianfilter[12];
	laplacianfilter[0] = 11;
	laplacianfilter[1] = laplacianfilter[2] = 3;
	laplacianfilter[3] = laplacianfilter[5] = laplacianfilter[9] =
			laplacianfilter[11] = -1.0f / 12;
	laplacianfilter[4] = laplacianfilter[6] = laplacianfilter[8] =
			laplacianfilter[10] = -2.0f / 12;
	laplacianfilter[7] = 12.0f / 12;

	convolute(laplacianfilter, 2);

	mode1 = dummymode1;
	mode2 = 1;
}

void bmphandler::gauss_sharpen(float sigma)
{
	unsigned char dummymode1 = mode1;

	gauss_line(sigma);
	bmp_sum();

	mode1 = dummymode1;
	mode2 = 1;
}

void bmphandler::gauss_line(float sigma)
{
	unsigned char dummymode1 = mode1;

	gaussian(sigma);
	bmp_diff();

	mode1 = dummymode1;
	mode2 = 1;
}

void bmphandler::moment_line()
{
	unsigned char dummymode1 = mode1;

	n_moment(3, 2);

	mode1 = dummymode1;
	mode2 = 2;
}

float* bmphandler::direction_map(float* sobelx, float* sobely)
{
	float* direct_map = sliceprovide->give_me();

	int i = width + 1;
	for (int j = 1; j < height - 1; j++)
	{
		for (int k = 1; k < width - 1; k++)
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

void bmphandler::nonmaximum_supr(float* direct_map)
{
	float* results = sliceprovide->give_me();
	float left_bit, right_bit;
	int i = width + 1;
	for (int j = 1; j < height - 1; j++)
	{
		for (int k = 1; k < width - 1; k++)
		{
			if (direct_map[i] < 22.5 || direct_map[i] >= 157.5)
			{
				left_bit = work_bits[i - 1];
				right_bit = work_bits[i + 1];
			}
			else if (direct_map[i] < 67.5)
			{
				left_bit = work_bits[i - 1 - width];
				right_bit = work_bits[i + 1 + width];
			}
			else if (direct_map[i] < 112.5)
			{
				left_bit = work_bits[i + width];
				right_bit = work_bits[i - width];
			}
			else if (direct_map[i] < 157.5)
			{
				left_bit = work_bits[i + width - 1];
				right_bit = work_bits[i - width + 1];
			}

			if (work_bits[i] < left_bit || work_bits[i] < right_bit)
				results[i] = 0;
			else
				results[i] = work_bits[i];
			i++;
		}

		i += 2;
	}

	sliceprovide->take_back(work_bits);
	work_bits = results;
	return;
}

void bmphandler::canny_line(float sigma, float thresh_low, float thresh_high)
{
	unsigned char dummymode1 = mode1;
	float* sobelx = sliceprovide->give_me();
	float* sobely = sliceprovide->give_me();
	float* tmp = sliceprovide->give_me();
	float* dummy;

	gaussian(sigma);
	//	swap_bmphelp();
	dummy = bmp_bits;
	bmp_bits = tmp;
	tmp = dummy;
	swap_bmpwork();
	sobelxy(&sobelx, &sobely);

	dummy = direction_map(sobelx, sobely);
	nonmaximum_supr(dummy);
	swap_bmpwork();
	hysteretic(thresh_low, thresh_high, true, 255);

	sliceprovide->take_back(dummy);
	sliceprovide->take_back(sobelx);
	sliceprovide->take_back(sobely);

	sliceprovide->take_back(bmp_bits);
	bmp_bits = tmp;

	mode1 = dummymode1;
	mode2 = 2;

	return;
}

void bmphandler::hysteretic(float thresh_low, float thresh_high,
		bool connectivity, float set_to)
{
	unsigned char dummymode = mode1;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			//			if(work_bits[i1]!=0){
			if (bmp_bits[i1] >= thresh_high)
			{
				results[i] = set_to;
				s.push_back(i);
			}
			else if (bmp_bits[i1] < thresh_low)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::hysteretic(float thresh_low, float thresh_high,
		bool connectivity, float* mask, float f,
		float set_to)
{
	unsigned char dummymode = mode1;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i1] != 0)
				results[i] = work_bits[i1];
			else
			{
				if (bmp_bits[i1] >= thresh_high)
				{
					results[i] = set_to;
					s.push_back(i);
				}
				else if (bmp_bits[i1] >= thresh_low)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::double_hysteretic(float thresh_low_l, float thresh_low_h,
		float thresh_high_l, float thresh_high_h,
		bool connectivity, float set_to)
{
	unsigned char dummymode = mode1;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (bmp_bits[i1] > thresh_high_h)
				results[i] = 0;
			else if (bmp_bits[i1] > thresh_high_l)
				results[i] = -1;
			else if (bmp_bits[i1] >= thresh_low_h)
			{
				results[i] = set_to;
				s.push_back(i);
			}
			else if (bmp_bits[i1] >= thresh_low_l)
				results[i] = -1;
			else
				results[i] = 0;

			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::double_hysteretic(float thresh_low_l, float thresh_low_h,
		float thresh_high_l, float thresh_high_h,
		bool connectivity, float* mask, float f,
		float set_to)
{
	unsigned char dummymode = mode1;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i1] != 0)
				results[i] = work_bits[i1];
			else
			{
				if (bmp_bits[i1] > thresh_high_h)
					results[i] = 0;
				else if (bmp_bits[i1] > thresh_high_l)
				{
					if (mask[i1] >= f)
					{
						results[i] = set_to;
						s.push_back(i);
					}
					else
						results[i] = -1;
				}
				else if (bmp_bits[i1] >= thresh_low_h)
				{
					results[i] = set_to;
					s.push_back(i);
				}
				else if (bmp_bits[i1] >= thresh_low_l)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::thresholded_growing(Point p, float thresh_low,
		float thresh_high, bool connectivity,
		float set_to)
{
	unsigned char dummymode = mode1;
	unsigned position = pt2coord(p);
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (bmp_bits[i1] > thresh_high || bmp_bits[i1] < thresh_low)
				results[i] = 0;
			else
				results[i] = -1;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	results[s.back()] = set_to;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	for (unsigned int i1 = 0; i1 < area + 2 * width + 2 * height + 4; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::thresholded_growing(Point p, float thresh_low,
		float thresh_high, bool connectivity,
		float set_to, std::vector<Point>* limits1)
{
	unsigned char dummymode = mode1;
	unsigned position = pt2coord(p);
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (bmp_bits[i1] > thresh_high || bmp_bits[i1] < thresh_low)
				results[i] = 0;
			else
				results[i] = -1;
			i++;
			i1++;
		}

		i += 2;
	}

	unsigned w = (unsigned)width + 2;
	for (const auto& it : *limits1)
		results[(it.py + 1) * w + it.px + 1] = 0;

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + w * (height + 1)] = 0;
	for (int j = 0; j <= ((int)w) * (height + 1); j += w)
		results[j] = results[j + width + 1] = 0;

	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	results[s.back()] = set_to;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	for (unsigned int i1 = 0; i1 < area + 2 * width + 2 * height + 4; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::thresholded_growinglimit(Point p, float thresh_low,
		float thresh_high, bool connectivity,
		float set_to)
{
	unsigned char dummymode = mode1;
	unsigned position = pt2coord(p);
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (bmp_bits[i1] > thresh_high || bmp_bits[i1] < thresh_low)
				results[i] = 0;
			else
				results[i] = -1;
			i++;
			i1++;
		}

		i += 2;
	}

	unsigned w = (unsigned)width + 2;
	for (std::vector<std::vector<Point>>::iterator vit = limits.begin();
			 vit != limits.end(); vit++)
	{
		for (std::vector<Point>::iterator it = vit->begin(); it != vit->end(); it++)
			results[(it->py + 1) * w + it->px + 1] = 0;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + w * (height + 1)] = 0;
	for (int j = 0; j <= ((int)w) * (height + 1); j += w)
		results[j] = results[j + width + 1] = 0;

	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	results[s.back()] = set_to;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	for (unsigned int i1 = 0; i1 < area + 2 * width + 2 * height + 4; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::thresholded_growing(Point p, float threshfactor_low,
		float threshfactor_high, bool connectivity,
		float set_to, Pair* tp)
{
	unsigned char dummymode = mode1;
	unsigned position = pt2coord(p);
	float value = bmp_bits[position];

	float thresh_low = threshfactor_low * value;
	float thresh_high = threshfactor_high * value;

	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (bmp_bits[i1] > thresh_high || bmp_bits[i1] < thresh_low)
				results[i] = 0;
			else
				results[i] = -1;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	results[s.back()] = set_to;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	for (unsigned int i1 = 0; i1 < area + 2 * width + 2 * height + 4; i1++)
		if (results[i1] == -1)
			results[i1] = 0;

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	tp->high = thresh_high;
	tp->low = thresh_low;
	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::thresholded_growing(float thresh_low, float thresh_high,
		bool connectivity, float* mask, float f,
		float set_to)
{
	unsigned char dummymode = mode1;
	f = f - f_tol;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i1] > 0)
				results[i] = work_bits[i1];
			else
			{
				if (bmp_bits[i1] <= thresh_high && bmp_bits[i1] >= thresh_low)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	hysteretic_growth(results, &s, width + 2, height + 2, connectivity, set_to);

	//	for(unsigned int i1=0;i1<area+2*width+2*height+4;i1++) if(results[i1]==-1) results[i1]=0;

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			work_bits[i2] = results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	free(results);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::hysteretic_growth(float* results, std::vector<int>* s,
		unsigned short w, unsigned short h,
		bool connectivity, float set_to)
{
	unsigned char dummymode = mode1;
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

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::hysteretic_growth(float* results, std::vector<int>* s,
		unsigned short w, unsigned short h,
		bool connectivity, float set_to, int nr)
{
	std::vector<int> sta;
	std::vector<int>* s1 = &sta;
	unsigned char dummymode = mode1;
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

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::sobel()
{
	unsigned char dummymode = mode1;
	float* tmp = sliceprovide->give_me();
	float* dummy;
	float mask1[4];
	float mask2[4];
	mask1[0] = mask2[0] = 3;
	mask1[1] = mask1[3] = mask2[3] = 1;
	mask1[2] = 2;
	mask2[2] = 0;
	mask2[1] = -1;

	convolute(mask1, 1);
	dummy = tmp;
	tmp = bmp_bits;
	bmp_bits = dummy;
	swap_bmpwork();
	convolute(mask2, 0);
	bmp_abs();
	work_bits = bmp_bits; // work_bits is already saved in dummy
	bmp_bits = tmp;
	convolute(mask1, 0);
	tmp = bmp_bits;
	bmp_bits = work_bits;
	work_bits = sliceprovide->give_me();
	convolute(mask2, 1);
	bmp_abs();
	sliceprovide->take_back(bmp_bits);
	bmp_bits = dummy;
	bmp_sum();
	sliceprovide->take_back(bmp_bits);
	bmp_bits = tmp;

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::sobel_finer()
{
	unsigned char dummymode = mode1;
	float* tmp = sliceprovide->give_me();
	float* dummy;
	float mask1[4];
	float mask2[4];
	mask1[0] = mask2[0] = 3;
	mask1[1] = mask1[3] = mask2[3] = 1;
	mask1[2] = 2;
	mask2[2] = 0;
	mask2[1] = -1;

	convolute(mask1, 1);
	dummy = tmp;
	tmp = bmp_bits;
	bmp_bits = dummy;
	swap_bmpwork();
	convolute(mask2, 0);
	bmp_abs();
	work_bits = bmp_bits; // work_bits is already saved in dummy
	bmp_bits = tmp;
	convolute(mask1, 0);
	tmp = bmp_bits;
	bmp_bits = work_bits;
	work_bits = sliceprovide->give_me();
	convolute(mask2, 1);
	bmp_abs();
	sliceprovide->take_back(bmp_bits);
	bmp_bits = dummy;
	for (unsigned i = 0; i < area; i++)
		work_bits[i] =
				sqrt(work_bits[i] * work_bits[i] + bmp_bits[i] * bmp_bits[i]);
	sliceprovide->take_back(bmp_bits);
	bmp_bits = tmp;

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::median_interquartile(bool median)
{
	unsigned char dummymode = mode1;
	std::vector<float> fvec;

	unsigned k = 0;

	for (unsigned short i = 0; i < height - 2; i++)
	{
		for (unsigned short j = 0; j < width - 2; j++)
		{
			fvec.clear();
			for (unsigned short l = 0; l < 3; l++)
			{
				for (unsigned short m = 0; m < 3; m++)
				{
					fvec.push_back(bmp_bits[k + l + m * width]);
				}
			}

			std::sort(fvec.begin(), fvec.end());
			if (median)
				work_bits[k + 1 + width] = fvec[4];
			else
				work_bits[k + 1 + width] = fvec[7] - fvec[1];
			k++;
		}
		k += 2;
	}

	if (median)
	{
		for (unsigned k = 0; k < width; k++)
			work_bits[k] = bmp_bits[k];
		for (unsigned k = area - width; k < area; k++)
			work_bits[k] = bmp_bits[k];
		for (unsigned k = 0; k < area; k += width)
			work_bits[k] = bmp_bits[k];
		for (unsigned k = width - 1; k < area; k += width)
			work_bits[k] = bmp_bits[k];
	}
	else
	{
		for (unsigned k = 0; k < width; k++)
			work_bits[k] = 0;
		for (unsigned k = area - width; k < area; k++)
			work_bits[k] = 0;
		for (unsigned k = 0; k < area; k += width)
			work_bits[k] = 0;
		for (unsigned k = width - 1; k < area; k += width)
			work_bits[k] = 0;
	}

	mode1 = dummymode;
	mode2 = 1;
}

void bmphandler::median_interquartile(float* median, float* iq)
{
	unsigned char dummymode = mode1;
	std::vector<float> fvec;

	unsigned k = 0;

	for (unsigned short i = 0; i < height - 2; i++)
	{
		for (unsigned short j = 0; j < width - 2; j++)
		{
			fvec.clear();
			for (unsigned short l = 0; l < 3; l++)
			{
				for (unsigned short m = 0; m < 3; m++)
				{
					fvec.push_back(bmp_bits[k + l + m * width]);
				}
			}

			std::sort(fvec.begin(), fvec.end());
			median[k + 1 + width] = fvec[4];
			iq[k + 1 + width] = fvec[7] - fvec[1];
			k++;
		}
		k += 2;
	}

	for (unsigned k = 0; k < width; k++)
		median[k] = bmp_bits[k];
	for (unsigned k = area - width; k < area; k++)
		median[k] = bmp_bits[k];
	for (unsigned k = 0; k < area; k += width)
		median[k] = bmp_bits[k];
	for (unsigned k = width - 1; k < area; k += width)
		median[k] = bmp_bits[k];
	for (unsigned k = 0; k < width; k++)
		iq[k] = 0;
	for (unsigned k = area - width; k < area; k++)
		iq[k] = 0;
	for (unsigned k = 0; k < area; k += width)
		iq[k] = 0;
	for (unsigned k = width - 1; k < area; k += width)
		iq[k] = 0;

	mode1 = dummymode;
	mode2 = 1;
}

void bmphandler::sigmafilter(float sigma, unsigned short nx, unsigned short ny)
{
	unsigned char dummymode = mode1;
	if (nx % 2 == 0)
		nx++;
	if (ny % 2 == 0)
		ny++;

	unsigned short counter;
	float summa;
	float dummy;

	unsigned i = 0;

	for (int j = 0; j <= height - ny; j++)
	{
		for (int k = 0; k <= width - nx; k++)
		{
			summa = 0;
			counter = 0;
			dummy = bmp_bits[i + nx / 2 + (ny / 2) * width];
			for (int l = 0; l < nx; l++)
			{
				for (int o = 0; o < ny; o++)
				{
					if (bmp_bits[i + l + o * width] < dummy + sigma &&
							bmp_bits[i + l + o * width] > dummy - sigma)
					{
						summa += bmp_bits[i + l + o * width];
						counter++;
					}
				}
			}
			work_bits[i + nx / 2 + (ny / 2) * width] = summa / counter;
			i++;
		}
		i = i + nx - 1;
	}

	mode1 = dummymode;
	mode2 = 1;
}

void bmphandler::sobelxy(float** sobelx, float** sobely)
{
	float mask1[4];
	float mask2[4];
	mask1[0] = mask2[0] = 3;
	mask1[1] = mask1[3] = mask2[3] = 1;
	mask1[2] = 2;
	mask2[2] = 0;
	mask2[1] = -1;

	convolute(mask1, 1);
	float* tmp = bmp_bits;
	bmp_bits = work_bits;
	work_bits = *sobelx;
	convolute(mask2, 0);
	work_bits = bmp_bits;
	bmp_bits = tmp;
	convolute(mask1, 0);
	tmp = bmp_bits;
	bmp_bits = work_bits;
	work_bits = *sobely;
	convolute(mask2, 1);
	work_bits = bmp_bits;
	bmp_bits = tmp;

	for (unsigned int i = 0; i < area; i++)
		work_bits[i] = abs((*sobelx)[i]) + abs((*sobely)[i]);
}

void bmphandler::compacthist()
{
	float histmap[256];
	histmap[0] = 0;
	float dummy = 0;

	for (int i = 1; i < 255; i++)
	{
		if (histogram[i] == 0 && histogram[i - 1] != 0 && histogram[i + 1] != 0)
			dummy += 1;
		else
			histmap[i] = dummy;
	}
	histmap[255] = dummy;

	for (unsigned int i = 0; i < area; i++)
		work_bits[i] -= histmap[std::min((int)work_bits[i], 255)];
}

float* bmphandler::find_modal(unsigned int thresh1, float thresh2)
{
	int n = 0;

	std::list<int> threshes;

	unsigned int temp_hist;
	bool pushable = false;

	int lastmin;
	unsigned int lastmax_h, lastmin_h;
	lastmin = 0;
	lastmax_h = lastmin_h = histogram[0];
	for (int i = 1; i < 256; i++)
	{
		temp_hist = histogram[i];

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

void bmphandler::subthreshold(int n1, int n2, unsigned int thresh1,
		float thresh2, float sigma)
{
	unsigned char dummymode = mode1;
	int dx = (width + n1 - 1) / n1;
	int dy = (height + n2 - 1) / n2;
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

			swap_bmpwork();
			make_histogram(p, dx, dy, true);
			swap_bmpwork();
			gaussian_hist(sigma);
			threshold(f_p = find_modal(thresh1, thresh2), p, dx, dy);
		}
	}

	free(f_p);

	mode1 = dummymode;
	mode2 = 2;
}

void bmphandler::erosion1(
		int n, bool connectivity) // true for 8-, false for 4-connectivity
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	float* results = sliceprovide->give_me();
	float* dummy;

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < area; i++)
			results[i] = work_bits[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < width; j++)
			{
				if (work_bits[i1] != work_bits[i1 + width])
				{
					if (work_bits[i1] == 0)
						results[i1 + width] = 0;
					else if (work_bits[i1 + width] == 0)
						results[i1] = 0;
				}

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (work_bits[i1] != work_bits[i1 + 1])
				{
					if (work_bits[i1] == 0)
						results[i1 + 1] = 0;
					else if (work_bits[i1 + 1] == 0)
						results[i1] = 0;
				}

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (height - 1); i++)
			{
				for (unsigned short j = 0; j < (width - 1); j++)
				{
					if (work_bits[i1] != work_bits[i1 + width + 1])
					{
						if (work_bits[i1] == 0)
							results[i1 + 1 + width] = 0;
						else if (work_bits[i1 + 1 + width] == 0)
							results[i1] = 0;
					}
					if (work_bits[i1 + 1] != work_bits[i1 + width])
					{
						if (work_bits[i1 + 1] == 0)
							results[i1 + width] = 0;
						else if (work_bits[i1 + width] == 0)
							results[i1 + 1] = 0;
					}

					i1++;
				}
				i1++;
			}
		}

		dummy = results;
		results = work_bits;
		work_bits = dummy;
	}

	sliceprovide->take_back(results);

	mode1 = dummymode1;
	mode2 = dummymode2;
}

void bmphandler::erosion(
		int n, bool connectivity) // true for 8-, false for 4-connectivity
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;
	float* results = sliceprovide->give_me();
	float* dummy;

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < area; i++)
			results[i] = work_bits[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < width; j++)
			{
				if (work_bits[i1] != work_bits[i1 + width])
					results[i1] = results[i1 + width] = 0;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (work_bits[i1] != work_bits[i1 + 1])
					results[i1] = results[i1 + 1] = 0;

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (height - 1); i++)
			{
				for (unsigned short j = 0; j < (width - 1); j++)
				{
					if (work_bits[i1] != work_bits[i1 + width + 1])
						results[i1] = results[i1 + width + 1] = 0;
					if (work_bits[i1 + 1] != work_bits[i1 + width])
						results[i1 + 1] = results[i1 + width] = 0;

					i1++;
				}
				i1++;
			}
		}

		dummy = results;
		results = work_bits;
		work_bits = dummy;
	}

	sliceprovide->take_back(results);

	mode1 = dummymode1;
	mode2 = dummymode2;
	return;
}

void bmphandler::dilation(
		int n, bool connectivity) // true for 8-, false for 4-connectivity
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	float* results = sliceprovide->give_me();
	float* dummy;

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < area; i++)
			results[i] = work_bits[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < width; j++)
			{
				if (work_bits[i1] != work_bits[i1 + width])
				{
					if (work_bits[i1] == 0)
						results[i1] = results[i1 + width];
					if (work_bits[i1 + width] == 0)
						results[i1 + width] = results[i1];
				}

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (work_bits[i1] != work_bits[i1 + 1])
				{
					if (work_bits[i1] == 0)
						results[i1] = results[i1 + 1];
					if (work_bits[i1 + 1] == 0)
						results[i1 + 1] = results[i1];
				}

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (height - 1); i++)
			{
				for (unsigned short j = 0; j < (width - 1); j++)
				{
					if (work_bits[i1] != work_bits[i1 + width + 1])
					{
						if (work_bits[i1] == 0)
							results[i1] = results[i1 + width + 1];
						if (work_bits[i1 + width + 1] == 0)
							results[i1 + width + 1] = results[i1];
					}
					if (work_bits[i1 + 1] != work_bits[i1 + width])
					{
						if (work_bits[i1 + 1] == 0)
							results[i1 + 1] = results[i1 + width];
						if (work_bits[i1 + width] == 0)
							results[i1 + width] = results[i1 + 1];
					}

					i1++;
				}
				i1++;
			}
		}

		dummy = results;
		results = work_bits;
		work_bits = dummy;
	}

	sliceprovide->take_back(results);

	mode1 = dummymode1;
	mode2 = dummymode2;

	return;
}

void bmphandler::closure(int n, bool connectivity)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	dilation(n, connectivity);
	erosion1(n, connectivity);

	mode1 = dummymode1;
	mode2 = dummymode2;

	return;
}

void bmphandler::open(int n, bool connectivity)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	erosion(n, connectivity);
	dilation(n, connectivity);

	mode1 = dummymode1;
	mode2 = dummymode2;

	return;
}

void bmphandler::mark_border(
		bool connectivity) // true for 8-, false for 4-connectivity
{
	float* results = sliceprovide->give_me();

	for (unsigned int i = 0; i < area; i++)
		results[i] = work_bits[i];

	int i1 = 0;

	for (unsigned short i = 0; i < (height - 1); i++)
	{
		for (unsigned short j = 0; j < width; j++)
		{
			if (work_bits[i1] > work_bits[i1 + width])
				results[i1] = -1;
			else if (work_bits[i1] < work_bits[i1 + width])
				results[i1 + width] = -1;

			i1++;
		}
	}

	i1 = 0;

	for (unsigned short i = 0; i < height; i++)
	{
		for (unsigned short j = 0; j < (width - 1); j++)
		{
			if (work_bits[i1] > work_bits[i1 + 1])
				results[i1] = -1;
			else if (work_bits[i1] < work_bits[i1 + 1])
				results[i1 + 1] = -1;

			i1++;
		}
		i1++;
	}

	if (connectivity)
	{
		i1 = 0;

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (work_bits[i1] > work_bits[i1 + width + 1])
					results[i1] = -1;
				else if (work_bits[i1] < work_bits[i1 + width + 1])
					results[i1 + width + 1] = -1;
				if (work_bits[i1 + 1] > work_bits[i1 + width])
					results[i1 + 1] = -1;
				else if (work_bits[i1 + 1] < work_bits[i1 + width])
					results[i1 + width] = -1;

				i1++;
			}
			i1++;
		}
	}

	sliceprovide->take_back(work_bits);
	work_bits = results;

	return;
}

void bmphandler::zero_crossings(
		bool connectivity) // true for 8-, false for 4-connectivity
{
	float* results = sliceprovide->give_me();

	for (unsigned int i = 0; i < area; i++)
		results[i] = 255;

	int i1 = 0;

	for (unsigned short i = 0; i < (height - 1); i++)
	{
		for (unsigned short j = 0; j < width; j++)
		{
			if (work_bits[i1] * work_bits[i1 + width] <= 0)
			{
				if (work_bits[i1] > 0)
					results[i1 + width] = -1;
				else if (work_bits[i1 + width] > 0)
					results[i1] = -1;
			}
			i1++;
		}
	}

	i1 = 0;

	for (unsigned short i = 0; i < height; i++)
	{
		for (unsigned short j = 0; j < (width - 1); j++)
		{
			if (work_bits[i1] * work_bits[i1 + 1] <= 0)
			{
				if (work_bits[i1] > 0)
					results[i1 + 1] = -1;
				else if (work_bits[i1 + 1] > 0)
					results[i1] = -1;
			}
			i1++;
		}
		i1++;
	}

	if (connectivity)
	{
		i1 = 0;

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (work_bits[i1] * work_bits[i1 + width + 1] <= 0)
				{
					if (work_bits[i1] > 0)
						results[i1 + width + 1] = -1;
					else if (work_bits[i1 + width + 1] > 0)
						results[i1] = -1;
				}
				if (work_bits[i1 + 1] * work_bits[i1 + width] <= 0)
				{
					if (work_bits[i1 + 1] > 0)
						results[i1 + width] = -1;
					else if (work_bits[i1 + width] > 0)
						results[i1 + 1] = -1;
				}
				i1++;
			}
			i1++;
		}
	}

	sliceprovide->take_back(work_bits);
	work_bits = results;

	mode2 = 2;

	return;
}

void bmphandler::zero_crossings(
		float thresh, bool connectivity) // true for 8-, false for 4-connectivity
{
	float* results = sliceprovide->give_me();

	for (unsigned int i = 0; i < area; i++)
		results[i] = 255;

	int i1 = 0;

	for (unsigned short i = 0; i < (height - 1); i++)
	{
		for (unsigned short j = 0; j < width; j++)
		{
			if (work_bits[i1] * work_bits[i1 + width] < 0 &&
					abs(work_bits[i1] - work_bits[i1 + width]) > thresh)
			{
				if (work_bits[i1] > 0)
					results[i1] = -1;
				else
					results[i1 + width] = -1;
			}
			i1++;
		}
	}

	i1 = 0;

	for (unsigned short i = 0; i < height; i++)
	{
		for (unsigned short j = 0; j < (width - 1); j++)
		{
			if (work_bits[i1] * work_bits[i1 + 1] < 0 &&
					abs(work_bits[i1] - work_bits[i1 + width]) > thresh)
			{
				if (work_bits[i1] > 0)
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

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (work_bits[i1] * work_bits[i1 + width + 1] < 0 &&
						abs(work_bits[i1] - work_bits[i1 + width]) > thresh)
				{
					if (work_bits[i1] > 0)
						results[i1] = -1;
					else
						results[i1 + width + 1] = -1;
				}
				if (work_bits[i1 + 1] * work_bits[i1 + width] < 0 &&
						abs(work_bits[i1] - work_bits[i1 + width]) > thresh)
				{
					if (work_bits[i1 + 1] > 0)
						results[i1 + 1] = -1;
					else
						results[i1 + width] = -1;
				}
				i1++;
			}
			i1++;
		}
	}

	sliceprovide->take_back(work_bits);
	work_bits = results;

	mode2 = 2;

	return;
}

void bmphandler::laplacian_zero(float sigma, float thresh, bool connectivity)
{
	unsigned char dummymode = mode1;

	float* tmp1;
	float* tmp2;

	gaussian(sigma);
	tmp1 = bmp_bits;
	bmp_bits = work_bits;
	work_bits = sliceprovide->give_me();
	//	swap_bmpwork();
	sobel();
	tmp2 = work_bits;
	work_bits = sliceprovide->give_me();
	//	swap_workhelp();
	laplacian1();

	//	for(unsigned i=0;i<area;i++) tmp2[i]-=thresh*work_bits[i];

	zero_crossings(connectivity);

	for (unsigned i = 0; i < area; i++)
	{
		if (work_bits[i] == -1 && tmp2[i] < thresh)
			work_bits[i] = 255;
		//		work_bits[i]=tmp2[i];
	}

	sliceprovide->take_back(bmp_bits);
	bmp_bits = tmp1;
	sliceprovide->take_back(tmp2);

	mode1 = dummymode;
	mode2 = 2;

	return;
}

void bmphandler::n_moment(short unsigned n, short unsigned p)
{
	unsigned char dummymode = mode1;

	float* results = sliceprovide->give_me();
	if (n % 2 == 0)
		n++;

	average(n);

	for (unsigned int i = 0; i < area; i++)
		results[i] = 0;

	for (unsigned short i = 0; i <= height - n; i++)
	{
		for (unsigned short j = 0; j <= width - n; j++)
		{
			for (unsigned short k = 0; k < n; k++)
			{
				for (unsigned short l = 0; l < n; l++)
				{
					results[(i + n / 2) * width + j + n / 2] +=
							pow(abs(bmp_bits[(i + k) * width + j + l] -
											work_bits[(i + n / 2) * width + j + n / 2]),
									p);
				}
			}
			results[(i + n / 2) * width + j + n / 2] =
					results[(i + n / 2) * width + j + n / 2] / (n * n);
		}
	}

	sliceprovide->take_back(work_bits);
	work_bits = results;

	mode1 = dummymode;
	mode2 = 1;

	return;
}

void bmphandler::n_moment_sigma(short unsigned n, short unsigned p, float sigma)
{
	unsigned char dummymode = mode1;
	float* results = sliceprovide->give_me();
	if (n % 2 == 0)
		n++;

	sigmafilter(sigma, n, n);

	unsigned count;
	float dummy;

	for (unsigned int i = 0; i < area; i++)
		results[i] = 0;

	for (unsigned short i = 0; i <= height - n; i++)
	{
		for (unsigned short j = 0; j <= width - n; j++)
		{
			count = 0;
			for (unsigned short k = 0; k < n; k++)
			{
				for (unsigned short l = 0; l < n; l++)
				{
					dummy = abs(bmp_bits[(i + k) * width + j + l] -
											work_bits[(i + n / 2) * width + j + n / 2]);
					if (dummy < sigma)
					{
						count++;
						results[(i + n / 2) * width + j + n / 2] +=
								pow(dummy, p);
					}
				}
			}
			results[(i + n / 2) * width + j + n / 2] =
					results[(i + n / 2) * width + j + n / 2] / (count);
		}
	}

	sliceprovide->take_back(work_bits);
	work_bits = results;

	mode1 = dummymode;
	mode2 = 1;

	return;
}

void bmphandler::aniso_diff(float dt, int n, float (*f)(float, float), float k,
		float restraint)
{
	unsigned char dummymode = mode1;
	bmp2work();
	cont_anisodiff(dt, n, f, k, restraint);
	mode1 = dummymode;
	mode2 = 1;
}

void bmphandler::cont_anisodiff(float dt, int n, float (*f)(float, float),
		float k, float restraint)
{
	float* flowx = sliceprovide->give_me(); // only area-height of it is used
	float* flowy = sliceprovide->give_me(); // only area-width of it is used

	float dummy;

	for (int l = 0; l < n; l++)
	{
		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < width - 1; j++)
			{
				dummy =
						(work_bits[i * width + j + 1] - work_bits[i * width + j]);
				flowx[i * (width - 1) + j] = f(dummy, k) * dummy;
			}
		}

		for (unsigned short i = 0; i < height - 1; i++)
		{
			for (unsigned short j = 0; j < width; j++)
			{
				dummy =
						(work_bits[(i + 1) * width + j] - work_bits[i * width + j]);
				flowy[i * width + j] = f(dummy, k) * dummy;
			}
		}

		for (unsigned int i = 0; i < area; i++)
			work_bits[i] += dt * restraint * (bmp_bits[i] - work_bits[i]);

		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < width - 1; j++)
			{
				work_bits[i * width + j] += dt * flowx[i * (width - 1) + j];
				work_bits[i * width + j + 1] -= dt * flowx[i * (width - 1) + j];
			}
		}

		for (unsigned short i = 0; i < height - 1; i++)
		{
			for (unsigned short j = 0; j < width; j++)
			{
				work_bits[i * width + j] += dt * flowy[i * width + j];
				work_bits[(i + 1) * width + j] -= dt * flowy[i * width + j];
			}
		}
	}

	sliceprovide->take_back(flowx);
	sliceprovide->take_back(flowy);

	mode2 = 1;
	return;
}

void bmphandler::to_bmpgrey(float* p_bits)
{
	for (unsigned int i = 0; i < area; i++)
	{
		p_bits[i] = floor(std::min(255.0f, std::max(0.0f, p_bits[i] + 0.5f)));
	}
	return;
}
void bmphandler::bucketsort(std::vector<unsigned int>* sorted, float* p_bits)
{
	for (unsigned int i = 0; i < area; i++)
	{
		sorted[(unsigned char)floor(p_bits[i] + 0.5f)].push_back(i);
	}
}

unsigned* bmphandler::watershed(bool connectivity)
{
	wshedobj.B.clear();
	wshedobj.M.clear();
	wshedobj.marks.clear();

	unsigned p, minbase, minbase_nr;
	unsigned basin_nr = 0;
	unsigned* Y = (unsigned*)malloc(sizeof(unsigned) * area);
	for (unsigned i = 0; i < area; i++)
		Y[i] = unvisited;

	std::vector<unsigned> sorted[256];
	std::vector<unsigned>::iterator it, it1;
	std::vector<unsigned> Bp;

	basin b;
	b.l = 0;
	merge_event m;

	bmp_is_grey = false; //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	if (!bmp_is_grey)
	{
		swap_bmpwork();
		Pair p1;
		get_range(&p1);
		scale_colors(p1);
		swap_bmpwork();
	}

	bucketsort(sorted, bmp_bits);

	for (unsigned i = 0; i < 256; i++)
	{
		for (it = sorted[i].begin(); it != sorted[i].end(); it++)
		{
			p = *it;
			Bp.clear();

			if (p % width != 0 && Y[p - 1] != unvisited)
				Bp.push_back(Y[p - 1]);
			if ((p + 1) % width != 0 && Y[p + 1] != unvisited)
				Bp.push_back(Y[p + 1]);
			if (p >= width && Y[p - width] != unvisited)
				Bp.push_back(Y[p - width]);
			if ((p + width) < area && Y[p + width] != unvisited)
				Bp.push_back(Y[p + width]);
			if (connectivity)
			{
				if (p % width != 0 && p >= width &&
						Y[p - 1 - width] != unvisited)
					Bp.push_back(Y[p - 1 - width]);
				if (p % width != 0 && (p + width) < area &&
						Y[p - 1 + width] != unvisited)
					Bp.push_back(Y[p + width - 1]);
				if ((p + 1) % width != 0 && p >= width &&
						Y[p - width + 1] != unvisited)
					Bp.push_back(Y[p - width + 1]);
				if ((p + 1) % width != 0 && (p + width) < area &&
						Y[p + width + 1] != unvisited)
					Bp.push_back(Y[p + width + 1]);
			}

			if (Bp.empty())
			{
				b.g = i;
				b.r = basin_nr;
				wshedobj.B.push_back(b);
				Y[p] = basin_nr;
				basin_nr++;
			}
			else
			{
				minbase_nr = Bp[0];
				minbase = deepest_con_bas(Bp[0]);
				for (unsigned j = 1; j < Bp.size(); j++)
					if (deepest_con_bas(Bp[j]) < minbase)
						minbase_nr = Bp[j];
				Y[p] = minbase_nr;

				for (it1 = Bp.begin(); it1 != Bp.end(); it1++)
				{
					if (deepest_con_bas(*it1) != deepest_con_bas(minbase_nr))
					{
						wshedobj.B[deepest_con_bas(*it1)].r =
								deepest_con_bas(minbase_nr);
						m.k = *it1;
						m.a = minbase_nr;
						m.g = i;
						wshedobj.M.push_back(m);
					}
				}
			}
		}
	}

	return Y;
}

unsigned* bmphandler::watershed_sobel(bool connectivity)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	unsigned i = pushstack_work();
	sobel();
	swap_bmpwork();
	unsigned* usp = watershed(connectivity);
	swap_bmpwork();
	getstack_work(i);
	removestack(i);
	//	popstack_work();

	mode1 = dummymode1;
	mode2 = dummymode2;

	return usp;
}

void bmphandler::construct_regions(unsigned h, unsigned* wshed)
{
	unsigned char dummymode1 = mode1;
	//unsigned char dummymode2=mode2;

	for (unsigned i = 0; i < wshedobj.B.size(); i++)
	{
		wshedobj.B[i].l = 0;
		wshedobj.B[i].r = i;
	}

	//	unsigned i1=1;
	//	for(std::vector<mark>::iterator it=marks.begin();it!=marks.end();it++) {
	//		set_marker(*it,i1,wshed);
	set_marker(wshed);
	//		i1++;
	//	}

	//	cout << wshed[126167]<<" "<<wshedobj.B[wshed[126167]].l<<endl;

	for (unsigned i = 0; i < wshedobj.M.size(); i++)
	{
		cond_merge(i, h);
	}

	labels2work(wshed, (unsigned)marks.size());

	mode1 = dummymode1;
	mode2 = 2;

	return;
}

void bmphandler::set_marker(unsigned* wshed)
{
	for (std::vector<Mark>::iterator it = marks.begin(); it != marks.end(); it++)
	{
		wshedobj.B[wshed[pt2coord((*it).p)]].l = (*it).mark;
	}
}

void bmphandler::add_mark(Point p, unsigned label, std::string str)
{
	Mark m;
	m.p.px = p.px;
	m.p.py = p.py;
	m.mark = label;
	m.name = str;

	marks.push_back(m);
}

bool bmphandler::remove_mark(Point p, unsigned radius)
{
	radius = radius * radius;

	std::vector<Mark>::iterator it = marks.begin();
	while (it != marks.end() &&
				 (unsigned int)((it->p.px - p.px) * (it->p.px - p.px) +
												(it->p.py - p.py) * (it->p.py - p.py)) > radius)
		it++;

	if (it != marks.end())
	{
		marks.erase(it);
		return true;
	}
	else
		return false;
}

void bmphandler::clear_marks()
{
	marks.clear();
}

unsigned bmphandler::deepest_con_bas(unsigned k)
{
	if (wshedobj.B[k].r != k)
		return deepest_con_bas(wshedobj.B[k].r);
	else
		return k;
}

void bmphandler::cond_merge(unsigned m, unsigned h)
{
	unsigned k, a;
	k = deepest_con_bas(wshedobj.M[m].k);
	a = deepest_con_bas(wshedobj.M[m].a);
	if (h >= (wshedobj.M[m].g - wshedobj.B[k].g))
	{
		if (wshedobj.B[a].l == 0 || wshedobj.B[k].l == 0 ||
				wshedobj.B[a].l == wshedobj.B[k].l)
		{
			wshedobj.B[k].r = a;
		}
		if (wshedobj.B[a].l == 0)
		{
			wshedobj.B[a].l = wshedobj.B[k].l;
		}
	}
}

void bmphandler::wshed2work(unsigned* Y)
{
	float d = 255.0f / wshedobj.B.size();
	for (unsigned i = 0; i < area; i++)
		work_bits[i] = Y[i] * d;

	mode2 = 2;
}

void bmphandler::labels2work(unsigned* Y, unsigned lnr)
{
	UNREFERENCED_PARAMETER(lnr);

	unsigned int maxim = 1;
	for (std::vector<Mark>::iterator it = marks.begin(); it != marks.end(); it++)
	{
		maxim = std::max(maxim, it->mark);
	}

	float d = 255.0f / maxim;
	for (unsigned i = 0; i < area; i++)
		work_bits[i] = d * label_lookup(i, Y);

	mode2 = 2;
}

unsigned bmphandler::label_lookup(unsigned i, unsigned* wshed)
{
	unsigned k = wshed[i];
	if (wshedobj.B[k].l == 0 && wshedobj.B[k].r != k)
		wshedobj.B[k].l = wshedobj.B[deepest_con_bas(k)].l;
	return wshedobj.B[k].l;
}

void bmphandler::load_line(std::vector<Point>* vec_pt)
{
	contour.clear();
	contour.add_points(vec_pt);
}

void bmphandler::presimplify_line(float d)
{
	contour.presimplify(d, true);
}

void bmphandler::dougpeuck_line(float epsilon)
{
	contour.doug_peuck(epsilon, true);
}

void bmphandler::plot_line() // very temporary solution...
{
	std::vector<Point> p_vec;

	//	contour.doug_peuck(3,true);

	contour.return_contour(&p_vec);
	contour.print_contour();

	unsigned dx, dy;
	Point P;

	unsigned n = (unsigned)contour.return_n();

	for (unsigned i = 0; i < n - 1; i++)
	{
		dx = std::max(p_vec[i].px, p_vec[i + 1].px) -
				 std::min(p_vec[i].px, p_vec[i + 1].px);
		dy = std::max(p_vec[i].py, p_vec[i + 1].py) -
				 std::min(p_vec[i].py, p_vec[i + 1].py);
		P.px = p_vec[i].px;
		P.py = p_vec[i].py;
		work_bits[pt2coord(P)] = 0;
		if (dx > dy)
		{
			for (unsigned j = 1; j < dx; j++)
			{
				P.px = short(0.5 +
										 ((float)p_vec[i + 1].px - (float)p_vec[i].px) * j /
												 dx +
										 (float)p_vec[i].px);
				P.py = short(0.5 +
										 ((float)p_vec[i + 1].py - (float)p_vec[i].py) * j /
												 dx +
										 (float)p_vec[i].py);
				work_bits[pt2coord(P)] = 255;
			}
		}
		else
		{
			for (unsigned j = 1; j < dy; j++)
			{
				P.px = short(0.5 +
										 ((float)p_vec[i + 1].px - (float)p_vec[i].px) * j /
												 dy +
										 (float)p_vec[i].px);
				P.py = short(0.5 +
										 ((float)p_vec[i + 1].py - (float)p_vec[i].py) * j /
												 dy +
										 (float)p_vec[i].py);
				work_bits[pt2coord(P)] = 255;
			}
		}
	}

	dx = std::max(p_vec[n - 1].px, p_vec[0].px) - std::min(p_vec[n - 1].px, p_vec[0].px);
	dy = std::max(p_vec[n - 1].py, p_vec[0].py) - std::min(p_vec[n - 1].py, p_vec[0].py);
	P.px = p_vec[n - 1].px;
	P.py = p_vec[n - 1].py;
	work_bits[pt2coord(P)] = 0;
	if (dx > dy)
	{
		for (unsigned j = 1; j < dx; j++)
		{
			P.px = short(
					0.5 + ((float)p_vec[0].px - (float)p_vec[n - 1].px) * j / dx +
					(float)p_vec[n - 1].px);
			P.py = short(
					0.5 + ((float)p_vec[0].py - (float)p_vec[n - 1].py) * j / dx +
					(float)p_vec[n - 1].py);
			work_bits[pt2coord(P)] = 255;
		}
	}
	else
	{
		for (unsigned j = 1; j < dy; j++)
		{
			P.px = short(
					0.5 + ((float)p_vec[0].px - (float)p_vec[n - 1].px) * j / dy +
					(float)p_vec[n - 1].px);
			P.py = short(
					0.5 + ((float)p_vec[0].py - (float)p_vec[n - 1].py) * j / dy +
					(float)p_vec[n - 1].py);
			work_bits[pt2coord(P)] = 255;
		}
	}

	return;
}

void bmphandler::connected_components(bool connectivity)
{
	unsigned char dummymode = mode1;

	float* temp = (float*)malloc(sizeof(float) * (area + width + height + 1));
	std::vector<float> maps;
	float newest = 0.1f;

	for (unsigned i = 0; i < area; i++)
		temp[i + i / width + width + 2] = bmp_bits[i];
	for (unsigned i = width + 1; i <= area + height; i += width + 1)
		temp[i] = -1e10f;
	for (unsigned i = 0; i <= width; i++)
		temp[i] = -2e10f;

	unsigned i1 = width + 2;
	unsigned i2 = 0;

	for (short j = 0; j < height; j++)
	{
		for (short k = 0; k < width; k++)
		{
			if (temp[i1] == temp[i1 - 1])
			{
				work_bits[i2] = base_connection(work_bits[i2 - 1], &maps);
				if (temp[i1] == temp[i1 - width - 1])
				{
					maps[(int)base_connection(work_bits[i2 - width], &maps)] =
							work_bits[i2];
				}
			}
			else
			{
				if (temp[i1] == temp[i1 - width - 1])
					work_bits[i2] =
							base_connection(work_bits[i2 - width], &maps);
				else if (connectivity && temp[i1] == temp[i1 - width - 2])
					work_bits[i2] =
							base_connection(work_bits[i2 - width - 1], &maps);
				else
				{
					maps.push_back(newest);
					work_bits[i2] = newest;
					newest++;
				}
			}

			if (connectivity && temp[i1 - 1] == temp[i1 - width - 1])
				maps[(int)base_connection(work_bits[i2 - 1], &maps)] =
						base_connection(work_bits[i2 - width], &maps);

			i1++;
			i2++;
		}
		i1++;
	}

	for (unsigned i = 0; i < area; i++)
		work_bits[i] = base_connection(work_bits[i], &maps);

	newest = 0.0f;
	for (i1 = 0; i1 < (unsigned)maps.size(); i1++)
		if ((unsigned)maps[i1] == i1)
		{
			maps[i1] = newest;
			newest++;
		}

	for (unsigned i = 0; i < area; i++)
		work_bits[i] = maps[(int)work_bits[i]];

	free(temp);

	mode1 = dummymode;
	mode2 = 2;

	return;
}

void bmphandler::connected_components(bool connectivity, std::set<float>& components)
{
	unsigned char dummymode = mode1;

	float* temp = (float*)malloc(sizeof(float) * (area + width + height + 1));
	std::vector<float> maps;
	float newest = 0.1f;

	for (unsigned i = 0; i < area; i++)
		temp[i + i / width + width + 2] = bmp_bits[i];
	for (unsigned i = width + 1; i <= area + height; i += width + 1)
		temp[i] = -1e10f;
	for (unsigned i = 0; i <= width; i++)
		temp[i] = -2e10f;

	unsigned i1 = width + 2;
	unsigned i2 = 0;

	for (short j = 0; j < height; j++)
	{
		for (short k = 0; k < width; k++)
		{
			if (temp[i1] == temp[i1 - 1])
			{
				work_bits[i2] = base_connection(work_bits[i2 - 1], &maps);
				if (temp[i1] == temp[i1 - width - 1])
				{
					maps[(int)base_connection(work_bits[i2 - width], &maps)] =
							work_bits[i2];
				}
			}
			else
			{
				if (temp[i1] == temp[i1 - width - 1])
					work_bits[i2] =
							base_connection(work_bits[i2 - width], &maps);
				else if (connectivity && temp[i1] == temp[i1 - width - 2])
					work_bits[i2] =
							base_connection(work_bits[i2 - width - 1], &maps);
				else
				{
					maps.push_back(newest);
					work_bits[i2] = newest;
					newest++;
				}
			}

			if (connectivity && temp[i1 - 1] == temp[i1 - width - 1])
				maps[(int)base_connection(work_bits[i2 - 1], &maps)] =
						base_connection(work_bits[i2 - width], &maps);

			i1++;
			i2++;
		}
		i1++;
	}

	for (unsigned i = 0; i < area; i++)
		work_bits[i] = base_connection(work_bits[i], &maps);

	newest = 0.0f;
	for (i1 = 0; i1 < (unsigned)maps.size(); i1++)
		if ((unsigned)maps[i1] == i1)
		{
			maps[i1] = newest;
			newest++;
		}

	for (unsigned i = 0; i < area; i++)
	{
		work_bits[i] = maps[(int)work_bits[i]];
		components.insert(work_bits[i]);
	}

	free(temp);

	mode1 = dummymode;
	mode2 = 2;
}

float bmphandler::base_connection(float c, std::vector<float>* maps)
{
	if (c == (*maps)[(int)c])
		return c;
	else
		return (*maps)[(int)c] = base_connection((*maps)[(int)c], maps);
}

void bmphandler::fill_gaps(short unsigned n, bool connectivity)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	float* tmp1;
	float* tmp2 = sliceprovide->give_me();
	float* dummy;
	bool* dummybool;
	float* bmpstore;
	bool* isinterface = (bool*)malloc(sizeof(bool) * area);
	bool* isinterfaceold = (bool*)malloc(sizeof(bool) * area);

	for (unsigned int i = 0; i < area; i++)
	{
		tmp2[i] = work_bits[i];
		isinterfaceold[i] = false;
	}
	closure(int(ceil(float(n) / 2)), connectivity);
	tmp1 = work_bits;
	bmpstore = bmp_bits;
	bmp_bits = tmp2;
	tmp2 = sliceprovide->give_me();
	work_bits = sliceprovide->give_me();
	connected_components(connectivity);

	float f = -1;
	for (unsigned int i = 0; i < area; i++)
	{
		if (bmp_bits[i] == 0)
			work_bits[i] = -1;
	}

	for (int l = 0; l < (int)std::ceil(float(n + 1) / 2); l++)
	{
		for (unsigned int i = 0; i < area; i++)
		{
			tmp2[i] = work_bits[i];
			isinterface[i] = isinterfaceold[i];
		}

		int i1 = 0;

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < width; j++)
			{
				if (work_bits[i1] != work_bits[i1 + width])
				{
					if (work_bits[i1] == f)
					{
						if (tmp2[i1] == f)
							tmp2[i1] = tmp2[i1 + width];
						else if (tmp2[i1] != tmp2[i1 + width])
							isinterface[i1] = true;
					}
					else if (work_bits[i1 + width] == f)
					{
						if (tmp2[i1 + width] == f)
							tmp2[i1 + width] = tmp2[i1];
						else if (tmp2[i1 + width] != tmp2[i1])
							isinterface[i1 + width] = true;
					}
					else
					{
						if (bmp_bits[i1] == 0)
							isinterface[i1] = true;
						if (bmp_bits[i1 + width] == 0)
							isinterface[i1 + width] = true;
					}
				}
				if (isinterfaceold[i1] && (bmp_bits[i1 + width] == 0))
					isinterface[i1 + width] = true;
				else if (isinterfaceold[i1 + width] && (bmp_bits[i1] == 0))
					isinterface[i1] = true;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (work_bits[i1] != work_bits[i1 + 1])
				{
					if (work_bits[i1] == f)
					{
						if (tmp2[i1] == f)
							tmp2[i1] = tmp2[i1 + 1];
						else if (tmp2[i1] != tmp2[i1 + 1])
							isinterface[i1] = true;
					}
					else if (work_bits[i1 + 1] == f)
					{
						if (tmp2[i1 + 1] == f)
							tmp2[i1 + 1] = tmp2[i1];
						else if (tmp2[i1 + 1] != tmp2[i1])
							isinterface[i1 + 1] = true;
					}
					else
					{
						if (bmp_bits[i1] == 0)
							isinterface[i1] = true;
						if (bmp_bits[i1 + 1] == 0)
							isinterface[i1 + 1] = true;
					}
				}
				if (isinterfaceold[i1] && (bmp_bits[i1 + 1] == 0))
					isinterface[i1 + 1] = true;
				else if (isinterfaceold[i1 + 1] && (bmp_bits[i1] == 0))
					isinterface[i1] = true;

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (height - 1); i++)
			{
				for (unsigned short j = 0; j < (width - 1); j++)
				{
					if (work_bits[i1] != work_bits[i1 + width + 1])
					{
						if (work_bits[i1] == f)
						{
							if (tmp2[i1] == f)
								tmp2[i1] = tmp2[i1 + width + 1];
							else if (tmp2[i1] != tmp2[i1 + width + 1])
								isinterface[i1] = true;
						}
						else if (work_bits[i1 + width + 1] == f)
						{
							if (tmp2[i1 + width + 1] == f)
								tmp2[i1 + width + 1] = tmp2[i1];
							else if (tmp2[i1 + width + 1] != tmp2[i1])
								isinterface[i1 + width + 1] = true;
						}
						else
						{
							if (bmp_bits[i1] == 0)
								isinterface[i1] = true;
							if (bmp_bits[i1 + width + 1] == 0)
								isinterface[i1 + width + 1] = true;
						}
					}
					if (isinterfaceold[i1] && (bmp_bits[i1 + width + 1] == 0))
						isinterface[i1 + width + 1] = true;
					else if (isinterfaceold[i1 + width + 1] &&
									 (bmp_bits[i1] == 0))
						isinterface[i1] = true;

					if (work_bits[i1 + 1] != work_bits[i1 + width])
					{
						if (work_bits[i1 + 1] == f)
						{
							if (tmp2[i1 + 1] == f)
								tmp2[i1 + 1] = tmp2[i1 + width];
							else if (tmp2[i1 + 1] != tmp2[i1 + width])
								isinterface[i1 + 1] = true;
						}
						else if (work_bits[i1 + width] == f)
						{
							if (tmp2[i1 + width] == f)
								tmp2[i1 + width] = tmp2[i1 + 1];
							else if (tmp2[i1 + width] != tmp2[i1 + 1])
								isinterface[i1 + width] = true;
						}
						else
						{
							if (bmp_bits[i1 + 1] == 0)
								isinterface[i1 + 1] = true;
							if (bmp_bits[i1 + width] == 0)
								isinterface[i1 + width] = true;
						}
					}
					if (isinterfaceold[i1 + 1] && (bmp_bits[i1 + width] == 0))
						isinterface[i1 + width] = true;
					else if (isinterfaceold[i1 + width] &&
									 (bmp_bits[i1 + 1] == 0))
						isinterface[i1 + 1] = true;

					i1++;
				}
				i1++;
			}
		}

		dummy = tmp2;
		tmp2 = work_bits;
		work_bits = dummy;

		dummybool = isinterface;
		isinterface = isinterfaceold;
		isinterfaceold = dummybool;
	}

	for (int l = 0; l < (int)std::floor(float(n - 1) / 2); l++)
	{
		for (unsigned int i = 0; i < area; i++)
		{
			isinterface[i] = isinterfaceold[i];
		}

		int i1 = 0;

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < width; j++)
			{
				if (isinterfaceold[i1] && (bmp_bits[i1 + width] == 0))
					isinterface[i1 + width] = true;
				else if (isinterfaceold[i1 + width] && (bmp_bits[i1] == 0))
					isinterface[i1] = true;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (isinterfaceold[i1] && (bmp_bits[i1 + 1] == 0))
					isinterface[i1 + 1] = true;
				else if (isinterfaceold[i1 + 1] && (bmp_bits[i1] == 0))
					isinterface[i1] = true;

				i1++;
			}
			i1++;
		}

		if (connectivity)
		{
			i1 = 0;
			for (unsigned short i = 0; i < (height - 1); i++)
			{
				for (unsigned short j = 0; j < (width - 1); j++)
				{
					if (isinterfaceold[i1] && (bmp_bits[i1 + width + 1] == 0))
						isinterface[i1 + width + 1] = true;
					else if (isinterfaceold[i1 + width + 1] &&
									 (bmp_bits[i1] == 0))
						isinterface[i1] = true;

					if (isinterfaceold[i1 + 1] && (bmp_bits[i1 + width] == 0))
						isinterface[i1 + width] = true;
					else if (isinterfaceold[i1 + width] &&
									 (bmp_bits[i1 + 1] == 0))
						isinterface[i1 + 1] = true;

					i1++;
				}
				i1++;
			}
		}

		dummy = tmp2;
		tmp2 = work_bits;
		work_bits = dummy;

		dummybool = isinterface;
		isinterface = isinterfaceold;
		isinterfaceold = dummybool;
	}

	for (unsigned int i = 0; i < area; i++)
	{
		if (bmp_bits[i] == 0 && isinterfaceold[i])
			bmp_bits[i] = tmp1[i];
		//		if(isinterfaceold[i])
		//			bmp_bits[i]=255;
		//		else bmp_bits[i]=0;
		//		bmp_bits[i]=tmp2[i];
	}

	sliceprovide->take_back(tmp2);
	sliceprovide->take_back(work_bits);
	sliceprovide->take_back(tmp1);
	work_bits = bmp_bits;
	bmp_bits = bmpstore;
	free(isinterface);
	free(isinterfaceold);

	mode1 = dummymode1;
	mode2 = dummymode2;

	return;
}

void bmphandler::fill_gapstissue(tissuelayers_size_t idx, short unsigned n,
		bool connectivity)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	float* workstore = work_bits;
	work_bits = sliceprovide->give_me();
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		work_bits[i] = (float)tissues[i];
	}

	fill_gaps(n, connectivity);

	for (unsigned int i = 0; i < area; i++)
	{
		tissues[i] = (tissues_size_t)work_bits[i];
	}
	sliceprovide->take_back(work_bits);
	work_bits = workstore;

	mode1 = dummymode1;
	mode2 = dummymode2;
}

void bmphandler::get_tissuecontours(tissuelayers_size_t idx, tissues_size_t f,
		std::vector<std::vector<Point>>* outer_line,
		std::vector<std::vector<Point>>* inner_line,
		int minsize)
{
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	tissues_size_t* tmp_bits = (tissues_size_t*)malloc(
			sizeof(tissues_size_t) * (width + 2) * (height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (width + 2) * (height + 2));
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i++)
		visited[i] = false;

	unsigned pos = width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner; //1 for outer, 7 for inner border
	short direction,
			directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, width + 3, width + 2, width + 1,
			-1, -width - 3, -width - 2, -width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned i = 0; i < height; i++)
	{
		for (unsigned j = 0; j < width; j++)
		{
			tmp_bits[pos] = tissues[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(width + 2); i++)
		tmp_bits[i] = TISSUES_SIZE_MAX;
	for (unsigned i = unsigned(width + 2) * (height + 1);
			 i < unsigned(width + 2) * (height + 2); i++)
		tmp_bits[i] = TISSUES_SIZE_MAX;
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = TISSUES_SIZE_MAX;
	for (unsigned i = width + 1; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = TISSUES_SIZE_MAX;

	pos = width + 2;
	while (pos < unsigned(width + 2) * (height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(width + 2) * (height + 1))
			pos++;

		if (pos < unsigned(width + 2) * (height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (width + 2) - 1);
			p.py = short(pos / (width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + width + 3] != f &&
					tmp_bits[pos + width + 2] != f &&
					tmp_bits[pos + width + 1] != f &&
					tmp_bits[pos - width - 1] != f &&
					tmp_bits[pos - width - 2] != f &&
					tmp_bits[pos - width - 3] != f)
			{
				vec_pt.push_back(p);
				if (1 >= minsize)
					(*outer_line).push_back(vec_pt);
				visited[pos] = true;
			}
			else
			{
				if (tmp_bits[pos - width - 3] == f)
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
					p.px = short(pos1 % (width + 2) - 1);
					p.py = short(pos1 / (width + 2) - 1);
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
							bmp_bits[it1->px + (unsigned)width * it1->py] = 255;
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
							bmp_bits[it1->px + (unsigned)width * it1->py] = 255;
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
	return;
}

void bmphandler::get_tissuecontours2_xmirrored(
		tissuelayers_size_t idx, tissues_size_t f,
		std::vector<std::vector<Point>>* outer_line, std::vector<std::vector<Point>>* inner_line,
		int minsize)
{
	int movpos[4];
	movpos[0] = 1;
	movpos[1] = width + 2;
	movpos[2] = -1;
	movpos[3] = -int(width + 2);

	int pos = width + 3;
	int pos1 = 0;
	int pos2;

	unsigned setto = TISSUES_SIZE_MAX + 1;
	unsigned* tmp_bits = (unsigned*)malloc(sizeof(unsigned) * (width + 2) * (height + 2));
	unsigned char* nrlines = (unsigned char*)malloc(sizeof(unsigned char) * (width + 2) * (height + 2));

	unsigned f1 = (unsigned)f;

	std::vector<Point> vec_pt;
	float vol;

	tissues_size_t* tissues = tissuelayers[idx];
	for (short unsigned i = 0; i < height; i++)
	{
		for (short unsigned j = 0; j < width; j++)
		{
			tmp_bits[pos] = (unsigned)tissues[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(width + 2); i++)
		tmp_bits[i] = setto;
	for (unsigned i = unsigned(width + 2) * (height + 1); i < unsigned(width + 2) * (height + 2); i++)
		tmp_bits[i] = setto;
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i += (width + 2))
		tmp_bits[i] = setto;
	for (unsigned i = width + 1; i < unsigned(width + 2) * (height + 2); i += (width + 2))
		tmp_bits[i] = setto;
	tmp_bits[1] = tmp_bits[width] = tmp_bits[unsigned(width + 2) * (height + 1) + 1] = tmp_bits[unsigned(width + 2) * (height + 2) - 2] = setto + 1;
	tmp_bits[width + 2] = tmp_bits[2 * width + 3] = tmp_bits[unsigned(width + 2) * (height)] = tmp_bits[unsigned(width + 2) * (height + 1) - 1] = setto + 2;

	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i++)
		nrlines[i] = 0;

	pos = 0;
	for (unsigned short i = 0; i < height + 1; i++)
	{
		for (unsigned short j = 0; j < width + 1; j++)
		{
			if ((tmp_bits[pos] != tmp_bits[pos + 1]) && (tmp_bits[pos] == f1 || tmp_bits[pos + 1] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos] != tmp_bits[pos + width + 2]) && (tmp_bits[pos] == f1 || tmp_bits[pos + width + 2] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos + width + 2] != tmp_bits[pos + width + 3]) && (tmp_bits[pos + width + 2] == f1 || tmp_bits[pos + width + 3] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos + 1] != tmp_bits[pos + width + 3]) && (tmp_bits[pos + 1] == f1 || tmp_bits[pos + width + 3] == f1))
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

	while (pos < int(width + 2) * (height + 2))
	{
		while (pos < int(width + 2) * (height + 2) && nrlines[pos] == 0)
			pos++;

		if (pos != int(width + 2) * (height + 2))
		{
			vec_pt.clear();
			inner = (tmp_bits[pos + width + 3] != f1);
			pos2 = pos;
			direction = 3;
			p.px = p2.px = 2 * (width + 1 - (pos % (width + 2)));
			p.py = p2.py = 2 * (pos / (width + 2)) + 1;

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
				if (tmp_bits[pos + width + 3] == f1)
					casenr += 4;
				if (tmp_bits[pos + width + 2] == f1)
					casenr += 8;
				nrlines[pos] -= 2;
				switch (casenr)
				{
				case 1:
					if (direction == 0)
					{
						p.px--;
						if (tmp_bits[pos + 1] != tmp_bits[pos + width + 2])
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
						if (tmp_bits[pos + 1] != tmp_bits[pos + width + 2])
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
						if (tmp_bits[pos] != tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos] != tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos + 1] != tmp_bits[pos + width + 2])
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
						if (tmp_bits[pos + 1] != tmp_bits[pos + width + 2])
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
					if (tmp_bits[pos + width + 2] != tmp_bits[pos + 1] ||
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
						if (tmp_bits[pos] != tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos] != tmp_bits[pos + width + 3])
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
					if (tmp_bits[pos] != tmp_bits[pos + width + 3] ||
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

void bmphandler::get_tissuecontours2_xmirrored(
		tissuelayers_size_t idx, tissues_size_t f,
		std::vector<std::vector<Point>>* outer_line,
		std::vector<std::vector<Point>>* inner_line,
		int minsize, float disttol)
{
	int movpos[4];
	movpos[0] = 1;
	movpos[1] = width + 2;
	movpos[2] = -1;
	movpos[3] = -int(width + 2);

	int pos = width + 3;
	int pos1 = 0;
	int pos2;

	unsigned setto = TISSUES_SIZE_MAX + 1;
	unsigned* tmp_bits = (unsigned*)malloc(sizeof(unsigned) * (width + 2) * (height + 2));
	unsigned f1 = (unsigned)f;
	unsigned char* nrlines = (unsigned char*)malloc(sizeof(unsigned char) * (width + 2) * (height + 2));

	std::vector<Point> vec_pt;
	std::vector<unsigned> vec_meetings;
	float vol;

	tissues_size_t* tissues = tissuelayers[idx];
	for (short unsigned i = 0; i < height; i++)
	{
		for (short unsigned j = 0; j < width; j++)
		{
			tmp_bits[pos] = (unsigned)tissues[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(width + 2); i++)
		tmp_bits[i] = setto;
	for (unsigned i = unsigned(width + 2) * (height + 1); i < unsigned(width + 2) * (height + 2); i++)
		tmp_bits[i] = setto;
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i += (width + 2))
		tmp_bits[i] = setto;
	for (unsigned i = width + 1; i < unsigned(width + 2) * (height + 2); i += (width + 2))
		tmp_bits[i] = setto;
	tmp_bits[1] = tmp_bits[width] = tmp_bits[unsigned(width + 2) * (height + 1) + 1] = tmp_bits[unsigned(width + 2) * (height + 2) - 2] = setto + 1;
	tmp_bits[width + 2] = tmp_bits[2 * width + 3] = tmp_bits[unsigned(width + 2) * (height)] = tmp_bits[unsigned(width + 2) * (height + 1) - 1] = setto + 2;

	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i++)
		nrlines[i] = 0;

	pos = 0;
	for (unsigned short i = 0; i < height + 1; i++)
	{
		for (unsigned short j = 0; j < width + 1; j++)
		{
			if ((tmp_bits[pos] != tmp_bits[pos + 1]) && (tmp_bits[pos] == f1 || tmp_bits[pos + 1] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos] != tmp_bits[pos + width + 2]) && (tmp_bits[pos] == f1 || tmp_bits[pos + width + 2] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos + width + 2] != tmp_bits[pos + width + 3]) && (tmp_bits[pos + width + 2] == f1 || tmp_bits[pos + width + 3] == f1))
				nrlines[pos]++;
			if ((tmp_bits[pos + 1] != tmp_bits[pos + width + 3]) && (tmp_bits[pos + 1] == f1 || tmp_bits[pos + width + 3] == f1))
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

	while (pos < int(width + 2) * (height + 2))
	{
		while (pos < int(width + 2) * (height + 2) && nrlines[pos] == 0)
			pos++;

		if (pos != int(width + 2) * (height + 2))
		{
			vec_pt.clear();
			vec_meetings.clear();
			inner = (tmp_bits[pos + width + 3] != f1);
			pos2 = pos;
			direction = 3;
			p.px = p2.px = 2 * (width + 1 - (pos % (width + 2)));
			p.py = p2.py = 2 * (pos / (width + 2)) + 1;

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
				if (tmp_bits[pos + width + 3] == f1)
					casenr += 4;
				if (tmp_bits[pos + width + 2] == f1)
					casenr += 8;
				nrlines[pos] -= 2;
				switch (casenr)
				{
				case 1:
					if (direction == 0)
					{
						p.px--;
						if (tmp_bits[pos + 1] != tmp_bits[pos + width + 2])
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
						if (tmp_bits[pos + 1] != tmp_bits[pos + width + 2])
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
						if (tmp_bits[pos] != tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos] != tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos + width + 2] !=
								tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos + width + 2] !=
								tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos + 1] != tmp_bits[pos + width + 2])
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
						if (tmp_bits[pos + 1] != tmp_bits[pos + width + 2])
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
					if (tmp_bits[pos + width + 2] != tmp_bits[pos + 1] ||
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
						if (tmp_bits[pos + width + 2] != tmp_bits[pos])
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
						if (tmp_bits[pos + width + 2] != tmp_bits[pos])
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
						if (tmp_bits[pos] != tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos] != tmp_bits[pos + width + 3])
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
						if (tmp_bits[pos + width + 3] != tmp_bits[pos + 1])
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
						if (tmp_bits[pos + width + 3] != tmp_bits[pos + 1])
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
					if (tmp_bits[pos] != tmp_bits[pos + width + 3] ||
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
				cc2.doug_peuck(disttol * 2, &vec_pt, &vec_meetings, &vec_simp);
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

void bmphandler::get_contours(float f, std::vector<std::vector<Point>>* outer_line,
		std::vector<std::vector<Point>>* inner_line, int minsize)
{
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	float* tmp_bits =
			(float*)malloc(sizeof(float) * (width + 2) * (height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (width + 2) * (height + 2));
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i++)
		visited[i] = false;

	unsigned pos = width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner; //1 for outer, 7 for inner border
	short direction,
			directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, width + 3, width + 2, width + 1,
			-1, -width - 3, -width - 2, -width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	for (unsigned i = 0; i < height; i++)
	{
		for (unsigned j = 0; j < width; j++)
		{
			tmp_bits[pos] = work_bits[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(width + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = unsigned(width + 2) * (height + 1);
			 i < unsigned(width + 2) * (height + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvisited;
	for (unsigned i = width + 1; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvisited;

	pos = width + 2;
	while (pos < unsigned(width + 2) * (height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(width + 2) * (height + 1))
			pos++;

		if (pos < unsigned(width + 2) * (height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (width + 2) - 1);
			p.py = short(pos / (width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + width + 3] != f &&
					tmp_bits[pos + width + 2] != f &&
					tmp_bits[pos + width + 1] != f &&
					tmp_bits[pos - width - 1] != f &&
					tmp_bits[pos - width - 2] != f &&
					tmp_bits[pos - width - 3] != f)
			{
				vec_pt.push_back(p);
				if (1 >= minsize)
					(*outer_line).push_back(vec_pt);
				visited[pos] = true;
			}
			else
			{
				if (tmp_bits[pos - width - 3] == f)
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
					p.px = short(pos1 % (width + 2) - 1);
					p.py = short(pos1 / (width + 2) - 1);
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
							bmp_bits[it1->px + (unsigned)width * it1->py] = 255;
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
							bmp_bits[it1->px + (unsigned)width * it1->py] = 255;
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
	return;
}

void bmphandler::get_contours(Point p, std::vector<std::vector<Point>>* outer_line,
		std::vector<std::vector<Point>>* inner_line, int minsize)
{
	get_contours(work_pt(p), outer_line, inner_line, minsize);
	return;
}

void bmphandler::distance_map(bool connectivity)
{
	unsigned char dummymode = mode1;

	std::vector<unsigned> sorted[256];
	std::vector<unsigned>::iterator it;

	bmp_is_grey = false; //xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx

	if (!bmp_is_grey)
	{
		swap_bmpwork();
		Pair p1;
		get_range(&p1);
		scale_colors(p1);
		swap_bmpwork();
	}

	bucketsort(sorted, bmp_bits);

	float wbp;
	unsigned p;
	for (unsigned i = 0; i < area; i++)
		work_bits[i] = 0;
	for (it = sorted[255].begin(); it != sorted[255].end(); it++)
	{
		work_bits[*it] = bmp_bits[*it];
	}

	for (unsigned i = 255; i > 0; i--)
	{
		for (it = sorted[i].begin(); it != sorted[i].end(); it++)
		{
			p = *it;
			wbp = work_bits[p];

			if ((unsigned)floor(wbp + 0.5f) == i)
			{
				if (p % width != 0 && work_bits[p - 1] < wbp - 1)
				{
					work_bits[p - 1] = wbp - 1;
					sorted[i - 1].push_back(p - 1);
				}
				if ((p + 1) % width != 0 && work_bits[p + 1] < wbp - 1)
				{
					work_bits[p + 1] = wbp - 1;
					sorted[i - 1].push_back(p + 1);
				}
				if (p >= width && work_bits[p - width] < wbp - 1)
				{
					work_bits[p - width] = wbp - 1;
					sorted[i - 1].push_back(p - width);
				}
				if ((p + width) < area && work_bits[p + width] < wbp - 1)
				{
					work_bits[p + width] = wbp - 1;
					sorted[i - 1].push_back(p + width);
				}
				if (connectivity)
				{
					if (p % width != 0 && p >= width &&
							work_bits[p - 1 - width] < wbp - 1)
					{
						work_bits[p - 1 - width] = wbp - 1;
						sorted[i - 1].push_back(p - 1 - width);
					}
					if (p % width != 0 && (p + width) < area &&
							work_bits[p - 1 + width] < wbp - 1)
					{
						work_bits[p - 1 + width] = wbp - 1;
						sorted[i - 1].push_back(p - 1 + width);
					}
					if ((p + 1) % width != 0 && p >= width &&
							work_bits[p - width + 1] < wbp - 1)
					{
						work_bits[p - width + 1] = wbp - 1;
						sorted[i - 1].push_back(p - width + 1);
					}
					if ((p + 1) % width != 0 && (p + width) < area &&
							work_bits[p + width + 1] < wbp - 1)
					{
						work_bits[p + width + 1] = wbp - 1;
						sorted[i - 1].push_back(p + width + 1);
					}
				}
			}
		}
	}

	mode1 = dummymode;
	mode2 = 1;

	return;
}

void bmphandler::distance_map(bool connectivity, float f,
		short unsigned levlset)
{
	unsigned char dummymode = mode1;
	std::vector<unsigned> v1, v2;
	std::vector<unsigned>*vp1, *vp2, *vpdummy;
	const float background = f - width - height;

	std::vector<std::vector<Point>> vo, vi;
	std::vector<unsigned>::iterator vuit;
	std::vector<Point>::iterator vpit;

	swap_bmpwork();
	get_contours(f, &vo, &vi, 0);
	swap_bmpwork();

	for (unsigned i = 0; i < (unsigned)vo.size(); i++)
		for (vpit = vo[i].begin(); vpit != vo[i].end(); vpit++)
			v1.push_back(pt2coord(*vpit));
	for (unsigned i = 0; i < (unsigned)vi.size(); i++)
		for (vpit = vi[i].begin(); vpit != vi[i].end(); vpit++)
			v1.push_back(pt2coord(*vpit));

	for (unsigned i = 0; i < area; i++)
		work_bits[i] = background;
	for (vuit = v1.begin(); vuit != v1.end(); vuit++)
		work_bits[*vuit] = f;

	if (levlset == 0)
		for (unsigned i = 0; i < area; i++)
			if (bmp_bits[i] == f)
				work_bits[i] = f;

	if (levlset == 1)
		for (unsigned i = 0; i < area; i++)
			if (bmp_bits[i] != f)
				work_bits[i] = f;

	vp1 = &v1;
	vp2 = &v2;

	float wbp;
	unsigned p;

	while (!(*vp1).empty())
	{
		for (vuit = (*vp1).begin(); vuit != (*vp1).end(); vuit++)
		{
			p = *vuit;
			wbp = work_bits[p];

			if (p % width != 0 && work_bits[p - 1] == background)
			{
				if (bmp_bits[p - 1] == f)
				{
					work_bits[p - 1] = wbp + 1;
					(*vp2).push_back(p - 1);
				}
				else
				{
					work_bits[p - 1] = wbp - 1;
					(*vp2).push_back(p - 1);
				}
			}

			if ((p + 1) % width != 0 && work_bits[p + 1] == background)
			{
				if (bmp_bits[p + 1] == f)
				{
					work_bits[p + 1] = wbp + 1;
					(*vp2).push_back(p + 1);
				}
				else
				{
					work_bits[p + 1] = wbp - 1;
					(*vp2).push_back(p + 1);
				}
			}

			if (p >= width && work_bits[p - width] == background)
			{
				if (bmp_bits[p - width] == f)
				{
					work_bits[p - width] = wbp + 1;
					(*vp2).push_back(p - width);
				}
				else
				{
					work_bits[p - width] = wbp - 1;
					(*vp2).push_back(p - width);
				}
			}

			if ((p + width) < area != 0 && work_bits[p + width] == background)
			{
				if (bmp_bits[p + width] == f)
				{
					work_bits[p + width] = wbp + 1;
					(*vp2).push_back(p + width);
				}
				else
				{
					work_bits[p + width] = wbp - 1;
					(*vp2).push_back(p + width);
				}
			}

			if (connectivity)
			{
				if (p % width != 0 && p >= width &&
						work_bits[p - 1 - width] == background)
				{
					if (bmp_bits[p - 1 - width] == f)
					{
						work_bits[p - 1 - width] = wbp + 1;
						(*vp2).push_back(p - 1 - width);
					}
					else
					{
						work_bits[p - 1 - width] = wbp - 1;
						(*vp2).push_back(p - 1 - width);
					}
				}

				if (p % width != 0 && (p + width) < area &&
						work_bits[p - 1 + width] == background)
				{
					if (bmp_bits[p - 1 + width] == f)
					{
						work_bits[p - 1 + width] = wbp + 1;
						(*vp2).push_back(p - 1 + width);
					}
					else
					{
						work_bits[p - 1 + width] = wbp - 1;
						(*vp2).push_back(p - 1 + width);
					}
				}

				if ((p + 1) % width != 0 && p >= width &&
						work_bits[p - width + 1] == background)
				{
					if (bmp_bits[p - width + 1] == f)
					{
						work_bits[p - width + 1] = wbp + 1;
						(*vp2).push_back(p - width + 1);
					}
					else
					{
						work_bits[p - width + 1] = wbp - 1;
						(*vp2).push_back(p - width + 1);
					}
				}

				if ((p + 1) % width != 0 && (p + width) < area &&
						work_bits[p + width + 1] == background)
				{
					if (bmp_bits[p + width + 1] == f)
					{
						work_bits[p + width + 1] = wbp + 1;
						(*vp2).push_back(p + width + 1);
					}
					else
					{
						work_bits[p + width + 1] = wbp - 1;
						(*vp2).push_back(p + width + 1);
					}
				}
			}
		}

		(*vp1).clear();
		vpdummy = vp1;
		vp1 = vp2;
		vp2 = vpdummy;
	}

	mode1 = dummymode;
	mode2 = 1;

	return;
}

unsigned* bmphandler::dead_reckoning(float f)
{
	unsigned char dummymode = mode1;
	unsigned* P = (unsigned*)malloc(area * sizeof(unsigned));

	for (unsigned i = 0; i < area; i++)
	{
		work_bits[i] = float((width + height) * (width + height));
		P[i] = area;
	}

	std::vector<std::vector<Point>> vo, vi;
	std::vector<unsigned>::iterator vuit;
	std::vector<Point>::iterator vpit;

	swap_bmpwork();
	get_contours(f, &vo, &vi, 0);
	swap_bmpwork();

	for (unsigned i = 0; i < (unsigned)vo.size(); i++)
	{
		for (vpit = vo[i].begin(); vpit != vo[i].end(); vpit++)
		{
			work_bits[pt2coord(*vpit)] = 0;
			P[pt2coord(*vpit)] = pt2coord(*vpit);
		}
		//		cout << ";"<<vo[i].size();
	}
	for (unsigned i = 0; i < (unsigned)vi.size(); i++)
	{
		for (vpit = vi[i].begin(); vpit != vi[i].end(); vpit++)
		{
			work_bits[pt2coord(*vpit)] = 0;
			P[pt2coord(*vpit)] = pt2coord(*vpit);
		}
		//		cout << "."<<vi[i].size();
	}

	unsigned j = 0;

	float d1 = 1;
	float d2 = sqrt(2.0f);

	for (unsigned short k = 0; k < height; k++)
	{
		for (unsigned short l = 0; l < width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && work_bits[j - 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j - width] + d1 < work_bits[j])
				{
					P[j] = P[j - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			if (l > 0 && work_bits[j - 1] + d1 < work_bits[j])
			{
				P[j] = P[j - 1];
				work_bits[j] =
						sqrt(float((l - P[j] % width) * (l - P[j] % width) +
											 (k - P[j] / width) * (k - P[j] / width)));
			}

			j++;
		}
	}

	j = area - 1;

	for (int k = height - 1; k >= 0; k--)
	{
		for (int l = width - 1; l >= 0; l--)
		{
			if ((l + 1) != width && work_bits[j + 1] + d1 < work_bits[j])
			{
				P[j] = P[j + 1];
				work_bits[j] =
						sqrt(float((l - P[j] % width) * (l - P[j] % width) +
											 (k - P[j] / width) * (k - P[j] / width)));
			}

			if ((k + 1) != height)
			{
				if (l > 0 && work_bits[j - 1 + width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 + width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j + width] + d1 < work_bits[j])
				{
					P[j] = P[j + width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 + width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 + width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			j--;
		}
	}

	j = 0;

	for (unsigned short k = 0; k < height; k++)
	{
		for (unsigned short l = 0; l < width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && work_bits[j - 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j - width] + d1 < work_bits[j])
				{
					P[j] = P[j - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			if (l > 0 && work_bits[j - 1] + d1 < work_bits[j])
			{
				P[j] = P[j - 1];
				work_bits[j] =
						sqrt(float((l - P[j] % width) * (l - P[j] % width) +
											 (k - P[j] / width) * (k - P[j] / width)));
			}

			j++;
		}
	}

	for (unsigned i = 0; i < area; i++)
		if (bmp_bits[i] != f)
			work_bits[i] = -work_bits[i];

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

	mode1 = dummymode;
	mode2 = 1;

	return P;
}

void bmphandler::dead_reckoning()
{
	unsigned char dummymode = mode1;
	unsigned* P = (unsigned*)malloc(area * sizeof(unsigned));

	for (unsigned i = 0; i < area; i++)
	{
		work_bits[i] = float((width + height) * (width + height));
		P[i] = area;
	}

	unsigned i1 = 0;
	for (unsigned short h = 0; h < height - 1; h++)
	{
		for (unsigned short w = 0; w < width; w++)
		{
			if (bmp_bits[i1] != bmp_bits[i1 + width])
			{
				work_bits[i1] = work_bits[i1 + width] = 0;
				P[i1] = i1;
				P[i1 + width] = i1 + width;
			}
			i1++;
		}
	}

	i1 = 0;
	for (unsigned short h = 0; h < height; h++)
	{
		for (unsigned short w = 0; w < width - 1; w++)
		{
			if (bmp_bits[i1] != bmp_bits[i1 + 1])
			{
				work_bits[i1] = work_bits[i1 + 1] = 0;
				P[i1] = i1;
				P[i1 + 1] = i1 + 1;
			}
			i1++;
		}
		i1++;
	}

	unsigned j = 0;

	float d1 = 1;
	float d2 = sqrt(2.0f);

	for (unsigned short k = 0; k < height; k++)
	{
		for (unsigned short l = 0; l < width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && work_bits[j - 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j - width] + d1 < work_bits[j])
				{
					P[j] = P[j - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			if (l > 0 && work_bits[j - 1] + d1 < work_bits[j])
			{
				P[j] = P[j - 1];
				work_bits[j] =
						sqrt(float((l - P[j] % width) * (l - P[j] % width) +
											 (k - P[j] / width) * (k - P[j] / width)));
			}

			j++;
		}
	}

	j = area - 1;

	for (int k = height - 1; k >= 0; k--)
	{
		for (int l = width - 1; l >= 0; l--)
		{
			if ((l + 1) != width && work_bits[j + 1] + d1 < work_bits[j])
			{
				P[j] = P[j + 1];
				work_bits[j] =
						sqrt(float((l - P[j] % width) * (l - P[j] % width) +
											 (k - P[j] / width) * (k - P[j] / width)));
			}

			if ((k + 1) != height)
			{
				if (l > 0 && work_bits[j - 1 + width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 + width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j + width] + d1 < work_bits[j])
				{
					P[j] = P[j + width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 + width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 + width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			j--;
		}
	}

	j = 0;

	for (unsigned short k = 0; k < height; k++)
	{
		for (unsigned short l = 0; l < width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && work_bits[j - 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j - width] + d1 < work_bits[j])
				{
					P[j] = P[j - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 - width];
					work_bits[j] =
							sqrt(float((l - P[j] % width) * (l - P[j] % width) +
												 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			if (l > 0 && work_bits[j - 1] + d1 < work_bits[j])
			{
				P[j] = P[j - 1];
				work_bits[j] =
						sqrt(float((l - P[j] % width) * (l - P[j] % width) +
											 (k - P[j] / width) * (k - P[j] / width)));
			}

			j++;
		}
	}

	//	for(unsigned i=0;i<area;i++)
	//		if(bmp_bits[i]==0) work_bits[i]=-work_bits[i];

	mode1 = dummymode;
	mode2 = 1;

	free(P);
	return;
}

unsigned* bmphandler::dead_reckoning_squared(float f)
{
	unsigned char dummymode = mode1;
	unsigned* P = (unsigned*)malloc(area * sizeof(unsigned));

	for (unsigned i = 0; i < area; i++)
	{
		work_bits[i] = float((width + height) * (width + height));
		P[i] = area;
	}

	std::vector<std::vector<Point>> vo, vi;
	std::vector<unsigned>::iterator vuit;
	std::vector<Point>::iterator vpit;

	swap_bmpwork();
	get_contours(f, &vo, &vi, 0);
	swap_bmpwork();

	for (unsigned i = 0; i < (unsigned)vo.size(); i++)
	{
		for (vpit = vo[i].begin(); vpit != vo[i].end(); vpit++)
		{
			work_bits[pt2coord(*vpit)] = 0;
			P[pt2coord(*vpit)] = pt2coord(*vpit);
		}
		//		cout << ";"<<vo[i].size();
	}
	for (unsigned i = 0; i < (unsigned)vi.size(); i++)
	{
		for (vpit = vi[i].begin(); vpit != vi[i].end(); vpit++)
		{
			work_bits[pt2coord(*vpit)] = 0;
			P[pt2coord(*vpit)] = pt2coord(*vpit);
		}
		//		cout << "."<<vi[i].size();
	}

	unsigned j = 0;

	float d1 = 1;
	float d2 = sqrt(2.0f);

	for (unsigned short k = 0; k < height; k++)
	{
		for (unsigned short l = 0; l < width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && work_bits[j - 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 - width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j - width] + d1 < work_bits[j])
				{
					P[j] = P[j - width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 - width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			if (l > 0 && work_bits[j - 1] + d1 < work_bits[j])
			{
				P[j] = P[j - 1];
				work_bits[j] = (float((l - P[j] % width) * (l - P[j] % width) +
															(k - P[j] / width) * (k - P[j] / width)));
			}

			j++;
		}
	}

	j = area - 1;

	for (int k = height - 1; k >= 0; k--)
	{
		for (int l = width - 1; l >= 0; l--)
		{
			if ((l + 1) != width && work_bits[j + 1] + d1 < work_bits[j])
			{
				P[j] = P[j + 1];
				work_bits[j] = (float((l - P[j] % width) * (l - P[j] % width) +
															(k - P[j] / width) * (k - P[j] / width)));
			}

			if ((k + 1) != height)
			{
				if (l > 0 && work_bits[j - 1 + width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 + width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j + width] + d1 < work_bits[j])
				{
					P[j] = P[j + width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 + width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 + width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			j--;
		}
	}

	j = 0;

	for (unsigned short k = 0; k < height; k++)
	{
		for (unsigned short l = 0; l < width; l++)
		{
			if (k > 0)
			{
				if (l > 0 && work_bits[j - 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j - 1 - width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}

				if (work_bits[j - width] + d1 < work_bits[j])
				{
					P[j] = P[j - width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}

				if ((l + 1) != width &&
						work_bits[j + 1 - width] + d2 < work_bits[j])
				{
					P[j] = P[j + 1 - width];
					work_bits[j] =
							(float((l - P[j] % width) * (l - P[j] % width) +
										 (k - P[j] / width) * (k - P[j] / width)));
				}
			}

			if (l > 0 && work_bits[j - 1] + d1 < work_bits[j])
			{
				P[j] = P[j - 1];
				work_bits[j] = (float((l - P[j] % width) * (l - P[j] % width) +
															(k - P[j] / width) * (k - P[j] / width)));
			}

			j++;
		}
	}

	for (unsigned i = 0; i < area; i++)
		if (bmp_bits[i] != f)
			work_bits[i] = -work_bits[i];

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

	mode1 = dummymode;
	mode2 = 1;

	return P;
}

void bmphandler::IFT_distance1(float f)
{
	unsigned char dummymode = mode1;
	ImageForestingTransformDistance IFTdist;
	IFTdist.distance_init(width, height, f, bmp_bits);
	float* f1 = IFTdist.return_pf();
	for (unsigned i = 0; i < area; i++)
	{
		if (bmp_bits[i] == f)
			work_bits[i] = f1[i];
		else
			work_bits[i] = -f1[i];
	}
	mode1 = dummymode;
	mode2 = 1;
	return;
}

void bmphandler::rgIFT(float* lb_map, float thresh)
{
	unsigned char dummymode = mode1;
	ImageForestingTransformRegionGrowing IFTrg;

	//	sobel();

	//	IFTrg.rg_init(width,height,work_bits,lb_map);
	IFTrg.rg_init(width, height, bmp_bits, lb_map);

	float* f1 = IFTrg.return_lb();
	float* f2 = IFTrg.return_pf();

	for (unsigned i = 0; i < area; i++)
	{
		if (f2[i] < thresh)
			work_bits[i] = f1[i];
		else
			work_bits[i] = 0;
	}

	mode1 = dummymode;
	mode2 = 2;

	return;
}

ImageForestingTransformRegionGrowing* bmphandler::IFTrg_init(float* lb_map)
{
	ImageForestingTransformRegionGrowing* IFTrg =
			new ImageForestingTransformRegionGrowing;

	//	float *tmp=work_bits;
	//	work_bits=sliceprovide->give_me();

	//	sobel();

	//	IFTrg->rg_init(width,height,work_bits,lb_map);
	IFTrg->rg_init(width, height, bmp_bits, lb_map);

	//	sliceprovide->take_back(work_bits);

	//	work_bits=tmp;

	return IFTrg;
}

ImageForestingTransformLivewire* bmphandler::livewireinit(Point pt)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	float* sobelx = sliceprovide->give_me();
	float* sobely = sliceprovide->give_me();
	float* tmp = sliceprovide->give_me();
	float* dummy;
	float* dummy1;
	float* grad = work_bits;
	work_bits = sliceprovide->give_me();

	gaussian(1);
	dummy = bmp_bits;
	bmp_bits = tmp;
	tmp = dummy;
	swap_bmpwork();
	sobelxy(&sobelx, &sobely);

	dummy = direction_map(sobelx, sobely);

	Pair p;
	get_range(&p);

	//	sliceprovide->take_back(sobely);
	//	sobely=grad;

	//	grad=work_bits;
	//	work_bits=sobelx;
	dummy1 = sobely;
	sobely = work_bits;
	work_bits = dummy1;

	laplacian_zero(2.0f, 30, false);

	if (p.high != 0)
		for (unsigned i = 0; i < area; i++)
			sobelx[i] = (0.43f * (1 - sobely[i] / p.high) +
									 0.43f * ((work_bits[i] + 1) / 256));
	else
		for (unsigned i = 0; i < area; i++)
			sobelx[i] = 0.43f * ((work_bits[i] + 1) / 256);

	ImageForestingTransformLivewire* lw = new ImageForestingTransformLivewire;
	lw->lw_init(width, height, sobelx, dummy, pt);

	sliceprovide->take_back(sobelx);
	sliceprovide->take_back(sobely);
	sliceprovide->take_back(bmp_bits);
	sliceprovide->take_back(work_bits);
	sliceprovide->take_back(dummy);
	bmp_bits = tmp;
	work_bits = grad;

	mode1 = dummymode1;
	mode2 = dummymode2;

	return lw;
}

void bmphandler::fill_contour(std::vector<Point>* vp, bool continuous)
{
	unsigned char dummymode = mode1;

	if (continuous)
	{
		std::vector<int> s;
		float* results =
				(float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

		int i = width + 3;
		int i1 = 0;
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < width; k++)
			{
				results[i] = -1;
				i++;
				i1++;
			}

			i += 2;
		}

		for (std::vector<Point>::iterator it = vp->begin(); it != vp->end(); it++)
		{
			results[it->px + 1 + (it->py + 1) * (width + 2)] = 0;
		}

		for (int j = 0; j < width + 2; j++)
			results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
		for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
			results[j] = results[j + width + 1] = 0;

		for (int j = width + 3; j < 2 * width + 3; j++)
		{
			if (results[j] == -1)
			{
				results[j] = 255.0f;
				s.push_back(j);
			}
		}
		for (unsigned int j = area + 2 * height + 1;
				 j < area + width + 2 * height + 1; j++)
		{
			if (results[j] == -1)
			{
				results[j] = 255.0f;
				s.push_back(j);
			}
		}
		for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
				 j += width + 2)
		{
			if (results[j] == -1)
			{
				results[j] = 255.0f;
				s.push_back(j);
			}
		}
		for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
				 j += width + 2)
		{
			if (results[j] == -1)
			{
				results[j] = 255.0f;
				s.push_back(j);
			}
		}

		hysteretic_growth(results, &s, width + 2, height + 2, false, 255.0f);

		i = width + 3;
		int i2 = 0;
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < width; k++)
			{
				work_bits[i2] = 255.0f - results[i];
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

		fill_contour(&vp1, true);
	}

	mode1 = dummymode;
	mode2 = 2;

	return;
}

void bmphandler::add_skin(unsigned i4, float setto)
{
	unsigned i = 0, pos, y, x, j;

	//Create a binary std::vector noTissue/Tissue
	std::vector<int> s;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i] == 0)
				s.push_back(-1);
			else
				s.push_back(0);
			i++;
		}
	}

	// i4 itetations through  y, -y, x, -x converting, each time a tissue beginning is find, one tissue pixel into skin
	//!! It is assumed that ix and iy are the same
	bool convertSkin = true;
	for (i = 1; i < i4 + 1; i++)
	{
		for (y = 0; y < height; y++)
		{
			pos = y * width;
			while (pos < (y + 1) * width)
			{
				if (s[pos] == 0)
				{
					if (convertSkin)
						s[pos] = 256;
					convertSkin = false;
				}
				else
					convertSkin = true;

				pos++;
			}

			pos = (y + 1) * width - 1;
			while (pos > y * width)
			{
				if (s[pos] == 0)
				{
					if (convertSkin)
						s[pos] = 256;
					convertSkin = false;
				}
				else
					convertSkin = true;

				pos--;
			}
		}

		for (x = 0; x < width; x++)
		{
			pos = x;
			while (pos < height * width)
			{
				if (s[pos] == 0)
				{
					if (convertSkin)
						s[pos] = 256;
					convertSkin = false;
				}
				else
					convertSkin = true;

				pos += width;
			}

			pos = width * (height - 1) + x;
			while (pos > width)
			{
				if (s[pos] == 0)
				{
					if (convertSkin)
						s[pos] = 256;
					convertSkin = false;
				}
				else
					convertSkin = true;

				pos -= width;
			}
		}
	}

	//go over the std::vector and set the skin pixel at the source pointer
	i = 0;
	for (j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (s[i] == 256)
				work_bits[i] = setto;
			i++;
		}
	}
}

void bmphandler::add_skin_outside(unsigned i4, float setto)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;
	std::vector<int> s;
	std::vector<int> s1;
	float* results =
			(float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i3 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i3] == 0)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	//hysteretic_growth(results,&s,width+2,height+2,false,255.0f);
	hysteretic_growth(results, &s, width + 2, height + 2, true, 255.0f);

	for (int j = 0; j <= ((int)width + 2) * (height + 2); j++)
		if (results[j] == 255.0f)
			results[j] = -1;

	//hysteretic_growth(results,&s1,width+2,height+2,false,255.0f,i4);
	hysteretic_growth(results, &s1, width + 2, height + 2, true, 255.0f, i4);

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (results[i] == 255.0f)
				work_bits[i2] = setto;
			//			work_bits[i2]=results[i];
			i++;
			i2++;
		}

		i += 2;
	}

	mode1 = dummymode1;
	mode2 = dummymode2;

	free(results);
}

void bmphandler::add_skintissue(tissuelayers_size_t idx, unsigned i4,
		tissues_size_t setto)
{
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i3 = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	hysteretic_growth(results, &s, width + 2, height + 2, false, 255.0f);

	unsigned short x, y;
	unsigned pos;
	unsigned i1;
	unsigned w = (unsigned)(width + 2);

	for (y = 1; y < height + 1; y++)
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

	for (y = 1; y < height + 1; y++)
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

	for (x = 1; x < width + 1; x++)
	{
		pos = x;
		i1 = 0;
		while (pos < (height + 1) * w)
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

	for (x = 1; x < width + 1; x++)
	{
		pos = w * (height + 1) + x;
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

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

void bmphandler::add_skintissue_outside(tissuelayers_size_t idx, unsigned i4,
		tissues_size_t setto)
{
	std::vector<int> s;
	std::vector<int> s1;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i3 = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	hysteretic_growth(results, &s, width + 2, height + 2, false, 255.0f);

	for (int j = 0; j <= ((int)width + 2) * (height + 2); j++)
		if (results[j] == 255.0f)
			results[j] = -1;

	hysteretic_growth(results, &s1, width + 2, height + 2, false, 255.0f, i4);

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

bool bmphandler::value_at_boundary(float value)
{
	// Top
	float* tmp = &work_bits[0];
	for (unsigned pos = 0; pos < width; pos++, tmp++)
	{
		if (*tmp == value)
			return true;
	}

	// Left & right
	for (unsigned pos = 1; pos < (height - 1); pos++)
	{
		if (work_bits[pos * width] == value)
			return true;
		else if (work_bits[(pos + 1) * width - 1] == value)
			return true;
	}

	// Bottom
	tmp = &work_bits[(height - 1) * width];
	for (unsigned pos = 0; pos < width; pos++, tmp++)
	{
		if (*tmp == value)
			return true;
	}
	return false;
}

bool bmphandler::tissuevalue_at_boundary(tissuelayers_size_t idx,
		tissues_size_t value)
{
	// Top
	tissues_size_t* tissues = tissuelayers[idx];
	tissues_size_t* tmp = &(tissues[0]);
	for (unsigned pos = 0; pos < width; pos++, tmp++)
	{
		if (*tmp == value)
			return true;
	}

	// Left & right
	for (unsigned pos = 1; pos < (height - 1); pos++)
	{
		if (tissues[pos * width] == value)
			return true;
		else if (tissues[(pos + 1) * width - 1] == value)
			return true;
	}

	// Bottom
	tmp = &(tissues[(height - 1) * width]);
	for (unsigned pos = 0; pos < width; pos++, tmp++)
	{
		if (*tmp == value)
			return true;
	}
	return false;
}

float bmphandler::add_skin(unsigned i)
{
	Pair p;
	get_range(&p);
	float setto;
	if (p.high <= 254.0f)
		setto = 255.0f;
	else if (p.low >= 1.0f)
		setto = 0.0f;
	else
	{
		setto = p.low;
		for (unsigned pos = 0; pos < area; pos++)
		{
			if (work_bits[pos] != p.high)
				setto = std::max(setto, work_bits[pos]);
		}
		setto = (setto + p.high) / 2;
	}

	add_skin(i, setto);
	return setto;
}

float bmphandler::add_skin_outside(unsigned i)
{
	Pair p;
	get_range(&p);
	float setto;
	if (p.high <= 254.0f)
		setto = 255.0f;
	else if (p.low >= 1.0f)
		setto = 0.0f;
	else
	{
		setto = p.low;
		for (unsigned pos = 0; pos < area; pos++)
		{
			if (work_bits[pos] != p.high)
				setto = std::max(setto, work_bits[pos]);
		}
		setto = (setto + p.high) / 2;
	}

	add_skin_outside(i, setto);
	return setto;
}

void bmphandler::fill_skin(int thicknessX, int thicknessY,
		tissues_size_t backgroundID, tissues_size_t skinID)
{
	//BL recommendation
	int skinThick = thicknessX;

	std::vector<int> dims;
	dims.push_back(width);
	dims.push_back(height);

	//int backgroundID = (bmp_bits[0]==bmp_bits[area-1]) ? bmp_bits[0] : 0;

	double max_d = skinThick == 1 ? 1.5 * skinThick : 1.2 * skinThick;
	std::vector<int> offsets;

	//Create the relative neighborhood
	for (int j = -skinThick; j <= skinThick; j++)
	{
		for (int i = -skinThick; i <= skinThick; i++)
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
	if (tissuelayers.size() > 0)
		tissues = tissuelayers[0];

	bool previewWay = true;

	if (previewWay)
	{
		float* bmp1 = this->return_bmp();

		tissues_size_t* tissue1 = this->return_tissues(0);
		this->pushstack_bmp();

		for (unsigned int i = 0; i < area; i++)
		{
			bmp1[i] = (float)tissue1[i];
		}
		this->dead_reckoning((float)0);
		bmp1 = this->return_work();

		for (int j = skinThick; j + skinThick < dims[1]; j++)
		{
			for (int i = skinThick; i + skinThick < dims[0]; i++)
			{
				int pos = i + j * dims[0];
				if (tissues[pos] == backgroundID)
				{
					//iterate through neighbors of pixel
					//if any of these are not
					for (int l = 0; l < offsets.size(); l++)
					{
						int idx = pos + offsets[l];
						assert(idx >= 0 && idx < area);
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
							work_bits[pos] = 255.0f;
					}
				}
			}
		}
		for (unsigned i = 0; i < area; i++)
		{
			if (bmp1[i] < 0)
				bmp1[i] = 0;
			else
				bmp1[i] = 255.0f;
		}

		this->set_mode(2, false);
		this->popstack_bmp();
	}

	else
	{
		for (int j = skinThick; j + skinThick < dims[1]; j++)
		{
			for (int i = skinThick; i + skinThick < dims[0]; i++)
			{
				int pos = i + j * dims[0];
				if (tissues[pos] == backgroundID)
				{
					//iterate through neighbors of pixel
					//if any of these are not
					for (int l = 0; l < offsets.size(); l++)
					{
						int idx = pos + offsets[l];
						assert(idx >= 0 && idx < area);

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

void bmphandler::flood_exterior(float setto)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i3 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i3] == 0)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	hysteretic_growth(results, &s, width + 2, height + 2, false, 255.0f);

	i = width + 3;
	i3 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (results[i] == 255.0f)
				work_bits[i3] = setto;
			i++;
			i3++;
		}

		i += 2;
	}

	mode1 = dummymode1;
	mode2 = dummymode2;

	free(results);
}

void bmphandler::flood_exteriortissue(tissuelayers_size_t idx,
		tissues_size_t setto)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i3 = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	hysteretic_growth(results, &s, width + 2, height + 2, false, 255.0f);

	i = width + 3;
	i3 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (results[i] == 255.0f)
				tissues[i3] = setto;
			i++;
			i3++;
		}

		i += 2;
	}

	mode1 = dummymode1;
	mode2 = dummymode2;

	free(results);
}

void bmphandler::fill_unassigned()
{
	Pair p;
	get_range(&p);
	float setto;
	if (p.high <= 254.0f)
		setto = 255.0f;
	else if (p.low >= 1.0f)
		setto = 0.0f;
	else
	{
		setto = p.low;
		for (unsigned pos = 0; pos < area; pos++)
		{
			if (work_bits[pos] != p.high)
				setto = std::max(setto, work_bits[pos]);
		}
		setto = (setto + p.high) / 2;
	}

	fill_unassigned(setto);
}

void bmphandler::fill_unassigned(float setto)
{
	std::vector<int> s;
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i3 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i3] == 0)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i3++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	hysteretic_growth(results, &s, width + 2, height + 2, false, 255.0f);

	unsigned pos = width + 3;
	unsigned long pos1 = 0;

	for (unsigned short py = 0; py < height; py++)
	{
		for (unsigned short px = 0; px < width; px++)
		{
			if (results[pos] != 255.0f && work_bits[pos1] == 0)
				work_bits[pos1] = setto;
			pos++;
			pos1++;
		}
		pos += 2;
	}

	free(results);
}

void bmphandler::fill_unassignedtissue(tissuelayers_size_t idx,
		tissues_size_t setto)
{
	std::vector<int> s;
	float* results =
			(float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i3 = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (results[j] == -1)
		{
			results[j] = 255.0f;
			s.push_back(j);
		}
	}

	hysteretic_growth(results, &s, width + 2, height + 2, false, 255.0f);

	unsigned pos = width + 3;
	unsigned long pos1 = 0;

	for (unsigned short py = 0; py < height; py++)
	{
		for (unsigned short px = 0; px < width; px++)
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

void bmphandler::adaptive_fuzzy(Point p, float m1, float s1, float s2,
		float thresh)
{
	UNREFERENCED_PARAMETER(thresh);
	ImageForestingTransformAdaptFuzzy af;
	af.fuzzy_init(width, height, bmp_bits, p, m1, s1, s2);
	float* pf = af.return_pf();
	for (unsigned i = 0; i < area; i++)
	{
		/*		if(pf[i]<1-thresh) work_bits[i]=255;
		else work_bits[i]=0;*/
		work_bits[i] = pf[i] * 255;
	}
	mode2 = 1;
	return;
}

void bmphandler::fast_marching(Point p, float sigma, float thresh)
{
	unsigned char dummymode = mode1;
	ImageForestingTransformFastMarching fm;

	float* dummy;
	float* lbl = sliceprovide->give_me();
	gaussian(sigma);
	dummy = lbl;
	lbl = bmp_bits;
	bmp_bits = dummy;
	swap_bmpwork();
	sobel();
	dummy = lbl;
	lbl = bmp_bits;
	bmp_bits = dummy;

	for (unsigned i = 0; i < area; i++)
	{
		work_bits[i] = exp(-work_bits[i] / thresh);
		lbl[i] = 0;
	}
	lbl[pt2coord(p)] = 1;

	fm.fastmarch_init(width, height, work_bits, lbl);
	float* pf = fm.return_pf();
	for (unsigned i = 0; i < area; i++)
	{
		work_bits[i] = pf[i];
	}
	Pair p1;
	get_range(&p1);
	//	cout << p1.high << " " << p1.low << endl;
	//	scale_colors(p1);

	sliceprovide->take_back(lbl);

	mode1 = dummymode;
	mode2 = 1;

	return;
}

ImageForestingTransformFastMarching*
		bmphandler::fastmarching_init(Point p, float sigma, float thresh)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;
	ImageForestingTransformFastMarching* fm =
			new ImageForestingTransformFastMarching;

	float* dummy;
	float* work_store = work_bits;
	work_bits = sliceprovide->give_me();
	gaussian(sigma);
	float* lbl = bmp_bits;
	bmp_bits = sliceprovide->give_me();
	swap_bmpwork();
	sobel();
	dummy = lbl;
	lbl = bmp_bits;
	bmp_bits = dummy;

	for (unsigned i = 0; i < area; i++)
	{
		work_bits[i] = exp(-work_bits[i] / thresh);
		lbl[i] = 0;
	}
	lbl[pt2coord(p)] = 1;

	fm->fastmarch_init(width, height, work_bits, lbl);

	sliceprovide->take_back(lbl);
	sliceprovide->take_back(work_bits);
	work_bits = work_store;

	mode1 = dummymode1;
	mode2 = dummymode2;

	return fm;
}

float bmphandler::extract_feature(Point p1, Point p2)
{
	fextract.init(bmp_bits, p1, p2, width, height);
	return fextract.return_average();
}

float bmphandler::extract_featurework(Point p1, Point p2)
{
	fextract.init(work_bits, p1, p2, width, height);
	return fextract.return_average();
}

float bmphandler::return_stdev()
{
	return fextract.return_stddev();
	;
}

Pair bmphandler::return_extrema()
{
	Pair p;
	fextract.return_extrema(&p);
	return p;
}

void bmphandler::classify(short nrclasses, short dim, float** bits,
		float* weights, float* centers, float maxdist)
{
	short k;
	float dist, distmin;
	maxdist = maxdist * maxdist;
	short unsigned cindex;

	for (unsigned i = 0; i < area; i++)
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
			work_bits[i] = 255.0f * (k + 1) / nrclasses;
		else
			work_bits[i] = 0;
	}

	mode2 = 2;

	return;
}

void bmphandler::kmeans(short nrtissues, short dim, float** bits,
		float* weights, unsigned int iternr,
		unsigned int converge)
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
	kmeans.init(width, height, nrtissues, dim, bits, weightsnew);
	kmeans.make_iter(iternr, converge);
	kmeans.return_m(work_bits);
	free(weightsnew);
	mode2 = 2;
}

void bmphandler::kmeans_mhd(short nrtissues, short dim,
		std::vector<std::string> mhdfiles,
		unsigned short slicenr, float* weights,
		unsigned int iternr, unsigned int converge)
{
	float** bits = new float*[dim];
	for (unsigned short j = 0; j + 1 < dim; j++)
	{
		bits[j + 1] = sliceprovide->give_me();
	}

	bits[0] = bmp_bits;
	for (unsigned short i = 0; i + 1 < dim; i++)
	{
		if (!ImageReader::getSlice(mhdfiles[i].c_str(), bits[i + 1], slicenr,
						width, height))
		{
			for (unsigned short j = 1; j < dim; j++)
				sliceprovide->take_back(bits[j]);
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
	kmeans.init(width, height, nrtissues, dim, bits, weightsnew);
	kmeans.make_iter(iternr, converge);
	kmeans.return_m(work_bits);
	free(weightsnew);

	for (unsigned short j = 1; j < dim; j++)
		sliceprovide->take_back(bits[j]);
	delete[] bits;

	mode2 = 2;
}

void bmphandler::kmeans_png(short nrtissues, short dim,
		std::vector<std::string> pngfiles,
		std::vector<int> exctractChannel,
		unsigned short slicenr, float* weights,
		unsigned int iternr, unsigned int converge,
		const std::string initCentersFile)
{
	float** bits = new float*[dim];
	for (unsigned short j = 0; j + 1 < dim; j++)
	{
		bits[j + 1] = sliceprovide->give_me();
	}

	bits[0] = bmp_bits;
	for (unsigned short i = 0; i + 1 < dim; i++)
	{
		if (!ChannelExtractor::getSlice(pngfiles[0].c_str(), bits[i + 1],
						exctractChannel[i], slicenr, width,
						height))
		{
			for (unsigned short j = 1; j < dim; j++)
				sliceprovide->take_back(bits[j]);
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
	if (initCentersFile != "")
	{
		float* centers = nullptr;
		int dimensions;
		int nrClasses;
		if (kmeans.get_centers_from_file(initCentersFile, centers, dimensions,
						nrClasses))
		{
			dim = dimensions;
			nrtissues = nrClasses;
			kmeans.init(width, height, nrtissues, dim, bits, weightsnew,
					centers);
		}
		else
		{
			QMessageBox msgBox;
			msgBox.setText("ERROR: reading centers initialization file.");
			msgBox.exec();
			return;
		}
	}
	else
	{
		kmeans.init(width, height, nrtissues, dim, bits, weightsnew);
	}
	kmeans.make_iter(iternr, converge);
	kmeans.return_m(work_bits);
	free(weightsnew);

	for (unsigned short j = 1; j < dim; j++)
		sliceprovide->take_back(bits[j]);
	delete[] bits;

	mode2 = 2;
}

void bmphandler::gamma_mhd(short nrtissues, short dim,
		std::vector<std::string> mhdfiles,
		unsigned short slicenr, float* weights,
		float** centers, float* tol_f, float* tol_d,
		Pair pixelsize)
{
	float** bits = new float*[dim];
	for (unsigned short j = 0; j + 1 < dim; j++)
	{
		bits[j + 1] = sliceprovide->give_me();
	}

	bits[0] = bmp_bits;
	for (unsigned short i = 0; i + 1 < dim; i++)
	{
		if (!ImageReader::getSlice(mhdfiles[i].c_str(), bits[i + 1], slicenr,
						width, height))
		{
			for (unsigned short j = 1; j < dim; j++)
				sliceprovide->take_back(bits[j]);
			delete[] bits;
			return;
		}
	}

	MultidimensionalGamma mdg;
	mdg.init(width, height, nrtissues, dim, bits, weights, centers, tol_f,
			tol_d, pixelsize.high, pixelsize.low);
	mdg.execute();
	mdg.return_image(work_bits);

	for (unsigned short j = 1; j < dim; j++)
		sliceprovide->take_back(bits[j]);
	delete[] bits;

	mode2 = 2;
}

void bmphandler::em(short nrtissues, short dim, float** bits, float* weights,
		unsigned int iternr, unsigned int converge)
{
	ExpectationMaximization em;
	em.init(width, height, nrtissues, dim, bits, weights);
	em.make_iter(iternr, converge);
	em.classify(work_bits);
	mode2 = 2;
}

void bmphandler::cannylevelset(float* initlev, float f, float sigma,
		float thresh_low, float thresh_high,
		float epsilon, float stepsize, unsigned nrsteps,
		unsigned reinitfreq)
{
	unsigned char dummymode = mode1;
	float* tmp = sliceprovide->give_me();
	float* dummy;

	canny_line(sigma, thresh_low, thresh_high);
	dummy = tmp;
	tmp = bmp_bits;
	bmp_bits = dummy;
	swap_bmpwork();
	//	dead_reckoning_squared(255.0f);
	//	IFT_distance1(255.0f);
	dead_reckoning(255.0f);
	dummy = tmp;
	tmp = bmp_bits;
	bmp_bits = dummy;
	//	for(unsigned i=0;i<area;i++) work_bits[i]=work_bits[i];
	Pair p;
	get_range(&p);
	//	cout << p.low << " " << p.high << endl;
	p.low = p.low * 10;
	p.high = p.high * 10;
	//	p.low=p.high*10;
	scale_colors(p);
	get_range(&p);
	//	cout <<"+"<< p.low << " " << p.high << endl;

	//	for(unsigned i=0;i<area;i++) work_bits[i]=0.0f;
	for (unsigned i = 0; i < area; i++)
		tmp[i] = 1.0f;

	Levelset levset;
	levset.init(height, width, initlev, f, tmp, work_bits, 0.0f, epsilon,
			stepsize);
	levset.iterate(nrsteps, reinitfreq);
	levset.return_levelset(work_bits);
	//	SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testdump1.bmp");

	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 0;
	//	threshold(thresh);
	for (unsigned i = 0; i < area; i++)
	{
		if (work_bits[i] < 0)
			//			work_bits[i]=tmp[i];
			work_bits[i] = 0;
		else
			//			work_bits[i]=256-tmp[i];
			work_bits[i] = 256;
	}
	sliceprovide->take_back(tmp);

	mode1 = dummymode;
	mode2 = 2;

	return;
}

void bmphandler::threshlevelset(float thresh_low, float thresh_high,
		float epsilon, float stepsize, unsigned nrsteps,
		unsigned reinitfreq)
{
	float mean = (thresh_high + thresh_low) / 2;
	float halfdiff = (thresh_high - thresh_low) / 2;
	for (unsigned i = 0; i < area; ++i)
		work_bits[i] = 1 - abs(bmp_bits[i] - mean) / halfdiff;

	Levelset levset;
	Point Pt;
	Pt.px = 376;
	Pt.py = 177;
	/*	Pt.px=215;
	Pt.py=266;*/
	float* tmp = sliceprovide->give_me();
	for (unsigned i = 0; i < area; i++)
		tmp[i] = 0;
	levset.init(height, width, Pt, work_bits, tmp, 1.0f, epsilon, stepsize);
	levset.iterate(nrsteps, reinitfreq);
	levset.return_levelset(work_bits);
	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 0;
	for (unsigned i = 0; i < area; i++)
	{
		if (work_bits[i] < 0)
			//			work_bits[i]=tmp[i];
			work_bits[i] = 0.0f;
		else
			//			work_bits[i]=256-tmp[i];
			work_bits[i] = 256.0f;
	}

	sliceprovide->take_back(tmp);

	mode2 = 2;

	return;
}

unsigned bmphandler::pushstack_bmp()
{
	float* bits = sliceprovide->give_me();

	for (unsigned i = 0; i < area; ++i)
	{
		bits[i] = bmp_bits[i];
	}

	bits_stack.push_back(bits);
	stackindex.push_back(stackcounter);
	mode_stack.push_back(mode1);

	return stackcounter++;
}

unsigned bmphandler::pushstack_work()
{
	float* bits = sliceprovide->give_me();

	for (unsigned i = 0; i < area; ++i)
	{
		bits[i] = work_bits[i];
	}

	bits_stack.push_back(bits);
	stackindex.push_back(stackcounter);
	mode_stack.push_back(mode2);

	return stackcounter++;
}

bool bmphandler::savestack(unsigned i, const char* filename)
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
		if (fwrite(&width, 1, sizeof(short unsigned), fp) <
				sizeof(short unsigned))
		{
			fclose(fp);
			return false;
		}
		if (fwrite(&height, 1, sizeof(short unsigned), fp) <
				sizeof(short unsigned))
		{
			fclose(fp);
			return false;
		}
		unsigned int bitsize = width * (unsigned)height * sizeof(float);
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

unsigned bmphandler::loadstack(const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if (fp == nullptr)
		return 123456;

	short unsigned width1, height1;
	if (fread(&width1, 1, sizeof(short unsigned), fp) <
					sizeof(short unsigned) ||
			width1 != width)
	{
		fclose(fp);
		return 123456;
	}
	if (fread(&height1, 1, sizeof(short unsigned), fp) <
					sizeof(short unsigned) ||
			height1 != height)
	{
		fclose(fp);
		return 123456;
	}
	unsigned int bitsize = width * (unsigned)height * sizeof(float);
	float* bits = sliceprovide->give_me();
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

unsigned bmphandler::pushstack_tissue(tissuelayers_size_t idx,
		tissues_size_t tissuenr)
{
	float* bits = sliceprovide->give_me();

	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned i = 0; i < area; ++i)
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

unsigned bmphandler::pushstack_help()
{
	float* bits = sliceprovide->give_me();

	for (unsigned i = 0; i < area; ++i)
	{
		bits[i] = help_bits[i];
	}

	bits_stack.push_back(bits);
	stackindex.push_back(stackcounter);
	mode_stack.push_back(0);

	return stackcounter++;
}

void bmphandler::removestack(unsigned i)
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
		sliceprovide->take_back(*it);
		bits_stack.erase(it);
		stackindex.erase(it1);
		mode_stack.erase(it2);
	}
}

void bmphandler::getstack_bmp(unsigned i)
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
		for (unsigned i = 0; i < area; i++)
			bmp_bits[i] = (*it)[i];
	}

	mode1 = (*it2);
}

void bmphandler::getstack_work(unsigned i)
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
		for (unsigned i = 0; i < area; i++)
			work_bits[i] = (*it)[i];
	}

	mode2 = (*it2);
}

void bmphandler::getstack_tissue(tissuelayers_size_t idx, unsigned i,
		tissues_size_t tissuenr, bool override)
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
	tissues_size_t* tissues = tissuelayers[idx];
	if (it != bits_stack.end())
	{
		if (override)
		{
			for (unsigned i = 0; i < area; i++)
			{
				if (((*it)[i] != 0) &&
						(!TissueInfos::GetTissueLocked(tissues[i])))
					tissues[i] = tissuenr;
			}
		}
		else
		{
			for (unsigned i = 0; i < area; i++)
			{
				if (((*it)[i] != 0) && (tissues[i] == 0))
					tissues[i] = tissuenr;
			}
		}
	}
}

void bmphandler::getstack_help(unsigned i)
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
		for (unsigned i = 0; i < area; i++)
			help_bits[i] = (*it)[i];
	}
}

float* bmphandler::getstack(unsigned i, unsigned char& mode)
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
		return 0;
	}
}

void bmphandler::popstack_bmp()
{
	if (!bits_stack.empty())
	{
		sliceprovide->take_back(bmp_bits);
		bmp_bits = bits_stack.back();
		mode1 = mode_stack.back();
		bits_stack.pop_back();
		stackindex.pop_back();
		mode_stack.pop_back();
	}
}

void bmphandler::popstack_work()
{
	if (!bits_stack.empty())
	{
		sliceprovide->take_back(work_bits);
		work_bits = bits_stack.back();
		mode2 = mode_stack.back();
		bits_stack.pop_back();
		stackindex.pop_back();
		mode_stack.pop_back();
	}
}

void bmphandler::popstack_help()
{
	if (!bits_stack.empty())
	{
		sliceprovide->take_back(help_bits);
		help_bits = bits_stack.back();
		bits_stack.pop_back();
		stackindex.pop_back();
		mode_stack.pop_back();
	}
}

bool bmphandler::isloaded() { return loaded; }

void bmphandler::clear_tissue(tissuelayers_size_t idx)
{
	tissues_size_t* tissues = tissuelayers[idx];
	std::fill(tissues, tissues + area, 0);
}

bool bmphandler::has_tissue(tissuelayers_size_t idx, tissues_size_t tissuetype)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		if (tissues[i] == tissuetype)
		{
			return true;
		}
	}
	return false;
}

void bmphandler::add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype,
		float f, bool override)
{
	tissues_size_t* tissues = tissuelayers[idx];
	if (override)
	{
		for (unsigned int i = 0; i < area; i++)
			if (work_bits[i] == f &&
					TissueInfos::GetTissueLocked(tissues[i]) == false)
			{
				tissues[i] = tissuetype;
			}
	}
	else
	{
		for (unsigned int i = 0; i < area; i++)
			if (work_bits[i] == f && tissues[i] == 0)
			{
				tissues[i] = tissuetype;
			}
		//for(unsigned int i=0;i<area;i++) if(work_bits[i]==f&&tissuelocked[tissues[i]]==false) {tissues[i]=tissuetype;}
	}
}

void bmphandler::add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype,
		bool* mask, bool override)
{
	tissues_size_t* tissues = tissuelayers[idx];
	if (override)
	{
		for (unsigned int i = 0; i < area; i++)
			if (mask[i] && TissueInfos::GetTissueLocked(tissues[i]) == false)
			{
				tissues[i] = tissuetype;
			}
	}
	else
	{
		for (unsigned int i = 0; i < area; i++)
			if (mask[i] && tissues[i] == 0)
			{
				tissues[i] = tissuetype;
			}
		//for(unsigned int i=0;i<area;i++) if(work_bits[i]==f&&tissuelocked[tissues[i]]==false) {tissues[i]=tissuetype;}
	}
}

void bmphandler::add2tissue_connected(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p, bool override)
{
	unsigned position = pt2coord(p);
	float f = work_bits[position];
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i1] == f && (tissues[i1] == 0 || (override && !TissueInfos::GetTissueLocked(tissues[i1]))))
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	std::vector<int> s;
	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	if (results[s.back()] == -1)
		results[s.back()] = 255.0f;

	int w = width + 2;

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

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

void bmphandler::add2tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, Point p, bool override)
{
	float f = work_pt(p);
	tissues_size_t* tissues = tissuelayers[idx];
	if (override)
	{
		for (unsigned int i = 0; i < area; i++)
			if (work_bits[i] == f && !TissueInfos::GetTissueLocked(tissues[i]))
				tissues[i] = tissuetype;
	}
	else
	{
		for (unsigned int i = 0; i < area; i++)
			if (work_bits[i] == f && tissues[i] == 0)
				tissues[i] = tissuetype;
	}
}

void bmphandler::add2tissue_thresh(tissuelayers_size_t idx,
		tissues_size_t tissuetype, Point p)
{
	float f = work_pt(p);
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
		if (work_bits[i] >= f)
			tissues[i] = tissuetype;
}

void bmphandler::subtract_tissue(tissuelayers_size_t idx,
		tissues_size_t tissuetype, Point p)
{
	float f = work_pt(p);
	subtract_tissue(idx, tissuetype, f);
}

void bmphandler::subtract_tissue_connected(tissuelayers_size_t idx,
		tissues_size_t tissuetype, Point p)
{
	unsigned position = pt2coord(p);
	std::vector<int> s;

	float f = work_bits[position];
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i1] == f && tissues[i1] == tissuetype)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	if (results[s.back()] == -1)
		results[s.back()] = 255.0f;

	int w = width + 2;

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

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

void bmphandler::subtract_tissue(tissuelayers_size_t idx, tissues_size_t tissuetype, float f)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
		if (work_bits[i] == f && tissues[i] == tissuetype)
			tissues[i] = 0;
}

void bmphandler::change2mask_connectedwork(bool* mask, Point p, bool addorsub)
{
	unsigned position = pt2coord(p);
	std::vector<int> s;

	float f = work_bits[position];
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[i1] == f)
				results[i] = -1;
			else
				results[i] = 0;
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	if (results[s.back()] == -1)
		results[s.back()] = 255.0f;

	int w = width + 2;

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

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

void bmphandler::change2mask_connectedtissue(tissuelayers_size_t idx, bool* mask, Point p, bool addorsub)
{
	unsigned position = pt2coord(p);
	std::vector<int> s;

	tissues_size_t* tissues = tissuelayers[idx];
	tissues_size_t f = tissues[position];
	float* results = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

	for (int j = 0; j < width + 2; j++)
		results[j] = results[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	if (results[s.back()] == -1)
		results[s.back()] = 255.0f;

	int w = width + 2;

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

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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

void bmphandler::tissue2work(tissuelayers_size_t idx, const std::vector<float>& mask)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		work_bits[i] = mask.at(tissues[i]);
	}

	mode2 = 2;
}

void bmphandler::tissue2work(tissuelayers_size_t idx)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		work_bits[i] = (float)tissues[i];
	}

	mode2 = 2;
}

void bmphandler::cleartissue(tissuelayers_size_t idx, tissues_size_t tissuetype)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		if (tissues[i] == tissuetype)
			tissues[i] = 0;
	}
}

void bmphandler::cap_tissue(tissues_size_t maxval)
{
	for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned int i = 0; i < area; i++)
		{
			if (tissues[i] > maxval)
				tissues[i] = 0;
		}
	}
}

void bmphandler::cleartissues(tissuelayers_size_t idx)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		tissues[i] = 0;
	}
}

void bmphandler::cleartissuesall()
{
	for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned int i = 0; i < area; i++)
		{
			tissues[i] = 0;
		}
	}
}

void bmphandler::erasework(bool* mask)
{
	for (unsigned int i = 0; i < area; i++)
	{
		if (mask[i])
			work_bits[i] = 0;
	}
}

void bmphandler::erasetissue(tissuelayers_size_t idx, bool* mask)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int i = 0; i < area; i++)
	{
		if (mask[i] && (!TissueInfos::GetTissueLocked(tissues[i])))
			tissues[i] = 0;
	}
}

void bmphandler::floodwork(bool* mask)
{
	unsigned position;
	std::queue<unsigned int> s;

	float* values = (float*)malloc(sizeof(float) * (area + 2 * width + 2 * height + 4));
	bool* bigmask = (bool*)malloc(sizeof(bool) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			values[i] = work_bits[i1];
			bigmask[i] = mask[i1];
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		bigmask[j] = bigmask[j + ((unsigned)width + 2) * (height + 1)] = false;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		bigmask[j] = bigmask[j + width + 1] = false;

	int w = width + 2;

	position = width + 3;
	for (int j = 1; j < height; j++)
	{
		for (int k = 1; k < width; k++)
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

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (mask[i2])
				work_bits[i2] = values[i];
			i++;
			i2++;
		}
		i += 2;
	}

	free(values);
	free(bigmask);
	return;
}

void bmphandler::floodtissue(tissuelayers_size_t idx, bool* mask)
{
	unsigned position;
	std::queue<unsigned int> s;

	tissues_size_t* values = (tissues_size_t*)malloc(sizeof(tissues_size_t) * (area + 2 * width + 2 * height + 4));
	bool* bigmask = (bool*)malloc(sizeof(bool) * (area + 2 * width + 2 * height + 4));

	int i = width + 3;
	int i1 = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			values[i] = tissues[i1];
			bigmask[i] = mask[i1];
			i++;
			i1++;
		}

		i += 2;
	}

	for (int j = 0; j < width + 2; j++)
		bigmask[j] = bigmask[j + ((unsigned)width + 2) * (height + 1)] = false;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		bigmask[j] = bigmask[j + width + 1] = false;

	int w = width + 2;

	position = width + 3;
	for (int j = 1; j < height; j++)
	{
		for (int k = 1; k < width; k++)
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

	i = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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
	return;
}

void bmphandler::correct_outline(float f, std::vector<Point>* newline)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;

	std::vector<std::vector<Point>> vvPouter, vvPinner;
	std::vector<Point> limit1, limit2;
	std::vector<Point>::iterator it, itold, startP, endP;
	std::vector<std::vector<Point>>::iterator vvPit;

	get_contours(f, &vvPouter, &vvPinner, 1);

	vvPouter.insert(vvPouter.end(), vvPinner.begin(), vvPinner.end());

	ImageForestingTransformDistance IFTdist;
	IFTdist.distance_init(width, height, f, work_bits);
	IFTdist.return_path(*(newline->begin()), &limit1);

	it = newline->end();
	it--;
	IFTdist.return_path(*it, &limit2);

	int counter1, counter2;

	Point p1, p2;
	it = limit1.end();
	it--;
	p1 = *it;
	it = limit2.end();
	it--;
	p2 = *it;

	bool found = false;
	vvPit = vvPouter.begin();
	while (!found && vvPit != vvPouter.end())
	{
		it = vvPit->begin();
		counter1 = 0;
		while (!found && it != vvPit->end())
		{
			//			if(it->px==(limit1.begin())->px&&it->py==(limit1.begin())->py){
			if (p1.px == it->px && p1.py == it->py)
			{
				found = true;
				startP = it;
				counter1--;
			}
			it++;
			counter1++;
		}
		vvPit++;
	}

	if (found)
	{
		vvPit--;
		found = false;
		it = vvPit->begin();
		counter2 = 0;
		while (!found && it != vvPit->end())
		{
			//			if(it->px==(limit2.begin())->px&&it->py==(limit2.begin())->py){
			if (p2.px == it->px && p2.py == it->py)
			{
				endP = it;
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
				it = startP;
				startP = endP;
				endP = it;
				order = false;
			}

			std::vector<Point> oldline, oldline1;
			if (counter2 - counter1 + 1 <
					(int)vvPit->size() - counter2 + counter1)
			{
				oldline.insert(oldline.begin(), startP, ++endP);
				oldline1.insert(oldline1.begin(), startP, endP);
			}
			else
			{
				order = !order;
				oldline.insert(oldline.begin(), endP, vvPit->end());
				oldline.insert(oldline.end(), vvPit->begin(), ++startP);
				oldline1.insert(oldline1.begin(), endP, vvPit->end());
				oldline1.insert(oldline1.end(), vvPit->begin(), startP);
			}

			oldline.insert(oldline.end(), newline->begin(), newline->end());
			oldline.insert(oldline.end(), limit1.begin(), limit1.end());
			oldline.insert(oldline.end(), limit2.begin(), limit2.end());

			std::vector<Point> changePts;
			Point p;
			bool in, in1;
			it = newline->begin();
			in = (work_bits[unsigned(width) * it->py + it->px] == f);
			in1 = !in;
			itold = it;
			it++;

			//			FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");

			while (it != newline->end())
			{
				if ((work_bits[unsigned(width) * it->py + it->px] == f) != in)
				{
					if (in)
					{
						changePts.push_back(*itold);
						//						fprintf(fp3,"*%i %i\n",(int)itold->px,(int)itold->py);
					}
					else
					{
						changePts.push_back(*it);
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
					changePts.push_back(p);
					p.px = itold->px;
					p.py = it->py;
					changePts.push_back(p);

					in = !in;
				}
				itold++;
				it++;
			}
			p.px = width;
			p.py = height;
			changePts.push_back(p);
			changePts.push_back(p);
			changePts.push_back(p);

			float* bkp = work_bits;
			work_bits = sliceprovide->give_me();

			fill_contour(&oldline, true);

			for (unsigned i = 0; i < area; i++)
			{
				if (work_bits[i] == 255.0f)
				{
					if (bkp[i] == f)
						bkp[i] = 0.0;
					else
						bkp[i] = f;
				}
			}

			for (it = newline->begin(); it != newline->end(); it++)
				bkp[unsigned(width) * it->py + it->px] = f;

			//			it=newline->begin();
			//			in=(work_bits[unsigned(width)*it->py+it->px]!=f);
			Point p1, p2, p3;
			it = changePts.begin();
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
						bkp[unsigned(width) * it1->py + it1->px] = f;
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
						bkp[unsigned(width) * it1->py + it1->px] = f;
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
						bkp[unsigned(width) * it1->py + it1->px] = f;
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
						bkp[unsigned(width) * it1->py + it1->px] = f;
				}
			}

			//			fclose(fp3);

			sliceprovide->take_back(work_bits);
			work_bits = bkp;
		}
	}

	mode1 = dummymode1;
	mode2 = dummymode2;
}

void bmphandler::correct_outlinetissue(tissuelayers_size_t idx,
		tissues_size_t f1,
		std::vector<Point>* newline)
{
	unsigned char dummymode1 = mode1;
	unsigned char dummymode2 = mode2;
	float f = float(f1);

	pushstack_work();
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned ineu = 0; ineu < area; ineu++)
		work_bits[ineu] = (float)tissues[ineu];

	std::vector<std::vector<Point>> vvPouter, vvPinner;
	std::vector<Point> limit1, limit2;
	std::vector<Point>::iterator it, itold, startP, endP;
	std::vector<std::vector<Point>>::iterator vvPit;

	get_contours(f, &vvPouter, &vvPinner, 1);

	vvPouter.insert(vvPouter.end(), vvPinner.begin(), vvPinner.end());

	ImageForestingTransformDistance IFTdist;
	IFTdist.distance_init(width, height, f, work_bits);
	IFTdist.return_path(*(newline->begin()), &limit1);

	it = newline->end();
	it--;
	IFTdist.return_path(*it, &limit2);

	int counter1, counter2;

	Point p1, p2;
	it = limit1.end();
	it--;
	p1 = *it;
	it = limit2.end();
	it--;
	p2 = *it;

	bool found = false;
	vvPit = vvPouter.begin();
	while (!found && vvPit != vvPouter.end())
	{
		it = vvPit->begin();
		counter1 = 0;
		while (!found && it != vvPit->end())
		{
			//			if(it->px==(limit1.begin())->px&&it->py==(limit1.begin())->py){
			if (p1.px == it->px && p1.py == it->py)
			{
				found = true;
				startP = it;
				counter1--;
			}
			it++;
			counter1++;
		}
		vvPit++;
	}

	if (found)
	{
		vvPit--;
		found = false;
		it = vvPit->begin();
		counter2 = 0;
		while (!found && it != vvPit->end())
		{
			//			if(it->px==(limit2.begin())->px&&it->py==(limit2.begin())->py){
			if (p2.px == it->px && p2.py == it->py)
			{
				endP = it;
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
				it = startP;
				startP = endP;
				endP = it;
				order = false;
			}

			std::vector<Point> oldline, oldline1;
			if (counter2 - counter1 + 1 <
					(int)vvPit->size() - counter2 + counter1)
			{
				oldline.insert(oldline.begin(), startP, ++endP);
				oldline1.insert(oldline1.begin(), startP, endP);
			}
			else
			{
				order = !order;
				oldline.insert(oldline.begin(), endP, vvPit->end());
				oldline.insert(oldline.end(), vvPit->begin(), ++startP);
				oldline1.insert(oldline1.begin(), endP, vvPit->end());
				oldline1.insert(oldline1.end(), vvPit->begin(), startP);
			}

			oldline.insert(oldline.end(), newline->begin(), newline->end());
			oldline.insert(oldline.end(), limit1.begin(), limit1.end());
			oldline.insert(oldline.end(), limit2.begin(), limit2.end());

			std::vector<Point> changePts;
			Point p;
			bool in, in1;
			it = newline->begin();
			in = (work_bits[unsigned(width) * it->py + it->px] == f);
			in1 = !in;
			itold = it;
			it++;

			//			FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");

			while (it != newline->end())
			{
				if ((work_bits[unsigned(width) * it->py + it->px] == f) != in)
				{
					if (in)
					{
						changePts.push_back(*itold);
						//						fprintf(fp3,"*%i %i\n",(int)itold->px,(int)itold->py);
					}
					else
					{
						changePts.push_back(*it);
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
					changePts.push_back(p);
					p.px = itold->px;
					p.py = it->py;
					changePts.push_back(p);

					in = !in;
				}
				itold++;
				it++;
			}
			p.px = width;
			p.py = height;
			changePts.push_back(p);
			changePts.push_back(p);
			changePts.push_back(p);

			float* bkp = work_bits;
			work_bits = sliceprovide->give_me();

			fill_contour(&oldline, true);

			for (unsigned i = 0; i < area; i++)
			{
				if (work_bits[i] == 255.0f)
				{
					if (bkp[i] == f)
						bkp[i] = 0.0;
					else
						bkp[i] = f;
				}
			}

			for (it = newline->begin(); it != newline->end(); it++)
				bkp[unsigned(width) * it->py + it->px] = f;

			//			it=newline->begin();
			//			in=(work_bits[unsigned(width)*it->py+it->px]!=f);
			Point p1, p2, p3;
			it = changePts.begin();
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
						bkp[unsigned(width) * it1->py + it1->px] = f;
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
						bkp[unsigned(width) * it1->py + it1->px] = f;
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
						bkp[unsigned(width) * it1->py + it1->px] = f;
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
						bkp[unsigned(width) * it1->py + it1->px] = f;
				}
			}

			//			fclose(fp3);

			sliceprovide->take_back(work_bits);
			work_bits = bkp;
		}
	}

	for (unsigned ineu = 0; ineu < area; ineu++)
		tissues[ineu] = (tissues_size_t)(work_bits[ineu] + 0.1f);

	popstack_work();

	mode1 = dummymode1;
	mode2 = dummymode2;
}

template<typename T, typename F>
void bmphandler::_brush(T* data, T f, Point p, int radius, bool draw, T f1,
		F is_locked)
{
	unsigned short dist = radius * radius;

	int xmin, xmax, ymin, ymax, d;
	if (p.px < radius)
		xmin = 0;
	else
		xmin = int(p.px - radius);
	if (p.px + radius >= width)
		xmax = int(width - 1);
	else
		xmax = int(p.px + radius);

	for (int x = xmin; x <= xmax; x++)
	{
		d = int(floor(sqrt(float(dist - (x - p.px) * (x - p.px)))));
		ymin = std::max(0, int(p.py) - d);
		ymax = std::min(int(height - 1), d + p.py);

		for (int y = ymin; y <= ymax; y++)
		{
			// don't modify locked pixels
			if (is_locked(data[y * unsigned(width) + x]))
				continue;

			if (draw)
			{
				data[y * unsigned(width) + x] = f;
			}
			else
			{
				if (data[y * unsigned(width) + x] == f)
					data[y * unsigned(width) + x] = f1;
			}
		}
	}
}

template<typename T, typename F>
void bmphandler::_brush(T* data, T f, Point p, float const radius, float dx,
		float dy, bool draw, T f1, F is_locked)
{
	float const radius_corrected = dx > dy
																		 ? std::floor(radius / dx + 0.5f) * dx
																		 : std::floor(radius / dy + 0.5f) * dy;
	float const radius_corrected2 = radius_corrected * radius_corrected;

	int const xradius = std::ceil(radius_corrected / dx);
	int const yradius = std::ceil(radius_corrected / dy);
	for (int x = std::max(0, p.px - xradius); x <= std::min(static_cast<int>(width) - 1, p.px + xradius); x++)
	{
		for (int y = std::max(0, p.py - yradius); y <= std::min(static_cast<int>(height) - 1, p.py + yradius); y++)
		{
			// don't modify locked pixels
			if (is_locked(data[y * unsigned(width) + x]))
				continue;

			if (std::pow(dx * (p.px - x), 2.f) + std::pow(dy * (p.py - y), 2.f) <= radius_corrected2)
			{
				if (draw)
				{
					data[y * unsigned(width) + x] = f;
				}
				else
				{
					if (data[y * unsigned(width) + x] == f)
						data[y * unsigned(width) + x] = f1;
				}
			}
		}
	}
}

void bmphandler::brush(float f, Point p, int radius, bool draw)
{
	_brush(work_bits, f, p, radius, draw, 0.f, [](float v) { return false; });
}

void bmphandler::brush(float f, Point p, float radius, float dx, float dy, bool draw)
{
	_brush(work_bits, f, p, radius, dx, dy, draw, 0.f,
			[](float v) { return false; });
}

void bmphandler::brushtissue(tissuelayers_size_t idx, tissues_size_t f, Point p,
		int radius, bool draw, tissues_size_t f1)
{
	_brush(tissuelayers[idx], f, p, radius, draw, f1,
			[](tissues_size_t v) { return TissueInfos::GetTissueLocked(v); });
}

void bmphandler::brushtissue(tissuelayers_size_t idx, tissues_size_t f, Point p,
		float radius, float dx, float dy, bool draw,
		tissues_size_t f1)
{
	_brush(tissuelayers[idx], f, p, radius, dx, dy, draw, f1,
			[](tissues_size_t v) { return TissueInfos::GetTissueLocked(v); });
}

void bmphandler::fill_holes(float f, int minsize)
{
	std::vector<std::vector<Point>> inner_line;
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	float* tmp_bits =
			(float*)malloc(sizeof(float) * (width + 2) * (height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (width + 2) * (height + 2));
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i++)
		visited[i] = false;

	unsigned pos = width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner; //1 for outer, 7 for inner border
	short direction,
			directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, width + 3, width + 2, width + 1,
			-1, -width - 3, -width - 2, -width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	for (unsigned i = 0; i < height; i++)
	{
		for (unsigned j = 0; j < width; j++)
		{
			tmp_bits[pos] = work_bits[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(width + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = unsigned(width + 2) * (height + 1);
			 i < unsigned(width + 2) * (height + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvisited;
	for (unsigned i = width + 1; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvisited;

	pos = width + 2;
	while (pos < unsigned(width + 2) * (height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(width + 2) * (height + 1))
			pos++;

		if (pos < unsigned(width + 2) * (height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (width + 2) - 1);
			p.py = short(pos / (width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + width + 3] != f &&
					tmp_bits[pos + width + 2] != f &&
					tmp_bits[pos + width + 1] != f &&
					tmp_bits[pos - width - 1] != f &&
					tmp_bits[pos - width - 2] != f &&
					tmp_bits[pos - width - 3] != f)
			{
				visited[pos] = true;
			}
			else
			{
				if (tmp_bits[pos - width - 3] == f)
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
					p.px = short(pos1 % (width + 2) - 1);
					p.py = short(pos1 / (width + 2) - 1);
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

	int i4 = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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
			tmp_bits[it.px + 1 + (it.py + 1) * (width + 2)] = 1;
		}
	}

	for (int j = 0; j < width + 2; j++)
		tmp_bits[j] = tmp_bits[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		tmp_bits[j] = tmp_bits[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}

	hysteretic_growth(tmp_bits, &s, width + 2, height + 2, false, 255.0f);

	i4 = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (tmp_bits[i4] == 0)
				work_bits[i2] = f;
			//			work_bits[i2]=tmp_bits[i4];
			i4++;
			i2++;
		}

		i4 += 2;
	}

	free(tmp_bits);

	return;
}

void bmphandler::fill_holestissue(tissuelayers_size_t idx, tissues_size_t f,
		int minsize)
{
	std::vector<std::vector<Point>> inner_line;
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	tissues_size_t* tmp_bits = (tissues_size_t*)malloc(
			sizeof(tissues_size_t) * (width + 2) * (height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (width + 2) * (height + 2));
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i++)
		visited[i] = false;

	unsigned pos = width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner; //1 for outer, 7 for inner border
	short direction,
			directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, width + 3, width + 2, width + 1,
			-1, -width - 3, -width - 2, -width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned i = 0; i < height; i++)
	{
		for (unsigned j = 0; j < width; j++)
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
	for (unsigned i = 0; i < unsigned(width + 2); i++)
		tmp_bits[i] = unvis;
	for (unsigned i = unsigned(width + 2) * (height + 1);
			 i < unsigned(width + 2) * (height + 2); i++)
		tmp_bits[i] = unvis;
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvis;
	for (unsigned i = width + 1; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvis;

	pos = width + 2;
	while (pos < unsigned(width + 2) * (height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(width + 2) * (height + 1))
			pos++;

		if (pos < unsigned(width + 2) * (height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (width + 2) - 1);
			p.py = short(pos / (width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + width + 3] != f &&
					tmp_bits[pos + width + 2] != f &&
					tmp_bits[pos + width + 1] != f &&
					tmp_bits[pos - width - 1] != f &&
					tmp_bits[pos - width - 2] != f &&
					tmp_bits[pos - width - 3] != f)
			{
				visited[pos] = true;
			}
			else
			{
				if (tmp_bits[pos - width - 3] == f)
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
					p.px = short(pos1 % (width + 2) - 1);
					p.py = short(pos1 / (width + 2) - 1);
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

	int i4 = width + 3;
	//int i1=0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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
			tmp_bits[it->px + 1 + (it->py + 1) * (width + 2)] = 1;
		}
	}

	for (int j = 0; j < width + 2; j++)
		tmp_bits[j] = tmp_bits[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		tmp_bits[j] = tmp_bits[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
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
		if (tmp_bits[i4 - width - 2] == 2)
		{
			tmp_bits[i4 - width - 2] = TISSUES_SIZE_MAX;
			s.push_back(i4 - width - 2);
		}
		if (tmp_bits[i4 + width + 2] == 2)
		{
			tmp_bits[i4 + width + 2] = TISSUES_SIZE_MAX;
			s.push_back(i4 + width + 2);
		}
	}

	i4 = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (tmp_bits[i4] == 2)
				tissues[i2] = f;
			i4++;
			i2++;
		}

		i4 += 2;
	}

	free(tmp_bits);

	return;
}

void bmphandler::remove_islands(float f, int minsize)
{
	std::vector<std::vector<Point>> outer_line;
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	float* tmp_bits =
			(float*)malloc(sizeof(float) * (width + 2) * (height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (width + 2) * (height + 2));
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i++)
		visited[i] = false;

	unsigned pos = width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner; //1 for outer, 7 for inner border
	short direction,
			directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, width + 3, width + 2, width + 1,
			-1, -width - 3, -width - 2, -width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	for (unsigned i = 0; i < height; i++)
	{
		for (unsigned j = 0; j < width; j++)
		{
			tmp_bits[pos] = work_bits[pos1];
			pos++;
			pos1++;
		}
		pos += 2;
	}

	for (unsigned i = 0; i < unsigned(width + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = unsigned(width + 2) * (height + 1);
			 i < unsigned(width + 2) * (height + 2); i++)
		tmp_bits[i] = unvisited;
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvisited;
	for (unsigned i = width + 1; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvisited;

	pos = width + 2;
	while (pos < unsigned(width + 2) * (height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(width + 2) * (height + 1))
			pos++;

		if (pos < unsigned(width + 2) * (height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (width + 2) - 1);
			p.py = short(pos / (width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + width + 3] != f &&
					tmp_bits[pos + width + 2] != f &&
					tmp_bits[pos + width + 1] != f &&
					tmp_bits[pos - width - 1] != f &&
					tmp_bits[pos - width - 2] != f &&
					tmp_bits[pos - width - 3] != f)
			{
				visited[pos] = true;
				vec_pt.push_back(p);
				if (1 < minsize)
					outer_line.push_back(vec_pt);
			}
			else
			{
				if (tmp_bits[pos - width - 3] == f)
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
					p.px = short(pos1 % (width + 2) - 1);
					p.py = short(pos1 / (width + 2) - 1);
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

	int i4 = width + 3;
	int i1 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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
			tmp_bits[it->px + 1 + (it->py + 1) * (width + 2)] = 1;
		}
	}

	for (int j = 0; j < width + 2; j++)
		tmp_bits[j] = tmp_bits[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		tmp_bits[j] = tmp_bits[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
	{
		if (tmp_bits[j] == -1)
		{
			tmp_bits[j] = 255.0f;
			s.push_back(j);
		}
	}

	hysteretic_growth(tmp_bits, &s, width + 2, height + 2, false, 255.0f);

	i4 = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (tmp_bits[i4] == 0)
				work_bits[i2] = 0;
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
			work_bits[it->px + it->py * width] = 0;
		}
	}

	free(tmp_bits);

	return;
}

void bmphandler::remove_islandstissue(tissuelayers_size_t idx, tissues_size_t f,
		int minsize)
{
	std::vector<std::vector<Point>> outer_line;
	minsize = 2 * minsize;
	float bubble_size;
	float linelength;
	short directionchange;

	tissues_size_t* tmp_bits = (tissues_size_t*)malloc(
			sizeof(tissues_size_t) * (width + 2) * (height + 2));
	bool* visited = (bool*)malloc(sizeof(bool) * (width + 2) * (height + 2));
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2); i++)
		visited[i] = false;

	unsigned pos = width + 3;
	unsigned pos1 = 0;
	unsigned pos2;
	unsigned possecond;
	bool done;
	short inner; //1 for outer, 7 for inner border
	short direction,
			directionold; // 0:rechts, 1:rechts oben, 2:oben, ... 7:rechts unten.
	Point p;

	std::vector<Point> vec_pt;
	int offset[8] = {1, width + 3, width + 2, width + 1,
			-1, -width - 3, -width - 2, -width - 1};
	float dy[8] = {0, 1, 1, 1, 0, -1, -1, -1};
	float dx[8] = {1, 1, 0, -1, -1, -1, 0, 1};
	float bordervolume[8] = {1, 0.75f, 0.5f, 0.25f, 2, 1.75f, 1.5f, 1.25f};

	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned i = 0; i < height; i++)
	{
		for (unsigned j = 0; j < width; j++)
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
	for (unsigned i = 0; i < unsigned(width + 2); i++)
		tmp_bits[i] = unvis;
	for (unsigned i = unsigned(width + 2) * (height + 1);
			 i < unsigned(width + 2) * (height + 2); i++)
		tmp_bits[i] = unvis;
	for (unsigned i = 0; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvis;
	for (unsigned i = width + 1; i < unsigned(width + 2) * (height + 2);
			 i += (width + 2))
		tmp_bits[i] = unvis;

	pos = width + 2;
	while (pos < unsigned(width + 2) * (height + 1))
	{
		while ((tmp_bits[pos] != f || tmp_bits[pos - 1] == f || visited[pos]) &&
					 pos < unsigned(width + 2) * (height + 1))
			pos++;

		if (pos < unsigned(width + 2) * (height + 1))
		{
			pos1 = pos;
			vec_pt.clear();
			p.px = short(pos % (width + 2) - 1);
			p.py = short(pos / (width + 2) - 1);
			//			vec_pt.push_back(p);

			if (tmp_bits[pos + 1] != f && tmp_bits[pos + width + 3] != f &&
					tmp_bits[pos + width + 2] != f &&
					tmp_bits[pos + width + 1] != f &&
					tmp_bits[pos - width - 1] != f &&
					tmp_bits[pos - width - 2] != f &&
					tmp_bits[pos - width - 3] != f)
			{
				visited[pos] = true;
				vec_pt.push_back(p);
				if (1 < minsize)
					outer_line.push_back(vec_pt);
			}
			else
			{
				if (tmp_bits[pos - width - 3] == f)
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
					p.px = short(pos1 % (width + 2) - 1);
					p.py = short(pos1 / (width + 2) - 1);
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

	int i4 = width + 3;
	//int i1=0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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
			tmp_bits[it->px + 1 + (it->py + 1) * (width + 2)] = 1;
		}
	}

	for (int j = 0; j < width + 2; j++)
		tmp_bits[j] = tmp_bits[j + ((unsigned)width + 2) * (height + 1)] = 0;
	for (int j = 0; j <= ((int)width + 2) * (height + 1); j += width + 2)
		tmp_bits[j] = tmp_bits[j + width + 1] = 0;

	for (int j = width + 3; j < 2 * width + 3; j++)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = area + 2 * height + 1;
			 j < area + width + 2 * height + 1; j++)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = 2 * width + 5; j <= area + 2 * height + 1;
			 j += width + 2)
	{
		if (tmp_bits[j] == 2)
		{
			tmp_bits[j] = TISSUES_SIZE_MAX;
			s.push_back(j);
		}
	}
	for (unsigned int j = 3 * width + 4; j <= area + width + 2 * height;
			 j += width + 2)
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
		if (tmp_bits[i4 - width - 2] == 2)
		{
			tmp_bits[i4 - width - 2] = TISSUES_SIZE_MAX;
			s.push_back(i4 - width - 2);
		}
		if (tmp_bits[i4 + width + 2] == 2)
		{
			tmp_bits[i4 + width + 2] = TISSUES_SIZE_MAX;
			s.push_back(i4 + width + 2);
		}
	}

	i4 = width + 3;
	int i2 = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
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
			tissues[it->px + it->py * width] = 0;
		}
	}

	free(tmp_bits);

	return;
}

/*void bmphandler::add_skin(unsigned i)
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

void bmphandler::add_vm(std::vector<Mark>* vm1)
{
	vvm.push_back(*vm1);
	maxim_store = std::max(maxim_store, vm1->begin()->mark);
}

void bmphandler::clear_vvm()
{
	vvm.clear();
	maxim_store = 1;
}

bool bmphandler::del_vm(Point p, short radius)
{
	short radius2 = radius * radius;
	std::vector<Mark>::iterator it;
	std::vector<std::vector<Mark>>::iterator vit;
	vit = vvm.begin();
	bool found = false;

	while (!found && vit != vvm.end())
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
		if (vvm.size() == 1)
		{
			clear_vvm();
		}
		else
		{
			unsigned maxim1 = vit->begin()->mark;
			std::vector<std::vector<Mark>>::iterator vit1;
			vit1 = vvm.begin();
			while (vit1 != vvm.end())
			{
				vit1++;
			}
			vvm.erase(vit);

			if (maxim_store == maxim1 && !vvm.empty())
			{
				maxim_store = 1;
				for (vit = vvm.begin(); vit != vvm.end(); vit++)
					maxim_store = std::max(maxim_store, vit->begin()->mark);
			}
		}
		return true;
	}
	else
		return false;
}

std::vector<std::vector<Mark>>* bmphandler::return_vvm() { return &vvm; }

unsigned bmphandler::return_vvmmaxim() { return maxim_store; }

void bmphandler::copy2vvm(std::vector<std::vector<Mark>>* vvm1)
{
	vvm = *vvm1;
}

void bmphandler::add_limit(std::vector<Point>* vp1)
{
	limits.push_back(*vp1);
}

void bmphandler::clear_limits()
{
	limits.clear();
}

bool bmphandler::del_limit(Point p, short radius)
{
	short radius2 = radius * radius;
	std::vector<Point>::iterator it;
	std::vector<std::vector<Point>>::iterator vit;
	vit = limits.begin();
	bool found = false;

	while (!found && vit != limits.end())
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
		if (limits.size() == 1)
		{
			clear_limits();
		}
		else
		{
			std::vector<std::vector<Point>>::iterator vit1;
			vit1 = limits.begin();
			while (vit1 != limits.end())
			{
				vit1++;
			}
			limits.erase(vit);
		}
		return true;
	}
	else
		return false;
}

std::vector<std::vector<Point>>* bmphandler::return_limits() { return &limits; }

void bmphandler::copy2limits(std::vector<std::vector<Point>>* limits1)
{
	limits = *limits1;
}

void bmphandler::map_tissue_indices(const std::vector<tissues_size_t>& indexMap)
{
	for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned int i = 0; i < area; ++i)
		{
			tissues[i] = indexMap[tissues[i]];
		}
	}
}

void bmphandler::remove_tissue(tissues_size_t tissuenr)
{
	for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
	{
		tissues_size_t* tissues = tissuelayers[idx];
		for (unsigned int i = 0; i < area; ++i)
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

void bmphandler::group_tissues(tissuelayers_size_t idx,
		std::vector<tissues_size_t>& olds,
		std::vector<tissues_size_t>& news)
{
	tissues_size_t crossref[TISSUES_SIZE_MAX + 1];
	for (int i = 0; i < TISSUES_SIZE_MAX + 1; i++)
		crossref[i] = (tissues_size_t)i;
	unsigned count = std::min(olds.size(), news.size());

	for (unsigned int i = 0; i < count; i++)
		crossref[olds[i]] = news[i];

	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned int j = 0; j < area; j++)
	{
		tissues[j] = crossref[tissues[j]];
	}
}

unsigned char bmphandler::return_mode(bool bmporwork)
{
	if (bmporwork)
		return mode1;
	else
		return mode2;
}

void bmphandler::set_mode(unsigned char mode, bool bmporwork)
{
	if (bmporwork)
		mode1 = mode;
	else
		mode2 = mode;
}

bool bmphandler::print_amascii_slice(tissuelayers_size_t idx,
		std::ofstream& streamname)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned i = 0; i < area; i++)
	{
		streamname << (int)tissues[i] << " " << std::endl;
	}
	return true;
}

bool bmphandler::print_vtkascii_slice(tissuelayers_size_t idx,
		std::ofstream& streamname)
{
	tissues_size_t* tissues = tissuelayers[idx];
	for (unsigned i = 0; i < area; i++)
	{
		streamname << (int)tissues[i] << " ";
	}
	streamname << std::endl;
	return true;
}

bool bmphandler::print_vtkbinary_slice(tissuelayers_size_t idx,
		std::ofstream& streamname)
{
	tissues_size_t* tissues = tissuelayers[idx];
	if (TissueInfos::GetTissueCount() <= 255)
	{
		if (sizeof(tissues_size_t) == sizeof(unsigned char))
		{
			streamname.write((char*)(tissues), area);
		}
		else
		{
			unsigned char* ucharBuffer = new unsigned char[area];
			for (unsigned int i = 0; i < area; i++)
			{
				ucharBuffer[i] = (unsigned char)tissues[i];
			}
			streamname.write((char*)(ucharBuffer), area);
			delete[] ucharBuffer;
		}
	}
	else
	{
		streamname.write((char*)(tissues), sizeof(tissues_size_t) * area);
	}
	return true;
}

void bmphandler::shifttissue()
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
		long offset = (long)width * y + x;
		for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
		{
			tissues_size_t* tissues = tissuelayers[idx];
			if (x >= 0 && y >= 0)
			{
				unsigned pos = area;
				for (int j = height; j > 0;)
				{
					j--;
					for (int k = width; k > 0;)
					{
						k--;
						pos--;
						if (k + x < width && j + y < height)
							tissues[pos + offset] = tissues[pos];
						tissues[pos] = 0;
					}
				}
			}
			else if (x <= 0 && y <= 0)
			{
				unsigned pos = 0;
				for (int j = 0; j < height; j++)
				{
					for (int k = 0; k < width; k++)
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
				for (int j = 0; j < height; j++)
				{
					for (int k = 0; k < width; k++)
					{
						if (k + x < width && j + y >= 0)
							tissues[pos + offset] = tissues[pos];
						tissues[pos] = 0;
						pos++;
					}
				}
			}
			else if (x < 0 && y > 0)
			{
				unsigned pos = area;
				for (int j = height; j > 0;)
				{
					j--;
					for (int k = width; k > 0;)
					{
						k--;
						pos--;
						if (k + x >= 0 && j + y < height)
							tissues[pos + offset] = tissues[pos];
						tissues[pos] = 0;
					}
				}
			}
		}
	}
}

void bmphandler::shiftbmp()
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
		long offset = (long)width * y + x;
		if (x >= 0 && y >= 0)
		{
			unsigned pos = area;
			for (int j = height; j > 0;)
			{
				j--;
				for (int k = width; k > 0;)
				{
					k--;
					pos--;
					if (k + x < width && j + y < height)
						bmp_bits[pos + offset] = bmp_bits[pos];
					bmp_bits[pos] = 0;
				}
			}
		}
		else if (x <= 0 && y <= 0)
		{
			unsigned pos = 0;
			for (int j = 0; j < height; j++)
			{
				for (int k = 0; k < width; k++)
				{
					if (k + x >= 0 && j + y >= 0)
						bmp_bits[pos + offset] = bmp_bits[pos];
					bmp_bits[pos] = 0;
					pos++;
				}
			}
		}
		else if (x > 0 && y < 0)
		{
			unsigned pos = 0;
			for (int j = 0; j < height; j++)
			{
				for (int k = 0; k < width; k++)
				{
					if (k + x < width && j + y >= 0)
						bmp_bits[pos + offset] = bmp_bits[pos];
					bmp_bits[pos] = 0;
					pos++;
				}
			}
		}
		else if (x < 0 && y > 0)
		{
			unsigned pos = area;
			for (int j = height; j > 0;)
			{
				j--;
				for (int k = width; k > 0;)
				{
					k--;
					pos--;
					if (k + x >= 0 && j + y < height)
						bmp_bits[pos + offset] = bmp_bits[pos];
					bmp_bits[pos] = 0;
				}
			}
		}
	}
}

unsigned long bmphandler::return_workpixelcount(float f)
{
	unsigned long pos = 0;
	unsigned long counter = 0;
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (work_bits[pos] == f)
				counter++;
			pos++;
		}
	}
	return counter;
}

unsigned long bmphandler::return_tissuepixelcount(tissuelayers_size_t idx,
		tissues_size_t c)
{
	unsigned long pos = 0;
	unsigned long counter = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	for (int j = 0; j < height; j++)
	{
		for (int k = 0; k < width; k++)
		{
			if (tissues[pos] == c)
				counter++;
			pos++;
		}
	}
	return counter;
}

bool bmphandler::get_extent(tissuelayers_size_t idx, tissues_size_t tissuenr,
		unsigned short extent[2][2])
{
	if (area == 0)
		return false;

	bool found = false;
	unsigned long pos = 0;
	tissues_size_t* tissues = tissuelayers[idx];
	while (!found && pos < area)
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
		extent[1][0] = pos / width;
		extent[0][0] = pos % width;
	}
	while (pos < (unsigned long)(extent[1][0] + 1) * width)
	{
		if (tissues[pos] == tissuenr)
			extent[0][1] = pos % width;
		pos++;
	}
	found = false;
	pos = area;
	while (!found && pos > 0)
	{
		pos--;
		if (tissues[pos] == tissuenr)
			found = true;
	}
	extent[1][1] = pos / width;
	for (unsigned short i = extent[1][0] + 1; i <= extent[1][1]; i++)
	{
		pos = (unsigned long)(i)*width;
		for (unsigned short j = 0; j < extent[0][0]; j++, pos++)
		{
			if (tissues[pos] == tissuenr)
				extent[0][0] = j;
		}
		pos = (unsigned long)(i + 1) * width - 1;
		for (unsigned short j = width - 1; j > extent[0][1]; j--, pos--)
		{
			if (tissues[pos] == tissuenr)
				extent[0][1] = j;
		}
	}

	return true;
}

void bmphandler::swap(bmphandler& bmph)
{
	Contour contourd;
	contourd = contour;
	contour = bmph.contour;
	bmph.contour = contourd;
	std::vector<Mark> marksd;
	marksd = marks;
	marks = bmph.marks;
	bmph.marks = marksd;
	short unsigned widthd;
	widthd = width;
	width = bmph.width;
	bmph.width = widthd;
	short unsigned heightd;
	heightd = height;
	height = bmph.height;
	bmph.height = heightd;
	unsigned int aread;
	aread = area;
	area = bmph.area;
	bmph.area = aread;
	float* bmp_bitsd;
	bmp_bitsd = bmp_bits;
	bmp_bits = bmph.bmp_bits;
	bmph.bmp_bits = bmp_bitsd;
	float* work_bitsd;
	work_bitsd = work_bits;
	work_bits = bmph.work_bits;
	bmph.work_bits = work_bitsd;
	float* help_bitsd;
	help_bitsd = help_bits;
	help_bits = bmph.help_bits;
	bmph.help_bits = help_bitsd;
	tissues_size_t* tissuesd;
	for (tissuelayers_size_t idx = 0; idx < tissuelayers.size(); ++idx)
	{
		tissuesd = tissuelayers[idx];
		tissuelayers[idx] = bmph.tissuelayers[idx];
		bmph.tissuelayers[idx] = tissuesd;
	}
	wshed_obj wshedobjd;
	wshedobjd = wshedobj;
	wshedobj = bmph.wshedobj;
	bmph.wshedobj = wshedobjd;
	bool bmp_is_greyd;
	bmp_is_greyd = bmp_is_grey;
	bmp_is_grey = bmph.bmp_is_grey;
	bmph.bmp_is_grey = bmp_is_greyd;
	bool work_is_greyd;
	work_is_greyd = work_is_grey;
	work_is_grey = bmph.work_is_grey;
	bmph.work_is_grey = work_is_greyd;
	bool loadedd;
	loadedd = loaded;
	loaded = bmph.loaded;
	bmph.loaded = loadedd;
	bool ownsliceproviderd;
	ownsliceproviderd = ownsliceprovider;
	ownsliceprovider = bmph.ownsliceprovider;
	bmph.ownsliceprovider = ownsliceproviderd;
	FeatureExtractor fextractd;
	fextractd = fextract;
	fextract = bmph.fextract;
	bmph.fextract = fextractd;
	std::list<float*> bits_stackd;
	bits_stackd = bits_stack;
	bits_stack = bmph.bits_stack;
	bmph.bits_stack = bits_stackd;
	std::list<unsigned char> mode_stackd;
	mode_stackd = mode_stack;
	mode_stack = bmph.mode_stack;
	bmph.mode_stack = mode_stackd;
	SliceProvider* sliceprovided;
	sliceprovided = sliceprovide;
	sliceprovide = bmph.sliceprovide;
	bmph.sliceprovide = sliceprovided;
	std::vector<std::vector<Mark>> vvmd;
	vvmd = vvm;
	vvm = bmph.vvm;
	bmph.vvm = vvmd;
	unsigned maxim_stored;
	maxim_stored = maxim_store;
	maxim_store = bmph.maxim_store;
	bmph.maxim_store = maxim_stored;
	std::vector<std::vector<Point>> limitsd;
	limitsd = limits;
	limits = bmph.limits;
	bmph.limits = limitsd;
	unsigned char mode1d;
	mode1d = mode1;
	mode1 = bmph.mode1;
	bmph.mode1 = mode1d;
	unsigned char mode2d;
	mode2d = mode2;
	mode2 = bmph.mode2;
	bmph.mode2 = mode2d;
}

bool bmphandler::unwrap(float jumpratio, float range, float shift)
{
	if (range == 0)
	{
		Pair p;
		get_bmprange(&p);
		range = p.high - p.low;
	}
	float jumpabs = jumpratio * range;
	unsigned int pos = 0;
	float shiftbegin = shift;
	for (unsigned short i = 0; i < height; i++)
	{
		if (i != 0)
		{
			if (bmp_bits[pos - width] - bmp_bits[pos] > jumpabs)
				shiftbegin += range;
			else if (bmp_bits[pos] - bmp_bits[pos - width] > jumpabs)
				shiftbegin -= range;
		}
		shift = shiftbegin;
		work_bits[pos] = bmp_bits[pos] + shift;
		for (unsigned short j = 1; j < width; j++)
		{
			if (bmp_bits[pos] - bmp_bits[pos + 1] > jumpabs)
				shift += range;
			else if (bmp_bits[pos + 1] - bmp_bits[pos] > jumpabs)
				shift -= range;
			pos++;
			work_bits[pos] = bmp_bits[pos] + shift;
		}
		pos++;
	}
	mode2 = mode1;
	for (pos = width - 1; pos + width < area; pos += width)
	{
		if (work_bits[pos + width] - work_bits[pos] > jumpabs)
			return false;
		if (work_bits[pos] - work_bits[pos + width] > jumpabs)
			return false;
	}
	return true;
}
