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

#include "SlicesHandler.h"
#include "bmp_read_1.h"
#include "config.h"

#include "AvwReader.h"
#include "ChannelExtractor.h"
#include "DicomReader.h"
#include "LoaderWidgets.h"
#include "Morpho.h"
#include "TestingMacros.h"
#include "TissueHierarchy.h"
#include "TissueInfos.h"
#include "XdmfImageMerger.h"
#include "XdmfImageReader.h"
#include "XdmfImageWriter.h"
#include "vtkEdgeCollapse.h"
#include "vtkGenericDataSetWriter.h"
#include "vtkImageExtractCompatibleMesher.h"

#include "Core/ExpectationMaximization.h"
#include "Core/HDF5Writer.h"
#include "Core/ImageForestingTransform.h"
#include "Core/ImageReader.h"
#include "Core/ImageToITK.h"
#include "Core/ImageWriter.h"
#include "Core/KMeans.h"
#include "Core/MarchingCubes.h"
#include "Core/MatlabExport.h"
#include "Core/MultidimensionalGamma.h"
#include "Core/Outline.h"
#include "Core/ProjectVersion.h"
#include "Core/RTDoseIODModule.h"
#include "Core/RTDoseReader.h"
#include "Core/RTDoseWriter.h"
#include "Core/SliceProvider.h"
#include "Core/SmoothSteps.h"
#include "Core/Transform.h"
#include "Core/Treaps.h"
#include "Core/VoxelSurface.h"

#include "vtkMyGDCMPolyDataReader.h"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDiscreteMarchingCubes.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>

#include <qdir.h>
#include <qfileinfo.h>
#include <qmessagebox.h>
#include <qprogressdialog.h>

#ifndef NO_OPENMP_SUPPORT
#	include <omp.h>
#endif

using namespace iseg;

namespace // PROJECT VERSION NUMBERS
{
// version 0: ?
// version 1: added displacement[3], ...?
// version 2: ?
// version 3: ?
// version 4: added dc[6], ...?
// version 5: removed dc[6] and displacement[3], added transform[4][4]
int const project_version = 5;

// version 0: tissues_size_t=unsigned char
// version 1: tissues_size_t=unsigned short, ...?
int const tissue_version = 1;
} // namespace

struct posit
{
	unsigned pxy;
	unsigned short pz;
	inline bool operator<(const posit& a) const
	{
		if (pz < a.pz || ((pz == a.pz) && (pxy < a.pxy)))
			return true;
		else
			return false;
	}
	inline bool operator==(const posit& a) const
	{
		return (pz == a.pz) && (pxy == a.pxy);
	}
};

SlicesHandler::SlicesHandler()
{
	loaded = false;
	thickness = dx = dy = 1.0f;
	transform.setIdentity();
	uelem = NULL;
	undo3D = true;
	startslice = 0;
	this->Compression = 1;
	this->ContiguousMemoryIO = false; // Default: slice-by-slice
	active_tissuelayer = 0;
	overlay = NULL;
	tissue_hierachy = new TissueHiearchy;

	redFactor = 0;
	greenFactor = 0;
	blueFactor = 0;

	color_lookup_table = nullptr;
}

SlicesHandler::~SlicesHandler() { delete tissue_hierachy; }

float SlicesHandler::get_work_pt(Point p, unsigned short slicenr)
{
	return image_slices[slicenr].work_pt(p);
}

void SlicesHandler::set_work_pt(Point p, unsigned short slicenr, float f)
{
	image_slices[slicenr].set_work_pt(p, f);
}

float SlicesHandler::get_bmp_pt(Point p, unsigned short slicenr)
{
	return image_slices[slicenr].bmp_pt(p);
}

void SlicesHandler::set_bmp_pt(Point p, unsigned short slicenr, float f)
{
	image_slices[slicenr].set_bmp_pt(p, f);
}

tissues_size_t SlicesHandler::get_tissue_pt(Point p, unsigned short slicenr)
{
	return image_slices[slicenr].tissues_pt(active_tissuelayer, p);
}

void SlicesHandler::set_tissue_pt(Point p, unsigned short slicenr,
								  tissues_size_t f)
{
	image_slices[slicenr].set_tissue_pt(active_tissuelayer, p, f);
}

std::vector<const float*> SlicesHandler::get_bmp() const
{
	std::vector<const float*> ptrs(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
	{
		ptrs[i] = image_slices[i].return_bmp();
	}
	return ptrs;
}

std::vector<float*> SlicesHandler::get_bmp()
{
	std::vector<float*> ptrs(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
	{
		ptrs[i] = image_slices[i].return_bmp();
	}
	return ptrs;
}

std::vector<const float*> SlicesHandler::get_work() const
{
	std::vector<const float*> ptrs(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
	{
		ptrs[i] = image_slices[i].return_work();
	}
	return ptrs;
}

std::vector<float*> SlicesHandler::get_work()
{
	std::vector<float*> ptrs(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
	{
		ptrs[i] = image_slices[i].return_work();
	}
	return ptrs;
}

std::vector<const tissues_size_t*>
	SlicesHandler::get_tissues(tissuelayers_size_t layeridx) const
{
	std::vector<const tissues_size_t*> ptrs(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
	{
		ptrs[i] = image_slices[i].return_tissues(layeridx);
	}
	return ptrs;
}

std::vector<tissues_size_t*>
	SlicesHandler::get_tissues(tissuelayers_size_t layeridx)
{
	std::vector<tissues_size_t*> ptrs(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
	{
		ptrs[i] = image_slices[i].return_tissues(layeridx);
	}
	return ptrs;
}

float* SlicesHandler::return_bmp(unsigned short slicenr1)
{
	return image_slices[slicenr1].return_bmp();
}

float* SlicesHandler::return_work(unsigned short slicenr1)
{
	return image_slices[slicenr1].return_work();
}

tissues_size_t* SlicesHandler::return_tissues(tissuelayers_size_t layeridx,
											  unsigned short slicenr1)
{
	return image_slices[slicenr1].return_tissues(layeridx);
}

float* SlicesHandler::return_overlay() { return overlay; }

int SlicesHandler::LoadDIBitmap(const char* filename, unsigned short slicenr,
								unsigned short nrofslices)
{
	UpdateColorLookupTable(nullptr);
	activeslice = 0;
	active_tissuelayer = 0;
	char name[100];
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;

	for (unsigned short i = 0; i < nrofslices; i++)
	{
		//		sprintf(name,"%s%0 3u.bmp",filename,i+slicenr);
		//		fprintf(fp,"%s %u.bmp",filename,i+slicenr);
		sprintf(name, "%s%u.bmp", filename, i + slicenr);

		j += (image_slices[i]).LoadDIBitmap(name);
	}

	width = image_slices[0].return_width();
	height = image_slices[0].return_height();
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	for (unsigned short i = 1; i < nrofslices; i++)
	{
		if (width != image_slices[i].return_width() ||
			height != image_slices[i].return_height())
			j = nrofslices + 1;
	}

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::LoadDIBitmap(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);
	activeslice = 0;
	active_tissuelayer = 0;
	startslice = 0;
	endslice = nrslices = (unsigned short)filenames.size();
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);
	int j = 0;

	if (image_slices[0].CheckBMPDepth(filenames[0]) > 8)
	{
		ChannelMixer channelMixer(filenames, NULL);
		channelMixer.move(QCursor::pos());
		if (!channelMixer.exec())
			//return 0;

			redFactor = channelMixer.GetRedFactor();
		greenFactor = channelMixer.GetGreenFactor();
		blueFactor = channelMixer.GetBlueFactor();

		for (unsigned short i = 0; i < nrslices; i++)
		{
			(image_slices[i])
				.SetConverterFactors(redFactor, greenFactor, blueFactor);
		}
	}

	for (unsigned short i = 0; i < nrslices; i++)
	{
		j += (image_slices[i]).LoadDIBitmap(filenames[i]);
	}

	width = image_slices[0].return_width();
	height = image_slices[0].return_height();
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	for (unsigned short i = 0; i < nrslices; i++)
	{
		if (width != image_slices[i].return_width() ||
			height != image_slices[i].return_height())
			j = nrslices + 1;
	}

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrslices);
	slice_bmpranges.resize(nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrslices);
		return 0;
	}
}

int SlicesHandler::LoadDIBitmap(std::vector<const char*> filenames,
								double refFactor, double blueFactor,
								double greenFactor)
{
	endslice = nrslices = (unsigned short)filenames.size();
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);

	for (unsigned short i = 0; i < nrslices; i++)
	{
		(image_slices[i])
			.SetRGBtoGrayScaleFactors(refFactor, blueFactor, greenFactor);
	}
	return LoadDIBitmap(filenames);
}

int SlicesHandler::LoadDIBitmap(const char* filename, unsigned short slicenr,
								unsigned short nrofslices, Point p,
								unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	char name[100];
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i + slicenr);
		j += (image_slices[i]).LoadDIBitmap(name, p, dx, dy);
	}

	width = dx;
	height = dy;
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::LoadDIBitmap(std::vector<const char*> filenames, Point p,
								unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	nrslices = (unsigned short)filenames.size();
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);
	int j = 0;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		j += (image_slices[i]).LoadDIBitmap(filenames[i], p, dx, dy);
	}

	width = dx;
	height = dy;
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrslices);
	slice_bmpranges.resize(nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrslices);
		return 0;
	}
}

int SlicesHandler::LoadPng(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(
		nullptr); // BL: here we could quantize colors instead and build color

	activeslice = 0;
	active_tissuelayer = 0;
	startslice = 0;
	endslice = nrslices = (unsigned short)filenames.size();
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);
	int j = 0;

	if (image_slices[0].CheckPNGDepth(filenames[0]) > 8)
	{
		ChannelMixer channelMixer(filenames, NULL);
		channelMixer.move(QCursor::pos());
		if (!channelMixer.exec())
			//return 0;

			redFactor = channelMixer.GetRedFactor();
		greenFactor = channelMixer.GetGreenFactor();
		blueFactor = channelMixer.GetBlueFactor();

		for (unsigned short i = 0; i < nrslices; i++)
		{
			(image_slices[i])
				.SetConverterFactors(redFactor, greenFactor, blueFactor);
		}
	}
	else
	{
		redFactor = 33;
		greenFactor = 33;
		blueFactor = 33;

		for (unsigned short i = 0; i < nrslices; i++)
		{
			(image_slices[i])
				.SetConverterFactors(redFactor, greenFactor, blueFactor);
		}
	}

	for (unsigned short i = 0; i < nrslices; i++)
	{
		j += (image_slices[i]).LoadPNGBitmap(filenames[i]);
	}

	width = image_slices[0].return_width();
	height = image_slices[0].return_height();
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	for (unsigned short i = 0; i < nrslices; i++)
	{
		if (width != image_slices[i].return_width() ||
			height != image_slices[i].return_height())
			j = nrslices + 1;
	}

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrslices);
	slice_bmpranges.resize(nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrslices);
		return 0;
	}
}

int SlicesHandler::LoadPng(std::vector<const char*> filenames, Point p,
						   unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	nrslices = (unsigned short)filenames.size();
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);
	int j = 0;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		j += (image_slices[i]).LoadDIBitmap(filenames[i], p, dx, dy);
	}

	width = dx;
	height = dy;
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrslices);
	slice_bmpranges.resize(nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrslices);
		return 0;
	}
}

int SlicesHandler::LoadPng(std::vector<const char*> filenames, double refFactor,
						   double blueFactor, double greenFactor)
{
	UpdateColorLookupTable(nullptr);

	endslice = nrslices = (unsigned short)filenames.size();
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);

	for (unsigned short i = 0; i < nrslices; i++)
	{
		(image_slices[i])
			.SetRGBtoGrayScaleFactors(refFactor, blueFactor, greenFactor);
	}
	return LoadPng(filenames);
}

int SlicesHandler::LoadDIJpg(const char* filename, unsigned short slicenr,
							 unsigned short nrofslices)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	char name[100];
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;

	for (unsigned short i = 0; i < nrofslices; i++)
	{
		//		sprintf(name,"%s%0 3u.bmp",filename,i+slicenr);
		//		fprintf(fp,"%s %u.bmp",filename,i+slicenr);
		sprintf(name, "%s%u.bmp", filename, i + slicenr);

		j += (image_slices[i]).LoadDIBitmap(name);
	}

	width = image_slices[0].return_width();
	height = image_slices[0].return_height();
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	for (unsigned short i = 1; i < nrofslices; i++)
	{
		if (width != image_slices[i].return_width() ||
			height != image_slices[i].return_height())
			j = nrofslices + 1;
	}

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::LoadDIJpg(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	startslice = 0;
	endslice = nrslices = (unsigned short)filenames.size();
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);
	int j = 0;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		j += (image_slices[i]).LoadDIBitmap(filenames[i]);
	}

	width = image_slices[0].return_width();
	height = image_slices[0].return_height();
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	for (unsigned short i = 0; i < nrslices; i++)
	{
		if (width != image_slices[i].return_width() ||
			height != image_slices[i].return_height())
			j = nrslices + 1;
	}

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrslices);
	slice_bmpranges.resize(nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrslices);
		return 0;
	}
}

int SlicesHandler::LoadDIJpg(const char* filename, unsigned short slicenr,
							 unsigned short nrofslices, Point p,
							 unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	char name[100];
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i + slicenr);
		j += (image_slices[i]).LoadDIBitmap(name, p, dx, dy);
	}

	width = dx;
	height = dy;
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::LoadDIJpg(std::vector<const char*> filenames, Point p,
							 unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	nrslices = (unsigned short)filenames.size();
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);
	int j = 0;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		j += (image_slices[i]).LoadDIBitmap(filenames[i], p, dx, dy);
	}

	width = dx;
	height = dy;
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrslices);
	slice_bmpranges.resize(nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrslices);
		return 0;
	}
}

int SlicesHandler::ReadRaw(const char* filename, short unsigned w,
						   short unsigned h, unsigned bitdepth,
						   unsigned short slicenr, unsigned short nrofslices)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	width = w;
	height = h;
	area = height * (unsigned int)width;
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += (image_slices[i]).ReadRaw(filename, w, h, bitdepth, slicenr + i);

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		cerr << "SlicesHandler::ReadRaw() : warning, loading failed..." << endl;
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawOverlay(const char* filename, unsigned bitdepth,
								  unsigned short slicenr)
{
	FILE* fp;			/* Open file pointer */
	int bitsize = area; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == NULL)
		return (NULL);

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;
		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == NULL)
		{
			cerr << "bmphandler::ReadRaw() : error, allocation failed" << endl;
			fclose(fp);
			return (NULL);
		}

		// int result = fseek(fp, (size_t)(bitsize)*slicenr, SEEK_SET);
		// const fpos_t pos = (fpos_t)(bitsize)*slicenr;
		// int result = fsetpos(fp, &pos);

		// Check data size
#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(bitsize)*nrslices - 1, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*nrslices - 1, SEEK_SET);
#endif
		if (result)
		{
			cerr
				<< "bmphandler::ReadRawOverlay() : error, file operation failed"
				<< endl;
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		// Move to slice
#ifdef _MSC_VER
		_fseeki64(fp, (__int64)(bitsize)*slicenr, SEEK_SET);
#else
		fseek(fp, (size_t)(bitsize)*slicenr, SEEK_SET);
#endif

		if (fread(bits_tmp, 1, bitsize, fp) < area)
		{
			cerr
				<< "bmphandler::ReadRawOverlay() : error, file operation failed"
				<< endl;
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		for (int i = 0; i < bitsize; i++)
		{
			overlay[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2)
	{
		unsigned short* bits_tmp;

		if ((bits_tmp = (unsigned short*)malloc(bitsize * 2)) == NULL)
		{
			cerr << "bmphandler::ReadRawOverlay() : error, allocation failed"
				 << endl;
			fclose(fp);
			return (NULL);
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
			cerr
				<< "bmphandler::ReadRawOverlay() : error, file operation failed"
				<< endl;
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		if (fread(bits_tmp, 1, (size_t)(bitsize)*2, fp) < area * 2)
		{
			cerr
				<< "bmphandler::ReadRawOverlay() : error, file operation failed"
				<< endl;
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		for (int i = 0; i < bitsize; i++)
		{
			overlay[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		cerr << "bmphandler::ReadRawOverlay() : error, unsupported depth..."
			 << endl;
		fclose(fp);
		return (NULL);
	}

	fclose(fp);
	return 1;
}

int SlicesHandler::ReadImage(const char* filename)
{
	UpdateColorLookupTable(nullptr);

	unsigned w, h, nrofslices;
	float spacing1[3];
	Transform transform1;
	if (ImageReader::getInfo(filename, w, h, nrofslices, spacing1, transform1))
	{
		newbmp(w, h, nrofslices);
		std::vector<float*> slices(nrslices);
		for (unsigned i = 0; i < nrslices; i++)
		{
			slices[i] = image_slices[i].return_bmp();
		}
		ImageReader::getVolume(filename, slices.data(), nrslices, width,
							   height);
		thickness = spacing1[2];
		dx = spacing1[0];
		dy = spacing1[1];
		transform = transform1;
		loaded = true;

		bmp2workall();
		return 1;
	}
	return 0;
}

int SlicesHandler::ReadOverlay(const char* filename, unsigned short slicenr)
{
	unsigned w, h, nrofslices;
	float spacing1[3];
	Transform transform1;
	if (ImageReader::getInfo(filename, w, h, nrofslices, spacing1, transform1))
	{
		return ImageReader::getVolume(filename, &overlay, slicenr, 1, width,
									  height);
	}
	return 0;
}

int SlicesHandler::ReadRTdose(const char* filename)
{
	UpdateColorLookupTable(nullptr);

	unsigned int dims[3];
	double spacing[3];
	double origin[3];
	double dc[6];

	// Read size data
	if (!RTDoseReader::ReadSizeData(filename, dims, spacing, origin, dc))
	{
		cerr << "ReadRTdose() : ReadSizeData() failed" << endl;
		return 0;
	}

	cerr << "SlicesHandler::ReadRTdose() : Extent = " << dims[0] << " "
		 << dims[1] << " " << dims[2] << endl;
	cerr << "SlicesHandler::ReadRTdose() : Origin = " << origin[0] << " "
		 << origin[1] << " " << origin[2] << endl;
	cerr << "SlicesHandler::ReadRTdose() : Spacing = " << spacing[0] << " "
		 << spacing[1] << " " << spacing[2] << endl;
	cerr << "SlicesHandler::ReadRTdose() : Direction Cosines Row = " << dc[0]
		 << " " << dc[1] << " " << dc[2] << endl;
	cerr << "SlicesHandler::ReadRTdose() : Direction Cosines Column = " << dc[3]
		 << " " << dc[4] << " " << dc[5] << endl;

	// Reallocate slices
	// taken from LoadProject()
	this->activeslice = 0;
	this->width = dims[0];
	this->height = dims[1];
	this->area = height * (unsigned int)width;
	this->nrslices = dims[2];
	this->startslice = 0;
	this->endslice = nrslices;
	this->set_pixelsize(spacing[0], spacing[1]);
	this->thickness = spacing[2];

	// WARNING this might neglect the third column of the "rotation" matrix (e.g. reflections)
	transform.setTransform(origin, dc);

	this->image_slices.resize(nrslices);
	this->os.set_sizenr(nrslices);
	this->set_slicethickness(thickness);
	this->loaded = true;
	for (unsigned short j = 0; j < nrslices; j++)
	{
		this->image_slices[j].newbmp(width, height);
	}

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Pass slice pointers to reader
	std::vector<float*> bmpslices(nrslices);
	for (unsigned i = 0; i < nrslices; i++)
	{
		bmpslices[i] = this->image_slices[i].return_bmp();
	}

	// Read pixel data
	bool res = RTDoseReader::ReadPixelData(filename, bmpslices.data());
	if (res)
	{
		loaded = true;
		bmp2workall();
	}
	else
	{
		cerr << "ReadRTdose() : ReadPixelData() failed" << endl;
		newbmp(width, height, nrslices);
	}

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrslices);
	slice_bmpranges.resize(nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	set_active_tissuelayer(0);

	return res;
}

bool SlicesHandler::LoadSurface(const char* filename,
								const bool overwrite_working)
{
	float spacing[3] = {dx, dy, thickness};
	unsigned dims[3] = {width, height, nrslices};
	auto slices = get_work();

	if (overwrite_working)
	{
		for (unsigned s = startslice; s < endslice; s++)
		{
			auto val = image_slices[s].return_work()[0];
			image_slices[s].clear_work();
		}
	}

	VoxelSurface voxeler;
	auto ret = voxeler.Run(filename, dims, spacing, transform, slices.data(),
						   startslice, endslice);
	return (ret != VoxelSurface::kNone);
}

int SlicesHandler::LoadAllHDF(const char* filename)
{
	unsigned w, h, nrofslices;
	float* pixsize;
	float* tr_1d;
	float offset[3];
	QStringList arrayNames;

	HDFImageReader reader;
	reader.SetFileName(filename);
	reader.SetReadContiguousMemory(GetContiguousMemory());
	reader.ParseHDF();
	w = reader.GetWidth();
	h = reader.GetHeight();
	nrofslices = reader.GetNumberOfSlices();
	pixsize = reader.GetPixelSize();
	tr_1d = reader.GetImageTransform();

	if ((w != width) || (h != height) || (nrslices != nrofslices))
	{
		cerr << "LoadAllHDF() : inconsistent dimensions" << endl;
		return 0;
	}

	for (int r = 0; r < 3; r++)
	{
		offset[r] = tr_1d[r * 4 + 3];
	}
	arrayNames = reader.GetArrayNames();

	cerr << "SlicesHandler::LoadAllHDF() : Extent = " << width << " " << height
		 << " " << nrofslices << endl;
	cerr << "SlicesHandler::LoadAllHDF() : Origin = " << offset[0] << " "
		 << offset[1] << " " << offset[2] << endl;
	cerr << "SlicesHandler::LoadAllHDF() : Spacing = " << pixsize[0] << " "
		 << pixsize[1] << " " << pixsize[2] << endl;
	const int NPA = arrayNames.size();
	cerr << "SlicesHandler::LoadAllHDF() : number of point arrays: " << NPA
		 << endl;
	for (int i = 0; i < NPA; ++i)
	{
		cerr << "\tarray id = " << i
			 << ", name = " << arrayNames[i].toAscii().data() << endl;
	}

	//	if((w!=this->width) || (h!=this->height) || (nrofslices!=this->nrslices))
	//	{
	//		cerr << "error, invalid dimensions..." << endl;
	//		return 0;
	//	}

	// read colors if any
	UpdateColorLookupTable(reader.ReadColorLookup());

	std::vector<float*> bmpslices(nrslices);
	std::vector<float*> workslices(nrslices);
	std::vector<tissues_size_t*> tissueslices(nrslices);
	for (unsigned i = 0; i < nrslices; i++)
	{
		bmpslices[i] = this->image_slices[i].return_bmp();
		workslices[i] = this->image_slices[i].return_work();
		tissueslices[i] = this->image_slices[i].return_tissues(0); // TODO
	}

	reader.SetImageSlices(bmpslices.data());
	reader.SetWorkSlices(workslices.data());
	reader.SetTissueSlices(tissueslices.data());

	return reader.Read();
}

void SlicesHandler::UpdateColorLookupTable(
	std::shared_ptr<ColorLookupTable> new_lut /*= nullptr*/)
{
	color_lookup_table = new_lut;

	// BL: todo update slice viewer
}

int SlicesHandler::LoadAllXdmf(const char* filename)
{
	unsigned w, h, nrofslices;
	QStringList arrayNames;

	XdmfImageReader reader;
	reader.SetFileName(filename);
	reader.SetReadContiguousMemory(GetContiguousMemory());
	reader.ParseXML();
	w = reader.GetWidth();
	h = reader.GetHeight();
	nrofslices = reader.GetNumberOfSlices();
	if ((w != width) || (h != height) || (nrslices != nrofslices))
	{
		cerr << "loadAllXdmf() : inconsistent dimensions" << endl;
		return 0;
	}
	auto pixsize = reader.GetPixelSize();
	auto transform = reader.GetImageTransform();
	float origin[3];
	transform.getOffset(origin);
	arrayNames = reader.GetArrayNames();

	cerr << "SlicesHandler::loadAllXdmf() : Extent = " << width << " " << height
		 << " " << nrofslices << endl;
	cerr << "SlicesHandler::loadAllXdmf() : Origin = " << origin[0] << " "
		 << origin[1] << " " << origin[2] << endl;
	cerr << "SlicesHandler::loadAllXdmf() : Spacing = " << pixsize[0] << " "
		 << pixsize[1] << " " << pixsize[2] << endl;
	const int NPA = arrayNames.size();
	cerr << "SlicesHandler::loadAllXdmf() : number of point arrays: " << NPA
		 << endl;
	for (int i = 0; i < NPA; ++i)
	{
		cerr << "\tarray id = " << i
			 << ", name = " << arrayNames[i].toAscii().data() << endl;
	}

	//	if((w!=this->width) || (h!=this->height) || (nrofslices!=this->nrslices))
	//	{
	//		cerr << "error, invalid dimensions..." << endl;
	//		return 0;
	//	}

	std::vector<float*> bmpslices(nrslices);
	std::vector<float*> workslices(nrslices);
	std::vector<tissues_size_t*> tissueslices(nrslices);

	for (unsigned i = 0; i < nrslices; i++)
	{
		bmpslices[i] = this->image_slices[i].return_bmp();
		workslices[i] = this->image_slices[i].return_work();
		tissueslices[i] = this->image_slices[i].return_tissues(0); // TODO
	}

	UpdateColorLookupTable(reader.ReadColorLookup());

	reader.SetImageSlices(bmpslices.data());
	reader.SetWorkSlices(workslices.data());
	reader.SetTissueSlices(tissueslices.data());
	int code = reader.Read();

	return code;
}

int SlicesHandler::SaveAllXdmf(const char* filename, int compression,
							   bool naked)
{
	float pixsize[3] = {dx, dy, thickness};

	std::vector<float*> bmpslices(endslice - startslice);
	std::vector<float*> workslices(endslice - startslice);
	std::vector<tissues_size_t*> tissueslices(endslice - startslice);
	for (unsigned i = startslice; i < endslice; i++)
	{
		bmpslices[i - startslice] = image_slices[i].return_bmp();
		workslices[i - startslice] = image_slices[i].return_work();
		tissueslices[i - startslice] =
			image_slices[i].return_tissues(0); // TODO
	}

	XdmfImageWriter writer;
	writer.SetCopyToContiguousMemory(GetContiguousMemory());
	writer.SetFileName(filename);
	writer.SetImageSlices(bmpslices.data());
	writer.SetWorkSlices(workslices.data());
	writer.SetTissueSlices(tissueslices.data());
	writer.SetNumberOfSlices(endslice - startslice);
	writer.SetWidth(width);
	writer.SetHeight(height);
	writer.SetPixelSize(pixsize);

	auto active_slices_transform = get_transform_active_slices();

	writer.SetImageTransform(active_slices_transform);
	writer.SetCompression(compression);
	bool ok = writer.Write(naked);
	ok &= writer.WriteColorLookup(color_lookup_table.get(), naked);
	ok &= TissueInfos::SaveTissuesHDF(
		filename, tissue_hierachy->selected_hierarchy(), naked, 0);
	ok &= SaveMarkersHDF(filename, naked, 0);
	return ok;
}

bool SlicesHandler::SaveMarkersHDF(const char* filename, bool naked,
								   unsigned short version)
{
	int compression = 1;

	QString qFileName(filename);
	QFileInfo fileInfo(qFileName);
	QString basename = fileInfo.completeBaseName();
	QString suffix = fileInfo.suffix();

	// save working directory
	QDir oldcwd = QDir::current();
	cerr << "storing current folder " << oldcwd.absolutePath().toAscii().data()
		 << endl;

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());
	cerr << "changing current folder to "
		 << fileInfo.absolutePath().toAscii().data() << endl;

	HDF5Writer writer;
	QString fname;
	if (naked)
		fname = basename + "." + suffix;
	else
		fname = basename + ".h5";

	if (!writer.open(fname.toAscii().data(), "append"))
	{
		cerr << "error opening " << fname.toAscii().data() << endl;
		return false;
	}
	writer.compression = compression;

	if (!writer.createGroup(std::string("Markers")))
	{
		cerr << "error creating markers section" << endl;
		return false;
	}

	int index1[1];
	std::vector<HDF5Writer::size_type> dim2;
	dim2.resize(1);
	dim2[0] = 1;

	index1[0] = (int)version;
	if (!writer.write(index1, dim2, std::string("/Markers/version")))
	{
		cerr << "error writing version" << endl;
		return false;
	}

	float marker_pos[3];
	std::vector<HDF5Writer::size_type> dim1;
	dim1.resize(1);
	dim1[0] = 3;

	int mark_counter = 0;
	for (unsigned i = startslice; i < endslice; i++)
	{
		std::vector<Mark> marks = *(image_slices[i].return_marks());
		for (auto cur_mark : marks)
		{
			QString mark_name = QString::fromStdString(cur_mark.name);
			if (mark_name.isEmpty())
			{
				mark_name = QString::fromStdString(
					"Mark_" + std::to_string(mark_counter));
				mark_counter++;
			}
			mark_name.replace(QString(" "), QString("_"), true);

			std::string mystring =
				std::string("/Markers/") + mark_name.toLocal8Bit().constData();
			writer.createGroup(mystring);

			marker_pos[0] = cur_mark.p.px * get_pixelsize().high;
			marker_pos[1] = cur_mark.p.py * get_pixelsize().low;
			marker_pos[2] = i * get_slicethickness();

			if (!writer.write(marker_pos, dim1,
							  std::string("/Markers/") +
								  mark_name.toLocal8Bit().constData() +
								  std::string("/marker_pos")))
			{
				cerr << "error writing marker_pos" << endl;
				return false;
			}
		}
	}

	writer.close();

	return true;
}

int SlicesHandler::SaveMergeAllXdmf(const char* filename,
									std::vector<QString>& mergeImagefilenames,
									unsigned short nrslicesTotal,
									int compression)
{
	float pixsize[3];
	float offset[3];

	get_displacement(offset);
	offset[2] += thickness * startslice;

	pixsize[0] = dx;
	pixsize[1] = dy;
	pixsize[2] = thickness;

	std::vector<float*> bmpslices(endslice - startslice);
	std::vector<float*> workslices(endslice - startslice);
	std::vector<tissues_size_t*> tissueslices(endslice - startslice);
	for (unsigned i = startslice; i < endslice; i++)
	{
		bmpslices[i - startslice] = image_slices[i].return_bmp();
		workslices[i - startslice] = image_slices[i].return_work();
		tissueslices[i - startslice] =
			image_slices[i].return_tissues(0); // TODO
	}

	XdmfImageMerger merger;
	merger.SetFileName(filename);
	merger.SetImageSlices(bmpslices.data());
	merger.SetWorkSlices(workslices.data());
	merger.SetTissueSlices(tissueslices.data());
	merger.SetNumberOfSlices(endslice - startslice);
	merger.SetTotalNumberOfSlices(nrslicesTotal);
	merger.SetWidth(width);
	merger.SetHeight(height);
	merger.SetPixelSize(pixsize);
	merger.SetOffset(offset);
	merger.SetCompression(compression);
	merger.SetMergeFileNames(mergeImagefilenames);
	if (merger.Write())
	{
		if (auto lut = GetColorLookupTable())
		{
			bool naked = false;
			XdmfImageWriter(filename).WriteColorLookup(lut.get(), naked);
		}
		return 1;
	}

	return 0;
}

int SlicesHandler::ReadAvw(const char* filename)
{
	UpdateColorLookupTable(nullptr);

	unsigned short w, h, nrofslices;
	avw::datatype type;
	float dx1, dy1, thickness1;
	avw::ReadHeader(filename, w, h, nrofslices, dx1, dy1, thickness1, type);
	thickness = thickness1;
	dx = dx1;
	dy = dy1;
	activeslice = 0;
	active_tissuelayer = 0;
	width = w;
	height = h;
	area = height * (unsigned int)width;
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += (image_slices[i]).ReadAvw(filename, i);

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRaw(const char* filename, short unsigned w,
						   short unsigned h, unsigned bitdepth,
						   unsigned short slicenr, unsigned short nrofslices,
						   Point p, unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	width = dx;
	height = dy;
	area = height * (unsigned int)width;
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += (image_slices[i])
				 .ReadRaw(filename, w, h, bitdepth, slicenr + i, p, dx, dy);

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawFloat(const char* filename, short unsigned w,
								short unsigned h, unsigned short slicenr,
								unsigned short nrofslices)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	width = w;
	height = h;
	area = height * (unsigned int)width;
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += (image_slices[i]).ReadRawFloat(filename, w, h, slicenr + i);

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawFloat(const char* filename, short unsigned w,
								short unsigned h, unsigned short slicenr,
								unsigned short nrofslices, Point p,
								unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	activeslice = 0;
	active_tissuelayer = 0;
	width = dx;
	height = dy;
	area = height * (unsigned int)width;
	startslice = 0;
	endslice = nrslices = nrofslices;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += (image_slices[i])
				 .ReadRawFloat(filename, w, h, slicenr + i, p, dx, dy);

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		loaded = true;
		return 1;
	}
	else
	{
		newbmp(width, height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReloadDIBitmap(const char* filename, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	char name[100];
	int j = 0;

	for (unsigned short i = startslice; i < endslice; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i - startslice + slicenr);
		j += (image_slices[i]).ReloadDIBitmap(name);
	}

	for (unsigned short i = startslice; i < endslice; i++)
	{
		if (width != image_slices[i].return_width() ||
			height != image_slices[i].return_height())
			j = nrslices + 1;
	}

	if (j == (endslice - startslice))
		return 1;
	else
	{
		newbmp(width, height, nrslices);
		return 0;
	}
}

int SlicesHandler::ReloadDIBitmap(const char* filename, Point p,
								  unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	char name[100];
	int j = 0;

	for (unsigned short i = startslice; i < endslice; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i - startslice + slicenr);
		j += (image_slices[i]).ReloadDIBitmap(name, p);
	}

	if (j == (endslice - startslice))
		return 1;
	else
	{
		newbmp(width, height, nrslices);
		return 0;
	}
}

int SlicesHandler::ReloadDIBitmap(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;

	if (filenames.size() == nrslices)
	{
		for (unsigned short i = 0; i < nrslices; i++)
		{
			j += (image_slices[i]).ReloadDIBitmap(filenames[i]);
		}

		for (unsigned short i = 0; i < nrslices; i++)
		{
			if (width != image_slices[i].return_width() ||
				height != image_slices[i].return_height())
				j = nrslices + 1;
		}

		if (j == nrslices)
			return 1;
		else
		{
			newbmp(width, height, nrslices);
			return 0;
		}
	}
	else if (filenames.size() == (endslice - startslice))
	{
		for (unsigned short i = startslice; i < endslice; i++)
		{
			j += (image_slices[i]).ReloadDIBitmap(filenames[i - startslice]);
		}

		for (unsigned short i = startslice; i < endslice; i++)
		{
			if (width != image_slices[i].return_width() ||
				height != image_slices[i].return_height())
				j = nrslices + 1;
		}

		if (j == (endslice - startslice))
			return 1;
		else
		{
			newbmp(width, height, nrslices);
			return 0;
		}
	}
	else
		return 0;
}

int SlicesHandler::ReloadDIBitmap(std::vector<const char*> filenames, Point p)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;

	if (filenames.size() == nrslices)
	{
		for (unsigned short i = 0; i < nrslices; i++)
		{
			j += (image_slices[i]).ReloadDIBitmap(filenames[i], p);
		}

		if (j == nrslices)
			return 1;
		else
		{
			newbmp(width, height, nrslices);
			return 0;
		}
	}
	else if (filenames.size() == (endslice - startslice))
	{
		for (unsigned short i = startslice; i < endslice; i++)
		{
			j += (image_slices[i]).ReloadDIBitmap(filenames[i - startslice], p);
		}

		if (j == (endslice - startslice))
			return 1;
		else
		{
			newbmp(width, height, nrslices);
			return 0;
		}
	}
	else
		return 0;
}

int SlicesHandler::ReloadRaw(const char* filename, unsigned bitdepth,
							 unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = startslice; i < endslice; i++)
		j += (image_slices[i])
				 .ReloadRaw(filename, bitdepth,
							(unsigned)slicenr + i - startslice);

	if (j == (endslice - startslice))
		return 1;
	else
		return 0;
}

int SlicesHandler::ReloadImage(const char* filename, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	unsigned w, h, nrofslices;
	float spacing1[3];
	Transform transform1;
	if (ImageReader::getInfo(filename, w, h, nrofslices, spacing1, transform1))
	{
		if ((w != width) || (h != height) ||
			(endslice - startslice + slicenr > nrofslices))
			return 0;
		std::vector<float*> slices(endslice - startslice);
		for (unsigned i = startslice; i < endslice; i++)
		{
			slices[i - startslice] = image_slices[i].return_bmp();
		}
		ImageReader::getVolume(filename, slices.data(), slicenr,
							   endslice - startslice, width, height);
		return 1;
	}
	return 0;
}

int SlicesHandler::ReloadRTdose(const char* filename, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

#if 0
	// TODO: TEST ***********************************************************************
	unsigned int dims[3];
	double spacing[3];
	double origin[3];

	dims[0] = this->width;
	dims[1] = this->height;
	dims[2] = this->nrslices;

	spacing[0] = this->get_pixelsize().high;
	spacing[1] = this->get_pixelsize().low;
	spacing[2] = this->thickness;

	origin[0] = this->displacement[0];
	origin[1] = this->displacement[1];
	origin[2] = this->displacement[2];

#	if 1
	// Pass slice pointers to writer
	size_t nrvalues = width;
	nrvalues *= height;
	nrvalues *= nrslices;

	std::vector<float> bmpslices(nrvalues);
	float *tmp = &bmpslices[0];
	for (unsigned i = 0; i < nrslices; i++) {
		memcpy(tmp, this->image_slices[i].return_bmp(), width * height * sizeof(float));
		tmp += width * height;
	}
#	else
	// Pass slice pointers to writer
	std::vector<float*> bmpslices(nrslices);
	for (unsigned i = 0; i < nrslices; i++)
	{
		bmpslices[i] = this->image_slices[i].return_bmp();
	}
#	endif

#	if 1 // gdcmRTDoseWriter
	gdcm::RTDoseWriter *writer = new gdcm::RTDoseWriter();
	// TODO: Testing input
	writer->SetAccessionNumber("0815");
	writer->SetBitsAllocated(32);
	writer->SetColumns(dims[1]);
	writer->SetFrameOfReferenceUID("1.3.12.2.1107.5.1.4.49212.30000012011007194512500000057");
	double dc[6] = { 1, 0, 0, 0, 1, 0 };
	writer->SetImageOrientationPatient(dc);
	writer->SetImagePositionPatient(origin);
	writer->SetInstanceNumber(1);
	writer->SetManufacturer("ADAC");
	writer->SetNumberOfFrames(dims[2]);
	writer->AddOperatorsName("John Doe");
	writer->AddOperatorsName("Jack Smith");
	unsigned short birthDate[3] = { 1948, 8, 30 };
	writer->SetPatientBirthDate(birthDate);
	writer->SetPatientID("158463");
	writer->SetPatientName("Balakc Yusuf");
	writer->SetPatientSex(gdcm::Male);
	writer->SetPixelData(bmpslices);
	writer->SetPixelSpacing(spacing);
	writer->SetPositionReferenceIndicator("Test");
	writer->SetReferringPhysicianName("Jane Doe");
	writer->SetRows(dims[0]);
	writer->SetSeriesInstanceUID("2.16.840.1.113669.2.937728.463871595.20120221120004.861140");
	writer->SetSeriesNumber(1234);
	writer->SetSliceThickness(spacing[2]);
	writer->SetSOPInstanceUID("2.16.840.1.113669.2.937728.463871595.20120221120005.51579");
	writer->AddSpecificCharacterSet(gdcm::Latin1);
	unsigned short studyDateTime[3] = { 2012,7,13 };
	writer->SetStudyDate(studyDateTime);
	writer->SetStudyID("0815");
	writer->SetStudyInstanceUID("1.3.12.2.1107.5.1.4.49212.3000001201090800348430000004");
	writer->SetStudyTime(studyDateTime);
	writer->AddReferencedRTPlanInstanceUID(std::string("2.16.840.1.113669.2.931128.463871595.20120221120004.977868"));
	writer->SetInstanceCreationDate(studyDateTime);
	writer->SetInstanceCreationTime(studyDateTime);
	writer->SetContentDate(studyDateTime);
	writer->SetContentTime(studyDateTime);
	writer->SetStationName("Some Station");
	writer->SetStudyDescription("Some Description");
	writer->SetManufacturersModelName("Some Name");
	writer->AddReferencedStudyInstanceUID(std::string("2.16.840.1.113669.2.931128.463871595.20120221120004.977868"));
	writer->AddSoftwareVersions("8.0m");
	writer->AddSoftwareVersions("Version: 8.2g");
	writer->SetNormalizationPoint(origin);

	writer->SetFileName(filename);
	if (!writer->Write()) {
		cerr << "WriteRTdose() : Write() failed" << endl;
		delete writer;
		return 0;
	}

	delete writer;
#	else // RTDoseWriter & RTDoseIODModule
	RTDoseWriter *writer = new RTDoseWriter();
	RTDoseIODModule *rtDose = new RTDoseIODModule();
	// TODO: Testing input
	rtDose->SetAccessionNumber("0815");
	rtDose->SetBitsAllocated(32);
	rtDose->SetColumns(dims[1]);
	rtDose->SetFrameOfReferenceUID("1.3.12.2.1107.5.1.4.49212.30000012011007194512500000057");
	double dc[6] = { 1, 0, 0, 0, 1, 0 };
	rtDose->SetImageOrientationPatient(dc);
	rtDose->SetImagePositionPatient(origin);
	rtDose->SetInstanceNumber(1);
	rtDose->SetManufacturer("ADAC");
	rtDose->SetNumberOfFrames(dims[2]);
	rtDose->AddOperatorsName("John Doe");
	rtDose->AddOperatorsName("Jack Smith");
	unsigned short birthDate[3] = { 1948, 8, 30 };
	rtDose->SetPatientBirthDate(birthDate);
	rtDose->SetPatientID("158463");
	rtDose->SetPatientName("Balakc Yusuf");
	rtDose->SetPatientSex(Male);
	rtDose->SetPixelData(bmpslices);
	rtDose->SetPixelSpacing(spacing);
	rtDose->SetPositionReferenceIndicator("Test");
	rtDose->SetReferringPhysicianName("Jane Doe");
	rtDose->SetRows(dims[0]);
	rtDose->SetSeriesInstanceUID("2.16.840.1.113669.2.937728.463871595.20120221120004.861140");
	rtDose->SetSeriesNumber(1234);
	rtDose->SetSliceThickness(spacing[2]);
	rtDose->SetSOPInstanceUID("2.16.840.1.113669.2.937728.463871595.20120221120005.51579");
	rtDose->AddSpecificCharacterSet(Latin1);
	unsigned short studyDateTime[3] = { 2012,7,13 };
	rtDose->SetStudyDate(studyDateTime);
	rtDose->SetStudyID("0815");
	rtDose->SetStudyInstanceUID("1.3.12.2.1107.5.1.4.49212.3000001201090800348430000004");
	rtDose->SetStudyTime(studyDateTime);
	rtDose->AddReferencedRTPlanInstanceUID(std::string("2.16.840.1.113669.2.931128.463871595.20120221120004.977868"));
	if (!writer->Write(filename, rtDose)) {
		cerr << "WriteRTdose() : Write() failed" << endl;
		return 0;
	}

	delete writer;
	delete rtDose;
#	endif
	return 1;

	// END TODO: TEST *******************************************************************
#else
	unsigned int dims[3];
	double spacing[3];
	double origin[3];
	double dc[6];

	// Read size data
	if (!RTDoseReader::ReadSizeData(filename, dims, spacing, origin, dc))
	{
		cerr << "ReloadRTdose() : ReadSizeData() failed" << endl;
		return 0;
	}

	if ((dims[0] != width) || (dims[1] != height) ||
		(endslice - startslice + slicenr > dims[2]))
		return 0;

	// Pass slice pointers to reader
	std::vector<float*> bmpslices(endslice - startslice);
	for (unsigned i = startslice; i < endslice; i++)
	{
		bmpslices[i - startslice] = this->image_slices[i].return_bmp();
	}

	// Read pixel data
	bool res = RTDoseReader::ReadPixelData(filename, bmpslices.data());
	if (res)
	{
		loaded = true;
	}
	else
	{
		cerr << "ReloadRTdose() : ReadPixelData() failed" << endl;
		newbmp(width, height, nrslices);
	}

	return res;
#endif
}

int SlicesHandler::ReloadAVW(const char* filename, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	unsigned short w, h, nrofslices;
	avw::datatype type;
	float dx1, dy1, thickness1;
	avw::ReadHeader(filename, w, h, nrofslices, dx1, dy1, thickness1, type);
	if ((w != width) || (h != height) || (endslice > nrofslices))
		return 0;
	if (h != height)
		return 0;

	int j = 0;
	for (unsigned short i = startslice; i < endslice; i++)
		j += (image_slices[i]).ReadAvw(filename, i);

	if (j == (endslice - startslice))
		return 1;
	return 0;
}

int SlicesHandler::ReloadRaw(const char* filename, short unsigned w,
							 short unsigned h, unsigned bitdepth,
							 unsigned short slicenr, Point p)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = startslice; i < endslice; i++)
		j += (image_slices[i])
				 .ReloadRaw(filename, w, h, bitdepth,
							(unsigned)slicenr + i - startslice, p);

	if (j == (endslice - startslice))
		return 1;
	else
		return 0;
}

std::vector<float*> SlicesHandler::LoadRawFloat(const char* filename,
												unsigned short startslice,
												unsigned short endslice,
												unsigned short slicenr,
												unsigned int area)
{
	std::vector<float*> slices_red;
	for (unsigned short i = startslice; i < endslice; i++)
	{
		float* slice_red = bmphandler::ReadRawFloat(
			filename, (unsigned)slicenr + i - startslice, area);
		slices_red.push_back(slice_red);
	}
	return slices_red;
}

int SlicesHandler::ReloadRawFloat(const char* filename, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = startslice; i < endslice; i++)
		j += (image_slices[i])
				 .ReloadRawFloat(filename, (unsigned)slicenr + i - startslice);

	if (j == (endslice - startslice))
		return 1;
	else
		return 0;
}

int SlicesHandler::ReloadRawFloat(const char* filename, short unsigned w,
								  short unsigned h, unsigned short slicenr,
								  Point p)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = startslice; i < endslice; i++)
		j += (image_slices[i])
				 .ReloadRawFloat(filename, w, h,
								 (unsigned)slicenr + i - startslice, p);

	if (j == (endslice - startslice))
		return 1;
	else
		return 0;
}

