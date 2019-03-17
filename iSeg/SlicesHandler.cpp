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
#include "StdStringToQString.h"
#include "bmp_read_1.h"
#include "config.h"

#include "AvwReader.h"
#include "ChannelExtractor.h"
#include "DicomReader.h"
#include "TestingMacros.h"
#include "TissueHierarchy.h"
#include "TissueInfos.h"
#include "XdmfImageMerger.h"
#include "XdmfImageReader.h"
#include "XdmfImageWriter.h"
#include "vtkEdgeCollapse.h"
#include "vtkGenericDataSetWriter.h"
#include "vtkImageExtractCompatibleMesher.h"

#include "Data/ItkProgressObserver.h"
#include "Data/SliceHandlerItkWrapper.h"
#include "Data/Transform.h"

#include "Core/ColorLookupTable.h"
#include "Core/ConnectedShapeBasedInterpolation.h"
#include "Core/ExpectationMaximization.h"
#include "Core/HDF5Writer.h"
#include "Core/ImageForestingTransform.h"
#include "Core/ImageReader.h"
#include "Core/ImageWriter.h"
#include "Core/KMeans.h"
#include "Core/MatlabExport.h"
#include "Core/Morpho.h"
#include "Core/MultidimensionalGamma.h"
#include "Core/Outline.h"
#include "Core/ProjectVersion.h"
#include "Core/RTDoseIODModule.h"
#include "Core/RTDoseReader.h"
#include "Core/RTDoseWriter.h"
#include "Core/SliceProvider.h"
#include "Core/SmoothSteps.h"
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
#include <vtkSTLReader.h>
#include <vtkPolyDataReader.h>
#include <vtkStringArray.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkWindowedSincPolyDataFilter.h>

#include <itkConnectedComponentImageFilter.h>

#include <boost/format.hpp>

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
	_activeslice = 0;
	_thickness = _dx = _dy = 1.0f;
	_transform.setIdentity();

	_width = _height = 0;
	_startslice = 0;
	_endslice = _nrslices = 0;

	_active_tissuelayer = 0;
	_color_lookup_table = nullptr;
	_tissue_hierachy = new TissueHiearchy;
	_overlay = nullptr;

	_loaded = false;
	_uelem = nullptr;
	_undo3D = true;
	_hdf5_compression = 1;
	_contiguous_memory_io = false; // Default: slice-by-slice
}

SlicesHandler::~SlicesHandler() { delete _tissue_hierachy; }

float SlicesHandler::get_work_pt(Point p, unsigned short slicenr)
{
	return _image_slices[slicenr].work_pt(p);
}

void SlicesHandler::set_work_pt(Point p, unsigned short slicenr, float f)
{
	_image_slices[slicenr].set_work_pt(p, f);
}

float SlicesHandler::get_bmp_pt(Point p, unsigned short slicenr)
{
	return _image_slices[slicenr].bmp_pt(p);
}

void SlicesHandler::set_bmp_pt(Point p, unsigned short slicenr, float f)
{
	_image_slices[slicenr].set_bmp_pt(p, f);
}

tissues_size_t SlicesHandler::get_tissue_pt(Point p, unsigned short slicenr)
{
	return _image_slices[slicenr].tissues_pt(_active_tissuelayer, p);
}

void SlicesHandler::set_tissue_pt(Point p, unsigned short slicenr, tissues_size_t f)
{
	_image_slices[slicenr].set_tissue_pt(_active_tissuelayer, p, f);
}

std::vector<const float*> SlicesHandler::source_slices() const
{
	std::vector<const float*> ptrs(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		ptrs[i] = _image_slices[i].return_bmp();
	}
	return ptrs;
}

std::vector<float*> SlicesHandler::source_slices()
{
	std::vector<float*> ptrs(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		ptrs[i] = _image_slices[i].return_bmp();
	}
	return ptrs;
}

std::vector<const float*> SlicesHandler::target_slices() const
{
	std::vector<const float*> ptrs(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		ptrs[i] = _image_slices[i].return_work();
	}
	return ptrs;
}

std::vector<float*> SlicesHandler::target_slices()
{
	std::vector<float*> ptrs(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		ptrs[i] = _image_slices[i].return_work();
	}
	return ptrs;
}

std::vector<const tissues_size_t*> SlicesHandler::tissue_slices(tissuelayers_size_t layeridx) const
{
	std::vector<const tissues_size_t*> ptrs(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		ptrs[i] = _image_slices[i].return_tissues(layeridx);
	}
	return ptrs;
}

std::vector<tissues_size_t*> SlicesHandler::tissue_slices(tissuelayers_size_t layeridx)
{
	std::vector<tissues_size_t*> ptrs(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		ptrs[i] = _image_slices[i].return_tissues(layeridx);
	}
	return ptrs;
}

std::vector<std::string> SlicesHandler::tissue_names() const
{
	std::vector<std::string> names(TissueInfos::GetTissueCount() + 1);
	names[0] = "Background";
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); i++)
	{
		names.at(i) = TissueInfos::GetTissueName(i);
	}
	return names;
}

std::vector<bool> SlicesHandler::tissue_locks() const
{
	std::vector<bool> locks(TissueInfos::GetTissueCount() + 1, false);
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); i++)
	{
		locks.at(i) = TissueInfos::GetTissueLocked(i);
	}
	return locks;
}

float* SlicesHandler::return_bmp(unsigned short slicenr1)
{
	return _image_slices[slicenr1].return_bmp();
}

float* SlicesHandler::return_work(unsigned short slicenr1)
{
	return _image_slices[slicenr1].return_work();
}

tissues_size_t* SlicesHandler::return_tissues(tissuelayers_size_t layeridx,
		unsigned short slicenr1)
{
	return _image_slices[slicenr1].return_tissues(layeridx);
}

float* SlicesHandler::return_overlay() { return _overlay; }

int SlicesHandler::LoadDIBitmap(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);
	_activeslice = 0;
	_active_tissuelayer = 0;
	_startslice = 0;
	_endslice = _nrslices = (unsigned short)filenames.size();
	_os.set_sizenr(_nrslices);
	_image_slices.resize(_nrslices);

	int j = 0;
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		j += _image_slices[i].LoadDIBitmap(filenames[i]);
	}

	_width = _image_slices[0].return_width();
	_height = _image_slices[0].return_height();
	_area = _height * (unsigned int)_width;

	new_overlay();

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		if (_width != _image_slices[i].return_width() ||
				_height != _image_slices[i].return_height())
			j = _nrslices + 1;
	}

	if (j == _nrslices)
	{
		// Ranges
		Pair dummy;
		_slice_ranges.resize(_nrslices);
		_slice_bmpranges.resize(_nrslices);
		compute_range_mode1(&dummy);
		compute_bmprange_mode1(&dummy);

		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, _nrslices);
		return 0;
	}
}

int SlicesHandler::LoadDIBitmap(std::vector<const char*> filenames, Point p,
		unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	_activeslice = 0;
	_active_tissuelayer = 0;
	_nrslices = (unsigned short)filenames.size();
	_os.set_sizenr(_nrslices);

	_image_slices.resize(_nrslices);
	int j = 0;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		j += _image_slices[i].LoadDIBitmap(filenames[i], p, dx, dy);
	}

	if (j == _nrslices)
	{
		// Ranges
		Pair dummy;
		_slice_ranges.resize(_nrslices);
		_slice_bmpranges.resize(_nrslices);
		compute_range_mode1(&dummy);
		compute_bmprange_mode1(&dummy);

		_loaded = true;
		_width = dx;
		_height = dy;
		_area = _height * (unsigned int)_width;

		new_overlay();
		return 1;
	}
	else
	{
		newbmp(_width, _height, _nrslices);
		return 0;
	}
}

void SlicesHandler::set_rgb_factors(int redFactor, int greenFactor, int blueFactor)
{
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		_image_slices[i].SetConverterFactors(redFactor, greenFactor, blueFactor);
	}
}

// TODO BL this function has a terrible impl, e.g. using member variables rgb, width/height, etc.
int SlicesHandler::LoadPng(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr); // BL: here we could quantize colors instead and build color

	_activeslice = 0;
	_active_tissuelayer = 0;
	_startslice = 0;
	_endslice = _nrslices = (unsigned short)filenames.size();
	_os.set_sizenr(_nrslices);
	_image_slices.resize(_nrslices);

	int j = 0;
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		j += _image_slices[i].LoadPNGBitmap(filenames[i]);
	}

	_width = _image_slices[0].return_width();
	_height = _image_slices[0].return_height();
	_area = _height * (unsigned int)_width;

	new_overlay();

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		if (_width != _image_slices[i].return_width() ||
				_height != _image_slices[i].return_height())
			j = _nrslices + 1;
	}

	// Ranges
	Pair dummy;
	_slice_ranges.resize(_nrslices);
	_slice_bmpranges.resize(_nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == _nrslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, _nrslices);
		return 0;
	}
}

int SlicesHandler::LoadPng(std::vector<const char*> filenames, Point p,
		unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	_activeslice = 0;
	_active_tissuelayer = 0;
	_nrslices = (unsigned short)filenames.size();
	_os.set_sizenr(_nrslices);

	_image_slices.resize(_nrslices);
	int j = 0;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		j += _image_slices[i].LoadDIBitmap(filenames[i], p, dx, dy);
	}

	_width = dx;
	_height = dy;
	_area = _height * (unsigned int)_width;

	new_overlay();

	// Ranges
	Pair dummy;
	_slice_ranges.resize(_nrslices);
	_slice_bmpranges.resize(_nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == _nrslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, _nrslices);
		return 0;
	}
}

int SlicesHandler::LoadDIJpg(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);

	_activeslice = 0;
	_active_tissuelayer = 0;
	_startslice = 0;
	_endslice = _nrslices = (unsigned short)filenames.size();
	_os.set_sizenr(_nrslices);

	_image_slices.resize(_nrslices);
	int j = 0;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		j += _image_slices[i].LoadDIBitmap(filenames[i]);
	}

	_width = _image_slices[0].return_width();
	_height = _image_slices[0].return_height();
	_area = _height * (unsigned int)_width;

	new_overlay();

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		if (_width != _image_slices[i].return_width() ||
				_height != _image_slices[i].return_height())
			j = _nrslices + 1;
	}

	// Ranges
	Pair dummy;
	_slice_ranges.resize(_nrslices);
	_slice_bmpranges.resize(_nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == _nrslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, _nrslices);
		return 0;
	}
}

int SlicesHandler::LoadDIJpg(std::vector<const char*> filenames, Point p,
		unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	_activeslice = 0;
	_active_tissuelayer = 0;
	_nrslices = (unsigned short)filenames.size();
	_os.set_sizenr(_nrslices);

	_image_slices.resize(_nrslices);
	int j = 0;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		j += _image_slices[i].LoadDIBitmap(filenames[i], p, dx, dy);
	}

	_width = dx;
	_height = dy;
	_area = _height * (unsigned int)_width;

	new_overlay();

	// Ranges
	Pair dummy;
	_slice_ranges.resize(_nrslices);
	_slice_bmpranges.resize(_nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == _nrslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, _nrslices);
		return 0;
	}
}