int SlicesHandler::ReloadRawTissues(const char* filename, unsigned bitdepth,
									unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = startslice; i < endslice; i++)
		j += (image_slices[i])
				 .ReloadRawTissues(filename, bitdepth,
								   (unsigned)slicenr + i - startslice);

	if (j == (endslice - startslice))
		return 1;
	else
		return 0;
}

int SlicesHandler::ReloadRawTissues(const char* filename, short unsigned w,
									short unsigned h, unsigned bitdepth,
									unsigned short slicenr, Point p)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = startslice; i < endslice; i++)
		j += (image_slices[i])
				 .ReloadRawTissues(filename, w, h, bitdepth,
								   (unsigned)slicenr + i - startslice, p);

	if (j == (endslice - startslice))
		return 1;
	else
		return 0;
}

FILE* SlicesHandler::SaveHeader(FILE* fp, short unsigned nr_slices_to_write,
								Transform transform_to_write)
{
	fwrite(&nr_slices_to_write, 1, sizeof(unsigned short), fp);
	float thick = -thickness;
	if (thick > 0)
		thick = -thick;
	fwrite(&thick, 1, sizeof(float), fp);
	fwrite(&dx, 1, sizeof(float), fp);
	fwrite(&dy, 1, sizeof(float), fp);
	int combinedVersion =
		iseg::CombineTissuesVersion(project_version, tissue_version);
	fwrite(&combinedVersion, 1, sizeof(int), fp);

	for (int r = 0; r < 4; r++)
	{
		const float* transform_r = transform_to_write[r];
		fwrite(&(transform_r[0]), 1, sizeof(float), fp);
		fwrite(&(transform_r[1]), 1, sizeof(float), fp);
		fwrite(&(transform_r[2]), 1, sizeof(float), fp);
		fwrite(&(transform_r[3]), 1, sizeof(float), fp);
	}

	return fp;
}

FILE* SlicesHandler::SaveProject(const char* filename,
								 const char* imageFileExtension)
{
	FILE* fp;

	if ((fp = fopen(filename, "wb")) == NULL)
		return NULL;

	fp = SaveHeader(fp, nrslices, transform);

	for (unsigned short j = 0; j < nrslices; j++)
	{
		fp = (image_slices[j]).save_proj(fp, false);
	}
	fp = (image_slices[0]).save_stack(fp);

	// SaveAllXdmf uses startslice/endslice to decide what to write - here we want to override that behavior
	unsigned short startslice1 = startslice;
	unsigned short endslice1 = endslice;
	startslice = 0;
	endslice = nrslices;
	unsigned char length1 = 0;
	while (imageFileExtension[length1] != '\0')
		length1++;
	length1++;
	fwrite(&length1, 1, sizeof(unsigned char), fp);
	fwrite(imageFileExtension, length1, sizeof(char), fp);
	QString imageFileName = QString(filename);
	int afterDot = imageFileName.lastIndexOf('.') + 1;
	imageFileName =
		imageFileName.remove(afterDot, imageFileName.length() - afterDot) +
		imageFileExtension;
	SaveAllXdmf(
		QFileInfo(filename).dir().absFilePath(imageFileName).toAscii().data(),
		this->Compression, false);

	startslice = startslice1;
	endslice = endslice1;

	return fp;
}

bool SlicesHandler::SaveCommunicationFile(const char* filename)
{
	unsigned short startslice1 = startslice;
	unsigned short endslice1 = endslice;
	startslice = 0;
	endslice = nrslices;

	SaveAllXdmf(
		QFileInfo(filename).dir().absFilePath(filename).toAscii().data(),
		this->Compression, true);

	startslice = startslice1;
	endslice = endslice1;

	return true;
}

FILE* SlicesHandler::SaveActiveSlices(const char* filename,
									  const char* imageFileExtension)
{
	FILE* fp;

	if ((fp = fopen(filename, "wb")) == NULL)
		return NULL;

	unsigned short slicecount = endslice - startslice;
	Transform transform_corrected = get_transform_active_slices();
	SaveHeader(fp, slicecount, transform_corrected);

	for (unsigned short j = startslice; j < endslice; j++)
	{
		fp = (image_slices[j]).save_proj(fp, false);
	}
	fp = (image_slices[0]).save_stack(fp);
	unsigned char length1 = 0;
	while (imageFileExtension[length1] != '\0')
		length1++;
	length1++;
	fwrite(&length1, 1, sizeof(unsigned char), fp);
	fwrite(imageFileExtension, length1, sizeof(char), fp);
	QString imageFileName = QString(filename);
	int afterDot = imageFileName.lastIndexOf('.') + 1;
	imageFileName =
		imageFileName.remove(afterDot, imageFileName.length() - afterDot) +
		imageFileExtension;
	SaveAllXdmf(
		QFileInfo(filename).dir().absFilePath(imageFileName).toAscii().data(),
		this->Compression, false);

	return fp;
}

FILE* SlicesHandler::MergeProjects(const char* savefilename,
								   std::vector<QString>& mergeFilenames)
{
	// Get number of slices to total
	unsigned short nrslicesTotal = nrslices;
	for (unsigned short i = 0; i < mergeFilenames.size(); i++)
	{
		if (!mergeFilenames[i].endsWith(".prj"))
		{
			return NULL;
		}

		FILE* fpMerge;
		// Add number of slices to total
		unsigned short nrslicesMerge = 0;
		if ((fpMerge = fopen(mergeFilenames[i].toAscii().data(), "rb")) == NULL)
			return NULL;
		fread(&nrslicesMerge, sizeof(unsigned short), 1, fpMerge);
		nrslicesTotal += nrslicesMerge;
		fclose(fpMerge);
	}

	// Save merged project
	FILE* fp;
	if ((fp = fopen(savefilename, "wb")) == NULL)
		return NULL;

	/// BL TODO what should merged transform be
	fp = SaveHeader(fp, nrslicesTotal, transform);

	// Save current project slices
	for (unsigned short j = 0; j < nrslices; j++)
	{
		fp = (image_slices[j]).save_proj(fp, false);
	}

	FILE* fpMerge;
	// Save merged project slices
	for (unsigned short i = 0; i < mergeFilenames.size(); i++)
	{
		if ((fpMerge = fopen(mergeFilenames[i].toAscii().data(), "rb")) == NULL)
			return NULL;

		// Skip header
		int version = 0;
		int tissuesVersion = 0;
		SlicesHandler dummy_SlicesHandler;
		dummy_SlicesHandler.LoadHeader(fpMerge, tissuesVersion, version);

		unsigned short mergeNrslices = dummy_SlicesHandler.return_nrslices();
		float mergeThickness = dummy_SlicesHandler.get_slicethickness();

		// Load input slices and save to merged project file
		bmphandler tmpSlice;
		for (unsigned short j = 0; j < mergeNrslices; j++)
		{
			fpMerge = tmpSlice.load_proj(fpMerge, tissuesVersion, false);
			fp = tmpSlice.save_proj(fp, false);
		}

		fclose(fpMerge);
	}

	fp = (image_slices[0]).save_stack(fp);

	unsigned short startslice1 = startslice;
	unsigned short endslice1 = endslice;
	startslice = 0;
	endslice = nrslices;
	QString imageFileExtension = "xmf";
	unsigned char length1 = 0;
	while (imageFileExtension[length1] != '\0')
		length1++;
	length1++;
	fwrite(&length1, 1, sizeof(unsigned char), fp);
	fwrite(imageFileExtension, length1, sizeof(char), fp);

	QString imageFileName = QString(savefilename);
	int afterDot = imageFileName.lastIndexOf('.') + 1;
	imageFileName =
		imageFileName.remove(afterDot, imageFileName.length() - afterDot) +
		imageFileExtension;
	if (!SaveMergeAllXdmf(QFileInfo(savefilename)
							  .dir()
							  .absFilePath(imageFileName)
							  .toAscii()
							  .data(),
						  mergeFilenames, nrslicesTotal, this->Compression))
		return NULL;
	startslice = startslice1;
	endslice = endslice1;

	return fp;
}

void SlicesHandler::LoadHeader(FILE* fp, int& tissuesVersion, int& version)
{
	activeslice = 0;

	fread(&nrslices, sizeof(unsigned short), 1, fp);
	startslice = 0;
	endslice = nrslices;
	fread(&thickness, sizeof(float), 1, fp);

	//    set_slicethickness(thickness);
	fread(&dx, sizeof(float), 1, fp);
	fread(&dy, sizeof(float), 1, fp);
	set_pixelsize(dx, dy);

	version = 0;
	tissuesVersion = 0;
	if (thickness < 0)
	{
		thickness = -thickness;
		int combinedVersion;
		fread(&combinedVersion, sizeof(int), 1, fp);
		iseg::ExtractTissuesVersion(combinedVersion, version, tissuesVersion);
	}

	if (version < 5)
	{
		float _displacement[3];
		float _directionCosines[6];
		Transform::setIdentity(_displacement, _directionCosines);

		if (version > 0)
		{
			fread(&(_displacement[0]), sizeof(float), 1, fp);
			fread(&(_displacement[1]), sizeof(float), 1, fp);
			fread(&(_displacement[2]), sizeof(float), 1, fp);
		}
		if (version > 3)
		{
			for (unsigned short i = 0; i < 6; ++i)
			{
				fread(&(_directionCosines[i]), sizeof(float), 1, fp);
			}
		}
		transform.setTransform(_displacement, _directionCosines);
	}
	else
	{
		for (int r = 0; r < 4; r++)
		{
			float* transform_r = transform[r];
			fread(&(transform_r[0]), sizeof(float), 1, fp);
			fread(&(transform_r[1]), sizeof(float), 1, fp);
			fread(&(transform_r[2]), sizeof(float), 1, fp);
			fread(&(transform_r[3]), sizeof(float), 1, fp);
		}
	}
}

FILE* SlicesHandler::LoadProject(const char* filename, int& tissuesVersion)
{
	FILE* fp;

	if ((fp = fopen(filename, "rb")) == NULL)
		return NULL;

	int version = 0;
	LoadHeader(fp, tissuesVersion, version);

	image_slices.resize(nrslices);

	os.set_sizenr(nrslices);

	if (version > 1)
	{
		for (unsigned short j = 0; j < nrslices; j++)
		{
			fp = (image_slices[j]).load_proj(fp, tissuesVersion, false);
		}
	}
	else
	{
		for (unsigned short j = 0; j < nrslices; j++)
		{
			fp = (image_slices[j]).load_proj(fp, tissuesVersion);
		}
	}

	set_slicethickness(thickness);

	fp = (image_slices[0]).load_stack(fp);

	width = (image_slices[0]).return_width();
	height = (image_slices[0]).return_height();
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	if (version > 1)
	{
		QString imageFileName;
		if (version > 2)
		{
			// Only load image file name extension
			char imageFileExtension[10];
			unsigned char length1;
			fread(&length1, sizeof(unsigned char), 1, fp);
			fread(imageFileExtension, sizeof(char), length1, fp);
			imageFileName = QString(filename);
			int afterDot = imageFileName.lastIndexOf('.') + 1;
			imageFileName = imageFileName.remove(
								afterDot, imageFileName.length() - afterDot) +
							QString(imageFileExtension);
		}
		else
		{
			// Load full image file name
			char filenameimage[200];
			unsigned char length1;
			fread(&length1, sizeof(unsigned char), 1, fp);
			fread(filenameimage, sizeof(char), length1, fp);
			//fscanf(fp,"%s",filenameimage);
			imageFileName = QString(filenameimage);
		}

		if (imageFileName.endsWith(".xmf", Qt::CaseInsensitive))
		{
			LoadAllXdmf(QFileInfo(filename)
							.dir()
							.absFilePath(imageFileName)
							.toAscii()
							.data());
		}
		else
			cerr << "error, unsupported format..." << endl;
	}

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrslices);
	slice_bmpranges.resize(nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	loaded = true;

	return fp;
}

bool SlicesHandler::LoadS4Llink(const char* filename, int& tissuesVersion)
{
	unsigned w, h, nrofslices;
	float* pixsize;
	float* tr_1d;
	QStringList arrayNames;

	HDFImageReader reader;
	reader.SetFileName(filename);
	reader.SetReadContiguousMemory(GetContiguousMemory());
	reader.ParseHDF();
	w = reader.GetWidth();
	h = reader.GetHeight();
	nrofslices = reader.GetNumberOfSlices();
	pixsize = reader.GetPixelSize();
	tr_1d = reader.GetImageTransform();
	arrayNames = reader.GetArrayNames();

	cerr << "SlicesHandler::LoadS4Llink() : Extent = " << width << " " << height
		 << " " << nrofslices << endl;
	cerr << "SlicesHandler::LoadS4Llink() : Spacing = " << pixsize[0] << " "
		 << pixsize[1] << " " << pixsize[2] << endl;
	const int NPA = arrayNames.size();
	cerr << "SlicesHandler::LoadS4Llink() : number of point arrays: " << NPA
		 << endl;
	for (int i = 0; i < NPA; ++i)
	{
		cerr << "\tarray id = " << i
			 << ", name = " << arrayNames[i].toAscii().data() << endl;
	}

	// taken from LoadProject()
	activeslice = 0;
	this->width = w;
	this->height = h;
	this->area = height * (unsigned int)width;
	this->nrslices = nrofslices;
	this->startslice = 0;
	this->endslice = nrslices;
	this->set_pixelsize(pixsize[0], pixsize[1]);
	this->thickness = pixsize[2];

	float* transform_1d = transform[0];
	std::copy(tr_1d, tr_1d + 16, transform_1d);

	this->image_slices.resize(nrslices);
	this->os.set_sizenr(nrslices);
	this->set_slicethickness(thickness);
	this->loaded = true;
	for (unsigned short j = 0; j < nrslices; j++)
	{
		this->image_slices[j].newbmp(w, h);
	}
	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	bool res = LoadAllHDF(filename);

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	set_active_tissuelayer(0);

	return res;
}

int SlicesHandler::SaveBmpRaw(const char* filename)
{
	return SaveRaw(filename, false);
}

int SlicesHandler::SaveWorkRaw(const char* filename)
{
	return SaveRaw(filename, true);
}

int SlicesHandler::SaveRaw(const char* filename, bool work)
{
	FILE* fp;
	unsigned char* bits_tmp;
	float* p_bits;

	bits_tmp = (unsigned char*)malloc(sizeof(unsigned char) * area);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	unsigned int bitsize = width * (unsigned)height;

	for (unsigned short j = 0; j < nrslices; j++)
	{
		if (work)
			p_bits = (image_slices[j]).return_work();
		else
			p_bits = (image_slices[j]).return_bmp();
		for (unsigned int i = 0; i < area; i++)
		{
			bits_tmp[i] = (unsigned char)(std::min(
				255.0, std::max(0.0, p_bits[i] + 0.5)));
		}
		if (fwrite(bits_tmp, 1, bitsize, fp) < (unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveTissueRaw(const char* filename)
{
	FILE* fp;
	tissues_size_t* bits_tmp;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	unsigned int bitsize = width * (unsigned)height;

	if ((TissueInfos::GetTissueCount() <= 255) &&
		(sizeof(tissues_size_t) > sizeof(unsigned char)))
	{
		unsigned char* ucharBuffer = new unsigned char[bitsize];
		for (unsigned short j = 0; j < nrslices; j++)
		{
			bits_tmp = (image_slices[j]).return_tissues(active_tissuelayer);
			for (unsigned int i = 0; i < bitsize; ++i)
			{
				ucharBuffer[i] = (unsigned char)bits_tmp[i];
			}
			if (fwrite(ucharBuffer, sizeof(unsigned char), bitsize, fp) <
				(unsigned int)bitsize)
			{
				fclose(fp);
				return (-1);
			}
		}
		delete[] ucharBuffer;
	}
	else
	{
		for (unsigned short j = 0; j < nrslices; j++)
		{
			bits_tmp = (image_slices[j]).return_tissues(active_tissuelayer);
			if (fwrite(bits_tmp, sizeof(tissues_size_t), bitsize, fp) <
				(unsigned int)bitsize)
			{
				fclose(fp);
				return (-1);
			}
		}
	}

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveRaw_resized(const char* filename, int dxm, int dxp,
								   int dym, int dyp, int dzm, int dzp,
								   bool work)
{
	if ((-(dxm + dxp) >= width) || (-(dym + dyp) >= height) ||
		(-(dzm + dzp) >= nrslices))
		return (-1);
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	int l2 = width + dxm + dxp;

	unsigned newarea =
		(unsigned)(width + dxm + dxp) * (unsigned)(height + dym + dyp);
	bits_tmp = (float*)malloc(sizeof(float) * newarea);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	for (unsigned i = 0; i < newarea; i++)
		bits_tmp[i] = 0;
	for (int i = 0; i < dzm; i++)
	{
		if (fwrite(bits_tmp, 1, sizeof(float) * newarea, fp) <
			sizeof(float) * newarea)
		{
			free(bits_tmp);
			fclose(fp);
			return (-1);
		}
	}

	unsigned posstart1, posstart2;
	unsigned skip1, skip2;
	skip1 = 0;
	skip2 = 0;
	if (dxm < 0)
	{
		skip1 += unsigned(-dxm);
	}
	else
	{
		skip2 += unsigned(dxm);
	}
	if (dxp < 0)
	{
		skip1 += unsigned(-dxp);
	}
	else
	{
		skip2 += unsigned(dxp);
	}
	int l1 = l2 - skip2;
	if (dxm < 0)
	{
		posstart1 = unsigned(-dxm);
		posstart2 = 0;
	}
	else
	{
		posstart1 = 0;
		posstart2 = unsigned(dxm);
	}
	int h1 = height;
	if (dym < 0)
	{
		h1 += dym;
		posstart1 += width * unsigned(-dym);
	}
	else
	{
		posstart2 += l2 * unsigned(dym);
	}
	if (dyp < 0)
		h1 += dyp;

	for (unsigned short j = (unsigned short)std::max(0, -dzm);
		 j < nrslices - (unsigned short)std::max(0, -dzp); j++)
	{
		if (work)
			p_bits = (image_slices[j]).return_work();
		else
			p_bits = (image_slices[j]).return_bmp();

		unsigned pos1, pos2;
		pos1 = posstart1;
		pos2 = posstart2;
		for (int y = 0; y < h1; y++)
		{
			for (int x = 0; x < l1; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2++;
			}
			pos1 += skip1;
			pos2 += skip2;
		}
		if (fwrite(bits_tmp, 1, sizeof(float) * newarea, fp) <
			sizeof(float) * newarea)
		{
			free(bits_tmp);
			fclose(fp);
			return (-1);
		}
	}

	for (unsigned i = 0; i < newarea; i++)
		bits_tmp[i] = 0;
	for (int i = 0; i < dzp; i++)
	{
		if (fwrite(bits_tmp, 1, sizeof(float) * newarea, fp) <
			sizeof(float) * newarea)
		{
			free(bits_tmp);
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveTissuesRaw_resized(const char* filename, int dxm,
										  int dxp, int dym, int dyp, int dzm,
										  int dzp)
{
	if ((-(dxm + dxp) >= width) || (-(dym + dyp) >= height) ||
		(-(dzm + dzp) >= nrslices))
		return (-1);
	FILE* fp;
	tissues_size_t* bits_tmp;

	int l2 = width + dxm + dxp;

	unsigned newarea =
		(unsigned)(width + dxm + dxp) * (unsigned)(height + dym + dyp);
	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * newarea);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	for (unsigned i = 0; i < newarea; i++)
		bits_tmp[i] = 0;
	for (int i = 0; i < dzm; i++)
	{
		if (fwrite(bits_tmp, sizeof(tissues_size_t), newarea, fp) < newarea)
		{
			free(bits_tmp);
			fclose(fp);
			return (-1);
		}
	}

	unsigned posstart1, posstart2;
	unsigned skip1, skip2;
	skip1 = 0;
	skip2 = 0;
	if (dxm < 0)
	{
		skip1 += unsigned(-dxm);
	}
	else
	{
		skip2 += unsigned(dxm);
	}
	if (dxp < 0)
	{
		skip1 += unsigned(-dxp);
	}
	else
	{
		skip2 += unsigned(dxp);
	}
	int l1 = l2 - skip2;
	if (dxm < 0)
	{
		posstart1 = unsigned(-dxm);
		posstart2 = 0;
	}
	else
	{
		posstart1 = 0;
		posstart2 = unsigned(dxm);
	}
	int h1 = height;
	if (dym < 0)
	{
		h1 += dym;
		posstart1 += width * unsigned(-dym);
	}
	else
	{
		posstart2 += l2 * unsigned(dym);
	}
	if (dyp < 0)
		h1 += dyp;

	for (unsigned short j = (unsigned short)std::max(0, -dzm);
		 j < nrslices - (unsigned short)std::max(0, -dzp); j++)
	{
		tissues_size_t* p_bits =
			(image_slices[j]).return_tissues(active_tissuelayer);
		unsigned pos1, pos2;
		pos1 = posstart1;
		pos2 = posstart2;
		for (int y = 0; y < h1; y++)
		{
			for (int x = 0; x < l1; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2++;
			}
			pos1 += skip1;
			pos2 += skip2;
		}
		if (fwrite(bits_tmp, sizeof(tissues_size_t), newarea, fp) < newarea)
		{
			free(bits_tmp);
			fclose(fp);
			return (-1);
		}
	}

	for (unsigned i = 0; i < newarea; i++)
		bits_tmp[i] = 0;
	for (int i = 0; i < dzp; i++)
	{
		if (fwrite(bits_tmp, sizeof(tissues_size_t), newarea, fp) < newarea)
		{
			free(bits_tmp);
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

bool SlicesHandler::SwapXY()
{
	auto lut = GetColorLookupTable();

	unsigned short w, h, nrslices;
	w = return_height();
	h = return_width();
	nrslices = return_nrslices();
	QString str1;
	bool ok = true;
	unsigned char mode1 = get_activebmphandler()->return_mode(true);
	unsigned char mode2 = get_activebmphandler()->return_mode(false);

	str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
	if (SaveRaw_xy_swapped(str1.ascii(), false) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("work_float.raw"));
	if (SaveRaw_xy_swapped(str1.ascii(), true) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("tissues.raw"));
	if (SaveTissuesRaw_xy_swapped(str1.ascii()) != 0)
		ok = false;

	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("work_float.raw"));
		if (ReadRawFloat(str1.ascii(), w, h, 0, nrslices) != 1)
			ok = false;
		set_modeall(mode1, true);
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
		if (ReloadRawFloat(str1.ascii(), 0) != 1)
			ok = false;
		set_modeall(mode2, false);
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("tissues.raw"));
		if (ReloadRawTissues(str1.ascii(), sizeof(tissues_size_t) * 8, 0) != 1)
			ok = false;
	}

	// BL TODO direction cosines don't reflect full transform
	float disp[3];
	get_displacement(disp);
	float dummy = disp[1];
	disp[1] = disp[0];
	disp[0] = dummy;
	set_displacement(disp);
	float dc[6];
	get_direction_cosines(dc);
	std::swap(dc[0], dc[3]);
	std::swap(dc[1], dc[4]);
	std::swap(dc[2], dc[5]);
	set_direction_cosines(dc);

	// add color lookup table again
	UpdateColorLookupTable(lut);

	SliceProviderInstaller::getinst()->report();

	return ok;
}

bool SlicesHandler::SwapYZ()
{
	auto lut = GetColorLookupTable();

	unsigned short w, h, nrslices;
	w = return_width();
	h = return_nrslices();
	nrslices = return_height();
	QString str1;
	bool ok = true;
	str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
	if (SaveRaw_yz_swapped(str1.ascii(), false) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("work_float.raw"));
	if (SaveRaw_yz_swapped(str1.ascii(), true) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("tissues.raw"));
	if (SaveTissuesRaw_yz_swapped(str1.ascii()) != 0)
		ok = false;

	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("work_float.raw"));
		if (ReadRawFloat(str1.ascii(), w, h, 0, nrslices) != 1)
			ok = false;
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
		if (ReloadRawFloat(str1.ascii(), 0) != 1)
			ok = false;
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("tissues.raw"));
		if (ReloadRawTissues(str1.ascii(), sizeof(tissues_size_t) * 8, 0) != 1)
			ok = false;
	}

	// BL TODO direction cosines don't reflect full transform
	float disp[3];
	get_displacement(disp);
	float dummy = disp[2];
	disp[2] = disp[1];
	disp[1] = dummy;
	set_displacement(disp);
	float dc[6];
	get_direction_cosines(dc);
	float cross[3]; // direction cosines of z-axis (in image coordinate frame)
	vtkMath::Cross(&dc[0], &dc[3], cross);
	vtkMath::Normalize(cross);
	for (unsigned short i = 0; i < 3; ++i)
	{
		dc[i + 3] = cross[i];
	}
	set_direction_cosines(dc);

	// add color lookup table again
	UpdateColorLookupTable(lut);

	SliceProviderInstaller::getinst()->report();

	return ok;
}

bool SlicesHandler::SwapXZ()
{
	auto lut = GetColorLookupTable();

	unsigned short w, h, nrslices;
	w = return_nrslices();
	h = return_height();
	nrslices = return_width();
	Pair p = get_pixelsize();
	float thick = get_slicethickness();
	QString str1;
	bool ok = true;
	str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
	if (SaveRaw_xz_swapped(str1.ascii(), false) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("work_float.raw"));
	if (SaveRaw_xz_swapped(str1.ascii(), true) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("tissues.raw"));
	if (SaveTissuesRaw_xz_swapped(str1.ascii()) != 0)
		ok = false;

	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("work_float.raw"));
		if (ReadRawFloat(str1.ascii(), w, h, 0, nrslices) != 1)
			ok = false;
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
		if (ReloadRawFloat(str1.ascii(), 0) != 1)
			ok = false;
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("tissues.raw"));
		if (ReloadRawTissues(str1.ascii(), sizeof(tissues_size_t) * 8, 0) != 1)
			ok = false;
	}

	// BL TODO direction cosines don't reflect full transform
	float disp[3];
	get_displacement(disp);
	float dummy = disp[2];
	disp[2] = disp[0];
	disp[0] = dummy;
	set_displacement(disp);
	float dc[6];
	get_direction_cosines(dc);
	float cross[3]; // direction cosines of z-axis (in image coordinate frame)
	vtkMath::Cross(&dc[0], &dc[3], cross);
	vtkMath::Normalize(cross);
	for (unsigned short i = 0; i < 3; ++i)
	{
		dc[i] = cross[i];
	}
	set_direction_cosines(dc);

	// add color lookup table again
	UpdateColorLookupTable(lut);

	SliceProviderInstaller::getinst()->report();

	return ok;
}

int SlicesHandler::SaveRaw_xy_swapped(const char* filename, bool work)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	bits_tmp = (float*)malloc(sizeof(float) * area);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	unsigned int bitsize = width * (unsigned)height;

	for (unsigned short j = 0; j < nrslices; j++)
	{
		if (work)
			p_bits = (image_slices[j]).return_work();
		else
			p_bits = (image_slices[j]).return_bmp();
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short y = 0; y < height; y++)
		{
			pos2 = y;
			for (unsigned short x = 0; x < width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2 += height;
			}
		}
		if (fwrite(bits_tmp, 1, sizeof(float) * bitsize, fp) <
			sizeof(float) * (unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveRaw_xy_swapped(const char* filename,
									  std::vector<float*> bits_to_swap,
									  short unsigned width,
									  short unsigned height,
									  short unsigned nrslices)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	bits_tmp = (float*)malloc(sizeof(float) * width * height);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	unsigned int bitsize = width * (unsigned)height;

	for (unsigned short j = 0; j < nrslices; j++)
	{
		p_bits = bits_to_swap[j];
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short y = 0; y < height; y++)
		{
			pos2 = y;
			for (unsigned short x = 0; x < width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2 += height;
			}
		}
		if (fwrite(bits_tmp, 1, sizeof(float) * bitsize, fp) <
			sizeof(float) * (unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveRaw_xz_swapped(const char* filename, bool work)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	unsigned int bitsize = nrslices * (unsigned)height;
	bits_tmp = (float*)malloc(sizeof(float) * bitsize);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	for (unsigned short x = 0; x < width; x++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < nrslices; z++)
		{
			if (work)
				p_bits = (image_slices[z]).return_work();
			else
				p_bits = (image_slices[z]).return_bmp();
			pos2 = z;
			pos1 = x;
			for (unsigned short y = 0; y < height; y++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1 += width;
				pos2 += nrslices;
			}
		}
		if (fwrite(bits_tmp, 1, sizeof(float) * bitsize, fp) <
			sizeof(float) * (unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveRaw_xz_swapped(const char* filename,
									  std::vector<float*> bits_to_swap,
									  short unsigned width,
									  short unsigned height,
									  short unsigned nrslices)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	unsigned int bitsize = nrslices * (unsigned)height;
	bits_tmp = (float*)malloc(sizeof(float) * bitsize);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	for (unsigned short x = 0; x < width; x++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < nrslices; z++)
		{
			p_bits = bits_to_swap[z];
			pos2 = z;
			pos1 = x;
			for (unsigned short y = 0; y < height; y++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1 += width;
				pos2 += nrslices;
			}
		}
		if (fwrite(bits_tmp, 1, sizeof(float) * bitsize, fp) <
			sizeof(float) * (unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveRaw_yz_swapped(const char* filename, bool work)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	unsigned int bitsize = nrslices * (unsigned)width;
	bits_tmp = (float*)malloc(sizeof(float) * bitsize);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	for (unsigned short y = 0; y < height; y++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < nrslices; z++)
		{
			if (work)
				p_bits = (image_slices[z]).return_work();
			else
				p_bits = (image_slices[z]).return_bmp();
			pos2 = z * width;
			pos1 = y * width;
			for (unsigned short x = 0; x < width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2++;
			}
		}
		if (fwrite(bits_tmp, 1, sizeof(float) * bitsize, fp) <
			sizeof(float) * (unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveRaw_yz_swapped(const char* filename,
									  std::vector<float*> bits_to_swap,
									  short unsigned width,
									  short unsigned height,
									  short unsigned nrslices)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	unsigned int bitsize = nrslices * (unsigned)width;
	bits_tmp = (float*)malloc(sizeof(float) * bitsize);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	for (unsigned short y = 0; y < height; y++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < nrslices; z++)
		{
			p_bits = bits_to_swap[z];
			pos2 = z * width;
			pos1 = y * width;
			for (unsigned short x = 0; x < width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2++;
			}
		}
		if (fwrite(bits_tmp, 1, sizeof(float) * bitsize, fp) <
			sizeof(float) * (unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveTissuesRaw(const char* filename)
{
	FILE* fp;
	tissues_size_t* bits_tmp;
	//float *p_bits;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	unsigned int bitsize = width * (unsigned)height;

	for (unsigned short j = 0; j < nrslices; j++)
	{
		bits_tmp = (image_slices[j]).return_tissues(active_tissuelayer);
		if (fwrite(bits_tmp, sizeof(tissues_size_t), bitsize, fp) <
			(unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveTissuesRaw_xy_swapped(const char* filename)
{
	FILE* fp;
	tissues_size_t* bits_tmp;
	tissues_size_t* p_bits;

	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	unsigned int bitsize = width * (unsigned)height;

	for (unsigned short j = 0; j < nrslices; j++)
	{
		p_bits = (image_slices[j]).return_tissues(active_tissuelayer); // TODO
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short y = 0; y < height; y++)
		{
			pos2 = y;
			for (unsigned short x = 0; x < width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2 += height;
			}
		}
		if (fwrite(bits_tmp, sizeof(tissues_size_t), bitsize, fp) <
			(unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveTissuesRaw_xz_swapped(const char* filename)
{
	FILE* fp;
	tissues_size_t* bits_tmp;
	tissues_size_t* p_bits;

	unsigned int bitsize = nrslices * (unsigned)height;
	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * bitsize);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	for (unsigned short x = 0; x < width; x++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < nrslices; z++)
		{
			p_bits =
				(image_slices[z]).return_tissues(active_tissuelayer); // TODO
			pos2 = z;
			pos1 = x;
			for (unsigned short y = 0; y < height; y++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1 += width;
				pos2 += nrslices;
			}
		}
		if (fwrite(bits_tmp, sizeof(tissues_size_t), bitsize, fp) <
			(unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveTissuesRaw_yz_swapped(const char* filename)
{
	FILE* fp;
	tissues_size_t* bits_tmp;
	tissues_size_t* p_bits;

	unsigned int bitsize = nrslices * (unsigned)width;
	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * bitsize);
	if (bits_tmp == NULL)
		return -1;

	if ((fp = fopen(filename, "wb")) == NULL)
		return (-1);

	for (unsigned short y = 0; y < height; y++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < nrslices; z++)
		{
			p_bits =
				(image_slices[z]).return_tissues(active_tissuelayer); // TODO
			pos2 = z * width;
			pos1 = y * width;
			for (unsigned short x = 0; x < width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2++;
			}
		}
		if (fwrite(bits_tmp, sizeof(tissues_size_t), bitsize, fp) <
			(unsigned int)bitsize)
		{
			fclose(fp);
			return (-1);
		}
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

int SlicesHandler::SaveBmpBitmap(const char* filename)
{
	char name[100];
	int j = 0;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += (image_slices[i]).SaveDIBitmap(name);
	}

	if (j == 0)
	{
		return 1;
	}
	else
		return 0;
}

int SlicesHandler::SaveWorkBitmap(const char* filename)
{
	char name[100];
	int j = 0;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += (image_slices[i]).SaveWorkBitmap(name);
	}

	if (j == 0)
	{
		return 1;
	}
	else
		return 0;
}

int SlicesHandler::SaveTissueBitmap(const char* filename)
{
	char name[100];
	int j = 0;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += (image_slices[i]).SaveTissueBitmap(active_tissuelayer, name);
	}

	if (j == 0)
	{
		return 1;
	}
	else
		return 0;
}

void SlicesHandler::work2bmp() { (image_slices[activeslice]).work2bmp(); }

void SlicesHandler::bmp2work() { (image_slices[activeslice]).bmp2work(); }

void SlicesHandler::swap_bmpwork()
{
	(image_slices[activeslice]).swap_bmpwork();
}

void SlicesHandler::work2bmpall()
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).work2bmp();

	return;
}

void SlicesHandler::bmp2workall()
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).bmp2work();

	return;
}

void SlicesHandler::work2tissueall()
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).work2tissue(active_tissuelayer);

	return;
}

void SlicesHandler::mergetissues(tissues_size_t tissuetype)
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).mergetissue(tissuetype, active_tissuelayer);

	return;
}

void SlicesHandler::tissue2workall()
{
	image_slices[activeslice].tissue2work(active_tissuelayer);
}

void SlicesHandler::tissue2workall3D()
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).tissue2work(active_tissuelayer);

	return;
}

void SlicesHandler::swap_bmpworkall()
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).swap_bmpwork();

	return;
}

void SlicesHandler::add_mark(Point p, unsigned label)
{
	(image_slices[activeslice]).add_mark(p, label);
}

void SlicesHandler::add_mark(Point p, unsigned label, std::string str)
{
	(image_slices[activeslice]).add_mark(p, label, str);
}

void SlicesHandler::clear_marks() { (image_slices[activeslice]).clear_marks(); }

bool SlicesHandler::remove_mark(Point p, unsigned radius)
{
	return (image_slices[activeslice]).remove_mark(p, radius);
}

void SlicesHandler::add_mark(Point p, unsigned label, unsigned short slicenr)
{
	if (slicenr < nrslices && slicenr >= 0)
		(image_slices[slicenr]).add_mark(p, label);
}

void SlicesHandler::add_mark(Point p, unsigned label, std::string str,
							 unsigned short slicenr)
{
	if (slicenr < nrslices && slicenr >= 0)
		(image_slices[slicenr]).add_mark(p, label, str);
}

bool SlicesHandler::remove_mark(Point p, unsigned radius,
								unsigned short slicenr)
{
	if (slicenr < nrslices && slicenr >= 0)
		return (image_slices[slicenr]).remove_mark(p, radius);
	else
		return false;
}

void SlicesHandler::get_labels(std::vector<augmentedmark>* labels)
{
	labels->clear();
	std::vector<Mark> labels1;
	for (unsigned short i = 0; i < nrslices; i++)
	{
		(image_slices[i]).get_labels(&labels1);
		for (size_t j = 0; j < labels1.size(); j++)
		{
			augmentedmark am;
			am.mark = labels1[j].mark;
			am.p = labels1[j].p;
			am.name = labels1[j].name;
			am.slicenr = i;
			labels->push_back(am);
		}
	}
}

void SlicesHandler::add_vm(std::vector<Mark>* vm1)
{
	(image_slices[activeslice]).add_vm(vm1);
}

bool SlicesHandler::remove_vm(Point p, unsigned radius)
{
	return (image_slices[activeslice]).del_vm(p, radius);
}

void SlicesHandler::add_vm(std::vector<Mark>* vm1, unsigned short slicenr)
{
	if (slicenr < nrslices && slicenr >= 0)
		(image_slices[slicenr]).add_vm(vm1);
}

bool SlicesHandler::remove_vm(Point p, unsigned radius, unsigned short slicenr)
{
	if (slicenr < nrslices && slicenr >= 0)
		return (image_slices[slicenr]).del_vm(p, radius);
	else
		return false;
}

void SlicesHandler::add_limit(std::vector<Point>* vp1)
{
	(image_slices[activeslice]).add_limit(vp1);
}

bool SlicesHandler::remove_limit(Point p, unsigned radius)
{
	return (image_slices[activeslice]).del_limit(p, radius);
}

void SlicesHandler::add_limit(std::vector<Point>* vp1, unsigned short slicenr)
{
	if (slicenr < nrslices && slicenr >= 0)
		(image_slices[slicenr]).add_limit(vp1);
}

bool SlicesHandler::remove_limit(Point p, unsigned radius,
								 unsigned short slicenr)
{
	if (slicenr < nrslices && slicenr >= 0)
		return (image_slices[slicenr]).del_limit(p, radius);
	else
		return false;
}

void SlicesHandler::newbmp(unsigned short width1, unsigned short height1,
						   unsigned short nrofslices)
{
	activeslice = 0;
	startslice = 0;
	endslice = nrslices = nrofslices;
	cerr << "nrslices = " << nrslices << endl;
	cerr << "nrofslices = " << nrofslices << endl;
	os.set_sizenr(nrslices);

	image_slices.resize(nrofslices);

	for (unsigned short i = 0; i < nrslices; i++)
		(image_slices[i]).newbmp(width1, height1);

	// Ranges
	Pair dummy;
	slice_ranges.resize(nrofslices);
	slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	loaded = true;

	width = width1;
	height = height1;
	area = height * (unsigned int)width;
	set_active_tissuelayer(0);

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	return;
}

void SlicesHandler::freebmp()
{
	for (unsigned short i = 0; i < nrslices; i++)
		(image_slices[i]).freebmp();

	loaded = false;

	return;
}

void SlicesHandler::clear_bmp()
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).clear_bmp();

	return;
}

void SlicesHandler::clear_work()
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).clear_work();

	return;
}

void SlicesHandler::clear_overlay()
{
	for (unsigned int i = 0; i < area; i++)
		overlay[i] = 0.0f;
}

void SlicesHandler::set_bmp(unsigned short slicenr, float* bits,
							unsigned char mode)
{
	(image_slices[slicenr]).set_bmp(bits, mode);
}

void SlicesHandler::set_work(unsigned short slicenr, float* bits,
							 unsigned char mode)
{
	(image_slices[slicenr]).set_work(bits, mode);
}

void SlicesHandler::set_tissue(unsigned short slicenr, tissues_size_t* bits)
{
	(image_slices[slicenr]).set_tissue(active_tissuelayer, bits);
}

void SlicesHandler::copy2bmp(unsigned short slicenr, float* bits,
							 unsigned char mode)
{
	(image_slices[slicenr]).copy2bmp(bits, mode);
}

void SlicesHandler::copy2work(unsigned short slicenr, float* bits,
							  unsigned char mode)
{
	(image_slices[slicenr]).copy2work(bits, mode);
}

void SlicesHandler::copy2tissue(unsigned short slicenr, tissues_size_t* bits)
{
	(image_slices[slicenr]).copy2tissue(active_tissuelayer, bits);
}

void SlicesHandler::copyfrombmp(unsigned short slicenr, float* bits)
{
	(image_slices[slicenr]).copyfrombmp(bits);
}

void SlicesHandler::copyfromwork(unsigned short slicenr, float* bits)
{
	(image_slices[slicenr]).copyfromwork(bits);
}

void SlicesHandler::copyfromtissue(unsigned short slicenr, tissues_size_t* bits)
{
	(image_slices[slicenr]).copyfromtissue(active_tissuelayer, bits);
}

#ifdef TISSUES_SIZE_TYPEDEF
void SlicesHandler::copyfromtissue(unsigned short slicenr, unsigned char* bits)
{
	(image_slices[slicenr]).copyfromtissue(active_tissuelayer, bits);
}
#endif // TISSUES_SIZE_TYPEDEF

void SlicesHandler::copyfromtissuepadded(unsigned short slicenr,
										 tissues_size_t* bits,
										 unsigned short padding)
{
	(image_slices[slicenr])
		.copyfromtissuepadded(active_tissuelayer, bits, padding);
}

unsigned int SlicesHandler::make_histogram(bool includeoutofrange)
{
	unsigned int* dummy;
	unsigned int l = 0;

	for (unsigned short i = startslice; i < endslice; i++)
		l += (image_slices[i]).make_histogram(includeoutofrange);

	dummy = (image_slices[startslice]).return_histogram();
	for (unsigned short i = 0; i < 255; i++)
		histogram[i] = dummy[i];

	for (unsigned short j = startslice + 1; j < endslice; j++)
	{
		dummy = (image_slices[j]).return_histogram();
		for (unsigned short i = 0; i < 255; i++)
			histogram[i] += dummy[i];
	}

	return l;
}

unsigned int* SlicesHandler::return_histogram() { return histogram; }

unsigned int SlicesHandler::return_area()
{
	return (image_slices[0]).return_area();
}

unsigned short SlicesHandler::return_width() { return width; }

unsigned short SlicesHandler::return_height() { return height; }

unsigned short SlicesHandler::return_nrslices() { return nrslices; }

unsigned short SlicesHandler::return_startslice() { return startslice; }

unsigned short SlicesHandler::return_endslice() { return endslice; }

void SlicesHandler::set_startslice(unsigned short startslice1)
{
	startslice = startslice1;
}

void SlicesHandler::set_endslice(unsigned short endslice1)
{
	endslice = endslice1;
}

bool SlicesHandler::isloaded() { return loaded; }

void SlicesHandler::gaussian(float sigma)
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).gaussian(sigma);

	return;
}

void SlicesHandler::fill_holes(float f, int minsize)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).fill_holes(f, minsize);
	}
}

void SlicesHandler::fill_holestissue(tissues_size_t f, int minsize)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).fill_holestissue(active_tissuelayer, f, minsize);
	}
}

void SlicesHandler::remove_islands(float f, int minsize)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).remove_islands(f, minsize);
	}
}

void SlicesHandler::remove_islandstissue(tissues_size_t f, int minsize)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).remove_islandstissue(active_tissuelayer, f, minsize);
	}
}

void SlicesHandler::fill_gaps(int minsize, bool connectivity)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).fill_gaps(minsize, connectivity);
	}
}

void SlicesHandler::adaptwork2bmp(float f)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).adaptwork2bmp(f);
	}
}

void SlicesHandler::fill_gapstissue(int minsize, bool connectivity)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i])
			.fill_gapstissue(active_tissuelayer, minsize, connectivity);
	}
}

bool SlicesHandler::value_at_boundary3D(float value)
{
	// Top
	float* tmp = &(image_slices[startslice].return_work()[0]);
	for (unsigned pos = 0; pos < area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	// Sides
	for (unsigned short i = startslice + 1; i < endslice - 1; i++)
	{
		if ((image_slices[i]).value_at_boundary(value))
		{
			return true;
		}
	}

	// Bottom
	tmp = &(image_slices[endslice - 1].return_work()[0]);
	for (unsigned pos = 0; pos < area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	return false;
}

bool SlicesHandler::tissuevalue_at_boundary3D(tissues_size_t value)
{
	// Top
	tissues_size_t* tmp =
		&(image_slices[startslice].return_tissues(active_tissuelayer)[0]);
	for (unsigned pos = 0; pos < area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	// Sides
	for (unsigned short i = startslice + 1; i < endslice - 1; i++)
	{
		if ((image_slices[i])
				.tissuevalue_at_boundary(active_tissuelayer, value))
		{
			return true;
		}
	}

	// Bottom
	tmp = &(image_slices[endslice - 1].return_tissues(active_tissuelayer)[0]);
	for (unsigned pos = 0; pos < area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	return false;
}

float SlicesHandler::add_skin(int i1)
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

		for (unsigned short i = startslice; i < endslice; i++)
		{
			float* bits = image_slices[i].return_work();
			for (unsigned pos = 0; pos < area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const iN = endslice;
#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).add_skin(i1, setto);
	}

	return setto;
}

float SlicesHandler::add_skin_outside(int i1)
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

		for (unsigned short i = startslice; i < endslice; i++)
		{
			float* bits = image_slices[i].return_work();
			for (unsigned pos = 0; pos < area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const iN = endslice;
#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).add_skin_outside(i1, setto);
	}

	return setto;
}

void SlicesHandler::add_skintissue(int i1, tissues_size_t f)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).add_skintissue(active_tissuelayer, i1, f);
	}
}

void SlicesHandler::add_skintissue_outside(int i1, tissues_size_t f)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).add_skintissue_outside(active_tissuelayer, i1, f);
	}
}

void SlicesHandler::fill_unassigned()
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

		for (unsigned short i = startslice; i < endslice; i++)
		{
			float* bits = image_slices[i].return_work();
			for (unsigned pos = 0; pos < area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const iN = endslice;
#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).fill_unassigned(setto);
	}
}

void SlicesHandler::fill_unassignedtissue(tissues_size_t f)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).fill_unassignedtissue(active_tissuelayer, f);
	}
}

void SlicesHandler::kmeans(unsigned short slicenr, short nrtissues,
						   unsigned int iternr, unsigned int converge)
{
	if (slicenr >= startslice && slicenr < endslice)
	{
		KMeans kmeans;
		float* bits[1];
		bits[0] = image_slices[slicenr].return_bmp();
		float weights[1];
		weights[0] = 1;
		kmeans.init(width, height, nrtissues, 1, bits, weights);
		kmeans.make_iter(iternr, converge);
		kmeans.return_m(image_slices[slicenr].return_work());
		image_slices[slicenr].set_mode(2, false);

		for (unsigned short i = startslice; i < slicenr; i++)
		{
			bits[0] = image_slices[i].return_bmp();
			kmeans.apply_to(bits, image_slices[i].return_work());
			image_slices[i].set_mode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < endslice; i++)
		{
			bits[0] = image_slices[i].return_bmp();
			kmeans.apply_to(bits, image_slices[i].return_work());
			image_slices[i].set_mode(2, false);
		}
	}
}

void SlicesHandler::kmeans_mhd(unsigned short slicenr, short nrtissues,
							   short dim, std::vector<std::string> mhdfiles,
							   float* weights, unsigned int iternr,
							   unsigned int converge)
{
	if (mhdfiles.size() + 1 < dim)
		return;
	if (slicenr >= startslice && slicenr < endslice)
	{
		KMeans kmeans;
		float** bits = new float*[dim];
		for (unsigned short j = 0; j + 1 < dim; j++)
		{
			bits[j + 1] = new float[area];
			if (bits[j + 1] == NULL)
			{
				for (unsigned short i = 1; i < j; i++)
					delete[] bits[i];
				delete[] bits;
				return;
			}
		}

		bits[0] = image_slices[slicenr].return_bmp();
		for (unsigned short i = 0; i + 1 < dim; i++)
		{
			if (!ImageReader::getSlice(mhdfiles[i].c_str(), bits[i + 1],
									   slicenr, width, height))
			{
				for (unsigned short j = 1; j < dim; j++)
					delete[] bits[j];
				delete[] bits;
				return;
			}
		}
		kmeans.init(width, height, nrtissues, dim, bits, weights);
		kmeans.make_iter(iternr, converge);
		kmeans.return_m(image_slices[slicenr].return_work());
		image_slices[slicenr].set_mode(2, false);

		for (unsigned short i = startslice; i < slicenr; i++)
		{
			bits[0] = image_slices[i].return_bmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ImageReader::getSlice(mhdfiles[k].c_str(), bits[k + 1], i,
										   width, height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.apply_to(bits, image_slices[i].return_work());
			image_slices[i].set_mode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < endslice; i++)
		{
			bits[0] = image_slices[i].return_bmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ImageReader::getSlice(mhdfiles[k].c_str(), bits[k + 1], i,
										   width, height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.apply_to(bits, image_slices[i].return_work());
			image_slices[i].set_mode(2, false);
		}

		for (unsigned short j = 1; j < dim; j++)
			delete[] bits[j];
		delete[] bits;
	}
}

void SlicesHandler::kmeans_png(unsigned short slicenr, short nrtissues,
							   short dim, std::vector<std::string> pngfiles,
							   std::vector<int> exctractChannel, float* weights,
							   unsigned int iternr, unsigned int converge,
							   const std::string initCentersFile)
{
	if (pngfiles.size() + 1 < dim)
		return;
	if (slicenr >= startslice && slicenr < endslice)
	{
		float** bits = new float*[dim];
		for (unsigned short j = 0; j + 1 < dim; j++)
		{
			bits[j + 1] = new float[area];
			if (bits[j + 1] == NULL)
			{
				for (unsigned short i = 1; i < j; i++)
					delete[] bits[i];
				delete[] bits;
				return;
			}
		}

		bits[0] = image_slices[slicenr].return_bmp();
		for (unsigned short i = 0; i + 1 < dim; i++)
		{
			if (!ChannelExtractor::getSlice(pngfiles[0].c_str(), bits[i + 1],
											exctractChannel[i], slicenr, width,
											height))
			{
				for (unsigned short j = 1; j < dim; j++)
					delete[] bits[j];
				delete[] bits;
				return;
			}
		}

		KMeans kmeans;
		if (initCentersFile != "")
		{
			float* centers = nullptr;
			int dimensions;
			int nrClasses;
			if (kmeans.get_centers_from_file(initCentersFile, centers,
											 dimensions, nrClasses))
			{
				dim = dimensions;
				nrtissues = nrClasses;
				kmeans.init(width, height, nrtissues, dim, bits, weights,
							centers);
			}
			else
			{
				QMessageBox msgBox;
				msgBox.setText("Error reading centers initialization file.");
				msgBox.exec();
				return;
			}
		}
		else
		{
			kmeans.init(width, height, nrtissues, dim, bits, weights);
		}
		kmeans.make_iter(iternr, converge);
		kmeans.return_m(image_slices[slicenr].return_work());
		image_slices[slicenr].set_mode(2, false);

		for (unsigned short i = startslice; i < slicenr; i++)
		{
			bits[0] = image_slices[i].return_bmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ChannelExtractor::getSlice(pngfiles[0].c_str(),
												bits[i + 1], exctractChannel[i],
												i, width, height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.apply_to(bits, image_slices[i].return_work());
			image_slices[i].set_mode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < endslice; i++)
		{
			bits[0] = image_slices[i].return_bmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ChannelExtractor::getSlice(pngfiles[0].c_str(),
												bits[i + 1], exctractChannel[i],
												i, width, height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.apply_to(bits, image_slices[i].return_work());
			image_slices[i].set_mode(2, false);
		}

		for (unsigned short j = 1; j < dim; j++)
			delete[] bits[j];
		delete[] bits;
	}
}

void SlicesHandler::em(unsigned short slicenr, short nrtissues,
					   unsigned int iternr, unsigned int converge)
{
	if (slicenr >= startslice && slicenr < endslice)
	{
		ExpectationMaximization em;
		float* bits[1];
		bits[0] = image_slices[slicenr].return_bmp();
		float weights[1];
		weights[0] = 1;
		em.init(width, height, nrtissues, 1, bits, weights);
		em.make_iter(iternr, converge);
		em.classify(image_slices[slicenr].return_work());
		image_slices[slicenr].set_mode(2, false);

		for (unsigned short i = startslice; i < slicenr; i++)
		{
			bits[0] = image_slices[i].return_bmp();
			em.apply_to(bits, image_slices[i].return_work());
			image_slices[i].set_mode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < endslice; i++)
		{
			bits[0] = image_slices[i].return_bmp();
			em.apply_to(bits, image_slices[i].return_work());
			image_slices[i].set_mode(2, false);
		}
	}
}

void SlicesHandler::aniso_diff(float dt, int n, float (*f)(float, float),
							   float k, float restraint)
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).aniso_diff(dt, n, f, k, restraint);

	return;
}

void SlicesHandler::cont_anisodiff(float dt, int n, float (*f)(float, float),
								   float k, float restraint)
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).cont_anisodiff(dt, n, f, k, restraint);

	return;
}

void SlicesHandler::median_interquartile(bool median)
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).median_interquartile(median);

	return;
}

void SlicesHandler::average(unsigned short n)
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).average(n);

	return;
}

void SlicesHandler::sigmafilter(float sigma, unsigned short nx,
								unsigned short ny)
{
	for (unsigned short i = startslice; i < endslice; i++)
		(image_slices[i]).sigmafilter(sigma, nx, ny);

	return;
}

void SlicesHandler::threshold(float* thresholds)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		(image_slices[i]).threshold(thresholds);
	}
}

void SlicesHandler::extractinterpolatesave_contours(
	int minsize, std::vector<tissues_size_t>& tissuevec, unsigned short between,
	bool dp, float epsilon, const char* filename)
{
	/*	os.clear();
	os.set_sizenr((between+1)*nrslices-between);
	os.set_thickness(thickness/(between+1));*/

	std::vector<std::vector<Point>> v1, v2;
	std::vector<Point_type> vP;

	SlicesHandler dummy3D;
	dummy3D.newbmp((unsigned short)(image_slices[0].return_width()),
				   (unsigned short)(image_slices[0].return_height()),
				   between + 2);
	dummy3D.set_slicethickness(thickness / (between + 1));
	Pair pair1 = get_pixelsize();
	dummy3D.set_pixelsize(pair1.high, pair1.low);

	FILE* fp = dummy3D.save_contourprologue(filename,
											(between + 1) * nrslices - between);

	for (unsigned short j = 0; j + 1 < nrslices; j++)
	{
		dummy3D.copy2tissue(0,
							image_slices[j].return_tissues(active_tissuelayer));
		dummy3D.copy2tissue(between + 1, image_slices[j + 1].return_tissues(
											 active_tissuelayer));
		dummy3D.interpolatetissuegrey(
			0, between + 1); // TODO: Use interpolatetissuegrey_medianset?

		dummy3D.extract_contours(minsize, tissuevec);
		if (dp)
			dummy3D.dougpeuck_line(epsilon);
		fp = dummy3D.save_contoursection(fp, 0, between, (between + 1) * j);
	}

	fp = dummy3D.save_contoursection(fp, 0, 0, (between + 1) * (nrslices - 1));

	fp = dummy3D.save_tissuenamescolors(fp);

	fclose(fp);

	/*	os.clear();
	os.set_sizenr(nrslices);
	os.set_thickness(thickness);*/

	return;
}

void SlicesHandler::extractinterpolatesave_contours2_xmirrored(
	int minsize, std::vector<tissues_size_t>& tissuevec, unsigned short between,
	bool dp, float epsilon, const char* filename)
{
	/*	os.clear();
	os.set_sizenr((between+1)*nrslices-between);
	os.set_thickness(thickness/(between+1));*/

	std::vector<std::vector<Point>> v1, v2;
	std::vector<Point_type> vP;

	SlicesHandler dummy3D;
	dummy3D.newbmp((unsigned short)(image_slices[0].return_width()),
				   (unsigned short)(image_slices[0].return_height()),
				   between + 2);
	dummy3D.set_slicethickness(thickness / (between + 1));
	Pair pair1 = get_pixelsize();
	dummy3D.set_pixelsize(pair1.high / 2, pair1.low / 2);

	FILE* fp = dummy3D.save_contourprologue(filename,
											(between + 1) * nrslices - between);

	for (unsigned short j = 0; j + 1 < nrslices; j++)
	{
		dummy3D.copy2tissue(0,
							image_slices[j].return_tissues(active_tissuelayer));
		dummy3D.copy2tissue(between + 1, image_slices[j + 1].return_tissues(
											 active_tissuelayer));
		dummy3D.interpolatetissuegrey(
			0, between + 1); // TODO: Use interpolatetissuegrey_medianset?

		//		dummy3D.extract_contours2(minsize, tissuevec);
		if (dp)
		{
			//			dummy3D.dougpeuck_line(epsilon*2);
			dummy3D.extract_contours2_xmirrored(minsize, tissuevec, epsilon);
		}
		else
		{
			dummy3D.extract_contours2_xmirrored(minsize, tissuevec);
		}
		dummy3D.shift_contours(-(int)dummy3D.return_width(),
							   -(int)dummy3D.return_height());
		fp = dummy3D.save_contoursection(fp, 0, between, (between + 1) * j);
	}

	fp = dummy3D.save_contoursection(fp, 0, 0, (between + 1) * (nrslices - 1));

	fp = dummy3D.save_tissuenamescolors(fp);

	fclose(fp);

	/*	os.clear();
	os.set_sizenr(nrslices);
	os.set_thickness(thickness);*/

	return;
}

void SlicesHandler::extract_contours(int minsize,
									 std::vector<tissues_size_t>& tissuevec)
{
	os.clear();
	std::vector<std::vector<Point>> v1, v2;
	std::vector<Point_type> vP;

	for (std::vector<tissues_size_t>::iterator it1 = tissuevec.begin();
		 it1 != tissuevec.end(); it1++)
	{
		for (unsigned short i = 0; i < nrslices; i++)
		{
			v1.clear();
			v2.clear();
			(image_slices[i])
				.get_tissuecontours(active_tissuelayer, *it1, &v1, &v2,
									minsize);
			for (std::vector<std::vector<Point>>::iterator it = v1.begin();
				 it != v1.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,true);
				os.add_line(i, *it1, &(*it), true);
			}
			for (std::vector<std::vector<Point>>::iterator it = v2.begin();
				 it != v2.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,false);
				os.add_line(i, *it1, &(*it), false);
			}
		}
	}

	return;
}

void SlicesHandler::extract_contours2_xmirrored(
	int minsize, std::vector<tissues_size_t>& tissuevec)
{
	/*float px=os.get_pixelsize().px;
	float py=os.get_pixelsize().py;
	os.set_pixelsize(p.px/2,p.py/2);*/
	os.clear();
	std::vector<std::vector<Point>> v1, v2;
	std::vector<Point_type> vP;

	for (std::vector<tissues_size_t>::iterator it1 = tissuevec.begin();
		 it1 != tissuevec.end(); it1++)
	{
		for (unsigned short i = 0; i < nrslices; i++)
		{
			v1.clear();
			v2.clear();
			(image_slices[i])
				.get_tissuecontours2_xmirrored(active_tissuelayer, *it1, &v1,
											   &v2, minsize);
			for (std::vector<std::vector<Point>>::iterator it = v1.begin();
				 it != v1.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,true);
				os.add_line(i, *it1, &(*it), true);
			}
			for (std::vector<std::vector<Point>>::iterator it = v2.begin();
				 it != v2.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,false);
				os.add_line(i, *it1, &(*it), false);
			}
		}
	}

	//os.set_pixelsize();

	return;
}

void SlicesHandler::extract_contours2_xmirrored(
	int minsize, std::vector<tissues_size_t>& tissuevec, float epsilon)
{
	/*float px=os.get_pixelsize().px;
	float py=os.get_pixelsize().py;
	os.set_pixelsize(p.px/2,p.py/2);*/
	os.clear();
	std::vector<std::vector<Point>> v1, v2;
	std::vector<Point_type> vP;

	for (std::vector<tissues_size_t>::iterator it1 = tissuevec.begin();
		 it1 != tissuevec.end(); it1++)
	{
		for (unsigned short i = 0; i < nrslices; i++)
		{
			v1.clear();
			v2.clear();
			(image_slices[i])
				.get_tissuecontours2_xmirrored(active_tissuelayer, *it1, &v1,
											   &v2, minsize, epsilon);
			for (std::vector<std::vector<Point>>::iterator it = v1.begin();
				 it != v1.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,true);
				os.add_line(i, *it1, &(*it), true);
			}
			for (std::vector<std::vector<Point>>::iterator it = v2.begin();
				 it != v2.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,false);
				os.add_line(i, *it1, &(*it), false);
			}
		}
	}

	//os.set_pixelsize();

	return;
}

void SlicesHandler::extract_contours(float f, int minsize,
									 tissues_size_t tissuetype)
{
	//	os.clear();
	std::vector<std::vector<Point>> v1, v2;
	std::vector<Point_type> vP;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		v1.clear();
		v2.clear();
		(image_slices[i]).get_contours(f, &v1, &v2, minsize);
		for (std::vector<std::vector<Point>>::iterator it = v1.begin();
			 it != v1.end(); it++)
		{
			//			Pointconvert(*it,vP);
			//			os.add_line(i,tissuetype,&vP,true);
			os.add_line(i, tissuetype, &(*it), true);
		}
		for (std::vector<std::vector<Point>>::iterator it = v2.begin();
			 it != v2.end(); it++)
		{
			//			Pointconvert(*it,vP);
			//			os.add_line(i,tissuetype,&vP,false);
			os.add_line(i, tissuetype, &(*it), false);
		}
	}

	return;
}

/*void SlicesHandler::Pointconvert(std::vector<Point> &v1, std::vector<Point_type> &v2)
{
	v2.clear();
	Point_type P;

	for(std::vector<Point>::iterator it=v1.begin();it!=v1.end();it++){
		P.px=(float)it->px;
		P.py=(float)it->py;
		v2.push_back(P);
	}

	return;
}*/

void SlicesHandler::bmp_sum()
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).bmp_sum();
	}
	return;
}

void SlicesHandler::bmp_add(float f)
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).bmp_add(f);
	}
	return;
}

void SlicesHandler::bmp_diff()
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).bmp_diff();
	}
	return;
}

void SlicesHandler::bmp_mult()
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).bmp_mult();
	}
	return;
}

void SlicesHandler::bmp_mult(float f)
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).bmp_mult(f);
	}
	return;
}

void SlicesHandler::bmp_overlay(float alpha)
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).bmp_overlay(alpha);
	}
	return;
}

void SlicesHandler::bmp_abs()
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).bmp_abs();
	}
	return;
}

void SlicesHandler::bmp_neg()
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).bmp_neg();
	}
	return;
}

void SlicesHandler::scale_colors(Pair p)
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).scale_colors(p);
	}
	return;
}

void SlicesHandler::crop_colors()
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).crop_colors();
	}
	return;
}

void SlicesHandler::get_range(Pair* pp)
{
	Pair p;
	(image_slices[startslice]).get_range(pp);

	for (unsigned short i = startslice + 1; i < endslice; i++)
	{
		(image_slices[i]).get_range(&p);
		if ((*pp).high < p.high)
			(*pp).high = p.high;
		if ((*pp).low > p.low)
			(*pp).low = p.low;
	}

	return;
}

void SlicesHandler::compute_range_mode1(Pair* pp)
{
	// Update ranges for all mode 1 slices and compute total range
	pp->high = 0.0f;
	pp->low = FLT_MAX;

#pragma omp parallel
	{
		// this data is thread private
		Pair p;
		float high = 0.f;
		float low = FLT_MAX;

		int const iN = nrslices;
#pragma omp for
		for (int i = 0; i < iN; i++)
		{
			if (image_slices[i].return_mode(false) == 1)
			{
				(image_slices[i]).get_range(&p);
				slice_ranges[i] = p;
				if (high < p.high)
					high = p.high;
				if (low > p.low)
					low = p.low;
			}
		}

#pragma omp critical
		{
			if (low < pp->low)
				pp->low = low;
			if (high > pp->high)
				pp->high = high;
		}
	}

	if (pp->high == 0.0f && pp->low == FLT_MAX)
	{
		// No mode 1 slices: Set to mode 2 range
		pp->low = 255.0f;
	}
}

void SlicesHandler::compute_range_mode1(unsigned short updateSlicenr, Pair* pp)
{
	// Update range for single mode 1 slice
	if (image_slices[updateSlicenr].return_mode(false) == 1)
	{
		(image_slices[updateSlicenr]).get_range(&slice_ranges[updateSlicenr]);
	}

	// Compute total range
	pp->high = 0.0f;
	pp->low = FLT_MAX;
	for (unsigned short i = 0; i < nrslices; ++i)
	{
		if (image_slices[i].return_mode(false) != 1)
			continue;
		Pair p = slice_ranges[i];
		if (pp->high < p.high)
			pp->high = p.high;
		if (pp->low > p.low)
			pp->low = p.low;
	}

	if (pp->high == 0.0f && pp->low == FLT_MAX)
	{
		// No mode 1 slices: Set to mode 2 range
		pp->low = 255.0f;
	}
}

void SlicesHandler::get_bmprange(Pair* pp)
{
	Pair p;
	(image_slices[startslice]).get_bmprange(pp);

	for (unsigned short i = startslice + 1; i < endslice; i++)
	{
		(image_slices[i]).get_bmprange(&p);
		if ((*pp).high < p.high)
			(*pp).high = p.high;
		if ((*pp).low > p.low)
			(*pp).low = p.low;
	}

	return;
}

void SlicesHandler::compute_bmprange_mode1(Pair* pp)
{
	// Update ranges for all mode 1 slices and compute total range
	pp->high = 0.0f;
	pp->low = FLT_MAX;

#pragma omp parallel
	{
		// this data is thread private
		Pair p;
		float high = 0.f;
		float low = FLT_MAX;

		int const iN = nrslices;
#pragma omp for
		for (int i = 0; i < iN; i++)
		{
			if (image_slices[i].return_mode(true) == 1)
			{
				(image_slices[i]).get_bmprange(&p);
				slice_bmpranges[i] = p;
				if (high < p.high)
					high = p.high;
				if (low > p.low)
					low = p.low;
			}
		}

#pragma omp critical
		{
			if (low < pp->low)
				pp->low = low;
			if (high > pp->high)
				pp->high = high;
		}
	}

	if (pp->high == 0.0f && pp->low == FLT_MAX)
	{
		// No mode 1 slices: Set to mode 2 range
		pp->low = 255.0f;
	}
}

void SlicesHandler::compute_bmprange_mode1(unsigned short updateSlicenr,
										   Pair* pp)
{
	// Update range for single mode 1 slice
	if (image_slices[updateSlicenr].return_mode(true) == 1)
	{
		(image_slices[updateSlicenr])
			.get_bmprange(&slice_bmpranges[updateSlicenr]);
	}

	// Compute total range
	pp->high = 0.0f;
	pp->low = FLT_MAX;
	for (unsigned short i = 0; i < nrslices; ++i)
	{
		if (image_slices[i].return_mode(true) != 1)
			continue;
		Pair p = slice_bmpranges[i];
		if (pp->high < p.high)
			pp->high = p.high;
		if (pp->low > p.low)
			pp->low = p.low;
	}

	if (pp->high == 0.0f && pp->low == FLT_MAX)
	{
		// No mode 1 slices: Set to mode 2 range
		pp->low = 255.0f;
	}
}

void SlicesHandler::get_rangetissue(tissues_size_t* pp)
{
	tissues_size_t p;
	(image_slices[startslice]).get_rangetissue(active_tissuelayer, pp);

	for (unsigned short i = startslice + 1; i < endslice; i++)
	{
		(image_slices[i]).get_rangetissue(active_tissuelayer, &p);
		if ((*pp) < p)
			(*pp) = p;
	}

	return;
}

void SlicesHandler::zero_crossings(bool connectivity)
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i]).zero_crossings(connectivity);
	}
	return;
}

void SlicesHandler::save_contours(const char* filename)
{
	os.print(filename, TissueInfos::GetTissueCount());
	FILE* fp = fopen(filename, "a");
	fp = save_tissuenamescolors(fp);
	fclose(fp);
	return;
}

void SlicesHandler::shift_contours(int dx, int dy)
{
	os.shift_contours(dx, dy);
}

void SlicesHandler::setextrusion_contours(int top, int bottom)
{
	os.set_thickness(bottom * thickness, 0);
	if (nrslices > 1)
	{
		os.set_thickness(top * thickness, nrslices - 1);
	}
}

void SlicesHandler::resetextrusion_contours()
{
	os.set_thickness(thickness, 0);
	if (nrslices > 1)
	{
		os.set_thickness(thickness, nrslices - 1);
	}
}

FILE* SlicesHandler::save_contourprologue(const char* filename,
										  unsigned nr_slices)
{
	return os.printprologue(filename, nr_slices, TissueInfos::GetTissueCount());
}

FILE* SlicesHandler::save_contoursection(FILE* fp, unsigned startslice1,
										 unsigned endslice1, unsigned offset)
{
	os.printsection(fp, startslice1, endslice1, offset,
					TissueInfos::GetTissueCount());
	return fp;
}

FILE* SlicesHandler::save_tissuenamescolors(FILE* fp)
{
	tissues_size_t tissueCount = TissueInfos::GetTissueCount();
	TissueInfoStruct* tissueInfo;

	fprintf(fp, "NT%u\n", tissueCount);

	if (tissueCount > 255)
	{ // Only print tissue indices which contain outlines

		// Collect used tissue indices in ascending order
		std::set<tissues_size_t> tissueIndices;
		os.insert_tissue_indices(tissueIndices);
		std::set<tissues_size_t>::iterator idxIt;
		for (idxIt = tissueIndices.begin(); idxIt != tissueIndices.end();
			 ++idxIt)
		{
			tissueInfo = TissueInfos::GetTissueInfo(*idxIt);
			fprintf(fp, "T%i %f %f %f %s\n", (int)*idxIt, tissueInfo->color[0],
					tissueInfo->color[1], tissueInfo->color[2],
					tissueInfo->name.ascii());
		}
	}
	else
	{
		// Print infos of all tissues
		for (unsigned i = 1; i <= tissueCount; i++)
		{
			tissueInfo = TissueInfos::GetTissueInfo(i);
			fprintf(fp, "T%i %f %f %f %s\n", (int)i, tissueInfo->color[0],
					tissueInfo->color[1], tissueInfo->color[2],
					tissueInfo->name.ascii());
		}
	}

	return fp;
}