int SlicesHandler::ReadRaw(const char* filename, short unsigned w,
		short unsigned h, unsigned bitdepth,
		unsigned short slicenr, unsigned short nrofslices)
{
	UpdateColorLookupTable(nullptr);

	_activeslice = 0;
	_active_tissuelayer = 0;
	_width = w;
	_height = h;
	_area = _height * (unsigned int)_width;
	_startslice = 0;
	_endslice = _nrslices = nrofslices;
	_os.set_sizenr(_nrslices);

	_image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += _image_slices[i].ReadRaw(filename, w, h, bitdepth, slicenr + i);

	new_overlay();

	// Ranges
	Pair dummy;
	_slice_ranges.resize(nrofslices);
	_slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		ISEG_WARNING_MSG("loading failed in 'ReadRaw'");
		newbmp(_width, _height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawOverlay(const char* filename, unsigned bitdepth,
		unsigned short slicenr)
{
	FILE* fp;						 /* Open file pointer */
	int bitsize = _area; /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == nullptr)
		return 0;

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1)
	{
		unsigned char* bits_tmp;
		if ((bits_tmp = (unsigned char*)malloc(bitsize)) == nullptr)
		{
			ISEG_ERROR_MSG("allocation failed in 'ReadRaw'");
			fclose(fp);
			return 0;
		}

		// int result = fseek(fp, (size_t)(bitsize)*slicenr, SEEK_SET);
		// const fpos_t pos = (fpos_t)(bitsize)*slicenr;
		// int result = fsetpos(fp, &pos);

		// Check data size
#ifdef _MSC_VER
		int result = _fseeki64(fp, (__int64)(bitsize)*_nrslices - 1, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*_nrslices - 1, SEEK_SET);
#endif
		if (result)
		{
			ISEG_ERROR_MSG("bmphandler::ReadRawOverlay() : file operation failed");
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		// Move to slice
#ifdef _MSC_VER
		_fseeki64(fp, (__int64)(bitsize)*slicenr, SEEK_SET);
#else
		fseek(fp, (size_t)(bitsize)*slicenr, SEEK_SET);
#endif

		if (fread(bits_tmp, 1, bitsize, fp) < _area)
		{
			ISEG_ERROR_MSG("bmphandler::ReadRawOverlay() : file operation failed");
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (int i = 0; i < bitsize; i++)
		{
			_overlay[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2)
	{
		unsigned short* bits_tmp;

		if ((bits_tmp = (unsigned short*)malloc(bitsize * 2)) == nullptr)
		{
			ISEG_ERROR_MSG("allocation failed in 'ReadRawOverlay'");
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
			ISEG_ERROR_MSG("bmphandler::ReadRawOverlay() : file operation failed");
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, (size_t)(bitsize)*2, fp) < _area * 2)
		{
			ISEG_ERROR_MSG("bmphandler::ReadRawOverlay() : file operation failed");
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (int i = 0; i < bitsize; i++)
		{
			_overlay[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else
	{
		ISEG_ERROR_MSG("unsupported depth in 'ReadRawOverlay'");
		fclose(fp);
		return 0;
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
		std::vector<float*> slices(_nrslices);
		for (unsigned i = 0; i < _nrslices; i++)
		{
			slices[i] = _image_slices[i].return_bmp();
		}
		ImageReader::getVolume(filename, slices.data(), _nrslices, _width, _height);
		_thickness = spacing1[2];
		_dx = spacing1[0];
		_dy = spacing1[1];
		_transform = transform1;
		_loaded = true;

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
		return ImageReader::getVolume(filename, &_overlay, slicenr, 1, _width,
				_height);
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
		ISEG_ERROR_MSG("'ReadSizeData' failed");
		return 0;
	}

	// Reallocate slices
	// taken from LoadProject()
	this->_activeslice = 0;
	this->_width = dims[0];
	this->_height = dims[1];
	this->_area = _height * (unsigned int)_width;
	this->_nrslices = dims[2];
	this->_startslice = 0;
	this->_endslice = _nrslices;
	this->set_pixelsize(spacing[0], spacing[1]);
	this->_thickness = spacing[2];

	// WARNING this might neglect the third column of the "rotation" matrix (e.g. reflections)
	_transform.setTransform(origin, dc);

	this->_image_slices.resize(_nrslices);
	this->_os.set_sizenr(_nrslices);
	this->set_slicethickness(_thickness);
	this->_loaded = true;
	for (unsigned short j = 0; j < _nrslices; j++)
	{
		this->_image_slices[j].newbmp(_width, _height);
	}

	new_overlay();

	// Pass slice pointers to reader
	std::vector<float*> bmpslices(_nrslices);
	for (unsigned i = 0; i < _nrslices; i++)
	{
		bmpslices[i] = this->_image_slices[i].return_bmp();
	}

	// Read pixel data
	bool res = RTDoseReader::ReadPixelData(filename, bmpslices.data());
	if (res)
	{
		_loaded = true;
		bmp2workall();
	}
	else
	{
		ISEG_ERROR_MSG("ReadPixelData() failed");
		newbmp(_width, _height, _nrslices);
	}

	// Ranges
	Pair dummy;
	_slice_ranges.resize(_nrslices);
	_slice_bmpranges.resize(_nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	set_active_tissuelayer(0);

	return res;
}

bool SlicesHandler::LoadSurface(const std::string& filename_in, bool overwrite_working, bool intersect)
{
	unsigned dims[3] = {_width, _height, _nrslices};
	auto slices = target_slices();

	if (overwrite_working)
	{
		clear_work();
	}

	std::string filename = filename_in;
	std::transform(filename_in.begin(), filename_in.end(), filename.begin(), ::tolower);

	vtkSmartPointer<vtkAlgorithm> reader;
	if (filename.substr(filename.find_last_of(".") + 1) == "stl")
	{
		auto stl_reader = vtkSmartPointer<vtkSTLReader>::New();
		stl_reader->SetFileName(filename.c_str());
		reader = stl_reader;
	}
	else if (filename.substr(filename.find_last_of(".") + 1) == "vtk")
	{
		auto vtk_reader = vtkSmartPointer<vtkPolyDataReader>::New();
		vtk_reader->SetFileName(filename.c_str());
		reader = vtk_reader;
	}
	
	reader->Update();
	auto surface = vtkPolyData::SafeDownCast(reader->GetOutputDataObject(0));

	VoxelSurface voxeler;
	VoxelSurface::eSurfaceImageOverlap ret;
	if (intersect)
	{
		ret = voxeler.Intersect(surface, slices, dims, spacing(), _transform, _startslice, _endslice);
	}
	else
	{
		ret = voxeler.Voxelize(surface, slices, dims, spacing(), _transform, _startslice, _endslice);
	}
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

	if ((w != _width) || (h != _height) || (_nrslices != nrofslices))
	{
		ISEG_ERROR_MSG("LoadAllHDF() : inconsistent dimensions");
		return 0;
	}

	for (int r = 0; r < 3; r++)
	{
		offset[r] = tr_1d[r * 4 + 3];
	}
	arrayNames = reader.GetArrayNames();

	const int NPA = arrayNames.size();

	// read colors if any
	UpdateColorLookupTable(reader.ReadColorLookup());

	std::vector<float*> bmpslices(_nrslices);
	std::vector<float*> workslices(_nrslices);
	std::vector<tissues_size_t*> tissueslices(_nrslices);
	for (unsigned i = 0; i < _nrslices; i++)
	{
		bmpslices[i] = this->_image_slices[i].return_bmp();
		workslices[i] = this->_image_slices[i].return_work();
		tissueslices[i] = this->_image_slices[i].return_tissues(0); // TODO
	}

	reader.SetImageSlices(bmpslices.data());
	reader.SetWorkSlices(workslices.data());
	reader.SetTissueSlices(tissueslices.data());

	return reader.Read();
}

void SlicesHandler::UpdateColorLookupTable(
		std::shared_ptr<ColorLookupTable> new_lut /*= nullptr*/)
{
	_color_lookup_table = new_lut;

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
	if ((w != _width) || (h != _height) || (_nrslices != nrofslices))
	{
		ISEG_ERROR_MSG("inconsistent dimensions in LoadAllXdmf");
		return 0;
	}
	auto pixsize = reader.GetPixelSize();
	auto transform = reader.GetImageTransform();
	float origin[3];
	transform.getOffset(origin);
	arrayNames = reader.GetArrayNames();

	const int NPA = arrayNames.size();

	std::vector<float*> bmpslices(_nrslices);
	std::vector<float*> workslices(_nrslices);
	std::vector<tissues_size_t*> tissueslices(_nrslices);

	for (unsigned i = 0; i < _nrslices; i++)
	{
		bmpslices[i] = this->_image_slices[i].return_bmp();
		workslices[i] = this->_image_slices[i].return_work();
		tissueslices[i] = this->_image_slices[i].return_tissues(0); // TODO
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
	float pixsize[3] = {_dx, _dy, _thickness};

	std::vector<float*> bmpslices(_endslice - _startslice);
	std::vector<float*> workslices(_endslice - _startslice);
	std::vector<tissues_size_t*> tissueslices(_endslice - _startslice);
	for (unsigned i = _startslice; i < _endslice; i++)
	{
		bmpslices[i - _startslice] = _image_slices[i].return_bmp();
		workslices[i - _startslice] = _image_slices[i].return_work();
		tissueslices[i - _startslice] =
				_image_slices[i].return_tissues(0); // TODO
	}

	XdmfImageWriter writer;
	writer.SetCopyToContiguousMemory(GetContiguousMemory());
	writer.SetFileName(filename);
	writer.SetImageSlices(bmpslices.data());
	writer.SetWorkSlices(workslices.data());
	writer.SetTissueSlices(tissueslices.data());
	writer.SetNumberOfSlices(_endslice - _startslice);
	writer.SetWidth(_width);
	writer.SetHeight(_height);
	writer.SetPixelSize(pixsize);

	auto active_slices_transform = get_transform_active_slices();

	writer.SetImageTransform(active_slices_transform);
	writer.SetCompression(compression);
	bool ok = writer.Write(naked);
	ok &= writer.WriteColorLookup(_color_lookup_table.get(), naked);
	ok &= TissueInfos::SaveTissuesHDF(filename, _tissue_hierachy->selected_hierarchy(), naked, 0);
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

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(fileInfo.absolutePath());

	HDF5Writer writer;
	QString fname;
	if (naked)
		fname = basename + "." + suffix;
	else
		fname = basename + ".h5";

	if (!writer.open(fname.toStdString(), "append"))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return false;
	}
	writer.compression = compression;

	if (!writer.createGroup(std::string("Markers")))
	{
		ISEG_ERROR_MSG("creating markers section");
		return false;
	}

	int index1[1];
	std::vector<HDF5Writer::size_type> dim2;
	dim2.resize(1);
	dim2[0] = 1;

	index1[0] = (int)version;
	if (!writer.write(index1, dim2, std::string("/Markers/version")))
	{
		ISEG_ERROR_MSG("writing version");
		return false;
	}

	float marker_pos[3];
	std::vector<HDF5Writer::size_type> dim1;
	dim1.resize(1);
	dim1[0] = 3;

	int mark_counter = 0;
	for (unsigned i = _startslice; i < _endslice; i++)
	{
		std::vector<Mark> marks = *(_image_slices[i].return_marks());
		for (auto cur_mark : marks)
		{
			QString mark_name = QString::fromStdString(cur_mark.name);
			if (mark_name.isEmpty())
			{
				mark_name = QString::fromStdString("Mark_" + std::to_string(mark_counter));
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
				ISEG_ERROR_MSG("writing marker_pos");
				return false;
			}
		}
	}

	writer.close();

	QDir::setCurrent(oldcwd.absolutePath());

	return true;
}

int SlicesHandler::SaveMergeAllXdmf(const char* filename,
		std::vector<QString>& mergeImagefilenames,
		unsigned short nrslicesTotal,
		int compression)
{
	float pixsize[3];

	auto active_slices_transform = get_transform_active_slices();

	pixsize[0] = _dx;
	pixsize[1] = _dy;
	pixsize[2] = _thickness;

	std::vector<float*> bmpslices(_endslice - _startslice);
	std::vector<float*> workslices(_endslice - _startslice);
	std::vector<tissues_size_t*> tissueslices(_endslice - _startslice);
	for (unsigned i = _startslice; i < _endslice; i++)
	{
		bmpslices[i - _startslice] = _image_slices[i].return_bmp();
		workslices[i - _startslice] = _image_slices[i].return_work();
		tissueslices[i - _startslice] = _image_slices[i].return_tissues(0);
	}

	XdmfImageMerger merger;
	merger.SetFileName(filename);
	merger.SetImageSlices(bmpslices.data());
	merger.SetWorkSlices(workslices.data());
	merger.SetTissueSlices(tissueslices.data());
	merger.SetNumberOfSlices(_endslice - _startslice);
	merger.SetTotalNumberOfSlices(nrslicesTotal);
	merger.SetWidth(_width);
	merger.SetHeight(_height);
	merger.SetPixelSize(pixsize);
	merger.SetImageTransform(active_slices_transform);
	merger.SetCompression(compression);
	merger.SetMergeFileNames(mergeImagefilenames);
	if (merger.Write())
	{
		bool naked = false;
		TissueInfos::SaveTissuesHDF(filename, _tissue_hierachy->selected_hierarchy(), naked, 0);
		// \todo save markers also ?
		if (auto lut = GetColorLookupTable())
		{
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
	_thickness = thickness1;
	_dx = dx1;
	_dy = dy1;
	_activeslice = 0;
	_active_tissuelayer = 0;
	_width = w;
	_height = h;
	_area = _height * (unsigned int)_width;
	_startslice = 0;
	_endslice = _nrslices = nrofslices;
	_os.set_sizenr(_nrslices);

	_image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += _image_slices[i].ReadAvw(filename, i);

	new_overlay();

	// Ranges
	Pair dummy;
	_slice_ranges.resize(nrofslices);
	_slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRaw(const char* filename, short unsigned w,
		short unsigned h, unsigned bitdepth,
		unsigned short slicenr, unsigned short nrofslices,
		Point p, unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	_activeslice = 0;
	_active_tissuelayer = 0;
	_width = dx;
	_height = dy;
	_area = _height * (unsigned int)_width;
	_startslice = 0;
	_endslice = _nrslices = nrofslices;
	_os.set_sizenr(_nrslices);

	_image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += _image_slices[i]
						 .ReadRaw(filename, w, h, bitdepth, slicenr + i, p, dx, dy);

	new_overlay();

	// Ranges
	Pair dummy;
	_slice_ranges.resize(nrofslices);
	_slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawFloat(const char* filename, short unsigned w,
		short unsigned h, unsigned short slicenr,
		unsigned short nrofslices)
{
	UpdateColorLookupTable(nullptr);

	_activeslice = 0;
	_active_tissuelayer = 0;
	_width = w;
	_height = h;
	_area = _height * (unsigned int)_width;
	_startslice = 0;
	_endslice = _nrslices = nrofslices;
	_os.set_sizenr(_nrslices);

	_image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += _image_slices[i].ReadRawFloat(filename, w, h, slicenr + i);

	new_overlay();

	// Ranges
	Pair dummy;
	_slice_ranges.resize(nrofslices);
	_slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawFloat(const char* filename, short unsigned w,
		short unsigned h, unsigned short slicenr,
		unsigned short nrofslices, Point p,
		unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	_activeslice = 0;
	_active_tissuelayer = 0;
	_width = dx;
	_height = dy;
	_area = _height * (unsigned int)_width;
	_startslice = 0;
	_endslice = _nrslices = nrofslices;
	_os.set_sizenr(_nrslices);

	_image_slices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
	{
		j += _image_slices[i].ReadRawFloat(filename, w, h, slicenr + i, p, dx, dy);
	}

	new_overlay();

	// Ranges
	Pair dummy;
	_slice_ranges.resize(nrofslices);
	_slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	if (j == nrofslices)
	{
		_loaded = true;
		return 1;
	}
	else
	{
		newbmp(_width, _height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReloadDIBitmap(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;

	if (filenames.size() == _nrslices)
	{
		for (unsigned short i = 0; i < _nrslices; i++)
		{
			j += _image_slices[i].ReloadDIBitmap(filenames[i]);
		}

		for (unsigned short i = 0; i < _nrslices; i++)
		{
			if (_width != _image_slices[i].return_width() ||
					_height != _image_slices[i].return_height())
				j = _nrslices + 1;
		}

		if (j == _nrslices)
			return 1;
		else
		{
			newbmp(_width, _height, _nrslices);
			return 0;
		}
	}
	else if (filenames.size() == (_endslice - _startslice))
	{
		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			j += _image_slices[i].ReloadDIBitmap(filenames[i - _startslice]);
		}

		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			if (_width != _image_slices[i].return_width() ||
					_height != _image_slices[i].return_height())
				j = _nrslices + 1;
		}

		if (j == (_endslice - _startslice))
			return 1;
		else
		{
			newbmp(_width, _height, _nrslices);
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

	if (filenames.size() == _nrslices)
	{
		for (unsigned short i = 0; i < _nrslices; i++)
		{
			j += _image_slices[i].ReloadDIBitmap(filenames[i], p);
		}

		if (j == _nrslices)
			return 1;
		else
		{
			newbmp(_width, _height, _nrslices);
			return 0;
		}
	}
	else if (filenames.size() == (_endslice - _startslice))
	{
		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			j += _image_slices[i].ReloadDIBitmap(filenames[i - _startslice], p);
		}

		if (j == (_endslice - _startslice))
			return 1;
		else
		{
			newbmp(_width, _height, _nrslices);
			return 0;
		}
	}
	else
		return 0;
}

int SlicesHandler::ReloadRaw(const char* filename, unsigned bitdepth, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = _startslice; i < _endslice; i++)
		j += _image_slices[i]
						 .ReloadRaw(filename, bitdepth,
								 (unsigned)slicenr + i - _startslice);

	if (j == (_endslice - _startslice))
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
		if ((w != _width) || (h != _height) || (_endslice + slicenr - _startslice > nrofslices))
		{
			return 0;
		}

		std::vector<float*> slices(_endslice - _startslice);
		for (unsigned i = _startslice; i < _endslice; i++)
		{
			slices[i - _startslice] = _image_slices[i].return_bmp();
		}
		ImageReader::getVolume(filename, slices.data(), slicenr, _endslice - _startslice, _width, _height);
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
		ISEG_ERROR_MSG("WriteRTdose() : Write() failed");
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
	if (!writer->Write(filename, rtDose)) 
	{
		ISEG_ERROR_MSG("WriteRTdose() : Write() failed");
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
		ISEG_ERROR_MSG("ReloadRTdose() : ReadSizeData() failed");
		return 0;
	}

	if ((dims[0] != _width) || (dims[1] != _height) ||
			(_endslice - _startslice + slicenr > dims[2]))
		return 0;

	// Pass slice pointers to reader
	std::vector<float*> bmpslices(_endslice - _startslice);
	for (unsigned i = _startslice; i < _endslice; i++)
	{
		bmpslices[i - _startslice] = this->_image_slices[i].return_bmp();
	}

	// Read pixel data
	bool res = RTDoseReader::ReadPixelData(filename, bmpslices.data());
	if (res)
	{
		_loaded = true;
	}
	else
	{
		ISEG_ERROR_MSG("ReloadRTdose() : ReadPixelData() failed");
		newbmp(_width, _height, _nrslices);
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
	if ((w != _width) || (h != _height) || (_endslice > nrofslices))
		return 0;
	if (h != _height)
		return 0;

	int j = 0;
	for (unsigned short i = _startslice; i < _endslice; i++)
		j += _image_slices[i].ReadAvw(filename, i);

	if (j == (_endslice - _startslice))
		return 1;
	return 0;
}

int SlicesHandler::ReloadRaw(const char* filename, short unsigned w,
		short unsigned h, unsigned bitdepth,
		unsigned short slicenr, Point p)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = _startslice; i < _endslice; i++)
		j += _image_slices[i]
						 .ReloadRaw(filename, w, h, bitdepth,
								 (unsigned)slicenr + i - _startslice, p);

	if (j == (_endslice - _startslice))
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
	for (unsigned short i = _startslice; i < _endslice; i++)
		j += _image_slices[i]
						 .ReloadRawFloat(filename, (unsigned)slicenr + i - _startslice);

	if (j == (_endslice - _startslice))
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
	for (unsigned short i = _startslice; i < _endslice; i++)
		j += _image_slices[i]
						 .ReloadRawFloat(filename, w, h,
								 (unsigned)slicenr + i - _startslice, p);

	if (j == (_endslice - _startslice))
		return 1;
	else
		return 0;
}

int SlicesHandler::ReloadRawTissues(const char* filename, unsigned bitdepth,
		unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = _startslice; i < _endslice; i++)
		j += _image_slices[i]
						 .ReloadRawTissues(filename, bitdepth,
								 (unsigned)slicenr + i - _startslice);

	if (j == (_endslice - _startslice))
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
	for (unsigned short i = _startslice; i < _endslice; i++)
		j += _image_slices[i]
						 .ReloadRawTissues(filename, w, h, bitdepth,
								 (unsigned)slicenr + i - _startslice, p);

	if (j == (_endslice - _startslice))
		return 1;
	else
		return 0;
}

FILE* SlicesHandler::SaveHeader(FILE* fp, short unsigned nr_slices_to_write,
		Transform transform_to_write)
{
	fwrite(&nr_slices_to_write, 1, sizeof(unsigned short), fp);
	float thick = -_thickness;
	if (thick > 0)
		thick = -thick;
	fwrite(&thick, 1, sizeof(float), fp);
	fwrite(&_dx, 1, sizeof(float), fp);
	fwrite(&_dy, 1, sizeof(float), fp);
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

	if ((fp = fopen(filename, "wb")) == nullptr)
		return nullptr;

	fp = SaveHeader(fp, _nrslices, _transform);

	for (unsigned short j = 0; j < _nrslices; j++)
	{
		fp = _image_slices[j].save_proj(fp, false);
	}
	fp = (_image_slices[0]).save_stack(fp);

	// SaveAllXdmf uses startslice/endslice to decide what to write - here we want to override that behavior
	unsigned short startslice1 = _startslice;
	unsigned short endslice1 = _endslice;
	_startslice = 0;
	_endslice = _nrslices;
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
			this->_hdf5_compression, false);

	_startslice = startslice1;
	_endslice = endslice1;

	return fp;
}

bool SlicesHandler::SaveCommunicationFile(const char* filename)
{
	unsigned short startslice1 = _startslice;
	unsigned short endslice1 = _endslice;
	_startslice = 0;
	_endslice = _nrslices;

	SaveAllXdmf(
			QFileInfo(filename).dir().absFilePath(filename).toAscii().data(),
			this->_hdf5_compression, true);

	_startslice = startslice1;
	_endslice = endslice1;

	return true;
}

FILE* SlicesHandler::SaveActiveSlices(const char* filename,
		const char* imageFileExtension)
{
	FILE* fp;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return nullptr;

	unsigned short slicecount = _endslice - _startslice;
	Transform transform_corrected = get_transform_active_slices();
	SaveHeader(fp, slicecount, transform_corrected);

	for (unsigned short j = _startslice; j < _endslice; j++)
	{
		fp = _image_slices[j].save_proj(fp, false);
	}
	fp = (_image_slices[0]).save_stack(fp);
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
			this->_hdf5_compression, false);

	return fp;
}

FILE* SlicesHandler::MergeProjects(const char* savefilename, std::vector<QString>& mergeFilenames)
{
	// Get number of slices to total
	unsigned short nrslicesTotal = _nrslices;
	for (unsigned short i = 0; i < mergeFilenames.size(); i++)
	{
		if (!mergeFilenames[i].endsWith(".prj"))
		{
			return nullptr;
		}

		FILE* fpMerge;
		// Add number of slices to total
		unsigned short nrslicesMerge = 0;
		if ((fpMerge = fopen(mergeFilenames[i].toAscii().data(), "rb")) == nullptr)
			return nullptr;
		fread(&nrslicesMerge, sizeof(unsigned short), 1, fpMerge);
		nrslicesTotal += nrslicesMerge;
		fclose(fpMerge);
	}

	// Save merged project
	FILE* fp;
	if ((fp = fopen(savefilename, "wb")) == nullptr)
		return nullptr;

	/// BL TODO what should merged transform be
	fp = SaveHeader(fp, nrslicesTotal, _transform);

	// Save current project slices
	for (unsigned short j = 0; j < _nrslices; j++)
	{
		fp = _image_slices[j].save_proj(fp, false);
	}

	FILE* fpMerge;
	// Save merged project slices
	for (unsigned short i = 0; i < mergeFilenames.size(); i++)
	{
		if ((fpMerge = fopen(mergeFilenames[i].toAscii().data(), "rb")) == nullptr)
		{
			return nullptr;
		}

		// Skip header
		int version = 0;
		int tissuesVersion = 0;
		SlicesHandler dummy_SlicesHandler;
		dummy_SlicesHandler.LoadHeader(fpMerge, tissuesVersion, version);

		unsigned short mergeNrslices = dummy_SlicesHandler.num_slices();
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

	fp = (_image_slices[0]).save_stack(fp);

	unsigned short startslice1 = _startslice;
	unsigned short endslice1 = _endslice;
	_startslice = 0;
	_endslice = _nrslices;
	QString imageFileExtension = "xmf";
	unsigned char length1 = 0;
	while (imageFileExtension[length1] != '\0')
		length1++;
	length1++;
	fwrite(&length1, 1, sizeof(unsigned char), fp);
	fwrite(imageFileExtension, length1, sizeof(char), fp);

	QString imageFileName = QString(savefilename);
	int afterDot = imageFileName.lastIndexOf('.') + 1;
	imageFileName = imageFileName.remove(afterDot, imageFileName.length() - afterDot) + imageFileExtension;

	auto imageFilePath = QFileInfo(savefilename).dir().absFilePath(imageFileName).toStdString();
	if (!SaveMergeAllXdmf(imageFilePath.c_str(), mergeFilenames, nrslicesTotal, this->_hdf5_compression))
	{
		return nullptr;
	}

	_startslice = startslice1;
	_endslice = endslice1;
	return fp;
}

void SlicesHandler::LoadHeader(FILE* fp, int& tissuesVersion, int& version)
{
	_activeslice = 0;

	fread(&_nrslices, sizeof(unsigned short), 1, fp);
	_startslice = 0;
	_endslice = _nrslices;
	fread(&_thickness, sizeof(float), 1, fp);

	//    set_slicethickness(thickness);
	fread(&_dx, sizeof(float), 1, fp);
	fread(&_dy, sizeof(float), 1, fp);
	set_pixelsize(_dx, _dy);

	version = 0;
	tissuesVersion = 0;
	if (_thickness < 0)
	{
		_thickness = -_thickness;
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
		_transform.setTransform(_displacement, _directionCosines);
	}
	else
	{
		for (int r = 0; r < 4; r++)
		{
			float* transform_r = _transform[r];
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

	if ((fp = fopen(filename, "rb")) == nullptr)
		return nullptr;

	int version = 0;
	LoadHeader(fp, tissuesVersion, version);

	_image_slices.resize(_nrslices);

	_os.set_sizenr(_nrslices);

	for (unsigned short j = 0; j < _nrslices; ++j)
	{
		// skip initializing because we load real data into the arrays below
		fp = _image_slices[j].load_proj(fp, tissuesVersion, version <= 1, false);
	}

	set_slicethickness(_thickness);

	fp = (_image_slices[0]).load_stack(fp);

	_width = (_image_slices[0]).return_width();
	_height = (_image_slices[0]).return_height();
	_area = _height * (unsigned int)_width;

	new_overlay();

	if (version > 1)
	{
		QString imageFileName;
		if (version > 2)
		{
			// Only load image file name extension
			char imageFileExt[10];
			unsigned char length1;
			fread(&length1, sizeof(unsigned char), 1, fp);
			fread(imageFileExt, sizeof(char), length1, fp);
			imageFileName = QString(filename);
			int afterDot = imageFileName.lastIndexOf('.') + 1;
			imageFileName = imageFileName.remove(afterDot, imageFileName.length() - afterDot) + QString(imageFileExt);
		}
		else
		{
			// Load full image file name
			char filenameimage[200];
			unsigned char length1;
			fread(&length1, sizeof(unsigned char), 1, fp);
			fread(filenameimage, sizeof(char), length1, fp);
			imageFileName = QString(filenameimage);
		}

		if (imageFileName.endsWith(".xmf", Qt::CaseInsensitive))
		{
			LoadAllXdmf(QFileInfo(filename).dir().absFilePath(imageFileName).toAscii().data());
		}
		else
		{
			ISEG_ERROR_MSG("unsupported format...");
		}
	}

	// Ranges
	Pair dummy;
	_slice_ranges.resize(_nrslices);
	_slice_bmpranges.resize(_nrslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	_loaded = true;

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

	const int NPA = arrayNames.size();

	// taken from LoadProject()
	_activeslice = 0;
	this->_width = w;
	this->_height = h;
	this->_area = _height * (unsigned int)_width;
	this->_nrslices = nrofslices;
	this->_startslice = 0;
	this->_endslice = _nrslices;
	this->set_pixelsize(pixsize[0], pixsize[1]);
	this->_thickness = pixsize[2];

	float* transform_1d = _transform[0];
	std::copy(tr_1d, tr_1d + 16, transform_1d);

	this->_image_slices.resize(_nrslices);
	this->_os.set_sizenr(_nrslices);
	this->set_slicethickness(_thickness);
	this->_loaded = true;
	for (unsigned short j = 0; j < _nrslices; j++)
	{
		this->_image_slices[j].newbmp(w, h);
	}

	new_overlay();

	bool res = LoadAllHDF(filename);

	// Ranges
	Pair dummy;
	_slice_ranges.resize(nrofslices);
	_slice_bmpranges.resize(nrofslices);
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

	bits_tmp = (unsigned char*)malloc(sizeof(unsigned char) * _area);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = _width * (unsigned)_height;

	for (unsigned short j = 0; j < _nrslices; j++)
	{
		if (work)
			p_bits = _image_slices[j].return_work();
		else
			p_bits = _image_slices[j].return_bmp();
		for (unsigned int i = 0; i < _area; i++)
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

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = _width * (unsigned)_height;

	if ((TissueInfos::GetTissueCount() <= 255) &&
			(sizeof(tissues_size_t) > sizeof(unsigned char)))
	{
		unsigned char* ucharBuffer = new unsigned char[bitsize];
		for (unsigned short j = 0; j < _nrslices; j++)
		{
			bits_tmp = _image_slices[j].return_tissues(_active_tissuelayer);
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
		for (unsigned short j = 0; j < _nrslices; j++)
		{
			bits_tmp = _image_slices[j].return_tissues(_active_tissuelayer);
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
	if ((-(dxm + dxp) >= _width) || (-(dym + dyp) >= _height) || (-(dzm + dzp) >= _nrslices))
		return (-1);
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	int l2 = _width + dxm + dxp;

	unsigned newarea =
			(unsigned)(_width + dxm + dxp) * (unsigned)(_height + dym + dyp);
	bits_tmp = (float*)malloc(sizeof(float) * newarea);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
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
	int h1 = _height;
	if (dym < 0)
	{
		h1 += dym;
		posstart1 += _width * unsigned(-dym);
	}
	else
	{
		posstart2 += l2 * unsigned(dym);
	}
	if (dyp < 0)
		h1 += dyp;

	for (unsigned short j = (unsigned short)std::max(0, -dzm);
			 j < _nrslices - (unsigned short)std::max(0, -dzp); j++)
	{
		if (work)
			p_bits = _image_slices[j].return_work();
		else
			p_bits = _image_slices[j].return_bmp();

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
	if ((-(dxm + dxp) >= _width) || (-(dym + dyp) >= _height) || (-(dzm + dzp) >= _nrslices))
		return (-1);
	FILE* fp;
	tissues_size_t* bits_tmp;

	int l2 = _width + dxm + dxp;

	unsigned newarea =
			(unsigned)(_width + dxm + dxp) * (unsigned)(_height + dym + dyp);
	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * newarea);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
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
	int h1 = _height;
	if (dym < 0)
	{
		h1 += dym;
		posstart1 += _width * unsigned(-dym);
	}
	else
	{
		posstart2 += l2 * unsigned(dym);
	}
	if (dyp < 0)
		h1 += dyp;

	for (unsigned short j = (unsigned short)std::max(0, -dzm);
			 j < _nrslices - (unsigned short)std::max(0, -dzp); j++)
	{
		tissues_size_t* p_bits =
				_image_slices[j].return_tissues(_active_tissuelayer);
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
	w = height();
	h = width();
	nrslices = num_slices();
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
	w = width();
	h = num_slices();
	nrslices = height();
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
	w = num_slices();
	h = height();
	nrslices = width();
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

	bits_tmp = (float*)malloc(sizeof(float) * _area);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = _width * (unsigned)_height;

	for (unsigned short j = 0; j < _nrslices; j++)
	{
		if (work)
			p_bits = _image_slices[j].return_work();
		else
			p_bits = _image_slices[j].return_bmp();
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short y = 0; y < _height; y++)
		{
			pos2 = y;
			for (unsigned short x = 0; x < _width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2 += _height;
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
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
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

	unsigned int bitsize = _nrslices * (unsigned)_height;
	bits_tmp = (float*)malloc(sizeof(float) * bitsize);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	for (unsigned short x = 0; x < _width; x++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < _nrslices; z++)
		{
			if (work)
				p_bits = (_image_slices[z]).return_work();
			else
				p_bits = (_image_slices[z]).return_bmp();
			pos2 = z;
			pos1 = x;
			for (unsigned short y = 0; y < _height; y++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1 += _width;
				pos2 += _nrslices;
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
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
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

	unsigned int bitsize = _nrslices * (unsigned)_width;
	bits_tmp = (float*)malloc(sizeof(float) * bitsize);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	for (unsigned short y = 0; y < _height; y++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < _nrslices; z++)
		{
			if (work)
				p_bits = (_image_slices[z]).return_work();
			else
				p_bits = (_image_slices[z]).return_bmp();
			pos2 = z * _width;
			pos1 = y * _width;
			for (unsigned short x = 0; x < _width; x++)
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
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
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

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = _width * (unsigned)_height;

	for (unsigned short j = 0; j < _nrslices; j++)
	{
		bits_tmp = _image_slices[j].return_tissues(_active_tissuelayer);
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

	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * _area);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = _width * (unsigned)_height;

	for (unsigned short j = 0; j < _nrslices; j++)
	{
		p_bits = _image_slices[j].return_tissues(_active_tissuelayer); // TODO
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short y = 0; y < _height; y++)
		{
			pos2 = y;
			for (unsigned short x = 0; x < _width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2 += _height;
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

	unsigned int bitsize = _nrslices * (unsigned)_height;
	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * bitsize);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	for (unsigned short x = 0; x < _width; x++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < _nrslices; z++)
		{
			p_bits =
					(_image_slices[z]).return_tissues(_active_tissuelayer); // TODO
			pos2 = z;
			pos1 = x;
			for (unsigned short y = 0; y < _height; y++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1 += _width;
				pos2 += _nrslices;
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

	unsigned int bitsize = _nrslices * (unsigned)_width;
	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * bitsize);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	for (unsigned short y = 0; y < _height; y++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < _nrslices; z++)
		{
			p_bits =
					(_image_slices[z]).return_tissues(_active_tissuelayer); // TODO
			pos2 = z * _width;
			pos1 = y * _width;
			for (unsigned short x = 0; x < _width; x++)
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

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += _image_slices[i].SaveDIBitmap(name);
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

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += _image_slices[i].SaveWorkBitmap(name);
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

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += _image_slices[i].SaveTissueBitmap(_active_tissuelayer, name);
	}

	if (j == 0)
	{
		return 1;
	}
	else
		return 0;
}

void SlicesHandler::work2bmp() { (_image_slices[_activeslice]).work2bmp(); }

void SlicesHandler::bmp2work() { (_image_slices[_activeslice]).bmp2work(); }

void SlicesHandler::swap_bmpwork()
{
	(_image_slices[_activeslice]).swap_bmpwork();
}

void SlicesHandler::work2bmpall()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].work2bmp();

	return;
}

void SlicesHandler::bmp2workall()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].bmp2work();

	return;
}

void SlicesHandler::work2tissueall()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].work2tissue(_active_tissuelayer);

	return;
}

void SlicesHandler::mergetissues(tissues_size_t tissuetype)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].mergetissue(tissuetype, _active_tissuelayer);
}

void SlicesHandler::tissue2workall()
{
	_image_slices[_activeslice].tissue2work(_active_tissuelayer);
}

void SlicesHandler::tissue2workall3D()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].tissue2work(_active_tissuelayer);
}

void SlicesHandler::swap_bmpworkall()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].swap_bmpwork();
}

void SlicesHandler::add_mark(Point p, unsigned label)
{
	(_image_slices[_activeslice]).add_mark(p, label);
}

void SlicesHandler::add_mark(Point p, unsigned label, std::string str)
{
	(_image_slices[_activeslice]).add_mark(p, label, str);
}

void SlicesHandler::clear_marks() { (_image_slices[_activeslice]).clear_marks(); }

bool SlicesHandler::remove_mark(Point p, unsigned radius)
{
	return (_image_slices[_activeslice]).remove_mark(p, radius);
}

void SlicesHandler::add_mark(Point p, unsigned label, unsigned short slicenr)
{
	if (slicenr < _nrslices && slicenr >= 0)
		(_image_slices[slicenr]).add_mark(p, label);
}

void SlicesHandler::add_mark(Point p, unsigned label, std::string str,
		unsigned short slicenr)
{
	if (slicenr < _nrslices && slicenr >= 0)
		(_image_slices[slicenr]).add_mark(p, label, str);
}

bool SlicesHandler::remove_mark(Point p, unsigned radius,
		unsigned short slicenr)
{
	if (slicenr < _nrslices && slicenr >= 0)
		return (_image_slices[slicenr]).remove_mark(p, radius);
	else
		return false;
}

void SlicesHandler::get_labels(std::vector<augmentedmark>* labels)
{
	labels->clear();
	std::vector<Mark> labels1;
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		_image_slices[i].get_labels(&labels1);
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
	(_image_slices[_activeslice]).add_vm(vm1);
}

bool SlicesHandler::remove_vm(Point p, unsigned radius)
{
	return (_image_slices[_activeslice]).del_vm(p, radius);
}

void SlicesHandler::add_vm(std::vector<Mark>* vm1, unsigned short slicenr)
{
	if (slicenr < _nrslices && slicenr >= 0)
		(_image_slices[slicenr]).add_vm(vm1);
}

bool SlicesHandler::remove_vm(Point p, unsigned radius, unsigned short slicenr)
{
	if (slicenr < _nrslices && slicenr >= 0)
		return (_image_slices[slicenr]).del_vm(p, radius);
	else
		return false;
}

void SlicesHandler::add_limit(std::vector<Point>* vp1)
{
	(_image_slices[_activeslice]).add_limit(vp1);
}

bool SlicesHandler::remove_limit(Point p, unsigned radius)
{
	return (_image_slices[_activeslice]).del_limit(p, radius);
}

void SlicesHandler::add_limit(std::vector<Point>* vp1, unsigned short slicenr)
{
	if (slicenr < _nrslices && slicenr >= 0)
		(_image_slices[slicenr]).add_limit(vp1);
}

bool SlicesHandler::remove_limit(Point p, unsigned radius,
		unsigned short slicenr)
{
	if (slicenr < _nrslices && slicenr >= 0)
		return (_image_slices[slicenr]).del_limit(p, radius);
	else
		return false;
}

void SlicesHandler::newbmp(unsigned short width1, unsigned short height1, unsigned short nrofslices, const std::function<void(float**)>& init_callback)
{
	_activeslice = 0;
	_startslice = 0;
	_endslice = _nrslices = nrofslices;
	_os.set_sizenr(_nrslices);
	_image_slices.resize(nrofslices);

	for (unsigned short i = 0; i < _nrslices; i++)
		_image_slices[i].newbmp(width1, height1);

	// now that memory is allocated give callback a chance to 'initialize' the data
	if (init_callback)
	{
		init_callback(source_slices().data());
	}

	// Ranges
	Pair dummy;
	_slice_ranges.resize(nrofslices);
	_slice_bmpranges.resize(nrofslices);
	compute_range_mode1(&dummy);
	compute_bmprange_mode1(&dummy);

	_loaded = true;

	_width = width1;
	_height = height1;
	_area = _height * (unsigned int)_width;
	set_active_tissuelayer(0);

	new_overlay();
}

void SlicesHandler::freebmp()
{
	for (unsigned short i = 0; i < _nrslices; i++)
		_image_slices[i].freebmp();

	_loaded = false;
}

void SlicesHandler::clear_bmp()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].clear_bmp();
}

void SlicesHandler::clear_work()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].clear_work();
}

void SlicesHandler::clear_overlay()
{
	for (unsigned int i = 0; i < _area; i++)
		_overlay[i] = 0.0f;
}

void SlicesHandler::new_overlay()
{
	if (_overlay != nullptr)
		free(_overlay);
	_overlay = (float*)malloc(sizeof(float) * _area);
	clear_overlay();
}

void SlicesHandler::set_bmp(unsigned short slicenr, float* bits,
		unsigned char mode)
{
	(_image_slices[slicenr]).set_bmp(bits, mode);
}

void SlicesHandler::set_work(unsigned short slicenr, float* bits,
		unsigned char mode)
{
	(_image_slices[slicenr]).set_work(bits, mode);
}

void SlicesHandler::set_tissue(unsigned short slicenr, tissues_size_t* bits)
{
	(_image_slices[slicenr]).set_tissue(_active_tissuelayer, bits);
}

void SlicesHandler::copy2bmp(unsigned short slicenr, float* bits,
		unsigned char mode)
{
	(_image_slices[slicenr]).copy2bmp(bits, mode);
}

void SlicesHandler::copy2work(unsigned short slicenr, float* bits,
		unsigned char mode)
{
	(_image_slices[slicenr]).copy2work(bits, mode);
}

void SlicesHandler::copy2tissue(unsigned short slicenr, tissues_size_t* bits)
{
	(_image_slices[slicenr]).copy2tissue(_active_tissuelayer, bits);
}

void SlicesHandler::copyfrombmp(unsigned short slicenr, float* bits)
{
	(_image_slices[slicenr]).copyfrombmp(bits);
}

void SlicesHandler::copyfromwork(unsigned short slicenr, float* bits)
{
	(_image_slices[slicenr]).copyfromwork(bits);
}

void SlicesHandler::copyfromtissue(unsigned short slicenr, tissues_size_t* bits)
{
	(_image_slices[slicenr]).copyfromtissue(_active_tissuelayer, bits);
}

#ifdef TISSUES_SIZE_TYPEDEF
void SlicesHandler::copyfromtissue(unsigned short slicenr, unsigned char* bits)
{
	(_image_slices[slicenr]).copyfromtissue(_active_tissuelayer, bits);
}
#endif // TISSUES_SIZE_TYPEDEF

void SlicesHandler::copyfromtissuepadded(unsigned short slicenr,
		tissues_size_t* bits,
		unsigned short padding)
{
	(_image_slices[slicenr])
			.copyfromtissuepadded(_active_tissuelayer, bits, padding);
}

unsigned int SlicesHandler::make_histogram(bool includeoutofrange)
{
	// \note unused function
	unsigned int histogram[256];
	unsigned int* dummy;
	unsigned int l = 0;

	for (unsigned short i = _startslice; i < _endslice; i++)
		l += _image_slices[i].make_histogram(includeoutofrange);

	dummy = (_image_slices[_startslice]).return_histogram();
	for (unsigned short i = 0; i < 255; i++)
		histogram[i] = dummy[i];

	for (unsigned short j = _startslice + 1; j < _endslice; j++)
	{
		dummy = _image_slices[j].return_histogram();
		for (unsigned short i = 0; i < 255; i++)
			histogram[i] += dummy[i];
	}

	return l;
}

unsigned int SlicesHandler::return_area()
{
	return (_image_slices[0]).return_area();
}

unsigned short SlicesHandler::width() const { return _width; }

unsigned short SlicesHandler::height() const { return _height; }

unsigned short SlicesHandler::num_slices() const { return _nrslices; }

unsigned short SlicesHandler::start_slice() const { return _startslice; }

unsigned short SlicesHandler::end_slice() const { return _endslice; }

void SlicesHandler::set_startslice(unsigned short startslice1)
{
	_startslice = startslice1;
}

void SlicesHandler::set_endslice(unsigned short endslice1)
{
	_endslice = endslice1;
}

bool SlicesHandler::isloaded() { return _loaded; }

void SlicesHandler::gaussian(float sigma)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].gaussian(sigma);

	return;
}

void SlicesHandler::fill_holes(float f, int minsize)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].fill_holes(f, minsize);
	}
}

void SlicesHandler::fill_holestissue(tissues_size_t f, int minsize)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].fill_holestissue(_active_tissuelayer, f, minsize);
	}
}

void SlicesHandler::remove_islands(float f, int minsize)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].remove_islands(f, minsize);
	}
}