void SlicesHandler::dougpeuck_line(float epsilon)
{
	os.doug_peuck(epsilon, true);
}

void SlicesHandler::hysteretic(float thresh_low, float thresh_high,
							   bool connectivity, unsigned short nrpasses)
{
	float setvalue = 255;
	unsigned short slicenr = startslice;

	clear_work();

	image_slices[slicenr].hysteretic(thresh_low, thresh_high, connectivity,
									 setvalue);
	for (unsigned short i = 0; i < nrpasses; i++)
	{
		while (++slicenr < endslice)
		{
			image_slices[slicenr].hysteretic(
				thresh_low, thresh_high, connectivity,
				image_slices[slicenr - 1].return_work(), setvalue - 1,
				setvalue);
		}
		setvalue++;
		slicenr--;
		while (slicenr-- > startslice)
		{
			image_slices[slicenr].hysteretic(
				thresh_low, thresh_high, connectivity,
				image_slices[slicenr + 1].return_work(), setvalue - 1,
				setvalue);
		}
		setvalue++;
		slicenr = startslice;
	}

	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 255 - f_tol;
	swap_bmpworkall();
	threshold(thresh);
}

void SlicesHandler::thresholded_growing(short unsigned slicenr, Point p,
										float threshfactor_low,
										float threshfactor_high,
										bool connectivity,
										unsigned short nrpasses)
{
	if (slicenr < endslice && slicenr >= startslice)
	{
		float setvalue = 255;
		Pair tp;

		clear_work();

		image_slices[slicenr].thresholded_growing(p, threshfactor_low,
												  threshfactor_high,
												  connectivity, setvalue, &tp);

		for (unsigned short i = 0; i < nrpasses; i++)
		{
			while (++slicenr < endslice)
			{
				image_slices[slicenr].thresholded_growing(
					tp.low, tp.high, connectivity,
					image_slices[slicenr - 1].return_work(), setvalue - 1,
					setvalue);
			}
			setvalue++;
			slicenr--;
			while (slicenr-- > startslice)
			{
				image_slices[slicenr].thresholded_growing(
					tp.low, tp.high, connectivity,
					image_slices[slicenr + 1].return_work(), setvalue - 1,
					setvalue);
			}
			setvalue++;
			slicenr = startslice;
		}

		float thresh[2];
		thresh[0] = 1;
		thresh[1] = 255 - f_tol;
		swap_bmpworkall();
		threshold(thresh);
	}
}

void SlicesHandler::thresholded_growing(
	short unsigned slicenr, Point p, float thresh_low, float thresh_high,
	float set_to) //bool connectivity,float set_to)
{
	if (slicenr >= startslice && slicenr < endslice)
	{
		unsigned position = p.px + p.py * (unsigned)width;
		std::vector<posit> s;
		posit p1;

		for (unsigned short z = startslice; z < endslice; z++)
		{
			float* work = image_slices[z].return_work();
			float* bmp = image_slices[z].return_bmp();
			int i = 0;
			for (unsigned short j = 0; j < height; j++)
			{
				for (unsigned short k = 0; k < width; k++)
				{
					if (bmp[i] > thresh_high || bmp[i] < thresh_low)
						work[i] = 0;
					else
						work[i] = -1;
					i++;
				}
			}
		}

		p1.pxy = position;
		p1.pz = slicenr;

		s.push_back(p1);
		float* work = image_slices[slicenr].return_work();
		work[position] = set_to;

		//	hysteretic_growth(results,&s,width+2,height+2,connectivity,set_to);
		posit i, j;

		while (!s.empty())
		{
			i = s.back();
			s.pop_back();

			work = image_slices[i.pz].return_work();
			if (i.pxy % width != 0 && work[i.pxy - 1] == -1)
			{
				work[i.pxy - 1] = set_to;
				j.pxy = i.pxy - 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			if ((i.pxy + 1) % width != 0 && work[i.pxy + 1] == -1)
			{
				work[i.pxy + 1] = set_to;
				j.pxy = i.pxy + 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pxy >= width && work[i.pxy - width] == -1)
			{
				work[i.pxy - width] = set_to;
				j.pxy = i.pxy - width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pxy < area - width && work[i.pxy + width] == -1)
			{
				work[i.pxy + width] = set_to;
				j.pxy = i.pxy + width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pz > startslice)
			{
				work = image_slices[i.pz - 1].return_work();
				if (work[i.pxy] == -1)
				{
					work[i.pxy] = set_to;
					j.pxy = i.pxy;
					j.pz = i.pz - 1;
					s.push_back(j);
				}
				//			if(connectivity){
				//			}
			}
			if (i.pz + 1 < endslice)
			{
				work = image_slices[i.pz + 1].return_work();
				if (work[i.pxy] == -1)
				{
					work[i.pxy] = set_to;
					j.pxy = i.pxy;
					j.pz = i.pz + 1;
					s.push_back(j);
				}
				//			if(connectivity){
				//			}
			}
			/*		if(connectivity){
			if(i.pxy%width!=0&&work[i.pxy-width-1]==-1) {work[i.pxy-w-1]=set_to; s.push(i-w-1);}
			if(i.pxy%width!=0&&work[i.pxy+width+1]==-1) {work[i.pxy+w+1]=set_to; s.push(i+w+1);}
			if(i.pxy%width!=0&&work[i.pxy-width+1]==-1) {work[i.pxy-w+1]=set_to; s.push(i-w+1);}
			if(i.pxy%width!=0&&work[i.pxy+width-1]==-1) {work[i.pxy+w-1]=set_to; s.push(i+w-1);}
			}*/
		}

		for (unsigned short z = startslice; z < endslice; z++)
		{
			work = image_slices[z].return_work();
			for (unsigned i1 = 0; i1 < area; i1++)
				if (work[i1] == -1)
					work[i1] = 0;
			image_slices[z].set_mode(2, false);
		}
	}

	return;
}

void SlicesHandler::add2tissueall_connected(tissues_size_t tissuetype, Point p,
											bool override)
{
	if (activeslice >= startslice && activeslice < endslice)
	{
		float set_to = (float)123E10;
		unsigned position = p.px + p.py * (unsigned)width;
		std::vector<posit> s;
		posit p1;

		p1.pxy = position;
		p1.pz = activeslice;

		s.push_back(p1);
		float* work = image_slices[activeslice].return_work();
		float f = work[position];
		tissues_size_t* tissue =
			image_slices[activeslice].return_tissues(active_tissuelayer);
		bool tissueLocked = TissueInfos::GetTissueLocked(tissue[position]);
		if (tissue[position] == 0 || (override && tissueLocked == false))
			tissue[position] = tissuetype;
		if (tissue[position] == 0 || (override && tissueLocked == false))
			work[position] = set_to;

		posit i, j;

		while (!s.empty())
		{
			i = s.back();
			s.pop_back();

			work = image_slices[i.pz].return_work();
			tissue = image_slices[i.pz].return_tissues(active_tissuelayer);
			//if(i.pxy%width!=0&&work[i.pxy-1]==f&&(override||tissue[i.pxy-1]==0)) {
			if (i.pxy % width != 0 && work[i.pxy - 1] == f &&
				(tissue[i.pxy - 1] == 0 ||
				 (override &&
				  TissueInfos::GetTissueLocked(tissue[i.pxy - 1]) == false)))
			{
				work[i.pxy - 1] = set_to;
				tissue[i.pxy - 1] = tissuetype;
				j.pxy = i.pxy - 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			//if((i.pxy+1)%width!=0&&work[i.pxy+1]==f&&(override||tissue[i.pxy+1]==0)) {
			if ((i.pxy + 1) % width != 0 && work[i.pxy + 1] == f &&
				(tissue[i.pxy + 1] == 0 ||
				 (override &&
				  TissueInfos::GetTissueLocked(tissue[i.pxy + 1]) == false)))
			{
				work[i.pxy + 1] = set_to;
				tissue[i.pxy + 1] = tissuetype;
				j.pxy = i.pxy + 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			//if(i.pxy>=width&&work[i.pxy-width]==f&&(override||tissue[i.pxy-width]==0)) {
			if (i.pxy >= width && work[i.pxy - width] == f &&
				(tissue[i.pxy - width] == 0 ||
				 (override && TissueInfos::GetTissueLocked(
								  tissue[i.pxy - width]) == false)))
			{
				work[i.pxy - width] = set_to;
				tissue[i.pxy - width] = tissuetype;
				j.pxy = i.pxy - width;
				j.pz = i.pz;
				s.push_back(j);
			}
			//if(i.pxy<=area-width&&work[i.pxy+width]==f&&(override||tissue[i.pxy+width]==0)) {
			if (i.pxy < area - width && work[i.pxy + width] == f &&
				(tissue[i.pxy + width] == 0 ||
				 (override && TissueInfos::GetTissueLocked(
								  tissue[i.pxy + width]) == false)))
			{
				work[i.pxy + width] = set_to;
				tissue[i.pxy + width] = tissuetype;
				j.pxy = i.pxy + width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pz > startslice)
			{
				work = image_slices[i.pz - 1].return_work();
				tissue =
					image_slices[i.pz - 1].return_tissues(active_tissuelayer);
				//if(work[i.pxy]==f&&(override||tissue[i.pxy]==0)) {
				if (work[i.pxy] == f &&
					(tissue[i.pxy] == 0 ||
					 (override &&
					  TissueInfos::GetTissueLocked(tissue[i.pxy]) == false)))
				{
					work[i.pxy] = set_to;
					tissue[i.pxy] = tissuetype;
					j.pxy = i.pxy;
					j.pz = i.pz - 1;
					s.push_back(j);
				}
			}
			if (i.pz + 1 < endslice)
			{
				work = image_slices[i.pz + 1].return_work();
				tissue =
					image_slices[i.pz + 1].return_tissues(active_tissuelayer);
				//if(work[i.pxy]==f&&(override||tissue[i.pxy]==0)) {
				if (work[i.pxy] == f &&
					(tissue[i.pxy] == 0 ||
					 (override &&
					  TissueInfos::GetTissueLocked(tissue[i.pxy]) == false)))
				{
					work[i.pxy] = set_to;
					tissue[i.pxy] = tissuetype;
					j.pxy = i.pxy;
					j.pz = i.pz + 1;
					s.push_back(j);
				}
			}
		}

		for (unsigned short z = startslice; z < endslice; z++)
		{
			work = image_slices[z].return_work();
			for (unsigned i1 = 0; i1 < area; i1++)
				if (work[i1] == set_to)
					work[i1] = f;
		}
	}

	return;
}

void SlicesHandler::subtract_tissueall_connected(tissues_size_t tissuetype,
												 Point p)
{
	if (activeslice < endslice && activeslice >= startslice)
	{
		float set_to = (float)123E10;
		unsigned position = p.px + p.py * (unsigned)width;
		std::vector<posit> s;
		posit p1;

		p1.pxy = position;
		p1.pz = activeslice;

		s.push_back(p1);
		float* work = image_slices[activeslice].return_work();
		float f = work[position];
		tissues_size_t* tissue =
			image_slices[activeslice].return_tissues(active_tissuelayer);
		if (tissue[position] == tissuetype)
			tissue[position] = tissuetype;
		if (tissue[position] == tissuetype)
			work[position] = set_to;

		posit i, j;

		while (!s.empty())
		{
			i = s.back();
			s.pop_back();

			work = image_slices[i.pz].return_work();
			tissue = image_slices[i.pz].return_tissues(active_tissuelayer);
			if (i.pxy % width != 0 && work[i.pxy - 1] == f &&
				tissue[i.pxy - 1] == tissuetype)
			{
				work[i.pxy - 1] = set_to;
				tissue[i.pxy - 1] = 0;
				j.pxy = i.pxy - 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			if ((i.pxy + 1) % width != 0 && work[i.pxy + 1] == f &&
				tissue[i.pxy + 1] == tissuetype)
			{
				work[i.pxy + 1] = set_to;
				tissue[i.pxy + 1] = 0;
				j.pxy = i.pxy + 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pxy >= width && work[i.pxy - width] == f &&
				tissue[i.pxy - width] == tissuetype)
			{
				work[i.pxy - width] = set_to;
				tissue[i.pxy - width] = 0;
				j.pxy = i.pxy - width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pxy < area - width && work[i.pxy + width] == f &&
				tissue[i.pxy + width] == tissuetype)
			{
				work[i.pxy + width] = set_to;
				tissue[i.pxy + width] = 0;
				j.pxy = i.pxy + width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pz > startslice)
			{
				work = image_slices[i.pz - 1].return_work();
				tissue =
					image_slices[i.pz - 1].return_tissues(active_tissuelayer);
				if (work[i.pxy] == f && tissue[i.pxy] == tissuetype)
				{
					work[i.pxy] = set_to;
					tissue[i.pxy] = 0;
					j.pxy = i.pxy;
					j.pz = i.pz - 1;
					s.push_back(j);
				}
				//			if(connectivity){
				//			}
			}
			if (i.pz + 1 < endslice)
			{
				work = image_slices[i.pz + 1].return_work();
				tissue =
					image_slices[i.pz + 1].return_tissues(active_tissuelayer);
				if (work[i.pxy] == f && tissue[i.pxy] == tissuetype)
				{
					work[i.pxy] = set_to;
					tissue[i.pxy] = 0;
					j.pxy = i.pxy;
					j.pz = i.pz + 1;
					s.push_back(j);
				}
				//			if(connectivity){
				//			}
			}
		}

		for (unsigned short z = startslice; z < endslice; z++)
		{
			work = image_slices[z].return_work();
			for (unsigned i1 = 0; i1 < area; i1++)
				if (work[i1] == set_to)
					work[i1] = f;
		}
	}

	return;
}

void SlicesHandler::double_hysteretic(float thresh_low_l, float thresh_low_h,
									  float thresh_high_l, float thresh_high_h,
									  bool connectivity,
									  unsigned short nrpasses)
{
	float setvalue = 255;
	unsigned short slicenr = startslice;

	clear_work();

	image_slices[slicenr].double_hysteretic(thresh_low_l, thresh_low_h,
											thresh_high_l, thresh_high_h,
											connectivity, setvalue);
	//	if(nrslices>1) {
	for (unsigned short i = 0; i < nrpasses; i++)
	{
		while (++slicenr < endslice)
		{
			image_slices[slicenr].double_hysteretic(
				thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h,
				connectivity, image_slices[slicenr - 1].return_work(),
				setvalue - 1, setvalue);
		}
		setvalue++;
		slicenr--;
		while (slicenr-- > startslice)
		{
			image_slices[slicenr].double_hysteretic(
				thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h,
				connectivity, image_slices[slicenr + 1].return_work(),
				setvalue - 1, setvalue);
		}
		setvalue++;
		slicenr = 0;
	}
	//	}

	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 255 - f_tol;
	swap_bmpworkall();
	threshold(thresh);

	//xxxa again. what happens to bmp? and what should we do with mode?

	return;
}

void SlicesHandler::double_hysteretic_allslices(float thresh_low_l,
												float thresh_low_h,
												float thresh_high_l,
												float thresh_high_h,
												bool connectivity, float set_to)
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		(image_slices[i])
			.double_hysteretic(thresh_low_l, thresh_low_h, thresh_high_l,
							   thresh_high_h, connectivity, set_to);
	}
}

void SlicesHandler::erosion(int n, bool connectivity)
{
#if 0
	for(unsigned short i=startslice;i<endslice;i++){
		(image_slices[i]).erosion(n,connectivity);
	}
#else
	auto all_slices = GetImage(iseg::SliceHandlerInterface::kTarget, false);

	itk::Size<3> radius;
	radius.Fill(n);

	auto ball = morpho::MakeBall(radius);

	auto output = morpho::MorphologicalOperation<float>(
		all_slices, ball, morpho::kErode, startslice, endslice);

	morpho::Paste<float>(output, all_slices, startslice, endslice);
#endif
}

void SlicesHandler::dilation(int n, bool connectivity)
{
#if 0
	for(unsigned short i=startslice;i<endslice;i++){
		(image_slices[i]).dilation(n,connectivity);
	}
#else
	auto all_slices = GetImage(iseg::SliceHandlerInterface::kTarget, false);

	itk::Size<3> radius;
	radius.Fill(n);

	auto ball = morpho::MakeBall(radius);

	auto output = morpho::MorphologicalOperation<float>(
		all_slices, ball, morpho::kDilate, startslice, endslice);

	morpho::Paste<float>(output, all_slices, startslice, endslice);
#endif
}

void SlicesHandler::closure(int n, bool connectivity)
{
#if 0
	for(unsigned short i=startslice;i<endslice;i++){
		(image_slices[i]).closure(n,connectivity);
	}
#else
	auto all_slices = GetImage(iseg::SliceHandlerInterface::kTarget, false);

	itk::Size<3> radius;
	radius.Fill(n);

	auto ball = morpho::MakeBall(radius);

	auto output = morpho::MorphologicalOperation<float>(
		all_slices, ball, morpho::kClose, startslice, endslice);

	morpho::Paste<float>(output, all_slices, startslice, endslice);
#endif
}

void SlicesHandler::open(int n, bool connectivity)
{
#if 0
	for(unsigned short i=startslice;i<endslice;i++){
		(image_slices[i]).open(n,connectivity);
	}
#else
	auto all_slices = GetImage(iseg::SliceHandlerInterface::kTarget, false);

	itk::Size<3> radius;
	radius.Fill(n);

	auto ball = morpho::MakeBall(radius);

	auto output = morpho::MorphologicalOperation<float>(
		all_slices, ball, morpho::kOpen, startslice, endslice);

	morpho::Paste<float>(output, all_slices, startslice, endslice);
#endif
}

void SlicesHandler::interpolateworkgrey(unsigned short slice1,
										unsigned short slice2)
{
	if (slice2 < slice1)
	{
		unsigned short dummy = slice1;
		slice1 = slice2;
		slice2 = dummy;
	}

	const short n = slice2 - slice1;

	if (n > 0)
	{
		image_slices[slice1].pushstack_bmp();
		image_slices[slice2].pushstack_bmp();

		image_slices[slice1].swap_bmpwork();
		image_slices[slice2].swap_bmpwork();

		image_slices[slice2].dead_reckoning();
		image_slices[slice1].dead_reckoning();

		float* bmp1 = image_slices[slice1].return_bmp();
		float* bmp2 = image_slices[slice2].return_bmp();
		float* work1 = image_slices[slice1].return_work();
		float* work2 = image_slices[slice2].return_work();

		Point p;
		float prop;
		unsigned short n1;
		unsigned i1 = 0;

		for (p.py = 0; p.py < height; p.py++)
		{
			for (p.px = 0; p.px < width; p.px++)
			{
				if (work2[i1] + work1[i1] != 0)
					prop = work1[i1] / (work2[i1] + work1[i1]);
				else
					prop = 0.5f;
				n1 = (unsigned short)n * prop;
				for (unsigned short j = 1; j <= n1 && j < n; j++)
				{
					image_slices[slice1 + j].set_work_pt(p, bmp1[i1]);
				}
				for (unsigned short j = n1 + 1; j < n; j++)
				{
					image_slices[slice1 + j].set_work_pt(p, bmp2[i1]);
				}
				i1++;
			}
		}

		image_slices[slice1].swap_bmpwork();
		image_slices[slice2].swap_bmpwork();

		for (unsigned short j = 1; j < n; j++)
		{
			image_slices[slice1 + j].set_mode(2, false);
		}

		image_slices[slice2].popstack_bmp();
		image_slices[slice1].popstack_bmp();
	}
}

void SlicesHandler::interpolateworkgrey_medianset(unsigned short slice1,
												  unsigned short slice2,
												  bool connectivity,
												  bool handleVanishingComp)
{
	// Beucher et al.: "Sets, Partitions and Functions Interpolations"
	// Modified algorithm: Do not interplate vanishing components

	if (slice2 < slice1)
	{
		unsigned short dummy = slice1;
		slice1 = slice2;
		slice2 = dummy;
	}

	const short n = slice2 - slice1;

	if (n > 1)
	{
		const short slicehalf = slice1 + (short)(0.5f * n);
		unsigned int nCells = 0;
		Pair work_range;
		image_slices[slice1].get_range(&work_range);
		nCells = std::max(nCells, (unsigned int)(work_range.high + 0.5f));
		image_slices[slice2].get_range(&work_range);
		nCells = std::max(nCells, (unsigned int)(work_range.high + 0.5f));
		unsigned short max_iterations =
			(unsigned short)std::sqrt((float)(width * width + height * height));

		// Backup images
		image_slices[slice1].pushstack_work();
		image_slices[slice2].pushstack_work();
		image_slices[slice1].pushstack_bmp();
		image_slices[slicehalf].pushstack_bmp();
		image_slices[slice2].pushstack_bmp();

		// Interplation input to bmp
		image_slices[slice1].swap_bmpwork();
		image_slices[slice2].swap_bmpwork();

		// Input images
		float* f_1 = image_slices[slice1].return_bmp();
		float* f_2 = image_slices[slice2].return_bmp();

		if (handleVanishingComp)
		{
			// Determine forward and backward vanishing components
			std::set<float> vanishing_comp_forward;
			std::set<float> vanishing_comp_backward;
			float* connected_comp_forward =
				(float*)malloc(sizeof(float) * area);
			float* connected_comp_backward =
				(float*)malloc(sizeof(float) * area);

			image_slices[slice1].connected_components(
				false, vanishing_comp_forward); // TODO: connectivity?
			image_slices[slice1].copyfromwork(connected_comp_forward);

			image_slices[slice2].connected_components(
				false, vanishing_comp_backward); // TODO: connectivity?
			image_slices[slice2].copyfromwork(connected_comp_backward);

			for (unsigned int i = 0; i < area; ++i)
			{
				if (f_1[i] == f_2[i])
				{
					std::set<float>::iterator iter =
						vanishing_comp_forward.find(connected_comp_forward[i]);
					if (iter != vanishing_comp_forward.end())
					{
						vanishing_comp_forward.erase(iter);
					}
					iter = vanishing_comp_backward.find(
						connected_comp_backward[i]);
					if (iter != vanishing_comp_backward.end())
					{
						vanishing_comp_backward.erase(iter);
					}
				}
			}

			// Remove vanishing components for interpolation
			tissues_size_t* tissue1 =
				image_slices[slice1].return_tissues(active_tissuelayer);
			tissues_size_t* tissue2 =
				image_slices[slice2].return_tissues(active_tissuelayer);
			for (unsigned int i = 0; i < area; ++i)
			{
				std::set<float>::iterator iter =
					vanishing_comp_forward.find(connected_comp_forward[i]);
				if (iter != vanishing_comp_forward.end())
				{
					f_2[i] = f_1[i];
				}
				else
				{
					iter = vanishing_comp_backward.find(
						connected_comp_backward[i]);
					if (iter != vanishing_comp_backward.end())
					{
						f_2[i] = f_1[i];
					}
				}
			}

			free(connected_comp_forward);
			free(connected_comp_backward);
		}

		// Interpolation results
		float* g_i = image_slices[slicehalf].return_work();
		float* gp_i = image_slices[slicehalf].return_bmp();

		// Initialize g_0 and gp_0
		for (unsigned int i = 0; i < area; ++i)
		{
			if (f_1[i] == f_2[i])
			{
				g_i[i] = f_1[i];
				gp_i[i] = (float)nCells - g_i[i];
			}
			else
			{
				g_i[i] = 0.0f;
				gp_i[i] = 0.0f;
			}
		}

		// Interpolation
		bool idempotence = false;
		for (unsigned short iter = 0; iter < max_iterations && !idempotence;
			 iter++)
		{
			// Dilate g_i and gp_i --> g_i_B and gp_i_B
			if (iter % 2 == 0)
			{
				image_slices[slice1].copy2work(g_i, 1);
				image_slices[slice1].dilation(1, connectivity);
			}
			else
			{
				image_slices[slice2].copy2work(gp_i, 1);
				image_slices[slice2].dilation(1, connectivity);
			}
			float* g_i_B = image_slices[slice1].return_work();
			float* gp_i_B = image_slices[slice2].return_work();

			// Compute g_i+1 and gp_i+1
			idempotence = true;
			if (iter % 2 == 0)
			{
				for (unsigned int i = 0; i < area; ++i)
				{
					if (g_i_B[i] + gp_i_B[i] <= nCells)
					{
						float tmp = std::max(g_i_B[i], g_i[i]);
						idempotence &= (g_i[i] == tmp);
						g_i[i] = tmp;
					}
				}
			}
			else
			{
				for (unsigned int i = 0; i < area; ++i)
				{
					if (g_i_B[i] + gp_i_B[i] <= nCells)
					{
						float tmp = std::max(gp_i_B[i], gp_i[i]);
						idempotence &= (gp_i[i] == tmp);
						gp_i[i] = tmp;
					}
				}
			}
		}

		if (handleVanishingComp)
		{
			image_slices[slice1].copy2work(f_1, 1);
			image_slices[slice2].copy2work(f_2, 1);

			// Restore images
			image_slices[slice2].popstack_bmp();
			image_slices[slicehalf].popstack_bmp();
			image_slices[slice1].popstack_bmp();

			// Recursion
			interpolateworkgrey_medianset(slice1, slicehalf, connectivity,
										  false);
			interpolateworkgrey_medianset(slicehalf, slice2, connectivity,
										  false);

			// Restore images
			image_slices[slice2].popstack_work();
			image_slices[slice1].popstack_work();
		}
		else
		{
			// Restore images
			image_slices[slice2].popstack_bmp();
			image_slices[slicehalf].popstack_bmp();
			image_slices[slice1].popstack_bmp();
			image_slices[slice2].popstack_work();
			image_slices[slice1].popstack_work();

			// Recursion
			interpolateworkgrey_medianset(slice1, slicehalf, connectivity,
										  false);
			interpolateworkgrey_medianset(slicehalf, slice2, connectivity,
										  false);
		}
	}
}

void SlicesHandler::interpolatetissuegrey(unsigned short slice1,
										  unsigned short slice2)
{
	if (slice2 < slice1)
	{
		unsigned short dummy = slice1;
		slice1 = slice2;
		slice2 = dummy;
	}

	const short n = slice2 - slice1;

	if (n > 0)
	{
		image_slices[slice1].pushstack_bmp();
		image_slices[slice2].pushstack_bmp();
		image_slices[slice1].pushstack_work();
		image_slices[slice2].pushstack_work();

		image_slices[slice1].tissue2work(active_tissuelayer);
		image_slices[slice2].tissue2work(active_tissuelayer);

		image_slices[slice1].swap_bmpwork();
		image_slices[slice2].swap_bmpwork();

		image_slices[slice2].dead_reckoning();
		image_slices[slice1].dead_reckoning();

		tissues_size_t* bmp1 =
			image_slices[slice1].return_tissues(active_tissuelayer);
		tissues_size_t* bmp2 =
			image_slices[slice2].return_tissues(active_tissuelayer);
		float* work1 = image_slices[slice1].return_work();
		float* work2 = image_slices[slice2].return_work();

		Point p;
		float prop;
		unsigned short n1;
		unsigned i1 = 0;

		for (p.py = 0; p.py < height; p.py++)
		{
			for (p.px = 0; p.px < width; p.px++)
			{
				if (work2[i1] + work1[i1] != 0)
					prop = work1[i1] / (work2[i1] + work1[i1]);
				else
					prop = 0.5f;
				n1 = (unsigned short)n * prop;
				for (unsigned short j = 1; j <= n1 && j < n; j++)
				{
					image_slices[slice1 + j].set_tissue_pt(active_tissuelayer,
														   p, bmp1[i1]);
				}
				for (unsigned short j = n1 + 1; j < n; j++)
				{
					image_slices[slice1 + j].set_tissue_pt(active_tissuelayer,
														   p, bmp2[i1]);
				}
				i1++;
			}
		}

		for (unsigned short j = 1; j < n; j++)
		{
			image_slices[slice1 + j].set_mode(2, false);
		}

		image_slices[slice1].popstack_work();
		image_slices[slice2].popstack_work();
		image_slices[slice2].popstack_bmp();
		image_slices[slice1].popstack_bmp();
	}
}

#if 0
void SlicesHandler::interpolatetissuegrey_medianset(unsigned short slice1, unsigned short slice2, bool connectivity)
{
	// Beucher et al.: "Sets, Partitions and Functions Interpolations"
	// Original algorithm: Unsound interpolation of vanishing components

	if(slice2<slice1){
		unsigned short dummy=slice1;
		slice1=slice2;
		slice2=dummy;
	}

	const short n=slice2-slice1;

	if(n>1){
		const short slicehalf = slice1 + (short)(0.5f*n);
		unsigned int nCells = TISSUES_SIZE_MAX;
		unsigned short max_iterations = (unsigned short)std::sqrt((float)(width*width+height*height));

		// Backup images
		image_slices[slice1].pushstack_work();
		image_slices[slicehalf].pushstack_work();
		image_slices[slice2].pushstack_work();
		image_slices[slice1].pushstack_bmp();
		image_slices[slicehalf].pushstack_bmp();
		image_slices[slice2].pushstack_bmp();

		// Interplation input to bmp
		image_slices[slice1].tissue2work(active_tissuelayer);
		image_slices[slice2].tissue2work(active_tissuelayer);
		image_slices[slice1].swap_bmpwork();
		image_slices[slice2].swap_bmpwork();

		// Interpolation results
		float *g_i=image_slices[slicehalf].return_work();
		float *gp_i=image_slices[slicehalf].return_bmp();

		// Initialize g_0 and gp_0
		float *f_1=image_slices[slice1].return_bmp();
		float *f_2=image_slices[slice2].return_bmp();
		for (unsigned int i = 0; i < area; ++i) {
			if (f_1[i] == f_2[i]) {
				g_i[i] = f_1[i];
				gp_i[i] = (float)nCells - g_i[i];
			} else {
				g_i[i] = 0.0f;
				gp_i[i] = 0.0f;
			}
		}

		// Interpolation
		bool idempotence = false;
		for(unsigned short iter = 0; iter < max_iterations && !idempotence; iter++) {

			// Dilate g_i and gp_i --> g_i_B and gp_i_B
			if (iter%2 == 0) {
				image_slices[slice1].copy2work(g_i, 1);
				image_slices[slice1].dilation(1, connectivity);
			} else {
				image_slices[slice2].copy2work(gp_i, 1);
				image_slices[slice2].dilation(1, connectivity);
			}
			float *g_i_B=image_slices[slice1].return_work();
			float *gp_i_B=image_slices[slice2].return_work();

			// Compute g_i+1 and gp_i+1
			idempotence = true;
			if (iter%2 == 0) {
				for (unsigned int i = 0; i < area; ++i) {
					if (g_i_B[i] + gp_i_B[i] <= nCells) {
						float tmp = max(g_i_B[i], g_i[i]);
						idempotence &= (g_i[i] == tmp);
						g_i[i] = tmp;
					}
				}
			} else {
				for (unsigned int i = 0; i < area; ++i) {
					if (g_i_B[i] + gp_i_B[i] <= nCells) {
						float tmp = max(gp_i_B[i], gp_i[i]);
						idempotence &= (gp_i[i] == tmp);
						gp_i[i] = tmp;
					}
				}
			}
		}

		// Assign tissues
		image_slices[slicehalf].work2tissue(active_tissuelayer);

		// Restore images
		image_slices[slice2].popstack_bmp();
		image_slices[slicehalf].popstack_bmp();
		image_slices[slice1].popstack_bmp();
		image_slices[slice2].popstack_work();
		image_slices[slicehalf].popstack_work();
		image_slices[slice1].popstack_work();

		// Recursion
		interpolatetissuegrey_medianset(slice1, slicehalf, connectivity);
		interpolatetissuegrey_medianset(slicehalf, slice2, connectivity);
	}
}
#endif

void SlicesHandler::interpolatetissuegrey_medianset(unsigned short slice1,
													unsigned short slice2,
													bool connectivity,
													bool handleVanishingComp)
{
	// Beucher et al.: "Sets, Partitions and Functions Interpolations"
	// Modified algorithm: Do not interplate vanishing components

	if (slice2 < slice1)
	{
		unsigned short dummy = slice1;
		slice1 = slice2;
		slice2 = dummy;
	}

	const short n = slice2 - slice1;

	if (n > 1)
	{
		const short slicehalf = slice1 + (short)(0.5f * n);
		unsigned int nCells = TISSUES_SIZE_MAX;
		unsigned short max_iterations =
			(unsigned short)std::sqrt((float)(width * width + height * height));

		// Backup images
		image_slices[slice1].pushstack_work();
		image_slices[slicehalf].pushstack_work();
		image_slices[slice2].pushstack_work();
		image_slices[slice1].pushstack_bmp();
		image_slices[slicehalf].pushstack_bmp();
		image_slices[slice2].pushstack_bmp();
		tissues_size_t* tissue1_copy = NULL;
		tissues_size_t* tissue2_copy = NULL;
		if (handleVanishingComp)
		{
			tissue1_copy =
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
			image_slices[slice1].copyfromtissue(active_tissuelayer,
												tissue1_copy);
			tissue2_copy =
				(tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
			image_slices[slice2].copyfromtissue(active_tissuelayer,
												tissue2_copy);
		}

		// Interplation input to bmp
		image_slices[slice1].tissue2work(active_tissuelayer);
		image_slices[slice2].tissue2work(active_tissuelayer);
		image_slices[slice1].swap_bmpwork();
		image_slices[slice2].swap_bmpwork();

		// Input images
		float* f_1 = image_slices[slice1].return_bmp();
		float* f_2 = image_slices[slice2].return_bmp();

		if (handleVanishingComp)
		{
			// Determine forward and backward vanishing components
			std::set<float> vanishing_comp_forward;
			std::set<float> vanishing_comp_backward;
			float* connected_comp_forward =
				(float*)malloc(sizeof(float) * area);
			float* connected_comp_backward =
				(float*)malloc(sizeof(float) * area);

			image_slices[slice1].connected_components(
				false, vanishing_comp_forward); // TODO: connectivity?
			image_slices[slice1].copyfromwork(connected_comp_forward);

			image_slices[slice2].connected_components(
				false, vanishing_comp_backward); // TODO: connectivity?
			image_slices[slice2].copyfromwork(connected_comp_backward);

			for (unsigned int i = 0; i < area; ++i)
			{
				if (f_1[i] == f_2[i])
				{
					std::set<float>::iterator iter =
						vanishing_comp_forward.find(connected_comp_forward[i]);
					if (iter != vanishing_comp_forward.end())
					{
						vanishing_comp_forward.erase(iter);
					}
					iter = vanishing_comp_backward.find(
						connected_comp_backward[i]);
					if (iter != vanishing_comp_backward.end())
					{
						vanishing_comp_backward.erase(iter);
					}
				}
			}

			// Remove vanishing components for interpolation
			tissues_size_t* tissue1 =
				image_slices[slice1].return_tissues(active_tissuelayer);
			tissues_size_t* tissue2 =
				image_slices[slice2].return_tissues(active_tissuelayer);
			for (unsigned int i = 0; i < area; ++i)
			{
				std::set<float>::iterator iter =
					vanishing_comp_forward.find(connected_comp_forward[i]);
				if (iter != vanishing_comp_forward.end())
				{
					tissue2[i] = tissue1[i];
				}
				else
				{
					iter = vanishing_comp_backward.find(
						connected_comp_backward[i]);
					if (iter != vanishing_comp_backward.end())
					{
						tissue2[i] = tissue1[i];
					}
				}
			}

			free(connected_comp_forward);
			free(connected_comp_backward);

			// Interplation modified input to bmp
			image_slices[slice1].tissue2work(active_tissuelayer);
			image_slices[slice2].tissue2work(active_tissuelayer);
			image_slices[slice1].swap_bmpwork();
			image_slices[slice2].swap_bmpwork();

			// Input images
			f_1 = image_slices[slice1].return_bmp();
			f_2 = image_slices[slice2].return_bmp();
		}

		// Interpolation results
		float* g_i = image_slices[slicehalf].return_work();
		float* gp_i = image_slices[slicehalf].return_bmp();

		// Initialize g_0 and gp_0
		for (unsigned int i = 0; i < area; ++i)
		{
			if (f_1[i] == f_2[i])
			{
				g_i[i] = f_1[i];
				gp_i[i] = (float)nCells - g_i[i];
			}
			else
			{
				g_i[i] = 0.0f;
				gp_i[i] = 0.0f;
			}
		}

		// Interpolation
		bool idempotence = false;
		for (unsigned short iter = 0; iter < max_iterations && !idempotence;
			 iter++)
		{
			// Dilate g_i and gp_i --> g_i_B and gp_i_B
			if (iter % 2 == 0)
			{
				image_slices[slice1].copy2work(g_i, 1);
				image_slices[slice1].dilation(1, connectivity);
			}
			else
			{
				image_slices[slice2].copy2work(gp_i, 1);
				image_slices[slice2].dilation(1, connectivity);
			}
			float* g_i_B = image_slices[slice1].return_work();
			float* gp_i_B = image_slices[slice2].return_work();

			// Compute g_i+1 and gp_i+1
			idempotence = true;
			if (iter % 2 == 0)
			{
				for (unsigned int i = 0; i < area; ++i)
				{
					if (g_i_B[i] + gp_i_B[i] <= nCells)
					{
						float tmp = std::max(g_i_B[i], g_i[i]);
						idempotence &= (g_i[i] == tmp);
						g_i[i] = tmp;
					}
				}
			}
			else
			{
				for (unsigned int i = 0; i < area; ++i)
				{
					if (g_i_B[i] + gp_i_B[i] <= nCells)
					{
						float tmp = std::max(gp_i_B[i], gp_i[i]);
						idempotence &= (gp_i[i] == tmp);
						gp_i[i] = tmp;
					}
				}
			}
		}

		// Assign tissues
		image_slices[slicehalf].work2tissue(active_tissuelayer);

		// Restore images
		image_slices[slice2].popstack_bmp();
		image_slices[slicehalf].popstack_bmp();
		image_slices[slice1].popstack_bmp();
		image_slices[slice2].popstack_work();
		image_slices[slicehalf].popstack_work();
		image_slices[slice1].popstack_work();

		// Recursion
		interpolatetissuegrey_medianset(slice1, slicehalf, connectivity, false);
		interpolatetissuegrey_medianset(slicehalf, slice2, connectivity, false);

		// Restore tissues
		if (handleVanishingComp)
		{
			image_slices[slice1].copy2tissue(active_tissuelayer, tissue1_copy);
			image_slices[slice2].copy2tissue(active_tissuelayer, tissue2_copy);
			free(tissue1_copy);
			free(tissue2_copy);
		}
	}
}

void SlicesHandler::interpolatetissue(unsigned short slice1,
									  unsigned short slice2,
									  tissues_size_t tissuetype)
{
	if (slice2 < slice1)
	{
		unsigned short dummy = slice1;
		slice1 = slice2;
		slice2 = dummy;
	}

	tissues_size_t* tissue1 =
		image_slices[slice1].return_tissues(active_tissuelayer);
	tissues_size_t* tissue2 =
		image_slices[slice2].return_tissues(active_tissuelayer);
	image_slices[slice1].pushstack_bmp();
	image_slices[slice2].pushstack_bmp();
	float* bmp1 = image_slices[slice1].return_bmp();
	float* bmp2 = image_slices[slice2].return_bmp();
	for (unsigned int i = 0; i < area; i++)
	{
		bmp1[i] = (float)tissue1[i];
		bmp2[i] = (float)tissue2[i];
	}

	image_slices[slice2].dead_reckoning((float)tissuetype);
	image_slices[slice1].dead_reckoning((float)tissuetype);

	bmp1 = image_slices[slice1].return_work();
	bmp2 = image_slices[slice2].return_work();
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < height; p.py++)
		{
			for (p.px = 0; p.px < width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				for (unsigned short j = 1; j < n; j++)
				{
					if (bmp1[i1] + delta * j >= 0)
						image_slices[slice1 + j].set_work_pt(p, 255.0f);
					else
						image_slices[slice1 + j].set_work_pt(p, 0.0f);
				}
				i1++;
			}
		}
	}

	for (unsigned i = 0; i < area; i++)
	{
		if (bmp1[i] < 0)
			bmp1[i] = 0;
		else
			bmp1[i] = 255.0f;
		if (bmp2[i] < 0)
			bmp2[i] = 0;
		else
			bmp2[i] = 255.0f;
	}

	for (unsigned short j = 1; j < n; j++)
	{
		image_slices[slice1 + j].set_mode(2, false);
	}

	image_slices[slice2].popstack_bmp();
	image_slices[slice1].popstack_bmp();
}

void SlicesHandler::interpolatetissue_medianset(unsigned short slice1,
												unsigned short slice2,
												tissues_size_t tissuetype,
												bool connectivity,
												bool handleVanishingComp)
{
	image_slices[slice1].tissue2work(active_tissuelayer, tissuetype);
	image_slices[slice2].tissue2work(active_tissuelayer, tissuetype);
	interpolateworkgrey_medianset(slice1, slice2, connectivity, true);
}

void SlicesHandler::extrapolatetissue(unsigned short origin1,
									  unsigned short origin2,
									  unsigned short target,
									  tissues_size_t tissuetype)
{
	if (origin2 < origin1)
	{
		unsigned short dummy = origin1;
		origin1 = origin2;
		origin2 = dummy;
	}

	tissues_size_t* tissue1 =
		image_slices[origin1].return_tissues(active_tissuelayer);
	tissues_size_t* tissue2 =
		image_slices[origin2].return_tissues(active_tissuelayer);
	image_slices[origin1].pushstack_bmp();
	image_slices[origin2].pushstack_bmp();
	image_slices[origin1].pushstack_work();
	image_slices[origin2].pushstack_work();
	float* bmp1 = image_slices[origin1].return_bmp();
	float* bmp2 = image_slices[origin2].return_bmp();
	for (unsigned int i = 0; i < area; i++)
	{
		bmp1[i] = (float)tissue1[i];
		bmp2[i] = (float)tissue2[i];
	}

	image_slices[origin1].dead_reckoning((float)tissuetype);
	image_slices[origin2].dead_reckoning((float)tissuetype);

	bmp1 = image_slices[origin1].return_work();
	bmp2 = image_slices[origin2].return_work();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < height; p.py++)
		{
			for (p.px = 0; p.px < width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				if (bmp1[i1] + delta * (target - origin1) >= 0)
					image_slices[target].set_work_pt(p, 255.0f);
				else
					image_slices[target].set_work_pt(p, 0.0f);
				i1++;
			}
		}
	}

	/*for(unsigned i=0;i<area;i++){
		if(bmp1[i]<0) bmp1[i]=0;
		else bmp1[i]=255.0f;
		if(bmp2[i]<0) bmp2[i]=0;
		else bmp2[i]=255.0f;
	}*/

	image_slices[target].set_mode(2, false);

	image_slices[origin2].popstack_work();
	image_slices[origin1].popstack_work();
	image_slices[origin2].popstack_bmp();
	image_slices[origin1].popstack_bmp();
}

void SlicesHandler::interpolatework(unsigned short slice1,
									unsigned short slice2)
{
	if (slice2 < slice1)
	{
		unsigned short dummy = slice1;
		slice1 = slice2;
		slice2 = dummy;
	}

	//tissues_size_t *tissue1=image_slices[slice1].return_tissues(active_tissuelayer);
	//tissues_size_t *tissue2=image_slices[slice2].return_tissues(active_tissuelayer);
	image_slices[slice1].pushstack_bmp();
	image_slices[slice2].pushstack_bmp();
	float* bmp1 = image_slices[slice1].return_bmp();
	float* bmp2 = image_slices[slice2].return_bmp();
	float* work1 = image_slices[slice1].return_work();
	float* work2 = image_slices[slice2].return_work();

	for (unsigned int i = 0; i < area; i++)
	{
		if (work1[i] != 0)
			bmp1[i] = 255.0f;
		else
			bmp1[i] = 0.0f;
		if (work2[i] != 0)
			bmp2[i] = 255.0f;
		else
			bmp2[i] = 0.0f;
	}

	image_slices[slice2].dead_reckoning(255.0f);
	image_slices[slice1].dead_reckoning(255.0f);

	bmp1 = image_slices[slice1].return_work();
	bmp2 = image_slices[slice2].return_work();
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < height; p.py++)
		{
			for (p.px = 0; p.px < width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				for (unsigned short j = 1; j < n; j++)
				{
					if (bmp1[i1] + delta * j >= 0)
						image_slices[slice1 + j].set_work_pt(p, 255.0f);
					else
						image_slices[slice1 + j].set_work_pt(p, 0.0f);
				}
				i1++;
			}
		}
	}

	for (unsigned i = 0; i < area; i++)
	{
		if (bmp1[i] < 0)
			bmp1[i] = 0;
		else
			bmp1[i] = 255.0f;
		if (bmp2[i] < 0)
			bmp2[i] = 0;
		else
			bmp2[i] = 255.0f;
	}

	for (unsigned short j = 1; j < n; j++)
	{
		image_slices[slice1 + j].set_mode(2, false);
	}

	image_slices[slice2].popstack_bmp();
	image_slices[slice1].popstack_bmp();
}

void SlicesHandler::extrapolatework(unsigned short origin1,
									unsigned short origin2,
									unsigned short target)
{
	if (origin2 < origin1)
	{
		unsigned short dummy = origin1;
		origin1 = origin2;
		origin2 = dummy;
	}

	//tissues_size_t *tissue1=image_slices[origin1].return_tissues(active_tissuelayer);
	//tissues_size_t *tissue2=image_slices[origin2].return_tissues(active_tissuelayer);
	image_slices[origin1].pushstack_bmp();
	image_slices[origin2].pushstack_bmp();
	image_slices[origin1].pushstack_work();
	image_slices[origin2].pushstack_work();
	float* bmp1 = image_slices[origin1].return_bmp();
	float* bmp2 = image_slices[origin2].return_bmp();
	float* work1 = image_slices[origin1].return_work();
	float* work2 = image_slices[origin2].return_work();

	for (unsigned int i = 0; i < area; i++)
	{
		if (work1[i] != 0)
			bmp1[i] = 255.0f;
		else
			bmp1[i] = 0.0f;
		if (work2[i] != 0)
			bmp2[i] = 255.0f;
		else
			bmp2[i] = 0.0f;
	}

	image_slices[origin2].dead_reckoning(255.0f);
	image_slices[origin1].dead_reckoning(255.0f);

	bmp1 = image_slices[origin1].return_work();
	bmp2 = image_slices[origin2].return_work();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < height; p.py++)
		{
			for (p.px = 0; p.px < width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				if (bmp1[i1] + delta * (target - origin1) >= 0)
					image_slices[target].set_work_pt(p, 255.0f);
				else
					image_slices[target].set_work_pt(p, 0.0f);
				i1++;
			}
		}
	}

	/*for(unsigned i=0;i<area;i++){
		if(bmp1[i]<0) bmp1[i]=0;
		else bmp1[i]=255.0f;
		if(bmp2[i]<0) bmp2[i]=0;
		else bmp2[i]=255.0f;
	}*/

	image_slices[target].set_mode(2, false);

	image_slices[origin2].popstack_work();
	image_slices[origin1].popstack_work();
	image_slices[origin2].popstack_bmp();
	image_slices[origin1].popstack_bmp();
}

void SlicesHandler::interpolate(unsigned short slice1, unsigned short slice2)
{
	float* bmp1 = image_slices[slice1].return_work();
	float* bmp2 = image_slices[slice2].return_work();
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < height; p.py++)
		{
			for (p.px = 0; p.px < width; p.px++)
			{
				delta = (bmp2[i] - bmp1[i]) / n;
				for (unsigned short j = 1; j < n; j++)
					image_slices[slice1 + j].set_work_pt(p,
														 bmp1[i] + delta * j);
				i++;
			}
		}
	}

	for (unsigned short j = 1; j < n; j++)
	{
		image_slices[slice1 + j].set_mode(1, false);
	}

	return;
}

void SlicesHandler::extrapolate(unsigned short origin1, unsigned short origin2,
								unsigned short target)
{
	float* bmp1 = image_slices[origin1].return_work();
	float* bmp2 = image_slices[origin2].return_work();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i = 0;

	for (p.py = 0; p.py < height; p.py++)
	{
		for (p.px = 0; p.px < width; p.px++)
		{
			delta = (bmp2[i] - bmp1[i]) / n;
			image_slices[target].set_work_pt(p, bmp1[i] +
													delta * (target - origin1));
			i++;
		}
	}

	image_slices[target].set_mode(1, false);

	return;
}

void SlicesHandler::interpolate(unsigned short slice1, unsigned short slice2,
								float* bmp1, float* bmp2)
{
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i = 0;

	for (p.py = 0; p.py < height; p.py++)
	{
		for (p.px = 0; p.px < width; p.px++)
		{
			delta = (bmp2[i] - bmp1[i]) / n;
			for (unsigned short j = 0; j <= n; j++)
				image_slices[slice1 + j].set_work_pt(p, bmp1[i] + delta * j);
			i++;
		}
	}

	for (unsigned short j = 1; j < n; j++)
	{
		image_slices[slice1 + j].set_mode(1, false);
	}

	return;
}

void SlicesHandler::set_slicethickness(float t)
{
	thickness = t;
	for (unsigned short i = 0; i < nrslices; i++)
	{
		os.set_thickness(t, i);
	}

	return;
}

float SlicesHandler::get_slicethickness() { return thickness; }

void SlicesHandler::set_pixelsize(float dx1, float dy1)
{
	dx = dx1;
	dy = dy1;
	os.set_pixelsize(dx, dy);
	return;
}

Pair SlicesHandler::get_pixelsize()
{
	Pair p;
	p.high = dx;
	p.low = dy;
	return p;
}

Transform SlicesHandler::get_transform() const { return transform; }

Transform SlicesHandler::get_transform_active_slices() const
{
	int plo[3] = {0, 0, -startslice};
	float spacing[3] = {dx, dy, thickness};

	Transform tr_corrected(transform);
	tr_corrected.paddingUpdateTransform(plo, spacing);
	return tr_corrected;
}

void SlicesHandler::set_transform(const Transform& tr) { transform = tr; }

void SlicesHandler::get_spacing(float spacing[3]) const
{
	spacing[0] = dx;
	spacing[1] = dy;
	spacing[2] = thickness;
}

void SlicesHandler::get_displacement(float disp[3]) const
{
	transform.getOffset(disp);
}

void SlicesHandler::set_displacement(const float disp[3])
{
	transform.setOffset(disp);
}

void SlicesHandler::get_direction_cosines(float dc[6]) const
{
	for (unsigned short i = 0; i < 3; i++)
	{
		dc[i] = transform[i][0];
		dc[i + 3] = transform[i][1];
	}
}

void SlicesHandler::set_direction_cosines(const float dc[6])
{
	float offset[3];
	transform.getOffset(offset);
	transform.setTransform(offset, dc);
}

void SlicesHandler::slicebmp_x(float* return_bits, unsigned short xcoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		dummy = image_slices[i].return_bmp();
		for (unsigned short j = 0; j < height; j++)
		{
			return_bits[n] = dummy[j * width + xcoord];
			n++;
		}
	}

	return;
}

void SlicesHandler::slicebmp_y(float* return_bits, unsigned short ycoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		dummy = image_slices[i].return_bmp();
		for (unsigned short j = 0; j < width; j++)
		{
			return_bits[n] = dummy[j + ycoord * width];
			n++;
		}
	}

	return;
}

float* SlicesHandler::slicebmp_x(unsigned short xcoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(height) * nrslices);
	slicebmp_x(result, xcoord);

	return result;
}

float* SlicesHandler::slicebmp_y(unsigned short ycoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(width) * nrslices);
	slicebmp_y(result, ycoord);

	return result;
}

void SlicesHandler::slicework_x(float* return_bits, unsigned short xcoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		dummy = image_slices[i].return_work();
		for (unsigned short j = 0; j < height; j++)
		{
			return_bits[n] = dummy[j * width + xcoord];
			n++;
		}
	}

	return;
}

void SlicesHandler::slicework_y(float* return_bits, unsigned short ycoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		dummy = image_slices[i].return_work();
		for (unsigned short j = 0; j < width; j++)
		{
			return_bits[n] = dummy[j + ycoord * width];
			n++;
		}
	}

	return;
}

float* SlicesHandler::slicework_x(unsigned short xcoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(height) * nrslices);
	slicework_x(result, xcoord);

	return result;
}

float* SlicesHandler::slicework_y(unsigned short ycoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(width) * nrslices);
	slicework_y(result, ycoord);

	return result;
}

void SlicesHandler::slicetissue_x(tissues_size_t* return_bits,
								  unsigned short xcoord)
{
	unsigned n = 0;
	tissues_size_t* dummy;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		dummy = image_slices[i].return_tissues(active_tissuelayer);
		for (unsigned short j = 0; j < height; j++)
		{
			return_bits[n] = dummy[j * width + xcoord];
			n++;
		}
	}

	return;
}

void SlicesHandler::slicetissue_y(tissues_size_t* return_bits,
								  unsigned short ycoord)
{
	unsigned n = 0;
	tissues_size_t* dummy;

	for (unsigned short i = 0; i < nrslices; i++)
	{
		dummy = image_slices[i].return_tissues(active_tissuelayer);
		for (unsigned short j = 0; j < width; j++)
		{
			return_bits[n] = dummy[j + ycoord * width];
			n++;
		}
	}

	return;
}

tissues_size_t* SlicesHandler::slicetissue_x(unsigned short xcoord)
{
	tissues_size_t* result = (tissues_size_t*)malloc(
		sizeof(tissues_size_t) * unsigned(height) * nrslices);
	slicetissue_x(result, xcoord);

	return result;
}

tissues_size_t* SlicesHandler::slicetissue_y(unsigned short ycoord)
{
	tissues_size_t* result = (tissues_size_t*)malloc(
		sizeof(tissues_size_t) * unsigned(width) * nrslices);
	slicetissue_y(result, ycoord);

	return result;
}

void SlicesHandler::slicework_z(unsigned short slicenr)
{
	image_slices[slicenr].return_work();
	return;
}

template<typename TScalarType>
int GetScalarType()
{
	assert("This type is not implemented" && 0);
	return VTK_VOID;
}
template<>
int GetScalarType<unsigned char>() { return VTK_UNSIGNED_CHAR; }
template<>
int GetScalarType<unsigned short>() { return VTK_UNSIGNED_SHORT; }

int SlicesHandler::extract_tissue_surfaces(
	const QString& filename, std::vector<tissues_size_t>& tissuevec,
	bool usediscretemc, float ratio, unsigned smoothingiterations,
	float passBand, float featureAngle)
{
	int error_counter = 0;
	cerr << "SlicesHandler::extract_tissue_surfaces" << endl;
	cerr << "\tratio " << ratio << endl;
	cerr << "\tsmoothingiterations " << smoothingiterations << endl;
	cerr << "\tfeatureAngle " << featureAngle << endl;
	cerr << "\tusediscretemc " << usediscretemc << endl;

	//
	// Copy label field into a vtkImageData object
	//
	const char* tissueIndexArrayName = "Domain";	 // this can be changed
	const char* tissueNameArrayName = "TissueNames"; // don't modify this
	const char* tissueColorArrayName = "Colors";	 // don't modify this

	vtkSmartPointer<vtkImageData> labelField =
		vtkSmartPointer<vtkImageData>::New();
	labelField->SetExtent(0, (int)return_width() + 1, 0,
						  (int)return_height() + 1, 0,
						  (int)(endslice - startslice) + 1);
	Pair ps = get_pixelsize();
	labelField->SetSpacing(ps.high, ps.low, get_slicethickness());
	// transform (translation and rotation) is applied at end of function
	labelField->SetOrigin(0, 0, 0);
	labelField->AllocateScalars(GetScalarType<tissues_size_t>(), 1);
	vtkDataArray* arr = labelField->GetPointData()->GetScalars();
	if (!arr)
	{
		cerr << "ERROR: no scalars" << endl;
		return -1;
	}
	arr->SetName(tissueIndexArrayName);
	labelField->GetPointData()->SetActiveScalars(tissueIndexArrayName);

	//
	// Copy tissue names and colors into labelField
	//
	tissues_size_t num_tissues = TissueInfos::GetTissueCount();
	vtkSmartPointer<vtkStringArray> names_array =
		vtkSmartPointer<vtkStringArray>::New();
	names_array->SetNumberOfTuples(num_tissues + 1);
	names_array->SetName(tissueNameArrayName);

	vtkSmartPointer<vtkFloatArray> color_array =
		vtkSmartPointer<vtkFloatArray>::New();
	color_array->SetNumberOfComponents(3);
	color_array->SetNumberOfTuples(num_tissues + 1);
	color_array->SetName(tissueColorArrayName);

	for (tissues_size_t i = 1; i < num_tissues; i++)
	{
		check_equal(TissueInfos::GetTissueType(TissueInfos::GetTissueName(i)),
					i);
		names_array->SetValue(i, TissueInfos::GetTissueName(i).ascii());
		float* color = TissueInfos::GetTissueColor(i);
		color_array->SetTuple(i, color);
	}

	labelField->GetFieldData()->AddArray(names_array);
	labelField->GetFieldData()->AddArray(color_array);

	tissues_size_t* field =
		(tissues_size_t*)labelField->GetScalarPointer(0, 0, 0);
	if (!field)
	{
		cerr << "ERROR: null pointer" << endl;
		return -1;
	}

	int const padding = 1;
	//labelField is already cropped
	unsigned short nrSlice = 0;
	for (unsigned short i = startslice; i < endslice; i++, nrSlice++)
	{
		copyfromtissuepadded(
			i,
			&(field[(nrSlice + 1) * (unsigned long long)(return_width() + 2) *
					(return_height() + 2)]),
			padding);
	}
	for (unsigned long long i = 0;
		 i < (unsigned long long)(return_width() + 2) * (return_height() + 2);
		 i++)
		field[i] = 0;
	for (unsigned long long i = (endslice - startslice + 1) *
								(unsigned long long)(return_width() + 2) *
								(return_height() + 2);
		 i < (endslice - startslice + 2) *
				 (unsigned long long)(return_width() + 2) *
				 (return_height() + 2);
		 i++)
		field[i] = 0;

	// Check the label field
	check(labelField != 0);
	//check( labelField->GetPointData()->HasArray(tissueIndexArrayName) );
	check(labelField->GetFieldData()->HasArray(tissueNameArrayName));
	check(labelField->GetFieldData()->HasArray(tissueColorArrayName));

	//
	// Now extract the surface from the label field
	//
	std::cerr << "Extract surface " << std::endl;
	vtkSmartPointer<vtkImageExtractCompatibleMesher> contour =
		vtkSmartPointer<vtkImageExtractCompatibleMesher>::New();
	vtkSmartPointer<vtkDiscreteMarchingCubes> cubes =
		vtkSmartPointer<vtkDiscreteMarchingCubes>::New();
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> smoother =
		vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	vtkSmartPointer<vtkEdgeCollapse> simplify =
		vtkSmartPointer<vtkEdgeCollapse>::New();

	vtkPolyData* output = NULL;
	if (usediscretemc)
	{
		cubes->SetInputData(labelField);
		cubes->SetComputeNormals(0);
		cubes->SetComputeGradients(0);
		//cubes->SetComputeScalars(0);
		cubes->SetNumberOfContours((int)tissuevec.size());
		for (size_t i = 0; i < tissuevec.size(); i++)
			cubes->SetValue((int)i, tissuevec[i]);
		cubes->Update();
		output = cubes->GetOutput();
	}
	else
	{
		contour->SetInputData(labelField);
		contour->SetOutputScalarName(tissueIndexArrayName);
		contour->UseTemplatesOn();
		contour->UseOctreeLocatorOff();
		contour->FiveTetrahedraPerVoxelOn();
		contour->SetBackgroundLabel(
			0); /// \todo this will not be extracted! is this correct?
		contour->Update();
		output = contour->GetOutput();
	}
	if (output)
	{
		//check( output->GetCellData()->HasArray(tissueIndexArrayName) );
		check(output->GetFieldData()->HasArray(tissueNameArrayName));
		check(output->GetFieldData()->HasArray(tissueColorArrayName));
	}

	//
	// Smooth surface
	//
	std::cerr << "Smooth surface " << std::endl;
	if (smoothingiterations > 0)
	{
		smoother->SetInputData(output);
		smoother->BoundarySmoothingOff();
		smoother->NonManifoldSmoothingOn();
		smoother->NormalizeCoordinatesOn();
		//smoother->FeatureEdgeSmoothingOn();
		//smoother->SetFeatureAngle(featureAngle);
		smoother->SetPassBand(passBand);
		smoother->SetNumberOfIterations(smoothingiterations);
		smoother->Update();
		output = smoother->GetOutput();
	}
	if (output)
	{
		//check( output->GetCellData()->HasArray(tissueIndexArrayName) );
		check(output->GetFieldData()->HasArray(tissueNameArrayName));
		check(output->GetFieldData()->HasArray(tissueColorArrayName));
	}

	//
	// Simplify surface
	// This originally used vtkDecimatePro (see svn rev. 2453 or earlier),
	// however, vtkEdgeCollapse avoids topological errors and self-intersections.
	//
	std::cerr << "Simplify surface " << std::endl;
	if (ratio < 0.02)
		ratio = 0.02;
	double targetReduction = 1.0 - ratio;
	// don't bother if below reduction rate of 5%
	if (targetReduction > 0.05)
	{
		double cellBnds[6];
		labelField->GetCellBounds(0, cellBnds);
		double x0[3] = {cellBnds[0], cellBnds[2], cellBnds[4]};
		double x1[3] = {cellBnds[1], cellBnds[3], cellBnds[5]};
		// cellSize is related to current edge length
		double cellSize =
			0.5 * std::sqrt(vtkMath::Distance2BetweenPoints(x0, x1));

		// min edge length -> cellSize  :  no edges will be collapsed, no edge shorter than 0
		// min edge length -> inf:  all edges will be collapsed, ""
		// If edge length is halved, number of triangles multiplies by 4
		double minEdgeLength = cellSize / (ratio * ratio);

		simplify->SetInputData(output);
		simplify->SetDomainLabelName(tissueIndexArrayName);
		simplify->SetMinimumEdgeLength(minEdgeLength);
		simplify->FlipEdgesOn();
		simplify->SetIntersectionCheckLevel(0);
		simplify->Update();
		output = simplify->GetOutput();

		// Case 65858: Set name and color info when the collapsed exporting tissue is only one
		if (tissuevec.size() == 1)
		{
			vtkSmartPointer<vtkStringArray> names_array_1 =
				vtkSmartPointer<vtkStringArray>::New();
			names_array_1->SetNumberOfTuples(1);
			names_array_1->SetName(tissueNameArrayName);

			vtkSmartPointer<vtkFloatArray> color_array_1 =
				vtkSmartPointer<vtkFloatArray>::New();
			color_array_1->SetNumberOfComponents(3);
			color_array_1->SetNumberOfTuples(1);
			color_array_1->SetName(tissueColorArrayName);

			for (tissues_size_t i = 1; i < num_tissues; i++)
			{
				float* color = TissueInfos::GetTissueColor(i);
				check_equal(
					TissueInfos::GetTissueType(TissueInfos::GetTissueName(i)),
					i);
				if (i == tissuevec[0])
				{
					names_array_1->SetValue(
						0, TissueInfos::GetTissueName(i).ascii());
					color_array_1->SetTuple(0, color);
				}
			}

			output->GetFieldData()->AddArray(names_array_1);
			output->GetFieldData()->AddArray(color_array_1);
		}
	}
	if (output)
	{
		//check( output->GetCellData()->HasArray(tissueIndexArrayName) );
		check(output->GetFieldData()->HasArray(tissueNameArrayName));
		check(output->GetFieldData()->HasArray(tissueColorArrayName));
	}

	//
	// Transform surface
	//
	std::vector<double> elems(16, 0.0);
	elems.back() = 1.0;
	double padding_displacement[3] = {-padding * ps.high, -padding * ps.low,
									  -padding * thickness +
										  thickness * startslice};
	for (int i = 0; i < 3; i++)
	{
		elems[i * 4 + 0] = transform[i][0];
		elems[i * 4 + 1] = transform[i][1];
		elems[i * 4 + 2] = transform[i][2];
		elems[i * 4 + 3] = transform[i][3] + padding_displacement[i];
	}

	vtkNew<vtkTransform> transform;
	transform->SetMatrix(elems.data());

	vtkNew<vtkTransformPolyDataFilter> transform_filter;
	transform_filter->SetInputData(output);
	transform_filter->SetTransform(transform.Get());

	//
	// Write output to file
	//
	std::cerr << "Save surface " << std::endl;
	vtkNew<vtkGenericDataSetWriter> writer;
	writer->SetFileName(filename.toAscii().data());
	writer->SetInputConnection(transform_filter->GetOutputPort());
	writer->SetMaterialArrayName(tissueIndexArrayName);
	writer->Write();

	return error_counter;
}

void SlicesHandler::triangulate(const char* filename,
								std::vector<tissues_size_t>& tissuevec)
{
	tissues_size_t** tissuebits = (tissues_size_t**)malloc(
		sizeof(tissues_size_t*) * (endslice - startslice));
	for (unsigned short i = startslice; i < endslice; i++)
		tissuebits[i - startslice] =
			image_slices[i].return_tissues(active_tissuelayer);

	std::vector<std::string> vstring;
	std::vector<RGB> colorvec;
	TissueInfoStruct* tissueInfo;
	for (std::vector<tissues_size_t>::iterator it = tissuevec.begin();
		 it != tissuevec.end(); it++)
	{
		tissueInfo = TissueInfos::GetTissueInfo(*it);
		QString a = tissueInfo->name;
		std::string b = std::string(tissueInfo->name.ascii());
		vstring.push_back((std::string)(tissueInfo->name.ascii()));
		RGB dummy;
		dummy.r = tissueInfo->color[0];
		dummy.g = tissueInfo->color[1];
		dummy.b = tissueInfo->color[2];
		colorvec.push_back(dummy);
	}

	MarchingCubes mc;
	mc.init(tissuebits, width, height, endslice - startslice, thickness, dx,
			dy);
	mc.marchingcubeprint(filename, tissuevec, colorvec, vstring);

	free(tissuebits);
	return;
}

void SlicesHandler::triangulate(const char* filename,
								std::vector<tissues_size_t>& tissuevec,
								std::vector<RGB>& colorvec)
{
	tissues_size_t** tissuebits = (tissues_size_t**)malloc(
		sizeof(tissues_size_t*) * (endslice - startslice));
	for (unsigned short i = startslice; i < endslice; i++)
		tissuebits[i - startslice] =
			image_slices[i].return_tissues(active_tissuelayer);

	std::vector<std::string> vstring;
	for (std::vector<tissues_size_t>::iterator it = tissuevec.begin();
		 it != tissuevec.end(); it++)
	{
		vstring.push_back((std::string)TissueInfos::GetTissueName(
			*it + 1)); // TODO: why +1???
	}

	MarchingCubes mc;
	mc.init(tissuebits, width, height, endslice - startslice, thickness, dx,
			dy);
	mc.marchingcubeprint(filename, tissuevec, colorvec, vstring);

	free(tissuebits);
	return;
}

void SlicesHandler::triangulatesimpl(const char* filename,
									 std::vector<tissues_size_t>& tissuevec,
									 float ratio)
{
	//QGenTriangleMesh Mesh;
	tissues_size_t** tissuebits = (tissues_size_t**)malloc(
		sizeof(tissues_size_t*) * (endslice - startslice));
	for (unsigned short i = startslice; i < endslice; i++)
		tissuebits[i - startslice] =
			image_slices[i].return_tissues(active_tissuelayer);

	std::vector<std::string> vstring;
	std::vector<RGB> colorvec;
	TissueInfoStruct* tissueInfo;
	for (std::vector<tissues_size_t>::iterator it = tissuevec.begin();
		 it != tissuevec.end(); it++)
	{
		tissueInfo = TissueInfos::GetTissueInfo(*it);
		QString a = tissueInfo->name;
		std::string b = std::string(tissueInfo->name.ascii());
		vstring.push_back((std::string)(tissueInfo->name.ascii()));
		RGB dummy;
		dummy.r = tissueInfo->color[0];
		dummy.g = tissueInfo->color[1];
		dummy.b = tissueInfo->color[2];
		colorvec.push_back(dummy);
	}

	MarchingCubes mc;
	mc.init(tissuebits, width, height, endslice - startslice, thickness, dx,
			dy);
	mc.marchingcubeprint(filename, tissuevec, colorvec, vstring);

	free(tissuebits);
	return;
}

void SlicesHandler::add2tissue(tissues_size_t tissuetype, Point p,
							   bool override)
{
	image_slices[activeslice].add2tissue(active_tissuelayer, tissuetype, p,
										 override);
}

void SlicesHandler::add2tissue(tissues_size_t tissuetype, bool* mask,
							   unsigned short slicenr, bool override)
{
	image_slices[slicenr].add2tissue(active_tissuelayer, tissuetype, mask,
									 override);
}

void SlicesHandler::add2tissueall(tissues_size_t tissuetype, Point p,
								  bool override)
{
	float f = image_slices[activeslice].work_pt(p);
	add2tissueall(tissuetype, f, override);
}

void SlicesHandler::add2tissue_connected(tissues_size_t tissuetype, Point p,
										 bool override)
{
	image_slices[activeslice].add2tissue_connected(active_tissuelayer,
												   tissuetype, p, override);
}

void SlicesHandler::add2tissue_thresh(tissues_size_t tissuetype, Point p)
{
	image_slices[activeslice].add2tissue_thresh(active_tissuelayer, tissuetype,
												p);
}

void SlicesHandler::subtract_tissue(tissues_size_t tissuetype, Point p)
{
	image_slices[activeslice].subtract_tissue(active_tissuelayer, tissuetype,
											  p);
}

void SlicesHandler::subtract_tissueall(tissues_size_t tissuetype, Point p)
{
	float f = image_slices[activeslice].work_pt(p);
	subtract_tissueall(tissuetype, f);
}

void SlicesHandler::subtract_tissueall(tissues_size_t tissuetype, Point p,
									   unsigned short slicenr)
{
	if (slicenr >= 0 && slicenr < nrslices)
	{
		float f = image_slices[slicenr].work_pt(p);
		subtract_tissueall(tissuetype, f);
	}
}

void SlicesHandler::subtract_tissueall(tissues_size_t tissuetype, float f)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		image_slices[i].subtract_tissue(active_tissuelayer, tissuetype, f);
	}
}

void SlicesHandler::subtract_tissue_connected(tissues_size_t tissuetype,
											  Point p)
{
	image_slices[activeslice].subtract_tissue_connected(active_tissuelayer,
														tissuetype, p);
}

void SlicesHandler::tissue2work(tissues_size_t tissuetype)
{
	image_slices[activeslice].tissue2work(active_tissuelayer, tissuetype);
}

void SlicesHandler::tissue2work3D(tissues_size_t tissuetype)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		image_slices[i].tissue2work(active_tissuelayer, tissuetype);
	}
}

void SlicesHandler::selectedtissue2work(tissues_size_t tissuetype)
{
	image_slices[activeslice].setissue2work(active_tissuelayer, tissuetype);
}

void SlicesHandler::selectedtissue2work3D(tissues_size_t tissuetype)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		image_slices[i].setissue2work(active_tissuelayer, tissuetype);
	}
}

void SlicesHandler::selectedtissue2mc(tissues_size_t tissuetype,
									  unsigned char** voxels)
{
	for (unsigned short i = startslice; i < endslice; i++)
	{
		int k = i - startslice;
		image_slices[i].tissue2mc(active_tissuelayer, tissuetype, voxels, k);
	}
}

void SlicesHandler::cleartissue(tissues_size_t tissuetype)
{
	image_slices[activeslice].cleartissue(active_tissuelayer, tissuetype);
}

void SlicesHandler::cleartissue3D(tissues_size_t tissuetype)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		image_slices[i].cleartissue(active_tissuelayer, tissuetype);
	}
}

void SlicesHandler::cleartissues()
{
	image_slices[activeslice].cleartissues(active_tissuelayer);
}

void SlicesHandler::cleartissues3D()
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		image_slices[i].cleartissues(active_tissuelayer);
	}
}

void SlicesHandler::add2tissueall(tissues_size_t tissuetype, Point p,
								  unsigned short slicenr, bool override)
{
	if (slicenr >= 0 && slicenr < nrslices)
	{
		float f = image_slices[slicenr].work_pt(p);
		add2tissueall(tissuetype, f, override);
	}

	return;
}

void SlicesHandler::add2tissueall(tissues_size_t tissuetype, float f,
								  bool override)
{
	int const iN = endslice;

#pragma omp parallel for
	for (int i = startslice; i < iN; i++)
	{
		image_slices[i].add2tissue(active_tissuelayer, tissuetype, f, override);
	}
}

void SlicesHandler::set_activeslice(unsigned int actslice)
{
	if (actslice < nrslices)
		activeslice = actslice;
}

void SlicesHandler::next_slice() { set_activeslice(activeslice + 1); }

void SlicesHandler::prev_slice()
{
	if (activeslice > 0)
		set_activeslice(activeslice - 1);
}

unsigned short SlicesHandler::get_next_featuring_slice(tissues_size_t type,
													   bool& found)
{
	found = true;
	for (unsigned i = activeslice + 1; i < nrslices; i++)
	{
		if ((image_slices[i]).has_tissue(active_tissuelayer, type))
		{
			return i;
		}
	}
	for (unsigned i = 0; i <= activeslice; i++)
	{
		if ((image_slices[i]).has_tissue(active_tissuelayer, type))
		{
			return i;
		}
	}

	found = false;
	return activeslice;
}

unsigned short SlicesHandler::get_activeslice() { return activeslice; }

bmphandler* SlicesHandler::get_activebmphandler()
{
	return &(image_slices[activeslice]);
}

tissuelayers_size_t SlicesHandler::get_active_tissuelayer()
{
	return active_tissuelayer;
}

void SlicesHandler::set_active_tissuelayer(tissuelayers_size_t idx)
{
	// TODO: Signaling, range checking
	active_tissuelayer = idx;
}

unsigned SlicesHandler::pushstack_bmp()
{
	return get_activebmphandler()->pushstack_bmp();
}

unsigned SlicesHandler::pushstack_bmp(unsigned int slice)
{
	return image_slices[slice].pushstack_bmp();
}

unsigned SlicesHandler::pushstack_work()
{
	return get_activebmphandler()->pushstack_work();
}

unsigned SlicesHandler::pushstack_work(unsigned int slice)
{
	return image_slices[slice].pushstack_work();
}

unsigned SlicesHandler::pushstack_tissue(tissues_size_t i)
{
	return get_activebmphandler()->pushstack_tissue(active_tissuelayer, i);
}

unsigned SlicesHandler::pushstack_tissue(tissues_size_t i, unsigned int slice)
{
	return image_slices[slice].pushstack_tissue(active_tissuelayer, i);
}

unsigned SlicesHandler::pushstack_help()
{
	return get_activebmphandler()->pushstack_help();
}

void SlicesHandler::removestack(unsigned i)
{
	get_activebmphandler()->removestack(i);
}

void SlicesHandler::clear_stack() { get_activebmphandler()->clear_stack(); }

void SlicesHandler::getstack_bmp(unsigned i)
{
	get_activebmphandler()->getstack_bmp(i);
}

void SlicesHandler::getstack_bmp(unsigned int slice, unsigned i)
{
	image_slices[slice].getstack_bmp(i);
}

void SlicesHandler::getstack_work(unsigned i)
{
	get_activebmphandler()->getstack_work(i);
}

void SlicesHandler::getstack_work(unsigned int slice, unsigned i)
{
	image_slices[slice].getstack_work(i);
}

void SlicesHandler::getstack_help(unsigned i)
{
	get_activebmphandler()->getstack_help(i);
}

void SlicesHandler::getstack_tissue(unsigned i, tissues_size_t tissuenr,
									bool override)
{
	get_activebmphandler()->getstack_tissue(active_tissuelayer, i, tissuenr,
											override);
}

void SlicesHandler::getstack_tissue(unsigned int slice, unsigned i,
									tissues_size_t tissuenr, bool override)
{
	image_slices[slice].getstack_tissue(active_tissuelayer, i, tissuenr,
										override);
}

float* SlicesHandler::getstack(unsigned i, unsigned char& mode)
{
	return get_activebmphandler()->getstack(i, mode);
}

void SlicesHandler::popstack_bmp() { get_activebmphandler()->popstack_bmp(); }

void SlicesHandler::popstack_work() { get_activebmphandler()->popstack_work(); }

void SlicesHandler::popstack_help() { get_activebmphandler()->popstack_help(); }

unsigned SlicesHandler::loadstack(const char* filename)
{
	return get_activebmphandler()->loadstack(filename);
}

void SlicesHandler::savestack(unsigned i, const char* filename)
{
	get_activebmphandler()->savestack(i, filename);
}

void SlicesHandler::start_undo(iseg::DataSelection& dataSelection)
{
	if (uelem == NULL)
	{
		uelem = new UndoElem;
		uelem->dataSelection = dataSelection;

		if (dataSelection.bmp)
		{
			uelem->bmp_old = image_slices[dataSelection.sliceNr].copy_bmp();
			uelem->mode1_old =
				image_slices[dataSelection.sliceNr].return_mode(true);
		}
		else
			uelem->bmp_old = NULL;

		if (dataSelection.work)
		{
			uelem->work_old = image_slices[dataSelection.sliceNr].copy_work();
			uelem->mode2_old =
				image_slices[dataSelection.sliceNr].return_mode(false);
		}
		else
			uelem->work_old = NULL;

		if (dataSelection.tissues)
			uelem->tissue_old = image_slices[dataSelection.sliceNr].copy_tissue(
				active_tissuelayer);
		else
			uelem->tissue_old = NULL;

		uelem->vvm_old.clear();
		if (dataSelection.vvm)
			uelem->vvm_old =
				*(image_slices[dataSelection.sliceNr].return_vvm());

		uelem->limits_old.clear();
		if (dataSelection.limits)
			uelem->limits_old =
				*(image_slices[dataSelection.sliceNr].return_limits());

		uelem->marks_old.clear();
		if (dataSelection.marks)
			uelem->marks_old =
				*(image_slices[dataSelection.sliceNr].return_marks());
	}

	return;
}