void SlicesHandler::remove_islandstissue(tissues_size_t f, int minsize)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].remove_islandstissue(_active_tissuelayer, f, minsize);
	}
}

void SlicesHandler::fill_gaps(int minsize, bool connectivity)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].fill_gaps(minsize, connectivity);
	}
}

void SlicesHandler::fill_gapstissue(int minsize, bool connectivity)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].fill_gapstissue(_active_tissuelayer, minsize, connectivity);
	}
}

bool SlicesHandler::value_at_boundary3D(float value)
{
	// Top
	float* tmp = &(_image_slices[_startslice].return_work()[0]);
	for (unsigned pos = 0; pos < _area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	// Sides
	for (unsigned short i = _startslice + 1; i < _endslice - 1; i++)
	{
		if (_image_slices[i].value_at_boundary(value))
		{
			return true;
		}
	}

	// Bottom
	tmp = &(_image_slices[_endslice - 1].return_work()[0]);
	for (unsigned pos = 0; pos < _area; pos++, tmp++)
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
	tissues_size_t* tmp = &(_image_slices[_startslice].return_tissues(_active_tissuelayer)[0]);
	for (unsigned pos = 0; pos < _area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	// Sides
	for (unsigned short i = _startslice + 1; i < _endslice - 1; i++)
	{
		if (_image_slices[i]
						.tissuevalue_at_boundary(_active_tissuelayer, value))
		{
			return true;
		}
	}

	// Bottom
	tmp = &(_image_slices[_endslice - 1].return_tissues(_active_tissuelayer)[0]);
	for (unsigned pos = 0; pos < _area; pos++, tmp++)
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

		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			float* bits = _image_slices[i].return_work();
			for (unsigned pos = 0; pos < _area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const iN = _endslice;
#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].add_skin(i1, setto);
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

		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			float* bits = _image_slices[i].return_work();
			for (unsigned pos = 0; pos < _area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const iN = _endslice;
#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].add_skin_outside(i1, setto);
	}

	return setto;
}

void SlicesHandler::add_skintissue(int i1, tissues_size_t f)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].add_skintissue(_active_tissuelayer, i1, f);
	}
}

void SlicesHandler::add_skintissue_outside(int i1, tissues_size_t f)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].add_skintissue_outside(_active_tissuelayer, i1, f);
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

		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			float* bits = _image_slices[i].return_work();
			for (unsigned pos = 0; pos < _area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const iN = _endslice;
#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].fill_unassigned(setto);
	}
}

void SlicesHandler::fill_unassignedtissue(tissues_size_t f)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].fill_unassignedtissue(_active_tissuelayer, f);
	}
}

void SlicesHandler::kmeans(unsigned short slicenr, short nrtissues, unsigned int iternr, unsigned int converge)
{
	if (slicenr >= _startslice && slicenr < _endslice)
	{
		KMeans kmeans;
		float* bits[1];
		bits[0] = _image_slices[slicenr].return_bmp();
		float weights[1];
		weights[0] = 1;
		kmeans.init(_width, _height, nrtissues, 1, bits, weights);
		kmeans.make_iter(iternr, converge);
		kmeans.return_m(_image_slices[slicenr].return_work());
		_image_slices[slicenr].set_mode(2, false);

		for (unsigned short i = _startslice; i < slicenr; i++)
		{
			bits[0] = _image_slices[i].return_bmp();
			kmeans.apply_to(bits, _image_slices[i].return_work());
			_image_slices[i].set_mode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < _endslice; i++)
		{
			bits[0] = _image_slices[i].return_bmp();
			kmeans.apply_to(bits, _image_slices[i].return_work());
			_image_slices[i].set_mode(2, false);
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
	if (slicenr >= _startslice && slicenr < _endslice)
	{
		KMeans kmeans;
		float** bits = new float*[dim];
		for (unsigned short j = 0; j + 1 < dim; j++)
		{
			bits[j + 1] = new float[_area];
			if (bits[j + 1] == nullptr)
			{
				for (unsigned short i = 1; i < j; i++)
					delete[] bits[i];
				delete[] bits;
				return;
			}
		}

		bits[0] = _image_slices[slicenr].return_bmp();
		for (unsigned short i = 0; i + 1 < dim; i++)
		{
			if (!ImageReader::getSlice(mhdfiles[i].c_str(), bits[i + 1],
							slicenr, _width, _height))
			{
				for (unsigned short j = 1; j < dim; j++)
					delete[] bits[j];
				delete[] bits;
				return;
			}
		}
		kmeans.init(_width, _height, nrtissues, dim, bits, weights);
		kmeans.make_iter(iternr, converge);
		kmeans.return_m(_image_slices[slicenr].return_work());
		_image_slices[slicenr].set_mode(2, false);

		for (unsigned short i = _startslice; i < slicenr; i++)
		{
			bits[0] = _image_slices[i].return_bmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ImageReader::getSlice(mhdfiles[k].c_str(), bits[k + 1], i,
								_width, _height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.apply_to(bits, _image_slices[i].return_work());
			_image_slices[i].set_mode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < _endslice; i++)
		{
			bits[0] = _image_slices[i].return_bmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ImageReader::getSlice(mhdfiles[k].c_str(), bits[k + 1], i,
								_width, _height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.apply_to(bits, _image_slices[i].return_work());
			_image_slices[i].set_mode(2, false);
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
	if (slicenr >= _startslice && slicenr < _endslice)
	{
		float** bits = new float*[dim];
		for (unsigned short j = 0; j + 1 < dim; j++)
		{
			bits[j + 1] = new float[_area];
			if (bits[j + 1] == nullptr)
			{
				for (unsigned short i = 1; i < j; i++)
					delete[] bits[i];
				delete[] bits;
				return;
			}
		}

		bits[0] = _image_slices[slicenr].return_bmp();
		for (unsigned short i = 0; i + 1 < dim; i++)
		{
			if (!ChannelExtractor::getSlice(pngfiles[0].c_str(), bits[i + 1],
							exctractChannel[i], slicenr, _width,
							_height))
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
				kmeans.init(_width, _height, nrtissues, dim, bits, weights,
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
			kmeans.init(_width, _height, nrtissues, dim, bits, weights);
		}
		kmeans.make_iter(iternr, converge);
		kmeans.return_m(_image_slices[slicenr].return_work());
		_image_slices[slicenr].set_mode(2, false);

		for (unsigned short i = _startslice; i < slicenr; i++)
		{
			bits[0] = _image_slices[i].return_bmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ChannelExtractor::getSlice(pngfiles[0].c_str(),
								bits[i + 1], exctractChannel[i],
								i, _width, _height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.apply_to(bits, _image_slices[i].return_work());
			_image_slices[i].set_mode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < _endslice; i++)
		{
			bits[0] = _image_slices[i].return_bmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ChannelExtractor::getSlice(pngfiles[0].c_str(),
								bits[i + 1], exctractChannel[i],
								i, _width, _height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.apply_to(bits, _image_slices[i].return_work());
			_image_slices[i].set_mode(2, false);
		}

		for (unsigned short j = 1; j < dim; j++)
			delete[] bits[j];
		delete[] bits;
	}
}

void SlicesHandler::em(unsigned short slicenr, short nrtissues,
		unsigned int iternr, unsigned int converge)
{
	if (slicenr >= _startslice && slicenr < _endslice)
	{
		ExpectationMaximization em;
		float* bits[1];
		bits[0] = _image_slices[slicenr].return_bmp();
		float weights[1];
		weights[0] = 1;
		em.init(_width, _height, nrtissues, 1, bits, weights);
		em.make_iter(iternr, converge);
		em.classify(_image_slices[slicenr].return_work());
		_image_slices[slicenr].set_mode(2, false);

		for (unsigned short i = _startslice; i < slicenr; i++)
		{
			bits[0] = _image_slices[i].return_bmp();
			em.apply_to(bits, _image_slices[i].return_work());
			_image_slices[i].set_mode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < _endslice; i++)
		{
			bits[0] = _image_slices[i].return_bmp();
			em.apply_to(bits, _image_slices[i].return_work());
			_image_slices[i].set_mode(2, false);
		}
	}
}

void SlicesHandler::aniso_diff(float dt, int n, float (*f)(float, float),
		float k, float restraint)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].aniso_diff(dt, n, f, k, restraint);

	return;
}

void SlicesHandler::cont_anisodiff(float dt, int n, float (*f)(float, float),
		float k, float restraint)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].cont_anisodiff(dt, n, f, k, restraint);

	return;
}

void SlicesHandler::median_interquartile(bool median)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].median_interquartile(median);
}

void SlicesHandler::average(unsigned short n)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].average(n);
}

void SlicesHandler::sigmafilter(float sigma, unsigned short nx,
		unsigned short ny)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].sigmafilter(sigma, nx, ny);
}

void SlicesHandler::threshold(float* thresholds)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].threshold(thresholds);
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
	dummy3D.newbmp((unsigned short)(_image_slices[0].return_width()),
			(unsigned short)(_image_slices[0].return_height()),
			between + 2);
	dummy3D.set_slicethickness(_thickness / (between + 1));
	Pair pair1 = get_pixelsize();
	dummy3D.set_pixelsize(pair1.high, pair1.low);

	FILE* fp = dummy3D.save_contourprologue(filename,
			(between + 1) * _nrslices - between);

	for (unsigned short j = 0; j + 1 < _nrslices; j++)
	{
		dummy3D.copy2tissue(0, _image_slices[j].return_tissues(_active_tissuelayer));
		dummy3D.copy2tissue(between + 1, _image_slices[j + 1].return_tissues(
																				 _active_tissuelayer));
		dummy3D.interpolatetissuegrey(0, between + 1); // TODO: Use interpolatetissuegrey_medianset?

		dummy3D.extract_contours(minsize, tissuevec);
		if (dp)
			dummy3D.dougpeuck_line(epsilon);
		fp = dummy3D.save_contoursection(fp, 0, between, (between + 1) * j);
	}

	fp = dummy3D.save_contoursection(fp, 0, 0, (between + 1) * (_nrslices - 1));

	fp = dummy3D.save_tissuenamescolors(fp);

	fclose(fp);
}

void SlicesHandler::extractinterpolatesave_contours2_xmirrored(
		int minsize, std::vector<tissues_size_t>& tissuevec, unsigned short between,
		bool dp, float epsilon, const char* filename)
{
	std::vector<std::vector<Point>> v1, v2;
	std::vector<Point_type> vP;

	SlicesHandler dummy3D;
	dummy3D.newbmp((unsigned short)(_image_slices[0].return_width()),
			(unsigned short)(_image_slices[0].return_height()),
			between + 2);
	dummy3D.set_slicethickness(_thickness / (between + 1));
	Pair pair1 = get_pixelsize();
	dummy3D.set_pixelsize(pair1.high / 2, pair1.low / 2);

	FILE* fp = dummy3D.save_contourprologue(filename,
			(between + 1) * _nrslices - between);

	for (unsigned short j = 0; j + 1 < _nrslices; j++)
	{
		dummy3D.copy2tissue(0,
				_image_slices[j].return_tissues(_active_tissuelayer));
		dummy3D.copy2tissue(between + 1, _image_slices[j + 1].return_tissues(
																				 _active_tissuelayer));
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
		dummy3D.shift_contours(-(int)dummy3D.width(),
				-(int)dummy3D.height());
		fp = dummy3D.save_contoursection(fp, 0, between, (between + 1) * j);
	}

	fp = dummy3D.save_contoursection(fp, 0, 0, (between + 1) * (_nrslices - 1));

	fp = dummy3D.save_tissuenamescolors(fp);

	fclose(fp);
}

void SlicesHandler::extract_contours(int minsize, std::vector<tissues_size_t>& tissuevec)
{
	_os.clear();
	std::vector<std::vector<Point>> v1, v2;
	std::vector<Point_type> vP;

	for (std::vector<tissues_size_t>::iterator it1 = tissuevec.begin();
			 it1 != tissuevec.end(); it1++)
	{
		for (unsigned short i = 0; i < _nrslices; i++)
		{
			v1.clear();
			v2.clear();
			_image_slices[i]
					.get_tissuecontours(_active_tissuelayer, *it1, &v1, &v2,
							minsize);
			for (std::vector<std::vector<Point>>::iterator it = v1.begin();
					 it != v1.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,true);
				_os.add_line(i, *it1, &(*it), true);
			}
			for (std::vector<std::vector<Point>>::iterator it = v2.begin();
					 it != v2.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,false);
				_os.add_line(i, *it1, &(*it), false);
			}
		}
	}
}

void SlicesHandler::extract_contours2_xmirrored(int minsize, std::vector<tissues_size_t>& tissuevec)
{
	_os.clear();
	std::vector<std::vector<Point>> v1, v2;

	for (auto tissue_label : tissuevec)
	{
		for (unsigned short i = 0; i < _nrslices; i++)
		{
			v1.clear();
			v2.clear();
			_image_slices[i].get_tissuecontours2_xmirrored(_active_tissuelayer, tissue_label, &v1, &v2, minsize);
			for (auto& line : v1)
			{
				_os.add_line(i, tissue_label, &line, true);
			}
			for (auto& line : v2)
			{
				_os.add_line(i, tissue_label, &line, false);
			}
		}
	}
}

void SlicesHandler::extract_contours2_xmirrored(int minsize, std::vector<tissues_size_t>& tissuevec, float epsilon)
{
	_os.clear();
	std::vector<std::vector<Point>> v1, v2;

	for (auto tissue_label : tissuevec)
	{
		for (unsigned short i = 0; i < _nrslices; i++)
		{
			v1.clear();
			v2.clear();
			_image_slices[i].get_tissuecontours2_xmirrored(_active_tissuelayer, tissue_label, &v1, &v2, minsize, epsilon);
			for (auto& line : v1)
			{
				_os.add_line(i, tissue_label, &line, true);
			}
			for (auto& line : v2)
			{
				_os.add_line(i, tissue_label, &line, false);
			}
		}
	}
}

void SlicesHandler::bmp_sum()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].bmp_sum();
	}
}

void SlicesHandler::bmp_add(float f)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].bmp_add(f);
	}
}

void SlicesHandler::bmp_diff()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].bmp_diff();
	}
}

void SlicesHandler::bmp_mult()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].bmp_mult();
	}
}

void SlicesHandler::bmp_mult(float f)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].bmp_mult(f);
	}
}

void SlicesHandler::bmp_overlay(float alpha)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].bmp_overlay(alpha);
	}
}

void SlicesHandler::bmp_abs()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].bmp_abs();
	}
}

void SlicesHandler::bmp_neg()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].bmp_neg();
	}
}

void SlicesHandler::scale_colors(Pair p)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].scale_colors(p);
	}
}

void SlicesHandler::crop_colors()
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].crop_colors();
	}
}

void SlicesHandler::get_range(Pair* pp)
{
	Pair p;
	_image_slices[_startslice].get_range(pp);

	for (unsigned short i = _startslice + 1; i < _endslice; i++)
	{
		_image_slices[i].get_range(&p);
		if ((*pp).high < p.high)
			(*pp).high = p.high;
		if ((*pp).low > p.low)
			(*pp).low = p.low;
	}
}