bool SlicesHandler::start_undoall(iseg::DataSelection& dataSelection)
{
	//abcd std::vector<unsigned short> vslicenr1;
	std::vector<unsigned> vslicenr1;
	for (unsigned i = 0; i < nrslices; i++)
		vslicenr1.push_back(i);

	return start_undo(dataSelection, vslicenr1);
}

//abcd bool SlicesHandler::start_undo(common::DataSelection &dataSelection,std::vector<unsigned short> vslicenr1)
bool SlicesHandler::start_undo(iseg::DataSelection& dataSelection,
							   std::vector<unsigned> vslicenr1)
{
	if (uelem == NULL)
	{
		MultiUndoElem* uelem1 = new MultiUndoElem;
		uelem = uelem1;
		//		uelem=new MultiUndoElem;
		//		MultiUndoElem *uelem1=dynamic_cast<MultiUndoElem *>(uelem);
		uelem->dataSelection = dataSelection;
		uelem1->vslicenr = vslicenr1;

		if (uelem->arraynr() < this->undoQueue.return_nrundoarraysmax())
		{
			//abcd std::vector<unsigned short>::iterator it;
			std::vector<unsigned>::iterator it;
			uelem1->vbmp_old.clear();
			uelem1->vmode1_old.clear();
			if (dataSelection.bmp)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
				{
					uelem1->vbmp_old.push_back(image_slices[*it].copy_bmp());
					uelem1->vmode1_old.push_back(
						image_slices[*it].return_mode(true));
				}
			uelem1->vwork_old.clear();
			uelem1->vmode2_old.clear();
			if (dataSelection.work)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
				{
					uelem1->vwork_old.push_back(image_slices[*it].copy_work());
					uelem1->vmode2_old.push_back(
						image_slices[*it].return_mode(false));
				}
			uelem1->vtissue_old.clear();
			if (dataSelection.tissues)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->vtissue_old.push_back(
						image_slices[*it].copy_tissue(active_tissuelayer));
			uelem1->vvvm_old.clear();
			if (dataSelection.vvm)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->vvvm_old.push_back(
						*(image_slices[*it].return_vvm()));
			uelem1->vlimits_old.clear();
			if (dataSelection.limits)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->vlimits_old.push_back(
						*(image_slices[*it].return_limits()));
			uelem1->marks_old.clear();
			if (dataSelection.marks)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->vmarks_old.push_back(
						*(image_slices[*it].return_marks()));

			return true;
		}
		else
		{
			free(uelem);
			uelem = NULL;
		}
	}

	return false;
}

void SlicesHandler::abort_undo()
{
	if (uelem != NULL)
	{
		free(uelem);
		uelem = NULL;
	}
}

void SlicesHandler::end_undo()
{
	if (uelem != NULL)
	{
		if (uelem->multi)
		{
			//unsigned short undotype=uelem->storetype;

			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(uelem);

			//abcd std::vector<unsigned short>::iterator it;
			std::vector<unsigned>::iterator it;

			uelem1->vbmp_new.clear();
			uelem1->vmode1_new.clear();
			//			if(undotype & 1)
			//				for(it=uelem1->vslicenr.begin();it!=uelem1->vslicenr.end();it++)
			//					uelem1->vbmp_new.push_back(image_slices[*it].copy_bmp());
			uelem1->vwork_new.clear();
			uelem1->vmode2_new.clear();
			//			if(undotype & 2)
			//				for(it=uelem1->vslicenr.begin();it!=uelem1->vslicenr.end();it++)
			//					uelem1->vwork_new.push_back(image_slices[*it].copy_work());
			uelem1->vtissue_new.clear();
			//			if(undotype & 4)
			//				for(it=uelem1->vslicenr.begin();it!=uelem1->vslicenr.end();it++)
			//					uelem1->vtissue_new.push_back(image_slices[*it].copy_tissue(active_tissuelayer));
			uelem1->vvvm_new.clear();
			//			if(undotype & 8)
			//				for(it=uelem1->vslicenr.begin();it!=uelem1->vslicenr.end();it++)
			//					uelem1->vvvm_new.push_back(*(image_slices[*it].return_vvm()));
			uelem1->vlimits_new.clear();
			//			if(undotype & 16)
			//				for(it=uelem1->vslicenr.begin();it!=uelem1->vslicenr.end();it++)
			//					uelem1->vlimits_new.push_back(*(image_slices[*it].return_limits()));
			uelem1->marks_new.clear();
			//			if(undotype & 32)
			//				for(it=uelem1->vslicenr.begin();it!=uelem1->vslicenr.end();it++)
			//					uelem1->vmarks_new.push_back(*(image_slices[*it].return_marks()));

			this->undoQueue.add_undo(uelem1);

			uelem = NULL;
		}
		else
		{
			//unsigned short undotype=uelem->storetype;
			//unsigned short slicenr1=uelem->slicenr;

			//			if(undotype & 1) uelem->bmp_new=image_slices[slicenr1].copy_bmp();
			//			else
			uelem->bmp_new = NULL;
			uelem->mode1_new = 0;

			//			if(undotype & 2) uelem->work_new=image_slices[slicenr1].copy_work();
			//			else
			uelem->work_new = NULL;
			uelem->mode2_new = 0;

			//			if(undotype & 4) uelem->tissue_new=image_slices[slicenr1].copy_tissue(active_tissuelayer);
			//			else
			uelem->tissue_new = NULL;

			uelem->vvm_new.clear();
			//			if(undotype & 8) uelem->vvm_new=*(image_slices[slicenr1].return_vvm());

			uelem->limits_new.clear();
			//			if(undotype & 16) uelem->limits_new=*(image_slices[slicenr1].return_limits());

			uelem->marks_new.clear();
			//			if(undotype & 32) uelem->marks_new=*(image_slices[slicenr1].return_marks());

			this->undoQueue.add_undo(uelem);

			uelem = NULL;
		}
	}
}

void SlicesHandler::merge_undo()
{
	if (uelem != NULL && !uelem->multi)
	{
		iseg::DataSelection dataSelection = uelem->dataSelection;
		/*		unsigned short slicenr1=uelem->slicenr;

		if(undotype & 1) uelem->bmp_new=image_slices[slicenr1].copy_bmp();
		else uelem->bmp_new=NULL;

		if(undotype & 2) uelem->work_new=image_slices[slicenr1].copy_work();
		else uelem->work_new=NULL;

		if(undotype & 4) uelem->tissue_new=image_slices[slicenr1].copy_tissue(active_tissuelayer);
		else uelem->tissue_new=NULL;

		uelem->vvm_new.clear();
		if(undotype & 8) uelem->vvm_new=*(image_slices[slicenr1].return_vvm());

		uelem->limits_new.clear();
		if(undotype & 16) uelem->limits_new=*(image_slices[slicenr1].return_limits());

		uelem->marks_new.clear();
		if(undotype & 32) uelem->marks_new=*(image_slices[slicenr1].return_marks());*/

		this->undoQueue.merge_undo(uelem);

		if (dataSelection.bmp)
		{
			uelem->bmp_new = NULL;
			uelem->bmp_old = NULL;
		}
		if (dataSelection.work)
		{
			uelem->work_new = NULL;
			uelem->work_old = NULL;
		}
		if (dataSelection.tissues)
		{
			uelem->tissue_new = NULL;
			uelem->tissue_old = NULL;
		}

		delete uelem;
		uelem = NULL;
	}
}

iseg::DataSelection SlicesHandler::undo()
{
	if (uelem == NULL)
	{
		uelem = this->undoQueue.undo();
		if (uelem->multi)
		{
			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(uelem);

			if (uelem1 != NULL)
			{
				unsigned short current_slice;
				iseg::DataSelection dataSelection = uelem->dataSelection;

				for (unsigned i = 0; i < uelem1->vslicenr.size(); i++)
				{
					current_slice = uelem1->vslicenr[i];
					if (dataSelection.bmp)
					{
						uelem1->vbmp_new.push_back(
							image_slices[current_slice].copy_bmp());
						uelem1->vmode1_new.push_back(
							image_slices[current_slice].return_mode(true));
						image_slices[current_slice].copy2bmp(
							uelem1->vbmp_old[i], uelem1->vmode1_old[i]);
						free(uelem1->vbmp_old[i]);
					}
					if (dataSelection.work)
					{
						uelem1->vwork_new.push_back(
							image_slices[current_slice].copy_work());
						uelem1->vmode2_new.push_back(
							image_slices[current_slice].return_mode(false));
						image_slices[current_slice].copy2work(
							uelem1->vwork_old[i], uelem1->vmode2_old[i]);
						free(uelem1->vwork_old[i]);
					}
					if (dataSelection.tissues)
					{
						uelem1->vtissue_new.push_back(
							image_slices[current_slice].copy_tissue(
								active_tissuelayer));
						image_slices[current_slice].copy2tissue(
							active_tissuelayer, uelem1->vtissue_old[i]);
						free(uelem1->vtissue_old[i]);
					}
					if (dataSelection.vvm)
					{
						uelem1->vvvm_new.push_back(
							*(image_slices[current_slice].return_vvm()));
						image_slices[current_slice].copy2vvm(
							&(uelem1->vvvm_old[i]));
					}
					if (dataSelection.limits)
					{
						uelem1->vlimits_new.push_back(
							*(image_slices[current_slice].return_limits()));
						image_slices[current_slice].copy2limits(
							&(uelem1->vlimits_old[i]));
					}
					if (dataSelection.marks)
					{
						uelem1->vmarks_new.push_back(
							*(image_slices[current_slice].return_marks()));
						image_slices[current_slice].copy2marks(
							&(uelem1->vmarks_old[i]));
					}
				}
				uelem1->vbmp_old.clear();
				uelem1->vmode1_old.clear();
				uelem1->vwork_old.clear();
				uelem1->vmode2_old.clear();
				uelem1->vtissue_old.clear();
				uelem1->vvvm_old.clear();
				uelem1->vlimits_old.clear();
				uelem1->vmarks_old.clear();

				uelem = NULL;

				return dataSelection;
			}
			else
				return iseg::DataSelection();
		}
		else
		{
			if (uelem != NULL)
			{
				iseg::DataSelection dataSelection = uelem->dataSelection;

				if (dataSelection.bmp)
				{
					uelem->bmp_new =
						image_slices[dataSelection.sliceNr].copy_bmp();
					uelem->mode1_new =
						image_slices[dataSelection.sliceNr].return_mode(true);
					image_slices[dataSelection.sliceNr].copy2bmp(
						uelem->bmp_old, uelem->mode1_old);
					free(uelem->bmp_old);
					uelem->bmp_old = NULL;
				}

				if (dataSelection.work)
				{
					uelem->work_new =
						image_slices[dataSelection.sliceNr].copy_work();
					uelem->mode2_new =
						image_slices[dataSelection.sliceNr].return_mode(false);
					image_slices[dataSelection.sliceNr].copy2work(
						uelem->work_old, uelem->mode2_old);
					free(uelem->work_old);
					uelem->work_old = NULL;
				}

				if (dataSelection.tissues)
				{
					uelem->tissue_new =
						image_slices[dataSelection.sliceNr].copy_tissue(
							active_tissuelayer);
					image_slices[dataSelection.sliceNr].copy2tissue(
						active_tissuelayer, uelem->tissue_old);
					free(uelem->tissue_old);
					uelem->tissue_old = NULL;
				}

				if (dataSelection.vvm)
				{
					uelem->vvm_new.clear();
					uelem->vvm_new =
						*(image_slices[dataSelection.sliceNr].return_vvm());
					image_slices[dataSelection.sliceNr].copy2vvm(
						&uelem->vvm_old);
					uelem->vvm_old.clear();
				}

				if (dataSelection.limits)
				{
					uelem->limits_new.clear();
					uelem->limits_new =
						*(image_slices[dataSelection.sliceNr].return_limits());
					image_slices[dataSelection.sliceNr].copy2limits(
						&uelem->limits_old);
					uelem->limits_old.clear();
				}

				if (dataSelection.marks)
				{
					uelem->marks_new.clear();
					uelem->marks_new =
						*(image_slices[dataSelection.sliceNr].return_marks());
					image_slices[dataSelection.sliceNr].copy2marks(
						&uelem->marks_old);
					uelem->marks_old.clear();
				}

				set_activeslice(dataSelection.sliceNr);

				uelem = NULL;

				return dataSelection;
			}
			else
				return iseg::DataSelection();
		}
	}
	else
		return iseg::DataSelection();
}

iseg::DataSelection SlicesHandler::redo()
{
	if (uelem == NULL)
	{
		uelem = this->undoQueue.redo();
		if (uelem == NULL)
			return iseg::DataSelection();
		if (uelem->multi)
		{
			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(uelem);

			if (uelem1 != NULL)
			{
				unsigned short current_slice;
				iseg::DataSelection dataSelection = uelem->dataSelection;
				for (unsigned i = 0; i < uelem1->vslicenr.size(); i++)
				{
					current_slice = uelem1->vslicenr[i];
					if (dataSelection.bmp)
					{
						uelem1->vbmp_old.push_back(
							image_slices[current_slice].copy_bmp());
						uelem1->vmode1_old.push_back(
							image_slices[current_slice].return_mode(true));
						image_slices[current_slice].copy2bmp(
							uelem1->vbmp_new[i], uelem1->vmode1_new[i]);
						free(uelem1->vbmp_new[i]);
					}
					if (dataSelection.work)
					{
						uelem1->vwork_old.push_back(
							image_slices[current_slice].copy_work());
						uelem1->vmode2_old.push_back(
							image_slices[current_slice].return_mode(false));
						image_slices[current_slice].copy2work(
							uelem1->vwork_new[i], uelem1->vmode2_new[i]);
						free(uelem1->vwork_new[i]);
					}
					if (dataSelection.tissues)
					{
						uelem1->vtissue_old.push_back(
							image_slices[current_slice].copy_tissue(
								active_tissuelayer));
						image_slices[current_slice].copy2tissue(
							active_tissuelayer, uelem1->vtissue_new[i]);
						free(uelem1->vtissue_new[i]);
					}
					if (dataSelection.vvm)
					{
						uelem1->vvvm_old.push_back(
							*(image_slices[current_slice].return_vvm()));
						image_slices[current_slice].copy2vvm(
							&(uelem1->vvvm_new[i]));
					}
					if (dataSelection.limits)
					{
						uelem1->vlimits_old.push_back(
							*(image_slices[current_slice].return_limits()));
						image_slices[current_slice].copy2limits(
							&(uelem1->vlimits_new[i]));
					}
					if (dataSelection.marks)
					{
						uelem1->vmarks_old.push_back(
							*(image_slices[current_slice].return_marks()));
						image_slices[current_slice].copy2marks(
							&(uelem1->vmarks_new[i]));
					}
				}
				uelem1->vbmp_new.clear();
				uelem1->vwork_new.clear();
				uelem1->vtissue_new.clear();
				uelem1->vvvm_new.clear();
				uelem1->vlimits_new.clear();
				uelem1->vmarks_new.clear();

				uelem = NULL;

				return dataSelection;
			}
			else
				return iseg::DataSelection();
		}
		else
		{
			if (uelem != NULL)
			{
				iseg::DataSelection dataSelection = uelem->dataSelection;

				if (dataSelection.bmp)
				{
					uelem->bmp_old =
						image_slices[dataSelection.sliceNr].copy_bmp();
					uelem->mode1_old =
						image_slices[dataSelection.sliceNr].return_mode(true);
					image_slices[dataSelection.sliceNr].copy2bmp(
						uelem->bmp_new, uelem->mode1_new);
					free(uelem->bmp_new);
					uelem->bmp_new = NULL;
				}

				if (dataSelection.work)
				{
					uelem->work_old =
						image_slices[dataSelection.sliceNr].copy_work();
					uelem->mode2_old =
						image_slices[dataSelection.sliceNr].return_mode(false);
					image_slices[dataSelection.sliceNr].copy2work(
						uelem->work_new, uelem->mode2_new);
					free(uelem->work_new);
					uelem->work_new = NULL;
				}

				if (dataSelection.tissues)
				{
					uelem->tissue_old =
						image_slices[dataSelection.sliceNr].copy_tissue(
							active_tissuelayer);
					image_slices[dataSelection.sliceNr].copy2tissue(
						active_tissuelayer, uelem->tissue_new);
					free(uelem->tissue_new);
					uelem->tissue_new = NULL;
				}

				if (dataSelection.vvm)
				{
					uelem->vvm_old.clear();
					uelem->vvm_old =
						*(image_slices[dataSelection.sliceNr].return_vvm());
					image_slices[dataSelection.sliceNr].copy2vvm(
						&uelem->vvm_new);
					uelem->vvm_new.clear();
				}

				if (dataSelection.limits)
				{
					uelem->limits_old.clear();
					uelem->limits_old =
						*(image_slices[dataSelection.sliceNr].return_limits());
					image_slices[dataSelection.sliceNr].copy2limits(
						&uelem->limits_new);
					uelem->limits_new.clear();
				}

				if (dataSelection.marks)
				{
					uelem->marks_old.clear();
					uelem->marks_old =
						*(image_slices[dataSelection.sliceNr].return_marks());
					image_slices[dataSelection.sliceNr].copy2marks(
						&uelem->marks_new);
					uelem->marks_new.clear();
				}

				set_activeslice(dataSelection.sliceNr);

				uelem = NULL;

				return dataSelection;
			}
			else
				return iseg::DataSelection();
		}
	}
	else
		return iseg::DataSelection();
}

void SlicesHandler::clear_undo()
{
	this->undoQueue.clear_undo();
	if (uelem != NULL)
		delete (uelem);
	uelem = NULL;

	return;
}

void SlicesHandler::reverse_undosliceorder()
{
	this->undoQueue.reverse_undosliceorder(nrslices);
	if (uelem != NULL)
	{
		uelem->dataSelection.sliceNr =
			nrslices - 1 - uelem->dataSelection.sliceNr;
	}

	return;
}

unsigned SlicesHandler::return_nrundo()
{
	return this->undoQueue.return_nrundo();
}

unsigned SlicesHandler::return_nrredo()
{
	return this->undoQueue.return_nrredo();
}

bool SlicesHandler::return_undo3D() { return undo3D; }

unsigned SlicesHandler::return_nrundosteps()
{
	return this->undoQueue.return_nrundomax();
}

unsigned SlicesHandler::return_nrundoarrays()
{
	return this->undoQueue.return_nrundoarraysmax();
}

void SlicesHandler::set_undo3D(bool undo3D1) { undo3D = undo3D1; }

void SlicesHandler::set_undonr(unsigned nr) { this->undoQueue.set_nrundo(nr); }

void SlicesHandler::set_undoarraynr(unsigned nr)
{
	this->undoQueue.set_nrundoarraysmax(nr);
}

int SlicesHandler::LoadDICOM(std::vector<const char*> lfilename)
{
	if (lfilename.size() > 0)
	{
		endslice = nrslices = (unsigned short)lfilename.size();
		os.set_sizenr(nrslices);
		image_slices.resize(nrslices);

		activeslice = 0;
		active_tissuelayer = 0;
		startslice = 0;

		unsigned short a, b, c;
		float d, e, thick1;
		float disp1[3];
		float dc1[6]; // direction cosines
		if (gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[0], a, b, c, d, e,
											   thick1, disp1, dc1))
		{
			Transform tr(disp1, dc1);

			set_pixelsize(d, e);
			set_slicethickness(thick1);
			set_transform(tr);
		}

		if (lfilename.size() > 1)
		{
			QFileInfo fi(lfilename[0]);
			QString directoryName = fi.absoluteDir().absolutePath();
			double newThick =
				gdcmvtk_rtstruct::GetZSPacing(directoryName.toStdString());
			if (newThick)
			{
				set_slicethickness(newThick);
			}
		}

		for (int i = 0; i < lfilename.size(); i++)
		{
			if (gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[i], a, b, c, d, e,
												   thick1, disp1, dc1))
			{
				if (c >= 1)
				{
					unsigned long totsize = (unsigned long)(a)*b * c;
					float* bits = (float*)malloc(sizeof(float) * totsize);

					if (bits == NULL)
						return 0;

					bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(
						lfilename[i], bits, a, b, c);
					if (!canload)
					{
						free(bits);
						return 0;
					}

					(image_slices[i])
						.LoadArray(&(bits[(unsigned long)(a)*b * 0]), a, b);

					free(bits);
				}
			}
		}

		endslice = nrslices = (unsigned short)(lfilename.size());
		os.set_sizenr(nrslices);

		// Ranges
		Pair dummy;
		slice_ranges.resize(nrslices);
		slice_bmpranges.resize(nrslices);
		compute_range_mode1(&dummy);
		compute_bmprange_mode1(&dummy);

		width = image_slices[0].return_width();
		height = image_slices[0].return_height();
		area = height * (unsigned int)width;

		if (overlay != NULL)
			free(overlay);

		overlay = (float*)malloc(sizeof(float) * area);
		clear_overlay();

		loaded = true;

		return true;
	}
	return false;
}

int SlicesHandler::LoadDICOM(std::vector<const char*> lfilename, Point p,
							 unsigned short dx, unsigned short dy)
{
	activeslice = 0;
	active_tissuelayer = 0;
	startslice = 0;

	if (lfilename.size() == 1)
	{
		unsigned short a, b, c;
		float d, e, thick1;
		float disp1[3];
		float dc1[6]; // direction cosines
		gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[0], a, b, c, d, e, thick1,
										   disp1, dc1);
		if (c > 1)
		{
			unsigned long totsize = (unsigned long)(a)*b * c;
			float* bits = (float*)malloc(sizeof(float) * totsize);
			if (bits == NULL)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0],
															   bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			endslice = nrslices = (unsigned short)c;
			os.set_sizenr(nrslices);
			image_slices.resize(nrslices);

			int j = 0;
			for (unsigned short i = 0; i < nrslices; i++)
			{
				if ((image_slices[i])
						.LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b, p,
								   dx, dy))
					j++;
			}

			free(bits);

			if (j < nrslices)
				return 0;

			// Ranges
			Pair dummy;
			slice_ranges.resize(nrslices);
			slice_bmpranges.resize(nrslices);
			compute_range_mode1(&dummy);
			compute_bmprange_mode1(&dummy);

			width = image_slices[0].return_width();
			height = image_slices[0].return_height();
			area = height * (unsigned int)width;

			if (overlay != NULL)
				free(overlay);
			overlay = (float*)malloc(sizeof(float) * area);
			clear_overlay();

			loaded = true;

			Transform tr(disp1, dc1);

			set_pixelsize(d, e);
			set_slicethickness(thick1);
			set_transform(tr);

			return 1;
		}
	}

	endslice = nrslices = (unsigned short)(lfilename.size());
	os.set_sizenr(nrslices);

	image_slices.resize(nrslices);
	int j = 0;

	float thick1 = DICOMsort(&lfilename);
	for (unsigned short i = 0; i < nrslices; i++)
	{
		if ((image_slices[i]).LoadDICOM(lfilename[i], p, dx, dy))
			j++;
	}

	width = image_slices[0].return_width();
	height = image_slices[0].return_height();
	area = height * (unsigned int)width;

	if (overlay != NULL)
		free(overlay);
	overlay = (float*)malloc(sizeof(float) * area);
	clear_overlay();

	if (j == nrslices)
	{
		loaded = true;

		DicomReader dcmr;
		if (dcmr.opendicom(lfilename[0]))
		{
			Pair p1;
			float disp1[3];
			float dc1[6]; // direction cosines

			if (thick1 <= 0)
				thick1 = dcmr.thickness();
			p1 = dcmr.pixelsize();
			dcmr.imagepos(disp1);
			disp1[0] += p.px * p1.low;
			disp1[1] += p.py * p1.high;
			dcmr.imageorientation(dc1);
			dcmr.closedicom();

			Transform tr(disp1, dc1);

			set_pixelsize(p1.high, p1.low);
			set_slicethickness(thick1);
			set_transform(tr);
		}

		// Ranges
		Pair dummy;
		slice_ranges.resize(nrslices);
		slice_bmpranges.resize(nrslices);
		compute_range_mode1(&dummy);
		compute_bmprange_mode1(&dummy);

		return 1;
	}
	else
		return 0;
}

int SlicesHandler::ReloadDICOM(std::vector<const char*> lfilename)
{
	if ((endslice - startslice) == (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = startslice; i < endslice; i++)
		{
			if ((image_slices[i]).ReloadDICOM(lfilename[i - startslice]))
				j++;
		}

		if (j == (startslice - endslice))
			return 1;
		else
			return 0;
	}
	else if (nrslices <= (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = 0; i < nrslices; i++)
		{
			if ((image_slices[i]).ReloadDICOM(lfilename[i]))
				j++;
		}

		if (j == nrslices)
			return 1;
		else
			return 0;
	}
	else if (lfilename.size() == 1)
	{
		unsigned short a, b, c;
		float d, e, thick1;
		float disp1[3];
		float dc1[6]; // direction cosines
		gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[0], a, b, c, d, e, thick1,
										   disp1, dc1);
		if (nrslices == c)
		{
			unsigned long totsize = (unsigned long)(a)*b * c;
			float* bits = (float*)malloc(sizeof(float) * totsize);
			if (bits == NULL)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0],
															   bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			int j = 0;
			for (unsigned short i = startslice; i < endslice; i++)
			{
				if ((image_slices[i])
						.LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b))
					j++;
			}

			free(bits);
			if (j < endslice - startslice)
				return 0;
			return 1;
		}
	}
	return 0;
}

int SlicesHandler::ReloadDICOM(std::vector<const char*> lfilename, Point p)
{
	if ((endslice - startslice) == (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = startslice; i < endslice; i++)
		{
			if ((image_slices[i]).ReloadDICOM(lfilename[i - startslice], p))
				j++;
		}

		if (j == (endslice - startslice))
			return 1;
		else
			return 0;
	}
	else if (nrslices <= (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = 0; i < nrslices; i++)
		{
			if ((image_slices[i]).ReloadDICOM(lfilename[i], p))
				j++;
		}

		if (j == nrslices)
			return 1;
		else
			return 0;
	}
	else if (lfilename.size() == 1)
	{
		unsigned short a, b, c;
		float d, e, thick1;
		float disp1[3];
		float dc1[6]; // direction cosines
		gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[0], a, b, c, d, e, thick1,
										   disp1, dc1);
		if (nrslices == c)
		{
			unsigned long totsize = (unsigned long)(a)*b * c;
			float* bits = (float*)malloc(sizeof(float) * totsize);
			if (bits == NULL)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0],
															   bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			int j = 0;
			for (unsigned short i = startslice; i < endslice; i++)
			{
				if ((image_slices[i])
						.LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b, p,
								   width, height))
					j++;
			}

			free(bits);
			if (j < endslice - startslice)
				return 0;
			return 1;
		}
	}
	else
		return 0;

	return 0;
}

float SlicesHandler::DICOMsort(std::vector<const char*>* lfilename)
{
	float retval = -1.0f;
	DicomReader dcmread;
	std::vector<float> vpos;

	for (std::vector<const char*>::iterator it = lfilename->begin();
		 it != lfilename->end(); it++)
	{
		dcmread.opendicom(*it);
		vpos.push_back(dcmread.slicepos());
		dcmread.closedicom();
	}

	float dummypos;
	const char* dummyname;
	short nrelem = (short)lfilename->size();

	for (short i = 0; i < nrelem - 1; i++)
	{
		for (short j = nrelem - 1; j > i; j--)
		{
			if (vpos[j] > vpos[j - 1])
			{
				dummypos = vpos[j];
				vpos[j] = vpos[j - 1];
				vpos[j - 1] = dummypos;

				dummyname = (*lfilename)[j];
				(*lfilename)[j] = (*lfilename)[j - 1];
				(*lfilename)[j - 1] = dummyname;
			}
		}
	}

	if (nrelem > 1)
	{
		retval = (vpos[0] - vpos[nrelem - 1]) / (nrelem - 1);
	}

	return retval;
}

void SlicesHandler::GetDICOMseriesnr(std::vector<const char*>* vnames,
									 std::vector<unsigned>* dicomseriesnr,
									 std::vector<unsigned>* dicomseriesnrlist)
{
	DicomReader dcmread;

	dicomseriesnr->clear();
	for (std::vector<const char*>::iterator it = vnames->begin();
		 it != vnames->end(); it++)
	{
		dcmread.opendicom(*it);
		std::vector<unsigned>::iterator it1 = dicomseriesnr->begin();
		unsigned u = dcmread.seriesnr();
		dicomseriesnrlist->push_back(u);
		dcmread.closedicom();
		while (it1 != dicomseriesnr->end() && (*it1) != u)
			it1++;
		if (it1 == dicomseriesnr->end())
		{
			dicomseriesnr->push_back(u);
		}
	}

	std::sort(dicomseriesnr->begin(), dicomseriesnr->end());
}

void SlicesHandler::permute_tissue_indices(tissues_size_t* indexMap)
{
	for (short unsigned i = 0; i < nrslices; i++)
	{
		image_slices[i].permute_tissue_indices(indexMap);
	}
}

void SlicesHandler::remove_tissue(tissues_size_t tissuenr,
								  tissues_size_t tissuecount1)
{
	for (short unsigned i = 0; i < nrslices; i++)
	{
		image_slices[i].remove_tissue(tissuenr, tissuecount1);
	}
	TissueInfos::RemoveTissue(tissuenr);
}

void SlicesHandler::remove_tissues(const std::set<tissues_size_t>& tissuenrs)
{
	// Remove in descending order
	tissues_size_t tissuecount = TissueInfos::GetTissueCount();
	for (auto riter = tissuenrs.rbegin(); riter != tissuenrs.rend(); ++riter)
	{
		int const iN = nrslices;

#pragma omp parallel for
		for (int i = 0; i < iN; i++)
		{
			image_slices[i].remove_tissue(*riter, tissuecount);
		}
		tissuecount--;
	}
	TissueInfos::RemoveTissues(tissuenrs);
}

void SlicesHandler::remove_tissueall()
{
	for (short unsigned i = 0; i < nrslices; i++)
	{
		image_slices[i].cleartissuesall();
	}
	TissueInfos::RemoveAllTissues();
	TissueInfoStruct tissue;
	tissue.locked = false;
	tissue.SetColor(1.0f, 0.0f, 0.0f);
	tissue.name = "Tissue1";
	TissueInfos::AddTissue(tissue);
}

void SlicesHandler::cap_tissue(tissues_size_t maxval)
{
	for (short unsigned i = 0; i < nrslices; i++)
	{
		image_slices[i].cap_tissue(maxval);
	}
}

void SlicesHandler::build255tissues()
{
	TissueInfos::RemoveAllTissues();
	QString sdummy;
	TissueInfoStruct tissue;
	tissue.locked = false;
	for (unsigned i = 0; i < 255; i++)
	{
		tissue.color[0] = (i % 7) * 0.166666666f;
		tissue.color[1] = ((i / 7) % 7) * 0.166666666f;
		tissue.color[2] = (i / 49) * 0.19f;
		tissue.name = QString("Tissue") + sdummy.setNum(i + 1);
		TissueInfos::AddTissue(tissue);
	}
}

void SlicesHandler::buildmissingtissues(tissues_size_t j)
{
	tissues_size_t tissueCount = TissueInfos::GetTissueCount();
	if (j > tissueCount)
	{
		QString sdummy;
		TissueInfoStruct tissue;
		tissue.locked = false;
		tissue.opac = 0.5f;
		for (tissues_size_t i = tissueCount + 1; i <= j; i++)
		{
			tissue.color[0] = ((i - 1) % 7) * 0.166666666f;
			tissue.color[1] = (((i - 1) / 7) % 7) * 0.166666666f;
			tissue.color[2] = ((i - 1) / 49) * 0.19f;
			tissue.name = QString("Tissue") + sdummy.setNum(i);
			TissueInfos::AddTissue(tissue);
		}
	}
}

void SlicesHandler::group_tissues(std::vector<tissues_size_t>& olds,
								  std::vector<tissues_size_t>& news)
{
	int const iN = nrslices;

#pragma omp parallel for
	for (int i = 0; i < iN; i++)
	{
		(image_slices[i]).group_tissues(active_tissuelayer, olds, news);
	}
}
void SlicesHandler::set_modeall(unsigned char mode, bool bmporwork)
{
	for (unsigned short i = startslice; i < endslice; i++)
		image_slices[i].set_mode(mode, bmporwork);
}

bool SlicesHandler::print_tissuemat(const char* filename)
{
	std::vector<tissues_size_t*> matrix(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
		matrix[i] = image_slices[i].return_tissues(active_tissuelayer);
	bool ok = matexport::print_matslices(
		filename, matrix.data(), int(width), int(height), int(nrslices),
		"iSeg tissue data v1.0", 21, "tissuedistrib", 13);
	return ok;
}

bool SlicesHandler::print_bmpmat(const char* filename)
{
	std::vector<float*> matrix(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
		matrix[i] = image_slices[i].return_bmp();
	bool ok = matexport::print_matslices(
		filename, matrix.data(), int(width), int(height), int(nrslices),
		"iSeg source data v1.0", 21, "sourcedistrib", 13);
	return ok;
}

bool SlicesHandler::print_workmat(const char* filename)
{
	std::vector<float*> matrix(nrslices);
	for (unsigned short i = 0; i < nrslices; i++)
		matrix[i] = image_slices[i].return_work();
	bool ok = matexport::print_matslices(
		filename, matrix.data(), int(width), int(height), int(nrslices),
		"iSeg target data v1.0", 21, "targetdistrib", 13);
	return ok;
}

bool SlicesHandler::print_atlas(const char* filename)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);

	// Write a header with a "magic number" and a version
	out << (quint32)0xD0C0B0A0;
	out << (qint32)iseg::CombineTissuesVersion(1, 1);

	out.setVersion(QDataStream::Qt_4_0);

	// Write the data
	out << (quint32)width << (quint32)height << (quint32)nrslices;
	out << (float)dx << (float)dy << (float)thickness;
	tissues_size_t tissueCount = TissueInfos::GetTissueCount();
	out << (quint32)tissueCount;
	TissueInfoStruct* tissueInfo;
	for (tissues_size_t tissuenr = 1; tissuenr <= tissueCount; tissuenr++)
	{
		tissueInfo = TissueInfos::GetTissueInfo(tissuenr);
		out << tissueInfo->name << tissueInfo->color[0] << tissueInfo->color[1]
			<< tissueInfo->color[2];
	}
	for (unsigned short i = 0; i < nrslices; i++)
	{
		out.writeRawData((char*)image_slices[i].return_bmp(),
						 (int)area * sizeof(float));
		out.writeRawData(
			(char*)image_slices[i].return_tissues(active_tissuelayer),
			(int)area * sizeof(tissues_size_t));
	}

	return true;
}

bool SlicesHandler::print_amascii(const char* filename)
{
	std::ofstream streamname;
	streamname.open(filename, std::ios_base::binary);
	if (streamname.good())
	{
		bool ok = true;
		streamname << "# AmiraMesh 3D ASCII 2.0" << endl
				   << endl;
		streamname << "# CreationDate: Fri Jun 16 14:24:32 2006" << endl
				   << endl
				   << endl;
		streamname << "define Lattice " << width << " " << height << " "
				   << nrslices << endl
				   << endl;
		streamname << "Parameters {" << endl;
		streamname << "    Materials {" << endl;
		streamname << "        Exterior {" << endl;
		streamname << "            Id 1" << endl;
		streamname << "        }" << endl;
		tissues_size_t tissueCount = TissueInfos::GetTissueCount();
		TissueInfoStruct* tissueInfo;
		for (tissues_size_t tc = 0; tc < tissueCount; tc++)
		{
			tissueInfo = TissueInfos::GetTissueInfo(tc + 1);
			QString nameCpy = tissueInfo->name;
			nameCpy = nameCpy.replace("�", "ae");
			nameCpy = nameCpy.replace("�", "Ae");
			nameCpy = nameCpy.replace("�", "oe");
			nameCpy = nameCpy.replace("�", "Oe");
			nameCpy = nameCpy.replace("�", "ue");
			nameCpy = nameCpy.replace("�", "Ue");
			streamname << "        " << nameCpy.ascii() << " {" << endl;
			streamname << "            Color " << tissueInfo->color[0] << " "
					   << tissueInfo->color[1] << " " << tissueInfo->color[2]
					   << "," << endl;
			streamname << "            Id " << tc + 2 << endl;
			streamname << "        }" << endl;
		}
		streamname << "    }" << endl;
		if (tissueCount <= 255)
		{
			streamname << "    Content \"" << width << "x" << height << "x"
					   << nrslices << " byte, uniform coordinates\"," << endl;
		}
		else
		{
			streamname << "    Content \"" << width << "x" << height << "x"
					   << nrslices << " ushort, uniform coordinates\"," << endl;
		}
		streamname << "    BoundingBox 0 " << width * dx << " 0 " << height * dy
				   << " 0 " << nrslices * thickness << "," << endl;
		streamname << "    CoordType \"uniform\"" << endl;
		streamname << "}" << endl
				   << endl;
		if (tissueCount <= 255)
		{
			streamname << "Lattice { byte Labels } @1" << endl
					   << endl;
		}
		else
		{
			streamname << "Lattice { ushort Labels } @1" << endl
					   << endl;
		}
		streamname << "# Data section follows" << endl;
		streamname << "@1" << endl;
		for (unsigned short i = 0; i < nrslices; i++)
			ok &= image_slices[i].print_amascii_slice(active_tissuelayer,
													  streamname);
		streamname << endl;

		streamname.close();
		return ok;
	}
	else
		return false;
}

/// This function returns a pointer to a vtkImageData
/// The user is responsible for deleting data ...
vtkImageData* SlicesHandler::make_vtktissueimage()
{
	const char* tissueNameArrayName = "TissueNames"; // don't modify this
	const char* tissueColorArrayName = "Colors";	 // don't modify this

	// copy label field
	vtkImageData* labelField = vtkImageData::New();
	labelField->SetExtent(0, (int)return_width() - 1, 0,
						  (int)return_height() - 1, 0,
						  (int)(endslice - startslice) - 1);
	Pair ps = get_pixelsize();
	float offset[3];
	get_displacement(offset);
	labelField->SetSpacing(ps.high, ps.low, thickness);
	labelField->SetOrigin(offset[0], offset[1],
						  offset[2] + thickness * startslice);
	if (TissueInfos::GetTissueCount() <= 255)
	{
		labelField->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		unsigned char* field =
			(unsigned char*)labelField->GetScalarPointer(0, 0, 0);
		for (unsigned short i = startslice; i < endslice; i++)
		{
			copyfromtissue(i, &(field[i * (unsigned long long)return_area()]));
		}
	}
	else if (sizeof(tissues_size_t) == sizeof(unsigned short))
	{
		labelField->AllocateScalars(VTK_UNSIGNED_SHORT, 1);
		tissues_size_t* field =
			(tissues_size_t*)labelField->GetScalarPointer(0, 0, 0);
		for (unsigned short i = startslice; i < endslice; i++)
		{
			copyfromtissue(i, &(field[i * (unsigned long long)return_area()]));
		}
	}
	else
	{
		cerr << "SlicesHandler::make_vtktissueimage: Error, tissues_size_t not "
				"implemented!"
			 << endl;
		labelField->Delete();
		return NULL;
	}

	// copy tissue names and colors
	tissues_size_t num_tissues = TissueInfos::GetTissueCount();
	vtkSmartPointer<vtkStringArray> names_array =
		vtkSmartPointer<vtkStringArray>::New();
	names_array->SetNumberOfTuples(num_tissues + 1);
	names_array->SetName(tissueNameArrayName);
	labelField->GetFieldData()->AddArray(names_array);

	vtkSmartPointer<vtkFloatArray> color_array =
		vtkSmartPointer<vtkFloatArray>::New();
	color_array->SetNumberOfComponents(3);
	color_array->SetNumberOfTuples(num_tissues + 1);
	color_array->SetName(tissueColorArrayName);
	labelField->GetFieldData()->AddArray(color_array);
	for (tissues_size_t i = 1; i < num_tissues; i++)
	{
		int error_counter = 0;
		check_equal(TissueInfos::GetTissueType(TissueInfos::GetTissueName(i)),
					i);
		names_array->SetValue(i, TissueInfos::GetTissueName(i).ascii());
		float* color = TissueInfos::GetTissueColor(i);
		std::cerr << TissueInfos::GetTissueName(i).ascii() << " " << color[0]
				  << "," << color[1] << "," << color[2] << std::endl;
		color_array->SetTuple(i, color);
	}

	std::cerr << "vtkImageData::GetActualMemorySize = "
			  << labelField->GetActualMemorySize() << " KB" << std::endl;
	return labelField;
}