void SlicesHandler::compute_range_mode1(Pair* pp)
{
	// Update ranges for all mode 1 slices and compute total range
	pp->low = FLT_MAX;
	pp->high = 0.f;

#pragma omp parallel
	{
		// this data is thread private
		Pair p;
		float low = FLT_MAX;
		float high = 0.f;

		const int iN = _nrslices;
#pragma omp for
		for (int i = 0; i < iN; i++)
		{
			if (_image_slices[i].return_mode(false) == 1)
			{
				_image_slices[i].get_range(&p);
				_slice_ranges[i] = p;
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

	if (pp->high < pp->low)
	{
		// No mode 1 slices: Set to mode 2 range
		pp->low = 255.0f;
	}
}

void SlicesHandler::compute_range_mode1(unsigned short updateSlicenr, Pair* pp)
{
	// Update range for single mode 1 slice
	if (_image_slices[updateSlicenr].return_mode(false) == 1)
	{
		_image_slices[updateSlicenr].get_range(&_slice_ranges[updateSlicenr]);
	}

	// Compute total range
	pp->high = 0.0f;
	pp->low = FLT_MAX;
	for (unsigned short i = 0; i < _nrslices; ++i)
	{
		if (_image_slices[i].return_mode(false) != 1)
			continue;
		Pair p = _slice_ranges[i];
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
	_image_slices[_startslice].get_bmprange(pp);

	for (unsigned short i = _startslice + 1; i < _endslice; i++)
	{
		_image_slices[i].get_bmprange(&p);
		if ((*pp).high < p.high)
			(*pp).high = p.high;
		if ((*pp).low > p.low)
			(*pp).low = p.low;
	}
}

void SlicesHandler::compute_bmprange_mode1(Pair* pp)
{
	// Update ranges for all mode 1 slices and compute total range
	pp->low = FLT_MAX;
	pp->high = 0.f;

#pragma omp parallel
	{
		// this data is thread private
		Pair p;
		float low = FLT_MAX;
		float high = 0.f;

		const int iN = _nrslices;
#pragma omp for
		for (int i = 0; i < iN; ++i)
		{
			if (_image_slices[i].return_mode(true) == 1)
			{
				_image_slices[i].get_bmprange(&p);
				_slice_bmpranges[i] = p;

				high = std::max(high, p.high);
				low = std::min(low, p.low);
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

	if (pp->high < pp->low)
	{
		// No mode 1 slices: Set to mode 2 range
		pp->low = 255.0f;
	}
}

void SlicesHandler::compute_bmprange_mode1(unsigned short updateSlicenr, Pair* pp)
{
	// Update range for single mode 1 slice
	if (_image_slices[updateSlicenr].return_mode(true) == 1)
	{
		_image_slices[updateSlicenr].get_bmprange(&_slice_bmpranges[updateSlicenr]);
	}

	// Compute total range
	pp->high = 0.0f;
	pp->low = FLT_MAX;
	for (unsigned short i = 0; i < _nrslices; ++i)
	{
		if (_image_slices[i].return_mode(true) != 1)
			continue;
		Pair p = _slice_bmpranges[i];
		if (pp->high < p.high)
			pp->high = p.high;
		if (pp->low > p.low)
			pp->low = p.low;
	}

	if (pp->high < pp->low)
	{
		// No mode 1 slices: Set to mode 2 range
		pp->low = 255.0f;
	}
}

void SlicesHandler::get_rangetissue(tissues_size_t* pp)
{
	tissues_size_t p;
	_image_slices[_startslice].get_rangetissue(_active_tissuelayer, pp);

	for (unsigned short i = _startslice + 1; i < _endslice; i++)
	{
		_image_slices[i].get_rangetissue(_active_tissuelayer, &p);
		if ((*pp) < p)
			(*pp) = p;
	}
}

void SlicesHandler::zero_crossings(bool connectivity)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].zero_crossings(connectivity);
	}
}

void SlicesHandler::save_contours(const char* filename)
{
	_os.print(filename, TissueInfos::GetTissueCount());
	FILE* fp = fopen(filename, "a");
	fp = save_tissuenamescolors(fp);
	fclose(fp);
}

void SlicesHandler::shift_contours(int dx, int dy)
{
	_os.shift_contours(dx, dy);
}

void SlicesHandler::setextrusion_contours(int top, int bottom)
{
	_os.set_thickness(bottom * _thickness, 0);
	if (_nrslices > 1)
	{
		_os.set_thickness(top * _thickness, _nrslices - 1);
	}
}

void SlicesHandler::resetextrusion_contours()
{
	_os.set_thickness(_thickness, 0);
	if (_nrslices > 1)
	{
		_os.set_thickness(_thickness, _nrslices - 1);
	}
}

FILE* SlicesHandler::save_contourprologue(const char* filename, unsigned nr_slices)
{
	return _os.printprologue(filename, nr_slices, TissueInfos::GetTissueCount());
}

FILE* SlicesHandler::save_contoursection(FILE* fp, unsigned startslice1, unsigned endslice1, unsigned offset)
{
	_os.printsection(fp, startslice1, endslice1, offset, TissueInfos::GetTissueCount());
	return fp;
}

FILE* SlicesHandler::save_tissuenamescolors(FILE* fp)
{
	tissues_size_t tissueCount = TissueInfos::GetTissueCount();
	TissueInfo* tissueInfo;

	fprintf(fp, "NT%u\n", tissueCount);

	if (tissueCount > 255)
	{ // Only print tissue indices which contain outlines

		// Collect used tissue indices in ascending order
		std::set<tissues_size_t> tissueIndices;
		_os.insert_tissue_indices(tissueIndices);
		std::set<tissues_size_t>::iterator idxIt;
		for (idxIt = tissueIndices.begin(); idxIt != tissueIndices.end();
				 ++idxIt)
		{
			tissueInfo = TissueInfos::GetTissueInfo(*idxIt);
			fprintf(fp, "T%i %f %f %f %s\n", (int)*idxIt, tissueInfo->color[0],
					tissueInfo->color[1], tissueInfo->color[2],
					tissueInfo->name.c_str());
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
					tissueInfo->name.c_str());
		}
	}

	return fp;
}

void SlicesHandler::dougpeuck_line(float epsilon)
{
	_os.doug_peuck(epsilon, true);
}

void SlicesHandler::hysteretic(float thresh_low, float thresh_high, bool connectivity, unsigned short nrpasses)
{
	float setvalue = 255;
	unsigned short slicenr = _startslice;

	clear_work();

	_image_slices[slicenr].hysteretic(thresh_low, thresh_high, connectivity,
			setvalue);
	for (unsigned short i = 0; i < nrpasses; i++)
	{
		while (++slicenr < _endslice)
		{
			_image_slices[slicenr].hysteretic(
					thresh_low, thresh_high, connectivity,
					_image_slices[slicenr - 1].return_work(), setvalue - 1,
					setvalue);
		}
		setvalue++;
		slicenr--;
		while (slicenr-- > _startslice)
		{
			_image_slices[slicenr].hysteretic(
					thresh_low, thresh_high, connectivity,
					_image_slices[slicenr + 1].return_work(), setvalue - 1,
					setvalue);
		}
		setvalue++;
		slicenr = _startslice;
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
	if (slicenr < _endslice && slicenr >= _startslice)
	{
		float setvalue = 255;
		Pair tp;

		clear_work();

		_image_slices[slicenr].thresholded_growing(p, threshfactor_low,
				threshfactor_high, connectivity, setvalue, &tp);

		for (unsigned short i = 0; i < nrpasses; i++)
		{
			while (++slicenr < _endslice)
			{
				_image_slices[slicenr].thresholded_growing(
						tp.low, tp.high, connectivity,
						_image_slices[slicenr - 1].return_work(), setvalue - 1,
						setvalue);
			}
			setvalue++;
			slicenr--;
			while (slicenr-- > _startslice)
			{
				_image_slices[slicenr].thresholded_growing(
						tp.low, tp.high, connectivity,
						_image_slices[slicenr + 1].return_work(), setvalue - 1,
						setvalue);
			}
			setvalue++;
			slicenr = _startslice;
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
	if (slicenr >= _startslice && slicenr < _endslice)
	{
		unsigned position = p.px + p.py * (unsigned)_width;
		std::vector<posit> s;
		posit p1;

		for (unsigned short z = _startslice; z < _endslice; z++)
		{
			float* work = _image_slices[z].return_work();
			float* bmp = _image_slices[z].return_bmp();
			int i = 0;
			for (unsigned short j = 0; j < _height; j++)
			{
				for (unsigned short k = 0; k < _width; k++)
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
		float* work = _image_slices[slicenr].return_work();
		work[position] = set_to;

		//	hysteretic_growth(results,&s,width+2,height+2,connectivity,set_to);
		posit i, j;

		while (!s.empty())
		{
			i = s.back();
			s.pop_back();

			work = _image_slices[i.pz].return_work();
			if (i.pxy % _width != 0 && work[i.pxy - 1] == -1)
			{
				work[i.pxy - 1] = set_to;
				j.pxy = i.pxy - 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			if ((i.pxy + 1) % _width != 0 && work[i.pxy + 1] == -1)
			{
				work[i.pxy + 1] = set_to;
				j.pxy = i.pxy + 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pxy >= _width && work[i.pxy - _width] == -1)
			{
				work[i.pxy - _width] = set_to;
				j.pxy = i.pxy - _width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pxy < _area - _width && work[i.pxy + _width] == -1)
			{
				work[i.pxy + _width] = set_to;
				j.pxy = i.pxy + _width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pz > _startslice)
			{
				work = _image_slices[i.pz - 1].return_work();
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
			if (i.pz + 1 < _endslice)
			{
				work = _image_slices[i.pz + 1].return_work();
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

		for (unsigned short z = _startslice; z < _endslice; z++)
		{
			work = _image_slices[z].return_work();
			for (unsigned i1 = 0; i1 < _area; i1++)
				if (work[i1] == -1)
					work[i1] = 0;
			_image_slices[z].set_mode(2, false);
		}
	}

	return;
}

void SlicesHandler::add2tissueall_connected(tissues_size_t tissuetype, Point p,
		bool override)
{
	if (_activeslice >= _startslice && _activeslice < _endslice)
	{
		float set_to = (float)123E10;
		unsigned position = p.px + p.py * (unsigned)_width;
		std::vector<posit> s;
		posit p1;

		p1.pxy = position;
		p1.pz = _activeslice;

		s.push_back(p1);
		float* work = _image_slices[_activeslice].return_work();
		float f = work[position];
		tissues_size_t* tissue =
				_image_slices[_activeslice].return_tissues(_active_tissuelayer);
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

			work = _image_slices[i.pz].return_work();
			tissue = _image_slices[i.pz].return_tissues(_active_tissuelayer);
			//if(i.pxy%width!=0&&work[i.pxy-1]==f&&(override||tissue[i.pxy-1]==0)) {
			if (i.pxy % _width != 0 && work[i.pxy - 1] == f &&
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
			if ((i.pxy + 1) % _width != 0 && work[i.pxy + 1] == f &&
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
			if (i.pxy >= _width && work[i.pxy - _width] == f &&
					(tissue[i.pxy - _width] == 0 ||
							(override && TissueInfos::GetTissueLocked(
															 tissue[i.pxy - _width]) == false)))
			{
				work[i.pxy - _width] = set_to;
				tissue[i.pxy - _width] = tissuetype;
				j.pxy = i.pxy - _width;
				j.pz = i.pz;
				s.push_back(j);
			}
			//if(i.pxy<=area-width&&work[i.pxy+width]==f&&(override||tissue[i.pxy+width]==0)) {
			if (i.pxy < _area - _width && work[i.pxy + _width] == f &&
					(tissue[i.pxy + _width] == 0 ||
							(override && TissueInfos::GetTissueLocked(
															 tissue[i.pxy + _width]) == false)))
			{
				work[i.pxy + _width] = set_to;
				tissue[i.pxy + _width] = tissuetype;
				j.pxy = i.pxy + _width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pz > _startslice)
			{
				work = _image_slices[i.pz - 1].return_work();
				tissue =
						_image_slices[i.pz - 1].return_tissues(_active_tissuelayer);
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
			if (i.pz + 1 < _endslice)
			{
				work = _image_slices[i.pz + 1].return_work();
				tissue =
						_image_slices[i.pz + 1].return_tissues(_active_tissuelayer);
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

		for (unsigned short z = _startslice; z < _endslice; z++)
		{
			work = _image_slices[z].return_work();
			for (unsigned i1 = 0; i1 < _area; i1++)
				if (work[i1] == set_to)
					work[i1] = f;
		}
	}
}

void SlicesHandler::subtract_tissueall_connected(tissues_size_t tissuetype,
		Point p)
{
	if (_activeslice < _endslice && _activeslice >= _startslice)
	{
		float set_to = (float)123E10;
		unsigned position = p.px + p.py * (unsigned)_width;
		std::vector<posit> s;
		posit p1;

		p1.pxy = position;
		p1.pz = _activeslice;

		s.push_back(p1);
		float* work = _image_slices[_activeslice].return_work();
		float f = work[position];
		tissues_size_t* tissue =
				_image_slices[_activeslice].return_tissues(_active_tissuelayer);
		if (tissue[position] == tissuetype)
			tissue[position] = tissuetype;
		if (tissue[position] == tissuetype)
			work[position] = set_to;

		posit i, j;

		while (!s.empty())
		{
			i = s.back();
			s.pop_back();

			work = _image_slices[i.pz].return_work();
			tissue = _image_slices[i.pz].return_tissues(_active_tissuelayer);
			if (i.pxy % _width != 0 && work[i.pxy - 1] == f &&
					tissue[i.pxy - 1] == tissuetype)
			{
				work[i.pxy - 1] = set_to;
				tissue[i.pxy - 1] = 0;
				j.pxy = i.pxy - 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			if ((i.pxy + 1) % _width != 0 && work[i.pxy + 1] == f &&
					tissue[i.pxy + 1] == tissuetype)
			{
				work[i.pxy + 1] = set_to;
				tissue[i.pxy + 1] = 0;
				j.pxy = i.pxy + 1;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pxy >= _width && work[i.pxy - _width] == f &&
					tissue[i.pxy - _width] == tissuetype)
			{
				work[i.pxy - _width] = set_to;
				tissue[i.pxy - _width] = 0;
				j.pxy = i.pxy - _width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pxy < _area - _width && work[i.pxy + _width] == f &&
					tissue[i.pxy + _width] == tissuetype)
			{
				work[i.pxy + _width] = set_to;
				tissue[i.pxy + _width] = 0;
				j.pxy = i.pxy + _width;
				j.pz = i.pz;
				s.push_back(j);
			}
			if (i.pz > _startslice)
			{
				work = _image_slices[i.pz - 1].return_work();
				tissue =
						_image_slices[i.pz - 1].return_tissues(_active_tissuelayer);
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
			if (i.pz + 1 < _endslice)
			{
				work = _image_slices[i.pz + 1].return_work();
				tissue =
						_image_slices[i.pz + 1].return_tissues(_active_tissuelayer);
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

		for (unsigned short z = _startslice; z < _endslice; z++)
		{
			work = _image_slices[z].return_work();
			for (unsigned i1 = 0; i1 < _area; i1++)
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
	unsigned short slicenr = _startslice;

	clear_work();

	_image_slices[slicenr].double_hysteretic(thresh_low_l, thresh_low_h,
			thresh_high_l, thresh_high_h,
			connectivity, setvalue);
	//	if(nrslices>1) {
	for (unsigned short i = 0; i < nrpasses; i++)
	{
		while (++slicenr < _endslice)
		{
			_image_slices[slicenr].double_hysteretic(
					thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h,
					connectivity, _image_slices[slicenr - 1].return_work(),
					setvalue - 1, setvalue);
		}
		setvalue++;
		slicenr--;
		while (slicenr-- > _startslice)
		{
			_image_slices[slicenr].double_hysteretic(
					thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h,
					connectivity, _image_slices[slicenr + 1].return_work(),
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
}

void SlicesHandler::double_hysteretic_allslices(float thresh_low_l,
		float thresh_low_h,
		float thresh_high_l,
		float thresh_high_h,
		bool connectivity, float set_to)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		_image_slices[i].double_hysteretic(thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h, connectivity, set_to);
	}
}

void SlicesHandler::erosion(boost::variant<int, float> radius, bool true3d)
{
	morpho::MorphologicalOperation(this, radius, morpho::kErode, true3d);
}

void SlicesHandler::dilation(boost::variant<int, float> radius, bool true3d)
{
	morpho::MorphologicalOperation(this, radius, morpho::kDilate, true3d);
}

void SlicesHandler::closure(boost::variant<int, float> radius, bool true3d)
{
	morpho::MorphologicalOperation(this, radius, morpho::kClose, true3d);
}

void SlicesHandler::open(boost::variant<int, float> radius, bool true3d)
{
	morpho::MorphologicalOperation(this, radius, morpho::kOpen, true3d);
}

void SlicesHandler::interpolateworkgrey(unsigned short slice1, unsigned short slice2, bool connected)
{
	if (slice2 < slice1)
	{
		std::swap(slice1, slice2);
	}
	if (slice1 + 1 >= slice2) // no slices inbetween
	{
		return;
	}

	const short n = slice2 - slice1;
	if (!connected)
	{
		_image_slices[slice1].pushstack_bmp();
		_image_slices[slice2].pushstack_bmp();

		_image_slices[slice1].swap_bmpwork();
		_image_slices[slice2].swap_bmpwork();

		_image_slices[slice2].dead_reckoning();
		_image_slices[slice1].dead_reckoning();

		float* bmp1 = _image_slices[slice1].return_bmp();
		float* bmp2 = _image_slices[slice2].return_bmp();
		float* work1 = _image_slices[slice1].return_work();
		float* work2 = _image_slices[slice2].return_work();

		Point p;
		float prop;
		unsigned short n1;
		unsigned i1 = 0;

		for (p.py = 0; p.py < _height; p.py++)
		{
			for (p.px = 0; p.px < _width; p.px++)
			{
				if (work2[i1] + work1[i1] != 0)
					prop = work1[i1] / (work2[i1] + work1[i1]);
				else
					prop = 0.5f;
				n1 = (unsigned short)n * prop;
				for (unsigned short j = 1; j <= n1 && j < n; j++)
				{
					_image_slices[slice1 + j].set_work_pt(p, bmp1[i1]);
				}
				for (unsigned short j = n1 + 1; j < n; j++)
				{
					_image_slices[slice1 + j].set_work_pt(p, bmp2[i1]);
				}
				i1++;
			}
		}

		_image_slices[slice1].swap_bmpwork();
		_image_slices[slice2].swap_bmpwork();

		for (unsigned short j = 1; j < n; j++)
		{
			_image_slices[slice1 + j].set_mode(2, false);
		}

		_image_slices[slice2].popstack_bmp();
		_image_slices[slice1].popstack_bmp();
	}
	else
	{
		SliceHandlerItkWrapper itk_handler(this);
		auto img1 = itk_handler.GetTargetSlice(slice1);
		auto img2 = itk_handler.GetTargetSlice(slice2);

		ConnectedShapeBasedInterpolation interpolator;
		try
		{
			auto interpolated_slices = interpolator.interpolate(img1, img2, n - 1);

			for (short i = 0; i < n - 1; i++)
			{
				auto slice = interpolated_slices[i];
				const float* source = slice->GetPixelContainer()->GetImportPointer();
				size_t source_len = slice->GetPixelContainer()->Size();
				// copy to target (idx = slice1 + i + 1)
				float* target = _image_slices[slice1 + i + 1].return_work();
				std::copy(source, source + source_len, target);
			}
		}
		catch (std::exception& e)
		{
			ISEG_ERROR("failed to interpolate slices " << e.what());
		}
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
		_image_slices[slice1].get_range(&work_range);
		nCells = std::max(nCells, (unsigned int)(work_range.high + 0.5f));
		_image_slices[slice2].get_range(&work_range);
		nCells = std::max(nCells, (unsigned int)(work_range.high + 0.5f));
		unsigned short max_iterations =
				(unsigned short)std::sqrt((float)(_width * _width + _height * _height));

		// Backup images
		_image_slices[slice1].pushstack_work();
		_image_slices[slice2].pushstack_work();
		_image_slices[slice1].pushstack_bmp();
		_image_slices[slicehalf].pushstack_bmp();
		_image_slices[slice2].pushstack_bmp();

		// Interplation input to bmp
		_image_slices[slice1].swap_bmpwork();
		_image_slices[slice2].swap_bmpwork();

		// Input images
		float* f_1 = _image_slices[slice1].return_bmp();
		float* f_2 = _image_slices[slice2].return_bmp();

		if (handleVanishingComp)
		{
			// Determine forward and backward vanishing components
			std::set<float> vanishing_comp_forward;
			std::set<float> vanishing_comp_backward;
			float* connected_comp_forward =
					(float*)malloc(sizeof(float) * _area);
			float* connected_comp_backward =
					(float*)malloc(sizeof(float) * _area);

			_image_slices[slice1].connected_components(
					false, vanishing_comp_forward); // TODO: connectivity?
			_image_slices[slice1].copyfromwork(connected_comp_forward);

			_image_slices[slice2].connected_components(
					false, vanishing_comp_backward); // TODO: connectivity?
			_image_slices[slice2].copyfromwork(connected_comp_backward);

			for (unsigned int i = 0; i < _area; ++i)
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
					_image_slices[slice1].return_tissues(_active_tissuelayer);
			tissues_size_t* tissue2 =
					_image_slices[slice2].return_tissues(_active_tissuelayer);
			for (unsigned int i = 0; i < _area; ++i)
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
		float* g_i = _image_slices[slicehalf].return_work();
		float* gp_i = _image_slices[slicehalf].return_bmp();

		// Initialize g_0 and gp_0
		for (unsigned int i = 0; i < _area; ++i)
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
				_image_slices[slice1].copy2work(g_i, 1);
				_image_slices[slice1].dilation(1, connectivity);
			}
			else
			{
				_image_slices[slice2].copy2work(gp_i, 1);
				_image_slices[slice2].dilation(1, connectivity);
			}
			float* g_i_B = _image_slices[slice1].return_work();
			float* gp_i_B = _image_slices[slice2].return_work();

			// Compute g_i+1 and gp_i+1
			idempotence = true;
			if (iter % 2 == 0)
			{
				for (unsigned int i = 0; i < _area; ++i)
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
				for (unsigned int i = 0; i < _area; ++i)
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
			_image_slices[slice1].copy2work(f_1, 1);
			_image_slices[slice2].copy2work(f_2, 1);

			// Restore images
			_image_slices[slice2].popstack_bmp();
			_image_slices[slicehalf].popstack_bmp();
			_image_slices[slice1].popstack_bmp();

			// Recursion
			interpolateworkgrey_medianset(slice1, slicehalf, connectivity,
					false);
			interpolateworkgrey_medianset(slicehalf, slice2, connectivity,
					false);

			// Restore images
			_image_slices[slice2].popstack_work();
			_image_slices[slice1].popstack_work();
		}
		else
		{
			// Restore images
			_image_slices[slice2].popstack_bmp();
			_image_slices[slicehalf].popstack_bmp();
			_image_slices[slice1].popstack_bmp();
			_image_slices[slice2].popstack_work();
			_image_slices[slice1].popstack_work();

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
		_image_slices[slice1].pushstack_bmp();
		_image_slices[slice2].pushstack_bmp();
		_image_slices[slice1].pushstack_work();
		_image_slices[slice2].pushstack_work();

		_image_slices[slice1].tissue2work(_active_tissuelayer);
		_image_slices[slice2].tissue2work(_active_tissuelayer);

		_image_slices[slice1].swap_bmpwork();
		_image_slices[slice2].swap_bmpwork();

		_image_slices[slice2].dead_reckoning();
		_image_slices[slice1].dead_reckoning();

		tissues_size_t* bmp1 =
				_image_slices[slice1].return_tissues(_active_tissuelayer);
		tissues_size_t* bmp2 =
				_image_slices[slice2].return_tissues(_active_tissuelayer);
		float* work1 = _image_slices[slice1].return_work();
		float* work2 = _image_slices[slice2].return_work();

		Point p;
		float prop;
		unsigned short n1;
		unsigned i1 = 0;

		for (p.py = 0; p.py < _height; p.py++)
		{
			for (p.px = 0; p.px < _width; p.px++)
			{
				if (work2[i1] + work1[i1] != 0)
					prop = work1[i1] / (work2[i1] + work1[i1]);
				else
					prop = 0.5f;
				n1 = (unsigned short)n * prop;
				for (unsigned short j = 1; j <= n1 && j < n; j++)
				{
					_image_slices[slice1 + j].set_tissue_pt(_active_tissuelayer,
							p, bmp1[i1]);
				}
				for (unsigned short j = n1 + 1; j < n; j++)
				{
					_image_slices[slice1 + j].set_tissue_pt(_active_tissuelayer,
							p, bmp2[i1]);
				}
				i1++;
			}
		}

		for (unsigned short j = 1; j < n; j++)
		{
			_image_slices[slice1 + j].set_mode(2, false);
		}

		_image_slices[slice1].popstack_work();
		_image_slices[slice2].popstack_work();
		_image_slices[slice2].popstack_bmp();
		_image_slices[slice1].popstack_bmp();
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
				(unsigned short)std::sqrt((float)(_width * _width + _height * _height));

		// Backup images
		_image_slices[slice1].pushstack_work();
		_image_slices[slicehalf].pushstack_work();
		_image_slices[slice2].pushstack_work();
		_image_slices[slice1].pushstack_bmp();
		_image_slices[slicehalf].pushstack_bmp();
		_image_slices[slice2].pushstack_bmp();
		tissues_size_t* tissue1_copy = nullptr;
		tissues_size_t* tissue2_copy = nullptr;
		if (handleVanishingComp)
		{
			tissue1_copy =
					(tissues_size_t*)malloc(sizeof(tissues_size_t) * _area);
			_image_slices[slice1].copyfromtissue(_active_tissuelayer,
					tissue1_copy);
			tissue2_copy =
					(tissues_size_t*)malloc(sizeof(tissues_size_t) * _area);
			_image_slices[slice2].copyfromtissue(_active_tissuelayer,
					tissue2_copy);
		}

		// Interplation input to bmp
		_image_slices[slice1].tissue2work(_active_tissuelayer);
		_image_slices[slice2].tissue2work(_active_tissuelayer);
		_image_slices[slice1].swap_bmpwork();
		_image_slices[slice2].swap_bmpwork();

		// Input images
		float* f_1 = _image_slices[slice1].return_bmp();
		float* f_2 = _image_slices[slice2].return_bmp();

		if (handleVanishingComp)
		{
			// Determine forward and backward vanishing components
			std::set<float> vanishing_comp_forward;
			std::set<float> vanishing_comp_backward;
			float* connected_comp_forward =
					(float*)malloc(sizeof(float) * _area);
			float* connected_comp_backward =
					(float*)malloc(sizeof(float) * _area);

			_image_slices[slice1].connected_components(
					false, vanishing_comp_forward); // TODO: connectivity?
			_image_slices[slice1].copyfromwork(connected_comp_forward);

			_image_slices[slice2].connected_components(
					false, vanishing_comp_backward); // TODO: connectivity?
			_image_slices[slice2].copyfromwork(connected_comp_backward);

			for (unsigned int i = 0; i < _area; ++i)
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
					_image_slices[slice1].return_tissues(_active_tissuelayer);
			tissues_size_t* tissue2 =
					_image_slices[slice2].return_tissues(_active_tissuelayer);
			for (unsigned int i = 0; i < _area; ++i)
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
			_image_slices[slice1].tissue2work(_active_tissuelayer);
			_image_slices[slice2].tissue2work(_active_tissuelayer);
			_image_slices[slice1].swap_bmpwork();
			_image_slices[slice2].swap_bmpwork();

			// Input images
			f_1 = _image_slices[slice1].return_bmp();
			f_2 = _image_slices[slice2].return_bmp();
		}

		// Interpolation results
		float* g_i = _image_slices[slicehalf].return_work();
		float* gp_i = _image_slices[slicehalf].return_bmp();

		// Initialize g_0 and gp_0
		for (unsigned int i = 0; i < _area; ++i)
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
				_image_slices[slice1].copy2work(g_i, 1);
				_image_slices[slice1].dilation(1, connectivity);
			}
			else
			{
				_image_slices[slice2].copy2work(gp_i, 1);
				_image_slices[slice2].dilation(1, connectivity);
			}
			float* g_i_B = _image_slices[slice1].return_work();
			float* gp_i_B = _image_slices[slice2].return_work();

			// Compute g_i+1 and gp_i+1
			idempotence = true;
			if (iter % 2 == 0)
			{
				for (unsigned int i = 0; i < _area; ++i)
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
				for (unsigned int i = 0; i < _area; ++i)
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
		_image_slices[slicehalf].work2tissue(_active_tissuelayer);

		// Restore images
		_image_slices[slice2].popstack_bmp();
		_image_slices[slicehalf].popstack_bmp();
		_image_slices[slice1].popstack_bmp();
		_image_slices[slice2].popstack_work();
		_image_slices[slicehalf].popstack_work();
		_image_slices[slice1].popstack_work();

		// Recursion
		interpolatetissuegrey_medianset(slice1, slicehalf, connectivity, false);
		interpolatetissuegrey_medianset(slicehalf, slice2, connectivity, false);

		// Restore tissues
		if (handleVanishingComp)
		{
			_image_slices[slice1].copy2tissue(_active_tissuelayer, tissue1_copy);
			_image_slices[slice2].copy2tissue(_active_tissuelayer, tissue2_copy);
			free(tissue1_copy);
			free(tissue2_copy);
		}
	}
}

void SlicesHandler::interpolatetissue(unsigned short slice1, unsigned short slice2,
		tissues_size_t tissuetype, bool connected)
{
	if (slice2 < slice1)
	{
		std::swap(slice1, slice2);
	}
	if (slice1 + 1 >= slice2) // no slices in between
	{
		return;
	}

	const short n = slice2 - slice1;
	if (!connected)
	{
		tissues_size_t* tissue1 = _image_slices[slice1].return_tissues(_active_tissuelayer);
		tissues_size_t* tissue2 = _image_slices[slice2].return_tissues(_active_tissuelayer);
		_image_slices[slice1].pushstack_bmp();
		_image_slices[slice2].pushstack_bmp();
		float* bmp1 = _image_slices[slice1].return_bmp();
		float* bmp2 = _image_slices[slice2].return_bmp();
		for (unsigned int i = 0; i < _area; i++)
		{
			bmp1[i] = (float)tissue1[i];
			bmp2[i] = (float)tissue2[i];
		}

		_image_slices[slice2].dead_reckoning((float)tissuetype);
		_image_slices[slice1].dead_reckoning((float)tissuetype);

		bmp1 = _image_slices[slice1].return_work();
		bmp2 = _image_slices[slice2].return_work();
		const short n = slice2 - slice1;
		Point p;
		float delta;
		unsigned i1 = 0;

		for (p.py = 0; p.py < _height; p.py++)
		{
			for (p.px = 0; p.px < _width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				for (unsigned short j = 1; j < n; j++)
				{
					if (bmp1[i1] + delta * j >= 0)
						_image_slices[slice1 + j].set_work_pt(p, 255.0f);
					else
						_image_slices[slice1 + j].set_work_pt(p, 0.0f);
				}
				i1++;
			}
		}

		for (unsigned i = 0; i < _area; i++)
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
			_image_slices[slice1 + j].set_mode(2, false);
		}

		_image_slices[slice2].popstack_bmp();
		_image_slices[slice1].popstack_bmp();
	}
	else
	{
		SliceHandlerItkWrapper itk_handler(this);
		auto tissues1 = itk_handler.GetTissuesSlice(slice1);
		auto tissues2 = itk_handler.GetTissuesSlice(slice2);

		ConnectedShapeBasedInterpolation interpolator;
		try
		{
			auto interpolated_slices = interpolator.interpolate(tissues1, tissues2, tissuetype, n - 1, true);

			for (short i = 0; i < interpolated_slices.size(); ++i)
			{
				auto slice = interpolated_slices[i];
				const float* source = slice->GetPixelContainer()->GetImportPointer();
				size_t source_len = slice->GetPixelContainer()->Size();
				// copy to target (idx = slice1 + i), slice1 is included
				float* target = _image_slices[slice1 + i].return_work();
				std::copy(source, source + source_len, target);
			}
		}
		catch (std::exception& e)
		{
			ISEG_ERROR("failed to interpolate slices " << e.what());
		}
	}
}

void SlicesHandler::interpolatetissue_medianset(unsigned short slice1,
		unsigned short slice2,
		tissues_size_t tissuetype,
		bool connectivity,
		bool handleVanishingComp)
{
	std::vector<float> mask(tissue_locks().size() + 1, 0.0f);
	mask.at(tissuetype) = 255.0f;

	_image_slices[slice1].tissue2work(_active_tissuelayer, mask);
	_image_slices[slice2].tissue2work(_active_tissuelayer, mask);
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
			_image_slices[origin1].return_tissues(_active_tissuelayer);
	tissues_size_t* tissue2 =
			_image_slices[origin2].return_tissues(_active_tissuelayer);
	_image_slices[origin1].pushstack_bmp();
	_image_slices[origin2].pushstack_bmp();
	_image_slices[origin1].pushstack_work();
	_image_slices[origin2].pushstack_work();
	float* bmp1 = _image_slices[origin1].return_bmp();
	float* bmp2 = _image_slices[origin2].return_bmp();
	for (unsigned int i = 0; i < _area; i++)
	{
		bmp1[i] = (float)tissue1[i];
		bmp2[i] = (float)tissue2[i];
	}

	_image_slices[origin1].dead_reckoning((float)tissuetype);
	_image_slices[origin2].dead_reckoning((float)tissuetype);

	bmp1 = _image_slices[origin1].return_work();
	bmp2 = _image_slices[origin2].return_work();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < _height; p.py++)
		{
			for (p.px = 0; p.px < _width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				if (bmp1[i1] + delta * (target - origin1) >= 0)
					_image_slices[target].set_work_pt(p, 255.0f);
				else
					_image_slices[target].set_work_pt(p, 0.0f);
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

	_image_slices[target].set_mode(2, false);

	_image_slices[origin2].popstack_work();
	_image_slices[origin1].popstack_work();
	_image_slices[origin2].popstack_bmp();
	_image_slices[origin1].popstack_bmp();
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
	_image_slices[slice1].pushstack_bmp();
	_image_slices[slice2].pushstack_bmp();
	float* bmp1 = _image_slices[slice1].return_bmp();
	float* bmp2 = _image_slices[slice2].return_bmp();
	float* work1 = _image_slices[slice1].return_work();
	float* work2 = _image_slices[slice2].return_work();

	for (unsigned int i = 0; i < _area; i++)
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

	_image_slices[slice2].dead_reckoning(255.0f);
	_image_slices[slice1].dead_reckoning(255.0f);

	bmp1 = _image_slices[slice1].return_work();
	bmp2 = _image_slices[slice2].return_work();
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < _height; p.py++)
		{
			for (p.px = 0; p.px < _width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				for (unsigned short j = 1; j < n; j++)
				{
					if (bmp1[i1] + delta * j >= 0)
						_image_slices[slice1 + j].set_work_pt(p, 255.0f);
					else
						_image_slices[slice1 + j].set_work_pt(p, 0.0f);
				}
				i1++;
			}
		}
	}

	for (unsigned i = 0; i < _area; i++)
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
		_image_slices[slice1 + j].set_mode(2, false);
	}

	_image_slices[slice2].popstack_bmp();
	_image_slices[slice1].popstack_bmp();
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
	_image_slices[origin1].pushstack_bmp();
	_image_slices[origin2].pushstack_bmp();
	_image_slices[origin1].pushstack_work();
	_image_slices[origin2].pushstack_work();
	float* bmp1 = _image_slices[origin1].return_bmp();
	float* bmp2 = _image_slices[origin2].return_bmp();
	float* work1 = _image_slices[origin1].return_work();
	float* work2 = _image_slices[origin2].return_work();

	for (unsigned int i = 0; i < _area; i++)
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

	_image_slices[origin2].dead_reckoning(255.0f);
	_image_slices[origin1].dead_reckoning(255.0f);

	bmp1 = _image_slices[origin1].return_work();
	bmp2 = _image_slices[origin2].return_work();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < _height; p.py++)
		{
			for (p.px = 0; p.px < _width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				if (bmp1[i1] + delta * (target - origin1) >= 0)
					_image_slices[target].set_work_pt(p, 255.0f);
				else
					_image_slices[target].set_work_pt(p, 0.0f);
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

	_image_slices[target].set_mode(2, false);

	_image_slices[origin2].popstack_work();
	_image_slices[origin1].popstack_work();
	_image_slices[origin2].popstack_bmp();
	_image_slices[origin1].popstack_bmp();
}

void SlicesHandler::interpolate(unsigned short slice1, unsigned short slice2)
{
	float* bmp1 = _image_slices[slice1].return_work();
	float* bmp2 = _image_slices[slice2].return_work();
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < _height; p.py++)
		{
			for (p.px = 0; p.px < _width; p.px++)
			{
				delta = (bmp2[i] - bmp1[i]) / n;
				for (unsigned short j = 1; j < n; j++)
					_image_slices[slice1 + j].set_work_pt(p,
							bmp1[i] + delta * j);
				i++;
			}
		}
	}

	for (unsigned short j = 1; j < n; j++)
	{
		_image_slices[slice1 + j].set_mode(1, false);
	}

	return;
}

void SlicesHandler::extrapolate(unsigned short origin1, unsigned short origin2,
		unsigned short target)
{
	float* bmp1 = _image_slices[origin1].return_work();
	float* bmp2 = _image_slices[origin2].return_work();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i = 0;

	for (p.py = 0; p.py < _height; p.py++)
	{
		for (p.px = 0; p.px < _width; p.px++)
		{
			delta = (bmp2[i] - bmp1[i]) / n;
			_image_slices[target].set_work_pt(p, bmp1[i] +
																							 delta * (target - origin1));
			i++;
		}
	}

	_image_slices[target].set_mode(1, false);

	return;
}

void SlicesHandler::interpolate(unsigned short slice1, unsigned short slice2,
		float* bmp1, float* bmp2)
{
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i = 0;

	for (p.py = 0; p.py < _height; p.py++)
	{
		for (p.px = 0; p.px < _width; p.px++)
		{
			delta = (bmp2[i] - bmp1[i]) / n;
			for (unsigned short j = 0; j <= n; j++)
				_image_slices[slice1 + j].set_work_pt(p, bmp1[i] + delta * j);
			i++;
		}
	}

	for (unsigned short j = 1; j < n; j++)
	{
		_image_slices[slice1 + j].set_mode(1, false);
	}

	return;
}

void SlicesHandler::set_slicethickness(float t)
{
	_thickness = t;
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		_os.set_thickness(t, i);
	}

	return;
}

float SlicesHandler::get_slicethickness() { return _thickness; }

void SlicesHandler::set_pixelsize(float dx1, float dy1)
{
	_dx = dx1;
	_dy = dy1;
	_os.set_pixelsize(_dx, _dy);
	return;
}

Pair SlicesHandler::get_pixelsize()
{
	Pair p;
	p.high = _dx;
	p.low = _dy;
	return p;
}

Transform SlicesHandler::transform() const { return _transform; }

Transform SlicesHandler::get_transform_active_slices() const
{
	int plo[3] = {0, 0, -static_cast<int>(_startslice)};
	float spacing[3] = {_dx, _dy, _thickness};

	Transform tr_corrected(_transform);
	tr_corrected.paddingUpdateTransform(plo, spacing);
	return tr_corrected;
}

void SlicesHandler::set_transform(const Transform& tr) { _transform = tr; }

Vec3 SlicesHandler::spacing() const
{
	return Vec3(_dx, _dy, _thickness);
}

void SlicesHandler::get_displacement(float disp[3]) const
{
	_transform.getOffset(disp);
}

void SlicesHandler::set_displacement(const float disp[3])
{
	_transform.setOffset(disp);
}

void SlicesHandler::get_direction_cosines(float dc[6]) const
{
	for (unsigned short i = 0; i < 3; i++)
	{
		dc[i] = _transform[i][0];
		dc[i + 3] = _transform[i][1];
	}
}

void SlicesHandler::set_direction_cosines(const float dc[6])
{
	float offset[3];
	_transform.getOffset(offset);
	_transform.setTransform(offset, dc);
}

void SlicesHandler::slicebmp_x(float* return_bits, unsigned short xcoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		dummy = _image_slices[i].return_bmp();
		for (unsigned short j = 0; j < _height; j++)
		{
			return_bits[n] = dummy[j * _width + xcoord];
			n++;
		}
	}

	return;
}

void SlicesHandler::slicebmp_y(float* return_bits, unsigned short ycoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		dummy = _image_slices[i].return_bmp();
		for (unsigned short j = 0; j < _width; j++)
		{
			return_bits[n] = dummy[j + ycoord * _width];
			n++;
		}
	}

	return;
}

float* SlicesHandler::slicebmp_x(unsigned short xcoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(_height) * _nrslices);
	slicebmp_x(result, xcoord);

	return result;
}

float* SlicesHandler::slicebmp_y(unsigned short ycoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(_width) * _nrslices);
	slicebmp_y(result, ycoord);

	return result;
}

void SlicesHandler::slicework_x(float* return_bits, unsigned short xcoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		dummy = _image_slices[i].return_work();
		for (unsigned short j = 0; j < _height; j++)
		{
			return_bits[n] = dummy[j * _width + xcoord];
			n++;
		}
	}

	return;
}

void SlicesHandler::slicework_y(float* return_bits, unsigned short ycoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		dummy = _image_slices[i].return_work();
		for (unsigned short j = 0; j < _width; j++)
		{
			return_bits[n] = dummy[j + ycoord * _width];
			n++;
		}
	}

	return;
}

float* SlicesHandler::slicework_x(unsigned short xcoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(_height) * _nrslices);
	slicework_x(result, xcoord);

	return result;
}

float* SlicesHandler::slicework_y(unsigned short ycoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(_width) * _nrslices);
	slicework_y(result, ycoord);

	return result;
}

void SlicesHandler::slicetissue_x(tissues_size_t* return_bits,
		unsigned short xcoord)
{
	unsigned n = 0;
	tissues_size_t* dummy;

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		dummy = _image_slices[i].return_tissues(_active_tissuelayer);
		for (unsigned short j = 0; j < _height; j++)
		{
			return_bits[n] = dummy[j * _width + xcoord];
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

	for (unsigned short i = 0; i < _nrslices; i++)
	{
		dummy = _image_slices[i].return_tissues(_active_tissuelayer);
		for (unsigned short j = 0; j < _width; j++)
		{
			return_bits[n] = dummy[j + ycoord * _width];
			n++;
		}
	}

	return;
}

tissues_size_t* SlicesHandler::slicetissue_x(unsigned short xcoord)
{
	tissues_size_t* result = (tissues_size_t*)malloc(
			sizeof(tissues_size_t) * unsigned(_height) * _nrslices);
	slicetissue_x(result, xcoord);

	return result;
}

tissues_size_t* SlicesHandler::slicetissue_y(unsigned short ycoord)
{
	tissues_size_t* result = (tissues_size_t*)malloc(
			sizeof(tissues_size_t) * unsigned(_width) * _nrslices);
	slicetissue_y(result, ycoord);

	return result;
}

void SlicesHandler::slicework_z(unsigned short slicenr)
{
	_image_slices[slicenr].return_work();
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
	ISEG_INFO_MSG("SlicesHandler::extract_tissue_surfaces");
	ISEG_INFO("\tratio " << ratio);
	ISEG_INFO("\tsmoothingiterations " << smoothingiterations);
	ISEG_INFO("\tfeatureAngle " << featureAngle);
	ISEG_INFO("\tusediscretemc " << usediscretemc);

	//
	// Copy label field into a vtkImageData object
	//
	const char* tissueIndexArrayName = "Domain";		 // this can be changed
	const char* tissueNameArrayName = "TissueNames"; // don't modify this
	const char* tissueColorArrayName = "Colors";		 // don't modify this

	vtkSmartPointer<vtkImageData> labelField =
			vtkSmartPointer<vtkImageData>::New();
	labelField->SetExtent(0, (int)width() + 1, 0,
			(int)height() + 1, 0,
			(int)(_endslice - _startslice) + 1);
	Pair ps = get_pixelsize();
	labelField->SetSpacing(ps.high, ps.low, get_slicethickness());
	// transform (translation and rotation) is applied at end of function
	labelField->SetOrigin(0, 0, 0);
	labelField->AllocateScalars(GetScalarType<tissues_size_t>(), 1);
	vtkDataArray* arr = labelField->GetPointData()->GetScalars();
	if (!arr)
	{
		ISEG_ERROR_MSG("no scalars");
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
		check_equal(TissueInfos::GetTissueType(TissueInfos::GetTissueName(i)), i);
		names_array->SetValue(i, TissueInfos::GetTissueName(i).c_str());
		auto color = TissueInfos::GetTissueColor(i);
		color_array->SetTuple(i, color.v.data());
	}

	labelField->GetFieldData()->AddArray(names_array);
	labelField->GetFieldData()->AddArray(color_array);

	tissues_size_t* field = (tissues_size_t*)labelField->GetScalarPointer(0, 0, 0);
	if (!field)
	{
		ISEG_ERROR_MSG("null pointer");
		return -1;
	}

	int const padding = 1;
	//labelField is already cropped
	unsigned short nrSlice = 0;
	for (unsigned short i = _startslice; i < _endslice; i++, nrSlice++)
	{
		copyfromtissuepadded(
				i,
				&(field[(nrSlice + 1) * (unsigned long long)(width() + 2) *
								(height() + 2)]),
				padding);
	}
	for (unsigned long long i = 0;
			 i < (unsigned long long)(width() + 2) * (height() + 2);
			 i++)
		field[i] = 0;
	for (unsigned long long i = (_endslice - _startslice + 1) *
															(unsigned long long)(width() + 2) *
															(height() + 2);
			 i < (_endslice - _startslice + 2) *
							 (unsigned long long)(width() + 2) *
							 (height() + 2);
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
	vtkSmartPointer<vtkImageExtractCompatibleMesher> contour =
			vtkSmartPointer<vtkImageExtractCompatibleMesher>::New();
	vtkSmartPointer<vtkDiscreteMarchingCubes> cubes =
			vtkSmartPointer<vtkDiscreteMarchingCubes>::New();
	vtkSmartPointer<vtkWindowedSincPolyDataFilter> smoother =
			vtkSmartPointer<vtkWindowedSincPolyDataFilter>::New();
	vtkSmartPointer<vtkEdgeCollapse> simplify =
			vtkSmartPointer<vtkEdgeCollapse>::New();

	vtkPolyData* output = nullptr;
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
				check_equal(TissueInfos::GetTissueType(TissueInfos::GetTissueName(i)), i);
				if (i == tissuevec[0])
				{
					names_array_1->SetValue(0, TissueInfos::GetTissueName(i).c_str());
					auto color = TissueInfos::GetTissueColor(i);
					color_array_1->SetTuple(0, color.v.data());
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
			-padding * _thickness +
					_thickness * _startslice};
	for (int i = 0; i < 3; i++)
	{
		elems[i * 4 + 0] = _transform[i][0];
		elems[i * 4 + 1] = _transform[i][1];
		elems[i * 4 + 2] = _transform[i][2];
		elems[i * 4 + 3] = _transform[i][3] + padding_displacement[i];
	}

	vtkNew<vtkTransform> transform;
	transform->SetMatrix(elems.data());

	vtkNew<vtkTransformPolyDataFilter> transform_filter;
	transform_filter->SetInputData(output);
	transform_filter->SetTransform(transform.Get());

	//
	// Write output to file
	//
	vtkNew<vtkGenericDataSetWriter> writer;
	writer->SetFileName(filename.toAscii().data());
	writer->SetInputConnection(transform_filter->GetOutputPort());
	writer->SetMaterialArrayName(tissueIndexArrayName);
	writer->Write();

	return error_counter;
}

void SlicesHandler::add2tissue(tissues_size_t tissuetype, Point p, bool override)
{
	_image_slices[_activeslice].add2tissue(_active_tissuelayer, tissuetype, p, override);
}

void SlicesHandler::add2tissue(tissues_size_t tissuetype, bool* mask, unsigned short slicenr, bool override)
{
	_image_slices[slicenr].add2tissue(_active_tissuelayer, tissuetype, mask, override);
}

void SlicesHandler::add2tissueall(tissues_size_t tissuetype, Point p, bool override)
{
	float f = _image_slices[_activeslice].work_pt(p);
	add2tissueall(tissuetype, f, override);
}

void SlicesHandler::add2tissue_connected(tissues_size_t tissuetype, Point p, bool override)
{
	_image_slices[_activeslice].add2tissue_connected(_active_tissuelayer,
			tissuetype, p, override);
}

void SlicesHandler::add2tissue_thresh(tissues_size_t tissuetype, Point p)
{
	_image_slices[_activeslice].add2tissue_thresh(_active_tissuelayer, tissuetype, p);
}

void SlicesHandler::subtract_tissue(tissues_size_t tissuetype, Point p)
{
	_image_slices[_activeslice].subtract_tissue(_active_tissuelayer, tissuetype, p);
}

void SlicesHandler::subtract_tissueall(tissues_size_t tissuetype, Point p)
{
	float f = _image_slices[_activeslice].work_pt(p);
	subtract_tissueall(tissuetype, f);
}

void SlicesHandler::subtract_tissueall(tissues_size_t tissuetype, Point p,
		unsigned short slicenr)
{
	if (slicenr >= 0 && slicenr < _nrslices)
	{
		float f = _image_slices[slicenr].work_pt(p);
		subtract_tissueall(tissuetype, f);
	}
}

void SlicesHandler::subtract_tissueall(tissues_size_t tissuetype, float f)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].subtract_tissue(_active_tissuelayer, tissuetype, f);
	}
}

void SlicesHandler::subtract_tissue_connected(tissues_size_t tissuetype,
		Point p)
{
	_image_slices[_activeslice].subtract_tissue_connected(_active_tissuelayer,
			tissuetype, p);
}

void SlicesHandler::selectedtissue2work(const std::vector<tissues_size_t>& tissuetype)
{
	std::vector<float> mask(tissue_locks().size() + 1, 0.0f);
	for (auto label : tissuetype)
	{
		mask.at(label) = 255.0f;
	}

	_image_slices[_activeslice].tissue2work(_active_tissuelayer, mask);
}

void SlicesHandler::selectedtissue2work3D(const std::vector<tissues_size_t>& tissuetype)
{
	std::vector<float> mask(tissue_locks().size() + 1, 0.0f);
	for (auto label : tissuetype)
	{
		mask.at(label) = 255.0f;
	}

	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].tissue2work(_active_tissuelayer, mask);
	}
}

void SlicesHandler::cleartissue(tissues_size_t tissuetype)
{
	_image_slices[_activeslice].cleartissue(_active_tissuelayer, tissuetype);
}

void SlicesHandler::cleartissue3D(tissues_size_t tissuetype)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].cleartissue(_active_tissuelayer, tissuetype);
	}
}

void SlicesHandler::cleartissues()
{
	_image_slices[_activeslice].cleartissues(_active_tissuelayer);
}

void SlicesHandler::cleartissues3D()
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].cleartissues(_active_tissuelayer);
	}
}

void SlicesHandler::add2tissueall(tissues_size_t tissuetype, Point p,
		unsigned short slicenr, bool override)
{
	if (slicenr >= 0 && slicenr < _nrslices)
	{
		float f = _image_slices[slicenr].work_pt(p);
		add2tissueall(tissuetype, f, override);
	}

	return;
}

void SlicesHandler::add2tissueall(tissues_size_t tissuetype, float f,
		bool override)
{
	int const iN = _endslice;

#pragma omp parallel for
	for (int i = _startslice; i < iN; i++)
	{
		_image_slices[i].add2tissue(_active_tissuelayer, tissuetype, f, override);
	}
}

void SlicesHandler::next_slice() { set_active_slice(_activeslice + 1); }

void SlicesHandler::prev_slice()
{
	if (_activeslice > 0)
		set_active_slice(_activeslice - 1);
}

unsigned short SlicesHandler::get_next_featuring_slice(tissues_size_t type,
		bool& found)
{
	found = true;
	for (unsigned i = _activeslice + 1; i < _nrslices; i++)
	{
		if (_image_slices[i].has_tissue(_active_tissuelayer, type))
		{
			return i;
		}
	}
	for (unsigned i = 0; i <= _activeslice; i++)
	{
		if (_image_slices[i].has_tissue(_active_tissuelayer, type))
		{
			return i;
		}
	}

	found = false;
	return _activeslice;
}

unsigned short SlicesHandler::active_slice() const { return _activeslice; }

void SlicesHandler::set_active_slice(unsigned short slice, bool signal_change)
{
	if (slice < _nrslices && slice != _activeslice)
	{
		_activeslice = slice;

		// notify observers that slice changed
		if (signal_change)
		{
			on_active_slice_changed(slice);
		}
	}
}

bmphandler* SlicesHandler::get_activebmphandler()
{
	return &(_image_slices[_activeslice]);
}

tissuelayers_size_t SlicesHandler::active_tissuelayer() const
{
	return _active_tissuelayer;
}

void SlicesHandler::set_active_tissuelayer(tissuelayers_size_t idx)
{
	// TODO: Signaling, range checking
	_active_tissuelayer = idx;
}

unsigned SlicesHandler::pushstack_bmp()
{
	return get_activebmphandler()->pushstack_bmp();
}

unsigned SlicesHandler::pushstack_bmp(unsigned int slice)
{
	return _image_slices[slice].pushstack_bmp();
}

unsigned SlicesHandler::pushstack_work()
{
	return get_activebmphandler()->pushstack_work();
}

unsigned SlicesHandler::pushstack_work(unsigned int slice)
{
	return _image_slices[slice].pushstack_work();
}

unsigned SlicesHandler::pushstack_tissue(tissues_size_t i)
{
	return get_activebmphandler()->pushstack_tissue(_active_tissuelayer, i);
}

unsigned SlicesHandler::pushstack_tissue(tissues_size_t i, unsigned int slice)
{
	return _image_slices[slice].pushstack_tissue(_active_tissuelayer, i);
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
	_image_slices[slice].getstack_bmp(i);
}

void SlicesHandler::getstack_work(unsigned i)
{
	get_activebmphandler()->getstack_work(i);
}

void SlicesHandler::getstack_work(unsigned int slice, unsigned i)
{
	_image_slices[slice].getstack_work(i);
}

void SlicesHandler::getstack_help(unsigned i)
{
	get_activebmphandler()->getstack_help(i);
}

void SlicesHandler::getstack_tissue(unsigned i, tissues_size_t tissuenr,
		bool override)
{
	get_activebmphandler()->getstack_tissue(_active_tissuelayer, i, tissuenr,
			override);
}

void SlicesHandler::getstack_tissue(unsigned int slice, unsigned i,
		tissues_size_t tissuenr, bool override)
{
	_image_slices[slice].getstack_tissue(_active_tissuelayer, i, tissuenr,
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
	if (_uelem == nullptr)
	{
		_uelem = new UndoElem;
		_uelem->dataSelection = dataSelection;

		if (dataSelection.bmp)
		{
			_uelem->bmp_old = _image_slices[dataSelection.sliceNr].copy_bmp();
			_uelem->mode1_old = _image_slices[dataSelection.sliceNr].return_mode(true);
		}
		else
		{
			_uelem->bmp_old = nullptr;
		}

		if (dataSelection.work)
		{
			_uelem->work_old = _image_slices[dataSelection.sliceNr].copy_work();
			_uelem->mode2_old = _image_slices[dataSelection.sliceNr].return_mode(false);
		}
		else
		{
			_uelem->work_old = nullptr;
		}

		if (dataSelection.tissues)
		{
			_uelem->tissue_old = _image_slices[dataSelection.sliceNr].copy_tissue(_active_tissuelayer);
		}
		else
		{
			_uelem->tissue_old = nullptr;
		}

		_uelem->vvm_old.clear();
		if (dataSelection.vvm)
		{
			_uelem->vvm_old = *(_image_slices[dataSelection.sliceNr].return_vvm());
		}

		_uelem->limits_old.clear();
		if (dataSelection.limits)
		{
			_uelem->limits_old = *(_image_slices[dataSelection.sliceNr].return_limits());
		}

		_uelem->marks_old.clear();
		if (dataSelection.marks)
		{
			_uelem->marks_old = *(_image_slices[dataSelection.sliceNr].return_marks());
		}
	}
}

bool SlicesHandler::start_undoall(iseg::DataSelection& dataSelection)
{
	//abcd std::vector<unsigned short> vslicenr1;
	std::vector<unsigned> vslicenr1;
	for (unsigned i = 0; i < _nrslices; i++)
		vslicenr1.push_back(i);

	return start_undo(dataSelection, vslicenr1);
}

//abcd bool SlicesHandler::start_undo(common::DataSelection &dataSelection,std::vector<unsigned short> vslicenr1)
bool SlicesHandler::start_undo(iseg::DataSelection& dataSelection,
		std::vector<unsigned> vslicenr1)
{
	if (_uelem == nullptr)
	{
		MultiUndoElem* uelem1 = new MultiUndoElem;
		_uelem = uelem1;
		//		uelem=new MultiUndoElem;
		//		MultiUndoElem *uelem1=dynamic_cast<MultiUndoElem *>(uelem);
		_uelem->dataSelection = dataSelection;
		uelem1->vslicenr = vslicenr1;

		if (_uelem->arraynr() < this->_undoQueue.return_nrundoarraysmax())
		{
			//abcd std::vector<unsigned short>::iterator it;
			std::vector<unsigned>::iterator it;
			uelem1->vbmp_old.clear();
			uelem1->vmode1_old.clear();
			if (dataSelection.bmp)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
				{
					uelem1->vbmp_old.push_back(_image_slices[*it].copy_bmp());
					uelem1->vmode1_old.push_back(
							_image_slices[*it].return_mode(true));
				}
			uelem1->vwork_old.clear();
			uelem1->vmode2_old.clear();
			if (dataSelection.work)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
				{
					uelem1->vwork_old.push_back(_image_slices[*it].copy_work());
					uelem1->vmode2_old.push_back(
							_image_slices[*it].return_mode(false));
				}
			uelem1->vtissue_old.clear();
			if (dataSelection.tissues)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->vtissue_old.push_back(
							_image_slices[*it].copy_tissue(_active_tissuelayer));
			uelem1->vvvm_old.clear();
			if (dataSelection.vvm)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->vvvm_old.push_back(
							*(_image_slices[*it].return_vvm()));
			uelem1->vlimits_old.clear();
			if (dataSelection.limits)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->vlimits_old.push_back(
							*(_image_slices[*it].return_limits()));
			uelem1->marks_old.clear();
			if (dataSelection.marks)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->vmarks_old.push_back(
							*(_image_slices[*it].return_marks()));

			return true;
		}
		else
		{
			free(_uelem);
			_uelem = nullptr;
		}
	}

	return false;
}

void SlicesHandler::abort_undo()
{
	if (_uelem != nullptr)
	{
		free(_uelem);
		_uelem = nullptr;
	}
}

void SlicesHandler::end_undo()
{
	if (_uelem != nullptr)
	{
		if (_uelem->multi)
		{
			//unsigned short undotype=uelem->storetype;

			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(_uelem);

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

			this->_undoQueue.add_undo(uelem1);

			_uelem = nullptr;
		}
		else
		{
			//unsigned short undotype=uelem->storetype;
			//unsigned short slicenr1=uelem->slicenr;

			//			if(undotype & 1) uelem->bmp_new=image_slices[slicenr1].copy_bmp();
			//			else
			_uelem->bmp_new = nullptr;
			_uelem->mode1_new = 0;

			//			if(undotype & 2) uelem->work_new=image_slices[slicenr1].copy_work();
			//			else
			_uelem->work_new = nullptr;
			_uelem->mode2_new = 0;

			//			if(undotype & 4) uelem->tissue_new=image_slices[slicenr1].copy_tissue(active_tissuelayer);
			//			else
			_uelem->tissue_new = nullptr;

			_uelem->vvm_new.clear();
			//			if(undotype & 8) uelem->vvm_new=*(image_slices[slicenr1].return_vvm());

			_uelem->limits_new.clear();
			//			if(undotype & 16) uelem->limits_new=*(image_slices[slicenr1].return_limits());

			_uelem->marks_new.clear();
			//			if(undotype & 32) uelem->marks_new=*(image_slices[slicenr1].return_marks());

			this->_undoQueue.add_undo(_uelem);

			_uelem = nullptr;
		}
	}
}

void SlicesHandler::merge_undo()
{
	if (_uelem != nullptr && !_uelem->multi)
	{
		iseg::DataSelection dataSelection = _uelem->dataSelection;
		/*		unsigned short slicenr1=uelem->slicenr;

		if(undotype & 1) uelem->bmp_new=image_slices[slicenr1].copy_bmp();
		else uelem->bmp_new=nullptr;

		if(undotype & 2) uelem->work_new=image_slices[slicenr1].copy_work();
		else uelem->work_new=nullptr;

		if(undotype & 4) uelem->tissue_new=image_slices[slicenr1].copy_tissue(active_tissuelayer);
		else uelem->tissue_new=nullptr;

		uelem->vvm_new.clear();
		if(undotype & 8) uelem->vvm_new=*(image_slices[slicenr1].return_vvm());

		uelem->limits_new.clear();
		if(undotype & 16) uelem->limits_new=*(image_slices[slicenr1].return_limits());

		uelem->marks_new.clear();
		if(undotype & 32) uelem->marks_new=*(image_slices[slicenr1].return_marks());*/

		this->_undoQueue.merge_undo(_uelem);

		if (dataSelection.bmp)
		{
			_uelem->bmp_new = nullptr;
			_uelem->bmp_old = nullptr;
		}
		if (dataSelection.work)
		{
			_uelem->work_new = nullptr;
			_uelem->work_old = nullptr;
		}
		if (dataSelection.tissues)
		{
			_uelem->tissue_new = nullptr;
			_uelem->tissue_old = nullptr;
		}

		delete _uelem;
		_uelem = nullptr;
	}
}

iseg::DataSelection SlicesHandler::undo()
{
	if (_uelem == nullptr)
	{
		_uelem = this->_undoQueue.undo();
		if (_uelem->multi)
		{
			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(_uelem);

			if (uelem1 != nullptr)
			{
				unsigned short current_slice;
				iseg::DataSelection dataSelection = _uelem->dataSelection;

				for (unsigned i = 0; i < uelem1->vslicenr.size(); i++)
				{
					current_slice = uelem1->vslicenr[i];
					if (dataSelection.bmp)
					{
						uelem1->vbmp_new.push_back(
								_image_slices[current_slice].copy_bmp());
						uelem1->vmode1_new.push_back(
								_image_slices[current_slice].return_mode(true));
						_image_slices[current_slice].copy2bmp(
								uelem1->vbmp_old[i], uelem1->vmode1_old[i]);
						free(uelem1->vbmp_old[i]);
					}
					if (dataSelection.work)
					{
						uelem1->vwork_new.push_back(
								_image_slices[current_slice].copy_work());
						uelem1->vmode2_new.push_back(
								_image_slices[current_slice].return_mode(false));
						_image_slices[current_slice].copy2work(
								uelem1->vwork_old[i], uelem1->vmode2_old[i]);
						free(uelem1->vwork_old[i]);
					}
					if (dataSelection.tissues)
					{
						uelem1->vtissue_new.push_back(
								_image_slices[current_slice].copy_tissue(
										_active_tissuelayer));
						_image_slices[current_slice].copy2tissue(
								_active_tissuelayer, uelem1->vtissue_old[i]);
						free(uelem1->vtissue_old[i]);
					}
					if (dataSelection.vvm)
					{
						uelem1->vvvm_new.push_back(
								*(_image_slices[current_slice].return_vvm()));
						_image_slices[current_slice].copy2vvm(
								&(uelem1->vvvm_old[i]));
					}
					if (dataSelection.limits)
					{
						uelem1->vlimits_new.push_back(
								*(_image_slices[current_slice].return_limits()));
						_image_slices[current_slice].copy2limits(
								&(uelem1->vlimits_old[i]));
					}
					if (dataSelection.marks)
					{
						uelem1->vmarks_new.push_back(
								*(_image_slices[current_slice].return_marks()));
						_image_slices[current_slice].copy2marks(
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

				_uelem = nullptr;

				return dataSelection;
			}
			else
				return iseg::DataSelection();
		}
		else
		{
			if (_uelem != nullptr)
			{
				iseg::DataSelection dataSelection = _uelem->dataSelection;

				if (dataSelection.bmp)
				{
					_uelem->bmp_new =
							_image_slices[dataSelection.sliceNr].copy_bmp();
					_uelem->mode1_new =
							_image_slices[dataSelection.sliceNr].return_mode(true);
					_image_slices[dataSelection.sliceNr].copy2bmp(
							_uelem->bmp_old, _uelem->mode1_old);
					free(_uelem->bmp_old);
					_uelem->bmp_old = nullptr;
				}

				if (dataSelection.work)
				{
					_uelem->work_new =
							_image_slices[dataSelection.sliceNr].copy_work();
					_uelem->mode2_new =
							_image_slices[dataSelection.sliceNr].return_mode(false);
					_image_slices[dataSelection.sliceNr].copy2work(
							_uelem->work_old, _uelem->mode2_old);
					free(_uelem->work_old);
					_uelem->work_old = nullptr;
				}

				if (dataSelection.tissues)
				{
					_uelem->tissue_new =
							_image_slices[dataSelection.sliceNr].copy_tissue(
									_active_tissuelayer);
					_image_slices[dataSelection.sliceNr].copy2tissue(
							_active_tissuelayer, _uelem->tissue_old);
					free(_uelem->tissue_old);
					_uelem->tissue_old = nullptr;
				}

				if (dataSelection.vvm)
				{
					_uelem->vvm_new.clear();
					_uelem->vvm_new =
							*(_image_slices[dataSelection.sliceNr].return_vvm());
					_image_slices[dataSelection.sliceNr].copy2vvm(
							&_uelem->vvm_old);
					_uelem->vvm_old.clear();
				}

				if (dataSelection.limits)
				{
					_uelem->limits_new.clear();
					_uelem->limits_new =
							*(_image_slices[dataSelection.sliceNr].return_limits());
					_image_slices[dataSelection.sliceNr].copy2limits(
							&_uelem->limits_old);
					_uelem->limits_old.clear();
				}

				if (dataSelection.marks)
				{
					_uelem->marks_new.clear();
					_uelem->marks_new =
							*(_image_slices[dataSelection.sliceNr].return_marks());
					_image_slices[dataSelection.sliceNr].copy2marks(
							&_uelem->marks_old);
					_uelem->marks_old.clear();
				}

				set_active_slice(dataSelection.sliceNr);

				_uelem = nullptr;

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
	if (_uelem == nullptr)
	{
		_uelem = this->_undoQueue.redo();
		if (_uelem == nullptr)
			return iseg::DataSelection();
		if (_uelem->multi)
		{
			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(_uelem);

			if (uelem1 != nullptr)
			{
				unsigned short current_slice;
				iseg::DataSelection dataSelection = _uelem->dataSelection;
				for (unsigned i = 0; i < uelem1->vslicenr.size(); i++)
				{
					current_slice = uelem1->vslicenr[i];
					if (dataSelection.bmp)
					{
						uelem1->vbmp_old.push_back(
								_image_slices[current_slice].copy_bmp());
						uelem1->vmode1_old.push_back(
								_image_slices[current_slice].return_mode(true));
						_image_slices[current_slice].copy2bmp(
								uelem1->vbmp_new[i], uelem1->vmode1_new[i]);
						free(uelem1->vbmp_new[i]);
					}
					if (dataSelection.work)
					{
						uelem1->vwork_old.push_back(
								_image_slices[current_slice].copy_work());
						uelem1->vmode2_old.push_back(
								_image_slices[current_slice].return_mode(false));
						_image_slices[current_slice].copy2work(
								uelem1->vwork_new[i], uelem1->vmode2_new[i]);
						free(uelem1->vwork_new[i]);
					}
					if (dataSelection.tissues)
					{
						uelem1->vtissue_old.push_back(
								_image_slices[current_slice].copy_tissue(
										_active_tissuelayer));
						_image_slices[current_slice].copy2tissue(
								_active_tissuelayer, uelem1->vtissue_new[i]);
						free(uelem1->vtissue_new[i]);
					}
					if (dataSelection.vvm)
					{
						uelem1->vvvm_old.push_back(
								*(_image_slices[current_slice].return_vvm()));
						_image_slices[current_slice].copy2vvm(
								&(uelem1->vvvm_new[i]));
					}
					if (dataSelection.limits)
					{
						uelem1->vlimits_old.push_back(
								*(_image_slices[current_slice].return_limits()));
						_image_slices[current_slice].copy2limits(
								&(uelem1->vlimits_new[i]));
					}
					if (dataSelection.marks)
					{
						uelem1->vmarks_old.push_back(
								*(_image_slices[current_slice].return_marks()));
						_image_slices[current_slice].copy2marks(
								&(uelem1->vmarks_new[i]));
					}
				}
				uelem1->vbmp_new.clear();
				uelem1->vwork_new.clear();
				uelem1->vtissue_new.clear();
				uelem1->vvvm_new.clear();
				uelem1->vlimits_new.clear();
				uelem1->vmarks_new.clear();

				_uelem = nullptr;

				return dataSelection;
			}
			else
				return iseg::DataSelection();
		}
		else
		{
			if (_uelem != nullptr)
			{
				iseg::DataSelection dataSelection = _uelem->dataSelection;

				if (dataSelection.bmp)
				{
					_uelem->bmp_old =
							_image_slices[dataSelection.sliceNr].copy_bmp();
					_uelem->mode1_old =
							_image_slices[dataSelection.sliceNr].return_mode(true);
					_image_slices[dataSelection.sliceNr].copy2bmp(
							_uelem->bmp_new, _uelem->mode1_new);
					free(_uelem->bmp_new);
					_uelem->bmp_new = nullptr;
				}

				if (dataSelection.work)
				{
					_uelem->work_old =
							_image_slices[dataSelection.sliceNr].copy_work();
					_uelem->mode2_old =
							_image_slices[dataSelection.sliceNr].return_mode(false);
					_image_slices[dataSelection.sliceNr].copy2work(
							_uelem->work_new, _uelem->mode2_new);
					free(_uelem->work_new);
					_uelem->work_new = nullptr;
				}

				if (dataSelection.tissues)
				{
					_uelem->tissue_old =
							_image_slices[dataSelection.sliceNr].copy_tissue(
									_active_tissuelayer);
					_image_slices[dataSelection.sliceNr].copy2tissue(
							_active_tissuelayer, _uelem->tissue_new);
					free(_uelem->tissue_new);
					_uelem->tissue_new = nullptr;
				}

				if (dataSelection.vvm)
				{
					_uelem->vvm_old.clear();
					_uelem->vvm_old =
							*(_image_slices[dataSelection.sliceNr].return_vvm());
					_image_slices[dataSelection.sliceNr].copy2vvm(
							&_uelem->vvm_new);
					_uelem->vvm_new.clear();
				}

				if (dataSelection.limits)
				{
					_uelem->limits_old.clear();
					_uelem->limits_old =
							*(_image_slices[dataSelection.sliceNr].return_limits());
					_image_slices[dataSelection.sliceNr].copy2limits(
							&_uelem->limits_new);
					_uelem->limits_new.clear();
				}

				if (dataSelection.marks)
				{
					_uelem->marks_old.clear();
					_uelem->marks_old =
							*(_image_slices[dataSelection.sliceNr].return_marks());
					_image_slices[dataSelection.sliceNr].copy2marks(
							&_uelem->marks_new);
					_uelem->marks_new.clear();
				}

				set_active_slice(dataSelection.sliceNr);

				_uelem = nullptr;

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
	this->_undoQueue.clear_undo();
	if (_uelem != nullptr)
		delete (_uelem);
	_uelem = nullptr;

	return;
}

void SlicesHandler::reverse_undosliceorder()
{
	this->_undoQueue.reverse_undosliceorder(_nrslices);
	if (_uelem != nullptr)
	{
		_uelem->dataSelection.sliceNr =
				_nrslices - 1 - _uelem->dataSelection.sliceNr;
	}

	return;
}

unsigned SlicesHandler::return_nrundo()
{
	return this->_undoQueue.return_nrundo();
}

unsigned SlicesHandler::return_nrredo()
{
	return this->_undoQueue.return_nrredo();
}

bool SlicesHandler::return_undo3D() { return _undo3D; }

unsigned SlicesHandler::return_nrundosteps()
{
	return this->_undoQueue.return_nrundomax();
}

unsigned SlicesHandler::return_nrundoarrays()
{
	return this->_undoQueue.return_nrundoarraysmax();
}

void SlicesHandler::set_undo3D(bool undo3D1) { _undo3D = undo3D1; }

void SlicesHandler::set_undonr(unsigned nr) { this->_undoQueue.set_nrundo(nr); }

void SlicesHandler::set_undoarraynr(unsigned nr)
{
	this->_undoQueue.set_nrundoarraysmax(nr);
}

int SlicesHandler::LoadDICOM(std::vector<const char*> lfilename)
{
	if (lfilename.size() > 0)
	{
		_endslice = _nrslices = (unsigned short)lfilename.size();
		_os.set_sizenr(_nrslices);
		_image_slices.resize(_nrslices);

		_activeslice = 0;
		_active_tissuelayer = 0;
		_startslice = 0;

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

					if (bits == nullptr)
						return 0;

					bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(
							lfilename[i], bits, a, b, c);
					if (!canload)
					{
						free(bits);
						return 0;
					}

					_image_slices[i]
							.LoadArray(&(bits[(unsigned long)(a)*b * 0]), a, b);

					free(bits);
				}
			}
		}

		_endslice = _nrslices = (unsigned short)(lfilename.size());
		_os.set_sizenr(_nrslices);

		// Ranges
		Pair dummy;
		_slice_ranges.resize(_nrslices);
		_slice_bmpranges.resize(_nrslices);
		compute_range_mode1(&dummy);
		compute_bmprange_mode1(&dummy);

		_width = _image_slices[0].return_width();
		_height = _image_slices[0].return_height();
		_area = _height * (unsigned int)_width;

		new_overlay();

		_loaded = true;

		return true;
	}
	return false;
}

int SlicesHandler::LoadDICOM(std::vector<const char*> lfilename, Point p,
		unsigned short dx, unsigned short dy)
{
	_activeslice = 0;
	_active_tissuelayer = 0;
	_startslice = 0;

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
			if (bits == nullptr)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0],
					bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			_endslice = _nrslices = (unsigned short)c;
			_os.set_sizenr(_nrslices);
			_image_slices.resize(_nrslices);

			int j = 0;
			for (unsigned short i = 0; i < _nrslices; i++)
			{
				if (_image_slices[i].LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b, p, dx, dy))
					j++;
			}

			free(bits);

			if (j < _nrslices)
				return 0;

			// Ranges
			Pair dummy;
			_slice_ranges.resize(_nrslices);
			_slice_bmpranges.resize(_nrslices);
			compute_range_mode1(&dummy);
			compute_bmprange_mode1(&dummy);

			_width = _image_slices[0].return_width();
			_height = _image_slices[0].return_height();
			_area = _height * (unsigned int)_width;

			new_overlay();

			_loaded = true;

			Transform tr(disp1, dc1);

			set_pixelsize(d, e);
			set_slicethickness(thick1);
			set_transform(tr);

			return 1;
		}
	}

	_endslice = _nrslices = (unsigned short)(lfilename.size());
	_os.set_sizenr(_nrslices);

	_image_slices.resize(_nrslices);
	int j = 0;

	float thick1 = DICOMsort(&lfilename);
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		if (_image_slices[i].LoadDICOM(lfilename[i], p, dx, dy))
			j++;
	}

	_width = _image_slices[0].return_width();
	_height = _image_slices[0].return_height();
	_area = _height * (unsigned int)_width;

	new_overlay();

	if (j == _nrslices)
	{
		_loaded = true;

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
		_slice_ranges.resize(_nrslices);
		_slice_bmpranges.resize(_nrslices);
		compute_range_mode1(&dummy);
		compute_bmprange_mode1(&dummy);

		return 1;
	}
	else
		return 0;
}

int SlicesHandler::ReloadDICOM(std::vector<const char*> lfilename)
{
	if ((_endslice - _startslice) == (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			if (_image_slices[i].ReloadDICOM(lfilename[i - _startslice]))
				j++;
		}

		if (j == (_startslice - _endslice))
			return 1;
		else
			return 0;
	}
	else if (_nrslices <= (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = 0; i < _nrslices; i++)
		{
			if (_image_slices[i].ReloadDICOM(lfilename[i]))
				j++;
		}

		if (j == _nrslices)
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
		if (_nrslices == c)
		{
			unsigned long totsize = (unsigned long)(a)*b * c;
			float* bits = (float*)malloc(sizeof(float) * totsize);
			if (bits == nullptr)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0],
					bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			int j = 0;
			for (unsigned short i = _startslice; i < _endslice; i++)
			{
				if (_image_slices[i]
								.LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b))
					j++;
			}

			free(bits);
			if (j < _endslice - _startslice)
				return 0;
			return 1;
		}
	}
	return 0;
}

int SlicesHandler::ReloadDICOM(std::vector<const char*> lfilename, Point p)
{
	if ((_endslice - _startslice) == (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			if (_image_slices[i].ReloadDICOM(lfilename[i - _startslice], p))
				j++;
		}

		if (j == (_endslice - _startslice))
			return 1;
		else
			return 0;
	}
	else if (_nrslices <= (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = 0; i < _nrslices; i++)
		{
			if (_image_slices[i].ReloadDICOM(lfilename[i], p))
				j++;
		}

		if (j == _nrslices)
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
		if (_nrslices == c)
		{
			unsigned long totsize = (unsigned long)(a)*b * c;
			float* bits = (float*)malloc(sizeof(float) * totsize);
			if (bits == nullptr)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0],
					bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			int j = 0;
			for (unsigned short i = _startslice; i < _endslice; i++)
			{
				if (_image_slices[i]
								.LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b, p,
										_width, _height))
					j++;
			}

			free(bits);
			if (j < _endslice - _startslice)
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
	for (auto it = vnames->begin(); it != vnames->end(); it++)
	{
		dcmread.opendicom(*it);
		auto it1 = dicomseriesnr->begin();
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

void SlicesHandler::map_tissue_indices(const std::vector<tissues_size_t>& indexMap)
{
	int const iN = _nrslices;

#pragma omp parallel for
	for (int i = 0; i < iN; i++)
	{
		_image_slices[i].map_tissue_indices(indexMap);
	}
}

void SlicesHandler::remove_tissue(tissues_size_t tissuenr)
{
	int const iN = _nrslices;

#pragma omp parallel for
	for (int i = 0; i < iN; i++)
	{
		_image_slices[i].remove_tissue(tissuenr);
	}
	TissueInfos::RemoveTissue(tissuenr);
}

void SlicesHandler::remove_tissues(const std::set<tissues_size_t>& tissuenrs)
{
	std::vector<bool> isSelected(TissueInfos::GetTissueCount() + 1, false);
	for (auto id : tissuenrs)
	{
		isSelected.at(id) = true;
	}

	std::vector<tissues_size_t> idxMap(isSelected.size(), 0);
	for (tissues_size_t oldIdx = 1, newIdx = 1; oldIdx < idxMap.size(); ++oldIdx)
	{
		if (!isSelected[oldIdx])
		{
			idxMap[oldIdx] = newIdx++;
		}
	}

	map_tissue_indices(idxMap);

	TissueInfos::RemoveTissues(tissuenrs);
}

void SlicesHandler::remove_tissueall()
{
	for (short unsigned i = 0; i < _nrslices; i++)
	{
		_image_slices[i].cleartissuesall();
	}
	TissueInfos::RemoveAllTissues();
	TissueInfo tissue;
	tissue.locked = false;
	tissue.color = Color(1.0f, 0.0f, 0.1f);
	tissue.name = "Tissue1";
	TissueInfos::AddTissue(tissue);
}

void SlicesHandler::cap_tissue(tissues_size_t maxval)
{
	for (short unsigned i = 0; i < _nrslices; i++)
	{
		_image_slices[i].cap_tissue(maxval);
	}
}

void SlicesHandler::buildmissingtissues(tissues_size_t j)
{
	tissues_size_t tissueCount = TissueInfos::GetTissueCount();
	if (j > tissueCount)
	{
		QString sdummy;
		TissueInfo tissue;
		tissue.locked = false;
		tissue.opac = 0.5f;
		for (tissues_size_t i = tissueCount + 1; i <= j; i++)
		{
			tissue.color[0] = ((i - 1) % 7) * 0.166666666f;
			tissue.color[1] = (((i - 1) / 7) % 7) * 0.166666666f;
			tissue.color[2] = ((i - 1) / 49) * 0.19f;
			tissue.name = (boost::format("Tissue%d") % i).str();
			TissueInfos::AddTissue(tissue);
		}
	}
}

std::vector<tissues_size_t> SlicesHandler::find_unused_tissues()
{
	std::vector<unsigned char> is_used(TissueInfos::GetTissueCount() + 1, 0);

	for (int i = 0, iN = _nrslices; i < iN; i++)
	{
		auto tissues = _image_slices[i].return_tissues(_active_tissuelayer);
		for (unsigned k = 0; k < _area; ++k)
		{
			is_used[tissues[k]] = 1;
		}
	}

	std::vector<tissues_size_t> unused_tissues;
	for (size_t i = 1; i < is_used.size(); ++i)
	{
		if (is_used[i] == 0)
		{
			ISEG_INFO("Unused tissue: " << TissueInfos::GetTissueName(i) << " (" << i << ")");
			unused_tissues.push_back(i);
		}
	}

	return unused_tissues;
}

void SlicesHandler::group_tissues(std::vector<tissues_size_t>& olds, std::vector<tissues_size_t>& news)
{
	int const iN = _nrslices;

#pragma omp parallel for
	for (int i = 0; i < iN; i++)
	{
		_image_slices[i].group_tissues(_active_tissuelayer, olds, news);
	}
}
void SlicesHandler::set_modeall(unsigned char mode, bool bmporwork)
{
	for (unsigned short i = _startslice; i < _endslice; i++)
		_image_slices[i].set_mode(mode, bmporwork);
}

bool SlicesHandler::print_tissuemat(const char* filename)
{
	std::vector<tissues_size_t*> matrix(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
		matrix[i] = _image_slices[i].return_tissues(_active_tissuelayer);
	bool ok = matexport::print_matslices(
			filename, matrix.data(), int(_width), int(_height), int(_nrslices),
			"iSeg tissue data v1.0", 21, "tissuedistrib", 13);
	return ok;
}

bool SlicesHandler::print_bmpmat(const char* filename)
{
	std::vector<float*> matrix(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
		matrix[i] = _image_slices[i].return_bmp();
	bool ok = matexport::print_matslices(
			filename, matrix.data(), int(_width), int(_height), int(_nrslices),
			"iSeg source data v1.0", 21, "sourcedistrib", 13);
	return ok;
}

bool SlicesHandler::print_workmat(const char* filename)
{
	std::vector<float*> matrix(_nrslices);
	for (unsigned short i = 0; i < _nrslices; i++)
		matrix[i] = _image_slices[i].return_work();
	bool ok = matexport::print_matslices(
			filename, matrix.data(), int(_width), int(_height), int(_nrslices),
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
	out << (quint32)_width << (quint32)_height << (quint32)_nrslices;
	out << (float)_dx << (float)_dy << (float)_thickness;
	tissues_size_t tissueCount = TissueInfos::GetTissueCount();
	out << (quint32)tissueCount;
	TissueInfo* tissueInfo;
	for (tissues_size_t tissuenr = 1; tissuenr <= tissueCount; tissuenr++)
	{
		tissueInfo = TissueInfos::GetTissueInfo(tissuenr);
		out << ToQ(tissueInfo->name) << tissueInfo->color[0] << tissueInfo->color[1] << tissueInfo->color[2];
	}
	for (unsigned short i = 0; i < _nrslices; i++)
	{
		out.writeRawData((char*)_image_slices[i].return_bmp(),
				(int)_area * sizeof(float));
		out.writeRawData(
				(char*)_image_slices[i].return_tissues(_active_tissuelayer),
				(int)_area * sizeof(tissues_size_t));
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
		streamname << "define Lattice " << _width << " " << _height << " "
							 << _nrslices << endl
							 << endl;
		streamname << "Parameters {" << endl;
		streamname << "    Materials {" << endl;
		streamname << "        Exterior {" << endl;
		streamname << "            Id 1" << endl;
		streamname << "        }" << endl;
		tissues_size_t tissueCount = TissueInfos::GetTissueCount();
		TissueInfo* tissueInfo;
		for (tissues_size_t tc = 0; tc < tissueCount; tc++)
		{
			tissueInfo = TissueInfos::GetTissueInfo(tc + 1);
			QString nameCpy = ToQ(tissueInfo->name);
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
			streamname << "    Content \"" << _width << "x" << _height << "x"
								 << _nrslices << " byte, uniform coordinates\"," << endl;
		}
		else
		{
			streamname << "    Content \"" << _width << "x" << _height << "x"
								 << _nrslices << " ushort, uniform coordinates\"," << endl;
		}
		streamname << "    BoundingBox 0 " << _width * _dx << " 0 " << _height * _dy
							 << " 0 " << _nrslices * _thickness << "," << endl;
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
		for (unsigned short i = 0; i < _nrslices; i++)
			ok &= _image_slices[i].print_amascii_slice(_active_tissuelayer,
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
	const char* tissueColorArrayName = "Colors";		 // don't modify this

	// copy label field
	vtkImageData* labelField = vtkImageData::New();
	labelField->SetExtent(0, (int)width() - 1, 0,
			(int)height() - 1, 0,
			(int)(_endslice - _startslice) - 1);
	Pair ps = get_pixelsize();
	float offset[3];
	get_displacement(offset);
	labelField->SetSpacing(ps.high, ps.low, _thickness);
	labelField->SetOrigin(offset[0], offset[1], offset[2] + _thickness * _startslice);
	if (TissueInfos::GetTissueCount() <= 255)
	{
		labelField->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		unsigned char* field =
				(unsigned char*)labelField->GetScalarPointer(0, 0, 0);
		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			copyfromtissue(i, &(field[i * (unsigned long long)return_area()]));
		}
	}
	else if (sizeof(tissues_size_t) == sizeof(unsigned short))
	{
		labelField->AllocateScalars(VTK_UNSIGNED_SHORT, 1);
		tissues_size_t* field = (tissues_size_t*)labelField->GetScalarPointer(0, 0, 0);
		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			copyfromtissue(i, &(field[i * (unsigned long long)return_area()]));
		}
	}
	else
	{
		ISEG_ERROR_MSG("SlicesHandler::make_vtktissueimage: Error, tissues_size_t not implemented!");
		labelField->Delete();
		return nullptr;
	}

	// copy tissue names and colors
	tissues_size_t num_tissues = TissueInfos::GetTissueCount();
	auto names_array = vtkSmartPointer<vtkStringArray>::New();
	names_array->SetNumberOfTuples(num_tissues + 1);
	names_array->SetName(tissueNameArrayName);
	labelField->GetFieldData()->AddArray(names_array);

	auto color_array = vtkSmartPointer<vtkFloatArray>::New();
	color_array->SetNumberOfComponents(3);
	color_array->SetNumberOfTuples(num_tissues + 1);
	color_array->SetName(tissueColorArrayName);
	labelField->GetFieldData()->AddArray(color_array);
	for (tissues_size_t i = 1; i < num_tissues; i++)
	{
		int error_counter = 0;
		check_equal(TissueInfos::GetTissueType(TissueInfos::GetTissueName(i)), i);
		names_array->SetValue(i, TissueInfos::GetTissueName(i).c_str());
		auto color = TissueInfos::GetTissueColor(i);
		ISEG_INFO(TissueInfos::GetTissueName(i).c_str() << " " << color[0] << "," << color[1] << "," << color[2]);
		color_array->SetTuple(i, color.v.data());
	}

	return labelField;
}

bool SlicesHandler::export_tissue(const char* filename, bool binary) const
{
	auto slices = const_cast<SlicesHandler*>(this)->tissue_slices(_active_tissuelayer);
	return ImageWriter(binary).writeVolume(filename, slices, true, this);
}

bool SlicesHandler::export_bmp(const char* filename, bool binary) const
{
	auto slices = const_cast<SlicesHandler*>(this)->source_slices();
	return ImageWriter(binary).writeVolume(filename, slices, true, this);
}

bool SlicesHandler::export_work(const char* filename, bool binary) const
{
	auto slices = const_cast<SlicesHandler*>(this)->target_slices();
	return ImageWriter(binary).writeVolume(filename, slices, true, this);
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
		TissueInfo* tissueInfo;
		for (tissues_size_t tissuenr = 1; tissuenr <= tissueCount; tissuenr++)
		{
			if (get_extent(tissuenr, onlyactiveslices, extent))
			{
				tissueInfo = TissueInfos::GetTissueInfo(tissuenr);
				streamname << "\t<label id=\"" << (int)tissuenr << "\" name=\""
									 << tissueInfo->name.c_str() << "\" color=\""
									 << tissueInfo->color[0] << " "
									 << tissueInfo->color[1] << " "
									 << tissueInfo->color[2] << "\">" << endl;
				streamname << "\t\t<dataset filename=\"";
				if (projname != nullptr)
				{
					streamname << projname;
				}
				streamname << "\" extent=\"" << extent[0][0] << " "
									 << extent[0][1] << " " << extent[1][0] << " "
									 << extent[1][1] << " " << extent[2][0] << " "
									 << extent[2][1];
				streamname << "\" global_bounds=\""
									 << extent[0][0] * _dx + offset[0] << " "
									 << extent[0][1] * _dx + offset[0] << " "
									 << extent[1][0] * _dy + offset[1] << " "
									 << extent[1][1] * _dy + offset[1] << " "
									 << extent[2][0] * _thickness + offset[2] << " "
									 << extent[2][1] * _thickness + offset[2] << "\">"
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
								 << TissueInfos::GetTissueName(tissuenr).c_str()
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
	endslice1 = _nrslices;
	if (onlyactiveslices)
	{
		startslice1 = _startslice;
		endslice1 = _endslice;
	}
	for (unsigned short i = startslice1; i < endslice1; i++)
	{
		if (!found)
		{
			found = _image_slices[i].get_extent(_active_tissuelayer, tissuenr,
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
			if (_image_slices[i].get_extent(_active_tissuelayer, tissuenr,
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
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		float* work;
		work = _image_slices[z].return_work();
		unsigned i = 0, pos, y, x;

		//Create a binary vector noTissue/Tissue
		std::vector<int> s;
		for (int j = 0; j < _height; j++)
		{
			for (int k = 0; k < _width; k++)
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
			for (y = 0; y < _height; y++)
			{
				pos = y * _width;
				while (pos < (y + 1) * _width)
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

				pos = (y + 1) * _width - 1;
				while (pos > y * _width)
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

			for (x = 0; x < _width; x++)
			{
				pos = x;
				while (pos < _height * _width)
				{
					if (s[pos] == 0)
					{
						if (convertSkin)
							s[pos] = 256;
						convertSkin = false;
					}
					else
						convertSkin = true;

					pos += _width;
				}

				pos = _width * (_height - 1) + x;
				while (pos > _width)
				{
					if (s[pos] == 0)
					{
						if (convertSkin)
							s[pos] = 256;
						convertSkin = false;
					}
					else
						convertSkin = true;

					pos -= _width;
				}
			}
		}

		//go over the vector and set the skin pixel at the source pointer
		i = 0;
		for (int j = 0; j < _height; j++)
		{
			for (int k = 0; k < _width; k++)
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

	for (unsigned short z = _startslice; z < _endslice - checkSliceDistance; z++)
	{
		float* work1;
		float* work2;
		work1 = _image_slices[z].return_work();
		work2 = _image_slices[z + checkSliceDistance].return_work();

		unsigned i = 0;
		for (int j = 0; j < _height; j++)
		{
			for (int k = 0; k < _width; k++)
			{
				if (work1[i] != 0 && work1[i] != setto)
					if (work2[i] == 0)
						work1[i] = setto;
				i++;
			}
		}
	}

	for (unsigned short z = _endslice - 1; z > _startslice + checkSliceDistance;
			 z--)
	{
		float* work1;
		float* work2;
		work1 = _image_slices[z].return_work();
		work2 = _image_slices[z - checkSliceDistance].return_work();

		unsigned i = 0;
		for (int j = 0; j < _height; j++)
		{
			for (int k = 0; k < _width; k++)
			{
				if (work1[i] != 0 && work1[i] != setto)
					if (work2[i] == 0)
						work1[i] = setto;
				i++;
			}
		}
	}
}

void SlicesHandler::add_skin3D_outside(int ix, int iy, int iz, float setto)
{
	float set_to = (float)123E10;
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		_image_slices[z].flood_exterior(set_to);
	}

	std::vector<posit> s;
	posit p1;

	p1.pz = _activeslice;

	float* work1;
	float* work2;
	for (unsigned short z = _startslice; z + 1 < _endslice; z++)
	{
		work1 = _image_slices[z].return_work();
		work2 = _image_slices[z + 1].return_work();
		for (unsigned long i = 0; i < _area; i++)
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
	for (unsigned short z = _endslice - 1; z > _startslice; z--)
	{
		work1 = _image_slices[z].return_work();
		work2 = _image_slices[z - 1].return_work();
		for (unsigned long i = 0; i < _area; i++)
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

		work = _image_slices[i.pz].return_work();
		if (i.pxy % _width != 0 && work[i.pxy - 1] == 0)
		{
			work[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % _width != 0 && work[i.pxy + 1] == 0)
		{
			work[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= _width && work[i.pxy - _width] == 0)
		{
			work[i.pxy - _width] = set_to;
			j.pxy = i.pxy - _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy < _area - _width && work[i.pxy + _width] == 0)
		{
			work[i.pxy + _width] = set_to;
			j.pxy = i.pxy + _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > _startslice)
		{
			work = _image_slices[i.pz - 1].return_work();
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < _endslice)
		{
			work = _image_slices[i.pz + 1].return_work();
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
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		work = _image_slices[z].return_work();

		for (y = 0; y < _height; y++)
		{
			pos = y * _width;
			i1 = 0;
			while (pos < (unsigned long)(y + 1) * _width)
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

		for (y = 0; y < _height; y++)
		{
			pos = (y + 1) * _width - 1;
			i1 = 0;
			while (pos > (unsigned long)y * _width)
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

		for (x = 0; x < _width; x++)
		{
			pos = x;
			i1 = 0;
			while (pos < _area)
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
				pos += _width;
			}
		}

		for (x = 0; x < _width; x++)
		{
			pos = _area + x - _width;
			i1 = 0;
			while (pos > _width)
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
				pos -= _width;
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

	unsigned short* counter = (unsigned short*)malloc(sizeof(unsigned short) * _area);
	for (unsigned i1 = 0; i1 < _area; i1++)
		counter[i1] = 0;
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		work = _image_slices[z].return_work();
		for (pos = 0; pos < _area; pos++)
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
	for (unsigned i1 = 0; i1 < _area; i1++)
		counter[i1] = 0;
	for (unsigned short z = _endslice - 1; z > _startslice; z--)
	{
		work = _image_slices[z].return_work();
		for (pos = 0; pos < _area; pos++)
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
	work = _image_slices[_startslice].return_work();
	for (pos = 0; pos < _area; pos++)
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

	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		work = _image_slices[z].return_work();
		for (unsigned i1 = 0; i1 < _area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}
}

void SlicesHandler::add_skin3D_outside2(int ix, int iy, int iz, float setto)
{
	float set_to = (float)123E10;
	float set_to2 = (float)321E10;
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		_image_slices[z].flood_exterior(set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	posit p1;

	//	p1.pxy=position;
	p1.pz = _activeslice;

	float* work1;
	float* work2;
	for (unsigned short z = _startslice; z + 1 < _endslice; z++)
	{
		work1 = _image_slices[z].return_work();
		work2 = _image_slices[z + 1].return_work();
		for (unsigned long i = 0; i < _area; i++)
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
	for (unsigned short z = _endslice - 1; z > _startslice; z--)
	{
		work1 = _image_slices[z].return_work();
		work2 = _image_slices[z - 1].return_work();
		for (unsigned long i = 0; i < _area; i++)
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

		work = _image_slices[i.pz].return_work();
		if (i.pxy % _width != 0 && work[i.pxy - 1] == 0)
		{
			work[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % _width != 0 && work[i.pxy + 1] == 0)
		{
			work[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= _width && work[i.pxy - _width] == 0)
		{
			work[i.pxy - _width] = set_to;
			j.pxy = i.pxy - _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy < _area - _width && work[i.pxy + _width] == 0)
		{
			work[i.pxy + _width] = set_to;
			j.pxy = i.pxy + _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > _startslice)
		{
			work = _image_slices[i.pz - 1].return_work();
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < _endslice)
		{
			work = _image_slices[i.pz + 1].return_work();
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
		for (unsigned short z = _startslice; z + 1 < _endslice; z++)
		{
			work1 = _image_slices[z].return_work();
			work2 = _image_slices[z + 1].return_work();
			for (unsigned long i = 0; i < _area; i++)
			{
				if (work1[i] == set_to)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.pxy = i;
						p1.pz = z;
						Treap<posit, unsigned int>::Node* n = nullptr;
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
						Treap<posit, unsigned int>::Node* n = nullptr;
						treap1.insert(n, p1, 1, subz);
						work2[i] = set_to2;
					}
				}
			}
		}
	}

	if (ix > 0)
	{
		for (unsigned short z = _startslice; z < _endslice; z++)
		{
			p1.pz = z;
			work1 = _image_slices[z].return_work();
			unsigned long i = 0;
			for (unsigned short y = 0; y < _height; y++)
			{
				for (unsigned short x = 0; x + 1 < _width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + 1] != set_to) &&
								(work1[i + 1] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n = nullptr;
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
							Treap<posit, unsigned int>::Node* n = nullptr;
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
		for (unsigned short z = _startslice; z < _endslice; z++)
		{
			p1.pz = z;
			work1 = _image_slices[z].return_work();
			unsigned long i = 0;
			for (unsigned short y = 0; y + 1 < _height; y++)
			{
				for (unsigned short x = 0; x < _width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + _width] != set_to) &&
								(work1[i + _width] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n = nullptr;
							treap1.insert(n, p1, 1, suby);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + _width] != set_to) &&
								(work1[i + _width] != set_to2))
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
						if (work1[i + _width] == set_to2)
						{
							p1.pxy = i + _width;
							Treap<posit, unsigned int>::Node* n1 =
									treap1.lookup(p1);
							if (n1->getPriority() > suby)
								treap1.update_priority(n1, suby);
						}
						else if (work1[i + _width] == set_to)
						{
							p1.pxy = i + _width;
							Treap<posit, unsigned int>::Node* n = nullptr;
							treap1.insert(n, p1, 1, suby);
							work1[i + _width] = set_to2;
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
	while ((n1 = treap1.get_top()) != nullptr)
	{
		p1 = n1->getKey();
		prior = n1->getPriority();
		work1 = _image_slices[p1.pz].return_work();
		work1[p1.pxy] = setto;
		if (p1.pxy % _width != 0)
		{
			if (work1[p1.pxy - 1] == set_to)
			{
				if (prior + 2 * subx <= totcount)
				{
					p2.pxy = p1.pxy - 1;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = nullptr;
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
		if ((p1.pxy + 1) % _width != 0)
		{
			if (work1[p1.pxy + 1] == set_to)
			{
				if (prior + 2 * subx <= totcount)
				{
					p2.pxy = p1.pxy + 1;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = nullptr;
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
		if (p1.pxy >= _width)
		{
			if (work1[p1.pxy - _width] == set_to)
			{
				if (prior + 2 * suby <= totcount)
				{
					p2.pxy = p1.pxy - _width;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = nullptr;
					treap1.insert(n, p2, 1, prior + suby);
					work1[p1.pxy - _width] = set_to2;
				}
			}
			else if (work1[p1.pxy - _width] == set_to2)
			{
				p2.pxy = p1.pxy - _width;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + suby)
					treap1.update_priority(n2, prior + suby);
			}
		}
		if (p1.pxy < _area - _width)
		{
			if (work1[p1.pxy + _width] == set_to)
			{
				if (prior + 2 * suby <= totcount)
				{
					p2.pxy = p1.pxy + _width;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = nullptr;
					treap1.insert(n, p2, 1, prior + suby);
					work1[p1.pxy + _width] = set_to2;
				}
			}
			else if (work1[p1.pxy + _width] == set_to2)
			{
				p2.pxy = p1.pxy + _width;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + suby)
					treap1.update_priority(n2, prior + suby);
			}
		}
		if (p1.pz > _startslice)
		{
			work2 = _image_slices[p1.pz - 1].return_work();
			if (work2[p1.pxy] == set_to)
			{
				if (prior + 2 * subz <= totcount)
				{
					p2.pxy = p1.pxy;
					p2.pz = p1.pz - 1;
					Treap<posit, unsigned int>::Node* n = nullptr;
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
		if (p1.pz + 1 < _endslice)
		{
			work2 = _image_slices[p1.pz + 1].return_work();
			if (work2[p1.pxy] == set_to)
			{
				if (prior + 2 * subz <= totcount)
				{
					p2.pxy = p1.pxy;
					p2.pz = p1.pz + 1;
					Treap<posit, unsigned int>::Node* n = nullptr;
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

	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		work = _image_slices[z].return_work();
		for (unsigned i1 = 0; i1 < _area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}

	return;
}

void SlicesHandler::add_skintissue3D_outside2(int ix, int iy, int iz, tissues_size_t f)
{
	tissues_size_t set_to = TISSUES_SIZE_MAX;
	tissues_size_t set_to2 = TISSUES_SIZE_MAX - 1;
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		_image_slices[z].flood_exteriortissue(_active_tissuelayer, set_to);
	}

	std::vector<posit> s;
	posit p1;

	p1.pz = _activeslice;

	tissues_size_t* work1;
	tissues_size_t* work2;
	for (unsigned short z = _startslice; z + 1 < _endslice; z++)
	{
		work1 = _image_slices[z].return_tissues(_active_tissuelayer);
		work2 = _image_slices[z + 1].return_tissues(_active_tissuelayer);
		for (unsigned long i = 0; i < _area; i++)
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
	for (unsigned short z = _endslice - 1; z > _startslice; z--)
	{
		work1 = _image_slices[z].return_tissues(_active_tissuelayer);
		work2 = _image_slices[z - 1].return_tissues(_active_tissuelayer);
		for (unsigned long i = 0; i < _area; i++)
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

		work = _image_slices[i.pz].return_tissues(_active_tissuelayer);
		if (i.pxy % _width != 0 && work[i.pxy - 1] == 0)
		{
			work[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % _width != 0 && work[i.pxy + 1] == 0)
		{
			work[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= _width && work[i.pxy - _width] == 0)
		{
			work[i.pxy - _width] = set_to;
			j.pxy = i.pxy - _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy < _area - _width && work[i.pxy + _width] == 0)
		{
			work[i.pxy + _width] = set_to;
			j.pxy = i.pxy + _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > _startslice)
		{
			work = _image_slices[i.pz - 1].return_tissues(_active_tissuelayer);
			if (work[i.pxy] == 0)
			{
				work[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < _endslice)
		{
			work = _image_slices[i.pz + 1].return_tissues(_active_tissuelayer);
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
		for (unsigned short z = _startslice; z + 1 < _endslice; z++)
		{
			work1 = _image_slices[z].return_tissues(_active_tissuelayer);
			work2 = _image_slices[z + 1].return_tissues(_active_tissuelayer);
			for (unsigned long i = 0; i < _area; i++)
			{
				if (work1[i] == set_to)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.pxy = i;
						p1.pz = z;
						Treap<posit, unsigned int>::Node* n = nullptr;
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
						Treap<posit, unsigned int>::Node* n = nullptr;
						treap1.insert(n, p1, 1, subz);
						work2[i] = set_to2;
					}
				}
			}
		}
	}

	if (ix > 0)
	{
		for (unsigned short z = _startslice; z < _endslice; z++)
		{
			p1.pz = z;
			work1 = _image_slices[z].return_tissues(_active_tissuelayer);
			unsigned long i = 0;
			for (unsigned short y = 0; y < _height; y++)
			{
				for (unsigned short x = 0; x + 1 < _width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + 1] != set_to) &&
								(work1[i + 1] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n = nullptr;
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
							Treap<posit, unsigned int>::Node* n = nullptr;
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
		for (unsigned short z = _startslice; z < _endslice; z++)
		{
			p1.pz = z;
			work1 = _image_slices[z].return_tissues(_active_tissuelayer);
			unsigned long i = 0;
			for (unsigned short y = 0; y + 1 < _height; y++)
			{
				for (unsigned short x = 0; x < _width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + _width] != set_to) &&
								(work1[i + _width] != set_to2))
						{
							p1.pxy = i;
							Treap<posit, unsigned int>::Node* n = nullptr;
							treap1.insert(n, p1, 1, suby);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + _width] != set_to) &&
								(work1[i + _width] != set_to2))
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
						if (work1[i + _width] == set_to2)
						{
							p1.pxy = i + _width;
							Treap<posit, unsigned int>::Node* n1 =
									treap1.lookup(p1);
							if (n1->getPriority() > suby)
								treap1.update_priority(n1, suby);
						}
						else if (work1[i + _width] == set_to)
						{
							p1.pxy = i + _width;
							Treap<posit, unsigned int>::Node* n = nullptr;
							treap1.insert(n, p1, 1, suby);
							work1[i + _width] = set_to2;
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
	while ((n1 = treap1.get_top()) != nullptr)
	{
		p1 = n1->getKey();
		prior = n1->getPriority();
		work1 = _image_slices[p1.pz].return_tissues(_active_tissuelayer);
		work1[p1.pxy] = f;
		if (p1.pxy % _width != 0)
		{
			if (work1[p1.pxy - 1] == set_to)
			{
				if (prior + subx <= totcount)
				{
					p2.pxy = p1.pxy - 1;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = nullptr;
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
		if ((p1.pxy + 1) % _width != 0)
		{
			if (work1[p1.pxy + 1] == set_to)
			{
				if (prior + subx <= totcount)
				{
					p2.pxy = p1.pxy + 1;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = nullptr;
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
		if (p1.pxy >= _width)
		{
			if (work1[p1.pxy - _width] == set_to)
			{
				if (prior + suby <= totcount)
				{
					p2.pxy = p1.pxy - _width;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = nullptr;
					treap1.insert(n, p2, 1, prior + suby);
					work1[p1.pxy - _width] = set_to2;
				}
			}
			else if (work1[p1.pxy - _width] == set_to2)
			{
				p2.pxy = p1.pxy - _width;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + suby)
					treap1.update_priority(n2, prior + suby);
			}
		}
		if (p1.pxy < _area - _width)
		{
			if (work1[p1.pxy + _width] == set_to)
			{
				if (prior + suby <= totcount)
				{
					p2.pxy = p1.pxy + _width;
					p2.pz = p1.pz;
					Treap<posit, unsigned int>::Node* n = nullptr;
					treap1.insert(n, p2, 1, prior + suby);
					work1[p1.pxy + _width] = set_to2;
				}
			}
			else if (work1[p1.pxy + _width] == set_to2)
			{
				p2.pxy = p1.pxy + _width;
				p2.pz = p1.pz;
				Treap<posit, unsigned int>::Node* n2 = treap1.lookup(p2);
				if (n2->getPriority() > prior + suby)
					treap1.update_priority(n2, prior + suby);
			}
		}
		if (p1.pz > _startslice)
		{
			work2 = _image_slices[p1.pz - 1].return_tissues(_active_tissuelayer);
			if (work2[p1.pxy] == set_to)
			{
				if (prior + subz <= totcount)
				{
					p2.pxy = p1.pxy;
					p2.pz = p1.pz - 1;
					Treap<posit, unsigned int>::Node* n = nullptr;
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
		if (p1.pz + 1 < _endslice)
		{
			work2 = _image_slices[p1.pz + 1].return_tissues(_active_tissuelayer);
			if (work2[p1.pxy] == set_to)
			{
				if (prior + subz <= totcount)
				{
					p2.pxy = p1.pxy;
					p2.pz = p1.pz + 1;
					Treap<posit, unsigned int>::Node* n = nullptr;
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

	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		work = _image_slices[z].return_tissues(_active_tissuelayer);
		for (unsigned i1 = 0; i1 < _area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}
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

		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			float* bits = _image_slices[i].return_work();
			for (unsigned pos = 0; pos < _area; pos++)
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

		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			float* bits = _image_slices[i].return_work();
			for (unsigned pos = 0; pos < _area; pos++)
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

		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			float* bits = _image_slices[i].return_work();
			for (unsigned pos = 0; pos < _area; pos++)
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

		for (unsigned short i = _startslice; i < _endslice; i++)
		{
			float* bits = _image_slices[i].return_work();
			for (unsigned pos = 0; pos < _area; pos++)
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
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		_image_slices[z].flood_exteriortissue(_active_tissuelayer, set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	posit p1;

	//	p1.pxy=position;
	p1.pz = _activeslice;

	tissues_size_t* tissue1;
	tissues_size_t* tissue2;
	for (unsigned short z = _startslice; z + 1 < _endslice; z++)
	{
		tissue1 = _image_slices[z].return_tissues(_active_tissuelayer);
		tissue2 = _image_slices[z + 1].return_tissues(_active_tissuelayer);
		for (unsigned long i = 0; i < _area; i++)
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
	for (unsigned short z = _endslice - 1; z > _startslice; z--)
	{
		tissue1 = _image_slices[z].return_tissues(_active_tissuelayer);
		tissue2 = _image_slices[z - 1].return_tissues(_active_tissuelayer);
		for (unsigned long i = 0; i < _area; i++)
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

		tissue = _image_slices[i.pz].return_tissues(_active_tissuelayer);
		if (i.pxy % _width != 0 && tissue[i.pxy - 1] == 0)
		{
			tissue[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % _width != 0 && tissue[i.pxy + 1] == 0)
		{
			tissue[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= _width && tissue[i.pxy - _width] == 0)
		{
			tissue[i.pxy - _width] = set_to;
			j.pxy = i.pxy - _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy < _area - _width && tissue[i.pxy + _width] == 0)
		{
			tissue[i.pxy + _width] = set_to;
			j.pxy = i.pxy + _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > _startslice)
		{
			tissue = _image_slices[i.pz - 1].return_tissues(_active_tissuelayer);
			if (tissue[i.pxy] == 0)
			{
				tissue[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < _endslice)
		{
			tissue = _image_slices[i.pz + 1].return_tissues(_active_tissuelayer);
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
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		tissue = _image_slices[z].return_tissues(_active_tissuelayer);

		for (y = 0; y < _height; y++)
		{
			pos = y * _width;
			i1 = 0;
			while (pos < (unsigned long)(y + 1) * _width)
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

		for (y = 0; y < _height; y++)
		{
			pos = (y + 1) * _width - 1;
			i1 = 0;
			while (pos > (unsigned long)y * _width)
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

		for (x = 0; x < _width; x++)
		{
			pos = x;
			i1 = 0;
			while (pos < _area)
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
				pos += _width;
			}
		}

		for (x = 0; x < _width; x++)
		{
			pos = _area + x - _width;
			i1 = 0;
			while (pos > _width)
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
				pos -= _width;
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
			(unsigned short*)malloc(sizeof(unsigned short) * _area);
	for (unsigned i1 = 0; i1 < _area; i1++)
		counter[i1] = 0;
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		tissue = _image_slices[z].return_tissues(_active_tissuelayer);
		for (pos = 0; pos < _area; pos++)
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
	for (unsigned i1 = 0; i1 < _area; i1++)
		counter[i1] = 0;
	for (unsigned short z = _endslice - 1; z > _startslice; z--)
	{
		tissue = _image_slices[z].return_tissues(_active_tissuelayer);
		for (pos = 0; pos < _area; pos++)
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
	tissue = _image_slices[_startslice].return_tissues(_active_tissuelayer);
	for (pos = 0; pos < _area; pos++)
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

	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		tissue = _image_slices[z].return_tissues(_active_tissuelayer);
		for (unsigned i1 = 0; i1 < _area; i1++)
			if (tissue[i1] == set_to)
				tissue[i1] = 0;
	}

	return;
}

void SlicesHandler::add_skintissue3D_outside(int ixyz, tissues_size_t f)
{
	tissues_size_t set_to = TISSUES_SIZE_MAX;
	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		_image_slices[z].flood_exteriortissue(_active_tissuelayer, set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<posit> s;
	std::vector<posit> s1;
	std::vector<posit>* s2 = &s1;
	std::vector<posit>* s3 = &s;
	posit p1;

	//	p1.pxy=position;
	p1.pz = _activeslice;

	tissues_size_t* tissue1;
	tissues_size_t* tissue2;
	for (unsigned short z = _startslice; z + 1 < _endslice; z++)
	{
		tissue1 = _image_slices[z].return_tissues(_active_tissuelayer);
		tissue2 = _image_slices[z + 1].return_tissues(_active_tissuelayer);
		for (unsigned long i = 0; i < _area; i++)
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
	for (unsigned short z = _endslice - 1; z > _startslice; z--)
	{
		tissue1 = _image_slices[z].return_tissues(_active_tissuelayer);
		tissue2 = _image_slices[z - 1].return_tissues(_active_tissuelayer);
		for (unsigned long i = 0; i < _area; i++)
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

		tissue = _image_slices[i.pz].return_tissues(_active_tissuelayer);
		if (i.pxy % _width != 0 && tissue[i.pxy - 1] == 0)
		{
			tissue[i.pxy - 1] = set_to;
			j.pxy = i.pxy - 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if ((i.pxy + 1) % _width != 0 && tissue[i.pxy + 1] == 0)
		{
			tissue[i.pxy + 1] = set_to;
			j.pxy = i.pxy + 1;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy >= _width && tissue[i.pxy - _width] == 0)
		{
			tissue[i.pxy - _width] = set_to;
			j.pxy = i.pxy - _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pxy <= _area - _width && tissue[i.pxy + _width] == 0)
		{
			tissue[i.pxy + _width] = set_to;
			j.pxy = i.pxy + _width;
			j.pz = i.pz;
			s.push_back(j);
		}
		if (i.pz > _startslice)
		{
			tissue = _image_slices[i.pz - 1].return_tissues(_active_tissuelayer);
			if (tissue[i.pxy] == 0)
			{
				tissue[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz - 1;
				s.push_back(j);
			}
		}
		if (i.pz + 1 < _endslice)
		{
			tissue = _image_slices[i.pz + 1].return_tissues(_active_tissuelayer);
			if (tissue[i.pxy] == 0)
			{
				tissue[i.pxy] = set_to;
				j.pxy = i.pxy;
				j.pz = i.pz + 1;
				s.push_back(j);
			}
		}
	}

	for (unsigned short z = _startslice; z + 1 < _endslice; z++)
	{
		tissue1 = _image_slices[z].return_tissues(_active_tissuelayer);
		tissue2 = _image_slices[z + 1].return_tissues(_active_tissuelayer);
		for (unsigned long i = 0; i < _area; i++)
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

	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		tissue1 = _image_slices[z].return_tissues(_active_tissuelayer);
		unsigned long i = 0;
		for (unsigned short y = 0; y < _height; y++)
		{
			for (unsigned short x = 0; x + 1 < _width; x++)
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
		for (unsigned short y = 0; y + 1 < _height; y++)
		{
			for (unsigned short x = 0; x < _width; x++)
			{
				if (tissue1[i] == set_to)
				{
					if (tissue1[i + _width] != set_to)
					{
						p1.pxy = i + _width;
						p1.pz = z;
						(*s2).push_back(p1);
					}
				}
				else
				{
					if (tissue1[i + _width] == set_to)
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

			tissue = _image_slices[i.pz].return_tissues(_active_tissuelayer);
			if (i.pxy % _width != 0 && tissue[i.pxy - 1] == set_to)
			{
				tissue[i.pxy - 1] = f;
				j.pxy = i.pxy - 1;
				j.pz = i.pz;
				(*s3).push_back(j);
			}
			if ((i.pxy + 1) % _width != 0 && tissue[i.pxy + 1] == set_to)
			{
				tissue[i.pxy + 1] = f;
				j.pxy = i.pxy + 1;
				j.pz = i.pz;
				(*s3).push_back(j);
			}
			if (i.pxy >= _width && tissue[i.pxy - _width] == set_to)
			{
				tissue[i.pxy - _width] = f;
				j.pxy = i.pxy - _width;
				j.pz = i.pz;
				(*s3).push_back(j);
			}
			if (i.pxy < _area - _width && tissue[i.pxy + _width] == set_to)
			{
				tissue[i.pxy + _width] = f;
				j.pxy = i.pxy + _width;
				j.pz = i.pz;
				(*s3).push_back(j);
			}
			if (i.pz > _startslice)
			{
				tissue =
						_image_slices[i.pz - 1].return_tissues(_active_tissuelayer);
				if (tissue[i.pxy] == set_to)
				{
					tissue[i.pxy] = f;
					j.pxy = i.pxy;
					j.pz = i.pz - 1;
					(*s3).push_back(j);
				}
			}
			if (i.pz + 1 < _endslice)
			{
				tissue =
						_image_slices[i.pz + 1].return_tissues(_active_tissuelayer);
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

	for (unsigned short z = _startslice; z < _endslice; z++)
	{
		tissue = _image_slices[z].return_tissues(_active_tissuelayer);
		for (unsigned i1 = 0; i1 < _area; i1++)
			if (tissue[i1] == set_to)
				tissue[i1] = 0;
	}

	return;
}

void SlicesHandler::fill_skin_3d(int thicknessX, int thicknessY, int thicknessZ,
		tissues_size_t backgroundID,
		tissues_size_t skinID)
{
	int numTasks = _nrslices;
	QProgressDialog progress("Fill Skin in progress...", "Cancel", 0, numTasks);
	progress.show();
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.setValue(0);

	int skinThick = thicknessX;

	std::vector<int> dims;
	dims.push_back(_width);
	dims.push_back(_height);
	dims.push_back(_nrslices);

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
		tissues_size_t* tissuesMain = _image_slices[i].return_tissues(0);
		for (int j = 0; j < _area && (!thereIsBG || !thereIsSkin); j++)
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
	for (int i = 0; i < _image_slices.size(); i++)
		tissuesVector.push_back(_image_slices[i].return_tissues(0));

	for (int i = 0; i < dims[2]; i++)
	{
		float* bmp1 = _image_slices[i].return_bmp();

		tissues_size_t* tissue1 = _image_slices[i].return_tissues(0);
		_image_slices[i].pushstack_bmp();

		for (unsigned int j = 0; j < _area; j++)
		{
			bmp1[j] = (float)tissue1[j];
		}

		_image_slices[i].dead_reckoning((float)0);
		bmp1 = _image_slices[i].return_work();
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
							size_t neighborSlice = z - ((offsetsSlices.size() - 1) / 2);

							//iterate through neighbors of pixel
							//if any of these are not
							for (int l = 0; l < offsetsSlices[z].size(); l++)
							{
								int idx = pos + offsetsSlices[z][l];
								assert(idx >= 0 && idx < _area);
								p.px = i;
								p.py = j;
								tissues_size_t value = tissuesVector[k + neighborSlice][idx];
								if (value == backgroundID)
								{
									for (int y = 0; y < offsetsSlices.size(); y++)
									{
										size_t neighborSlice2 = y - ((offsetsSlices.size() - 1) / 2);

										//iterate through neighbors of pixel
										//if any of these are not
										for (int m = 0; m < offsetsSlices[y].size(); m++)
										{
											int idx2 = idx + offsetsSlices[y][m];
											assert(idx2 >= 0 && idx2 < _area);
											if (k + neighborSlice + neighborSlice2 < dims[2]) // && k + neighborSlice + neighborSlice2 >= 0 (always true)
											{
												tissues_size_t value2 = tissuesVector[k + neighborSlice + neighborSlice2][idx2];
												if (value2 != backgroundID && value2 != skinID)
												{
													changesToMakeStruct changes;
													changes.sliceNumber = k + neighborSlice;
													changes.positionConvert = idx;
													if (std::find(partialChanges.begin(), partialChanges.end(), changes) == partialChanges.end())
														partialChanges.push_back(changes);
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
	{
		for (int j = 0; j < partialChangesThreads[i].size(); j++)
		{
			if (partialChangesThreads[i][j].sliceNumber != -1)
			{
				size_t slice = partialChangesThreads[i][j].sliceNumber;
				int pos = partialChangesThreads[i][j].positionConvert;
				_image_slices[slice].return_work()[pos] = 255.0f;
			}
		}
	}
	for (int i = dims[2] - 1; i >= 0; i--)
	{
		float* bmp1 = _image_slices[i].return_bmp();

		for (unsigned k = 0; k < _area; k++)
		{
			if (bmp1[k] < 0)
				bmp1[k] = 0;
			else
				bmp1[k] = 255.0f;
		}

		_image_slices[i].set_mode(2, false);
		_image_slices[i].popstack_bmp();
	}

	progress.setValue(numTasks);
}

float SlicesHandler::calculate_volume(Point p, unsigned short slicenr)
{
	Pair p1 = get_pixelsize();
	unsigned long count = 0;
	float f = get_work_pt(p, slicenr);
	for (unsigned short j = _startslice; j < _endslice; j++)
		count += _image_slices[j].return_workpixelcount(f);
	return get_slicethickness() * p1.high * p1.low * count;
}

float SlicesHandler::calculate_tissuevolume(Point p, unsigned short slicenr)
{
	Pair p1 = get_pixelsize();
	unsigned long count = 0;
	tissues_size_t c = get_tissue_pt(p, slicenr);
	for (unsigned short j = _startslice; j < _endslice; j++)
		count += _image_slices[j].return_tissuepixelcount(_active_tissuelayer, c);
	return get_slicethickness() * p1.high * p1.low * count;
}

void SlicesHandler::inversesliceorder()
{
	if (_nrslices > 0)
	{
		//		bmphandler dummy;
		unsigned short rcounter = _nrslices - 1;
		for (unsigned short i = 0; i < _nrslices / 2; i++, rcounter--)
		{
			_image_slices[i].swap(_image_slices[rcounter]);
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
		bits[j + 1] = new float[_area];
		if (bits[j + 1] == nullptr)
		{
			for (unsigned short i = 1; i < j; i++)
				delete[] bits[i];
			delete[] bits;
			return;
		}
	}

	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		bits[0] = _image_slices[i].return_bmp();
		for (unsigned short k = 0; k + 1 < dim; k++)
		{
			if (!ImageReader::getSlice(mhdfiles[k].c_str(), bits[k + 1], i,
							_width, _height))
			{
				for (unsigned short j = 1; j < dim; j++)
					delete[] bits[j];
				delete[] bits;
				return;
			}
		}
		MultidimensionalGamma mdg;
		Pair pair1 = get_pixelsize();
		mdg.init(_width, _height, nrtissues, dim, bits, weights, centers, tol_f,
				tol_d, pair1.high, pair1.low);
		mdg.execute();
		mdg.return_image(_image_slices[i].return_work());
		_image_slices[i].set_mode(2, false);
	}

	for (unsigned short j = 1; j < dim; j++)
		delete[] bits[j];
	delete[] bits;
	//	}
}

void SlicesHandler::stepsmooth_z(unsigned short n)
{
	if (n > (_endslice - _startslice))
		return;
	SmoothSteps stepsm;
	unsigned short linelength1 = _endslice - _startslice;
	tissues_size_t* line = new tissues_size_t[linelength1];
	stepsm.init(n, linelength1, TissueInfos::GetTissueCount() + 1);
	for (unsigned long i = 0; i < _area; i++)
	{
		unsigned short k = 0;
		for (unsigned short j = _startslice; j < _endslice; j++, k++)
		{
			line[k] = (_image_slices[j].return_tissues(_active_tissuelayer))[i];
		}
		stepsm.dostepsmooth(line);
		k = 0;
		for (unsigned short j = _startslice; j < _endslice; j++, k++)
		{
			(_image_slices[j].return_tissues(_active_tissuelayer))[i] = line[k];
		}
	}
	delete[] line;
}

void SlicesHandler::smooth_tissues(unsigned short n)
{
	// TODO: Implement criterion: Cells must be contiguous to the center of the specified filter
	std::map<tissues_size_t, unsigned short> tissuecount;
	tissues_size_t* tissuesnew =
			(tissues_size_t*)malloc(sizeof(tissues_size_t) * _area);
	if (n % 2 == 0)
		n++;
	unsigned short filtersize = n * n;
	n = (unsigned short)(0.5f * n);

	for (unsigned short j = _startslice; j < _endslice; j++)
	{
		_image_slices[j].copyfromtissue(_active_tissuelayer, tissuesnew);
		tissues_size_t* tissuesold =
				_image_slices[j].return_tissues(_active_tissuelayer);
		for (unsigned short y = n; y < _height - n; y++)
		{
			for (unsigned short x = n; x < _width - n; x++)
			{
				// Tissue count within filter size
				tissuecount.clear();
				for (short i = -n; i <= n; i++)
				{
					for (short j = -n; j <= n; j++)
					{
						tissues_size_t currtissue =
								tissuesold[(y + i) * _width + (x + j)];
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
				tissues_size_t tissuemajor = tissuesold[y * _width + x];
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
				tissuesnew[y * _width + x] = tissuemajor;
			}
		}
		_image_slices[j].copy2tissue(_active_tissuelayer, tissuesnew);
	}

	free(tissuesnew);
}

void SlicesHandler::regrow(unsigned short sourceslicenr,
		unsigned short targetslicenr, int n)
{
	float* lbmap = (float*)malloc(sizeof(float) * _area);
	for (unsigned i = 0; i < _area; i++)
		lbmap[i] = 0;

	tissues_size_t* dummy;
	tissues_size_t* results1 =
			(tissues_size_t*)malloc(sizeof(tissues_size_t) * _area);
	tissues_size_t* results2 =
			(tissues_size_t*)malloc(sizeof(tissues_size_t) * _area);
	tissues_size_t* tissues1 =
			_image_slices[sourceslicenr].return_tissues(_active_tissuelayer);
	for (unsigned int i = 0; i < _area; i++)
	{
		if (tissues1[i] == 0)
			results2[i] = TISSUES_SIZE_MAX;
		else
			results2[i] = tissues1[i];
	}

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < _area; i++)
			results1[i] = results2[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (_height - 1); i++)
		{
			for (unsigned short j = 0; j < _width; j++)
			{
				if (results2[i1] != results2[i1 + _width])
					results1[i1] = results1[i1 + _width] = 0;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < _height; i++)
		{
			for (unsigned short j = 0; j < (_width - 1); j++)
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

	for (unsigned int i = 0; i < _area; i++)
		lbmap[i] = float(results2[i]);

	delete results1;
	delete results2;

	ImageForestingTransformRegionGrowing* IFTrg =
			_image_slices[sourceslicenr].IFTrg_init(lbmap);
	float thresh = 0;

	float* f2 = IFTrg->return_pf();
	for (unsigned i = 0; i < _area; i++)
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
			_image_slices[targetslicenr].return_tissues(_active_tissuelayer);

	for (unsigned i = 0; i < _area; i++)
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
	for (unsigned short i = _startslice; i < _endslice; i++)
	{
		if (i > _startslice)
		{
			if ((_image_slices[i - 1]).return_bmp()[0] -
							_image_slices[i].return_bmp()[0] >
					jumpabs)
				shift += range;
			else if (_image_slices[i].return_bmp()[0] -
									 (_image_slices[i - 1]).return_bmp()[0] >
							 jumpabs)
				shift -= range;
		}
		ok &= _image_slices[i].unwrap(jumpratio, range, shift);
		;
	}
	if (_area > 0)
	{
		for (unsigned short i = _startslice + 1; i < _endslice; i++)
		{
			if ((_image_slices[i - 1]).return_work()[_area - 1] -
							_image_slices[i].return_work()[_area - 1] >
					jumpabs)
				return false;
			if (_image_slices[i].return_work()[_area - 1] -
							(_image_slices[i - 1]).return_work()[_area - 1] >
					jumpabs)
				return false;
		}
	}
	return ok;
}

unsigned SlicesHandler::GetNumberOfUndoSteps()
{
	return this->_undoQueue.return_nrundomax();
}

void SlicesHandler::SetNumberOfUndoSteps(unsigned n)
{
	this->_undoQueue.set_nrundo(n);
}

unsigned SlicesHandler::GetNumberOfUndoArrays()
{
	return this->_undoQueue.return_nrundoarraysmax();
}

void SlicesHandler::SetNumberOfUndoArrays(unsigned n)
{
	this->_undoQueue.set_nrundoarraysmax(n);
}

std::vector<iseg::tissues_size_t> SlicesHandler::tissue_selection() const
{
	auto sel_set = TissueInfos::GetSelectedTissues();
	return std::vector<iseg::tissues_size_t>(sel_set.begin(), sel_set.end());
}

void SlicesHandler::set_tissue_selection(const std::vector<tissues_size_t>& sel)
{
	TissueInfos::SetSelectedTissues(std::set<tissues_size_t>(sel.begin(), sel.end()));
	on_tissue_selection_changed(sel);
}

size_t SlicesHandler::number_of_colors() const
{
	return (_color_lookup_table != 0) ? _color_lookup_table->NumberOfColors() : 0;
}

void SlicesHandler::get_color(size_t idx, unsigned char& r, unsigned char& g, unsigned char& b) const
{
	if (_color_lookup_table != 0)
	{
		_color_lookup_table->GetColor(idx, r, g, b);
	}
}

bool SlicesHandler::compute_target_connectivity(ProgressInfo* progress)
{
	using input_type = SliceHandlerItkWrapper::image_ref_type;
	using image_type = itk::Image<unsigned, 3>;

	SliceHandlerItkWrapper wrapper(this);
	auto all_slices = wrapper.GetTarget(true);

	auto observer = ItkProgressObserver::New();

	auto filter = itk::ConnectedComponentImageFilter<input_type, image_type>::New();
	filter->SetInput(all_slices);
	filter->SetFullyConnected(true);
	if (progress)
	{
		observer->SetProgressInfo(progress);
		filter->AddObserver(itk::ProgressEvent(), observer);
	}
	try
	{
		filter->Update();

		if (!progress || (progress && !progress->wasCanceled()))
		{
			// copy result back
			iseg::Paste<image_type, input_type>(filter->GetOutput(), all_slices);

			// auto-scale target rendering
			set_target_fixed_range(false);

			return true;
		}
	}
	catch (itk::ExceptionObject& e)
	{
		ISEG_ERROR("Exception occurred: " << e.what());
	}
	return false;
}

bool SlicesHandler::compute_split_tissues(tissues_size_t tissue, ProgressInfo* progress)
{
	using input_type = SliceHandlerItkWrapper::tissues_ref_type;
	using internal_type = itk::Image<unsigned char, 3>;
	using image_type = itk::Image<unsigned, 3>;

	SliceHandlerItkWrapper wrapper(this);
	auto all_slices = wrapper.GetTissues(true);

	auto observer = ItkProgressObserver::New();

	auto threshold = itk::BinaryThresholdImageFilter<input_type, internal_type>::New();
	threshold->SetLowerThreshold(tissue);
	threshold->SetUpperThreshold(tissue);
	threshold->SetInput(all_slices);

	auto filter = itk::ConnectedComponentImageFilter<internal_type, image_type>::New();
	filter->SetInput(threshold->GetOutput());
	filter->SetFullyConnected(true);
	if (progress)
	{
		observer->SetProgressInfo(progress);
		filter->AddObserver(itk::ProgressEvent(), observer);
	}
	try
	{
		filter->Update();
		auto components = filter->GetOutput();

		if (!progress || (progress && !progress->wasCanceled()))
		{
			tissues_size_t Ninitial = TissueInfos::GetTissueCount();

			// add tissue infos
			auto N = filter->GetObjectCount();
			if (N > 1)
			{
				// find which object is largest -> this one will keep its original name & color
				std::vector<size_t> hist(N + 1, 0);
				itk::ImageRegionConstIterator<image_type> it(components, components->GetLargestPossibleRegion());
				for (it.GoToBegin(); !it.IsAtEnd(); ++it)
				{
					hist[it.Get()]++;
				}
				hist[0] = 0;
				const size_t max_label = std::distance(hist.begin(), std::max_element(hist.begin(), hist.end()));

				// mapping from object number to new tissue index
				std::vector<tissues_size_t> object2index(N + 1, 0);
				object2index[max_label] = tissue;
				tissues_size_t idx = 1;
				for (tissues_size_t i = 1; i <= N; ++i)
				{
					if (i != max_label)
					{
						TissueInfo info(*TissueInfos::GetTissueInfo(tissue));
						info.name += (boost::format("_%d") % static_cast<int>(idx)).str();
						TissueInfos::AddTissue(info);
						object2index.at(i) = Ninitial + idx++;
					}
				}

				// iterate over connected components, add to tissues
				itk::ImageRegionIterator<input_type> ot(all_slices, all_slices->GetLargestPossibleRegion());
				for (it.GoToBegin(), ot.GoToBegin(); !it.IsAtEnd(); ++it, ++ot)
				{
					if (it.Get() != 0)
					{
						ot.Set(object2index.at(it.Get()));
					}
				}
			}

			return true;
		}
	}
	catch (itk::ExceptionObject& e)
	{
		ISEG_ERROR("Exception occurred: " << e.what());
	}
	return false;
}