bool SlicesHandler::export_tissue(const char* filename, bool binary) const
{
	// BL TODO startslice-endslice
	auto slices = get_tissues(active_tissuelayer);
	float spacing[3] = {dx, dy, thickness};
	return ImageWriter(binary).writeVolume(
		filename, slices.data(), width, height, nrslices, spacing, transform);
}

bool SlicesHandler::export_bmp(const char* filename, bool binary) const
{
	// BL TODO startslice-endslice
	auto slices = get_bmp();
	float spacing[3] = {dx, dy, thickness};
	return ImageWriter(binary).writeVolume(
		filename, slices.data(), width, height, nrslices, spacing, transform);
}

bool SlicesHandler::export_work(const char* filename, bool binary) const
{
	// BL TODO startslice-endslice
	auto slices = get_work();
	float spacing[3] = {dx, dy, thickness};
	return ImageWriter(binary).writeVolume(
		filename, slices.data(), width, height, nrslices, spacing, transform);
}

bool SlicesHandler::print_xmlregionextent(const char* filename,
										  bool onlyactiveslices,
										  const char* projname)
{
	float offset[3];
	get_displacement(offset);

	std::ofstream streamname;
	streamname.open(filename, std::ios_base::binary);
	if (streamname.good())
	{
		bool ok = true;
		streamname << "<?xml version=\"1.0\"?>" << endl;
		streamname << "<IndexFile type=\"Extent\" version=\"0.1\">" << endl;
		unsigned short extent[3][2];
		tissues_size_t tissueCount = TissueInfos::GetTissueCount();
		TissueInfoStruct* tissueInfo;
		for (tissues_size_t tissuenr = 1; tissuenr <= tissueCount; tissuenr++)
		{
			if (get_extent(tissuenr, onlyactiveslices, extent))
			{
				tissueInfo = TissueInfos::GetTissueInfo(tissuenr);
				streamname << "\t<label id=\"" << (int)tissuenr << "\" name=\""
						   << tissueInfo->name.ascii() << "\" color=\""
						   << tissueInfo->color[0] << " "
						   << tissueInfo->color[1] << " "
						   << tissueInfo->color[2] << "\">" << endl;
				streamname << "\t\t<dataset filename=\"";
				if (projname != NULL)
				{
					streamname << projname;
				}
				streamname << "\" extent=\"" << extent[0][0] << " "
						   << extent[0][1] << " " << extent[1][0] << " "
						   << extent[1][1] << " " << extent[2][0] << " "
						   << extent[2][1];
				streamname << "\" global_bounds=\""
						   << extent[0][0] * dx + offset[0] << " "
						   << extent[0][1] * dx + offset[0] << " "
						   << extent[1][0] * dy + offset[1] << " "
						   << extent[1][1] * dy + offset[1] << " "
						   << extent[2][0] * thickness + offset[2] << " "
						   << extent[2][1] * thickness + offset[2] << "\">"
						   << endl;
				streamname << "\t\t</dataset>" << endl;
				streamname << "\t</label>" << endl;
			}
		}
		streamname << "</IndexFile>" << endl;
		streamname.close();
		return ok;
	}
	else
		return false;
}

bool SlicesHandler::print_tissueindex(const char* filename,
									  bool onlyactiveslices,
									  const char* projname)
{
	std::ofstream streamname;
	streamname.open(filename, std::ios_base::binary);
	if (streamname.good())
	{
		bool ok = true;
		streamname << "0 \"Background\"\n";
		for (tissues_size_t tissuenr = 1;
			 tissuenr <= TissueInfos::GetTissueCount(); tissuenr++)
		{
			streamname << (int)tissuenr << " \""
					   << TissueInfos::GetTissueName(tissuenr).ascii()
					   << "\"\n";
		}
		streamname.close();
		return ok;
	}
	else
		return false;
}

bool SlicesHandler::get_extent(tissues_size_t tissuenr, bool onlyactiveslices,
							   unsigned short extent[3][2])
{
	bool found = false;
	unsigned short extent1[2][2];
	unsigned short startslice1, endslice1;
	startslice1 = 0;
	endslice1 = nrslices;
	if (onlyactiveslices)
	{
		startslice1 = startslice;
		endslice1 = endslice;
	}
	for (unsigned short i = startslice1; i < endslice1; i++)
	{
		if (!found)
		{
			found = image_slices[i].get_extent(active_tissuelayer, tissuenr,
											   extent1);
			if (found)
			{
				extent[2][0] = i;
				extent[2][1] = i;
				extent[0][0] = extent1[0][0];
				extent[1][0] = extent1[1][0];
				extent[0][1] = extent1[0][1];
				extent[1][1] = extent1[1][1];
			}
		}
		else
		{
			if (image_slices[i].get_extent(active_tissuelayer, tissuenr,
										   extent1))
			{
				if (extent1[0][0] < extent[0][0])
					extent[0][0] = extent1[0][0];
				if (extent1[1][0] < extent[1][0])
					extent[1][0] = extent1[1][0];
				if (extent1[0][1] > extent[0][1])
					extent[0][1] = extent1[0][1];
				if (extent1[1][1] > extent[1][1])
					extent[1][1] = extent1[1][1];
				extent[2][1] = i;
			}
		}
	}

	return found;
}

void SlicesHandler::add_skin3D(int ix, int iy, int iz, float setto)
{
	// ix,iy,iz are in pixels

	//Create skin in each slice as same way is done in the 2D AddSkin process
	for (unsigned short z = startslice; z < endslice; z++)
	{
		float* work;
		work = image_slices[z].return_work();
		unsigned i = 0, pos, y, x;

		//Create a binary vector noTissue/Tissue
		std::vector<int> s;
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < width; k++)
			{
				if (work[i] == 0)
					s.push_back(-1);
				else
					s.push_back(0);
				i++;
			}
		}

		// i4 itetations through  y, -y, x, -x converting, each time a tissue beginning is find, one tissue pixel into skin
		//!! It is assumed that ix and iy are the same
		bool convertSkin = true;
		for (i = 1; i < ix + 1; i++)
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

		//go over the vector and set the skin pixel at the source pointer
		i = 0;
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < width; k++)
			{
				if (s[i] == 256)
					work[i] = setto;
				i++;
			}
		}
	}

	// To create skin in 3D, it is check if where there is tissue, in the thickness (up&down) distance there is neither tissue nor skin
	//The conversion is already done.
	//int checkSliceDistance = int(iz/thickness);
	int checkSliceDistance = iz;

	for (unsigned short z = startslice; z < endslice - checkSliceDistance; z++)
	{
		float* work1;
		float* work2;
		work1 = image_slices[z].return_work();
		work2 = image_slices[z + checkSliceDistance].return_work();

		unsigned i = 0;
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < width; k++)
			{
				if (work1[i] != 0 && work1[i] != setto)
					if (work2[i] == 0)
						work1[i] = setto;
				i++;
			}
		}
	}

	for (unsigned short z = endslice - 1; z > startslice + checkSliceDistance;
		 z--)
	{
		float* work1;
		float* work2;
		work1 = image_slices[z].return_work();
		work2 = image_slices[z - checkSliceDistance].return_work();

		unsigned i = 0;
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < width; k++)
			{
				if (work1[i] != 0 && work1[i] != setto)
					if (work2[i] == 0)
						work1[i] = setto;
				i++;
			}
		}
	}

	// I leave the old implementation just in case there is something that is want to be recovered
	/*
	float set_to=(float)123E10;
	for(unsigned short z=startslice;z<endslice;z++) {
		image_slices[z].flood_exterior(set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	posit p1;

//	p1.pxy=position;
	p1.pz=activeslice;

	float *work1;
	float *work2;
	for(unsigned short z=startslice;z+1<endslice;z++) {
		work1=image_slices[z].return_work();
		work2=image_slices[z+1].return_work();
		for(unsigned long i=0;i<area;i++) {
			if(work1[i]==0&&work2[i]==set_to) {
				work1[i]=set_to;
				p1.pxy=i;
				p1.pz=z;
				s.push_back(p1);
			}
		}
	}
	for(unsigned short z=endslice-1;z>startslice;z--) {
		work1=image_slices[z].return_work();
		work2=image_slices[z-1].return_work();
		for(unsigned long i=0;i<area;i++) {
			if(work1[i]==0&&work2[i]==set_to) {
				work1[i]=set_to;
				p1.pxy=i;
				p1.pz=z;
				s.push_back(p1);
			}
		}
	}

	posit i,j;
	float *work;

	while(!s.empty()){
		i=s.back();
		s.pop_back();

		work=image_slices[i.pz].return_work();
		if(i.pxy%width!=0&&work[i.pxy-1]==0) {
			work[i.pxy-1]=set_to; 
			j.pxy=i.pxy-1;
			j.pz=i.pz;
			s.push_back(j);
		}
		if((i.pxy+1)%width!=0&&work[i.pxy+1]==0) {
			work[i.pxy+1]=set_to; 
			j.pxy=i.pxy+1;
			j.pz=i.pz;
			s.push_back(j);
		}
		if(i.pxy>=width&&work[i.pxy-width]==0) {
			work[i.pxy-width]=set_to; 
			j.pxy=i.pxy-width;
			j.pz=i.pz;
			s.push_back(j);
		}
		if(i.pxy<area-width&&work[i.pxy+width]==0) {
			work[i.pxy+width]=set_to; 
			j.pxy=i.pxy+width;
			j.pz=i.pz;
			s.push_back(j);
		}
		if(i.pz>startslice){
			work=image_slices[i.pz-1].return_work();
			if(work[i.pxy]==0) {
				work[i.pxy]=set_to; 
				j.pxy=i.pxy;
				j.pz=i.pz-1;
				s.push_back(j);
			}
		}
		if(i.pz+1<endslice){
			work=image_slices[i.pz+1].return_work();
			if(work[i.pxy]==0) {
				work[i.pxy]=set_to; 
				j.pxy=i.pxy;
				j.pz=i.pz+1;
				s.push_back(j);
			}
		}
	}

	unsigned short x,y;
	unsigned long pos;
	unsigned short i1;
	for(unsigned short z=startslice;z<endslice;z++){
		work=image_slices[z].return_work();

		for(y=0;y<height;y++){
			pos=y*width;
			i1=0;
			while(pos<(unsigned long)(y+1)*width){
				if(work[pos]==set_to) i1=ix;
				else {
					if(i1>0) {
						work[pos]=setto;
						i1--;
					}
				}
				pos++;
			}
		}

		for(y=0;y<height;y++){
			pos=(y+1)*width-1;
			i1=0;
			while(pos>(unsigned long)y*width){
				if(work[pos]==set_to) i1=ix;
				else {
					if(i1>0) {
						work[pos]=setto;
						i1--;
					}
				}
				pos--;
			}
			if(work[pos]==set_to) i1=ix;
			else {
				if(i1>0) {
					work[pos]=setto;
					i1--;
				}
			}
		}

		for(x=0;x<width;x++){
			pos=x;
			i1=0;
			while(pos<area){
				if(work[pos]==set_to) i1=iy;
				else {
					if(i1>0) {
						work[pos]=setto;
						i1--;
					}
				}
				pos+=width;
			}
		}

		for(x=0;x<width;x++){
			pos=area+x-width;
			i1=0;
			while(pos>width){
				if(work[pos]==set_to) i1=iy;
				else {
					if(i1>0) {
						work[pos]=setto;
						i1--;
					}
				}
				pos-=width;
			}
			if(work[pos]==set_to) i1=iy;
			else {
				if(i1>0) {
					work[pos]=setto;
					i1--;
				}
			}
		}
	}

	unsigned short *counter=(unsigned short*)malloc(sizeof(unsigned short)*area);
	for(unsigned i1=0;i1<area;i1++) counter[i1]=0;
	for(unsigned short z=startslice;z<endslice;z++){
		work=image_slices[z].return_work();
		for(pos=0;pos<area;pos++){
			if(work[pos]==set_to) counter[pos]=iz;
			else {
				if(counter[pos]>0) {
					work[pos]=setto;
					counter[pos]--;
				}
			}
		}
	}
	for(unsigned i1=0;i1<area;i1++) counter[i1]=0;
	for(unsigned short z=endslice-1;z>startslice;z--){
		work=image_slices[z].return_work();
		for(pos=0;pos<area;pos++){
			if(work[pos]==set_to) counter[pos]=iz;
			else {
				if(counter[pos]>0) {
					work[pos]=setto;
					counter[pos]--;
				}
			}
		}
	}
	work=image_slices[startslice].return_work();
	for(pos=0;pos<area;pos++){
		if(work[pos]==set_to) counter[pos]=iz;
		else {
			if(counter[pos]>0) {
				work[pos]=setto;
				counter[pos]--;
			}
		}
	}
	free(counter);

	for(unsigned short z=startslice;z<endslice;z++){
		work=image_slices[z].return_work();
		for(unsigned i1=0;i1<area;i1++) if(work[i1]==set_to) work[i1]=0;
	}*/

	return;
}

void SlicesHandler::add_skin3D_outside(int ix, int iy, int iz, float setto)
{
	float set_to = (float)123E10;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		image_slices[z].flood_exterior(set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	posit p1;

	//	p1.pxy=position;
	p1.pz = activeslice;

	float* work1;
	float* work2;
	for (unsigned short z = startslice; z + 1 < endslice; z++)
	{
		work1 = image_slices[z].return_work();
		work2 = image_slices[z + 1].return_work();
		for (unsigned long i = 0; i < area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = endslice - 1; z > startslice; z--)
	{
		work1 = image_slices[z].return_work();
		work2 = image_slices[z - 1].return_work();
		for (unsigned long i = 0; i < area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}

	posit i, j;
	float* work;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		work = image_slices[i.pz].return_work();
		if (i.pxy % width != 0 && work[i.pxy - 1] == 0)
		{
			work[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % width != 0 && work[i.pxy + 1] == 0)
		{
			work[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= width && work[i.pxy - width] == 0)
		{
			work[i.pxy - width] = set_to;
			j.pxy = i.pxy - width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy < area - width && work[i.pxy + width] == 0)
		{
			work[i.pxy + width] = set_to;
			j.pxy = i.pxy + width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > startslice)
		{
			work = image_slices[i.pz - 1].return_work();
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < endslice)
		{
			work = image_slices[i.pz + 1].return_work();
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz + 1;
				s.push_back(j);
			}
		}
	}

	unsigned short x, y;
	unsigned long pos;
	unsigned short i1;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		work = image_slices[z].return_work();

		for (y = 0; y < height; y++)
		{
			pos = y * width;
			i1 = 0;
			while (pos < (unsigned long)(y + 1) * width)
			{
				if (work[pos] != set_to)
					i1 = ix;
				else
				{
					if (i1 > 0)
					{
						work[pos] = setto;
						i1--;
					}
				}
				pos++;
			}
		}

		for (y = 0; y < height; y++)
		{
			pos = (y + 1) * width - 1;
			i1 = 0;
			while (pos > (unsigned long)y * width)
			{
				if (work[pos] != set_to)
					i1 = ix;
				else
				{
					if (i1 > 0)
					{
						work[pos] = setto;
						i1--;
					}
				}
				pos--;
			}
			if (work[pos] != set_to)
				i1 = ix;
			else
			{
				if (i1 > 0)
				{
					work[pos] = setto;
					i1--;
				}
			}
		}

		for (x = 0; x < width; x++)
		{
			pos = x;
			i1 = 0;
			while (pos < area)
			{
				if (work[pos] != set_to)
					i1 = iy;
				else
				{
					if (i1 > 0)
					{
						work[pos] = setto;
						i1--;
					}
				}
				pos += width;
			}
		}

		for (x = 0; x < width; x++)
		{
			pos = area + x - width;
			i1 = 0;
			while (pos > width)
			{
				if (work[pos] != set_to)
					i1 = iy;
				else
				{
					if (i1 > 0)
					{
						work[pos] = setto;
						i1--;
					}
				}
				pos -= width;
			}
			if (work[pos] != set_to)
				i1 = iy;
			else
			{
				if (i1 > 0)
				{
					work[pos] = setto;
					i1--;
				}
			}
		}
	}

	unsigned short* counter =
		(unsigned short*)malloc(sizeof(unsigned short) * area);
	for (unsigned i1 = 0; i1 < area; i1++)
		counter[i1] = 0;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		work = image_slices[z].return_work();
		for (pos = 0; pos < area; pos++)
		{
			if (work[pos] != set_to)
				counter[pos] = iz;
			else
			{
				if (counter[pos] > 0)
				{
					work[pos] = setto;
					counter[pos]--;
				}
			}
		}
	}
	for (unsigned i1 = 0; i1 < area; i1++)
		counter[i1] = 0;
	for (unsigned short z = endslice - 1; z > startslice; z--)
	{
		work = image_slices[z].return_work();
		for (pos = 0; pos < area; pos++)
		{
			if (work[pos] != set_to)
				counter[pos] = iz;
			else
			{
				if (counter[pos] > 0)
				{
					work[pos] = setto;
					counter[pos]--;
				}
			}
		}
	}
	work = image_slices[startslice].return_work();
	for (pos = 0; pos < area; pos++)
	{
		if (work[pos] != set_to)
			counter[pos] = iz;
		else
		{
			if (counter[pos] > 0)
			{
				work[pos] = setto;
				counter[pos]--;
			}
		}
	}
	free(counter);

	for (unsigned short z = startslice; z < endslice; z++)
	{
		work = image_slices[z].return_work();
		for (unsigned i1 = 0; i1 < area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}

	return;
}

void SlicesHandler::add_skin3D_outside2(int ix, int iy, int iz, float setto)
{
	float set_to = (float)123E10;
	float set_to2 = (float)321E10;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		image_slices[z].flood_exterior(set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	posit p1;

	//	p1.pxy=position;
	p1.pz = activeslice;

	float* work1;
	float* work2;
	for (unsigned short z = startslice; z + 1 < endslice; z++)
	{
		work1 = image_slices[z].return_work();
		work2 = image_slices[z + 1].return_work();
		for (unsigned long i = 0; i < area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = endslice - 1; z > startslice; z--)
	{
		work1 = image_slices[z].return_work();
		work2 = image_slices[z - 1].return_work();
		for (unsigned long i = 0; i < area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}

	posit i, j;
	float* work;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		work = image_slices[i.pz].return_work();
		if (i.pxy % width != 0 && work[i.pxy - 1] == 0)
		{
			work[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % width != 0 && work[i.pxy + 1] == 0)
		{
			work[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= width && work[i.pxy - width] == 0)
		{
			work[i.pxy - width] = set_to;
			j.pxy = i.pxy - width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy < area - width && work[i.pxy + width] == 0)
		{
			work[i.pxy + width] = set_to;
			j.pxy = i.pxy + width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > startslice)
		{
			work = image_slices[i.pz - 1].return_work();
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < endslice)
		{
			work = image_slices[i.pz + 1].return_work();
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz + 1;
				s.push_back(j);
			}
		}
	}

	unsigned int totcount = (ix + 1) * (iy + 1) * (iz + 1);
	unsigned int subx = (iy + 1) * (iz + 1);
	unsigned int suby = (ix + 1) * (iz + 1);
	unsigned int subz = (iy + 1) * (ix + 1);

	Treap<posit, unsigned int> treap1;
	if (iz > 0)
	{
		for (unsigned short z = startslice; z + 1 < endslice; z++)
		{
			work1 = image_slices[z].return_work();
			work2 = image_slices[z + 1].return_work();
			for (unsigned long i = 0; i < area; i++)
			{
				if (work1[i] == set_to)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.pxy = i;
						p1.pz = z;
						Treap<posit, unsigned int>::Node* n = NULL;
						treap1.insert(n, p1, 1, subz);
						work1[i] = set_to2;
					}
				}
				else if (work1[i] == set_to2)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.pxy = i;
						p1.pz = z;
						Treap<posit, unsigned int>::Node* n1 =
							treap1.lookup(p1);
						if (n1->getPriority() > subz)
							treap1.update_priority(n1, subz);
					}
				}
				else
				{
					if (work2[i] == set_to2)
					{
						p1.pxy = i;
						p1.pz = z + 1;
						Treap<posit, unsigned int>::Node* n1 =
							treap1.lookup(p1);
						if (n1->getPriority() > subz)
							treap1.update_priority(n1, subz);
					}
					else if (work2[i] == set_to)
					{
						p1.pxy = i;
						p1.pz = z + 1;
						Treap<posit, unsigned int>::Node* n = NULL;
						treap1.insert(n, p1, 1, subz);
						work2[i] = set_to2;
					}
				}
			}
		}
	}

	if (ix > 0)
	{
		for (unsigned short z = startslice; z < endslice; z++)
		{
			p1.pz = z;
			work1 = image_slices[z].return_work();
			unsigned long i = 0;
			for (unsigned short y = 0; y < height; y++)
			{
				for (unsigned short x = 0; x + 1 < width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + 1] != set_to) &&
							(work1[i + 1] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n = NULL;
							treap1.insert(n, p1, 1, subx);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + 1] != set_to) &&
							(work1[i + 1] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n1 =
								treap1.lookup(p1);
							if (n1->getPriority() > subx)
								treap1.update_priority(n1, subx);
						}
					}
					else
					{
						if (work1[i + 1] == set_to2)
						{
							p1.pxy = i + 1;
							Treap<posit, unsigned int>::Node* n1 =
								treap1.lookup(p1);
							if (n1->getPriority() > subx)
								treap1.update_priority(n1, subx);
						}
						else if (work1[i + 1] == set_to)
						{
							p1.pxy = i + 1;
							Treap<posit, unsigned int>::Node* n = NULL;
							treap1.insert(n, p1, 1, subx);
							work1[i + 1] = set_to2;
						}
					}
					i++;
				}
				i++;
			}
		}
	}

	if (iy > 0)
	{
		for (unsigned short z = startslice; z < endslice; z++)
		{
			p1.pz = z;
			work1 = image_slices[z].return_work();
			unsigned long i = 0;
			for (unsigned short y = 0; y + 1 < height; y++)
			{
				for (unsigned short x = 0; x < width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + width] != set_to) &&
							(work1[i + width] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n = NULL;
							treap1.insert(n, p1, 1, suby);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + width] != set_to) &&
							(work1[i + width] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n1 =
								treap1.lookup(p1);
							if (n1->getPriority() > suby)
								treap1.update_priority(n1, suby);
						}
					}
					else
					{
						if (work1[i + width] == set_to2)
						{
							p1.pxy = i + width;
							Treap<posit, unsigned int>::Node* n1 =
								treap1.lookup(p1);
							if (n1->getPriority() > suby)
								treap1.update_priority(n1, suby);
						}
						else if (work1[i + width] == set_to)
						{
							p1.pxy = i + width;
							Treap<posit, unsigned int>::Node* n = NULL;
							treap1.insert(n, p1, 1, suby);
							work1[i + width] = set_to2;
						}
					}
					i++;
				}
			}
		}
	}

	Treap<posit, unsigned int>::Node* n1;
	posit p2;
	unsigned int prior;
	while ((n1 = treap1.get_top()) != NULL)
	{
		p1 = n1->getKey();
		prior = n1->getPriority();
		work1 = image_slices[p1.pz].return_work();
		work1[p1.pxy] = setto;
		if (p1.pxy % width != 0)
		{
			if (work1[p1.pxy - 1] == set_to)
			{
				if (prior + 2 * subx <= totcount)
				{
					p2.pxy = p1.pxy - 1;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + subx);
					work1[p1.pxy - 1] = set_to2;
				}
			}
			else if (work1[p1.pxy - 1] == set_to2)
			{
				p2.pxy = p1.pxy - 1;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + subx)
					treap1.update_priority(n2, prior + subx);
			}
		}
		if ((p1.pxy + 1) % width != 0)
		{
			if (work1[p1.pxy + 1] == set_to)
			{
				if (prior + 2 * subx <= totcount)
				{
					p2.pxy = p1.pxy + 1;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + subx);
					work1[p1.pxy + 1] = set_to2;
				}
			}
			else if (work1[p1.pxy + 1] == set_to2)
			{
				p2.pxy = p1.pxy + 1;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + subx)
					treap1.update_priority(n2, prior + subx);
			}
		}
		if (p1.pxy >= width)
		{
			if (work1[p1.pxy - width] == set_to)
			{
				if (prior + 2 * suby <= totcount)
				{
					p2.pxy = p1.pxy - width;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + suby);
					work1[p1.pxy - width] = set_to2;
				}
			}
			else if (work1[p1.pxy - width] == set_to2)
			{
				p2.pxy = p1.pxy - width;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + suby)
					treap1.update_priority(n2, prior + suby);
			}
		}
		if (p1.pxy < area - width)
		{
			if (work1[p1.pxy + width] == set_to)
			{
				if (prior + 2 * suby <= totcount)
				{
					p2.pxy = p1.pxy + width;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + suby);
					work1[p1.pxy + width] = set_to2;
				}
			}
			else if (work1[p1.pxy + width] == set_to2)
			{
				p2.pxy = p1.pxy + width;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + suby)
					treap1.update_priority(n2, prior + suby);
			}
		}
		if (p1.pz > startslice)
		{
			work2 = image_slices[p1.pz - 1].return_work();
			if (work2[p1.pxy] == set_to)
			{
				if (prior + 2 * subz <= totcount)
				{
					p2.pxy = p1.pxy;
					p2.pz = p1.pz - 1;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + subz);
					work2[p1.pxy] = set_to2;
				}
			}
			else if (work2[p1.pxy] == set_to2)
			{
				p2.pxy = p1.pxy;
				p2.pz = p1.pz - 1;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + subz)
					treap1.update_priority(n2, prior + subz);
			}
		}
		if (p1.pz + 1 < endslice)
		{
			work2 = image_slices[p1.pz + 1].return_work();
			if (work2[p1.pxy] == set_to)
			{
				if (prior + 2 * subz <= totcount)
				{
					p2.pxy = p1.pxy;
					p2.pz = p1.pz + 1;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + subz);
					work2[p1.pxy] = set_to2;
				}
			}
			else if (work2[p1.pxy] == set_to2)
			{
				p2.pxy = p1.pxy;
				p2.pz = p1.pz + 1;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + subz)
					treap1.update_priority(n2, prior + subz);
			}
		}

		delete treap1.remove(n1);
	}

	for (unsigned short z = startslice; z < endslice; z++)
	{
		work = image_slices[z].return_work();
		for (unsigned i1 = 0; i1 < area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}

	return;
}

void SlicesHandler::add_skintissue3D_outside2(int ix, int iy, int iz,
											  tissues_size_t f)
{
	tissues_size_t set_to = TISSUES_SIZE_MAX;
	tissues_size_t set_to2 = TISSUES_SIZE_MAX - 1;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		image_slices[z].flood_exteriortissue(active_tissuelayer, set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	posit p1;

	//	p1.pxy=position;
	p1.pz = activeslice;

	tissues_size_t* work1;
	tissues_size_t* work2;
	for (unsigned short z = startslice; z + 1 < endslice; z++)
	{
		work1 = image_slices[z].return_tissues(active_tissuelayer);
		work2 = image_slices[z + 1].return_tissues(active_tissuelayer);
		for (unsigned long i = 0; i < area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = endslice - 1; z > startslice; z--)
	{
		work1 = image_slices[z].return_tissues(active_tissuelayer);
		work2 = image_slices[z - 1].return_tissues(active_tissuelayer);
		for (unsigned long i = 0; i < area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}

	posit i, j;
	tissues_size_t* work;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		work = image_slices[i.pz].return_tissues(active_tissuelayer);
		if (i.pxy % width != 0 && work[i.pxy - 1] == 0)
		{
			work[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % width != 0 && work[i.pxy + 1] == 0)
		{
			work[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= width && work[i.pxy - width] == 0)
		{
			work[i.pxy - width] = set_to;
			j.pxy = i.pxy - width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy < area - width && work[i.pxy + width] == 0)
		{
			work[i.pxy + width] = set_to;
			j.pxy = i.pxy + width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > startslice)
		{
			work = image_slices[i.pz - 1].return_tissues(active_tissuelayer);
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < endslice)
		{
			work = image_slices[i.pz + 1].return_tissues(active_tissuelayer);
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz + 1;
				s.push_back(j);
			}
		}
	}

	unsigned int totcount = (ix + 1) * (iy + 1) * (iz + 1);
	unsigned int subx = (iy + 1) * (iz + 1);
	unsigned int suby = (ix + 1) * (iz + 1);
	unsigned int subz = (iy + 1) * (ix + 1);

	Treap<posit, unsigned int> treap1;
	if (iz > 0)
	{
		for (unsigned short z = startslice; z + 1 < endslice; z++)
		{
			work1 = image_slices[z].return_tissues(active_tissuelayer);
			work2 = image_slices[z + 1].return_tissues(active_tissuelayer);
			for (unsigned long i = 0; i < area; i++)
			{
				if (work1[i] == set_to)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.pxy = i;
						p1.pz = z;
						Treap<posit, unsigned int>::Node* n = NULL;
						treap1.insert(n, p1, 1, subz);
						work1[i] = set_to2;
					}
				}
				else if (work1[i] == set_to2)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.pxy = i;
						p1.pz = z;
						Treap<posit, unsigned int>::Node* n1 =
							treap1.lookup(p1);
						if (n1->getPriority() > subz)
							treap1.update_priority(n1, subz);
					}
				}
				else
				{
					if (work2[i] == set_to2)
					{
						p1.pxy = i;
						p1.pz = z + 1;
						Treap<posit, unsigned int>::Node* n1 =
							treap1.lookup(p1);
						if (n1->getPriority() > subz)
							treap1.update_priority(n1, subz);
					}
					else if (work2[i] == set_to)
					{
						p1.pxy = i;
						p1.pz = z + 1;
						Treap<posit, unsigned int>::Node* n = NULL;
						treap1.insert(n, p1, 1, subz);
						work2[i] = set_to2;
					}
				}
			}
		}
	}

	if (ix > 0)
	{
		for (unsigned short z = startslice; z < endslice; z++)
		{
			p1.pz = z;
			work1 = image_slices[z].return_tissues(active_tissuelayer);
			unsigned long i = 0;
			for (unsigned short y = 0; y < height; y++)
			{
				for (unsigned short x = 0; x + 1 < width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + 1] != set_to) &&
							(work1[i + 1] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n = NULL;
							treap1.insert(n, p1, 1, subx);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + 1] != set_to) &&
							(work1[i + 1] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n1 =
								treap1.lookup(p1);
							if (n1->getPriority() > subx)
								treap1.update_priority(n1, subx);
						}
					}
					else
					{
						if (work1[i + 1] == set_to2)
						{
							p1.pxy = i + 1;
							Treap<posit, unsigned int>::Node* n1 =
								treap1.lookup(p1);
							if (n1->getPriority() > subx)
								treap1.update_priority(n1, subx);
						}
						else if (work1[i + 1] == set_to)
						{
							p1.pxy = i + 1;
							Treap<posit, unsigned int>::Node* n = NULL;
							treap1.insert(n, p1, 1, subx);
							work1[i + 1] = set_to2;
						}
					}
					i++;
				}
				i++;
			}
		}
	}

	if (iy > 0)
	{
		for (unsigned short z = startslice; z < endslice; z++)
		{
			p1.pz = z;
			work1 = image_slices[z].return_tissues(active_tissuelayer);
			unsigned long i = 0;
			for (unsigned short y = 0; y + 1 < height; y++)
			{
				for (unsigned short x = 0; x < width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + width] != set_to) &&
							(work1[i + width] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n = NULL;
							treap1.insert(n, p1, 1, suby);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + width] != set_to) &&
							(work1[i + width] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n1 =
								treap1.lookup(p1);
							if (n1->getPriority() > suby)
								treap1.update_priority(n1, suby);
						}
					}
					else
					{
						if (work1[i + width] == set_to2)
						{
							p1.pxy = i + width;
							Treap<posit, unsigned int>::Node* n1 =
								treap1.lookup(p1);
							if (n1->getPriority() > suby)
								treap1.update_priority(n1, suby);
						}
						else if (work1[i + width] == set_to)
						{
							p1.pxy = i + width;
							Treap<posit, unsigned int>::Node* n = NULL;
							treap1.insert(n, p1, 1, suby);
							work1[i + width] = set_to2;
						}
					}
					i++;
				}
			}
		}
	}

	Treap<posit, unsigned int>::Node* n1;
	posit p2;
	unsigned int prior;
	while ((n1 = treap1.get_top()) != NULL)
	{
		p1 = n1->getKey();
		prior = n1->getPriority();
		work1 = image_slices[p1.pz].return_tissues(active_tissuelayer);
		work1[p1.pxy] = f;
		if (p1.pxy % width != 0)
		{
			if (work1[p1.pxy - 1] == set_to)
			{
				if (prior + subx <= totcount)
				{
					p2.pxy = p1.pxy - 1;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + subx);
					work1[p1.pxy - 1] = set_to2;
				}
			}
			else if (work1[p1.pxy - 1] == set_to2)
			{
				p2.pxy = p1.pxy - 1;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + subx)
					treap1.update_priority(n2, prior + subx);
			}
		}
		if ((p1.pxy + 1) % width != 0)
		{
			if (work1[p1.pxy + 1] == set_to)
			{
				if (prior + subx <= totcount)
				{
					p2.pxy = p1.pxy + 1;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + subx);
					work1[p1.pxy + 1] = set_to2;
				}
			}
			else if (work1[p1.pxy + 1] == set_to2)
			{
				p2.pxy = p1.pxy + 1;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + subx)
					treap1.update_priority(n2, prior + subx);
			}
		}
		if (p1.pxy >= width)
		{
			if (work1[p1.pxy - width] == set_to)
			{
				if (prior + suby <= totcount)
				{
					p2.pxy = p1.pxy - width;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + suby);
					work1[p1.pxy - width] = set_to2;
				}
			}
			else if (work1[p1.pxy - width] == set_to2)
			{
				p2.pxy = p1.pxy - width;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + suby)
					treap1.update_priority(n2, prior + suby);
			}
		}
		if (p1.pxy < area - width)
		{
			if (work1[p1.pxy + width] == set_to)
			{
				if (prior + suby <= totcount)
				{
					p2.pxy = p1.pxy + width;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + suby);
					work1[p1.pxy + width] = set_to2;
				}
			}
			else if (work1[p1.pxy + width] == set_to2)
			{
				p2.pxy = p1.pxy + width;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + suby)
					treap1.update_priority(n2, prior + suby);
			}
		}
		if (p1.pz > startslice)
		{
			work2 = image_slices[p1.pz - 1].return_tissues(active_tissuelayer);
			if (work2[p1.pxy] == set_to)
			{
				if (prior + subz <= totcount)
				{
					p2.pxy = p1.pxy;
					p2.pz = p1.pz - 1;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + subz);
					work2[p1.pxy] = set_to2;
				}
			}
			else if (work2[p1.pxy] == set_to2)
			{
				p2.pxy = p1.pxy;
				p2.pz = p1.pz - 1;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + subz)
					treap1.update_priority(n2, prior + subz);
			}
		}
		if (p1.pz + 1 < endslice)
		{
			work2 = image_slices[p1.pz + 1].return_tissues(active_tissuelayer);
			if (work2[p1.pxy] == set_to)
			{
				if (prior + subz <= totcount)
				{
					p2.pxy = p1.pxy;
					p2.pz = p1.pz + 1;
					Treap<posit, unsigned int>::Node* n = NULL;
					treap1.insert(n, p2, 1, prior + subz);
					work2[p1.pxy] = set_to2;
				}
			}
			else if (work2[p1.pxy] == set_to2)
			{
				p2.pxy = p1.pxy;
				p2.pz = p1.pz + 1;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + subz)
					treap1.update_priority(n2, prior + subz);
			}
		}

		delete treap1.remove(n1);
	}

	for (unsigned short z = startslice; z < endslice; z++)
	{
		work = image_slices[z].return_tissues(active_tissuelayer);
		for (unsigned i1 = 0; i1 < area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}

	return;
}

float SlicesHandler::add_skin3D(int i1)
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

		for (unsigned short i = startslice; i < endslice; i++)
		{
			float* bits = image_slices[i].return_work();
			for (unsigned pos = 0; pos < area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	add_skin3D(i1, i1, i1, setto);
	return setto;
}

float SlicesHandler::add_skin3D(int ix, int iy, int iz)
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

		for (unsigned short i = startslice; i < endslice; i++)
		{
			float* bits = image_slices[i].return_work();
			for (unsigned pos = 0; pos < area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	// ix,iy,iz are in pixels
	add_skin3D(ix, iy, iz, setto);
	return setto;
}

float SlicesHandler::add_skin3D_outside(int i1)
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

		for (unsigned short i = startslice; i < endslice; i++)
		{
			float* bits = image_slices[i].return_work();
			for (unsigned pos = 0; pos < area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	add_skin3D_outside(i1, i1, i1, setto);
	return setto;
}

float SlicesHandler::add_skin3D_outside2(int ix, int iy, int iz)
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

		for (unsigned short i = startslice; i < endslice; i++)
		{
			float* bits = image_slices[i].return_work();
			for (unsigned pos = 0; pos < area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	add_skin3D_outside2(ix, iy, iz, setto);
	return setto;
}

void SlicesHandler::add_skintissue3D(int ix, int iy, int iz, tissues_size_t f)
{
	tissues_size_t set_to = TISSUES_SIZE_MAX;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		image_slices[z].flood_exteriortissue(active_tissuelayer, set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	posit p1;

	//	p1.pxy=position;
	p1.pz = activeslice;

	tissues_size_t* tissue1;
	tissues_size_t* tissue2;
	for (unsigned short z = startslice; z + 1 < endslice; z++)
	{
		tissue1 = image_slices[z].return_tissues(active_tissuelayer);
		tissue2 = image_slices[z + 1].return_tissues(active_tissuelayer);
		for (unsigned long i = 0; i < area; i++)
		{
			if (tissue1[i] == 0 && tissue2[i] == set_to)
			{
				tissue1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = endslice - 1; z > startslice; z--)
	{
		tissue1 = image_slices[z].return_tissues(active_tissuelayer);
		tissue2 = image_slices[z - 1].return_tissues(active_tissuelayer);
		for (unsigned long i = 0; i < area; i++)
		{
			if (tissue1[i] == 0 && tissue2[i] == set_to)
			{
				tissue1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}

	posit i, j;
	tissues_size_t* tissue;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		tissue = image_slices[i.pz].return_tissues(active_tissuelayer);
		if (i.pxy % width != 0 && tissue[i.pxy - 1] == 0)
		{
			tissue[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % width != 0 && tissue[i.pxy + 1] == 0)
		{
			tissue[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= width && tissue[i.pxy - width] == 0)
		{
			tissue[i.pxy - width] = set_to;
			j.pxy = i.pxy - width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy < area - width && tissue[i.pxy + width] == 0)
		{
			tissue[i.pxy + width] = set_to;
			j.pxy = i.pxy + width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > startslice)
		{
			tissue = image_slices[i.pz - 1].return_tissues(active_tissuelayer);
			if (tissue[i.pxy] == 0)
			{
				tissue[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < endslice)
		{
			tissue = image_slices[i.pz + 1].return_tissues(active_tissuelayer);
			if (tissue[i.pxy] == 0)
			{
				tissue[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz + 1;
				s.push_back(j);
			}
		}
	}

	unsigned short x, y;
	unsigned long pos;
	unsigned short i1;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		tissue = image_slices[z].return_tissues(active_tissuelayer);

		for (y = 0; y < height; y++)
		{
			pos = y * width;
			i1 = 0;
			while (pos < (unsigned long)(y + 1) * width)
			{
				if (tissue[pos] == set_to)
					i1 = ix;
				else
				{
					if (i1 > 0 && (!TissueInfos::GetTissueLocked(tissue[pos])))
					{
						tissue[pos] = f;
						i1--;
					}
				}
				pos++;
			}
		}

		for (y = 0; y < height; y++)
		{
			pos = (y + 1) * width - 1;
			i1 = 0;
			while (pos > (unsigned long)y * width)
			{
				if (tissue[pos] == set_to)
					i1 = ix;
				else
				{
					if (i1 > 0)
					{
						if (!TissueInfos::GetTissueLocked(tissue[pos]))
							tissue[pos] = f;
						i1--;
					}
				}
				pos--;
			}
			if (tissue[pos] == set_to)
				i1 = ix;
			else
			{
				if (i1 > 0)
				{
					if (!TissueInfos::GetTissueLocked(tissue[pos]))
						tissue[pos] = f;
					i1--;
				}
			}
		}

		for (x = 0; x < width; x++)
		{
			pos = x;
			i1 = 0;
			while (pos < area)
			{
				if (tissue[pos] == set_to)
					i1 = iy;
				else
				{
					if (i1 > 0)
					{
						if (!TissueInfos::GetTissueLocked(tissue[pos]))
							tissue[pos] = f;
						i1--;
					}
				}
				pos += width;
			}
		}

		for (x = 0; x < width; x++)
		{
			pos = area + x - width;
			i1 = 0;
			while (pos > width)
			{
				if (tissue[pos] == set_to)
					i1 = iy;
				else
				{
					if (i1 > 0)
					{
						if (!TissueInfos::GetTissueLocked(tissue[pos]))
							tissue[pos] = f;
						i1--;
					}
				}
				pos -= width;
			}
			if (tissue[pos] == set_to)
				i1 = iy;
			else
			{
				if (i1 > 0)
				{
					if (!TissueInfos::GetTissueLocked(tissue[pos]))
						tissue[pos] = f;
					i1--;
				}
			}
		}
	}

	unsigned short* counter =
		(unsigned short*)malloc(sizeof(unsigned short) * area);
	for (unsigned i1 = 0; i1 < area; i1++)
		counter[i1] = 0;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		tissue = image_slices[z].return_tissues(active_tissuelayer);
		for (pos = 0; pos < area; pos++)
		{
			if (tissue[pos] == set_to)
				counter[pos] = iz;
			else
			{
				if (counter[pos] > 0)
				{
					if (!TissueInfos::GetTissueLocked(tissue[pos]))
						tissue[pos] = f;
					counter[pos]--;
				}
			}
		}
	}
	for (unsigned i1 = 0; i1 < area; i1++)
		counter[i1] = 0;
	for (unsigned short z = endslice - 1; z > startslice; z--)
	{
		tissue = image_slices[z].return_tissues(active_tissuelayer);
		for (pos = 0; pos < area; pos++)
		{
			if (tissue[pos] == set_to)
				counter[pos] = iz;
			else
			{
				if (counter[pos] > 0)
				{
					if (!TissueInfos::GetTissueLocked(tissue[pos]))
						tissue[pos] = f;
					counter[pos]--;
				}
			}
		}
	}
	tissue = image_slices[startslice].return_tissues(active_tissuelayer);
	for (pos = 0; pos < area; pos++)
	{
		if (tissue[pos] == set_to)
			counter[pos] = iz;
		else
		{
			if (counter[pos] > 0)
			{
				if (!TissueInfos::GetTissueLocked(tissue[pos]))
					tissue[pos] = f;
				counter[pos]--;
			}
		}
	}
	free(counter);

	for (unsigned short z = startslice; z < endslice; z++)
	{
		tissue = image_slices[z].return_tissues(active_tissuelayer);
		for (unsigned i1 = 0; i1 < area; i1++)
			if (tissue[i1] == set_to)
				tissue[i1] = 0;
	}

	return;
}

void SlicesHandler::add_skintissue3D_outside(int ixyz, tissues_size_t f)
{
	tissues_size_t set_to = TISSUES_SIZE_MAX;
	for (unsigned short z = startslice; z < endslice; z++)
	{
		image_slices[z].flood_exteriortissue(active_tissuelayer, set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	std::vector<posit> s1;
	std::vector<posit>* s2 = &s1;
	std::vector<posit>* s3 = &s;
	posit p1;

	//	p1.pxy=position;
	p1.pz = activeslice;

	tissues_size_t* tissue1;
	tissues_size_t* tissue2;
	for (unsigned short z = startslice; z + 1 < endslice; z++)
	{
		tissue1 = image_slices[z].return_tissues(active_tissuelayer);
		tissue2 = image_slices[z + 1].return_tissues(active_tissuelayer);
		for (unsigned long i = 0; i < area; i++)
		{
			if (tissue1[i] == 0 && tissue2[i] == set_to)
			{
				tissue1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = endslice - 1; z > startslice; z--)
	{
		tissue1 = image_slices[z].return_tissues(active_tissuelayer);
		tissue2 = image_slices[z - 1].return_tissues(active_tissuelayer);
		for (unsigned long i = 0; i < area; i++)
		{
			if (tissue1[i] == 0 && tissue2[i] == set_to)
			{
				tissue1[i] = set_to;
				p1.pxy = i;
				p1.pz = z;
				s.push_back(p1);
			}
		}
	}

	posit i, j;
	tissues_size_t* tissue;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		tissue = image_slices[i.pz].return_tissues(active_tissuelayer);
		if (i.pxy % width != 0 && tissue[i.pxy - 1] == 0)
		{
			tissue[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % width != 0 && tissue[i.pxy + 1] == 0)
		{
			tissue[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= width && tissue[i.pxy - width] == 0)
		{
			tissue[i.pxy - width] = set_to;
			j.pxy = i.pxy - width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy <= area - width && tissue[i.pxy + width] == 0)
		{
			tissue[i.pxy + width] = set_to;
			j.pxy = i.pxy + width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > startslice)
		{
			tissue = image_slices[i.pz - 1].return_tissues(active_tissuelayer);
			if (tissue[i.pxy] == 0)
			{
				tissue[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < endslice)
		{
			tissue = image_slices[i.pz + 1].return_tissues(active_tissuelayer);
			if (tissue[i.pxy] == 0)
			{
				tissue[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz + 1;
				s.push_back(j);
			}
		}
	}

	for (unsigned short z = startslice; z + 1 < endslice; z++)
	{
		tissue1 = image_slices[z].return_tissues(active_tissuelayer);
		tissue2 = image_slices[z + 1].return_tissues(active_tissuelayer);
		for (unsigned long i = 0; i < area; i++)
		{
			if (tissue1[i] == set_to)
			{
				if (tissue2[i] != set_to)
				{
					p1.pxy = i;
					p1.pz = z + 1;
					(*s2).push_back(p1);
				}
			}
			else
			{
				if (tissue2[i] == set_to)
				{
					p1.pxy = i;
					p1.pz = z;
					(*s2).push_back(p1);
				}
			}
		}
	}

	for (unsigned short z = startslice; z < endslice; z++)
	{
		tissue1 = image_slices[z].return_tissues(active_tissuelayer);
		unsigned long i = 0;
		for (unsigned short y = 0; y < height; y++)
		{
			for (unsigned short x = 0; x + 1 < width; x++)
			{
				if (tissue1[i] == set_to)
				{
					if (tissue1[i + 1] != set_to)
					{
						p1.pxy = i + 1;
						p1.pz = z;
						(*s2).push_back(p1);
					}
				}
				else
				{
					if (tissue1[i + 1] == set_to)
					{
						p1.pxy = i;
						p1.pz = z;
						(*s2).push_back(p1);
					}
				}
				i++;
			}
			i++;
		}
		i = 0;
		for (unsigned short y = 0; y + 1 < height; y++)
		{
			for (unsigned short x = 0; x < width; x++)
			{
				if (tissue1[i] == set_to)
				{
					if (tissue1[i + width] != set_to)
					{
						p1.pxy = i + width;
						p1.pz = z;
						(*s2).push_back(p1);
					}
				}
				else
				{
					if (tissue1[i + width] == set_to)
					{
						p1.pxy = i;
						p1.pz = z;
						(*s2).push_back(p1);
					}
				}
				i++;
			}
		}
	}

	for (int i4 = 0; i4 < ixyz; i4++)
	{
		while (!(*s2).empty())
		{
			i = (*s2).back();
			(*s2).pop_back();

			tissue = image_slices[i.pz].return_tissues(active_tissuelayer);
			if (i.pxy % width != 0 && tissue[i.pxy - 1] == set_to)
			{
				tissue[i.pxy - 1] = f;
				j.pxy = i.pxy - 1;
				j.pz = i.pz;
				(*s3).push_back(j);
			}
			if ((i.pxy + 1) % width != 0 && tissue[i.pxy + 1] == set_to)
			{
				tissue[i.pxy + 1] = f;
				j.pxy = i.pxy + 1;
				j.pz = i.pz;
				(*s3).push_back(j);
			}
			if (i.pxy >= width && tissue[i.pxy - width] == set_to)
			{
				tissue[i.pxy - width] = f;
				j.pxy = i.pxy - width;
				j.pz = i.pz;
				(*s3).push_back(j);
			}
			if (i.pxy < area - width && tissue[i.pxy + width] == set_to)
			{
				tissue[i.pxy + width] = f;
				j.pxy = i.pxy + width;
				j.pz = i.pz;
				(*s3).push_back(j);
			}
			if (i.pz > startslice)
			{
				tissue =
					image_slices[i.pz - 1].return_tissues(active_tissuelayer);
				if (tissue[i.pxy] == set_to)
				{
					tissue[i.pxy] = f;
					j.pxy = i.pxy;
					j.pz = i.pz - 1;
					(*s3).push_back(j);
				}
			}
			if (i.pz + 1 < endslice)
			{
				tissue =
					image_slices[i.pz + 1].return_tissues(active_tissuelayer);
				if (tissue[i.pxy] == set_to)
				{
					tissue[i.pxy] = f;
					j.pxy = i.pxy;
					j.pz = i.pz + 1;
					(*s3).push_back(j);
				}
			}
		}
		std::vector<posit>* sdummy = s2;
		s2 = s3;
		s3 = sdummy;
	}

	while (!(*s2).empty())
	{
		(*s2).pop_back();
	}

	for (unsigned short z = startslice; z < endslice; z++)
	{
		tissue = image_slices[z].return_tissues(active_tissuelayer);
		for (unsigned i1 = 0; i1 < area; i1++)
			if (tissue[i1] == set_to)
				tissue[i1] = 0;
	}

	return;
}

void SlicesHandler::fill_skin_3d(int thicknessX, int thicknessY, int thicknessZ,
								 tissues_size_t backgroundID,
								 tissues_size_t skinID)
{
	int numTasks = nrslices;
	QProgressDialog progress("Fill Skin in progress...", "Cancel", 0, numTasks);
	progress.show();
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.setValue(0);

	int skinThick = thicknessX;

	std::vector<int> dims;
	dims.push_back(width);
	dims.push_back(height);
	dims.push_back(nrslices);

	double max_d = skinThick == 1 ? 1.75 * skinThick : 1.2 * skinThick;

	//Create the relative neighborhood
	std::vector<std::vector<int>> offsetsSlices;
	for (int z = -skinThick; z <= skinThick; z++)
	{
		std::vector<int> offsets;
		for (int y = -skinThick; y <= skinThick; y++)
		{
			for (int x = -skinThick; x <= skinThick; x++)
			{
				if (sqrt(x * x + y * y + z * z) < max_d)
				{
					int shift = x + y * dims[0];
					if (shift != 0)
					{
						offsets.push_back(shift);
					}
				}
			}
		}
		offsetsSlices.push_back(offsets);
	}

	struct changesToMakeStruct
	{
		size_t sliceNumber;
		int positionConvert;

		bool operator==(const changesToMakeStruct& m) const
		{
			return ((m.sliceNumber == sliceNumber) &&
					(m.positionConvert == positionConvert));
		}
	};

	bool thereIsBG = false;
	bool thereIsSkin = false;
	//unsigned long bgPixels=0;
	//unsigned long skinPixels=0;
	for (int i = 0; i < dims[2]; i++)
	{
		tissues_size_t* tissuesMain = image_slices[i].return_tissues(0);
		for (int j = 0; j < area && (!thereIsBG || !thereIsSkin); j++)
		{
			tissues_size_t value = tissuesMain[j];
			if (value == backgroundID)
				//bgPixels++;
				thereIsBG = true;
			else if (value == skinID)
				//skinPixels++;
				thereIsSkin = true;
		}
		if (thereIsSkin && thereIsBG)
			break;
	}

	if (!thereIsBG || !thereIsSkin)
	{
		progress.setValue(numTasks);
		return;
	}

	std::vector<tissues_size_t*> tissuesVector;
	for (int i = 0; i < image_slices.size(); i++)
		tissuesVector.push_back(image_slices[i].return_tissues(0));

	for (int i = 0; i < dims[2]; i++)
	{
		float* bmp1 = image_slices[i].return_bmp();

		tissues_size_t* tissue1 = image_slices[i].return_tissues(0);
		image_slices[i].pushstack_bmp();

		for (unsigned int j = 0; j < area; j++)
		{
			bmp1[j] = (float)tissue1[j];
		}

		image_slices[i].dead_reckoning((float)0);
		bmp1 = image_slices[i].return_work();
	}

#ifdef NO_OPENMP_SUPPORT
	const int numberThreads = 1;
#else
	const int numberThreads = omp_get_max_threads();
#endif
	int sliceCounter = 0;
	std::vector<std::vector<changesToMakeStruct>> partialChangesThreads;

	for (int i = 0; i < numberThreads; i++)
	{
		changesToMakeStruct sample;
		sample.sliceNumber = -1;
		sample.positionConvert = -1;
		std::vector<changesToMakeStruct> samplePerThread;
		samplePerThread.push_back(sample);
		partialChangesThreads.push_back(samplePerThread);
	}

	//if(skinPixels<bgPixels/neighbors)
	{
#pragma omp parallel for
		for (int k = skinThick; k < dims[2] - skinThick; k++)
		{
			std::vector<changesToMakeStruct> partialChanges;

			Point p;
			for (int j = skinThick; j < dims[1] - skinThick; j++)
			{
				for (int i = skinThick; i < dims[0] - skinThick; i++)
				{
					int pos = i + j * dims[0];
					if (tissuesVector[k][pos] == skinID)
					{
						for (int z = 0; z < offsetsSlices.size(); z++)
						{
							size_t neighborSlice =
								z - ((offsetsSlices.size() - 1) / 2);

							//iterate through neighbors of pixel
							//if any of these are not
							for (int l = 0; l < offsetsSlices[z].size(); l++)
							{
								int idx = pos + offsetsSlices[z][l];
								assert(idx >= 0 && idx < area);
								p.px = i;
								p.py = j;
								tissues_size_t value =
									tissuesVector[k + neighborSlice][idx];
								if (value == backgroundID)
								{
									for (int y = 0; y < offsetsSlices.size();
										 y++)
									{
										size_t neighborSlice2 =
											y -
											((offsetsSlices.size() - 1) / 2);

										//iterate through neighbors of pixel
										//if any of these are not
										for (int m = 0;
											 m < offsetsSlices[y].size(); m++)
										{
											int idx2 =
												idx + offsetsSlices[y][m];
											assert(idx2 >= 0 && idx2 < area);
											if (k + neighborSlice +
														neighborSlice2 >=
													0 &&
												k + neighborSlice +
														neighborSlice2 <
													dims[2])
											{
												tissues_size_t value2 =
													tissuesVector
														[k + neighborSlice +
														 neighborSlice2][idx2];
												if (value2 != backgroundID &&
													value2 != skinID)
												{
													changesToMakeStruct changes;
													changes.sliceNumber =
														k + neighborSlice;
													changes.positionConvert =
														idx;
													if (std::find(partialChanges
																	  .begin(),
																  partialChanges
																	  .end(),
																  changes) ==
														partialChanges.end())
														partialChanges
															.push_back(changes);
													//image_slices[k+neighborSlice].return_work()[idx]=255.0f;
													break;
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}

#ifdef NO_OPENMP_SUPPORT
			int thread_id = 0;
#else
			int thread_id = omp_get_thread_num();
#endif

			for (int i = 0; i < partialChanges.size(); i++)
			{
				partialChangesThreads[thread_id].push_back(partialChanges[i]);
			}

			if (0 == thread_id)
			{
				sliceCounter++;
				progress.setValue(numberThreads * sliceCounter);
			}
		}
	}

	for (int i = 0; i < partialChangesThreads.size(); i++)
		for (int j = 0; j < partialChangesThreads[i].size(); j++)
			if (partialChangesThreads[i][j].sliceNumber != -1)
			{
				size_t slice = partialChangesThreads[i][j].sliceNumber;
				int pos = partialChangesThreads[i][j].positionConvert;
				image_slices[slice].return_work()[pos] = 255.0f;
			}

	for (int i = dims[2] - 1; i >= 0; i--)
	{
		float* bmp1 = image_slices[i].return_bmp();

		for (unsigned k = 0; k < area; k++)
		{
			if (bmp1[k] < 0)
				bmp1[k] = 0;
			else
				bmp1[k] = 255.0f;
		}

		image_slices[i].set_mode(2, false);
		image_slices[i].popstack_bmp();
	}

	progress.setValue(numTasks);
}

float SlicesHandler::calculate_volume(Point p, unsigned short slicenr)
{
	Pair p1 = get_pixelsize();
	unsigned long count = 0;
	float f = get_work_pt(p, slicenr);
	for (unsigned short j = startslice; j < endslice; j++)
		count += image_slices[j].return_workpixelcount(f);
	return get_slicethickness() * p1.high * p1.low * count;
}

float SlicesHandler::calculate_tissuevolume(Point p, unsigned short slicenr)
{
	Pair p1 = get_pixelsize();
	unsigned long count = 0;
	tissues_size_t c = get_tissue_pt(p, slicenr);
	for (unsigned short j = startslice; j < endslice; j++)
		count += image_slices[j].return_tissuepixelcount(active_tissuelayer, c);
	return get_slicethickness() * p1.high * p1.low * count;
}

void SlicesHandler::inversesliceorder()
{
	if (nrslices > 0)
	{
		//		bmphandler dummy;
		unsigned short rcounter = nrslices - 1;
		for (unsigned short i = 0; i < nrslices / 2; i++, rcounter--)
		{
			image_slices[i].swap(image_slices[rcounter]);
			/*dummy=image_slices[i];
			image_slices[i]=image_slices[rcounter];
			image_slices[rcounter]=dummy;*/
		}
		reverse_undosliceorder();
	}
}

void SlicesHandler::gamma_mhd(unsigned short slicenr, short nrtissues,
							  short dim, std::vector<std::string> mhdfiles,
							  float* weights, float** centers, float* tol_f,
							  float* tol_d)
{
	if (mhdfiles.size() + 1 < dim)
		return;
	//	if(slicenr>=startslice&&slicenr<endslice){
	float** bits = new float*[dim];
	for (unsigned short j = 0; j + 1 < dim; j++)
	{
		bits[j + 1] = new float[area];
		if (bits[j + 1] == NULL)
		{
			for (unsigned short i = 1; i < j; i++)
				delete[] bits[i];
			delete[] bits;
			return;
		}
	}

	for (unsigned short i = startslice; i < endslice; i++)
	{
		bits[0] = image_slices[i].return_bmp();
		for (unsigned short k = 0; k + 1 < dim; k++)
		{
			if (!ImageReader::getSlice(mhdfiles[k].c_str(), bits[k + 1], i,
									   width, height))
			{
				for (unsigned short j = 1; j < dim; j++)
					delete[] bits[j];
				delete[] bits;
				return;
			}
		}
		MultidimensionalGamma mdg;
		Pair pair1 = get_pixelsize();
		mdg.init(width, height, nrtissues, dim, bits, weights, centers, tol_f,
				 tol_d, pair1.high, pair1.low);
		mdg.execute();
		mdg.return_image(image_slices[i].return_work());
		image_slices[i].set_mode(2, false);
	}

	for (unsigned short j = 1; j < dim; j++)
		delete[] bits[j];
	delete[] bits;
	//	}
}

void SlicesHandler::stepsmooth_z(unsigned short n)
{
	if (n > (endslice - startslice))
		return;
	SmoothSteps stepsm;
	unsigned short linelength1 = endslice - startslice;
	tissues_size_t* line = new tissues_size_t[linelength1];
	stepsm.init(n, linelength1, TissueInfos::GetTissueCount() + 1);
	for (unsigned long i = 0; i < area; i++)
	{
		unsigned short k = 0;
		for (unsigned short j = startslice; j < endslice; j++, k++)
		{
			line[k] = (image_slices[j].return_tissues(active_tissuelayer))[i];
		}
		stepsm.dostepsmooth(line);
		k = 0;
		for (unsigned short j = startslice; j < endslice; j++, k++)
		{
			(image_slices[j].return_tissues(active_tissuelayer))[i] = line[k];
		}
	}
	delete[] line;
}

void SlicesHandler::smooth_tissues(unsigned short n)
{
	// TODO: Implement criterion: Cells must be contiguous to the center of the specified filter
	std::map<tissues_size_t, unsigned short> tissuecount;
	tissues_size_t* tissuesnew =
		(tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
	if (n % 2 == 0)
		n++;
	unsigned short filtersize = n * n;
	n = (unsigned short)(0.5f * n);

	for (unsigned short j = startslice; j < endslice; j++)
	{
		image_slices[j].copyfromtissue(active_tissuelayer, tissuesnew);
		tissues_size_t* tissuesold =
			image_slices[j].return_tissues(active_tissuelayer);
		for (unsigned short y = n; y < height - n; y++)
		{
			for (unsigned short x = n; x < width - n; x++)
			{
				// Tissue count within filter size
				tissuecount.clear();
				for (short i = -n; i <= n; i++)
				{
					for (short j = -n; j <= n; j++)
					{
						tissues_size_t currtissue =
							tissuesold[(y + i) * width + (x + j)];
						if (tissuecount.find(currtissue) == tissuecount.end())
						{
							tissuecount.insert(
								std::pair<tissues_size_t, unsigned short>(
									currtissue, 1));
						}
						else
						{
							tissuecount[currtissue]++;
						}
					}
				}
#if 0 // Majority                                                              \
			// Find tissue with majority
				tissues_size_t tissuemajor = tissuesold[y*width+x];
				unsigned short tissuemajorcount = tissuecount[tissuemajor];
				for (std::map<tissues_size_t, unsigned short>::iterator iter = tissuecount.begin();
					iter != tissuecount.end(); ++iter) {
					if (iter->second > tissuemajorcount) {
						tissuemajor = iter->first;
						tissuemajorcount = iter->second;
					}
				}
#else // Majority and half or more                                             \
			// Find tissue covering at least half of the pixels
				unsigned short halffiltersize =
					(unsigned short)(0.5f * filtersize + 0.5f);
				tissues_size_t tissuemajor = tissuesold[y * width + x];
				unsigned short tissuemajorcount = tissuecount[tissuemajor];
				for (std::map<tissues_size_t, unsigned short>::iterator iter =
						 tissuecount.begin();
					 iter != tissuecount.end(); ++iter)
				{
					if (iter->second > tissuemajorcount &&
						iter->second >= halffiltersize)
					{
						tissuemajor = iter->first;
						tissuemajorcount = iter->second;
					}
				}
#endif
				// Assign new tissue
				tissuesnew[y * width + x] = tissuemajor;
			}
		}
		image_slices[j].copy2tissue(active_tissuelayer, tissuesnew);
	}

	free(tissuesnew);
}

void SlicesHandler::regrow(unsigned short sourceslicenr,
						   unsigned short targetslicenr, int n)
{
	float* lbmap = (float*)malloc(sizeof(float) * area);
	for (unsigned i = 0; i < area; i++)
		lbmap[i] = 0;

	tissues_size_t* dummy;
	tissues_size_t* results1 =
		(tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
	tissues_size_t* results2 =
		(tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
	tissues_size_t* tissues1 =
		image_slices[sourceslicenr].return_tissues(active_tissuelayer);
	for (unsigned int i = 0; i < area; i++)
	{
		if (tissues1[i] == 0)
			results2[i] = TISSUES_SIZE_MAX;
		else
			results2[i] = tissues1[i];
	}

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < area; i++)
			results1[i] = results2[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (height - 1); i++)
		{
			for (unsigned short j = 0; j < width; j++)
			{
				if (results2[i1] != results2[i1 + width])
					results1[i1] = results1[i1 + width] = 0;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < height; i++)
		{
			for (unsigned short j = 0; j < (width - 1); j++)
			{
				if (results2[i1] != results2[i1 + 1])
					results1[i1] = results1[i1 + 1] = 0;

				i1++;
			}
			i1++;
		}

		dummy = results1;
		results1 = results2;
		results2 = dummy;
	}

	for (unsigned int i = 0; i < area; i++)
		lbmap[i] = float(results2[i]);

	delete results1;
	delete results2;

	ImageForestingTransformRegionGrowing* IFTrg =
		image_slices[sourceslicenr].IFTrg_init(lbmap);
	float thresh = 0;

	float* f2 = IFTrg->return_pf();
	for (unsigned i = 0; i < area; i++)
	{
		if (thresh < f2[i])
		{
			thresh = f2[i];
		}
	}
	if (thresh == 0)
		thresh = 1;

	float* f1 = IFTrg->return_lb();
	tissues_size_t* tissue_bits =
		image_slices[targetslicenr].return_tissues(active_tissuelayer);

	for (unsigned i = 0; i < area; i++)
	{
		// if(f2[i]<thresh&&f1[i]<255.1f) tissue_bits[i]=(tissues_size_t)(f1[i]+0.1f);
		if (f2[i] < thresh && f1[i] < (float)TISSUES_SIZE_MAX + 0.1f)
			tissue_bits[i] = (tissues_size_t)(f1[i] + 0.1f); // TODO: Correct?
		else
			tissue_bits[i] = 0;
	}

	delete IFTrg;
	delete lbmap;
}

bool SlicesHandler::unwrap(float jumpratio, float shift)
{
	bool ok = true;
	Pair p;
	get_bmprange(&p);
	float range = p.high - p.low;
	float jumpabs = jumpratio * range;
	for (unsigned short i = startslice; i < endslice; i++)
	{
		if (i > startslice)
		{
			if ((image_slices[i - 1]).return_bmp()[0] -
					(image_slices[i]).return_bmp()[0] >
				jumpabs)
				shift += range;
			else if ((image_slices[i]).return_bmp()[0] -
						 (image_slices[i - 1]).return_bmp()[0] >
					 jumpabs)
				shift -= range;
		}
		ok &= (image_slices[i]).unwrap(jumpratio, range, shift);
		;
	}
	if (area > 0)
	{
		for (unsigned short i = startslice + 1; i < endslice; i++)
		{
			if ((image_slices[i - 1]).return_work()[area - 1] -
					(image_slices[i]).return_work()[area - 1] >
				jumpabs)
				return false;
			if ((image_slices[i]).return_work()[area - 1] -
					(image_slices[i - 1]).return_work()[area - 1] >
				jumpabs)
				return false;
		}
	}
	return ok;
}

unsigned SlicesHandler::GetNumberOfUndoSteps()
{
	return this->undoQueue.return_nrundomax();
}

void SlicesHandler::SetNumberOfUndoSteps(unsigned n)
{
	this->undoQueue.set_nrundo(n);
}

unsigned SlicesHandler::GetNumberOfUndoArrays()
{
	return this->undoQueue.return_nrundoarraysmax();
}

void SlicesHandler::SetNumberOfUndoArrays(unsigned n)
{
	this->undoQueue.set_nrundoarraysmax(n);
}

std::vector<float> SlicesHandler::GetBoundingBox()
{
	std::vector<float> bbox;

	unsigned int dimensionX = return_width();
	unsigned int dimensionY = return_height();
	unsigned int dimensionZ = return_nrslices();

	if (dimensionX * dimensionY * dimensionZ == 0)
		return bbox;

	float offset[3], dc[6];
	get_displacement(offset);
	get_direction_cosines(dc);

	Vec3 origin(offset[0], offset[1], offset[2]);

	Vec3 d1(dc[0], dc[1], dc[2]);
	Vec3 d2(dc[3], dc[4], dc[5]);
	Vec3 d3(d1 ^ d2);

	bbox.resize(6);
	bbox[0] =
		(float)std::min<double>(origin[0], origin[0] + d1[0] * dimensionX * dx);
	bbox[1] =
		(float)std::max<double>(origin[0], origin[0] + d1[0] * dimensionX * dx);
	bbox[2] =
		(float)std::min<double>(origin[1], origin[1] + d2[1] * dimensionY * dy);
	bbox[3] =
		(float)std::max<double>(origin[1], origin[1] + d2[1] * dimensionY * dy);
	bbox[4] = (float)std::min<double>(
		origin[2], origin[2] + d3[2] * dimensionZ * thickness);
	bbox[5] = (float)std::max<double>(
		origin[2], origin[2] + d3[2] * dimensionZ * thickness);

	return bbox;
}

void SlicesHandler::GetITKImage(itk::Image<float, 3>* inputim)
{
	GetITKImage(inputim, startslice, endslice);
}

void SlicesHandler::GetITKImageGM(itk::Image<float, 3>* inputim)
{
	GetITKImageGM(inputim, startslice, endslice);
}

void SlicesHandler::GetITKImageFB(itk::Image<float, 3>* inputim)
{
	GetITKImageFB(inputim, startslice, endslice);
}

void SlicesHandler::ModifyWork(itk::Image<unsigned int, 3>* output)
{
	ModifyWork(output, startslice, endslice);
}

void SlicesHandler::ModifyWorkFloat(itk::Image<float, 3>* output)
{
	ModifyWorkFloat(output, startslice, endslice);
}

void SlicesHandler::GetITKImage(itk::Image<float, 3>* inputim, int begin,
								int end)
{
	const SlicesHandler* self = this; // for const slices
	auto slices = self->get_bmp();
	float spacing[3] = {dx, dy, thickness};
	ImageToITK::copy(slices.data(), width, height, (unsigned)begin,
					 (unsigned)(end - begin), spacing, transform, inputim);
}

void SlicesHandler::GetITKImageGM(itk::Image<float, 3>* inputim, int begin,
								  int end)
{
	float spacing[3] = {dx, dy, thickness};
	ImageToITK::setup<float>(width, height, begin, end, spacing, transform,
							 inputim);

	itk::Image<float, 3>::IndexType pi;
	auto size = inputim->GetLargestPossibleRegion().GetSize();

	for (unsigned z = 0; z < size[2]; z++)
	{
		pi.SetElement(2, z);
		std::vector<float> fieldin((int)size[0] * (int)size[1]);
		float* field = &fieldin[0];
		std::vector<float> fieldwork((int)size[0] * (int)size[1]);
		float* fieldw = &fieldwork[0];
		copyfrombmp(begin + z, field);
		copyfromwork(begin + z, fieldw);
		for (unsigned y = begin; y < size[1]; y++)
		{
			pi.SetElement(1, y);
			for (unsigned x = 0; x < size[0]; x++)
			{
				pi.SetElement(0, x);
				if (fieldw[x + y * size[0]] == 0)
					field[x + y * size[0]] = 0;
				inputim->SetPixel(pi, field[x + y * size[0]]);
			}
		}
	}
	inputim->Update();
}

void SlicesHandler::GetITKImageFB(itk::Image<float, 3>* inputim, int begin,
								  int end)
{
	float spacing[3] = {dx, dy, thickness};
	ImageToITK::setup<float>(width, height, begin, end, spacing, transform,
							 inputim);

	itk::Image<float, 3>::IndexType pi;
	auto size = inputim->GetLargestPossibleRegion().GetSize();

	for (unsigned z = 0; z < size[2]; z++)
	{
		pi.SetElement(2, z);
		std::vector<float> fieldin((int)size[0] * (int)size[1]);
		float* field = &fieldin[0];
		copyfromwork(begin + z,
					 field); //&([z*(unsigned long long)return_area()])
		for (unsigned y = begin; y < size[1]; y++)
		{
			pi.SetElement(1, y);
			for (unsigned x = 0; x < size[0]; x++)
			{
				pi.SetElement(0, x);
				inputim->SetPixel(pi, field[x + y * size[0]]);
			}
		}
	}
	inputim->Update();
}

void SlicesHandler::ModifyWork(itk::Image<unsigned int, 3>* output, int begin,
							   int end)
{
	typedef itk::Image<unsigned int, 3> TInput;
	TInput::SizeType size;
	size[0] = width;	   // size along X
	size[1] = height;	  // size along Y
	size[2] = end - begin; // size along Z

	TInput::IndexType pi;

	for (unsigned z = 0; z < size[2]; z++)
	{
		pi.SetElement(2, z);
		std::vector<float> fieldin((int)size[0] * (int)size[1]);
		float* field = &fieldin[0];
		for (unsigned y = begin; y < size[1]; y++)
		{
			pi.SetElement(1, y);
			for (unsigned x = 0; x < size[0]; x++)
			{
				pi.SetElement(0, x);
				field[x + y * size[0]] = output->GetPixel(pi);
			}
		}
		copy2work(begin + z, field, 1);
	}
}

void SlicesHandler::ModifyWorkFloat(itk::Image<float, 3>* output, int begin,
									int end)
{
	typedef itk::Image<unsigned int, 3> TInput;
	TInput::SizeType size;
	size[0] = width;	   // size along X
	size[1] = height;	  // size along Y
	size[2] = end - begin; // size along Z
	TInput::IndexType pi;

	for (unsigned z = 0; z < output->GetLargestPossibleRegion().GetSize(2); z++)
	{
		pi.SetElement(2, z);
		std::vector<float> fieldin((int)size[0] * (int)size[1]);
		float* field = &fieldin[0];
		for (unsigned y = 0; y < output->GetLargestPossibleRegion().GetSize(1);
			 y++)
		{
			pi.SetElement(1, y);
			for (unsigned x = 0;
				 x < output->GetLargestPossibleRegion().GetSize(0); x++)
			{
				pi.SetElement(0, x);
				field[x + y * size[0]] = output->GetPixel(pi);
			}
		}
		copy2work(begin + z, field, 1);
	}
}

void SlicesHandler::GetSeed(itk::Image<float, 3>::IndexType* seed)
{
	typedef itk::Image<float, 3> TInput;
	TInput::IndexType pi;

	for (unsigned z = 0; z < (int)(endslice - startslice); z++)
	{
		pi.SetElement(2, z);
		std::vector<float> fieldin((int)return_width() * (int)return_height());
		float* field = &fieldin[0];
		copyfromwork(z, field); //&([z*(unsigned long long)return_area()])
		for (unsigned y = 0; y < return_height(); y++)
		{
			pi.SetElement(1, y);
			for (unsigned x = 0; x < return_width(); x++)
			{
				pi.SetElement(0, x);
				if (abs(field[x + y * return_width()]) > 4500)
				{
					*seed = pi;
				}
			}
		}
	}
}

template<typename T>
typename itk::SliceContiguousImage<T>::Pointer
	_GetITKView(std::vector<T*>& slices, size_t dims[3], double spacing[3])
{
	typedef itk::SliceContiguousImage<T> SliceContiguousImageType;

	auto image = SliceContiguousImageType::New();
	image->SetSpacing(spacing);
	// \bug Transform (rotation/offset) not set

	typename SliceContiguousImageType::IndexType start;
	start.Fill(0);

	typename SliceContiguousImageType::SizeType size;
	size[0] = dims[0];
	size[1] = dims[1];
	size[2] = dims[2];

	typename SliceContiguousImageType::RegionType region(start, size);
	image->SetRegions(region);
	image->Allocate();

	// Set slice pointers
	auto container = SliceContiguousImageType::PixelContainer::New();
	container->SetImportPointersForSlices(slices, size[0] * size[1], false);
	image->SetPixelContainer(container);

	return image;
}

itk::SliceContiguousImage<float>::Pointer
	SlicesHandler::GetImage(eImageType type, bool active_slices)
{
	size_t dims[3] = {
		width, height,
		static_cast<size_t>(active_slices ? endslice - startslice : nrslices)};
	double spacing[3] = {dx, dy, thickness};

	auto all_slices = (type == eImageType::kSource) ? get_bmp() : get_work();
	if (!active_slices)
	{
		return _GetITKView(all_slices, dims, spacing);
	}

	std::vector<float*> slices;
	for (unsigned i = startslice; i < endslice; ++i)
	{
		slices.push_back(all_slices[i]);
	}
	return _GetITKView(slices, dims, spacing);
}

itk::SliceContiguousImage<unsigned short>::Pointer
	SlicesHandler::GetTissues(bool active_slices)
{
	size_t dims[3] = {
		width, height,
		static_cast<size_t>(active_slices ? endslice - startslice : nrslices)};
	double spacing[3] = {dx, dy, thickness};

	auto all_slices = get_tissues(active_tissuelayer);
	if (!active_slices)
	{
		return _GetITKView(all_slices, dims, spacing);
	}

	std::vector<tissues_size_t*> slices;
	for (unsigned i = startslice; i < endslice; ++i)
	{
		slices.push_back(all_slices[i]);
	}
	return _GetITKView(slices, dims, spacing);
}
