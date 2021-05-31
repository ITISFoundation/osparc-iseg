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
#include "Data/SlicesHandlerITKInterface.h"
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
#include <vtkPolyDataReader.h>
#include <vtkSTLReader.h>
#include <vtkSmartPointer.h>
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

namespace iseg {

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

struct Posit
{
	unsigned m_Pxy;
	unsigned short m_Pz;
	inline bool operator<(const Posit& a) const
	{
		if (m_Pz < a.m_Pz || ((m_Pz == a.m_Pz) && (m_Pxy < a.m_Pxy)))
			return true;
		else
			return false;
	}
	inline bool operator==(const Posit& a) const
	{
		return (m_Pz == a.m_Pz) && (m_Pxy == a.m_Pxy);
	}
};

SlicesHandler::SlicesHandler()
{
	m_Activeslice = 0;
	m_Thickness = m_Dx = m_Dy = 1.0f;
	m_Transform.SetIdentity();

	m_Width = m_Height = 0;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = 0;

	m_ActiveTissuelayer = 0;
	m_ColorLookupTable = nullptr;
	m_TissueHierachy = new TissueHiearchy;
	m_Overlay = nullptr;

	m_Loaded = false;
	m_Uelem = nullptr;
	m_Undo3D = true;
}

SlicesHandler::~SlicesHandler() { delete m_TissueHierachy; }

float SlicesHandler::GetWorkPt(Point p, unsigned short slicenr)
{
	return m_ImageSlices[slicenr].WorkPt(p);
}

void SlicesHandler::SetWorkPt(Point p, unsigned short slicenr, float f)
{
	m_ImageSlices[slicenr].SetWorkPt(p, f);
}

float SlicesHandler::GetBmpPt(Point p, unsigned short slicenr)
{
	return m_ImageSlices[slicenr].BmpPt(p);
}

void SlicesHandler::SetBmpPt(Point p, unsigned short slicenr, float f)
{
	m_ImageSlices[slicenr].SetBmpPt(p, f);
}

tissues_size_t SlicesHandler::GetTissuePt(Point p, unsigned short slicenr)
{
	return m_ImageSlices[slicenr].TissuesPt(m_ActiveTissuelayer, p);
}

void SlicesHandler::SetTissuePt(Point p, unsigned short slicenr, tissues_size_t f)
{
	m_ImageSlices[slicenr].SetTissuePt(m_ActiveTissuelayer, p, f);
}

std::vector<const float*> SlicesHandler::SourceSlices() const
{
	std::vector<const float*> ptrs(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		ptrs[i] = m_ImageSlices[i].ReturnBmp();
	}
	return ptrs;
}

std::vector<float*> SlicesHandler::SourceSlices()
{
	std::vector<float*> ptrs(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		ptrs[i] = m_ImageSlices[i].ReturnBmp();
	}
	return ptrs;
}

std::vector<const float*> SlicesHandler::TargetSlices() const
{
	std::vector<const float*> ptrs(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		ptrs[i] = m_ImageSlices[i].ReturnWork();
	}
	return ptrs;
}

std::vector<float*> SlicesHandler::TargetSlices()
{
	std::vector<float*> ptrs(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		ptrs[i] = m_ImageSlices[i].ReturnWork();
	}
	return ptrs;
}

std::vector<const tissues_size_t*> SlicesHandler::TissueSlices(tissuelayers_size_t layeridx) const
{
	std::vector<const tissues_size_t*> ptrs(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		ptrs[i] = m_ImageSlices[i].ReturnTissues(layeridx);
	}
	return ptrs;
}

std::vector<tissues_size_t*> SlicesHandler::TissueSlices(tissuelayers_size_t layeridx)
{
	std::vector<tissues_size_t*> ptrs(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		ptrs[i] = m_ImageSlices[i].ReturnTissues(layeridx);
	}
	return ptrs;
}

std::vector<std::string> SlicesHandler::TissueNames() const
{
	std::vector<std::string> names(TissueInfos::GetTissueCount() + 1);
	names[0] = "Background";
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); i++)
	{
		names.at(i) = TissueInfos::GetTissueName(i);
	}
	return names;
}

std::vector<bool> SlicesHandler::TissueLocks() const
{
	std::vector<bool> locks(TissueInfos::GetTissueCount() + 1, false);
	for (tissues_size_t i = 1; i <= TissueInfos::GetTissueCount(); i++)
	{
		locks.at(i) = TissueInfos::GetTissueLocked(i);
	}
	return locks;
}

float* SlicesHandler::ReturnBmp(unsigned short slicenr1)
{
	return m_ImageSlices[slicenr1].ReturnBmp();
}

float* SlicesHandler::ReturnWork(unsigned short slicenr1)
{
	return m_ImageSlices[slicenr1].ReturnWork();
}

tissues_size_t* SlicesHandler::ReturnTissues(tissuelayers_size_t layeridx, unsigned short slicenr1)
{
	return m_ImageSlices[slicenr1].ReturnTissues(layeridx);
}

float* SlicesHandler::ReturnOverlay() { return m_Overlay; }

int SlicesHandler::LoadDIBitmap(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);
	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = (unsigned short)filenames.size();
	m_Os.SetSizenr(m_Nrslices);
	m_ImageSlices.resize(m_Nrslices);

	int j = 0;
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		j += m_ImageSlices[i].LoadDIBitmap(filenames[i]);
	}

	m_Width = m_ImageSlices[0].ReturnWidth();
	m_Height = m_ImageSlices[0].ReturnHeight();
	m_Area = m_Height * (unsigned int)m_Width;

	NewOverlay();

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		if (m_Width != m_ImageSlices[i].ReturnWidth() ||
				m_Height != m_ImageSlices[i].ReturnHeight())
			j = m_Nrslices + 1;
	}

	if (j == m_Nrslices)
	{
		// Ranges
		Pair dummy;
		m_SliceRanges.resize(m_Nrslices);
		m_SliceBmpranges.resize(m_Nrslices);
		ComputeRangeMode1(&dummy);
		ComputeBmprangeMode1(&dummy);

		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, m_Nrslices);
		return 0;
	}
}

int SlicesHandler::LoadDIBitmap(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Nrslices = (unsigned short)filenames.size();
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(m_Nrslices);
	int j = 0;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		j += m_ImageSlices[i].LoadDIBitmap(filenames[i], p, dx, dy);
	}

	if (j == m_Nrslices)
	{
		// Ranges
		Pair dummy;
		m_SliceRanges.resize(m_Nrslices);
		m_SliceBmpranges.resize(m_Nrslices);
		ComputeRangeMode1(&dummy);
		ComputeBmprangeMode1(&dummy);

		m_Loaded = true;
		m_Width = dx;
		m_Height = dy;
		m_Area = m_Height * (unsigned int)m_Width;

		NewOverlay();
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, m_Nrslices);
		return 0;
	}
}

void SlicesHandler::SetRgbFactors(int redFactor, int greenFactor, int blueFactor)
{
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		m_ImageSlices[i].SetConverterFactors(redFactor, greenFactor, blueFactor);
	}
}

// TODO BL this function has a terrible impl, e.g. using member variables rgb, width/height, etc.
int SlicesHandler::LoadPng(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr); // BL: here we could quantize colors instead and build color

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = (unsigned short)filenames.size();
	m_Os.SetSizenr(m_Nrslices);
	m_ImageSlices.resize(m_Nrslices);

	int j = 0;
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		j += m_ImageSlices[i].LoadPNGBitmap(filenames[i]);
	}

	m_Width = m_ImageSlices[0].ReturnWidth();
	m_Height = m_ImageSlices[0].ReturnHeight();
	m_Area = m_Height * (unsigned int)m_Width;

	NewOverlay();

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		if (m_Width != m_ImageSlices[i].ReturnWidth() ||
				m_Height != m_ImageSlices[i].ReturnHeight())
			j = m_Nrslices + 1;
	}

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(m_Nrslices);
	m_SliceBmpranges.resize(m_Nrslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == m_Nrslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, m_Nrslices);
		return 0;
	}
}

int SlicesHandler::LoadPng(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Nrslices = (unsigned short)filenames.size();
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(m_Nrslices);
	int j = 0;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		j += m_ImageSlices[i].LoadDIBitmap(filenames[i], p, dx, dy);
	}

	m_Width = dx;
	m_Height = dy;
	m_Area = m_Height * (unsigned int)m_Width;

	NewOverlay();

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(m_Nrslices);
	m_SliceBmpranges.resize(m_Nrslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == m_Nrslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, m_Nrslices);
		return 0;
	}
}

int SlicesHandler::LoadDIJpg(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = (unsigned short)filenames.size();
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(m_Nrslices);
	int j = 0;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		j += m_ImageSlices[i].LoadDIBitmap(filenames[i]);
	}

	m_Width = m_ImageSlices[0].ReturnWidth();
	m_Height = m_ImageSlices[0].ReturnHeight();
	m_Area = m_Height * (unsigned int)m_Width;

	NewOverlay();

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		if (m_Width != m_ImageSlices[i].ReturnWidth() ||
				m_Height != m_ImageSlices[i].ReturnHeight())
			j = m_Nrslices + 1;
	}

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(m_Nrslices);
	m_SliceBmpranges.resize(m_Nrslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == m_Nrslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, m_Nrslices);
		return 0;
	}
}

int SlicesHandler::LoadDIJpg(std::vector<const char*> filenames, Point p, unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Nrslices = (unsigned short)filenames.size();
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(m_Nrslices);
	int j = 0;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		j += m_ImageSlices[i].LoadDIBitmap(filenames[i], p, dx, dy);
	}

	m_Width = dx;
	m_Height = dy;
	m_Area = m_Height * (unsigned int)m_Width;

	NewOverlay();

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(m_Nrslices);
	m_SliceBmpranges.resize(m_Nrslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == m_Nrslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, m_Nrslices);
		return 0;
	}
}

int SlicesHandler::ReadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, unsigned short nrofslices)
{
	UpdateColorLookupTable(nullptr);

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Width = w;
	m_Height = h;
	m_Area = m_Height * (unsigned int)m_Width;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = nrofslices;
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += m_ImageSlices[i].ReadRaw(filename, w, h, bitdepth, slicenr + i);

	NewOverlay();

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(nrofslices);
	m_SliceBmpranges.resize(nrofslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == nrofslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		ISEG_WARNING_MSG("loading failed in 'ReadRaw'");
		Newbmp(m_Width, m_Height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawOverlay(const char* filename, unsigned bitdepth, unsigned short slicenr)
{
	FILE* fp;							/* Open file pointer */
	int bitsize = m_Area; /* Size of bitmap */

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
		int result = _fseeki64(fp, (__int64)(bitsize)*m_Nrslices - 1, SEEK_SET);
#else
		int result = fseek(fp, (size_t)(bitsize)*m_Nrslices - 1, SEEK_SET);
#endif
		if (result)
		{
			ISEG_ERROR_MSG("Bmphandler::ReadRawOverlay() : file operation failed");
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

		if (fread(bits_tmp, 1, bitsize, fp) < m_Area)
		{
			ISEG_ERROR_MSG("Bmphandler::ReadRawOverlay() : file operation failed");
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (int i = 0; i < bitsize; i++)
		{
			m_Overlay[i] = (float)bits_tmp[i];
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
			ISEG_ERROR_MSG("Bmphandler::ReadRawOverlay() : file operation failed");
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		if (fread(bits_tmp, 1, (size_t)(bitsize)*2, fp) < m_Area * 2)
		{
			ISEG_ERROR_MSG("Bmphandler::ReadRawOverlay() : file operation failed");
			free(bits_tmp);
			fclose(fp);
			return 0;
		}

		for (int i = 0; i < bitsize; i++)
		{
			m_Overlay[i] = (float)bits_tmp[i];
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
	if (ImageReader::GetInfo(filename, w, h, nrofslices, spacing1, transform1))
	{
		Newbmp(w, h, nrofslices);
		std::vector<float*> slices(m_Nrslices);
		for (unsigned i = 0; i < m_Nrslices; i++)
		{
			slices[i] = m_ImageSlices[i].ReturnBmp();
		}
		ImageReader::GetVolume(filename, slices.data(), m_Nrslices, m_Width, m_Height);
		m_Thickness = spacing1[2];
		m_Dx = spacing1[0];
		m_Dy = spacing1[1];
		m_Transform = transform1;
		m_Loaded = true;

		Bmp2workall();
		return 1;
	}
	return 0;
}

int SlicesHandler::ReadOverlay(const char* filename, unsigned short slicenr)
{
	unsigned w, h, nrofslices;
	float spacing1[3];
	Transform transform1;
	if (ImageReader::GetInfo(filename, w, h, nrofslices, spacing1, transform1))
	{
		return ImageReader::GetVolume(filename, &m_Overlay, slicenr, 1, m_Width, m_Height);
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
	this->m_Activeslice = 0;
	this->m_Width = dims[0];
	this->m_Height = dims[1];
	this->m_Area = m_Height * (unsigned int)m_Width;
	this->m_Nrslices = dims[2];
	this->m_Startslice = 0;
	this->m_Endslice = m_Nrslices;
	this->SetPixelsize(spacing[0], spacing[1]);
	this->m_Thickness = spacing[2];

	// WARNING this might neglect the third column of the "rotation" matrix (e.g. reflections)
	m_Transform.SetTransform(origin, dc);

	this->m_ImageSlices.resize(m_Nrslices);
	this->m_Os.SetSizenr(m_Nrslices);
	this->SetSlicethickness(m_Thickness);
	this->m_Loaded = true;
	for (unsigned short j = 0; j < m_Nrslices; j++)
	{
		this->m_ImageSlices[j].Newbmp(m_Width, m_Height);
	}

	NewOverlay();

	// Pass slice pointers to reader
	std::vector<float*> bmpslices(m_Nrslices);
	for (unsigned i = 0; i < m_Nrslices; i++)
	{
		bmpslices[i] = this->m_ImageSlices[i].ReturnBmp();
	}

	// Read pixel data
	bool res = RTDoseReader::ReadPixelData(filename, bmpslices.data());
	if (res)
	{
		m_Loaded = true;
		Bmp2workall();
	}
	else
	{
		ISEG_ERROR_MSG("ReadPixelData() failed");
		Newbmp(m_Width, m_Height, m_Nrslices);
	}

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(m_Nrslices);
	m_SliceBmpranges.resize(m_Nrslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	SetActiveTissuelayer(0);

	return res;
}

bool SlicesHandler::LoadSurface(const std::string& filename_in, bool overwrite_working, bool intersect)
{
	unsigned dims[3] = {m_Width, m_Height, m_Nrslices};
	auto slices = TargetSlices();

	if (overwrite_working)
	{
		ClearWork();
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
		ret = voxeler.Intersect(surface, slices, dims, Spacing(), m_Transform, m_Startslice, m_Endslice);
	}
	else
	{
		ret = voxeler.Voxelize(surface, slices, dims, Spacing(), m_Transform, m_Startslice, m_Endslice);
	}
	return (ret != VoxelSurface::kNone);
}

int SlicesHandler::LoadAllHDF(const char* filename)
{
	unsigned w, h, nrofslices;
	float* pixsize;
	float* tr_1d;
	float offset[3];
	QStringList array_names;

	HDFImageReader reader;
	reader.SetFileName(filename);
	reader.SetReadContiguousMemory(GetContiguousMemory());
	reader.ParseHDF();
	w = reader.GetWidth();
	h = reader.GetHeight();
	nrofslices = reader.GetNumberOfSlices();
	pixsize = reader.GetPixelSize();
	tr_1d = reader.GetImageTransform();

	if ((w != m_Width) || (h != m_Height) || (m_Nrslices != nrofslices))
	{
		ISEG_ERROR_MSG("LoadAllHDF() : inconsistent dimensions");
		return 0;
	}

	for (int r = 0; r < 3; r++)
	{
		offset[r] = tr_1d[r * 4 + 3];
	}

	// read colors if any
	UpdateColorLookupTable(reader.ReadColorLookup());

	std::vector<float*> bmpslices(m_Nrslices);
	std::vector<float*> workslices(m_Nrslices);
	std::vector<tissues_size_t*> tissueslices(m_Nrslices);
	for (unsigned i = 0; i < m_Nrslices; i++)
	{
		bmpslices[i] = this->m_ImageSlices[i].ReturnBmp();
		workslices[i] = this->m_ImageSlices[i].ReturnWork();
		tissueslices[i] = this->m_ImageSlices[i].ReturnTissues(0); // TODO
	}

	reader.SetImageSlices(bmpslices.data());
	reader.SetWorkSlices(workslices.data());
	reader.SetTissueSlices(tissueslices.data());

	return reader.Read();
}

void SlicesHandler::UpdateColorLookupTable(std::shared_ptr<ColorLookupTable> new_lut /*= nullptr*/)
{
	m_ColorLookupTable = new_lut;

	// BL: todo update slice viewer
}

int SlicesHandler::LoadAllXdmf(const char* filename)
{
	unsigned w, h, nrofslices;
	QStringList array_names;

	XdmfImageReader reader;
	reader.SetFileName(filename);
	reader.SetReadContiguousMemory(GetContiguousMemory());
	reader.ParseXML();
	w = reader.GetWidth();
	h = reader.GetHeight();
	nrofslices = reader.GetNumberOfSlices();
	if ((w != m_Width) || (h != m_Height) || (m_Nrslices != nrofslices))
	{
		ISEG_ERROR_MSG("inconsistent dimensions in LoadAllXdmf");
		return 0;
	}
	auto transform = reader.GetImageTransform();
	float origin[3];
	transform.GetOffset(origin);

	std::vector<float*> bmpslices(m_Nrslices);
	std::vector<float*> workslices(m_Nrslices);
	std::vector<tissues_size_t*> tissueslices(m_Nrslices);

	for (unsigned i = 0; i < m_Nrslices; i++)
	{
		bmpslices[i] = this->m_ImageSlices[i].ReturnBmp();
		workslices[i] = this->m_ImageSlices[i].ReturnWork();
		tissueslices[i] = this->m_ImageSlices[i].ReturnTissues(0); // TODO
	}

	UpdateColorLookupTable(reader.ReadColorLookup());

	reader.SetImageSlices(bmpslices.data());
	reader.SetWorkSlices(workslices.data());
	reader.SetTissueSlices(tissueslices.data());
	int code = reader.Read();

	return code;
}

int SlicesHandler::SaveAllXdmf(const char* filename, int compression, bool save_work, bool naked)
{
	float pixsize[3] = {m_Dx, m_Dy, m_Thickness};

	std::vector<float*> bmpslices(m_Endslice - m_Startslice);
	std::vector<float*> workslices(m_Endslice - m_Startslice);
	std::vector<tissues_size_t*> tissueslices(m_Endslice - m_Startslice);
	for (unsigned i = m_Startslice; i < m_Endslice; i++)
	{
		bmpslices[i - m_Startslice] = m_ImageSlices[i].ReturnBmp();
		workslices[i - m_Startslice] = m_ImageSlices[i].ReturnWork();
		tissueslices[i - m_Startslice] = m_ImageSlices[i].ReturnTissues(0); // TODO
	}

	XdmfImageWriter writer;
	writer.SetCopyToContiguousMemory(GetContiguousMemory());
	writer.SetFileName(filename);
	writer.SetImageSlices(bmpslices.data());
	writer.SetWorkSlices(save_work ? workslices.data() : nullptr);
	writer.SetTissueSlices(tissueslices.data());
	writer.SetNumberOfSlices(m_Endslice - m_Startslice);
	writer.SetWidth(m_Width);
	writer.SetHeight(m_Height);
	writer.SetPixelSize(pixsize);

	auto active_slices_transform = GetTransformActiveSlices();

	writer.SetImageTransform(active_slices_transform);
	writer.SetCompression(compression);
	bool ok = writer.Write(naked);
	ok &= writer.WriteColorLookup(m_ColorLookupTable.get(), naked);
	ok &= TissueInfos::SaveTissuesHDF(filename, m_TissueHierachy->SelectedHierarchy(), naked, 0);
	ok &= SaveMarkersHDF(filename, naked, 0);
	return ok;
}

bool SlicesHandler::SaveMarkersHDF(const char* filename, bool naked, unsigned short version)
{
	int compression = 1;

	QString q_file_name(filename);
	QFileInfo file_info(q_file_name);
	QString basename = file_info.completeBaseName();
	QString suffix = file_info.suffix();

	// save working directory
	QDir oldcwd = QDir::current();

	// enter the xmf file folder so relative names for hdf5 files work
	QDir::setCurrent(file_info.absolutePath());

	HDF5Writer writer;
	QString fname;
	if (naked)
		fname = basename + "." + suffix;
	else
		fname = basename + ".h5";

	if (!writer.Open(fname.toStdString(), "append"))
	{
		ISEG_ERROR("opening " << fname.toStdString());
		return false;
	}
	writer.m_Compression = compression;

	if (!writer.CreateGroup(std::string("Markers")))
	{
		ISEG_ERROR_MSG("creating markers section");
		return false;
	}

	int index1[1];
	std::vector<HDF5Writer::size_type> dim2;
	dim2.resize(1);
	dim2[0] = 1;

	index1[0] = (int)version;
	if (!writer.Write(index1, dim2, std::string("/Markers/version")))
	{
		ISEG_ERROR_MSG("writing version");
		return false;
	}

	float marker_pos[3];
	std::vector<HDF5Writer::size_type> dim1;
	dim1.resize(1);
	dim1[0] = 3;

	int mark_counter = 0;
	for (unsigned i = m_Startslice; i < m_Endslice; i++)
	{
		std::vector<Mark> marks = *(m_ImageSlices[i].ReturnMarks());
		for (const auto& cur_mark : marks)
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
			writer.CreateGroup(mystring);

			marker_pos[0] = cur_mark.p.px * GetPixelsize().high;
			marker_pos[1] = cur_mark.p.py * GetPixelsize().low;
			marker_pos[2] = i * GetSlicethickness();

			if (!writer.Write(marker_pos, dim1, std::string("/Markers/") + mark_name.toLocal8Bit().constData() + std::string("/marker_pos")))
			{
				ISEG_ERROR_MSG("writing marker_pos");
				return false;
			}
		}
	}

	writer.Close();

	QDir::setCurrent(oldcwd.absolutePath());

	return true;
}

int SlicesHandler::SaveMergeAllXdmf(const char* filename, std::vector<QString>& mergeImagefilenames, unsigned short nrslicesTotal, int compression)
{
	float pixsize[3];

	auto active_slices_transform = GetTransformActiveSlices();

	pixsize[0] = m_Dx;
	pixsize[1] = m_Dy;
	pixsize[2] = m_Thickness;

	std::vector<float*> bmpslices(m_Endslice - m_Startslice);
	std::vector<float*> workslices(m_Endslice - m_Startslice);
	std::vector<tissues_size_t*> tissueslices(m_Endslice - m_Startslice);
	for (unsigned i = m_Startslice; i < m_Endslice; i++)
	{
		bmpslices[i - m_Startslice] = m_ImageSlices[i].ReturnBmp();
		workslices[i - m_Startslice] = m_ImageSlices[i].ReturnWork();
		tissueslices[i - m_Startslice] = m_ImageSlices[i].ReturnTissues(0);
	}

	XdmfImageMerger merger;
	merger.SetFileName(filename);
	merger.SetImageSlices(bmpslices.data());
	merger.SetWorkSlices(workslices.data());
	merger.SetTissueSlices(tissueslices.data());
	merger.SetNumberOfSlices(m_Endslice - m_Startslice);
	merger.SetTotalNumberOfSlices(nrslicesTotal);
	merger.SetWidth(m_Width);
	merger.SetHeight(m_Height);
	merger.SetPixelSize(pixsize);
	merger.SetImageTransform(active_slices_transform);
	merger.SetCompression(compression);
	merger.SetMergeFileNames(mergeImagefilenames);
	if (merger.Write())
	{
		bool naked = false;
		TissueInfos::SaveTissuesHDF(filename, m_TissueHierachy->SelectedHierarchy(), naked, 0);
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
	avw::eDataType type;
	float dx1, dy1, thickness1;
	avw::ReadHeader(filename, w, h, nrofslices, dx1, dy1, thickness1, type);
	m_Thickness = thickness1;
	m_Dx = dx1;
	m_Dy = dy1;
	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Width = w;
	m_Height = h;
	m_Area = m_Height * (unsigned int)m_Width;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = nrofslices;
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += m_ImageSlices[i].ReadAvw(filename, i);

	NewOverlay();

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(nrofslices);
	m_SliceBmpranges.resize(nrofslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == nrofslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, unsigned short nrofslices, Point p, unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Width = dx;
	m_Height = dy;
	m_Area = m_Height * (unsigned int)m_Width;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = nrofslices;
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += m_ImageSlices[i]
						 .ReadRaw(filename, w, h, bitdepth, slicenr + i, p, dx, dy);

	NewOverlay();

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(nrofslices);
	m_SliceBmpranges.resize(nrofslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == nrofslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr, unsigned short nrofslices)
{
	UpdateColorLookupTable(nullptr);

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Width = w;
	m_Height = h;
	m_Area = m_Height * (unsigned int)m_Width;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = nrofslices;
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
		j += m_ImageSlices[i].ReadRawFloat(filename, w, h, slicenr + i);

	NewOverlay();

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(nrofslices);
	m_SliceBmpranges.resize(nrofslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == nrofslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr, unsigned short nrofslices, Point p, unsigned short dx, unsigned short dy)
{
	UpdateColorLookupTable(nullptr);

	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Width = dx;
	m_Height = dy;
	m_Area = m_Height * (unsigned int)m_Width;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = nrofslices;
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(nrofslices);
	int j = 0;
	for (unsigned short i = 0; i < nrofslices; i++)
	{
		j += m_ImageSlices[i].ReadRawFloat(filename, w, h, slicenr + i, p, dx, dy);
	}

	NewOverlay();

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(nrofslices);
	m_SliceBmpranges.resize(nrofslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	if (j == nrofslices)
	{
		m_Loaded = true;
		return 1;
	}
	else
	{
		Newbmp(m_Width, m_Height, nrofslices);
		return 0;
	}
}

int SlicesHandler::ReloadDIBitmap(std::vector<const char*> filenames)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;

	if (filenames.size() == m_Nrslices)
	{
		for (unsigned short i = 0; i < m_Nrslices; i++)
		{
			j += m_ImageSlices[i].ReloadDIBitmap(filenames[i]);
		}

		for (unsigned short i = 0; i < m_Nrslices; i++)
		{
			if (m_Width != m_ImageSlices[i].ReturnWidth() ||
					m_Height != m_ImageSlices[i].ReturnHeight())
				j = m_Nrslices + 1;
		}

		if (j == m_Nrslices)
			return 1;
		else
		{
			Newbmp(m_Width, m_Height, m_Nrslices);
			return 0;
		}
	}
	else if (filenames.size() == (m_Endslice - m_Startslice))
	{
		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			j += m_ImageSlices[i].ReloadDIBitmap(filenames[i - m_Startslice]);
		}

		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			if (m_Width != m_ImageSlices[i].ReturnWidth() ||
					m_Height != m_ImageSlices[i].ReturnHeight())
				j = m_Nrslices + 1;
		}

		if (j == (m_Endslice - m_Startslice))
			return 1;
		else
		{
			Newbmp(m_Width, m_Height, m_Nrslices);
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

	if (filenames.size() == m_Nrslices)
	{
		for (unsigned short i = 0; i < m_Nrslices; i++)
		{
			j += m_ImageSlices[i].ReloadDIBitmap(filenames[i], p);
		}

		if (j == m_Nrslices)
			return 1;
		else
		{
			Newbmp(m_Width, m_Height, m_Nrslices);
			return 0;
		}
	}
	else if (filenames.size() == (m_Endslice - m_Startslice))
	{
		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			j += m_ImageSlices[i].ReloadDIBitmap(filenames[i - m_Startslice], p);
		}

		if (j == (m_Endslice - m_Startslice))
			return 1;
		else
		{
			Newbmp(m_Width, m_Height, m_Nrslices);
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
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		j += m_ImageSlices[i]
						 .ReloadRaw(filename, bitdepth, (unsigned)slicenr + i - m_Startslice);

	if (j == (m_Endslice - m_Startslice))
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
	if (ImageReader::GetInfo(filename, w, h, nrofslices, spacing1, transform1))
	{
		if ((w != m_Width) || (h != m_Height) || (m_Endslice + slicenr - m_Startslice > nrofslices))
		{
			return 0;
		}

		std::vector<float*> slices(m_Endslice - m_Startslice);
		for (unsigned i = m_Startslice; i < m_Endslice; i++)
		{
			slices[i - m_Startslice] = m_ImageSlices[i].ReturnBmp();
		}
		ImageReader::GetVolume(filename, slices.data(), slicenr, m_Endslice - m_Startslice, m_Width, m_Height);
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

	if ((dims[0] != m_Width) || (dims[1] != m_Height) ||
			(m_Endslice - m_Startslice + slicenr > dims[2]))
		return 0;

	// Pass slice pointers to reader
	std::vector<float*> bmpslices(m_Endslice - m_Startslice);
	for (unsigned i = m_Startslice; i < m_Endslice; i++)
	{
		bmpslices[i - m_Startslice] = this->m_ImageSlices[i].ReturnBmp();
	}

	// Read pixel data
	bool res = RTDoseReader::ReadPixelData(filename, bmpslices.data());
	if (res)
	{
		m_Loaded = true;
	}
	else
	{
		ISEG_ERROR_MSG("ReloadRTdose() : ReadPixelData() failed");
		Newbmp(m_Width, m_Height, m_Nrslices);
	}

	return res;
#endif
}

int SlicesHandler::ReloadAVW(const char* filename, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	unsigned short w, h, nrofslices;
	avw::eDataType type;
	float dx1, dy1, thickness1;
	avw::ReadHeader(filename, w, h, nrofslices, dx1, dy1, thickness1, type);
	if ((w != m_Width) || (h != m_Height) || (m_Endslice > nrofslices))
		return 0;
	if (h != m_Height)
		return 0;

	int j = 0;
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		j += m_ImageSlices[i].ReadAvw(filename, i);

	if (j == (m_Endslice - m_Startslice))
		return 1;
	return 0;
}

int SlicesHandler::ReloadRaw(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, Point p)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		j += m_ImageSlices[i]
						 .ReloadRaw(filename, w, h, bitdepth, (unsigned)slicenr + i - m_Startslice, p);

	if (j == (m_Endslice - m_Startslice))
		return 1;
	else
		return 0;
}

std::vector<float*> SlicesHandler::LoadRawFloat(const char* filename, unsigned short startslice, unsigned short endslice, unsigned short slicenr, unsigned int area)
{
	std::vector<float*> slices_red;
	for (unsigned short i = startslice; i < endslice; i++)
	{
		float* slice_red = Bmphandler::ReadRawFloat(filename, (unsigned)slicenr + i - startslice, area);
		slices_red.push_back(slice_red);
	}
	return slices_red;
}

int SlicesHandler::ReloadRawFloat(const char* filename, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		j += m_ImageSlices[i]
						 .ReloadRawFloat(filename, (unsigned)slicenr + i - m_Startslice);

	if (j == (m_Endslice - m_Startslice))
		return 1;
	else
		return 0;
}

int SlicesHandler::ReloadRawFloat(const char* filename, short unsigned w, short unsigned h, unsigned short slicenr, Point p)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		j += m_ImageSlices[i]
						 .ReloadRawFloat(filename, w, h, (unsigned)slicenr + i - m_Startslice, p);

	if (j == (m_Endslice - m_Startslice))
		return 1;
	else
		return 0;
}

int SlicesHandler::ReloadRawTissues(const char* filename, unsigned bitdepth, unsigned short slicenr)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		j += m_ImageSlices[i]
						 .ReloadRawTissues(filename, bitdepth, (unsigned)slicenr + i - m_Startslice);

	if (j == (m_Endslice - m_Startslice))
		return 1;
	else
		return 0;
}

int SlicesHandler::ReloadRawTissues(const char* filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, Point p)
{
	UpdateColorLookupTable(nullptr);

	int j = 0;
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		j += m_ImageSlices[i]
						 .ReloadRawTissues(filename, w, h, bitdepth, (unsigned)slicenr + i - m_Startslice, p);

	if (j == (m_Endslice - m_Startslice))
		return 1;
	else
		return 0;
}

FILE* SlicesHandler::SaveHeader(FILE* fp, short unsigned nr_slices_to_write, Transform transform_to_write)
{
	fwrite(&nr_slices_to_write, 1, sizeof(unsigned short), fp);
	float thick = -m_Thickness;
	if (thick > 0)
		thick = -thick;
	fwrite(&thick, 1, sizeof(float), fp);
	fwrite(&m_Dx, 1, sizeof(float), fp);
	fwrite(&m_Dy, 1, sizeof(float), fp);
	int combined_version =
			iseg::CombineTissuesVersion(project_version, tissue_version);
	fwrite(&combined_version, 1, sizeof(int), fp);

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

FILE* SlicesHandler::SaveProject(const char* filename, const char* imageFileExtension)
{
	FILE* fp;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return nullptr;

	fp = SaveHeader(fp, m_Nrslices, m_Transform);

	for (unsigned short j = 0; j < m_Nrslices; j++)
	{
		fp = m_ImageSlices[j].SaveProj(fp, false);
	}
	fp = (m_ImageSlices[0]).SaveStack(fp);

	// SaveAllXdmf uses startslice/endslice to decide what to write - here we want to override that behavior
	unsigned short startslice1 = m_Startslice;
	unsigned short endslice1 = m_Endslice;
	m_Startslice = 0;
	m_Endslice = m_Nrslices;
	unsigned char length1 = 0;
	while (imageFileExtension[length1] != '\0')
		length1++;
	length1++;
	fwrite(&length1, 1, sizeof(unsigned char), fp);
	fwrite(imageFileExtension, length1, sizeof(char), fp);
	QString image_file_name = QString(filename);
	int after_dot = image_file_name.lastIndexOf('.') + 1;
	image_file_name =
			image_file_name.remove(after_dot, image_file_name.length() - after_dot) +
			imageFileExtension;
	SaveAllXdmf(QFileInfo(filename).dir().absFilePath(image_file_name).toAscii().data(), this->m_Hdf5Compression, this->m_SaveTarget, false);

	m_Startslice = startslice1;
	m_Endslice = endslice1;

	return fp;
}

bool SlicesHandler::SaveCommunicationFile(const char* filename)
{
	unsigned short startslice1 = m_Startslice;
	unsigned short endslice1 = m_Endslice;
	m_Startslice = 0;
	m_Endslice = m_Nrslices;

	SaveAllXdmf(QFileInfo(filename).dir().absFilePath(filename).toAscii().data(), this->m_Hdf5Compression, this->m_SaveTarget, true);

	m_Startslice = startslice1;
	m_Endslice = endslice1;

	return true;
}

FILE* SlicesHandler::SaveActiveSlices(const char* filename, const char* imageFileExtension)
{
	FILE* fp;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return nullptr;

	unsigned short slicecount = m_Endslice - m_Startslice;
	Transform transform_corrected = GetTransformActiveSlices();
	SaveHeader(fp, slicecount, transform_corrected);

	for (unsigned short j = m_Startslice; j < m_Endslice; j++)
	{
		fp = m_ImageSlices[j].SaveProj(fp, false);
	}
	fp = (m_ImageSlices[0]).SaveStack(fp);
	unsigned char length1 = 0;
	while (imageFileExtension[length1] != '\0')
		length1++;
	length1++;
	fwrite(&length1, 1, sizeof(unsigned char), fp);
	fwrite(imageFileExtension, length1, sizeof(char), fp);
	QString image_file_name = QString(filename);
	int after_dot = image_file_name.lastIndexOf('.') + 1;
	image_file_name =
			image_file_name.remove(after_dot, image_file_name.length() - after_dot) +
			imageFileExtension;
	SaveAllXdmf(QFileInfo(filename).dir().absFilePath(image_file_name).toAscii().data(), this->m_Hdf5Compression, this->m_SaveTarget, false);

	return fp;
}

FILE* SlicesHandler::MergeProjects(const char* savefilename, std::vector<QString>& mergeFilenames)
{
	// Get number of slices to total
	unsigned short nrslices_total = m_Nrslices;
	for (unsigned short i = 0; i < mergeFilenames.size(); i++)
	{
		if (!mergeFilenames[i].endsWith(".prj"))
		{
			return nullptr;
		}

		FILE* fp_merge;
		// Add number of slices to total
		unsigned short nrslices_merge = 0;
		if ((fp_merge = fopen(mergeFilenames[i].toAscii().data(), "rb")) == nullptr)
			return nullptr;
		fread(&nrslices_merge, sizeof(unsigned short), 1, fp_merge);
		nrslices_total += nrslices_merge;
		fclose(fp_merge);
	}

	// Save merged project
	FILE* fp;
	if ((fp = fopen(savefilename, "wb")) == nullptr)
		return nullptr;

	/// BL TODO what should merged transform be
	fp = SaveHeader(fp, nrslices_total, m_Transform);

	// Save current project slices
	for (unsigned short j = 0; j < m_Nrslices; j++)
	{
		fp = m_ImageSlices[j].SaveProj(fp, false);
	}

	FILE* fp_merge;
	// Save merged project slices
	for (unsigned short i = 0; i < mergeFilenames.size(); i++)
	{
		if ((fp_merge = fopen(mergeFilenames[i].toAscii().data(), "rb")) == nullptr)
		{
			return nullptr;
		}

		// Skip header
		int version = 0;
		int tissues_version = 0;
		SlicesHandler dummy_slices_handler;
		dummy_slices_handler.LoadHeader(fp_merge, tissues_version, version);

		unsigned short merge_nrslices = dummy_slices_handler.NumSlices();
		//float mergeThickness = dummy_SlicesHandler.get_slicethickness();

		// Load input slices and save to merged project file
		Bmphandler tmp_slice;
		for (unsigned short j = 0; j < merge_nrslices; j++)
		{
			fp_merge = tmp_slice.LoadProj(fp_merge, tissues_version, false);
			fp = tmp_slice.SaveProj(fp, false);
		}

		fclose(fp_merge);
	}

	fp = (m_ImageSlices[0]).SaveStack(fp);

	unsigned short startslice1 = m_Startslice;
	unsigned short endslice1 = m_Endslice;
	m_Startslice = 0;
	m_Endslice = m_Nrslices;
	QString image_file_extension = "xmf";
	unsigned char length1 = 0;
	while (image_file_extension[length1] != '\0')
		length1++;
	length1++;
	fwrite(&length1, 1, sizeof(unsigned char), fp);
	fwrite(image_file_extension, length1, sizeof(char), fp);

	QString image_file_name = QString(savefilename);
	int after_dot = image_file_name.lastIndexOf('.') + 1;
	image_file_name = image_file_name.remove(after_dot, image_file_name.length() - after_dot) + image_file_extension;

	auto image_file_path = QFileInfo(savefilename).dir().absFilePath(image_file_name).toStdString();
	if (!SaveMergeAllXdmf(image_file_path.c_str(), mergeFilenames, nrslices_total, this->m_Hdf5Compression))
	{
		return nullptr;
	}

	m_Startslice = startslice1;
	m_Endslice = endslice1;
	return fp;
}

void SlicesHandler::LoadHeader(FILE* fp, int& tissuesVersion, int& version)
{
	m_Activeslice = 0;

	fread(&m_Nrslices, sizeof(unsigned short), 1, fp);
	m_Startslice = 0;
	m_Endslice = m_Nrslices;
	fread(&m_Thickness, sizeof(float), 1, fp);

	//    set_slicethickness(thickness);
	fread(&m_Dx, sizeof(float), 1, fp);
	fread(&m_Dy, sizeof(float), 1, fp);
	SetPixelsize(m_Dx, m_Dy);

	version = 0;
	tissuesVersion = 0;
	if (m_Thickness < 0)
	{
		m_Thickness = -m_Thickness;
		int combined_version;
		fread(&combined_version, sizeof(int), 1, fp);
		iseg::ExtractTissuesVersion(combined_version, version, tissuesVersion);
	}

	if (version < 5)
	{
		float displacement[3];
		float direction_cosines[6];
		Transform::SetIdentity(displacement, direction_cosines);

		if (version > 0)
		{
			fread(&(displacement[0]), sizeof(float), 1, fp);
			fread(&(displacement[1]), sizeof(float), 1, fp);
			fread(&(displacement[2]), sizeof(float), 1, fp);
		}
		if (version > 3)
		{
			for (unsigned short i = 0; i < 6; ++i)
			{
				fread(&(direction_cosines[i]), sizeof(float), 1, fp);
			}
		}
		m_Transform.SetTransform(displacement, direction_cosines);
	}
	else
	{
		for (int r = 0; r < 4; r++)
		{
			float* transform_r = m_Transform[r];
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

	m_ImageSlices.resize(m_Nrslices);

	m_Os.SetSizenr(m_Nrslices);

	for (unsigned short j = 0; j < m_Nrslices; ++j)
	{
		// skip initializing because we load real data into the arrays below
		fp = m_ImageSlices[j].LoadProj(fp, tissuesVersion, version <= 1, false);
	}

	SetSlicethickness(m_Thickness);

	fp = (m_ImageSlices[0]).LoadStack(fp);

	m_Width = (m_ImageSlices[0]).ReturnWidth();
	m_Height = (m_ImageSlices[0]).ReturnHeight();
	m_Area = m_Height * (unsigned int)m_Width;

	NewOverlay();

	if (version > 1)
	{
		QString image_file_name;
		if (version > 2)
		{
			// Only load image file name extension
			char image_file_ext[10];
			unsigned char length1;
			fread(&length1, sizeof(unsigned char), 1, fp);
			fread(image_file_ext, sizeof(char), length1, fp);
			image_file_name = QString(filename);
			int after_dot = image_file_name.lastIndexOf('.') + 1;
			image_file_name = image_file_name.remove(after_dot, image_file_name.length() - after_dot) + QString(image_file_ext);
		}
		else
		{
			// Load full image file name
			char filenameimage[200];
			unsigned char length1;
			fread(&length1, sizeof(unsigned char), 1, fp);
			fread(filenameimage, sizeof(char), length1, fp);
			image_file_name = QString(filenameimage);
		}

		if (image_file_name.endsWith(".xmf", Qt::CaseInsensitive))
		{
			LoadAllXdmf(QFileInfo(filename).dir().absFilePath(image_file_name).toAscii().data());
		}
		else
		{
			ISEG_ERROR_MSG("unsupported format...");
		}
	}

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(m_Nrslices);
	m_SliceBmpranges.resize(m_Nrslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	m_Loaded = true;

	return fp;
}

bool SlicesHandler::LoadS4Llink(const char* filename, int& tissuesVersion)
{
	unsigned w, h, nrofslices;
	float* pixsize;
	float* tr_1d;
	QStringList array_names;

	HDFImageReader reader;
	reader.SetFileName(filename);
	reader.SetReadContiguousMemory(GetContiguousMemory());
	reader.ParseHDF();
	w = reader.GetWidth();
	h = reader.GetHeight();
	nrofslices = reader.GetNumberOfSlices();
	pixsize = reader.GetPixelSize();
	tr_1d = reader.GetImageTransform();

	// taken from LoadProject()
	m_Activeslice = 0;
	this->m_Width = w;
	this->m_Height = h;
	this->m_Area = m_Height * (unsigned int)m_Width;
	this->m_Nrslices = nrofslices;
	this->m_Startslice = 0;
	this->m_Endslice = m_Nrslices;
	this->SetPixelsize(pixsize[0], pixsize[1]);
	this->m_Thickness = pixsize[2];

	float* transform_1d = m_Transform[0];
	std::copy(tr_1d, tr_1d + 16, transform_1d);

	this->m_ImageSlices.resize(m_Nrslices);
	this->m_Os.SetSizenr(m_Nrslices);
	this->SetSlicethickness(m_Thickness);
	this->m_Loaded = true;
	for (unsigned short j = 0; j < m_Nrslices; j++)
	{
		this->m_ImageSlices[j].Newbmp(w, h);
	}

	NewOverlay();

	bool res = LoadAllHDF(filename);

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(nrofslices);
	m_SliceBmpranges.resize(nrofslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	SetActiveTissuelayer(0);

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

	bits_tmp = (unsigned char*)malloc(sizeof(unsigned char) * m_Area);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = m_Width * (unsigned)m_Height;

	for (unsigned short j = 0; j < m_Nrslices; j++)
	{
		if (work)
			p_bits = m_ImageSlices[j].ReturnWork();
		else
			p_bits = m_ImageSlices[j].ReturnBmp();
		for (unsigned int i = 0; i < m_Area; i++)
		{
			bits_tmp[i] = (unsigned char)(std::min(255.0, std::max(0.0, p_bits[i] + 0.5)));
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

	unsigned int bitsize = m_Width * (unsigned)m_Height;

	if ((TissueInfos::GetTissueCount() <= 255) &&
			(sizeof(tissues_size_t) > sizeof(unsigned char)))
	{
		unsigned char* uchar_buffer = new unsigned char[bitsize];
		for (unsigned short j = 0; j < m_Nrslices; j++)
		{
			bits_tmp = m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer);
			for (unsigned int i = 0; i < bitsize; ++i)
			{
				uchar_buffer[i] = (unsigned char)bits_tmp[i];
			}
			if (fwrite(uchar_buffer, sizeof(unsigned char), bitsize, fp) <
					(unsigned int)bitsize)
			{
				fclose(fp);
				return (-1);
			}
		}
		delete[] uchar_buffer;
	}
	else
	{
		for (unsigned short j = 0; j < m_Nrslices; j++)
		{
			bits_tmp = m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer);
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

int SlicesHandler::SaveRawResized(const char* filename, int dxm, int dxp, int dym, int dyp, int dzm, int dzp, bool work)
{
	if ((-(dxm + dxp) >= m_Width) || (-(dym + dyp) >= m_Height) || (-(dzm + dzp) >= m_Nrslices))
		return (-1);
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	int l2 = m_Width + dxm + dxp;

	unsigned newarea =
			(unsigned)(m_Width + dxm + dxp) * (unsigned)(m_Height + dym + dyp);
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
	int h1 = m_Height;
	if (dym < 0)
	{
		h1 += dym;
		posstart1 += m_Width * unsigned(-dym);
	}
	else
	{
		posstart2 += l2 * unsigned(dym);
	}
	if (dyp < 0)
		h1 += dyp;

	for (unsigned short j = (unsigned short)std::max(0, -dzm);
			 j < m_Nrslices - (unsigned short)std::max(0, -dzp); j++)
	{
		if (work)
			p_bits = m_ImageSlices[j].ReturnWork();
		else
			p_bits = m_ImageSlices[j].ReturnBmp();

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

int SlicesHandler::SaveTissuesRawResized(const char* filename, int dxm, int dxp, int dym, int dyp, int dzm, int dzp)
{
	if ((-(dxm + dxp) >= m_Width) || (-(dym + dyp) >= m_Height) || (-(dzm + dzp) >= m_Nrslices))
		return (-1);
	FILE* fp;
	tissues_size_t* bits_tmp;

	int l2 = m_Width + dxm + dxp;

	unsigned newarea =
			(unsigned)(m_Width + dxm + dxp) * (unsigned)(m_Height + dym + dyp);
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
	int h1 = m_Height;
	if (dym < 0)
	{
		h1 += dym;
		posstart1 += m_Width * unsigned(-dym);
	}
	else
	{
		posstart2 += l2 * unsigned(dym);
	}
	if (dyp < 0)
		h1 += dyp;

	for (unsigned short j = (unsigned short)std::max(0, -dzm);
			 j < m_Nrslices - (unsigned short)std::max(0, -dzp); j++)
	{
		tissues_size_t* p_bits =
				m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer);
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
	w = Height();
	h = Width();
	nrslices = NumSlices();
	QString str1;
	bool ok = true;
	unsigned char mode1 = GetActivebmphandler()->ReturnMode(true);
	unsigned char mode2 = GetActivebmphandler()->ReturnMode(false);

	str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
	if (SaveRawXySwapped(str1.ascii(), false) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("work_float.raw"));
	if (SaveRawXySwapped(str1.ascii(), true) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("tissues.raw"));
	if (SaveTissuesRawXySwapped(str1.ascii()) != 0)
		ok = false;

	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("work_float.raw"));
		if (ReadRawFloat(str1.ascii(), w, h, 0, nrslices) != 1)
			ok = false;
		SetModeall(mode1, true);
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
		if (ReloadRawFloat(str1.ascii(), 0) != 1)
			ok = false;
		SetModeall(mode2, false);
	}
	if (ok)
	{
		str1 = QDir::temp().absFilePath(QString("tissues.raw"));
		if (ReloadRawTissues(str1.ascii(), sizeof(tissues_size_t) * 8, 0) != 1)
			ok = false;
	}

	// BL TODO direction cosines don't reflect full transform
	float disp[3];
	GetDisplacement(disp);
	float dummy = disp[1];
	disp[1] = disp[0];
	disp[0] = dummy;
	SetDisplacement(disp);
	float dc[6];
	GetDirectionCosines(dc);
	std::swap(dc[0], dc[3]);
	std::swap(dc[1], dc[4]);
	std::swap(dc[2], dc[5]);
	SetDirectionCosines(dc);

	// add color lookup table again
	UpdateColorLookupTable(lut);

	SliceProviderInstaller::Getinst()->Report();

	return ok;
}

bool SlicesHandler::SwapYZ()
{
	auto lut = GetColorLookupTable();

	unsigned short w, h, nrslices;
	w = Width();
	h = NumSlices();
	nrslices = Height();
	QString str1;
	bool ok = true;
	str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
	if (SaveRawYzSwapped(str1.ascii(), false) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("work_float.raw"));
	if (SaveRawYzSwapped(str1.ascii(), true) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("tissues.raw"));
	if (SaveTissuesRawYzSwapped(str1.ascii()) != 0)
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
	GetDisplacement(disp);
	float dummy = disp[2];
	disp[2] = disp[1];
	disp[1] = dummy;
	SetDisplacement(disp);
	float dc[6];
	GetDirectionCosines(dc);
	float cross[3]; // direction cosines of z-axis (in image coordinate frame)
	vtkMath::Cross(&dc[0], &dc[3], cross);
	vtkMath::Normalize(cross);
	for (unsigned short i = 0; i < 3; ++i)
	{
		dc[i + 3] = cross[i];
	}
	SetDirectionCosines(dc);

	// add color lookup table again
	UpdateColorLookupTable(lut);

	SliceProviderInstaller::Getinst()->Report();

	return ok;
}

bool SlicesHandler::SwapXZ()
{
	auto lut = GetColorLookupTable();

	unsigned short w, h, nrslices;
	w = NumSlices();
	h = Height();
	nrslices = Width();
	//Pair p = get_pixelsize();
	//float thick = get_slicethickness();
	QString str1;
	bool ok = true;
	str1 = QDir::temp().absFilePath(QString("bmp_float.raw"));
	if (SaveRawXzSwapped(str1.ascii(), false) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("work_float.raw"));
	if (SaveRawXzSwapped(str1.ascii(), true) != 0)
		ok = false;
	str1 = QDir::temp().absFilePath(QString("tissues.raw"));
	if (SaveTissuesRawXzSwapped(str1.ascii()) != 0)
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
	GetDisplacement(disp);
	float dummy = disp[2];
	disp[2] = disp[0];
	disp[0] = dummy;
	SetDisplacement(disp);
	float dc[6];
	GetDirectionCosines(dc);
	float cross[3]; // direction cosines of z-axis (in image coordinate frame)
	vtkMath::Cross(&dc[0], &dc[3], cross);
	vtkMath::Normalize(cross);
	for (unsigned short i = 0; i < 3; ++i)
	{
		dc[i] = cross[i];
	}
	SetDirectionCosines(dc);

	// add color lookup table again
	UpdateColorLookupTable(lut);

	SliceProviderInstaller::Getinst()->Report();

	return ok;
}

int SlicesHandler::SaveRawXySwapped(const char* filename, bool work)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	bits_tmp = (float*)malloc(sizeof(float) * m_Area);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = m_Width * (unsigned)m_Height;

	for (unsigned short j = 0; j < m_Nrslices; j++)
	{
		if (work)
			p_bits = m_ImageSlices[j].ReturnWork();
		else
			p_bits = m_ImageSlices[j].ReturnBmp();
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short y = 0; y < m_Height; y++)
		{
			pos2 = y;
			for (unsigned short x = 0; x < m_Width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2 += m_Height;
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

int SlicesHandler::SaveRawXySwapped(const char* filename, std::vector<float*> bits_to_swap, short unsigned width, short unsigned height, short unsigned nrslices)
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

int SlicesHandler::SaveRawXzSwapped(const char* filename, bool work)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	unsigned int bitsize = m_Nrslices * (unsigned)m_Height;
	bits_tmp = (float*)malloc(sizeof(float) * bitsize);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	for (unsigned short x = 0; x < m_Width; x++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < m_Nrslices; z++)
		{
			if (work)
				p_bits = (m_ImageSlices[z]).ReturnWork();
			else
				p_bits = (m_ImageSlices[z]).ReturnBmp();
			pos2 = z;
			pos1 = x;
			for (unsigned short y = 0; y < m_Height; y++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1 += m_Width;
				pos2 += m_Nrslices;
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

int SlicesHandler::SaveRawXzSwapped(const char* filename, std::vector<float*> bits_to_swap, short unsigned width, short unsigned height, short unsigned nrslices)
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

int SlicesHandler::SaveRawYzSwapped(const char* filename, bool work)
{
	FILE* fp;
	float* bits_tmp;
	float* p_bits;

	unsigned int bitsize = m_Nrslices * (unsigned)m_Width;
	bits_tmp = (float*)malloc(sizeof(float) * bitsize);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	for (unsigned short y = 0; y < m_Height; y++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < m_Nrslices; z++)
		{
			if (work)
				p_bits = (m_ImageSlices[z]).ReturnWork();
			else
				p_bits = (m_ImageSlices[z]).ReturnBmp();
			pos2 = z * m_Width;
			pos1 = y * m_Width;
			for (unsigned short x = 0; x < m_Width; x++)
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

int SlicesHandler::SaveRawYzSwapped(const char* filename, std::vector<float*> bits_to_swap, short unsigned width, short unsigned height, short unsigned nrslices)
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

	unsigned int bitsize = m_Width * (unsigned)m_Height;

	for (unsigned short j = 0; j < m_Nrslices; j++)
	{
		bits_tmp = m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer);
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

int SlicesHandler::SaveTissuesRawXySwapped(const char* filename)
{
	FILE* fp;
	tissues_size_t* bits_tmp;
	tissues_size_t* p_bits;

	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	unsigned int bitsize = m_Width * (unsigned)m_Height;

	for (unsigned short j = 0; j < m_Nrslices; j++)
	{
		p_bits = m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer); // TODO
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short y = 0; y < m_Height; y++)
		{
			pos2 = y;
			for (unsigned short x = 0; x < m_Width; x++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1++;
				pos2 += m_Height;
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

int SlicesHandler::SaveTissuesRawXzSwapped(const char* filename)
{
	FILE* fp;
	tissues_size_t* bits_tmp;
	tissues_size_t* p_bits;

	unsigned int bitsize = m_Nrslices * (unsigned)m_Height;
	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * bitsize);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	for (unsigned short x = 0; x < m_Width; x++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < m_Nrslices; z++)
		{
			p_bits =
					(m_ImageSlices[z]).ReturnTissues(m_ActiveTissuelayer); // TODO
			pos2 = z;
			pos1 = x;
			for (unsigned short y = 0; y < m_Height; y++)
			{
				bits_tmp[pos2] = p_bits[pos1];
				pos1 += m_Width;
				pos2 += m_Nrslices;
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

int SlicesHandler::SaveTissuesRawYzSwapped(const char* filename)
{
	FILE* fp;
	tissues_size_t* bits_tmp;
	tissues_size_t* p_bits;

	unsigned int bitsize = m_Nrslices * (unsigned)m_Width;
	bits_tmp = (tissues_size_t*)malloc(sizeof(tissues_size_t) * bitsize);
	if (bits_tmp == nullptr)
		return -1;

	if ((fp = fopen(filename, "wb")) == nullptr)
		return (-1);

	for (unsigned short y = 0; y < m_Height; y++)
	{
		unsigned pos1, pos2;
		pos1 = 0;
		for (unsigned short z = 0; z < m_Nrslices; z++)
		{
			p_bits =
					(m_ImageSlices[z]).ReturnTissues(m_ActiveTissuelayer); // TODO
			pos2 = z * m_Width;
			pos1 = y * m_Width;
			for (unsigned short x = 0; x < m_Width; x++)
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

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += m_ImageSlices[i].SaveDIBitmap(name);
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

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += m_ImageSlices[i].SaveWorkBitmap(name);
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

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		sprintf(name, "%s%u.bmp", filename, i);
		j += m_ImageSlices[i].SaveTissueBitmap(m_ActiveTissuelayer, name);
	}

	if (j == 0)
	{
		return 1;
	}
	else
		return 0;
}

void SlicesHandler::Work2bmp() { (m_ImageSlices[m_Activeslice]).Work2bmp(); }

void SlicesHandler::Bmp2work() { (m_ImageSlices[m_Activeslice]).Bmp2work(); }

void SlicesHandler::SwapBmpwork()
{
	(m_ImageSlices[m_Activeslice]).SwapBmpwork();
}

void SlicesHandler::Work2bmpall()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].Work2bmp();
}

void SlicesHandler::Bmp2workall()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].Bmp2work();
}

void SlicesHandler::Work2tissueall()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].Work2tissue(m_ActiveTissuelayer);
}

void SlicesHandler::Mergetissues(tissues_size_t tissuetype)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].Mergetissue(tissuetype, m_ActiveTissuelayer);
}

void SlicesHandler::Tissue2workall()
{
	m_ImageSlices[m_Activeslice].Tissue2work(m_ActiveTissuelayer);
}

void SlicesHandler::Tissue2workall3D()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].Tissue2work(m_ActiveTissuelayer);
}

void SlicesHandler::SwapBmpworkall()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].SwapBmpwork();
}

void SlicesHandler::AddMark(Point p, unsigned label, std::string str)
{
	(m_ImageSlices[m_Activeslice]).AddMark(p, label, str);
}

void SlicesHandler::ClearMarks()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		(m_ImageSlices[i]).ClearMarks();
}

bool SlicesHandler::RemoveMark(Point p, unsigned radius)
{
	return (m_ImageSlices[m_Activeslice]).RemoveMark(p, radius);
}

void SlicesHandler::AddMark(Point p, unsigned label, unsigned short slicenr, std::string str)
{
	if (slicenr < m_Nrslices && slicenr >= 0)
		(m_ImageSlices[slicenr]).AddMark(p, label, str);
}

bool SlicesHandler::RemoveMark(Point p, unsigned radius, unsigned short slicenr)
{
	if (slicenr < m_Nrslices && slicenr >= 0)
		return (m_ImageSlices[slicenr]).RemoveMark(p, radius);
	else
		return false;
}

void SlicesHandler::GetLabels(std::vector<AugmentedMark>* labels)
{
	labels->clear();
	std::vector<Mark> labels1;
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		m_ImageSlices[i].GetLabels(&labels1);
		for (size_t j = 0; j < labels1.size(); j++)
		{
			AugmentedMark am;
			am.mark = labels1[j].mark;
			am.p = labels1[j].p;
			am.name = labels1[j].name;
			am.slicenr = i;
			labels->push_back(am);
		}
	}
}

void SlicesHandler::AddVm(std::vector<Mark>* vm1)
{
	(m_ImageSlices[m_Activeslice]).AddVm(vm1);
}

bool SlicesHandler::RemoveVm(Point p, unsigned radius)
{
	return (m_ImageSlices[m_Activeslice]).DelVm(p, radius);
}

void SlicesHandler::AddVm(std::vector<Mark>* vm1, unsigned short slicenr)
{
	if (slicenr < m_Nrslices && slicenr >= 0)
		(m_ImageSlices[slicenr]).AddVm(vm1);
}

bool SlicesHandler::RemoveVm(Point p, unsigned radius, unsigned short slicenr)
{
	if (slicenr < m_Nrslices && slicenr >= 0)
		return (m_ImageSlices[slicenr]).DelVm(p, radius);
	else
		return false;
}

void SlicesHandler::AddLimit(std::vector<Point>* vp1)
{
	(m_ImageSlices[m_Activeslice]).AddLimit(vp1);
}

bool SlicesHandler::RemoveLimit(Point p, unsigned radius)
{
	return (m_ImageSlices[m_Activeslice]).DelLimit(p, radius);
}

void SlicesHandler::AddLimit(std::vector<Point>* vp1, unsigned short slicenr)
{
	if (slicenr < m_Nrslices && slicenr >= 0)
		(m_ImageSlices[slicenr]).AddLimit(vp1);
}

bool SlicesHandler::RemoveLimit(Point p, unsigned radius, unsigned short slicenr)
{
	if (slicenr < m_Nrslices && slicenr >= 0)
		return (m_ImageSlices[slicenr]).DelLimit(p, radius);
	else
		return false;
}

void SlicesHandler::Newbmp(unsigned short width1, unsigned short height1, unsigned short nrofslices, const std::function<void(float**)>& init_callback)
{
	m_Activeslice = 0;
	m_Startslice = 0;
	m_Endslice = m_Nrslices = nrofslices;
	m_Os.SetSizenr(m_Nrslices);
	m_ImageSlices.resize(nrofslices);

	for (unsigned short i = 0; i < m_Nrslices; i++)
		m_ImageSlices[i].Newbmp(width1, height1);

	// now that memory is allocated give callback a chance to 'initialize' the data
	if (init_callback)
	{
		init_callback(SourceSlices().data());
	}

	// Ranges
	Pair dummy;
	m_SliceRanges.resize(nrofslices);
	m_SliceBmpranges.resize(nrofslices);
	ComputeRangeMode1(&dummy);
	ComputeBmprangeMode1(&dummy);

	m_Loaded = true;

	m_Width = width1;
	m_Height = height1;
	m_Area = m_Height * (unsigned int)m_Width;
	SetActiveTissuelayer(0);

	NewOverlay();
}

void SlicesHandler::Freebmp()
{
	for (unsigned short i = 0; i < m_Nrslices; i++)
		m_ImageSlices[i].Freebmp();

	m_Loaded = false;
}

void SlicesHandler::ClearBmp()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].ClearBmp();
}

void SlicesHandler::ClearWork()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].ClearWork();
}

void SlicesHandler::ClearOverlay()
{
	for (unsigned int i = 0; i < m_Area; i++)
		m_Overlay[i] = 0.0f;
}

void SlicesHandler::NewOverlay()
{
	if (m_Overlay != nullptr)
		free(m_Overlay);
	m_Overlay = (float*)malloc(sizeof(float) * m_Area);
	ClearOverlay();
}

void SlicesHandler::SetBmp(unsigned short slicenr, float* bits, unsigned char mode)
{
	(m_ImageSlices[slicenr]).SetBmp(bits, mode);
}

void SlicesHandler::SetWork(unsigned short slicenr, float* bits, unsigned char mode)
{
	(m_ImageSlices[slicenr]).SetWork(bits, mode);
}

void SlicesHandler::SetTissue(unsigned short slicenr, tissues_size_t* bits)
{
	(m_ImageSlices[slicenr]).SetTissue(m_ActiveTissuelayer, bits);
}

void SlicesHandler::Copy2bmp(unsigned short slicenr, float* bits, unsigned char mode)
{
	(m_ImageSlices[slicenr]).Copy2bmp(bits, mode);
}

void SlicesHandler::Copy2work(unsigned short slicenr, float* bits, unsigned char mode)
{
	(m_ImageSlices[slicenr]).Copy2work(bits, mode);
}

void SlicesHandler::Copy2tissue(unsigned short slicenr, tissues_size_t* bits)
{
	(m_ImageSlices[slicenr]).Copy2tissue(m_ActiveTissuelayer, bits);
}

void SlicesHandler::Copyfrombmp(unsigned short slicenr, float* bits)
{
	(m_ImageSlices[slicenr]).Copyfrombmp(bits);
}

void SlicesHandler::Copyfromwork(unsigned short slicenr, float* bits)
{
	(m_ImageSlices[slicenr]).Copyfromwork(bits);
}

void SlicesHandler::Copyfromtissue(unsigned short slicenr, tissues_size_t* bits)
{
	(m_ImageSlices[slicenr]).Copyfromtissue(m_ActiveTissuelayer, bits);
}

#ifdef TISSUES_SIZE_TYPEDEF
void SlicesHandler::Copyfromtissue(unsigned short slicenr, unsigned char* bits)
{
	(m_ImageSlices[slicenr]).Copyfromtissue(m_ActiveTissuelayer, bits);
}
#endif // TISSUES_SIZE_TYPEDEF

void SlicesHandler::Copyfromtissuepadded(unsigned short slicenr, tissues_size_t* bits, unsigned short padding)
{
	(m_ImageSlices[slicenr])
			.Copyfromtissuepadded(m_ActiveTissuelayer, bits, padding);
}

unsigned int SlicesHandler::MakeHistogram(bool includeoutofrange)
{
	// \note unused function
	unsigned int histogram[256];
	unsigned int* dummy;
	unsigned int l = 0;

	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		l += m_ImageSlices[i].MakeHistogram(includeoutofrange);

	dummy = (m_ImageSlices[m_Startslice]).ReturnHistogram();
	for (unsigned short i = 0; i < 255; i++)
		histogram[i] = dummy[i];

	for (unsigned short j = m_Startslice + 1; j < m_Endslice; j++)
	{
		dummy = m_ImageSlices[j].ReturnHistogram();
		for (unsigned short i = 0; i < 255; i++)
			histogram[i] += dummy[i];
	}

	return l;
}

unsigned int SlicesHandler::ReturnArea()
{
	return (m_ImageSlices[0]).ReturnArea();
}

unsigned short SlicesHandler::Width() const { return m_Width; }

unsigned short SlicesHandler::Height() const { return m_Height; }

unsigned short SlicesHandler::NumSlices() const { return m_Nrslices; }

unsigned short SlicesHandler::StartSlice() const { return m_Startslice; }

unsigned short SlicesHandler::EndSlice() const { return m_Endslice; }

void SlicesHandler::SetStartslice(unsigned short startslice1)
{
	m_Startslice = startslice1;
}

void SlicesHandler::SetEndslice(unsigned short endslice1)
{
	m_Endslice = endslice1;
}

bool SlicesHandler::Isloaded() const { return m_Loaded; }

void SlicesHandler::Gaussian(float sigma)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].Gaussian(sigma);
}

void SlicesHandler::FillHoles(float f, int minsize)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].FillHoles(f, minsize);
	}
}

void SlicesHandler::FillHolestissue(tissues_size_t f, int minsize)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].FillHolestissue(m_ActiveTissuelayer, f, minsize);
	}
}

void SlicesHandler::RemoveIslands(float f, int minsize)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].RemoveIslands(f, minsize);
	}
}

void SlicesHandler::RemoveIslandstissue(tissues_size_t f, int minsize)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].RemoveIslandstissue(m_ActiveTissuelayer, f, minsize);
	}
}

void SlicesHandler::FillGaps(int minsize, bool connectivity)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].FillGaps(minsize, connectivity);
	}
}

void SlicesHandler::FillGapstissue(int minsize, bool connectivity)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].FillGapstissue(m_ActiveTissuelayer, minsize, connectivity);
	}
}

bool SlicesHandler::ValueAtBoundary3D(float value)
{
	// Top
	float* tmp = &(m_ImageSlices[m_Startslice].ReturnWork()[0]);
	for (unsigned pos = 0; pos < m_Area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	// Sides
	for (unsigned short i = m_Startslice + 1; i < m_Endslice - 1; i++)
	{
		if (m_ImageSlices[i].ValueAtBoundary(value))
		{
			return true;
		}
	}

	// Bottom
	tmp = &(m_ImageSlices[m_Endslice - 1].ReturnWork()[0]);
	for (unsigned pos = 0; pos < m_Area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	return false;
}

bool SlicesHandler::TissuevalueAtBoundary3D(tissues_size_t value)
{
	// Top
	tissues_size_t* tmp = &(m_ImageSlices[m_Startslice].ReturnTissues(m_ActiveTissuelayer)[0]);
	for (unsigned pos = 0; pos < m_Area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	// Sides
	for (unsigned short i = m_Startslice + 1; i < m_Endslice - 1; i++)
	{
		if (m_ImageSlices[i]
						.TissuevalueAtBoundary(m_ActiveTissuelayer, value))
		{
			return true;
		}
	}

	// Bottom
	tmp = &(m_ImageSlices[m_Endslice - 1].ReturnTissues(m_ActiveTissuelayer)[0]);
	for (unsigned pos = 0; pos < m_Area; pos++, tmp++)
	{
		if (*tmp == value)
		{
			return true;
		}
	}

	return false;
}

float SlicesHandler::AddSkin(int i1)
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

		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			float* bits = m_ImageSlices[i].ReturnWork();
			for (unsigned pos = 0; pos < m_Area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const i_n = m_Endslice;
#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].AddSkin(i1, setto);
	}

	return setto;
}

float SlicesHandler::AddSkinOutside(int i1)
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

		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			float* bits = m_ImageSlices[i].ReturnWork();
			for (unsigned pos = 0; pos < m_Area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const i_n = m_Endslice;
#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].AddSkinOutside(i1, setto);
	}

	return setto;
}

void SlicesHandler::AddSkintissue(int i1, tissues_size_t f)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].AddSkintissue(m_ActiveTissuelayer, i1, f);
	}
}

void SlicesHandler::AddSkintissueOutside(int i1, tissues_size_t f)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].AddSkintissueOutside(m_ActiveTissuelayer, i1, f);
	}
}

void SlicesHandler::FillUnassigned()
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

		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			float* bits = m_ImageSlices[i].ReturnWork();
			for (unsigned pos = 0; pos < m_Area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	int const i_n = m_Endslice;
#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].FillUnassigned(setto);
	}
}

void SlicesHandler::FillUnassignedtissue(tissues_size_t f)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].FillUnassignedtissue(m_ActiveTissuelayer, f);
	}
}

void SlicesHandler::Kmeans(unsigned short slicenr, short nrtissues, unsigned int iternr, unsigned int converge)
{
	if (slicenr >= m_Startslice && slicenr < m_Endslice)
	{
		KMeans kmeans;
		float* bits[1];
		bits[0] = m_ImageSlices[slicenr].ReturnBmp();
		float weights[1];
		weights[0] = 1;
		kmeans.Init(m_Width, m_Height, nrtissues, 1, bits, weights);
		kmeans.MakeIter(iternr, converge);
		kmeans.ReturnM(m_ImageSlices[slicenr].ReturnWork());
		m_ImageSlices[slicenr].SetMode(2, false);

		for (unsigned short i = m_Startslice; i < slicenr; i++)
		{
			bits[0] = m_ImageSlices[i].ReturnBmp();
			kmeans.ApplyTo(bits, m_ImageSlices[i].ReturnWork());
			m_ImageSlices[i].SetMode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < m_Endslice; i++)
		{
			bits[0] = m_ImageSlices[i].ReturnBmp();
			kmeans.ApplyTo(bits, m_ImageSlices[i].ReturnWork());
			m_ImageSlices[i].SetMode(2, false);
		}
	}
}

void SlicesHandler::KmeansMhd(unsigned short slicenr, short nrtissues, short dim, std::vector<std::string> mhdfiles, float* weights, unsigned int iternr, unsigned int converge)
{
	if (mhdfiles.size() + 1 < dim)
		return;
	if (slicenr >= m_Startslice && slicenr < m_Endslice)
	{
		KMeans kmeans;
		float** bits = new float*[dim];
		for (unsigned short j = 0; j + 1 < dim; j++)
		{
			bits[j + 1] = new float[m_Area];
			if (bits[j + 1] == nullptr)
			{
				for (unsigned short i = 1; i < j; i++)
					delete[] bits[i];
				delete[] bits;
				return;
			}
		}

		bits[0] = m_ImageSlices[slicenr].ReturnBmp();
		for (unsigned short i = 0; i + 1 < dim; i++)
		{
			if (!ImageReader::GetSlice(mhdfiles[i].c_str(), bits[i + 1], slicenr, m_Width, m_Height))
			{
				for (unsigned short j = 1; j < dim; j++)
					delete[] bits[j];
				delete[] bits;
				return;
			}
		}
		kmeans.Init(m_Width, m_Height, nrtissues, dim, bits, weights);
		kmeans.MakeIter(iternr, converge);
		kmeans.ReturnM(m_ImageSlices[slicenr].ReturnWork());
		m_ImageSlices[slicenr].SetMode(2, false);

		for (unsigned short i = m_Startslice; i < slicenr; i++)
		{
			bits[0] = m_ImageSlices[i].ReturnBmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ImageReader::GetSlice(mhdfiles[k].c_str(), bits[k + 1], i, m_Width, m_Height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.ApplyTo(bits, m_ImageSlices[i].ReturnWork());
			m_ImageSlices[i].SetMode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < m_Endslice; i++)
		{
			bits[0] = m_ImageSlices[i].ReturnBmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ImageReader::GetSlice(mhdfiles[k].c_str(), bits[k + 1], i, m_Width, m_Height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.ApplyTo(bits, m_ImageSlices[i].ReturnWork());
			m_ImageSlices[i].SetMode(2, false);
		}

		for (unsigned short j = 1; j < dim; j++)
			delete[] bits[j];
		delete[] bits;
	}
}

void SlicesHandler::KmeansPng(unsigned short slicenr, short nrtissues, short dim, std::vector<std::string> pngfiles, std::vector<int> exctractChannel, float* weights, unsigned int iternr, unsigned int converge, const std::string initCentersFile)
{
	if (pngfiles.size() + 1 < dim)
		return;
	if (slicenr >= m_Startslice && slicenr < m_Endslice)
	{
		float** bits = new float*[dim];
		for (unsigned short j = 0; j + 1 < dim; j++)
		{
			bits[j + 1] = new float[m_Area];
			if (bits[j + 1] == nullptr)
			{
				for (unsigned short i = 1; i < j; i++)
					delete[] bits[i];
				delete[] bits;
				return;
			}
		}

		bits[0] = m_ImageSlices[slicenr].ReturnBmp();
		for (unsigned short i = 0; i + 1 < dim; i++)
		{
			if (!ChannelExtractor::getSlice(pngfiles[0].c_str(), bits[i + 1], exctractChannel[i], slicenr, m_Width, m_Height))
			{
				for (unsigned short j = 1; j < dim; j++)
					delete[] bits[j];
				delete[] bits;
				return;
			}
		}

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
				kmeans.Init(m_Width, m_Height, nrtissues, dim, bits, weights, centers);
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
			kmeans.Init(m_Width, m_Height, nrtissues, dim, bits, weights);
		}
		kmeans.MakeIter(iternr, converge);
		kmeans.ReturnM(m_ImageSlices[slicenr].ReturnWork());
		m_ImageSlices[slicenr].SetMode(2, false);

		for (unsigned short i = m_Startslice; i < slicenr; i++)
		{
			bits[0] = m_ImageSlices[i].ReturnBmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ChannelExtractor::getSlice(pngfiles[0].c_str(), bits[i + 1], exctractChannel[i], i, m_Width, m_Height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.ApplyTo(bits, m_ImageSlices[i].ReturnWork());
			m_ImageSlices[i].SetMode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < m_Endslice; i++)
		{
			bits[0] = m_ImageSlices[i].ReturnBmp();
			for (unsigned short k = 0; k + 1 < dim; k++)
			{
				if (!ChannelExtractor::getSlice(pngfiles[0].c_str(), bits[i + 1], exctractChannel[i], i, m_Width, m_Height))
				{
					for (unsigned short j = 1; j < dim; j++)
						delete[] bits[j];
					delete[] bits;
					return;
				}
			}
			kmeans.ApplyTo(bits, m_ImageSlices[i].ReturnWork());
			m_ImageSlices[i].SetMode(2, false);
		}

		for (unsigned short j = 1; j < dim; j++)
			delete[] bits[j];
		delete[] bits;
	}
}

void SlicesHandler::Em(unsigned short slicenr, short nrtissues, unsigned int iternr, unsigned int converge)
{
	if (slicenr >= m_Startslice && slicenr < m_Endslice)
	{
		ExpectationMaximization em;
		float* bits[1];
		bits[0] = m_ImageSlices[slicenr].ReturnBmp();
		float weights[1];
		weights[0] = 1;
		em.Init(m_Width, m_Height, nrtissues, 1, bits, weights);
		em.MakeIter(iternr, converge);
		em.Classify(m_ImageSlices[slicenr].ReturnWork());
		m_ImageSlices[slicenr].SetMode(2, false);

		for (unsigned short i = m_Startslice; i < slicenr; i++)
		{
			bits[0] = m_ImageSlices[i].ReturnBmp();
			em.ApplyTo(bits, m_ImageSlices[i].ReturnWork());
			m_ImageSlices[i].SetMode(2, false);
		}
		for (unsigned short i = slicenr + 1; i < m_Endslice; i++)
		{
			bits[0] = m_ImageSlices[i].ReturnBmp();
			em.ApplyTo(bits, m_ImageSlices[i].ReturnWork());
			m_ImageSlices[i].SetMode(2, false);
		}
	}
}

void SlicesHandler::AnisoDiff(float dt, int n, float (*f)(float, float), float k, float restraint)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].AnisoDiff(dt, n, f, k, restraint);
}

void SlicesHandler::ContAnisodiff(float dt, int n, float (*f)(float, float), float k, float restraint)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].ContAnisodiff(dt, n, f, k, restraint);
}

void SlicesHandler::MedianInterquartile(bool median)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].MedianInterquartile(median);
}

void SlicesHandler::Average(unsigned short n)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].Average(n);
}

void SlicesHandler::Sigmafilter(float sigma, unsigned short nx, unsigned short ny)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].Sigmafilter(sigma, nx, ny);
}

void SlicesHandler::Threshold(float* thresholds)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].Threshold(thresholds);
	}
}

void SlicesHandler::ExtractinterpolatesaveContours(int minsize, std::vector<tissues_size_t>& tissuevec, unsigned short between, bool dp, float epsilon, const char* filename)
{
	/*	os.clear();
	os.set_sizenr((between+1)*nrslices-between);
	os.set_thickness(thickness/(between+1));*/

	std::vector<std::vector<Point>> v1, v2;
	std::vector<point_type> v_p;

	SlicesHandler dummy3_d;
	dummy3_d.Newbmp((unsigned short)(m_ImageSlices[0].ReturnWidth()), (unsigned short)(m_ImageSlices[0].ReturnHeight()), between + 2);
	dummy3_d.SetSlicethickness(m_Thickness / (between + 1));
	Pair pair1 = GetPixelsize();
	dummy3_d.SetPixelsize(pair1.high, pair1.low);

	FILE* fp = dummy3_d.SaveContourprologue(filename, (between + 1) * m_Nrslices - between);

	for (unsigned short j = 0; j + 1 < m_Nrslices; j++)
	{
		dummy3_d.Copy2tissue(0, m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer));
		dummy3_d.Copy2tissue(between + 1, m_ImageSlices[j + 1].ReturnTissues(m_ActiveTissuelayer));
		dummy3_d.Interpolatetissuegrey(0, between + 1); // TODO: Use interpolatetissuegrey_medianset?

		dummy3_d.ExtractContours(minsize, tissuevec);
		if (dp)
			dummy3_d.DougpeuckLine(epsilon);
		fp = dummy3_d.SaveContoursection(fp, 0, between, (between + 1) * j);
	}

	fp = dummy3_d.SaveContoursection(fp, 0, 0, (between + 1) * (m_Nrslices - 1));

	fp = dummy3_d.SaveTissuenamescolors(fp);

	fclose(fp);
}

void SlicesHandler::ExtractinterpolatesaveContours2Xmirrored(int minsize, std::vector<tissues_size_t>& tissuevec, unsigned short between, bool dp, float epsilon, const char* filename)
{
	std::vector<std::vector<Point>> v1, v2;
	std::vector<point_type> v_p;

	SlicesHandler dummy3_d;
	dummy3_d.Newbmp((unsigned short)(m_ImageSlices[0].ReturnWidth()), (unsigned short)(m_ImageSlices[0].ReturnHeight()), between + 2);
	dummy3_d.SetSlicethickness(m_Thickness / (between + 1));
	Pair pair1 = GetPixelsize();
	dummy3_d.SetPixelsize(pair1.high / 2, pair1.low / 2);

	FILE* fp = dummy3_d.SaveContourprologue(filename, (between + 1) * m_Nrslices - between);

	for (unsigned short j = 0; j + 1 < m_Nrslices; j++)
	{
		dummy3_d.Copy2tissue(0, m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer));
		dummy3_d.Copy2tissue(between + 1, m_ImageSlices[j + 1].ReturnTissues(m_ActiveTissuelayer));
		dummy3_d.Interpolatetissuegrey(0, between + 1); // TODO: Use interpolatetissuegrey_medianset?

		//		dummy3D.extract_contours2(minsize, tissuevec);
		if (dp)
		{
			//			dummy3D.dougpeuck_line(epsilon*2);
			dummy3_d.ExtractContours2Xmirrored(minsize, tissuevec, epsilon);
		}
		else
		{
			dummy3_d.ExtractContours2Xmirrored(minsize, tissuevec);
		}
		dummy3_d.ShiftContours(-(int)dummy3_d.Width(), -(int)dummy3_d.Height());
		fp = dummy3_d.SaveContoursection(fp, 0, between, (between + 1) * j);
	}

	fp = dummy3_d.SaveContoursection(fp, 0, 0, (between + 1) * (m_Nrslices - 1));

	fp = dummy3_d.SaveTissuenamescolors(fp);

	fclose(fp);
}

void SlicesHandler::ExtractContours(int minsize, std::vector<tissues_size_t>& tissuevec)
{
	m_Os.Clear();
	std::vector<std::vector<Point>> v1, v2;
	std::vector<point_type> v_p;

	for (std::vector<tissues_size_t>::iterator it1 = tissuevec.begin();
			 it1 != tissuevec.end(); it1++)
	{
		for (unsigned short i = 0; i < m_Nrslices; i++)
		{
			v1.clear();
			v2.clear();
			m_ImageSlices[i]
					.GetTissuecontours(m_ActiveTissuelayer, *it1, &v1, &v2, minsize);
			for (std::vector<std::vector<Point>>::iterator it = v1.begin();
					 it != v1.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,true);
				m_Os.AddLine(i, *it1, &(*it), true);
			}
			for (std::vector<std::vector<Point>>::iterator it = v2.begin();
					 it != v2.end(); it++)
			{
				//			Pointconvert(*it,vP);
				//			os.add_line(i,tissuetype,&vP,false);
				m_Os.AddLine(i, *it1, &(*it), false);
			}
		}
	}
}

void SlicesHandler::ExtractContours2Xmirrored(int minsize, std::vector<tissues_size_t>& tissuevec)
{
	m_Os.Clear();
	std::vector<std::vector<Point>> v1, v2;

	for (auto tissue_label : tissuevec)
	{
		for (unsigned short i = 0; i < m_Nrslices; i++)
		{
			v1.clear();
			v2.clear();
			m_ImageSlices[i].GetTissuecontours2Xmirrored(m_ActiveTissuelayer, tissue_label, &v1, &v2, minsize);
			for (auto& line : v1)
			{
				m_Os.AddLine(i, tissue_label, &line, true);
			}
			for (auto& line : v2)
			{
				m_Os.AddLine(i, tissue_label, &line, false);
			}
		}
	}
}

void SlicesHandler::ExtractContours2Xmirrored(int minsize, std::vector<tissues_size_t>& tissuevec, float epsilon)
{
	m_Os.Clear();
	std::vector<std::vector<Point>> v1, v2;

	for (auto tissue_label : tissuevec)
	{
		for (unsigned short i = 0; i < m_Nrslices; i++)
		{
			v1.clear();
			v2.clear();
			m_ImageSlices[i].GetTissuecontours2Xmirrored(m_ActiveTissuelayer, tissue_label, &v1, &v2, minsize, epsilon);
			for (auto& line : v1)
			{
				m_Os.AddLine(i, tissue_label, &line, true);
			}
			for (auto& line : v2)
			{
				m_Os.AddLine(i, tissue_label, &line, false);
			}
		}
	}
}

void SlicesHandler::BmpSum()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].BmpSum();
	}
}

void SlicesHandler::BmpAdd(float f)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].BmpAdd(f);
	}
}

void SlicesHandler::BmpDiff()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].BmpDiff();
	}
}

void SlicesHandler::BmpMult()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].BmpMult();
	}
}

void SlicesHandler::BmpMult(float f)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].BmpMult(f);
	}
}

void SlicesHandler::BmpOverlay(float alpha)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].BmpOverlay(alpha);
	}
}

void SlicesHandler::BmpAbs()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].BmpAbs();
	}
}

void SlicesHandler::BmpNeg()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].BmpNeg();
	}
}

void SlicesHandler::ScaleColors(Pair p)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].ScaleColors(p);
	}
}

void SlicesHandler::CropColors()
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].CropColors();
	}
}

void SlicesHandler::GetRange(Pair* pp)
{
	Pair p;
	m_ImageSlices[m_Startslice].GetRange(pp);

	for (unsigned short i = m_Startslice + 1; i < m_Endslice; i++)
	{
		m_ImageSlices[i].GetRange(&p);
		if ((*pp).high < p.high)
			(*pp).high = p.high;
		if ((*pp).low > p.low)
			(*pp).low = p.low;
	}
}

void SlicesHandler::ComputeRangeMode1(Pair* pp)
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

		const int i_n = m_Nrslices;
#pragma omp for
		for (int i = 0; i < i_n; i++)
		{
			if (m_ImageSlices[i].ReturnMode(false) == 1)
			{
				m_ImageSlices[i].GetRange(&p);
				m_SliceRanges[i] = p;
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

void SlicesHandler::ComputeRangeMode1(unsigned short updateSlicenr, Pair* pp)
{
	// Update range for single mode 1 slice
	if (m_ImageSlices[updateSlicenr].ReturnMode(false) == 1)
	{
		m_ImageSlices[updateSlicenr].GetRange(&m_SliceRanges[updateSlicenr]);
	}

	// Compute total range
	pp->high = 0.0f;
	pp->low = FLT_MAX;
	for (unsigned short i = 0; i < m_Nrslices; ++i)
	{
		if (m_ImageSlices[i].ReturnMode(false) != 1)
			continue;
		Pair p = m_SliceRanges[i];
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

void SlicesHandler::GetBmprange(Pair* pp)
{
	Pair p;
	m_ImageSlices[m_Startslice].GetBmprange(pp);

	for (unsigned short i = m_Startslice + 1; i < m_Endslice; i++)
	{
		m_ImageSlices[i].GetBmprange(&p);
		if ((*pp).high < p.high)
			(*pp).high = p.high;
		if ((*pp).low > p.low)
			(*pp).low = p.low;
	}
}

void SlicesHandler::ComputeBmprangeMode1(Pair* pp)
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

		const int i_n = m_Nrslices;
#pragma omp for
		for (int i = 0; i < i_n; ++i)
		{
			if (m_ImageSlices[i].ReturnMode(true) == 1)
			{
				m_ImageSlices[i].GetBmprange(&p);
				m_SliceBmpranges[i] = p;

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

void SlicesHandler::ComputeBmprangeMode1(unsigned short updateSlicenr, Pair* pp)
{
	// Update range for single mode 1 slice
	if (m_ImageSlices[updateSlicenr].ReturnMode(true) == 1)
	{
		m_ImageSlices[updateSlicenr].GetBmprange(&m_SliceBmpranges[updateSlicenr]);
	}

	// Compute total range
	pp->high = 0.0f;
	pp->low = FLT_MAX;
	for (unsigned short i = 0; i < m_Nrslices; ++i)
	{
		if (m_ImageSlices[i].ReturnMode(true) != 1)
			continue;
		Pair p = m_SliceBmpranges[i];
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

void SlicesHandler::GetRangetissue(tissues_size_t* pp)
{
	tissues_size_t p;
	m_ImageSlices[m_Startslice].GetRangetissue(m_ActiveTissuelayer, pp);

	for (unsigned short i = m_Startslice + 1; i < m_Endslice; i++)
	{
		m_ImageSlices[i].GetRangetissue(m_ActiveTissuelayer, &p);
		if ((*pp) < p)
			(*pp) = p;
	}
}

void SlicesHandler::ZeroCrossings(bool connectivity)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].ZeroCrossings(connectivity);
	}
}

void SlicesHandler::SaveContours(const char* filename)
{
	m_Os.Print(filename, TissueInfos::GetTissueCount());
	FILE* fp = fopen(filename, "a");
	fp = SaveTissuenamescolors(fp);
	fclose(fp);
}

void SlicesHandler::ShiftContours(int dx, int dy)
{
	m_Os.ShiftContours(dx, dy);
}

void SlicesHandler::SetextrusionContours(int top, int bottom)
{
	m_Os.SetThickness(bottom * m_Thickness, 0);
	if (m_Nrslices > 1)
	{
		m_Os.SetThickness(top * m_Thickness, m_Nrslices - 1);
	}
}

void SlicesHandler::ResetextrusionContours()
{
	m_Os.SetThickness(m_Thickness, 0);
	if (m_Nrslices > 1)
	{
		m_Os.SetThickness(m_Thickness, m_Nrslices - 1);
	}
}

FILE* SlicesHandler::SaveContourprologue(const char* filename, unsigned nr_slices)
{
	return m_Os.Printprologue(filename, nr_slices, TissueInfos::GetTissueCount());
}

FILE* SlicesHandler::SaveContoursection(FILE* fp, unsigned startslice1, unsigned endslice1, unsigned offset)
{
	m_Os.Printsection(fp, startslice1, endslice1, offset, TissueInfos::GetTissueCount());
	return fp;
}

FILE* SlicesHandler::SaveTissuenamescolors(FILE* fp)
{
	tissues_size_t tissue_count = TissueInfos::GetTissueCount();
	TissueInfo* tissue_info;

	fprintf(fp, "NT%u\n", tissue_count);

	if (tissue_count > 255)
	{ // Only print tissue indices which contain outlines

		// Collect used tissue indices in ascending order
		std::set<tissues_size_t> tissue_indices;
		m_Os.InsertTissueIndices(tissue_indices);
		std::set<tissues_size_t>::iterator idx_it;
		for (idx_it = tissue_indices.begin(); idx_it != tissue_indices.end();
				 ++idx_it)
		{
			tissue_info = TissueInfos::GetTissueInfo(*idx_it);
			fprintf(fp, "T%i %f %f %f %s\n", (int)*idx_it, tissue_info->m_Color[0], tissue_info->m_Color[1], tissue_info->m_Color[2], tissue_info->m_Name.c_str());
		}
	}
	else
	{
		// Print infos of all tissues
		for (unsigned i = 1; i <= tissue_count; i++)
		{
			tissue_info = TissueInfos::GetTissueInfo(i);
			fprintf(fp, "T%i %f %f %f %s\n", (int)i, tissue_info->m_Color[0], tissue_info->m_Color[1], tissue_info->m_Color[2], tissue_info->m_Name.c_str());
		}
	}

	return fp;
}

void SlicesHandler::DougpeuckLine(float epsilon)
{
	m_Os.DougPeuck(epsilon, true);
}

void SlicesHandler::Hysteretic(float thresh_low, float thresh_high, bool connectivity, unsigned short nrpasses)
{
	float setvalue = 255;
	unsigned short slicenr = m_Startslice;

	ClearWork();

	m_ImageSlices[slicenr].Hysteretic(thresh_low, thresh_high, connectivity, setvalue);
	for (unsigned short i = 0; i < nrpasses; i++)
	{
		while (++slicenr < m_Endslice)
		{
			m_ImageSlices[slicenr].Hysteretic(thresh_low, thresh_high, connectivity, m_ImageSlices[slicenr - 1].ReturnWork(), setvalue - 1, setvalue);
		}
		setvalue++;
		slicenr--;
		while (slicenr-- > m_Startslice)
		{
			m_ImageSlices[slicenr].Hysteretic(thresh_low, thresh_high, connectivity, m_ImageSlices[slicenr + 1].ReturnWork(), setvalue - 1, setvalue);
		}
		setvalue++;
		slicenr = m_Startslice;
	}

	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 255 - f_tol;
	SwapBmpworkall();
	Threshold(thresh);
}

void SlicesHandler::ThresholdedGrowing(short unsigned slicenr, Point p, float threshfactor_low, float threshfactor_high, bool connectivity, unsigned short nrpasses)
{
	if (slicenr < m_Endslice && slicenr >= m_Startslice)
	{
		float setvalue = 255;
		Pair tp;

		ClearWork();

		m_ImageSlices[slicenr].ThresholdedGrowing(p, threshfactor_low, threshfactor_high, connectivity, setvalue, &tp);

		for (unsigned short i = 0; i < nrpasses; i++)
		{
			while (++slicenr < m_Endslice)
			{
				m_ImageSlices[slicenr].ThresholdedGrowing(tp.low, tp.high, connectivity, m_ImageSlices[slicenr - 1].ReturnWork(), setvalue - 1, setvalue);
			}
			setvalue++;
			slicenr--;
			while (slicenr-- > m_Startslice)
			{
				m_ImageSlices[slicenr].ThresholdedGrowing(tp.low, tp.high, connectivity, m_ImageSlices[slicenr + 1].ReturnWork(), setvalue - 1, setvalue);
			}
			setvalue++;
			slicenr = m_Startslice;
		}

		float thresh[2];
		thresh[0] = 1;
		thresh[1] = 255 - f_tol;
		SwapBmpworkall();
		Threshold(thresh);
	}
}

void SlicesHandler::ThresholdedGrowing(short unsigned slicenr, Point p, float thresh_low, float thresh_high, float set_to) //bool connectivity,float set_to)
{
	if (slicenr >= m_Startslice && slicenr < m_Endslice)
	{
		unsigned position = p.px + p.py * (unsigned)m_Width;
		std::vector<Posit> s;
		Posit p1;

		for (unsigned short z = m_Startslice; z < m_Endslice; z++)
		{
			float* work = m_ImageSlices[z].ReturnWork();
			float* bmp = m_ImageSlices[z].ReturnBmp();
			int i = 0;
			for (unsigned short j = 0; j < m_Height; j++)
			{
				for (unsigned short k = 0; k < m_Width; k++)
				{
					if (bmp[i] > thresh_high || bmp[i] < thresh_low)
						work[i] = 0;
					else
						work[i] = -1;
					i++;
				}
			}
		}

		p1.m_Pxy = position;
		p1.m_Pz = slicenr;

		s.push_back(p1);
		float* work = m_ImageSlices[slicenr].ReturnWork();
		work[position] = set_to;

		//	hysteretic_growth(results,&s,width+2,height+2,connectivity,set_to);
		Posit i, j;

		while (!s.empty())
		{
			i = s.back();
			s.pop_back();

			work = m_ImageSlices[i.m_Pz].ReturnWork();
			if (i.m_Pxy % m_Width != 0 && work[i.m_Pxy - 1] == -1)
			{
				work[i.m_Pxy - 1] = set_to;
				j.m_Pxy = i.m_Pxy - 1;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if ((i.m_Pxy + 1) % m_Width != 0 && work[i.m_Pxy + 1] == -1)
			{
				work[i.m_Pxy + 1] = set_to;
				j.m_Pxy = i.m_Pxy + 1;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if (i.m_Pxy >= m_Width && work[i.m_Pxy - m_Width] == -1)
			{
				work[i.m_Pxy - m_Width] = set_to;
				j.m_Pxy = i.m_Pxy - m_Width;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if (i.m_Pxy < m_Area - m_Width && work[i.m_Pxy + m_Width] == -1)
			{
				work[i.m_Pxy + m_Width] = set_to;
				j.m_Pxy = i.m_Pxy + m_Width;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if (i.m_Pz > m_Startslice)
			{
				work = m_ImageSlices[i.m_Pz - 1].ReturnWork();
				if (work[i.m_Pxy] == -1)
				{
					work[i.m_Pxy] = set_to;
					j.m_Pxy = i.m_Pxy;
					j.m_Pz = i.m_Pz - 1;
					s.push_back(j);
				}
				//			if(connectivity){
				//			}
			}
			if (i.m_Pz + 1 < m_Endslice)
			{
				work = m_ImageSlices[i.m_Pz + 1].ReturnWork();
				if (work[i.m_Pxy] == -1)
				{
					work[i.m_Pxy] = set_to;
					j.m_Pxy = i.m_Pxy;
					j.m_Pz = i.m_Pz + 1;
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

		for (unsigned short z = m_Startslice; z < m_Endslice; z++)
		{
			work = m_ImageSlices[z].ReturnWork();
			for (unsigned i1 = 0; i1 < m_Area; i1++)
				if (work[i1] == -1)
					work[i1] = 0;
			m_ImageSlices[z].SetMode(2, false);
		}
	}
}

void SlicesHandler::Add2tissueallConnected(tissues_size_t tissuetype, Point p, bool override)
{
	if (m_Activeslice >= m_Startslice && m_Activeslice < m_Endslice)
	{
		float set_to = (float)123E10;
		unsigned position = p.px + p.py * (unsigned)m_Width;
		std::vector<Posit> s;
		Posit p1;

		p1.m_Pxy = position;
		p1.m_Pz = m_Activeslice;

		s.push_back(p1);
		float* work = m_ImageSlices[m_Activeslice].ReturnWork();
		float f = work[position];
		tissues_size_t* tissue =
				m_ImageSlices[m_Activeslice].ReturnTissues(m_ActiveTissuelayer);
		bool tissue_locked = TissueInfos::GetTissueLocked(tissue[position]);
		if (tissue[position] == 0 || (override && tissue_locked == false))
			tissue[position] = tissuetype;
		if (tissue[position] == 0 || (override && tissue_locked == false))
			work[position] = set_to;

		Posit i, j;

		while (!s.empty())
		{
			i = s.back();
			s.pop_back();

			work = m_ImageSlices[i.m_Pz].ReturnWork();
			tissue = m_ImageSlices[i.m_Pz].ReturnTissues(m_ActiveTissuelayer);
			//if(i.pxy%width!=0&&work[i.pxy-1]==f&&(override||tissue[i.pxy-1]==0)) {
			if (i.m_Pxy % m_Width != 0 && work[i.m_Pxy - 1] == f &&
					(tissue[i.m_Pxy - 1] == 0 ||
							(override &&
									TissueInfos::GetTissueLocked(tissue[i.m_Pxy - 1]) == false)))
			{
				work[i.m_Pxy - 1] = set_to;
				tissue[i.m_Pxy - 1] = tissuetype;
				j.m_Pxy = i.m_Pxy - 1;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			//if((i.pxy+1)%width!=0&&work[i.pxy+1]==f&&(override||tissue[i.pxy+1]==0)) {
			if ((i.m_Pxy + 1) % m_Width != 0 && work[i.m_Pxy + 1] == f &&
					(tissue[i.m_Pxy + 1] == 0 ||
							(override &&
									TissueInfos::GetTissueLocked(tissue[i.m_Pxy + 1]) == false)))
			{
				work[i.m_Pxy + 1] = set_to;
				tissue[i.m_Pxy + 1] = tissuetype;
				j.m_Pxy = i.m_Pxy + 1;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			//if(i.pxy>=width&&work[i.pxy-width]==f&&(override||tissue[i.pxy-width]==0)) {
			if (i.m_Pxy >= m_Width && work[i.m_Pxy - m_Width] == f &&
					(tissue[i.m_Pxy - m_Width] == 0 ||
							(override && TissueInfos::GetTissueLocked(tissue[i.m_Pxy - m_Width]) == false)))
			{
				work[i.m_Pxy - m_Width] = set_to;
				tissue[i.m_Pxy - m_Width] = tissuetype;
				j.m_Pxy = i.m_Pxy - m_Width;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			//if(i.pxy<=area-width&&work[i.pxy+width]==f&&(override||tissue[i.pxy+width]==0)) {
			if (i.m_Pxy < m_Area - m_Width && work[i.m_Pxy + m_Width] == f &&
					(tissue[i.m_Pxy + m_Width] == 0 ||
							(override && TissueInfos::GetTissueLocked(tissue[i.m_Pxy + m_Width]) == false)))
			{
				work[i.m_Pxy + m_Width] = set_to;
				tissue[i.m_Pxy + m_Width] = tissuetype;
				j.m_Pxy = i.m_Pxy + m_Width;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if (i.m_Pz > m_Startslice)
			{
				work = m_ImageSlices[i.m_Pz - 1].ReturnWork();
				tissue =
						m_ImageSlices[i.m_Pz - 1].ReturnTissues(m_ActiveTissuelayer);
				//if(work[i.pxy]==f&&(override||tissue[i.pxy]==0)) {
				if (work[i.m_Pxy] == f &&
						(tissue[i.m_Pxy] == 0 ||
								(override &&
										TissueInfos::GetTissueLocked(tissue[i.m_Pxy]) == false)))
				{
					work[i.m_Pxy] = set_to;
					tissue[i.m_Pxy] = tissuetype;
					j.m_Pxy = i.m_Pxy;
					j.m_Pz = i.m_Pz - 1;
					s.push_back(j);
				}
			}
			if (i.m_Pz + 1 < m_Endslice)
			{
				work = m_ImageSlices[i.m_Pz + 1].ReturnWork();
				tissue =
						m_ImageSlices[i.m_Pz + 1].ReturnTissues(m_ActiveTissuelayer);
				//if(work[i.pxy]==f&&(override||tissue[i.pxy]==0)) {
				if (work[i.m_Pxy] == f &&
						(tissue[i.m_Pxy] == 0 ||
								(override &&
										TissueInfos::GetTissueLocked(tissue[i.m_Pxy]) == false)))
				{
					work[i.m_Pxy] = set_to;
					tissue[i.m_Pxy] = tissuetype;
					j.m_Pxy = i.m_Pxy;
					j.m_Pz = i.m_Pz + 1;
					s.push_back(j);
				}
			}
		}

		for (unsigned short z = m_Startslice; z < m_Endslice; z++)
		{
			work = m_ImageSlices[z].ReturnWork();
			for (unsigned i1 = 0; i1 < m_Area; i1++)
				if (work[i1] == set_to)
					work[i1] = f;
		}
	}
}

void SlicesHandler::SubtractTissueallConnected(tissues_size_t tissuetype, Point p)
{
	if (m_Activeslice < m_Endslice && m_Activeslice >= m_Startslice)
	{
		float set_to = (float)123E10;
		unsigned position = p.px + p.py * (unsigned)m_Width;
		std::vector<Posit> s;
		Posit p1;

		p1.m_Pxy = position;
		p1.m_Pz = m_Activeslice;

		s.push_back(p1);
		float* work = m_ImageSlices[m_Activeslice].ReturnWork();
		float f = work[position];
		tissues_size_t* tissue =
				m_ImageSlices[m_Activeslice].ReturnTissues(m_ActiveTissuelayer);
		if (tissue[position] == tissuetype)
			tissue[position] = tissuetype;
		if (tissue[position] == tissuetype)
			work[position] = set_to;

		Posit i, j;

		while (!s.empty())
		{
			i = s.back();
			s.pop_back();

			work = m_ImageSlices[i.m_Pz].ReturnWork();
			tissue = m_ImageSlices[i.m_Pz].ReturnTissues(m_ActiveTissuelayer);
			if (i.m_Pxy % m_Width != 0 && work[i.m_Pxy - 1] == f &&
					tissue[i.m_Pxy - 1] == tissuetype)
			{
				work[i.m_Pxy - 1] = set_to;
				tissue[i.m_Pxy - 1] = 0;
				j.m_Pxy = i.m_Pxy - 1;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if ((i.m_Pxy + 1) % m_Width != 0 && work[i.m_Pxy + 1] == f &&
					tissue[i.m_Pxy + 1] == tissuetype)
			{
				work[i.m_Pxy + 1] = set_to;
				tissue[i.m_Pxy + 1] = 0;
				j.m_Pxy = i.m_Pxy + 1;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if (i.m_Pxy >= m_Width && work[i.m_Pxy - m_Width] == f &&
					tissue[i.m_Pxy - m_Width] == tissuetype)
			{
				work[i.m_Pxy - m_Width] = set_to;
				tissue[i.m_Pxy - m_Width] = 0;
				j.m_Pxy = i.m_Pxy - m_Width;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if (i.m_Pxy < m_Area - m_Width && work[i.m_Pxy + m_Width] == f &&
					tissue[i.m_Pxy + m_Width] == tissuetype)
			{
				work[i.m_Pxy + m_Width] = set_to;
				tissue[i.m_Pxy + m_Width] = 0;
				j.m_Pxy = i.m_Pxy + m_Width;
				j.m_Pz = i.m_Pz;
				s.push_back(j);
			}
			if (i.m_Pz > m_Startslice)
			{
				work = m_ImageSlices[i.m_Pz - 1].ReturnWork();
				tissue =
						m_ImageSlices[i.m_Pz - 1].ReturnTissues(m_ActiveTissuelayer);
				if (work[i.m_Pxy] == f && tissue[i.m_Pxy] == tissuetype)
				{
					work[i.m_Pxy] = set_to;
					tissue[i.m_Pxy] = 0;
					j.m_Pxy = i.m_Pxy;
					j.m_Pz = i.m_Pz - 1;
					s.push_back(j);
				}
				//			if(connectivity){
				//			}
			}
			if (i.m_Pz + 1 < m_Endslice)
			{
				work = m_ImageSlices[i.m_Pz + 1].ReturnWork();
				tissue =
						m_ImageSlices[i.m_Pz + 1].ReturnTissues(m_ActiveTissuelayer);
				if (work[i.m_Pxy] == f && tissue[i.m_Pxy] == tissuetype)
				{
					work[i.m_Pxy] = set_to;
					tissue[i.m_Pxy] = 0;
					j.m_Pxy = i.m_Pxy;
					j.m_Pz = i.m_Pz + 1;
					s.push_back(j);
				}
				//			if(connectivity){
				//			}
			}
		}

		for (unsigned short z = m_Startslice; z < m_Endslice; z++)
		{
			work = m_ImageSlices[z].ReturnWork();
			for (unsigned i1 = 0; i1 < m_Area; i1++)
				if (work[i1] == set_to)
					work[i1] = f;
		}
	}
}

void SlicesHandler::DoubleHysteretic(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, unsigned short nrpasses)
{
	float setvalue = 255;
	unsigned short slicenr = m_Startslice;

	ClearWork();

	m_ImageSlices[slicenr].DoubleHysteretic(thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h, connectivity, setvalue);
	//	if(nrslices>1) {
	for (unsigned short i = 0; i < nrpasses; i++)
	{
		while (++slicenr < m_Endslice)
		{
			m_ImageSlices[slicenr].DoubleHysteretic(thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h, connectivity, m_ImageSlices[slicenr - 1].ReturnWork(), setvalue - 1, setvalue);
		}
		setvalue++;
		slicenr--;
		while (slicenr-- > m_Startslice)
		{
			m_ImageSlices[slicenr].DoubleHysteretic(thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h, connectivity, m_ImageSlices[slicenr + 1].ReturnWork(), setvalue - 1, setvalue);
		}
		setvalue++;
		slicenr = 0;
	}
	//	}

	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 255 - f_tol;
	SwapBmpworkall();
	Threshold(thresh);

	//xxxa again. what happens to bmp? and what should we do with mode?
}

void SlicesHandler::DoubleHystereticAllslices(float thresh_low_l, float thresh_low_h, float thresh_high_l, float thresh_high_h, bool connectivity, float set_to)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		m_ImageSlices[i].DoubleHysteretic(thresh_low_l, thresh_low_h, thresh_high_l, thresh_high_h, connectivity, set_to);
	}
}

void SlicesHandler::Interpolateworkgrey(unsigned short slice1, unsigned short slice2, bool connected)
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
		m_ImageSlices[slice1].PushstackBmp();
		m_ImageSlices[slice2].PushstackBmp();

		m_ImageSlices[slice1].SwapBmpwork();
		m_ImageSlices[slice2].SwapBmpwork();

		m_ImageSlices[slice2].DeadReckoning();
		m_ImageSlices[slice1].DeadReckoning();

		float* bmp1 = m_ImageSlices[slice1].ReturnBmp();
		float* bmp2 = m_ImageSlices[slice2].ReturnBmp();
		float* work1 = m_ImageSlices[slice1].ReturnWork();
		float* work2 = m_ImageSlices[slice2].ReturnWork();

		Point p;
		float prop;
		unsigned short n1;
		unsigned i1 = 0;

		for (p.py = 0; p.py < m_Height; p.py++)
		{
			for (p.px = 0; p.px < m_Width; p.px++)
			{
				if (work2[i1] + work1[i1] != 0)
					prop = work1[i1] / (work2[i1] + work1[i1]);
				else
					prop = 0.5f;
				n1 = (unsigned short)n * prop;
				for (unsigned short j = 1; j <= n1 && j < n; j++)
				{
					m_ImageSlices[slice1 + j].SetWorkPt(p, bmp1[i1]);
				}
				for (unsigned short j = n1 + 1; j < n; j++)
				{
					m_ImageSlices[slice1 + j].SetWorkPt(p, bmp2[i1]);
				}
				i1++;
			}
		}

		m_ImageSlices[slice1].SwapBmpwork();
		m_ImageSlices[slice2].SwapBmpwork();

		for (unsigned short j = 1; j < n; j++)
		{
			m_ImageSlices[slice1 + j].SetMode(2, false);
		}

		m_ImageSlices[slice2].PopstackBmp();
		m_ImageSlices[slice1].PopstackBmp();
	}
	else
	{
		SlicesHandlerITKInterface itk_handler(this);
		auto img1 = itk_handler.GetTargetSlice(slice1);
		auto img2 = itk_handler.GetTargetSlice(slice2);

		ConnectedShapeBasedInterpolation interpolator;
		try
		{
			auto interpolated_slices = interpolator.Interpolate(img1, img2, n - 1);

			for (short i = 0; i < n - 1; i++)
			{
				auto slice = interpolated_slices[i];
				const float* source = slice->GetPixelContainer()->GetImportPointer();
				size_t source_len = slice->GetPixelContainer()->Size();
				// copy to target (idx = slice1 + i + 1)
				float* target = m_ImageSlices[slice1 + i + 1].ReturnWork();
				std::copy(source, source + source_len, target);
			}
		}
		catch (std::exception& e)
		{
			ISEG_ERROR("failed to interpolate slices " << e.what());
		}
	}
}

void SlicesHandler::InterpolateworkgreyMedianset(unsigned short slice1, unsigned short slice2, bool connectivity, bool handleVanishingComp)
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
		unsigned int n_cells = 0;
		Pair work_range;
		m_ImageSlices[slice1].GetRange(&work_range);
		n_cells = std::max(n_cells, (unsigned int)(work_range.high + 0.5f));
		m_ImageSlices[slice2].GetRange(&work_range);
		n_cells = std::max(n_cells, (unsigned int)(work_range.high + 0.5f));
		unsigned short max_iterations =
				(unsigned short)std::sqrt((float)(m_Width * m_Width + m_Height * m_Height));

		// Backup images
		m_ImageSlices[slice1].PushstackWork();
		m_ImageSlices[slice2].PushstackWork();
		m_ImageSlices[slice1].PushstackBmp();
		m_ImageSlices[slicehalf].PushstackBmp();
		m_ImageSlices[slice2].PushstackBmp();

		// Interplation input to bmp
		m_ImageSlices[slice1].SwapBmpwork();
		m_ImageSlices[slice2].SwapBmpwork();

		// Input images
		float* f_1 = m_ImageSlices[slice1].ReturnBmp();
		float* f_2 = m_ImageSlices[slice2].ReturnBmp();

		if (handleVanishingComp)
		{
			// Determine forward and backward vanishing components
			std::set<float> vanishing_comp_forward;
			std::set<float> vanishing_comp_backward;
			float* connected_comp_forward =
					(float*)malloc(sizeof(float) * m_Area);
			float* connected_comp_backward =
					(float*)malloc(sizeof(float) * m_Area);

			m_ImageSlices[slice1].ConnectedComponents(false, vanishing_comp_forward); // TODO: connectivity?
			m_ImageSlices[slice1].Copyfromwork(connected_comp_forward);

			m_ImageSlices[slice2].ConnectedComponents(false, vanishing_comp_backward); // TODO: connectivity?
			m_ImageSlices[slice2].Copyfromwork(connected_comp_backward);

			for (unsigned int i = 0; i < m_Area; ++i)
			{
				if (f_1[i] == f_2[i])
				{
					std::set<float>::iterator iter =
							vanishing_comp_forward.find(connected_comp_forward[i]);
					if (iter != vanishing_comp_forward.end())
					{
						vanishing_comp_forward.erase(iter);
					}
					iter = vanishing_comp_backward.find(connected_comp_backward[i]);
					if (iter != vanishing_comp_backward.end())
					{
						vanishing_comp_backward.erase(iter);
					}
				}
			}

			// Remove vanishing components for interpolation
			for (unsigned int i = 0; i < m_Area; ++i)
			{
				auto iter = vanishing_comp_forward.find(connected_comp_forward[i]);
				if (iter != vanishing_comp_forward.end())
				{
					f_2[i] = f_1[i];
				}
				else
				{
					iter = vanishing_comp_backward.find(connected_comp_backward[i]);
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
		float* g_i = m_ImageSlices[slicehalf].ReturnWork();
		float* gp_i = m_ImageSlices[slicehalf].ReturnBmp();

		// Initialize g_0 and gp_0
		for (unsigned int i = 0; i < m_Area; ++i)
		{
			if (f_1[i] == f_2[i])
			{
				g_i[i] = f_1[i];
				gp_i[i] = (float)n_cells - g_i[i];
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
				m_ImageSlices[slice1].Copy2work(g_i, 1);
				m_ImageSlices[slice1].Dilation(1, connectivity);
			}
			else
			{
				m_ImageSlices[slice2].Copy2work(gp_i, 1);
				m_ImageSlices[slice2].Dilation(1, connectivity);
			}
			float* g_i_b = m_ImageSlices[slice1].ReturnWork();
			float* gp_i_b = m_ImageSlices[slice2].ReturnWork();

			// Compute g_i+1 and gp_i+1
			idempotence = true;
			if (iter % 2 == 0)
			{
				for (unsigned int i = 0; i < m_Area; ++i)
				{
					if (g_i_b[i] + gp_i_b[i] <= n_cells)
					{
						float tmp = std::max(g_i_b[i], g_i[i]);
						idempotence &= (g_i[i] == tmp);
						g_i[i] = tmp;
					}
				}
			}
			else
			{
				for (unsigned int i = 0; i < m_Area; ++i)
				{
					if (g_i_b[i] + gp_i_b[i] <= n_cells)
					{
						float tmp = std::max(gp_i_b[i], gp_i[i]);
						idempotence &= (gp_i[i] == tmp);
						gp_i[i] = tmp;
					}
				}
			}
		}

		if (handleVanishingComp)
		{
			m_ImageSlices[slice1].Copy2work(f_1, 1);
			m_ImageSlices[slice2].Copy2work(f_2, 1);

			// Restore images
			m_ImageSlices[slice2].PopstackBmp();
			m_ImageSlices[slicehalf].PopstackBmp();
			m_ImageSlices[slice1].PopstackBmp();

			// Recursion
			InterpolateworkgreyMedianset(slice1, slicehalf, connectivity, false);
			InterpolateworkgreyMedianset(slicehalf, slice2, connectivity, false);

			// Restore images
			m_ImageSlices[slice2].PopstackWork();
			m_ImageSlices[slice1].PopstackWork();
		}
		else
		{
			// Restore images
			m_ImageSlices[slice2].PopstackBmp();
			m_ImageSlices[slicehalf].PopstackBmp();
			m_ImageSlices[slice1].PopstackBmp();
			m_ImageSlices[slice2].PopstackWork();
			m_ImageSlices[slice1].PopstackWork();

			// Recursion
			InterpolateworkgreyMedianset(slice1, slicehalf, connectivity, false);
			InterpolateworkgreyMedianset(slicehalf, slice2, connectivity, false);
		}
	}
}

void SlicesHandler::Interpolatetissuegrey(unsigned short slice1, unsigned short slice2)
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
		m_ImageSlices[slice1].PushstackBmp();
		m_ImageSlices[slice2].PushstackBmp();
		m_ImageSlices[slice1].PushstackWork();
		m_ImageSlices[slice2].PushstackWork();

		m_ImageSlices[slice1].Tissue2work(m_ActiveTissuelayer);
		m_ImageSlices[slice2].Tissue2work(m_ActiveTissuelayer);

		m_ImageSlices[slice1].SwapBmpwork();
		m_ImageSlices[slice2].SwapBmpwork();

		m_ImageSlices[slice2].DeadReckoning();
		m_ImageSlices[slice1].DeadReckoning();

		tissues_size_t* bmp1 =
				m_ImageSlices[slice1].ReturnTissues(m_ActiveTissuelayer);
		tissues_size_t* bmp2 =
				m_ImageSlices[slice2].ReturnTissues(m_ActiveTissuelayer);
		float* work1 = m_ImageSlices[slice1].ReturnWork();
		float* work2 = m_ImageSlices[slice2].ReturnWork();

		Point p;
		float prop;
		unsigned short n1;
		unsigned i1 = 0;

		for (p.py = 0; p.py < m_Height; p.py++)
		{
			for (p.px = 0; p.px < m_Width; p.px++)
			{
				if (work2[i1] + work1[i1] != 0)
					prop = work1[i1] / (work2[i1] + work1[i1]);
				else
					prop = 0.5f;
				n1 = (unsigned short)n * prop;
				for (unsigned short j = 1; j <= n1 && j < n; j++)
				{
					m_ImageSlices[slice1 + j].SetTissuePt(m_ActiveTissuelayer, p, bmp1[i1]);
				}
				for (unsigned short j = n1 + 1; j < n; j++)
				{
					m_ImageSlices[slice1 + j].SetTissuePt(m_ActiveTissuelayer, p, bmp2[i1]);
				}
				i1++;
			}
		}

		for (unsigned short j = 1; j < n; j++)
		{
			m_ImageSlices[slice1 + j].SetMode(2, false);
		}

		m_ImageSlices[slice1].PopstackWork();
		m_ImageSlices[slice2].PopstackWork();
		m_ImageSlices[slice2].PopstackBmp();
		m_ImageSlices[slice1].PopstackBmp();
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

void SlicesHandler::InterpolatetissuegreyMedianset(unsigned short slice1, unsigned short slice2, bool connectivity, bool handleVanishingComp)
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
		unsigned int n_cells = TISSUES_SIZE_MAX;
		unsigned short max_iterations =
				(unsigned short)std::sqrt((float)(m_Width * m_Width + m_Height * m_Height));

		// Backup images
		m_ImageSlices[slice1].PushstackWork();
		m_ImageSlices[slicehalf].PushstackWork();
		m_ImageSlices[slice2].PushstackWork();
		m_ImageSlices[slice1].PushstackBmp();
		m_ImageSlices[slicehalf].PushstackBmp();
		m_ImageSlices[slice2].PushstackBmp();
		tissues_size_t* tissue1_copy = nullptr;
		tissues_size_t* tissue2_copy = nullptr;
		if (handleVanishingComp)
		{
			tissue1_copy =
					(tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area);
			m_ImageSlices[slice1].Copyfromtissue(m_ActiveTissuelayer, tissue1_copy);
			tissue2_copy =
					(tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area);
			m_ImageSlices[slice2].Copyfromtissue(m_ActiveTissuelayer, tissue2_copy);
		}

		// Interplation input to bmp
		m_ImageSlices[slice1].Tissue2work(m_ActiveTissuelayer);
		m_ImageSlices[slice2].Tissue2work(m_ActiveTissuelayer);
		m_ImageSlices[slice1].SwapBmpwork();
		m_ImageSlices[slice2].SwapBmpwork();

		// Input images
		float* f_1 = m_ImageSlices[slice1].ReturnBmp();
		float* f_2 = m_ImageSlices[slice2].ReturnBmp();

		if (handleVanishingComp)
		{
			// Determine forward and backward vanishing components
			std::set<float> vanishing_comp_forward;
			std::set<float> vanishing_comp_backward;
			float* connected_comp_forward =
					(float*)malloc(sizeof(float) * m_Area);
			float* connected_comp_backward =
					(float*)malloc(sizeof(float) * m_Area);

			m_ImageSlices[slice1].ConnectedComponents(false, vanishing_comp_forward); // TODO: connectivity?
			m_ImageSlices[slice1].Copyfromwork(connected_comp_forward);

			m_ImageSlices[slice2].ConnectedComponents(false, vanishing_comp_backward); // TODO: connectivity?
			m_ImageSlices[slice2].Copyfromwork(connected_comp_backward);

			for (unsigned int i = 0; i < m_Area; ++i)
			{
				if (f_1[i] == f_2[i])
				{
					std::set<float>::iterator iter =
							vanishing_comp_forward.find(connected_comp_forward[i]);
					if (iter != vanishing_comp_forward.end())
					{
						vanishing_comp_forward.erase(iter);
					}
					iter = vanishing_comp_backward.find(connected_comp_backward[i]);
					if (iter != vanishing_comp_backward.end())
					{
						vanishing_comp_backward.erase(iter);
					}
				}
			}

			// Remove vanishing components for interpolation
			tissues_size_t* tissue1 =
					m_ImageSlices[slice1].ReturnTissues(m_ActiveTissuelayer);
			tissues_size_t* tissue2 =
					m_ImageSlices[slice2].ReturnTissues(m_ActiveTissuelayer);
			for (unsigned int i = 0; i < m_Area; ++i)
			{
				std::set<float>::iterator iter =
						vanishing_comp_forward.find(connected_comp_forward[i]);
				if (iter != vanishing_comp_forward.end())
				{
					tissue2[i] = tissue1[i];
				}
				else
				{
					iter = vanishing_comp_backward.find(connected_comp_backward[i]);
					if (iter != vanishing_comp_backward.end())
					{
						tissue2[i] = tissue1[i];
					}
				}
			}

			free(connected_comp_forward);
			free(connected_comp_backward);

			// Interplation modified input to bmp
			m_ImageSlices[slice1].Tissue2work(m_ActiveTissuelayer);
			m_ImageSlices[slice2].Tissue2work(m_ActiveTissuelayer);
			m_ImageSlices[slice1].SwapBmpwork();
			m_ImageSlices[slice2].SwapBmpwork();

			// Input images
			f_1 = m_ImageSlices[slice1].ReturnBmp();
			f_2 = m_ImageSlices[slice2].ReturnBmp();
		}

		// Interpolation results
		float* g_i = m_ImageSlices[slicehalf].ReturnWork();
		float* gp_i = m_ImageSlices[slicehalf].ReturnBmp();

		// Initialize g_0 and gp_0
		for (unsigned int i = 0; i < m_Area; ++i)
		{
			if (f_1[i] == f_2[i])
			{
				g_i[i] = f_1[i];
				gp_i[i] = (float)n_cells - g_i[i];
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
				m_ImageSlices[slice1].Copy2work(g_i, 1);
				m_ImageSlices[slice1].Dilation(1, connectivity);
			}
			else
			{
				m_ImageSlices[slice2].Copy2work(gp_i, 1);
				m_ImageSlices[slice2].Dilation(1, connectivity);
			}
			float* g_i_b = m_ImageSlices[slice1].ReturnWork();
			float* gp_i_b = m_ImageSlices[slice2].ReturnWork();

			// Compute g_i+1 and gp_i+1
			idempotence = true;
			if (iter % 2 == 0)
			{
				for (unsigned int i = 0; i < m_Area; ++i)
				{
					if (g_i_b[i] + gp_i_b[i] <= n_cells)
					{
						float tmp = std::max(g_i_b[i], g_i[i]);
						idempotence &= (g_i[i] == tmp);
						g_i[i] = tmp;
					}
				}
			}
			else
			{
				for (unsigned int i = 0; i < m_Area; ++i)
				{
					if (g_i_b[i] + gp_i_b[i] <= n_cells)
					{
						float tmp = std::max(gp_i_b[i], gp_i[i]);
						idempotence &= (gp_i[i] == tmp);
						gp_i[i] = tmp;
					}
				}
			}
		}

		// Assign tissues
		m_ImageSlices[slicehalf].Work2tissue(m_ActiveTissuelayer);

		// Restore images
		m_ImageSlices[slice2].PopstackBmp();
		m_ImageSlices[slicehalf].PopstackBmp();
		m_ImageSlices[slice1].PopstackBmp();
		m_ImageSlices[slice2].PopstackWork();
		m_ImageSlices[slicehalf].PopstackWork();
		m_ImageSlices[slice1].PopstackWork();

		// Recursion
		InterpolatetissuegreyMedianset(slice1, slicehalf, connectivity, false);
		InterpolatetissuegreyMedianset(slicehalf, slice2, connectivity, false);

		// Restore tissues
		if (handleVanishingComp)
		{
			m_ImageSlices[slice1].Copy2tissue(m_ActiveTissuelayer, tissue1_copy);
			m_ImageSlices[slice2].Copy2tissue(m_ActiveTissuelayer, tissue2_copy);
			free(tissue1_copy);
			free(tissue2_copy);
		}
	}
}

void SlicesHandler::Interpolatetissue(unsigned short slice1, unsigned short slice2, tissues_size_t tissuetype, bool connected)
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
		tissues_size_t* tissue1 = m_ImageSlices[slice1].ReturnTissues(m_ActiveTissuelayer);
		tissues_size_t* tissue2 = m_ImageSlices[slice2].ReturnTissues(m_ActiveTissuelayer);
		m_ImageSlices[slice1].PushstackBmp();
		m_ImageSlices[slice2].PushstackBmp();
		float* bmp1 = m_ImageSlices[slice1].ReturnBmp();
		float* bmp2 = m_ImageSlices[slice2].ReturnBmp();
		for (unsigned int i = 0; i < m_Area; i++)
		{
			bmp1[i] = (float)tissue1[i];
			bmp2[i] = (float)tissue2[i];
		}

		m_ImageSlices[slice2].DeadReckoning((float)tissuetype);
		m_ImageSlices[slice1].DeadReckoning((float)tissuetype);

		bmp1 = m_ImageSlices[slice1].ReturnWork();
		bmp2 = m_ImageSlices[slice2].ReturnWork();
		const short n = slice2 - slice1;
		Point p;
		float delta;
		unsigned i1 = 0;

		for (p.py = 0; p.py < m_Height; p.py++)
		{
			for (p.px = 0; p.px < m_Width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				for (unsigned short j = 1; j < n; j++)
				{
					if (bmp1[i1] + delta * j >= 0)
						m_ImageSlices[slice1 + j].SetWorkPt(p, 255.0f);
					else
						m_ImageSlices[slice1 + j].SetWorkPt(p, 0.0f);
				}
				i1++;
			}
		}

		for (unsigned i = 0; i < m_Area; i++)
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
			m_ImageSlices[slice1 + j].SetMode(2, false);
		}

		m_ImageSlices[slice2].PopstackBmp();
		m_ImageSlices[slice1].PopstackBmp();
	}
	else
	{
		SlicesHandlerITKInterface itk_handler(this);
		auto tissues1 = itk_handler.GetTissuesSlice(slice1);
		auto tissues2 = itk_handler.GetTissuesSlice(slice2);

		ConnectedShapeBasedInterpolation interpolator;
		try
		{
			auto interpolated_slices = interpolator.Interpolate(tissues1, tissues2, tissuetype, n - 1, true);

			for (short i = 0; i < interpolated_slices.size(); ++i)
			{
				auto slice = interpolated_slices[i];
				const float* source = slice->GetPixelContainer()->GetImportPointer();
				size_t source_len = slice->GetPixelContainer()->Size();
				// copy to target (idx = slice1 + i), slice1 is included
				float* target = m_ImageSlices[slice1 + i].ReturnWork();
				std::copy(source, source + source_len, target);
			}
		}
		catch (std::exception& e)
		{
			ISEG_ERROR("failed to interpolate slices " << e.what());
		}
	}
}

void SlicesHandler::InterpolatetissueMedianset(unsigned short slice1, unsigned short slice2, tissues_size_t tissuetype, bool connectivity, bool handleVanishingComp)
{
	std::vector<float> mask(TissueLocks().size() + 1, 0.0f);
	mask.at(tissuetype) = 255.0f;

	m_ImageSlices[slice1].Tissue2work(m_ActiveTissuelayer, mask);
	m_ImageSlices[slice2].Tissue2work(m_ActiveTissuelayer, mask);
	InterpolateworkgreyMedianset(slice1, slice2, connectivity, true);
}

void SlicesHandler::Extrapolatetissue(unsigned short origin1, unsigned short origin2, unsigned short target, tissues_size_t tissuetype)
{
	if (origin2 < origin1)
	{
		unsigned short dummy = origin1;
		origin1 = origin2;
		origin2 = dummy;
	}

	tissues_size_t* tissue1 =
			m_ImageSlices[origin1].ReturnTissues(m_ActiveTissuelayer);
	tissues_size_t* tissue2 =
			m_ImageSlices[origin2].ReturnTissues(m_ActiveTissuelayer);
	m_ImageSlices[origin1].PushstackBmp();
	m_ImageSlices[origin2].PushstackBmp();
	m_ImageSlices[origin1].PushstackWork();
	m_ImageSlices[origin2].PushstackWork();
	float* bmp1 = m_ImageSlices[origin1].ReturnBmp();
	float* bmp2 = m_ImageSlices[origin2].ReturnBmp();
	for (unsigned int i = 0; i < m_Area; i++)
	{
		bmp1[i] = (float)tissue1[i];
		bmp2[i] = (float)tissue2[i];
	}

	m_ImageSlices[origin1].DeadReckoning((float)tissuetype);
	m_ImageSlices[origin2].DeadReckoning((float)tissuetype);

	bmp1 = m_ImageSlices[origin1].ReturnWork();
	bmp2 = m_ImageSlices[origin2].ReturnWork();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < m_Height; p.py++)
		{
			for (p.px = 0; p.px < m_Width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				if (bmp1[i1] + delta * (target - origin1) >= 0)
					m_ImageSlices[target].SetWorkPt(p, 255.0f);
				else
					m_ImageSlices[target].SetWorkPt(p, 0.0f);
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

	m_ImageSlices[target].SetMode(2, false);

	m_ImageSlices[origin2].PopstackWork();
	m_ImageSlices[origin1].PopstackWork();
	m_ImageSlices[origin2].PopstackBmp();
	m_ImageSlices[origin1].PopstackBmp();
}

void SlicesHandler::Interpolatework(unsigned short slice1, unsigned short slice2)
{
	if (slice2 < slice1)
	{
		unsigned short dummy = slice1;
		slice1 = slice2;
		slice2 = dummy;
	}

	//tissues_size_t *tissue1=image_slices[slice1].return_tissues(active_tissuelayer);
	//tissues_size_t *tissue2=image_slices[slice2].return_tissues(active_tissuelayer);
	m_ImageSlices[slice1].PushstackBmp();
	m_ImageSlices[slice2].PushstackBmp();
	float* bmp1 = m_ImageSlices[slice1].ReturnBmp();
	float* bmp2 = m_ImageSlices[slice2].ReturnBmp();
	float* work1 = m_ImageSlices[slice1].ReturnWork();
	float* work2 = m_ImageSlices[slice2].ReturnWork();

	for (unsigned int i = 0; i < m_Area; i++)
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

	m_ImageSlices[slice2].DeadReckoning(255.0f);
	m_ImageSlices[slice1].DeadReckoning(255.0f);

	bmp1 = m_ImageSlices[slice1].ReturnWork();
	bmp2 = m_ImageSlices[slice2].ReturnWork();
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < m_Height; p.py++)
		{
			for (p.px = 0; p.px < m_Width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				for (unsigned short j = 1; j < n; j++)
				{
					if (bmp1[i1] + delta * j >= 0)
						m_ImageSlices[slice1 + j].SetWorkPt(p, 255.0f);
					else
						m_ImageSlices[slice1 + j].SetWorkPt(p, 0.0f);
				}
				i1++;
			}
		}
	}

	for (unsigned i = 0; i < m_Area; i++)
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
		m_ImageSlices[slice1 + j].SetMode(2, false);
	}

	m_ImageSlices[slice2].PopstackBmp();
	m_ImageSlices[slice1].PopstackBmp();
}

void SlicesHandler::Extrapolatework(unsigned short origin1, unsigned short origin2, unsigned short target)
{
	if (origin2 < origin1)
	{
		unsigned short dummy = origin1;
		origin1 = origin2;
		origin2 = dummy;
	}

	//tissues_size_t *tissue1=image_slices[origin1].return_tissues(active_tissuelayer);
	//tissues_size_t *tissue2=image_slices[origin2].return_tissues(active_tissuelayer);
	m_ImageSlices[origin1].PushstackBmp();
	m_ImageSlices[origin2].PushstackBmp();
	m_ImageSlices[origin1].PushstackWork();
	m_ImageSlices[origin2].PushstackWork();
	float* bmp1 = m_ImageSlices[origin1].ReturnBmp();
	float* bmp2 = m_ImageSlices[origin2].ReturnBmp();
	float* work1 = m_ImageSlices[origin1].ReturnWork();
	float* work2 = m_ImageSlices[origin2].ReturnWork();

	for (unsigned int i = 0; i < m_Area; i++)
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

	m_ImageSlices[origin2].DeadReckoning(255.0f);
	m_ImageSlices[origin1].DeadReckoning(255.0f);

	bmp1 = m_ImageSlices[origin1].ReturnWork();
	bmp2 = m_ImageSlices[origin2].ReturnWork();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i1 = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < m_Height; p.py++)
		{
			for (p.px = 0; p.px < m_Width; p.px++)
			{
				delta = (bmp2[i1] - bmp1[i1]) / n;
				if (bmp1[i1] + delta * (target - origin1) >= 0)
					m_ImageSlices[target].SetWorkPt(p, 255.0f);
				else
					m_ImageSlices[target].SetWorkPt(p, 0.0f);
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

	m_ImageSlices[target].SetMode(2, false);

	m_ImageSlices[origin2].PopstackWork();
	m_ImageSlices[origin1].PopstackWork();
	m_ImageSlices[origin2].PopstackBmp();
	m_ImageSlices[origin1].PopstackBmp();
}

void SlicesHandler::Interpolate(unsigned short slice1, unsigned short slice2)
{
	float* bmp1 = m_ImageSlices[slice1].ReturnWork();
	float* bmp2 = m_ImageSlices[slice2].ReturnWork();
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i = 0;

	if (n != 0)
	{
		for (p.py = 0; p.py < m_Height; p.py++)
		{
			for (p.px = 0; p.px < m_Width; p.px++)
			{
				delta = (bmp2[i] - bmp1[i]) / n;
				for (unsigned short j = 1; j < n; j++)
					m_ImageSlices[slice1 + j].SetWorkPt(p, bmp1[i] + delta * j);
				i++;
			}
		}
	}

	for (unsigned short j = 1; j < n; j++)
	{
		m_ImageSlices[slice1 + j].SetMode(1, false);
	}
}

void SlicesHandler::Extrapolate(unsigned short origin1, unsigned short origin2, unsigned short target)
{
	float* bmp1 = m_ImageSlices[origin1].ReturnWork();
	float* bmp2 = m_ImageSlices[origin2].ReturnWork();
	const short n = origin2 - origin1;
	Point p;
	float delta;
	unsigned i = 0;

	for (p.py = 0; p.py < m_Height; p.py++)
	{
		for (p.px = 0; p.px < m_Width; p.px++)
		{
			delta = (bmp2[i] - bmp1[i]) / n;
			m_ImageSlices[target].SetWorkPt(p, bmp1[i] +
																						 delta * (target - origin1));
			i++;
		}
	}

	m_ImageSlices[target].SetMode(1, false);
}

void SlicesHandler::Interpolate(unsigned short slice1, unsigned short slice2, float* bmp1, float* bmp2)
{
	const short n = slice2 - slice1;
	Point p;
	float delta;
	unsigned i = 0;

	for (p.py = 0; p.py < m_Height; p.py++)
	{
		for (p.px = 0; p.px < m_Width; p.px++)
		{
			delta = (bmp2[i] - bmp1[i]) / n;
			for (unsigned short j = 0; j <= n; j++)
				m_ImageSlices[slice1 + j].SetWorkPt(p, bmp1[i] + delta * j);
			i++;
		}
	}

	for (unsigned short j = 1; j < n; j++)
	{
		m_ImageSlices[slice1 + j].SetMode(1, false);
	}
}

void SlicesHandler::SetSlicethickness(float t)
{
	m_Thickness = t;
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		m_Os.SetThickness(t, i);
	}
}

float SlicesHandler::GetSlicethickness() const { return m_Thickness; }

void SlicesHandler::SetPixelsize(float dx1, float dy1)
{
	m_Dx = dx1;
	m_Dy = dy1;
	m_Os.SetPixelsize(m_Dx, m_Dy);
}

Pair SlicesHandler::GetPixelsize() const
{
	Pair p;
	p.high = m_Dx;
	p.low = m_Dy;
	return p;
}

Transform SlicesHandler::ImageTransform() const { return m_Transform; }

Transform SlicesHandler::GetTransformActiveSlices() const
{
	int plo[3] = {0, 0, -static_cast<int>(m_Startslice)};

	Transform tr_corrected(m_Transform);
	tr_corrected.PaddingUpdateTransform(plo, Spacing());
	return tr_corrected;
}

void SlicesHandler::SetTransform(const Transform& tr) { m_Transform = tr; }

Vec3 SlicesHandler::Spacing() const
{
	return Vec3(m_Dx, m_Dy, m_Thickness);
}

void SlicesHandler::GetDisplacement(float disp[3]) const
{
	m_Transform.GetOffset(disp);
}

void SlicesHandler::SetDisplacement(const float disp[3])
{
	m_Transform.SetOffset(disp);
}

void SlicesHandler::GetDirectionCosines(float dc[6]) const
{
	for (unsigned short i = 0; i < 3; i++)
	{
		dc[i] = m_Transform[i][0];
		dc[i + 3] = m_Transform[i][1];
	}
}

void SlicesHandler::SetDirectionCosines(const float dc[6])
{
	float offset[3];
	m_Transform.GetOffset(offset);
	m_Transform.SetTransform(offset, dc);
}

void SlicesHandler::SlicebmpX(float* return_bits, unsigned short xcoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		dummy = m_ImageSlices[i].ReturnBmp();
		for (unsigned short j = 0; j < m_Height; j++)
		{
			return_bits[n] = dummy[j * m_Width + xcoord];
			n++;
		}
	}
}

void SlicesHandler::SlicebmpY(float* return_bits, unsigned short ycoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		dummy = m_ImageSlices[i].ReturnBmp();
		for (unsigned short j = 0; j < m_Width; j++)
		{
			return_bits[n] = dummy[j + ycoord * m_Width];
			n++;
		}
	}
}

float* SlicesHandler::SlicebmpX(unsigned short xcoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(m_Height) * m_Nrslices);
	SlicebmpX(result, xcoord);

	return result;
}

float* SlicesHandler::SlicebmpY(unsigned short ycoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(m_Width) * m_Nrslices);
	SlicebmpY(result, ycoord);

	return result;
}

void SlicesHandler::SliceworkX(float* return_bits, unsigned short xcoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		dummy = m_ImageSlices[i].ReturnWork();
		for (unsigned short j = 0; j < m_Height; j++)
		{
			return_bits[n] = dummy[j * m_Width + xcoord];
			n++;
		}
	}
}

void SlicesHandler::SliceworkY(float* return_bits, unsigned short ycoord)
{
	unsigned n = 0;
	float* dummy;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		dummy = m_ImageSlices[i].ReturnWork();
		for (unsigned short j = 0; j < m_Width; j++)
		{
			return_bits[n] = dummy[j + ycoord * m_Width];
			n++;
		}
	}
}

float* SlicesHandler::SliceworkX(unsigned short xcoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(m_Height) * m_Nrslices);
	SliceworkX(result, xcoord);

	return result;
}

float* SlicesHandler::SliceworkY(unsigned short ycoord)
{
	float* result = (float*)malloc(sizeof(float) * unsigned(m_Width) * m_Nrslices);
	SliceworkY(result, ycoord);

	return result;
}

void SlicesHandler::SlicetissueX(tissues_size_t* return_bits, unsigned short xcoord)
{
	unsigned n = 0;
	tissues_size_t* dummy;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		dummy = m_ImageSlices[i].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned short j = 0; j < m_Height; j++)
		{
			return_bits[n] = dummy[j * m_Width + xcoord];
			n++;
		}
	}
}

void SlicesHandler::SlicetissueY(tissues_size_t* return_bits, unsigned short ycoord)
{
	unsigned n = 0;
	tissues_size_t* dummy;

	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		dummy = m_ImageSlices[i].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned short j = 0; j < m_Width; j++)
		{
			return_bits[n] = dummy[j + ycoord * m_Width];
			n++;
		}
	}
}

tissues_size_t* SlicesHandler::SlicetissueX(unsigned short xcoord)
{
	tissues_size_t* result = (tissues_size_t*)malloc(sizeof(tissues_size_t) * unsigned(m_Height) * m_Nrslices);
	SlicetissueX(result, xcoord);

	return result;
}

tissues_size_t* SlicesHandler::SlicetissueY(unsigned short ycoord)
{
	tissues_size_t* result = (tissues_size_t*)malloc(sizeof(tissues_size_t) * unsigned(m_Width) * m_Nrslices);
	SlicetissueY(result, ycoord);

	return result;
}

void SlicesHandler::SliceworkZ(unsigned short slicenr)
{
	m_ImageSlices[slicenr].ReturnWork();
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

int SlicesHandler::ExtractTissueSurfaces(const QString& filename, std::vector<tissues_size_t>& tissuevec, bool usediscretemc, float ratio, unsigned smoothingiterations, float passBand, float featureAngle)
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
	const char* tissue_index_array_name = "Domain";			// this can be changed
	const char* tissue_name_array_name = "TissueNames"; // don't modify this
	const char* tissue_color_array_name = "Colors";			// don't modify this

	vtkSmartPointer<vtkImageData> label_field =
			vtkSmartPointer<vtkImageData>::New();
	label_field->SetExtent(0, (int)Width() + 1, 0, (int)Height() + 1, 0, (int)(m_Endslice - m_Startslice) + 1);
	Pair ps = GetPixelsize();
	label_field->SetSpacing(ps.high, ps.low, GetSlicethickness());
	// transform (translation and rotation) is applied at end of function
	label_field->SetOrigin(0, 0, 0);
	label_field->AllocateScalars(GetScalarType<tissues_size_t>(), 1);
	vtkDataArray* arr = label_field->GetPointData()->GetScalars();
	if (!arr)
	{
		ISEG_ERROR_MSG("no scalars");
		return -1;
	}
	arr->SetName(tissue_index_array_name);
	label_field->GetPointData()->SetActiveScalars(tissue_index_array_name);

	//
	// Copy tissue names and colors into labelField
	//
	tissues_size_t num_tissues = TissueInfos::GetTissueCount();
	vtkSmartPointer<vtkStringArray> names_array =
			vtkSmartPointer<vtkStringArray>::New();
	names_array->SetNumberOfTuples(num_tissues + 1);
	names_array->SetName(tissue_name_array_name);

	vtkSmartPointer<vtkFloatArray> color_array =
			vtkSmartPointer<vtkFloatArray>::New();
	color_array->SetNumberOfComponents(3);
	color_array->SetNumberOfTuples(num_tissues + 1);
	color_array->SetName(tissue_color_array_name);

	for (tissues_size_t i = 1; i < num_tissues; i++)
	{
		check_equal(TissueInfos::GetTissueType(TissueInfos::GetTissueName(i)), i);
		names_array->SetValue(i, TissueInfos::GetTissueName(i).c_str());
		auto color = TissueInfos::GetTissueColor(i);
		color_array->SetTuple(i, color.v.data());
	}

	label_field->GetFieldData()->AddArray(names_array);
	label_field->GetFieldData()->AddArray(color_array);

	tissues_size_t* field = (tissues_size_t*)label_field->GetScalarPointer(0, 0, 0);
	if (!field)
	{
		ISEG_ERROR_MSG("null pointer");
		return -1;
	}

	int const padding = 1;
	//labelField is already cropped
	unsigned short nr_slice = 0;
	for (unsigned short i = m_Startslice; i < m_Endslice; i++, nr_slice++)
	{
		Copyfromtissuepadded(i, &(field[(nr_slice + 1) * (unsigned long long)(Width() + 2) * (Height() + 2)]), padding);
	}
	for (unsigned long long i = 0;
			 i < (unsigned long long)(Width() + 2) * (Height() + 2);
			 i++)
		field[i] = 0;
	for (unsigned long long i = (m_Endslice - m_Startslice + 1) *
															(unsigned long long)(Width() + 2) *
															(Height() + 2);
			 i < (m_Endslice - m_Startslice + 2) *
							 (unsigned long long)(Width() + 2) *
							 (Height() + 2);
			 i++)
		field[i] = 0;

	// Check the label field
	check(label_field != nullptr);
	//check( labelField->GetPointData()->HasArray(tissueIndexArrayName) );
	check(label_field->GetFieldData()->HasArray(tissue_name_array_name));
	check(label_field->GetFieldData()->HasArray(tissue_color_array_name));

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
		cubes->SetInputData(label_field);
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
		contour->SetInputData(label_field);
		contour->SetOutputScalarName(tissue_index_array_name);
		contour->UseTemplatesOn();
		contour->UseOctreeLocatorOff();
		contour->FiveTetrahedraPerVoxelOn();
		contour->SetBackgroundLabel(0); /// \todo this will not be extracted! is this correct?
		contour->Update();
		output = contour->GetOutput();
	}
	if (output)
	{
		//check( output->GetCellData()->HasArray(tissueIndexArrayName) );
		check(output->GetFieldData()->HasArray(tissue_name_array_name));
		check(output->GetFieldData()->HasArray(tissue_color_array_name));
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
		check(output->GetFieldData()->HasArray(tissue_name_array_name));
		check(output->GetFieldData()->HasArray(tissue_color_array_name));
	}

	//
	// Simplify surface
	// This originally used vtkDecimatePro (see svn rev. 2453 or earlier), // however, vtkEdgeCollapse avoids topological errors and self-intersections.
	//
	if (ratio < 0.02)
		ratio = 0.02;
	double target_reduction = 1.0 - ratio;
	// don't bother if below reduction rate of 5%
	if (target_reduction > 0.05)
	{
		double cell_bnds[6];
		label_field->GetCellBounds(0, cell_bnds);
		double x0[3] = {cell_bnds[0], cell_bnds[2], cell_bnds[4]};
		double x1[3] = {cell_bnds[1], cell_bnds[3], cell_bnds[5]};
		// cellSize is related to current edge length
		double cell_size =
				0.5 * std::sqrt(vtkMath::Distance2BetweenPoints(x0, x1));

		// min edge length -> cellSize  :  no edges will be collapsed, no edge shorter than 0
		// min edge length -> inf:  all edges will be collapsed, ""
		// If edge length is halved, number of triangles multiplies by 4
		double min_edge_length = cell_size / (ratio * ratio);

		simplify->SetInputData(output);
		simplify->SetDomainLabelName(tissue_index_array_name);
		simplify->SetMinimumEdgeLength(min_edge_length);
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
			names_array_1->SetName(tissue_name_array_name);

			vtkSmartPointer<vtkFloatArray> color_array_1 =
					vtkSmartPointer<vtkFloatArray>::New();
			color_array_1->SetNumberOfComponents(3);
			color_array_1->SetNumberOfTuples(1);
			color_array_1->SetName(tissue_color_array_name);

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
		check(output->GetFieldData()->HasArray(tissue_name_array_name));
		check(output->GetFieldData()->HasArray(tissue_color_array_name));
	}

	//
	// Transform surface
	//
	std::vector<double> elems(16, 0.0);
	elems.back() = 1.0;
	double padding_displacement[3] = {-padding * ps.high, -padding * ps.low, -padding * m_Thickness + m_Thickness * m_Startslice};
	for (int i = 0; i < 3; i++)
	{
		elems[i * 4 + 0] = m_Transform[i][0];
		elems[i * 4 + 1] = m_Transform[i][1];
		elems[i * 4 + 2] = m_Transform[i][2];
		elems[i * 4 + 3] = m_Transform[i][3] + padding_displacement[i];
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
	writer->SetMaterialArrayName(tissue_index_array_name);
	writer->Write();

	return error_counter;
}

void SlicesHandler::Add2tissue(tissues_size_t tissuetype, Point p, bool override)
{
	m_ImageSlices[m_Activeslice].Add2tissue(m_ActiveTissuelayer, tissuetype, p, override);
}

void SlicesHandler::Add2tissue(tissues_size_t tissuetype, bool* mask, unsigned short slicenr, bool override)
{
	m_ImageSlices[slicenr].Add2tissue(m_ActiveTissuelayer, tissuetype, mask, override);
}

void SlicesHandler::Add2tissueall(tissues_size_t tissuetype, Point p, bool override)
{
	float f = m_ImageSlices[m_Activeslice].WorkPt(p);
	Add2tissueall(tissuetype, f, override);
}

void SlicesHandler::Add2tissueConnected(tissues_size_t tissuetype, Point p, bool override)
{
	m_ImageSlices[m_Activeslice].Add2tissueConnected(m_ActiveTissuelayer, tissuetype, p, override);
}

void SlicesHandler::Add2tissueThresh(tissues_size_t tissuetype, Point p)
{
	m_ImageSlices[m_Activeslice].Add2tissueThresh(m_ActiveTissuelayer, tissuetype, p);
}

void SlicesHandler::SubtractTissue(tissues_size_t tissuetype, Point p)
{
	m_ImageSlices[m_Activeslice].SubtractTissue(m_ActiveTissuelayer, tissuetype, p);
}

void SlicesHandler::SubtractTissueall(tissues_size_t tissuetype, Point p)
{
	float f = m_ImageSlices[m_Activeslice].WorkPt(p);
	SubtractTissueall(tissuetype, f);
}

void SlicesHandler::SubtractTissueall(tissues_size_t tissuetype, Point p, unsigned short slicenr)
{
	if (slicenr >= 0 && slicenr < m_Nrslices)
	{
		float f = m_ImageSlices[slicenr].WorkPt(p);
		SubtractTissueall(tissuetype, f);
	}
}

void SlicesHandler::SubtractTissueall(tissues_size_t tissuetype, float f)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].SubtractTissue(m_ActiveTissuelayer, tissuetype, f);
	}
}

void SlicesHandler::SubtractTissueConnected(tissues_size_t tissuetype, Point p)
{
	m_ImageSlices[m_Activeslice].SubtractTissueConnected(m_ActiveTissuelayer, tissuetype, p);
}

void SlicesHandler::Selectedtissue2work(const std::vector<tissues_size_t>& tissuetype)
{
	std::vector<float> mask(TissueLocks().size() + 1, 0.0f);
	for (auto label : tissuetype)
	{
		mask.at(label) = 255.0f;
	}

	m_ImageSlices[m_Activeslice].Tissue2work(m_ActiveTissuelayer, mask);
}

void SlicesHandler::Selectedtissue2work3D(const std::vector<tissues_size_t>& tissuetype)
{
	std::vector<float> mask(TissueLocks().size() + 1, 0.0f);
	for (auto label : tissuetype)
	{
		mask.at(label) = 255.0f;
	}

	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].Tissue2work(m_ActiveTissuelayer, mask);
	}
}

void SlicesHandler::Cleartissue(tissues_size_t tissuetype)
{
	m_ImageSlices[m_Activeslice].Cleartissue(m_ActiveTissuelayer, tissuetype);
}

void SlicesHandler::Cleartissue3D(tissues_size_t tissuetype)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].Cleartissue(m_ActiveTissuelayer, tissuetype);
	}
}

void SlicesHandler::Cleartissues()
{
	m_ImageSlices[m_Activeslice].Cleartissues(m_ActiveTissuelayer);
}

void SlicesHandler::Cleartissues3D()
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].Cleartissues(m_ActiveTissuelayer);
	}
}

void SlicesHandler::Add2tissueall(tissues_size_t tissuetype, Point p, unsigned short slicenr, bool override)
{
	if (slicenr >= 0 && slicenr < m_Nrslices)
	{
		float f = m_ImageSlices[slicenr].WorkPt(p);
		Add2tissueall(tissuetype, f, override);
	}
}

void SlicesHandler::Add2tissueall(tissues_size_t tissuetype, float f, bool override)
{
	int const i_n = m_Endslice;

#pragma omp parallel for
	for (int i = m_Startslice; i < i_n; i++)
	{
		m_ImageSlices[i].Add2tissue(m_ActiveTissuelayer, tissuetype, f, override);
	}
}

void SlicesHandler::NextSlice() { SetActiveSlice(m_Activeslice + 1); }

void SlicesHandler::PrevSlice()
{
	if (m_Activeslice > 0)
		SetActiveSlice(m_Activeslice - 1);
}

unsigned short SlicesHandler::GetNextFeaturingSlice(tissues_size_t type, bool& found)
{
	found = true;
	for (unsigned i = m_Activeslice + 1; i < m_Nrslices; i++)
	{
		if (m_ImageSlices[i].HasTissue(m_ActiveTissuelayer, type))
		{
			return i;
		}
	}
	for (unsigned i = 0; i <= m_Activeslice; i++)
	{
		if (m_ImageSlices[i].HasTissue(m_ActiveTissuelayer, type))
		{
			return i;
		}
	}

	found = false;
	return m_Activeslice;
}

unsigned short SlicesHandler::ActiveSlice() const { return m_Activeslice; }

void SlicesHandler::SetActiveSlice(unsigned short slice, bool signal_change)
{
	if (slice < m_Nrslices && slice != m_Activeslice)
	{
		m_Activeslice = slice;

		// notify observers that slice changed
		if (signal_change)
		{
			m_OnActiveSliceChanged(slice);
		}
	}
}

Bmphandler* SlicesHandler::GetActivebmphandler()
{
	return &(m_ImageSlices[m_Activeslice]);
}

tissuelayers_size_t SlicesHandler::ActiveTissuelayer() const
{
	return m_ActiveTissuelayer;
}

void SlicesHandler::SetActiveTissuelayer(tissuelayers_size_t idx)
{
	// TODO: Signaling, range checking
	m_ActiveTissuelayer = idx;
}

unsigned SlicesHandler::PushstackBmp()
{
	return GetActivebmphandler()->PushstackBmp();
}

unsigned SlicesHandler::PushstackBmp(unsigned int slice)
{
	return m_ImageSlices[slice].PushstackBmp();
}

unsigned SlicesHandler::PushstackWork()
{
	return GetActivebmphandler()->PushstackWork();
}

unsigned SlicesHandler::PushstackWork(unsigned int slice)
{
	return m_ImageSlices[slice].PushstackWork();
}

unsigned SlicesHandler::PushstackTissue(tissues_size_t i)
{
	return GetActivebmphandler()->PushstackTissue(m_ActiveTissuelayer, i);
}

unsigned SlicesHandler::PushstackTissue(tissues_size_t i, unsigned int slice)
{
	return m_ImageSlices[slice].PushstackTissue(m_ActiveTissuelayer, i);
}

unsigned SlicesHandler::PushstackHelp()
{
	return GetActivebmphandler()->PushstackHelp();
}

void SlicesHandler::Removestack(unsigned i)
{
	GetActivebmphandler()->Removestack(i);
}

void SlicesHandler::ClearStack() { GetActivebmphandler()->ClearStack(); }

void SlicesHandler::GetstackBmp(unsigned i)
{
	GetActivebmphandler()->GetstackBmp(i);
}

void SlicesHandler::GetstackBmp(unsigned int slice, unsigned i)
{
	m_ImageSlices[slice].GetstackBmp(i);
}

void SlicesHandler::GetstackWork(unsigned i)
{
	GetActivebmphandler()->GetstackWork(i);
}

void SlicesHandler::GetstackWork(unsigned int slice, unsigned i)
{
	m_ImageSlices[slice].GetstackWork(i);
}

void SlicesHandler::GetstackHelp(unsigned i)
{
	GetActivebmphandler()->GetstackHelp(i);
}

void SlicesHandler::GetstackTissue(unsigned i, tissues_size_t tissuenr, bool override)
{
	GetActivebmphandler()->GetstackTissue(m_ActiveTissuelayer, i, tissuenr, override);
}

void SlicesHandler::GetstackTissue(unsigned int slice, unsigned i, tissues_size_t tissuenr, bool override)
{
	m_ImageSlices[slice].GetstackTissue(m_ActiveTissuelayer, i, tissuenr, override);
}

float* SlicesHandler::Getstack(unsigned i, unsigned char& mode)
{
	return GetActivebmphandler()->Getstack(i, mode);
}

void SlicesHandler::PopstackBmp() { GetActivebmphandler()->PopstackBmp(); }

void SlicesHandler::PopstackWork() { GetActivebmphandler()->PopstackWork(); }

void SlicesHandler::PopstackHelp() { GetActivebmphandler()->PopstackHelp(); }

unsigned SlicesHandler::Loadstack(const char* filename)
{
	return GetActivebmphandler()->Loadstack(filename);
}

void SlicesHandler::Savestack(unsigned i, const char* filename)
{
	GetActivebmphandler()->Savestack(i, filename);
}

void SlicesHandler::StartUndo(DataSelection& dataSelection)
{
	if (m_Uelem == nullptr)
	{
		m_Uelem = new UndoElem;
		m_Uelem->m_DataSelection = dataSelection;

		if (dataSelection.bmp)
		{
			m_Uelem->m_BmpOld = m_ImageSlices[dataSelection.sliceNr].CopyBmp();
			m_Uelem->m_Mode1Old = m_ImageSlices[dataSelection.sliceNr].ReturnMode(true);
		}
		else
		{
			m_Uelem->m_BmpOld = nullptr;
		}

		if (dataSelection.work)
		{
			m_Uelem->m_WorkOld = m_ImageSlices[dataSelection.sliceNr].CopyWork();
			m_Uelem->m_Mode2Old = m_ImageSlices[dataSelection.sliceNr].ReturnMode(false);
		}
		else
		{
			m_Uelem->m_WorkOld = nullptr;
		}

		if (dataSelection.tissues)
		{
			m_Uelem->m_TissueOld = m_ImageSlices[dataSelection.sliceNr].CopyTissue(m_ActiveTissuelayer);
		}
		else
		{
			m_Uelem->m_TissueOld = nullptr;
		}

		m_Uelem->m_VvmOld.clear();
		if (dataSelection.vvm)
		{
			m_Uelem->m_VvmOld = *(m_ImageSlices[dataSelection.sliceNr].ReturnVvm());
		}

		m_Uelem->m_LimitsOld.clear();
		if (dataSelection.limits)
		{
			m_Uelem->m_LimitsOld = *(m_ImageSlices[dataSelection.sliceNr].ReturnLimits());
		}

		m_Uelem->m_MarksOld.clear();
		if (dataSelection.marks)
		{
			m_Uelem->m_MarksOld = *(m_ImageSlices[dataSelection.sliceNr].ReturnMarks());
		}
	}
}

bool SlicesHandler::StartUndoall(DataSelection& dataSelection)
{
	//abcd std::vector<unsigned short> vslicenr1;
	std::vector<unsigned> vslicenr1;
	for (unsigned i = 0; i < m_Nrslices; i++)
		vslicenr1.push_back(i);

	return StartUndo(dataSelection, vslicenr1);
}

//abcd bool SlicesHandler::start_undo(common::DataSelection &dataSelection,std::vector<unsigned short> vslicenr1)
bool SlicesHandler::StartUndo(DataSelection& dataSelection, std::vector<unsigned> vslicenr1)
{
	if (m_Uelem == nullptr)
	{
		MultiUndoElem* uelem1 = new MultiUndoElem;
		m_Uelem = uelem1;
		//		uelem=new MultiUndoElem;
		//		MultiUndoElem *uelem1=dynamic_cast<MultiUndoElem *>(uelem);
		m_Uelem->m_DataSelection = dataSelection;
		uelem1->m_Vslicenr = vslicenr1;

		if (m_Uelem->Arraynr() < this->m_UndoQueue.ReturnNrundoarraysmax())
		{
			//abcd std::vector<unsigned short>::iterator it;
			std::vector<unsigned>::iterator it;
			uelem1->m_VbmpOld.clear();
			uelem1->m_Vmode1Old.clear();
			if (dataSelection.bmp)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
				{
					uelem1->m_VbmpOld.push_back(m_ImageSlices[*it].CopyBmp());
					uelem1->m_Vmode1Old.push_back(m_ImageSlices[*it].ReturnMode(true));
				}
			uelem1->m_VworkOld.clear();
			uelem1->m_Vmode2Old.clear();
			if (dataSelection.work)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
				{
					uelem1->m_VworkOld.push_back(m_ImageSlices[*it].CopyWork());
					uelem1->m_Vmode2Old.push_back(m_ImageSlices[*it].ReturnMode(false));
				}
			uelem1->m_VtissueOld.clear();
			if (dataSelection.tissues)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->m_VtissueOld.push_back(m_ImageSlices[*it].CopyTissue(m_ActiveTissuelayer));
			uelem1->m_VvvmOld.clear();
			if (dataSelection.vvm)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->m_VvvmOld.push_back(*(m_ImageSlices[*it].ReturnVvm()));
			uelem1->m_VlimitsOld.clear();
			if (dataSelection.limits)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->m_VlimitsOld.push_back(*(m_ImageSlices[*it].ReturnLimits()));
			uelem1->m_MarksOld.clear();
			if (dataSelection.marks)
				for (it = vslicenr1.begin(); it != vslicenr1.end(); it++)
					uelem1->m_VmarksOld.push_back(*(m_ImageSlices[*it].ReturnMarks()));

			return true;
		}
		else
		{
			delete m_Uelem;
			m_Uelem = nullptr;
		}
	}

	return false;
}

void SlicesHandler::AbortUndo()
{
	if (m_Uelem != nullptr)
	{
		delete m_Uelem;
		m_Uelem = nullptr;
	}
}

void SlicesHandler::EndUndo()
{
	if (m_Uelem != nullptr)
	{
		if (m_Uelem->Multi())
		{
			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(m_Uelem);

			uelem1->m_VbmpNew.clear();
			uelem1->m_Vmode1New.clear();

			uelem1->m_VworkNew.clear();
			uelem1->m_Vmode2New.clear();

			uelem1->m_VtissueNew.clear();

			uelem1->m_VvvmNew.clear();

			uelem1->m_VlimitsNew.clear();

			uelem1->m_MarksNew.clear();

			this->m_UndoQueue.AddUndo(uelem1);

			m_Uelem = nullptr;
		}
		else
		{
			m_Uelem->m_BmpNew = nullptr;
			m_Uelem->m_Mode1New = 0;

			m_Uelem->m_WorkNew = nullptr;
			m_Uelem->m_Mode2New = 0;

			m_Uelem->m_TissueNew = nullptr;

			m_Uelem->m_VvmNew.clear();

			m_Uelem->m_LimitsNew.clear();

			m_Uelem->m_MarksNew.clear();

			this->m_UndoQueue.AddUndo(m_Uelem);

			m_Uelem = nullptr;
		}
	}
}

void SlicesHandler::MergeUndo()
{
	if (m_Uelem != nullptr && !m_Uelem->Multi())
	{
		DataSelection data_selection = m_Uelem->m_DataSelection;

		this->m_UndoQueue.MergeUndo(m_Uelem);

		if (data_selection.bmp)
		{
			m_Uelem->m_BmpNew = nullptr;
			m_Uelem->m_BmpOld = nullptr;
		}
		if (data_selection.work)
		{
			m_Uelem->m_WorkNew = nullptr;
			m_Uelem->m_WorkOld = nullptr;
		}
		if (data_selection.tissues)
		{
			m_Uelem->m_TissueNew = nullptr;
			m_Uelem->m_TissueOld = nullptr;
		}

		delete m_Uelem;
		m_Uelem = nullptr;
	}
}

DataSelection SlicesHandler::Undo()
{
	if (m_Uelem == nullptr)
	{
		m_Uelem = this->m_UndoQueue.Undo();
		if (m_Uelem->Multi())
		{
			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(m_Uelem);

			if (uelem1 != nullptr)
			{
				unsigned short current_slice;
				DataSelection data_selection = m_Uelem->m_DataSelection;

				for (unsigned i = 0; i < uelem1->m_Vslicenr.size(); i++)
				{
					current_slice = uelem1->m_Vslicenr[i];
					if (data_selection.bmp)
					{
						uelem1->m_VbmpNew.push_back(m_ImageSlices[current_slice].CopyBmp());
						uelem1->m_Vmode1New.push_back(m_ImageSlices[current_slice].ReturnMode(true));
						m_ImageSlices[current_slice].Copy2bmp(uelem1->m_VbmpOld[i], uelem1->m_Vmode1Old[i]);
						free(uelem1->m_VbmpOld[i]);
					}
					if (data_selection.work)
					{
						uelem1->m_VworkNew.push_back(m_ImageSlices[current_slice].CopyWork());
						uelem1->m_Vmode2New.push_back(m_ImageSlices[current_slice].ReturnMode(false));
						m_ImageSlices[current_slice].Copy2work(uelem1->m_VworkOld[i], uelem1->m_Vmode2Old[i]);
						free(uelem1->m_VworkOld[i]);
					}
					if (data_selection.tissues)
					{
						uelem1->m_VtissueNew.push_back(m_ImageSlices[current_slice].CopyTissue(m_ActiveTissuelayer));
						m_ImageSlices[current_slice].Copy2tissue(m_ActiveTissuelayer, uelem1->m_VtissueOld[i]);
						free(uelem1->m_VtissueOld[i]);
					}
					if (data_selection.vvm)
					{
						uelem1->m_VvvmNew.push_back(*(m_ImageSlices[current_slice].ReturnVvm()));
						m_ImageSlices[current_slice].Copy2vvm(&(uelem1->m_VvvmOld[i]));
					}
					if (data_selection.limits)
					{
						uelem1->m_VlimitsNew.push_back(*(m_ImageSlices[current_slice].ReturnLimits()));
						m_ImageSlices[current_slice].Copy2limits(&(uelem1->m_VlimitsOld[i]));
					}
					if (data_selection.marks)
					{
						uelem1->m_VmarksNew.push_back(*(m_ImageSlices[current_slice].ReturnMarks()));
						m_ImageSlices[current_slice].Copy2marks(&(uelem1->m_VmarksOld[i]));
					}
				}
				uelem1->m_VbmpOld.clear();
				uelem1->m_Vmode1Old.clear();
				uelem1->m_VworkOld.clear();
				uelem1->m_Vmode2Old.clear();
				uelem1->m_VtissueOld.clear();
				uelem1->m_VvvmOld.clear();
				uelem1->m_VlimitsOld.clear();
				uelem1->m_VmarksOld.clear();

				m_Uelem = nullptr;

				return data_selection;
			}
			else
				return {};
		}
		else
		{
			if (m_Uelem != nullptr)
			{
				DataSelection data_selection = m_Uelem->m_DataSelection;

				if (data_selection.bmp)
				{
					m_Uelem->m_BmpNew = m_ImageSlices[data_selection.sliceNr].CopyBmp();
					m_Uelem->m_Mode1New = m_ImageSlices[data_selection.sliceNr].ReturnMode(true);
					m_ImageSlices[data_selection.sliceNr].Copy2bmp(m_Uelem->m_BmpOld, m_Uelem->m_Mode1Old);
					free(m_Uelem->m_BmpOld);
					m_Uelem->m_BmpOld = nullptr;
				}

				if (data_selection.work)
				{
					m_Uelem->m_WorkNew = m_ImageSlices[data_selection.sliceNr].CopyWork();
					m_Uelem->m_Mode2New = m_ImageSlices[data_selection.sliceNr].ReturnMode(false);
					m_ImageSlices[data_selection.sliceNr].Copy2work(m_Uelem->m_WorkOld, m_Uelem->m_Mode2Old);
					free(m_Uelem->m_WorkOld);
					m_Uelem->m_WorkOld = nullptr;
				}

				if (data_selection.tissues)
				{
					m_Uelem->m_TissueNew = m_ImageSlices[data_selection.sliceNr].CopyTissue(m_ActiveTissuelayer);
					m_ImageSlices[data_selection.sliceNr].Copy2tissue(m_ActiveTissuelayer, m_Uelem->m_TissueOld);
					free(m_Uelem->m_TissueOld);
					m_Uelem->m_TissueOld = nullptr;
				}

				if (data_selection.vvm)
				{
					m_Uelem->m_VvmNew.clear();
					m_Uelem->m_VvmNew = *(m_ImageSlices[data_selection.sliceNr].ReturnVvm());
					m_ImageSlices[data_selection.sliceNr].Copy2vvm(&m_Uelem->m_VvmOld);
					m_Uelem->m_VvmOld.clear();
				}

				if (data_selection.limits)
				{
					m_Uelem->m_LimitsNew.clear();
					m_Uelem->m_LimitsNew = *(m_ImageSlices[data_selection.sliceNr].ReturnLimits());
					m_ImageSlices[data_selection.sliceNr].Copy2limits(&m_Uelem->m_LimitsOld);
					m_Uelem->m_LimitsOld.clear();
				}

				if (data_selection.marks)
				{
					m_Uelem->m_MarksNew.clear();
					m_Uelem->m_MarksNew = *(m_ImageSlices[data_selection.sliceNr].ReturnMarks());
					m_ImageSlices[data_selection.sliceNr].Copy2marks(&m_Uelem->m_MarksOld);
					m_Uelem->m_MarksOld.clear();
				}

				SetActiveSlice(data_selection.sliceNr);

				m_Uelem = nullptr;

				return data_selection;
			}
			else
				return DataSelection();
		}
	}
	else
		return {};
}

DataSelection SlicesHandler::Redo()
{
	if (m_Uelem == nullptr)
	{
		m_Uelem = this->m_UndoQueue.Redo();
		if (m_Uelem == nullptr)
			return {};
		if (m_Uelem->Multi())
		{
			MultiUndoElem* uelem1 = dynamic_cast<MultiUndoElem*>(m_Uelem);

			if (uelem1 != nullptr)
			{
				unsigned short current_slice;
				DataSelection data_selection = m_Uelem->m_DataSelection;
				for (unsigned i = 0; i < uelem1->m_Vslicenr.size(); i++)
				{
					current_slice = uelem1->m_Vslicenr[i];
					if (data_selection.bmp)
					{
						uelem1->m_VbmpOld.push_back(m_ImageSlices[current_slice].CopyBmp());
						uelem1->m_Vmode1Old.push_back(m_ImageSlices[current_slice].ReturnMode(true));
						m_ImageSlices[current_slice].Copy2bmp(uelem1->m_VbmpNew[i], uelem1->m_Vmode1New[i]);
						free(uelem1->m_VbmpNew[i]);
					}
					if (data_selection.work)
					{
						uelem1->m_VworkOld.push_back(m_ImageSlices[current_slice].CopyWork());
						uelem1->m_Vmode2Old.push_back(m_ImageSlices[current_slice].ReturnMode(false));
						m_ImageSlices[current_slice].Copy2work(uelem1->m_VworkNew[i], uelem1->m_Vmode2New[i]);
						free(uelem1->m_VworkNew[i]);
					}
					if (data_selection.tissues)
					{
						uelem1->m_VtissueOld.push_back(m_ImageSlices[current_slice].CopyTissue(m_ActiveTissuelayer));
						m_ImageSlices[current_slice].Copy2tissue(m_ActiveTissuelayer, uelem1->m_VtissueNew[i]);
						free(uelem1->m_VtissueNew[i]);
					}
					if (data_selection.vvm)
					{
						uelem1->m_VvvmOld.push_back(*(m_ImageSlices[current_slice].ReturnVvm()));
						m_ImageSlices[current_slice].Copy2vvm(&(uelem1->m_VvvmNew[i]));
					}
					if (data_selection.limits)
					{
						uelem1->m_VlimitsOld.push_back(*(m_ImageSlices[current_slice].ReturnLimits()));
						m_ImageSlices[current_slice].Copy2limits(&(uelem1->m_VlimitsNew[i]));
					}
					if (data_selection.marks)
					{
						uelem1->m_VmarksOld.push_back(*(m_ImageSlices[current_slice].ReturnMarks()));
						m_ImageSlices[current_slice].Copy2marks(&(uelem1->m_VmarksNew[i]));
					}
				}
				uelem1->m_VbmpNew.clear();
				uelem1->m_VworkNew.clear();
				uelem1->m_VtissueNew.clear();
				uelem1->m_VvvmNew.clear();
				uelem1->m_VlimitsNew.clear();
				uelem1->m_VmarksNew.clear();

				m_Uelem = nullptr;

				return data_selection;
			}
			else
				return {};
		}
		else
		{
			if (m_Uelem != nullptr)
			{
				DataSelection data_selection = m_Uelem->m_DataSelection;

				if (data_selection.bmp)
				{
					m_Uelem->m_BmpOld =
							m_ImageSlices[data_selection.sliceNr].CopyBmp();
					m_Uelem->m_Mode1Old =
							m_ImageSlices[data_selection.sliceNr].ReturnMode(true);
					m_ImageSlices[data_selection.sliceNr].Copy2bmp(m_Uelem->m_BmpNew, m_Uelem->m_Mode1New);
					free(m_Uelem->m_BmpNew);
					m_Uelem->m_BmpNew = nullptr;
				}

				if (data_selection.work)
				{
					m_Uelem->m_WorkOld =
							m_ImageSlices[data_selection.sliceNr].CopyWork();
					m_Uelem->m_Mode2Old =
							m_ImageSlices[data_selection.sliceNr].ReturnMode(false);
					m_ImageSlices[data_selection.sliceNr].Copy2work(m_Uelem->m_WorkNew, m_Uelem->m_Mode2New);
					free(m_Uelem->m_WorkNew);
					m_Uelem->m_WorkNew = nullptr;
				}

				if (data_selection.tissues)
				{
					m_Uelem->m_TissueOld =
							m_ImageSlices[data_selection.sliceNr].CopyTissue(m_ActiveTissuelayer);
					m_ImageSlices[data_selection.sliceNr].Copy2tissue(m_ActiveTissuelayer, m_Uelem->m_TissueNew);
					free(m_Uelem->m_TissueNew);
					m_Uelem->m_TissueNew = nullptr;
				}

				if (data_selection.vvm)
				{
					m_Uelem->m_VvmOld.clear();
					m_Uelem->m_VvmOld =
							*(m_ImageSlices[data_selection.sliceNr].ReturnVvm());
					m_ImageSlices[data_selection.sliceNr].Copy2vvm(&m_Uelem->m_VvmNew);
					m_Uelem->m_VvmNew.clear();
				}

				if (data_selection.limits)
				{
					m_Uelem->m_LimitsOld.clear();
					m_Uelem->m_LimitsOld =
							*(m_ImageSlices[data_selection.sliceNr].ReturnLimits());
					m_ImageSlices[data_selection.sliceNr].Copy2limits(&m_Uelem->m_LimitsNew);
					m_Uelem->m_LimitsNew.clear();
				}

				if (data_selection.marks)
				{
					m_Uelem->m_MarksOld.clear();
					m_Uelem->m_MarksOld =
							*(m_ImageSlices[data_selection.sliceNr].ReturnMarks());
					m_ImageSlices[data_selection.sliceNr].Copy2marks(&m_Uelem->m_MarksNew);
					m_Uelem->m_MarksNew.clear();
				}

				SetActiveSlice(data_selection.sliceNr);

				m_Uelem = nullptr;

				return data_selection;
			}
			else
				return DataSelection();
		}
	}
	else
		return {};
}

void SlicesHandler::ClearUndo()
{
	this->m_UndoQueue.ClearUndo();
	if (m_Uelem != nullptr)
		delete (m_Uelem);

	m_Uelem = nullptr;
}

void SlicesHandler::ReverseUndosliceorder()
{
	this->m_UndoQueue.ReverseUndosliceorder(m_Nrslices);
	if (m_Uelem != nullptr)
	{
		m_Uelem->m_DataSelection.sliceNr = m_Nrslices - 1 - m_Uelem->m_DataSelection.sliceNr;
	}
}

unsigned SlicesHandler::ReturnNrundo()
{
	return this->m_UndoQueue.ReturnNrundo();
}

unsigned SlicesHandler::ReturnNrredo()
{
	return this->m_UndoQueue.ReturnNrredo();
}

bool SlicesHandler::ReturnUndo3D() const { return m_Undo3D; }

unsigned SlicesHandler::ReturnNrundosteps()
{
	return this->m_UndoQueue.ReturnNrundomax();
}

unsigned SlicesHandler::ReturnNrundoarrays()
{
	return this->m_UndoQueue.ReturnNrundoarraysmax();
}

void SlicesHandler::SetUndo3D(bool undo3D1) { m_Undo3D = undo3D1; }

void SlicesHandler::SetUndonr(unsigned nr) { this->m_UndoQueue.SetNrundo(nr); }

void SlicesHandler::SetUndoarraynr(unsigned nr)
{
	this->m_UndoQueue.SetNrundoarraysmax(nr);
}

int SlicesHandler::LoadDICOM(std::vector<const char*> lfilename)
{
	if (!lfilename.empty())
	{
		m_Endslice = m_Nrslices = (unsigned short)lfilename.size();
		m_Os.SetSizenr(m_Nrslices);
		m_ImageSlices.resize(m_Nrslices);

		// make sure files are sorted according to z-position
		if (lfilename.size() > 1)
		{
			DICOMsort(&lfilename);
		}

		m_Activeslice = 0;
		m_ActiveTissuelayer = 0;
		m_Startslice = 0;

		unsigned short a, b, c;
		float d, e, thick1;
		float disp1[3];
		float rot[3][3]; // rotation matrix
		std::vector<std::string> files(lfilename.begin(), lfilename.end());
		if (gdcmvtk_rtstruct::GetSizeUsingGDCM(files, a, b, c, d, e, thick1, disp1, rot[0], rot[1], rot[2]))
		{
			ISEG_INFO("Dicom series slice thickness: " << thick1)
			Transform tr;
			tr.SetRotation(rot[0], rot[1], rot[2]);
			tr.SetOffset(disp1);

			SetPixelsize(d, e);
			SetSlicethickness(thick1);
			SetTransform(tr);
		}

		if (lfilename.size() > 1)
		{
			double new_thick = gdcmvtk_rtstruct::GetZSPacing(files);
			if (new_thick)
			{
				ISEG_INFO("Dicom series slice z-spacing: " << new_thick)
				SetSlicethickness(new_thick);
			}
			else
			{
				ISEG_INFO("GetZSPacing failed: " << new_thick)
			}
		}

		for (int i = 0; i < lfilename.size(); i++)
		{
			float dc1[6]; // not used
			if (gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[i], a, b, c, d, e, thick1, disp1, dc1))
			{
				if (c >= 1)
				{
					size_t totsize = static_cast<size_t>(a) * b * c;
					std::vector<float> bits(totsize);

					if (bits.empty())
						return 0;

					bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[i], bits.data(), a, b, c);
					if (!canload)
					{
						return 0;
					}

					m_ImageSlices[i].LoadArray(&(bits[0]), a, b);
				}
			}
		}

		m_Endslice = m_Nrslices = (unsigned short)(lfilename.size());
		m_Os.SetSizenr(m_Nrslices);

		// Ranges
		Pair dummy;
		m_SliceRanges.resize(m_Nrslices);
		m_SliceBmpranges.resize(m_Nrslices);
		ComputeRangeMode1(&dummy);
		ComputeBmprangeMode1(&dummy);

		m_Width = m_ImageSlices[0].ReturnWidth();
		m_Height = m_ImageSlices[0].ReturnHeight();
		m_Area = m_Height * static_cast<unsigned int>(m_Width);

		NewOverlay();

		m_Loaded = true;

		return true;
	}
	return false;
}

int SlicesHandler::LoadDICOM(std::vector<const char*> lfilename, Point p, unsigned short dx, unsigned short dy)
{
	m_Activeslice = 0;
	m_ActiveTissuelayer = 0;
	m_Startslice = 0;

	if (lfilename.size() == 1)
	{
		unsigned short a, b, c;
		float d, e, thick1;
		float disp1[3];
		float dc1[6]; // direction cosines
		gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[0], a, b, c, d, e, thick1, disp1, dc1);
		if (c > 1)
		{
			unsigned long totsize = (unsigned long)(a)*b * c;
			float* bits = (float*)malloc(sizeof(float) * totsize);
			if (bits == nullptr)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0], bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			m_Endslice = m_Nrslices = (unsigned short)c;
			m_Os.SetSizenr(m_Nrslices);
			m_ImageSlices.resize(m_Nrslices);

			int j = 0;
			for (unsigned short i = 0; i < m_Nrslices; i++)
			{
				if (m_ImageSlices[i].LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b, p, dx, dy))
					j++;
			}

			free(bits);

			if (j < m_Nrslices)
				return 0;

			// Ranges
			Pair dummy;
			m_SliceRanges.resize(m_Nrslices);
			m_SliceBmpranges.resize(m_Nrslices);
			ComputeRangeMode1(&dummy);
			ComputeBmprangeMode1(&dummy);

			m_Width = m_ImageSlices[0].ReturnWidth();
			m_Height = m_ImageSlices[0].ReturnHeight();
			m_Area = m_Height * (unsigned int)m_Width;

			NewOverlay();

			m_Loaded = true;

			Transform tr(disp1, dc1);

			SetPixelsize(d, e);
			SetSlicethickness(thick1);
			SetTransform(tr);

			return 1;
		}
	}

	m_Endslice = m_Nrslices = (unsigned short)(lfilename.size());
	m_Os.SetSizenr(m_Nrslices);

	m_ImageSlices.resize(m_Nrslices);
	int j = 0;

	float thick1 = DICOMsort(&lfilename);
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		if (m_ImageSlices[i].LoadDICOM(lfilename[i], p, dx, dy))
			j++;
	}

	m_Width = m_ImageSlices[0].ReturnWidth();
	m_Height = m_ImageSlices[0].ReturnHeight();
	m_Area = m_Height * (unsigned int)m_Width;

	NewOverlay();

	if (j == m_Nrslices)
	{
		m_Loaded = true;

		DicomReader dcmr;
		if (dcmr.Opendicom(lfilename[0]))
		{
			Pair p1;
			float disp1[3];
			float dc1[6]; // direction cosines

			if (thick1 <= 0)
				thick1 = dcmr.Thickness();
			p1 = dcmr.Pixelsize();
			dcmr.Imagepos(disp1);
			disp1[0] += p.px * p1.low;
			disp1[1] += p.py * p1.high;
			dcmr.Imageorientation(dc1);
			dcmr.Closedicom();

			Transform tr(disp1, dc1);

			SetPixelsize(p1.high, p1.low);
			SetSlicethickness(thick1);
			SetTransform(tr);
		}

		// Ranges
		Pair dummy;
		m_SliceRanges.resize(m_Nrslices);
		m_SliceBmpranges.resize(m_Nrslices);
		ComputeRangeMode1(&dummy);
		ComputeBmprangeMode1(&dummy);

		return 1;
	}
	else
	{
		return 0;
	}
}

int SlicesHandler::ReloadDICOM(std::vector<const char*> lfilename)
{
	if ((m_Endslice - m_Startslice) == (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			if (m_ImageSlices[i].ReloadDICOM(lfilename[i - m_Startslice]))
				j++;
		}

		if (j == (m_Startslice - m_Endslice))
			return 1;
		else
			return 0;
	}
	else if (m_Nrslices <= (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = 0; i < m_Nrslices; i++)
		{
			if (m_ImageSlices[i].ReloadDICOM(lfilename[i]))
				j++;
		}

		if (j == m_Nrslices)
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
		gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[0], a, b, c, d, e, thick1, disp1, dc1);
		if (m_Nrslices == c)
		{
			unsigned long totsize = (unsigned long)(a)*b * c;
			float* bits = (float*)malloc(sizeof(float) * totsize);
			if (bits == nullptr)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0], bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			int j = 0;
			for (unsigned short i = m_Startslice; i < m_Endslice; i++)
			{
				if (m_ImageSlices[i]
								.LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b))
					j++;
			}

			free(bits);
			if (j < m_Endslice - m_Startslice)
				return 0;
			return 1;
		}
	}
	return 0;
}

int SlicesHandler::ReloadDICOM(std::vector<const char*> lfilename, Point p)
{
	if ((m_Endslice - m_Startslice) == (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			if (m_ImageSlices[i].ReloadDICOM(lfilename[i - m_Startslice], p))
				j++;
		}

		if (j == (m_Endslice - m_Startslice))
			return 1;
		else
			return 0;
	}
	else if (m_Nrslices <= (unsigned short)lfilename.size())
	{
		int j = 0;

		DICOMsort(&lfilename);
		for (unsigned short i = 0; i < m_Nrslices; i++)
		{
			if (m_ImageSlices[i].ReloadDICOM(lfilename[i], p))
				j++;
		}

		if (j == m_Nrslices)
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
		gdcmvtk_rtstruct::GetSizeUsingGDCM(lfilename[0], a, b, c, d, e, thick1, disp1, dc1);
		if (m_Nrslices == c)
		{
			unsigned long totsize = (unsigned long)(a)*b * c;
			float* bits = (float*)malloc(sizeof(float) * totsize);
			if (bits == nullptr)
				return 0;
			bool canload = gdcmvtk_rtstruct::GetDicomUsingGDCM(lfilename[0], bits, a, b, c);
			if (!canload)
			{
				free(bits);
				return 0;
			}

			int j = 0;
			for (unsigned short i = m_Startslice; i < m_Endslice; i++)
			{
				if (m_ImageSlices[i]
								.LoadArray(&(bits[(unsigned long)(a)*b * i]), a, b, p, m_Width, m_Height))
					j++;
			}

			free(bits);
			if (j < m_Endslice - m_Startslice)
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

	for (const auto& fname : *lfilename)
	{
		dcmread.Opendicom(fname);
		vpos.push_back(dcmread.Slicepos());
		dcmread.Closedicom();
	}

	short nrelem = (short)lfilename->size();

	for (short i = 0; i < nrelem - 1; i++)
	{
		for (short j = nrelem - 1; j > i; j--)
		{
			if (vpos[j] > vpos[j - 1])
			{
				std::swap(vpos[j], vpos[j - 1]);
				std::swap((*lfilename)[j], (*lfilename)[j - 1]);
			}
		}
	}

	if (nrelem > 1)
	{
		retval = (vpos[0] - vpos[nrelem - 1]) / (nrelem - 1);
	}

	return retval;
}

void SlicesHandler::GetDICOMseriesnr(std::vector<const char*>* vnames, std::vector<unsigned>* dicomseriesnr, std::vector<unsigned>* dicomseriesnrlist)
{
	DicomReader dcmread;

	dicomseriesnr->clear();
	for (const auto& fname : *vnames)
	{
		dcmread.Opendicom(fname);
		unsigned u = dcmread.Seriesnr();
		dcmread.Closedicom();

		dicomseriesnrlist->push_back(u);

		if (std::find(dicomseriesnr->begin(), dicomseriesnr->end(), u) == dicomseriesnr->end())
		{
			dicomseriesnr->push_back(u);
		}
	}

	std::sort(dicomseriesnr->begin(), dicomseriesnr->end());
}

void SlicesHandler::MaskSource(bool all_slices, float maskvalue)
{
	std::vector<bool> mask(TissueInfos::GetTissueCount() + 1, false);
	const auto selected_tissues = TissueInfos::GetSelectedTissues();
	for (auto label : selected_tissues)
	{
		mask.at(label) = true;
	}

	const int i_n = m_Endslice - m_Startslice;

#pragma omp parallel for
	for (int i = 0; i < i_n; ++i)
	{
		const int slice = m_Startslice + i;
		const auto tissue = TissueSlices(0)[slice];
		auto source = SourceSlices()[slice];

		for (unsigned int k = 0; k < m_Area; ++k)
		{
			if (!mask[tissue[k]])
			{
				source[k] = maskvalue;
			}
		}
	}
}

void SlicesHandler::MapTissueIndices(const std::vector<tissues_size_t>& indexMap)
{
	int const i_n = m_Nrslices;

#pragma omp parallel for
	for (int i = 0; i < i_n; i++)
	{
		m_ImageSlices[i].MapTissueIndices(indexMap);
	}
}

void SlicesHandler::RemoveTissue(tissues_size_t tissuenr)
{
	int const i_n = m_Nrslices;

#pragma omp parallel for
	for (int i = 0; i < i_n; i++)
	{
		m_ImageSlices[i].RemoveTissue(tissuenr);
	}
	TissueInfos::RemoveTissue(tissuenr);
}

void SlicesHandler::RemoveTissues(const std::set<tissues_size_t>& tissuenrs)
{
	std::vector<bool> is_selected(TissueInfos::GetTissueCount() + 1, false);
	for (auto id : tissuenrs)
	{
		is_selected.at(id) = true;
	}

	std::vector<tissues_size_t> idx_map(is_selected.size(), 0);
	for (tissues_size_t old_idx = 1, new_idx = 1; old_idx < idx_map.size(); ++old_idx)
	{
		if (!is_selected[old_idx])
		{
			idx_map[old_idx] = new_idx++;
		}
	}

	MapTissueIndices(idx_map);

	TissueInfos::RemoveTissues(tissuenrs);
}

void SlicesHandler::RemoveTissueall()
{
	for (short unsigned i = 0; i < m_Nrslices; i++)
	{
		m_ImageSlices[i].Cleartissuesall();
	}
	TissueInfos::RemoveAllTissues();
	TissueInfo tissue;
	tissue.m_Locked = false;
	tissue.m_Color = Color(1.0f, 0.0f, 0.1f);
	tissue.m_Name = "Tissue1";
	TissueInfos::AddTissue(tissue);
}

void SlicesHandler::CapTissue(tissues_size_t maxval)
{
	for (short unsigned i = 0; i < m_Nrslices; i++)
	{
		m_ImageSlices[i].CapTissue(maxval);
	}
}

void SlicesHandler::Buildmissingtissues(tissues_size_t j)
{
	tissues_size_t tissue_count = TissueInfos::GetTissueCount();
	if (j > tissue_count)
	{
		TissueInfo tissue;
		tissue.m_Locked = false;
		tissue.m_Opac = 0.5f;
		for (tissues_size_t i = tissue_count + 1; i <= j; i++)
		{
			tissue.m_Color[0] = ((i - 1) % 7) * 0.166666666f;
			tissue.m_Color[1] = (((i - 1) / 7) % 7) * 0.166666666f;
			tissue.m_Color[2] = ((i - 1) / 49) * 0.19f;
			tissue.m_Name = (boost::format("Tissue%d") % i).str();
			TissueInfos::AddTissue(tissue);
		}
	}
}

std::vector<tissues_size_t> SlicesHandler::FindUnusedTissues()
{
	std::vector<unsigned char> is_used(TissueInfos::GetTissueCount() + 1, 0);

	for (int i = 0, i_n = m_Nrslices; i < i_n; i++)
	{
		auto tissues = m_ImageSlices[i].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned k = 0; k < m_Area; ++k)
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

void SlicesHandler::GroupTissues(std::vector<tissues_size_t>& olds, std::vector<tissues_size_t>& news)
{
	int const i_n = m_Nrslices;

#pragma omp parallel for
	for (int i = 0; i < i_n; i++)
	{
		m_ImageSlices[i].GroupTissues(m_ActiveTissuelayer, olds, news);
	}
}
void SlicesHandler::SetModeall(unsigned char mode, bool bmporwork)
{
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		m_ImageSlices[i].SetMode(mode, bmporwork);
}

bool SlicesHandler::PrintTissuemat(const char* filename)
{
	std::vector<tissues_size_t*> matrix(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
		matrix[i] = m_ImageSlices[i].ReturnTissues(m_ActiveTissuelayer);
	bool ok = matexport::print_matslices(filename, matrix.data(), int(m_Width), int(m_Height), int(m_Nrslices), "iSeg tissue data v1.0", 21, "tissuedistrib", 13);
	return ok;
}

bool SlicesHandler::PrintBmpmat(const char* filename)
{
	std::vector<float*> matrix(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
		matrix[i] = m_ImageSlices[i].ReturnBmp();
	bool ok = matexport::print_matslices(filename, matrix.data(), int(m_Width), int(m_Height), int(m_Nrslices), "iSeg source data v1.0", 21, "sourcedistrib", 13);
	return ok;
}

bool SlicesHandler::PrintWorkmat(const char* filename)
{
	std::vector<float*> matrix(m_Nrslices);
	for (unsigned short i = 0; i < m_Nrslices; i++)
		matrix[i] = m_ImageSlices[i].ReturnWork();
	bool ok = matexport::print_matslices(filename, matrix.data(), int(m_Width), int(m_Height), int(m_Nrslices), "iSeg target data v1.0", 21, "targetdistrib", 13);
	return ok;
}

bool SlicesHandler::PrintAtlas(const char* filename)
{
	QFile file(filename);
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);

	// Write a header with a "magic number" and a version
	out << (quint32)0xD0C0B0A0;
	out << (qint32)iseg::CombineTissuesVersion(1, 1);

	out.setVersion(QDataStream::Qt_4_0);

	// Write the data
	out << (quint32)m_Width << (quint32)m_Height << (quint32)m_Nrslices;
	out << (float)m_Dx << (float)m_Dy << (float)m_Thickness;
	tissues_size_t tissue_count = TissueInfos::GetTissueCount();
	out << (quint32)tissue_count;
	TissueInfo* tissue_info;
	for (tissues_size_t tissuenr = 1; tissuenr <= tissue_count; tissuenr++)
	{
		tissue_info = TissueInfos::GetTissueInfo(tissuenr);
		out << ToQ(tissue_info->m_Name) << tissue_info->m_Color[0] << tissue_info->m_Color[1] << tissue_info->m_Color[2];
	}
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		out.writeRawData((char*)m_ImageSlices[i].ReturnBmp(), (int)m_Area * sizeof(float));
		out.writeRawData((char*)m_ImageSlices[i].ReturnTissues(m_ActiveTissuelayer), (int)m_Area * sizeof(tissues_size_t));
	}

	return true;
}

bool SlicesHandler::PrintAmascii(const char* filename)
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
		streamname << "define Lattice " << m_Width << " " << m_Height << " "
							 << m_Nrslices << endl
							 << endl;
		streamname << "Parameters {" << endl;
		streamname << "    Materials {" << endl;
		streamname << "        Exterior {" << endl;
		streamname << "            Id 1" << endl;
		streamname << "        }" << endl;
		tissues_size_t tissue_count = TissueInfos::GetTissueCount();
		TissueInfo* tissue_info;
		for (tissues_size_t tc = 0; tc < tissue_count; tc++)
		{
			tissue_info = TissueInfos::GetTissueInfo(tc + 1);
			QString name_cpy = ToQ(tissue_info->m_Name);
			name_cpy = name_cpy.replace("�", "ae");
			name_cpy = name_cpy.replace("�", "Ae");
			name_cpy = name_cpy.replace("�", "oe");
			name_cpy = name_cpy.replace("�", "Oe");
			name_cpy = name_cpy.replace("�", "ue");
			name_cpy = name_cpy.replace("�", "Ue");
			streamname << "        " << name_cpy.ascii() << " {" << endl;
			streamname << "            Color " << tissue_info->m_Color[0] << " "
								 << tissue_info->m_Color[1] << " " << tissue_info->m_Color[2]
								 << "," << endl;
			streamname << "            Id " << tc + 2 << endl;
			streamname << "        }" << endl;
		}
		streamname << "    }" << endl;
		if (tissue_count <= 255)
		{
			streamname << "    Content \"" << m_Width << "x" << m_Height << "x"
								 << m_Nrslices << " byte, uniform coordinates\"," << endl;
		}
		else
		{
			streamname << "    Content \"" << m_Width << "x" << m_Height << "x"
								 << m_Nrslices << " ushort, uniform coordinates\"," << endl;
		}
		streamname << "    BoundingBox 0 " << m_Width * m_Dx << " 0 " << m_Height * m_Dy
							 << " 0 " << m_Nrslices * m_Thickness << "," << endl;
		streamname << "    CoordType \"uniform\"" << endl;
		streamname << "}" << endl
							 << endl;
		if (tissue_count <= 255)
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
		for (unsigned short i = 0; i < m_Nrslices; i++)
			ok &= m_ImageSlices[i].PrintAmasciiSlice(m_ActiveTissuelayer, streamname);
		streamname << endl;

		streamname.close();
		return ok;
	}
	else
		return false;
}

/// This function returns a pointer to a vtkImageData
/// The user is responsible for deleting data ...
vtkImageData* SlicesHandler::MakeVtktissueimage()
{
	const char* tissue_name_array_name = "TissueNames"; // don't modify this
	const char* tissue_color_array_name = "Colors";			// don't modify this

	// copy label field
	vtkImageData* label_field = vtkImageData::New();
	label_field->SetExtent(0, (int)Width() - 1, 0, (int)Height() - 1, 0, (int)(m_Endslice - m_Startslice) - 1);
	Pair ps = GetPixelsize();
	float offset[3];
	GetDisplacement(offset);
	label_field->SetSpacing(ps.high, ps.low, m_Thickness);
	label_field->SetOrigin(offset[0], offset[1], offset[2] + m_Thickness * m_Startslice);
	if (TissueInfos::GetTissueCount() <= 255)
	{
		label_field->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
		unsigned char* field =
				(unsigned char*)label_field->GetScalarPointer(0, 0, 0);
		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			Copyfromtissue(i, &(field[i * (unsigned long long)ReturnArea()]));
		}
	}
	else if (sizeof(tissues_size_t) == sizeof(unsigned short))
	{
		label_field->AllocateScalars(VTK_UNSIGNED_SHORT, 1);
		tissues_size_t* field = (tissues_size_t*)label_field->GetScalarPointer(0, 0, 0);
		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			Copyfromtissue(i, &(field[i * (unsigned long long)ReturnArea()]));
		}
	}
	else
	{
		ISEG_ERROR_MSG("SlicesHandler::make_vtktissueimage: Error, tissues_size_t not implemented!");
		label_field->Delete();
		return nullptr;
	}

	// copy tissue names and colors
	tissues_size_t num_tissues = TissueInfos::GetTissueCount();
	auto names_array = vtkSmartPointer<vtkStringArray>::New();
	names_array->SetNumberOfTuples(num_tissues + 1);
	names_array->SetName(tissue_name_array_name);
	label_field->GetFieldData()->AddArray(names_array);

	auto color_array = vtkSmartPointer<vtkFloatArray>::New();
	color_array->SetNumberOfComponents(3);
	color_array->SetNumberOfTuples(num_tissues + 1);
	color_array->SetName(tissue_color_array_name);
	label_field->GetFieldData()->AddArray(color_array);
	for (tissues_size_t i = 1; i < num_tissues; i++)
	{
		int error_counter = 0;
		check_equal(TissueInfos::GetTissueType(TissueInfos::GetTissueName(i)), i);
		names_array->SetValue(i, TissueInfos::GetTissueName(i).c_str());
		auto color = TissueInfos::GetTissueColor(i);
		ISEG_INFO(TissueInfos::GetTissueName(i).c_str() << " " << color[0] << "," << color[1] << "," << color[2]);
		color_array->SetTuple(i, color.v.data());
	}

	return label_field;
}

bool SlicesHandler::ExportTissue(const char* filename, bool binary) const
{
	auto slices = const_cast<SlicesHandler*>(this)->TissueSlices(m_ActiveTissuelayer);
	return ImageWriter(binary).WriteVolume(filename, slices, ImageWriter::kActiveSlices, this);
}

bool SlicesHandler::ExportBmp(const char* filename, bool binary) const
{
	auto slices = const_cast<SlicesHandler*>(this)->SourceSlices();
	return ImageWriter(binary).WriteVolume(filename, slices, ImageWriter::kActiveSlices, this);
}

bool SlicesHandler::ExportWork(const char* filename, bool binary) const
{
	auto slices = const_cast<SlicesHandler*>(this)->TargetSlices();
	return ImageWriter(binary).WriteVolume(filename, slices, ImageWriter::kActiveSlices, this);
}

bool SlicesHandler::PrintXmlregionextent(const char* filename, bool onlyactiveslices, const char* projname)
{
	float offset[3];
	GetDisplacement(offset);

	std::ofstream streamname;
	streamname.open(filename, std::ios_base::binary);
	if (streamname.good())
	{
		bool ok = true;
		streamname << "<?xml version=\"1.0\"?>" << endl;
		streamname << "<IndexFile type=\"Extent\" version=\"0.1\">" << endl;
		unsigned short extent[3][2];
		tissues_size_t tissue_count = TissueInfos::GetTissueCount();
		TissueInfo* tissue_info;
		for (tissues_size_t tissuenr = 1; tissuenr <= tissue_count; tissuenr++)
		{
			if (GetExtent(tissuenr, onlyactiveslices, extent))
			{
				tissue_info = TissueInfos::GetTissueInfo(tissuenr);
				streamname << "\t<label id=\"" << (int)tissuenr << "\" name=\""
									 << tissue_info->m_Name.c_str() << "\" color=\""
									 << tissue_info->m_Color[0] << " "
									 << tissue_info->m_Color[1] << " "
									 << tissue_info->m_Color[2] << "\">" << endl;
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
									 << extent[0][0] * m_Dx + offset[0] << " "
									 << extent[0][1] * m_Dx + offset[0] << " "
									 << extent[1][0] * m_Dy + offset[1] << " "
									 << extent[1][1] * m_Dy + offset[1] << " "
									 << extent[2][0] * m_Thickness + offset[2] << " "
									 << extent[2][1] * m_Thickness + offset[2] << "\">"
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

bool SlicesHandler::PrintTissueindex(const char* filename, bool onlyactiveslices, const char* projname)
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

bool SlicesHandler::GetExtent(tissues_size_t tissuenr, bool onlyactiveslices, unsigned short extent[3][2])
{
	bool found = false;
	unsigned short extent1[2][2];
	unsigned short startslice1, endslice1;
	startslice1 = 0;
	endslice1 = m_Nrslices;
	if (onlyactiveslices)
	{
		startslice1 = m_Startslice;
		endslice1 = m_Endslice;
	}
	for (unsigned short i = startslice1; i < endslice1; i++)
	{
		if (!found)
		{
			found = m_ImageSlices[i].GetExtent(m_ActiveTissuelayer, tissuenr, extent1);
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
			if (m_ImageSlices[i].GetExtent(m_ActiveTissuelayer, tissuenr, extent1))
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

void SlicesHandler::AddSkin3D(int ix, int iy, int iz, float setto)
{
	// ix,iy,iz are in pixels

	//Create skin in each slice as same way is done in the 2D AddSkin process
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		float* work;
		work = m_ImageSlices[z].ReturnWork();
		unsigned i = 0, pos, y, x;

		//Create a binary vector noTissue/Tissue
		std::vector<int> s;
		for (int j = 0; j < m_Height; j++)
		{
			for (int k = 0; k < m_Width; k++)
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
		bool convert_skin = true;
		for (i = 1; i < ix + 1; i++)
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

		//go over the vector and set the skin pixel at the source pointer
		i = 0;
		for (int j = 0; j < m_Height; j++)
		{
			for (int k = 0; k < m_Width; k++)
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
	int check_slice_distance = iz;

	for (unsigned short z = m_Startslice; z < m_Endslice - check_slice_distance; z++)
	{
		float* work1;
		float* work2;
		work1 = m_ImageSlices[z].ReturnWork();
		work2 = m_ImageSlices[z + check_slice_distance].ReturnWork();

		unsigned i = 0;
		for (int j = 0; j < m_Height; j++)
		{
			for (int k = 0; k < m_Width; k++)
			{
				if (work1[i] != 0 && work1[i] != setto)
					if (work2[i] == 0)
						work1[i] = setto;
				i++;
			}
		}
	}

	for (unsigned short z = m_Endslice - 1; z > m_Startslice + check_slice_distance;
			 z--)
	{
		float* work1;
		float* work2;
		work1 = m_ImageSlices[z].ReturnWork();
		work2 = m_ImageSlices[z - check_slice_distance].ReturnWork();

		unsigned i = 0;
		for (int j = 0; j < m_Height; j++)
		{
			for (int k = 0; k < m_Width; k++)
			{
				if (work1[i] != 0 && work1[i] != setto)
					if (work2[i] == 0)
						work1[i] = setto;
				i++;
			}
		}
	}
}

void SlicesHandler::AddSkin3DOutside(int ix, int iy, int iz, float setto)
{
	float set_to = (float)123E10;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		m_ImageSlices[z].FloodExterior(set_to);
	}

	std::vector<Posit> s;
	Posit p1;

	p1.m_Pz = m_Activeslice;

	float* work1;
	float* work2;
	for (unsigned short z = m_Startslice; z + 1 < m_Endslice; z++)
	{
		work1 = m_ImageSlices[z].ReturnWork();
		work2 = m_ImageSlices[z + 1].ReturnWork();
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = m_Endslice - 1; z > m_Startslice; z--)
	{
		work1 = m_ImageSlices[z].ReturnWork();
		work2 = m_ImageSlices[z - 1].ReturnWork();
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}

	Posit i, j;
	float* work;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		work = m_ImageSlices[i.m_Pz].ReturnWork();
		if (i.m_Pxy % m_Width != 0 && work[i.m_Pxy - 1] == 0)
		{
			work[i.m_Pxy - 1] = set_to;
			j.m_Pxy = i.m_Pxy - 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if ((i.m_Pxy + 1) % m_Width != 0 && work[i.m_Pxy + 1] == 0)
		{
			work[i.m_Pxy + 1] = set_to;
			j.m_Pxy = i.m_Pxy + 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy >= m_Width && work[i.m_Pxy - m_Width] == 0)
		{
			work[i.m_Pxy - m_Width] = set_to;
			j.m_Pxy = i.m_Pxy - m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy < m_Area - m_Width && work[i.m_Pxy + m_Width] == 0)
		{
			work[i.m_Pxy + m_Width] = set_to;
			j.m_Pxy = i.m_Pxy + m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pz > m_Startslice)
		{
			work = m_ImageSlices[i.m_Pz - 1].ReturnWork();
			if (work[i.m_Pxy] == 0)
			{
				work[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz - 1;
				s.push_back(j);
			}
		}
		if (i.m_Pz + 1 < m_Endslice)
		{
			work = m_ImageSlices[i.m_Pz + 1].ReturnWork();
			if (work[i.m_Pxy] == 0)
			{
				work[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz + 1;
				s.push_back(j);
			}
		}
	}

	unsigned short x, y;
	unsigned long pos;
	unsigned short i1;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		work = m_ImageSlices[z].ReturnWork();

		for (y = 0; y < m_Height; y++)
		{
			pos = y * m_Width;
			i1 = 0;
			while (pos < (unsigned long)(y + 1) * m_Width)
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

		for (y = 0; y < m_Height; y++)
		{
			pos = (y + 1) * m_Width - 1;
			i1 = 0;
			while (pos > (unsigned long)y * m_Width)
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

		for (x = 0; x < m_Width; x++)
		{
			pos = x;
			i1 = 0;
			while (pos < m_Area)
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
				pos += m_Width;
			}
		}

		for (x = 0; x < m_Width; x++)
		{
			pos = m_Area + x - m_Width;
			i1 = 0;
			while (pos > m_Width)
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
				pos -= m_Width;
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

	unsigned short* counter = (unsigned short*)malloc(sizeof(unsigned short) * m_Area);
	for (unsigned i1 = 0; i1 < m_Area; i1++)
		counter[i1] = 0;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		work = m_ImageSlices[z].ReturnWork();
		for (pos = 0; pos < m_Area; pos++)
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
	for (unsigned i1 = 0; i1 < m_Area; i1++)
		counter[i1] = 0;
	for (unsigned short z = m_Endslice - 1; z > m_Startslice; z--)
	{
		work = m_ImageSlices[z].ReturnWork();
		for (pos = 0; pos < m_Area; pos++)
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
	work = m_ImageSlices[m_Startslice].ReturnWork();
	for (pos = 0; pos < m_Area; pos++)
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

	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		work = m_ImageSlices[z].ReturnWork();
		for (unsigned i1 = 0; i1 < m_Area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}
}

void SlicesHandler::AddSkin3DOutside2(int ix, int iy, int iz, float setto)
{
	float set_to = (float)123E10;
	float set_to2 = (float)321E10;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		m_ImageSlices[z].FloodExterior(set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<Posit> s;
	Posit p1;

	//	p1.pxy=position;
	p1.m_Pz = m_Activeslice;

	float* work1;
	float* work2;
	for (unsigned short z = m_Startslice; z + 1 < m_Endslice; z++)
	{
		work1 = m_ImageSlices[z].ReturnWork();
		work2 = m_ImageSlices[z + 1].ReturnWork();
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = m_Endslice - 1; z > m_Startslice; z--)
	{
		work1 = m_ImageSlices[z].ReturnWork();
		work2 = m_ImageSlices[z - 1].ReturnWork();
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}

	Posit i, j;
	float* work;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		work = m_ImageSlices[i.m_Pz].ReturnWork();
		if (i.m_Pxy % m_Width != 0 && work[i.m_Pxy - 1] == 0)
		{
			work[i.m_Pxy - 1] = set_to;
			j.m_Pxy = i.m_Pxy - 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if ((i.m_Pxy + 1) % m_Width != 0 && work[i.m_Pxy + 1] == 0)
		{
			work[i.m_Pxy + 1] = set_to;
			j.m_Pxy = i.m_Pxy + 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy >= m_Width && work[i.m_Pxy - m_Width] == 0)
		{
			work[i.m_Pxy - m_Width] = set_to;
			j.m_Pxy = i.m_Pxy - m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy < m_Area - m_Width && work[i.m_Pxy + m_Width] == 0)
		{
			work[i.m_Pxy + m_Width] = set_to;
			j.m_Pxy = i.m_Pxy + m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pz > m_Startslice)
		{
			work = m_ImageSlices[i.m_Pz - 1].ReturnWork();
			if (work[i.m_Pxy] == 0)
			{
				work[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz - 1;
				s.push_back(j);
			}
		}
		if (i.m_Pz + 1 < m_Endslice)
		{
			work = m_ImageSlices[i.m_Pz + 1].ReturnWork();
			if (work[i.m_Pxy] == 0)
			{
				work[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz + 1;
				s.push_back(j);
			}
		}
	}

	unsigned int totcount = (ix + 1) * (iy + 1) * (iz + 1);
	unsigned int subx = (iy + 1) * (iz + 1);
	unsigned int suby = (ix + 1) * (iz + 1);
	unsigned int subz = (iy + 1) * (ix + 1);

	Treap<Posit, unsigned int> treap1;
	if (iz > 0)
	{
		for (unsigned short z = m_Startslice; z + 1 < m_Endslice; z++)
		{
			work1 = m_ImageSlices[z].ReturnWork();
			work2 = m_ImageSlices[z + 1].ReturnWork();
			for (unsigned long i = 0; i < m_Area; i++)
			{
				if (work1[i] == set_to)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.m_Pxy = i;
						p1.m_Pz = z;
						Treap<Posit, unsigned int>::Node* n = nullptr;
						treap1.Insert(n, p1, 1, subz);
						work1[i] = set_to2;
					}
				}
				else if (work1[i] == set_to2)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.m_Pxy = i;
						p1.m_Pz = z;
						Treap<Posit, unsigned int>::Node* n1 = treap1.Lookup(p1);
						if (n1->GetPriority() > subz)
							treap1.UpdatePriority(n1, subz);
					}
				}
				else
				{
					if (work2[i] == set_to2)
					{
						p1.m_Pxy = i;
						p1.m_Pz = z + 1;
						Treap<Posit, unsigned int>::Node* n1 = treap1.Lookup(p1);
						if (n1->GetPriority() > subz)
							treap1.UpdatePriority(n1, subz);
					}
					else if (work2[i] == set_to)
					{
						p1.m_Pxy = i;
						p1.m_Pz = z + 1;
						Treap<Posit, unsigned int>::Node* n = nullptr;
						treap1.Insert(n, p1, 1, subz);
						work2[i] = set_to2;
					}
				}
			}
		}
	}

	if (ix > 0)
	{
		for (unsigned short z = m_Startslice; z < m_Endslice; z++)
		{
			p1.m_Pz = z;
			work1 = m_ImageSlices[z].ReturnWork();
			unsigned long i = 0;
			for (unsigned short y = 0; y < m_Height; y++)
			{
				for (unsigned short x = 0; x + 1 < m_Width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + 1] != set_to) &&
								(work1[i + 1] != set_to2))
						{
							p1.m_Pxy = i;
							Treap<Posit, unsigned int>::Node* n = nullptr;
							treap1.Insert(n, p1, 1, subx);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + 1] != set_to) &&
								(work1[i + 1] != set_to2))
						{
							p1.m_Pxy = i;
							Treap<Posit, unsigned int>::Node* n1 =
									treap1.Lookup(p1);
							if (n1->GetPriority() > subx)
								treap1.UpdatePriority(n1, subx);
						}
					}
					else
					{
						if (work1[i + 1] == set_to2)
						{
							p1.m_Pxy = i + 1;
							Treap<Posit, unsigned int>::Node* n1 =
									treap1.Lookup(p1);
							if (n1->GetPriority() > subx)
								treap1.UpdatePriority(n1, subx);
						}
						else if (work1[i + 1] == set_to)
						{
							p1.m_Pxy = i + 1;
							Treap<Posit, unsigned int>::Node* n = nullptr;
							treap1.Insert(n, p1, 1, subx);
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
		for (unsigned short z = m_Startslice; z < m_Endslice; z++)
		{
			p1.m_Pz = z;
			work1 = m_ImageSlices[z].ReturnWork();
			unsigned long i = 0;
			for (unsigned short y = 0; y + 1 < m_Height; y++)
			{
				for (unsigned short x = 0; x < m_Width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + m_Width] != set_to) &&
								(work1[i + m_Width] != set_to2))
						{
							p1.m_Pxy = i;
							Treap<Posit, unsigned int>::Node* n = nullptr;
							treap1.Insert(n, p1, 1, suby);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + m_Width] != set_to) &&
								(work1[i + m_Width] != set_to2))
						{
							p1.m_Pxy = i;
							Treap<Posit, unsigned int>::Node* n1 =
									treap1.Lookup(p1);
							if (n1->GetPriority() > suby)
								treap1.UpdatePriority(n1, suby);
						}
					}
					else
					{
						if (work1[i + m_Width] == set_to2)
						{
							p1.m_Pxy = i + m_Width;
							Treap<Posit, unsigned int>::Node* n1 =
									treap1.Lookup(p1);
							if (n1->GetPriority() > suby)
								treap1.UpdatePriority(n1, suby);
						}
						else if (work1[i + m_Width] == set_to)
						{
							p1.m_Pxy = i + m_Width;
							Treap<Posit, unsigned int>::Node* n = nullptr;
							treap1.Insert(n, p1, 1, suby);
							work1[i + m_Width] = set_to2;
						}
					}
					i++;
				}
			}
		}
	}

	Treap<Posit, unsigned int>::Node* n1;
	Posit p2;
	unsigned int prior;
	while ((n1 = treap1.GetTop()) != nullptr)
	{
		p1 = n1->GetKey();
		prior = n1->GetPriority();
		work1 = m_ImageSlices[p1.m_Pz].ReturnWork();
		work1[p1.m_Pxy] = setto;
		if (p1.m_Pxy % m_Width != 0)
		{
			if (work1[p1.m_Pxy - 1] == set_to)
			{
				if (prior + 2 * subx <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy - 1;
					p2.m_Pz = p1.m_Pz;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + subx);
					work1[p1.m_Pxy - 1] = set_to2;
				}
			}
			else if (work1[p1.m_Pxy - 1] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy - 1;
				p2.m_Pz = p1.m_Pz;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + subx)
					treap1.UpdatePriority(n2, prior + subx);
			}
		}
		if ((p1.m_Pxy + 1) % m_Width != 0)
		{
			if (work1[p1.m_Pxy + 1] == set_to)
			{
				if (prior + 2 * subx <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy + 1;
					p2.m_Pz = p1.m_Pz;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + subx);
					work1[p1.m_Pxy + 1] = set_to2;
				}
			}
			else if (work1[p1.m_Pxy + 1] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy + 1;
				p2.m_Pz = p1.m_Pz;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + subx)
					treap1.UpdatePriority(n2, prior + subx);
			}
		}
		if (p1.m_Pxy >= m_Width)
		{
			if (work1[p1.m_Pxy - m_Width] == set_to)
			{
				if (prior + 2 * suby <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy - m_Width;
					p2.m_Pz = p1.m_Pz;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + suby);
					work1[p1.m_Pxy - m_Width] = set_to2;
				}
			}
			else if (work1[p1.m_Pxy - m_Width] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy - m_Width;
				p2.m_Pz = p1.m_Pz;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + suby)
					treap1.UpdatePriority(n2, prior + suby);
			}
		}
		if (p1.m_Pxy < m_Area - m_Width)
		{
			if (work1[p1.m_Pxy + m_Width] == set_to)
			{
				if (prior + 2 * suby <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy + m_Width;
					p2.m_Pz = p1.m_Pz;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + suby);
					work1[p1.m_Pxy + m_Width] = set_to2;
				}
			}
			else if (work1[p1.m_Pxy + m_Width] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy + m_Width;
				p2.m_Pz = p1.m_Pz;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + suby)
					treap1.UpdatePriority(n2, prior + suby);
			}
		}
		if (p1.m_Pz > m_Startslice)
		{
			work2 = m_ImageSlices[p1.m_Pz - 1].ReturnWork();
			if (work2[p1.m_Pxy] == set_to)
			{
				if (prior + 2 * subz <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy;
					p2.m_Pz = p1.m_Pz - 1;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + subz);
					work2[p1.m_Pxy] = set_to2;
				}
			}
			else if (work2[p1.m_Pxy] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy;
				p2.m_Pz = p1.m_Pz - 1;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + subz)
					treap1.UpdatePriority(n2, prior + subz);
			}
		}
		if (p1.m_Pz + 1 < m_Endslice)
		{
			work2 = m_ImageSlices[p1.m_Pz + 1].ReturnWork();
			if (work2[p1.m_Pxy] == set_to)
			{
				if (prior + 2 * subz <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy;
					p2.m_Pz = p1.m_Pz + 1;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + subz);
					work2[p1.m_Pxy] = set_to2;
				}
			}
			else if (work2[p1.m_Pxy] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy;
				p2.m_Pz = p1.m_Pz + 1;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + subz)
					treap1.UpdatePriority(n2, prior + subz);
			}
		}

		delete treap1.Remove(n1);
	}

	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		work = m_ImageSlices[z].ReturnWork();
		for (unsigned i1 = 0; i1 < m_Area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}
}

void SlicesHandler::AddSkintissue3DOutside2(int ix, int iy, int iz, tissues_size_t f)
{
	tissues_size_t set_to = TISSUES_SIZE_MAX;
	tissues_size_t set_to2 = TISSUES_SIZE_MAX - 1;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		m_ImageSlices[z].FloodExteriortissue(m_ActiveTissuelayer, set_to);
	}

	std::vector<Posit> s;
	Posit p1;

	p1.m_Pz = m_Activeslice;

	tissues_size_t* work1;
	tissues_size_t* work2;
	for (unsigned short z = m_Startslice; z + 1 < m_Endslice; z++)
	{
		work1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		work2 = m_ImageSlices[z + 1].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = m_Endslice - 1; z > m_Startslice; z--)
	{
		work1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		work2 = m_ImageSlices[z - 1].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (work1[i] == 0 && work2[i] == set_to)
			{
				work1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}

	Posit i, j;
	tissues_size_t* work;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		work = m_ImageSlices[i.m_Pz].ReturnTissues(m_ActiveTissuelayer);
		if (i.m_Pxy % m_Width != 0 && work[i.m_Pxy - 1] == 0)
		{
			work[i.m_Pxy - 1] = set_to;
			j.m_Pxy = i.m_Pxy - 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if ((i.m_Pxy + 1) % m_Width != 0 && work[i.m_Pxy + 1] == 0)
		{
			work[i.m_Pxy + 1] = set_to;
			j.m_Pxy = i.m_Pxy + 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy >= m_Width && work[i.m_Pxy - m_Width] == 0)
		{
			work[i.m_Pxy - m_Width] = set_to;
			j.m_Pxy = i.m_Pxy - m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy < m_Area - m_Width && work[i.m_Pxy + m_Width] == 0)
		{
			work[i.m_Pxy + m_Width] = set_to;
			j.m_Pxy = i.m_Pxy + m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pz > m_Startslice)
		{
			work = m_ImageSlices[i.m_Pz - 1].ReturnTissues(m_ActiveTissuelayer);
			if (work[i.m_Pxy] == 0)
			{
				work[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz - 1;
				s.push_back(j);
			}
		}
		if (i.m_Pz + 1 < m_Endslice)
		{
			work = m_ImageSlices[i.m_Pz + 1].ReturnTissues(m_ActiveTissuelayer);
			if (work[i.m_Pxy] == 0)
			{
				work[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz + 1;
				s.push_back(j);
			}
		}
	}

	unsigned int totcount = (ix + 1) * (iy + 1) * (iz + 1);
	unsigned int subx = (iy + 1) * (iz + 1);
	unsigned int suby = (ix + 1) * (iz + 1);
	unsigned int subz = (iy + 1) * (ix + 1);

	Treap<Posit, unsigned int> treap1;
	if (iz > 0)
	{
		for (unsigned short z = m_Startslice; z + 1 < m_Endslice; z++)
		{
			work1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
			work2 = m_ImageSlices[z + 1].ReturnTissues(m_ActiveTissuelayer);
			for (unsigned long i = 0; i < m_Area; i++)
			{
				if (work1[i] == set_to)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.m_Pxy = i;
						p1.m_Pz = z;
						Treap<Posit, unsigned int>::Node* n = nullptr;
						treap1.Insert(n, p1, 1, subz);
						work1[i] = set_to2;
					}
				}
				else if (work1[i] == set_to2)
				{
					if ((work2[i] != set_to) && (work2[i] != set_to2))
					{
						p1.m_Pxy = i;
						p1.m_Pz = z;
						Treap<Posit, unsigned int>::Node* n1 =
								treap1.Lookup(p1);
						if (n1->GetPriority() > subz)
							treap1.UpdatePriority(n1, subz);
					}
				}
				else
				{
					if (work2[i] == set_to2)
					{
						p1.m_Pxy = i;
						p1.m_Pz = z + 1;
						Treap<Posit, unsigned int>::Node* n1 =
								treap1.Lookup(p1);
						if (n1->GetPriority() > subz)
							treap1.UpdatePriority(n1, subz);
					}
					else if (work2[i] == set_to)
					{
						p1.m_Pxy = i;
						p1.m_Pz = z + 1;
						Treap<Posit, unsigned int>::Node* n = nullptr;
						treap1.Insert(n, p1, 1, subz);
						work2[i] = set_to2;
					}
				}
			}
		}
	}

	if (ix > 0)
	{
		for (unsigned short z = m_Startslice; z < m_Endslice; z++)
		{
			p1.m_Pz = z;
			work1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
			unsigned long i = 0;
			for (unsigned short y = 0; y < m_Height; y++)
			{
				for (unsigned short x = 0; x + 1 < m_Width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + 1] != set_to) &&
								(work1[i + 1] != set_to2))
						{
							p1.m_Pxy = i;
							Treap<Posit, unsigned int>::Node* n = nullptr;
							treap1.Insert(n, p1, 1, subx);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + 1] != set_to) &&
								(work1[i + 1] != set_to2))
						{
							p1.m_Pxy = i;
							Treap<Posit, unsigned int>::Node* n1 =
									treap1.Lookup(p1);
							if (n1->GetPriority() > subx)
								treap1.UpdatePriority(n1, subx);
						}
					}
					else
					{
						if (work1[i + 1] == set_to2)
						{
							p1.m_Pxy = i + 1;
							Treap<Posit, unsigned int>::Node* n1 =
									treap1.Lookup(p1);
							if (n1->GetPriority() > subx)
								treap1.UpdatePriority(n1, subx);
						}
						else if (work1[i + 1] == set_to)
						{
							p1.m_Pxy = i + 1;
							Treap<Posit, unsigned int>::Node* n = nullptr;
							treap1.Insert(n, p1, 1, subx);
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
		for (unsigned short z = m_Startslice; z < m_Endslice; z++)
		{
			p1.m_Pz = z;
			work1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
			unsigned long i = 0;
			for (unsigned short y = 0; y + 1 < m_Height; y++)
			{
				for (unsigned short x = 0; x < m_Width; x++)
				{
					if (work1[i] == set_to)
					{
						if ((work1[i + m_Width] != set_to) &&
								(work1[i + m_Width] != set_to2))
						{
							p1.m_Pxy = i;
							Treap<Posit, unsigned int>::Node* n = nullptr;
							treap1.Insert(n, p1, 1, suby);
							work1[i] = set_to2;
						}
					}
					else if (work1[i] == set_to2)
					{
						if ((work1[i + m_Width] != set_to) &&
								(work1[i + m_Width] != set_to2))
						{
							p1.m_Pxy = i;
							Treap<Posit, unsigned int>::Node* n1 =
									treap1.Lookup(p1);
							if (n1->GetPriority() > suby)
								treap1.UpdatePriority(n1, suby);
						}
					}
					else
					{
						if (work1[i + m_Width] == set_to2)
						{
							p1.m_Pxy = i + m_Width;
							Treap<Posit, unsigned int>::Node* n1 =
									treap1.Lookup(p1);
							if (n1->GetPriority() > suby)
								treap1.UpdatePriority(n1, suby);
						}
						else if (work1[i + m_Width] == set_to)
						{
							p1.m_Pxy = i + m_Width;
							Treap<Posit, unsigned int>::Node* n = nullptr;
							treap1.Insert(n, p1, 1, suby);
							work1[i + m_Width] = set_to2;
						}
					}
					i++;
				}
			}
		}
	}

	Treap<Posit, unsigned int>::Node* n1;
	Posit p2;
	unsigned int prior;
	while ((n1 = treap1.GetTop()) != nullptr)
	{
		p1 = n1->GetKey();
		prior = n1->GetPriority();
		work1 = m_ImageSlices[p1.m_Pz].ReturnTissues(m_ActiveTissuelayer);
		work1[p1.m_Pxy] = f;
		if (p1.m_Pxy % m_Width != 0)
		{
			if (work1[p1.m_Pxy - 1] == set_to)
			{
				if (prior + subx <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy - 1;
					p2.m_Pz = p1.m_Pz;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + subx);
					work1[p1.m_Pxy - 1] = set_to2;
				}
			}
			else if (work1[p1.m_Pxy - 1] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy - 1;
				p2.m_Pz = p1.m_Pz;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + subx)
					treap1.UpdatePriority(n2, prior + subx);
			}
		}
		if ((p1.m_Pxy + 1) % m_Width != 0)
		{
			if (work1[p1.m_Pxy + 1] == set_to)
			{
				if (prior + subx <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy + 1;
					p2.m_Pz = p1.m_Pz;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + subx);
					work1[p1.m_Pxy + 1] = set_to2;
				}
			}
			else if (work1[p1.m_Pxy + 1] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy + 1;
				p2.m_Pz = p1.m_Pz;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + subx)
					treap1.UpdatePriority(n2, prior + subx);
			}
		}
		if (p1.m_Pxy >= m_Width)
		{
			if (work1[p1.m_Pxy - m_Width] == set_to)
			{
				if (prior + suby <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy - m_Width;
					p2.m_Pz = p1.m_Pz;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + suby);
					work1[p1.m_Pxy - m_Width] = set_to2;
				}
			}
			else if (work1[p1.m_Pxy - m_Width] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy - m_Width;
				p2.m_Pz = p1.m_Pz;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + suby)
					treap1.UpdatePriority(n2, prior + suby);
			}
		}
		if (p1.m_Pxy < m_Area - m_Width)
		{
			if (work1[p1.m_Pxy + m_Width] == set_to)
			{
				if (prior + suby <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy + m_Width;
					p2.m_Pz = p1.m_Pz;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + suby);
					work1[p1.m_Pxy + m_Width] = set_to2;
				}
			}
			else if (work1[p1.m_Pxy + m_Width] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy + m_Width;
				p2.m_Pz = p1.m_Pz;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + suby)
					treap1.UpdatePriority(n2, prior + suby);
			}
		}
		if (p1.m_Pz > m_Startslice)
		{
			work2 = m_ImageSlices[p1.m_Pz - 1].ReturnTissues(m_ActiveTissuelayer);
			if (work2[p1.m_Pxy] == set_to)
			{
				if (prior + subz <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy;
					p2.m_Pz = p1.m_Pz - 1;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + subz);
					work2[p1.m_Pxy] = set_to2;
				}
			}
			else if (work2[p1.m_Pxy] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy;
				p2.m_Pz = p1.m_Pz - 1;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + subz)
					treap1.UpdatePriority(n2, prior + subz);
			}
		}
		if (p1.m_Pz + 1 < m_Endslice)
		{
			work2 = m_ImageSlices[p1.m_Pz + 1].ReturnTissues(m_ActiveTissuelayer);
			if (work2[p1.m_Pxy] == set_to)
			{
				if (prior + subz <= totcount)
				{
					p2.m_Pxy = p1.m_Pxy;
					p2.m_Pz = p1.m_Pz + 1;
					Treap<Posit, unsigned int>::Node* n = nullptr;
					treap1.Insert(n, p2, 1, prior + subz);
					work2[p1.m_Pxy] = set_to2;
				}
			}
			else if (work2[p1.m_Pxy] == set_to2)
			{
				p2.m_Pxy = p1.m_Pxy;
				p2.m_Pz = p1.m_Pz + 1;
				Treap<Posit, unsigned int>::Node* n2 = treap1.Lookup(p2);
				if (n2->GetPriority() > prior + subz)
					treap1.UpdatePriority(n2, prior + subz);
			}
		}

		delete treap1.Remove(n1);
	}

	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		work = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned i1 = 0; i1 < m_Area; i1++)
			if (work[i1] == set_to)
				work[i1] = 0;
	}
}

float SlicesHandler::AddSkin3D(int i1)
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

		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			float* bits = m_ImageSlices[i].ReturnWork();
			for (unsigned pos = 0; pos < m_Area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	AddSkin3D(i1, i1, i1, setto);
	return setto;
}

float SlicesHandler::AddSkin3D(int ix, int iy, int iz)
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

		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			float* bits = m_ImageSlices[i].ReturnWork();
			for (unsigned pos = 0; pos < m_Area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	// ix,iy,iz are in pixels
	AddSkin3D(ix, iy, iz, setto);
	return setto;
}

float SlicesHandler::AddSkin3DOutside(int i1)
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

		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			float* bits = m_ImageSlices[i].ReturnWork();
			for (unsigned pos = 0; pos < m_Area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	AddSkin3DOutside(i1, i1, i1, setto);
	return setto;
}

float SlicesHandler::AddSkin3DOutside2(int ix, int iy, int iz)
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

		for (unsigned short i = m_Startslice; i < m_Endslice; i++)
		{
			float* bits = m_ImageSlices[i].ReturnWork();
			for (unsigned pos = 0; pos < m_Area; pos++)
			{
				if (bits[pos] != p.high)
					setto = std::max(setto, bits[pos]);
			}
		}

		setto = (setto + p.high) / 2;
	}

	AddSkin3DOutside2(ix, iy, iz, setto);
	return setto;
}

void SlicesHandler::AddSkintissue3D(int ix, int iy, int iz, tissues_size_t f)
{
	tissues_size_t set_to = TISSUES_SIZE_MAX;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		m_ImageSlices[z].FloodExteriortissue(m_ActiveTissuelayer, set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<Posit> s;
	Posit p1;

	//	p1.pxy=position;
	p1.m_Pz = m_Activeslice;

	tissues_size_t* tissue1;
	tissues_size_t* tissue2;
	for (unsigned short z = m_Startslice; z + 1 < m_Endslice; z++)
	{
		tissue1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		tissue2 = m_ImageSlices[z + 1].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (tissue1[i] == 0 && tissue2[i] == set_to)
			{
				tissue1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = m_Endslice - 1; z > m_Startslice; z--)
	{
		tissue1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		tissue2 = m_ImageSlices[z - 1].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (tissue1[i] == 0 && tissue2[i] == set_to)
			{
				tissue1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}

	Posit i, j;
	tissues_size_t* tissue;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		tissue = m_ImageSlices[i.m_Pz].ReturnTissues(m_ActiveTissuelayer);
		if (i.m_Pxy % m_Width != 0 && tissue[i.m_Pxy - 1] == 0)
		{
			tissue[i.m_Pxy - 1] = set_to;
			j.m_Pxy = i.m_Pxy - 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if ((i.m_Pxy + 1) % m_Width != 0 && tissue[i.m_Pxy + 1] == 0)
		{
			tissue[i.m_Pxy + 1] = set_to;
			j.m_Pxy = i.m_Pxy + 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy >= m_Width && tissue[i.m_Pxy - m_Width] == 0)
		{
			tissue[i.m_Pxy - m_Width] = set_to;
			j.m_Pxy = i.m_Pxy - m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy < m_Area - m_Width && tissue[i.m_Pxy + m_Width] == 0)
		{
			tissue[i.m_Pxy + m_Width] = set_to;
			j.m_Pxy = i.m_Pxy + m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pz > m_Startslice)
		{
			tissue = m_ImageSlices[i.m_Pz - 1].ReturnTissues(m_ActiveTissuelayer);
			if (tissue[i.m_Pxy] == 0)
			{
				tissue[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz - 1;
				s.push_back(j);
			}
		}
		if (i.m_Pz + 1 < m_Endslice)
		{
			tissue = m_ImageSlices[i.m_Pz + 1].ReturnTissues(m_ActiveTissuelayer);
			if (tissue[i.m_Pxy] == 0)
			{
				tissue[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz + 1;
				s.push_back(j);
			}
		}
	}

	unsigned short x, y;
	unsigned long pos;
	unsigned short i1;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		tissue = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);

		for (y = 0; y < m_Height; y++)
		{
			pos = y * m_Width;
			i1 = 0;
			while (pos < (unsigned long)(y + 1) * m_Width)
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

		for (y = 0; y < m_Height; y++)
		{
			pos = (y + 1) * m_Width - 1;
			i1 = 0;
			while (pos > (unsigned long)y * m_Width)
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

		for (x = 0; x < m_Width; x++)
		{
			pos = x;
			i1 = 0;
			while (pos < m_Area)
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
				pos += m_Width;
			}
		}

		for (x = 0; x < m_Width; x++)
		{
			pos = m_Area + x - m_Width;
			i1 = 0;
			while (pos > m_Width)
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
				pos -= m_Width;
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
			(unsigned short*)malloc(sizeof(unsigned short) * m_Area);
	for (unsigned i1 = 0; i1 < m_Area; i1++)
		counter[i1] = 0;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		tissue = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		for (pos = 0; pos < m_Area; pos++)
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
	for (unsigned i1 = 0; i1 < m_Area; i1++)
		counter[i1] = 0;
	for (unsigned short z = m_Endslice - 1; z > m_Startslice; z--)
	{
		tissue = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		for (pos = 0; pos < m_Area; pos++)
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
	tissue = m_ImageSlices[m_Startslice].ReturnTissues(m_ActiveTissuelayer);
	for (pos = 0; pos < m_Area; pos++)
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

	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		tissue = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned i1 = 0; i1 < m_Area; i1++)
			if (tissue[i1] == set_to)
				tissue[i1] = 0;
	}
}

void SlicesHandler::AddSkintissue3DOutside(int ixyz, tissues_size_t f)
{
	tissues_size_t set_to = TISSUES_SIZE_MAX;
	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		m_ImageSlices[z].FloodExteriortissue(m_ActiveTissuelayer, set_to);
	}

	//Point p;
	//unsigned position;
	std::vector<Posit> s;
	std::vector<Posit> s1;
	std::vector<Posit>* s2 = &s1;
	std::vector<Posit>* s3 = &s;
	Posit p1;

	//	p1.pxy=position;
	p1.m_Pz = m_Activeslice;

	tissues_size_t* tissue1;
	tissues_size_t* tissue2;
	for (unsigned short z = m_Startslice; z + 1 < m_Endslice; z++)
	{
		tissue1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		tissue2 = m_ImageSlices[z + 1].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (tissue1[i] == 0 && tissue2[i] == set_to)
			{
				tissue1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}
	for (unsigned short z = m_Endslice - 1; z > m_Startslice; z--)
	{
		tissue1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		tissue2 = m_ImageSlices[z - 1].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (tissue1[i] == 0 && tissue2[i] == set_to)
			{
				tissue1[i] = set_to;
				p1.m_Pxy = i;
				p1.m_Pz = z;
				s.push_back(p1);
			}
		}
	}

	Posit i, j;
	tissues_size_t* tissue;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();

		tissue = m_ImageSlices[i.m_Pz].ReturnTissues(m_ActiveTissuelayer);
		if (i.m_Pxy % m_Width != 0 && tissue[i.m_Pxy - 1] == 0)
		{
			tissue[i.m_Pxy - 1] = set_to;
			j.m_Pxy = i.m_Pxy - 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if ((i.m_Pxy + 1) % m_Width != 0 && tissue[i.m_Pxy + 1] == 0)
		{
			tissue[i.m_Pxy + 1] = set_to;
			j.m_Pxy = i.m_Pxy + 1;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy >= m_Width && tissue[i.m_Pxy - m_Width] == 0)
		{
			tissue[i.m_Pxy - m_Width] = set_to;
			j.m_Pxy = i.m_Pxy - m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pxy <= m_Area - m_Width && tissue[i.m_Pxy + m_Width] == 0)
		{
			tissue[i.m_Pxy + m_Width] = set_to;
			j.m_Pxy = i.m_Pxy + m_Width;
			j.m_Pz = i.m_Pz;
			s.push_back(j);
		}
		if (i.m_Pz > m_Startslice)
		{
			tissue = m_ImageSlices[i.m_Pz - 1].ReturnTissues(m_ActiveTissuelayer);
			if (tissue[i.m_Pxy] == 0)
			{
				tissue[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz - 1;
				s.push_back(j);
			}
		}
		if (i.m_Pz + 1 < m_Endslice)
		{
			tissue = m_ImageSlices[i.m_Pz + 1].ReturnTissues(m_ActiveTissuelayer);
			if (tissue[i.m_Pxy] == 0)
			{
				tissue[i.m_Pxy] = set_to;
				j.m_Pxy = i.m_Pxy;
				j.m_Pz = i.m_Pz + 1;
				s.push_back(j);
			}
		}
	}

	for (unsigned short z = m_Startslice; z + 1 < m_Endslice; z++)
	{
		tissue1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		tissue2 = m_ImageSlices[z + 1].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned long i = 0; i < m_Area; i++)
		{
			if (tissue1[i] == set_to)
			{
				if (tissue2[i] != set_to)
				{
					p1.m_Pxy = i;
					p1.m_Pz = z + 1;
					(*s2).push_back(p1);
				}
			}
			else
			{
				if (tissue2[i] == set_to)
				{
					p1.m_Pxy = i;
					p1.m_Pz = z;
					(*s2).push_back(p1);
				}
			}
		}
	}

	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		tissue1 = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		unsigned long i = 0;
		for (unsigned short y = 0; y < m_Height; y++)
		{
			for (unsigned short x = 0; x + 1 < m_Width; x++)
			{
				if (tissue1[i] == set_to)
				{
					if (tissue1[i + 1] != set_to)
					{
						p1.m_Pxy = i + 1;
						p1.m_Pz = z;
						(*s2).push_back(p1);
					}
				}
				else
				{
					if (tissue1[i + 1] == set_to)
					{
						p1.m_Pxy = i;
						p1.m_Pz = z;
						(*s2).push_back(p1);
					}
				}
				i++;
			}
			i++;
		}
		i = 0;
		for (unsigned short y = 0; y + 1 < m_Height; y++)
		{
			for (unsigned short x = 0; x < m_Width; x++)
			{
				if (tissue1[i] == set_to)
				{
					if (tissue1[i + m_Width] != set_to)
					{
						p1.m_Pxy = i + m_Width;
						p1.m_Pz = z;
						(*s2).push_back(p1);
					}
				}
				else
				{
					if (tissue1[i + m_Width] == set_to)
					{
						p1.m_Pxy = i;
						p1.m_Pz = z;
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

			tissue = m_ImageSlices[i.m_Pz].ReturnTissues(m_ActiveTissuelayer);
			if (i.m_Pxy % m_Width != 0 && tissue[i.m_Pxy - 1] == set_to)
			{
				tissue[i.m_Pxy - 1] = f;
				j.m_Pxy = i.m_Pxy - 1;
				j.m_Pz = i.m_Pz;
				(*s3).push_back(j);
			}
			if ((i.m_Pxy + 1) % m_Width != 0 && tissue[i.m_Pxy + 1] == set_to)
			{
				tissue[i.m_Pxy + 1] = f;
				j.m_Pxy = i.m_Pxy + 1;
				j.m_Pz = i.m_Pz;
				(*s3).push_back(j);
			}
			if (i.m_Pxy >= m_Width && tissue[i.m_Pxy - m_Width] == set_to)
			{
				tissue[i.m_Pxy - m_Width] = f;
				j.m_Pxy = i.m_Pxy - m_Width;
				j.m_Pz = i.m_Pz;
				(*s3).push_back(j);
			}
			if (i.m_Pxy < m_Area - m_Width && tissue[i.m_Pxy + m_Width] == set_to)
			{
				tissue[i.m_Pxy + m_Width] = f;
				j.m_Pxy = i.m_Pxy + m_Width;
				j.m_Pz = i.m_Pz;
				(*s3).push_back(j);
			}
			if (i.m_Pz > m_Startslice)
			{
				tissue =
						m_ImageSlices[i.m_Pz - 1].ReturnTissues(m_ActiveTissuelayer);
				if (tissue[i.m_Pxy] == set_to)
				{
					tissue[i.m_Pxy] = f;
					j.m_Pxy = i.m_Pxy;
					j.m_Pz = i.m_Pz - 1;
					(*s3).push_back(j);
				}
			}
			if (i.m_Pz + 1 < m_Endslice)
			{
				tissue =
						m_ImageSlices[i.m_Pz + 1].ReturnTissues(m_ActiveTissuelayer);
				if (tissue[i.m_Pxy] == set_to)
				{
					tissue[i.m_Pxy] = f;
					j.m_Pxy = i.m_Pxy;
					j.m_Pz = i.m_Pz + 1;
					(*s3).push_back(j);
				}
			}
		}
		std::vector<Posit>* sdummy = s2;
		s2 = s3;
		s3 = sdummy;
	}

	while (!(*s2).empty())
	{
		(*s2).pop_back();
	}

	for (unsigned short z = m_Startslice; z < m_Endslice; z++)
	{
		tissue = m_ImageSlices[z].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned i1 = 0; i1 < m_Area; i1++)
			if (tissue[i1] == set_to)
				tissue[i1] = 0;
	}
}

void SlicesHandler::FillSkin3d(int thicknessX, int thicknessY, int thicknessZ, tissues_size_t backgroundID, tissues_size_t skinID)
{
	int num_tasks = m_Nrslices;
	QProgressDialog progress("Fill Skin in progress...", "Cancel", 0, num_tasks);
	progress.show();
	progress.setWindowModality(Qt::WindowModal);
	progress.setModal(true);
	progress.setValue(0);

	int skin_thick = thicknessX;

	std::vector<int> dims;
	dims.push_back(m_Width);
	dims.push_back(m_Height);
	dims.push_back(m_Nrslices);

	double max_d = skin_thick == 1 ? 1.75 * skin_thick : 1.2 * skin_thick;

	//Create the relative neighborhood
	std::vector<std::vector<int>> offsets_slices;
	for (int z = -skin_thick; z <= skin_thick; z++)
	{
		std::vector<int> offsets;
		for (int y = -skin_thick; y <= skin_thick; y++)
		{
			for (int x = -skin_thick; x <= skin_thick; x++)
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
		offsets_slices.push_back(offsets);
	}

	struct ChangesToMakeStruct
	{
		size_t m_SliceNumber;
		int m_PositionConvert;

		bool operator==(const ChangesToMakeStruct& m) const
		{
			return ((m.m_SliceNumber == m_SliceNumber) && (m.m_PositionConvert == m_PositionConvert));
		}
	};

	bool there_is_bg = false;
	bool there_is_skin = false;
	//unsigned long bgPixels=0;
	//unsigned long skinPixels=0;
	for (int i = 0; i < dims[2]; i++)
	{
		tissues_size_t* tissues_main = m_ImageSlices[i].ReturnTissues(0);
		for (int j = 0; j < m_Area && (!there_is_bg || !there_is_skin); j++)
		{
			tissues_size_t value = tissues_main[j];
			if (value == backgroundID)
				//bgPixels++;
				there_is_bg = true;
			else if (value == skinID)
				//skinPixels++;
				there_is_skin = true;
		}
		if (there_is_skin && there_is_bg)
			break;
	}

	if (!there_is_bg || !there_is_skin)
	{
		progress.setValue(num_tasks);
		return;
	}

	std::vector<tissues_size_t*> tissues_vector;
	for (int i = 0; i < m_ImageSlices.size(); i++)
		tissues_vector.push_back(m_ImageSlices[i].ReturnTissues(0));

	for (int i = 0; i < dims[2]; i++)
	{
		float* bmp1 = m_ImageSlices[i].ReturnBmp();

		tissues_size_t* tissue1 = m_ImageSlices[i].ReturnTissues(0);
		m_ImageSlices[i].PushstackBmp();

		for (unsigned int j = 0; j < m_Area; j++)
		{
			bmp1[j] = (float)tissue1[j];
		}

		m_ImageSlices[i].DeadReckoning((float)0);
		bmp1 = m_ImageSlices[i].ReturnWork();
	}

#ifdef NO_OPENMP_SUPPORT
	const int number_threads = 1;
#else
	const int number_threads = omp_get_max_threads();
#endif
	int slice_counter = 0;
	std::vector<std::vector<ChangesToMakeStruct>> partial_changes_threads;

	for (int i = 0; i < number_threads; i++)
	{
		ChangesToMakeStruct sample;
		sample.m_SliceNumber = -1;
		sample.m_PositionConvert = -1;
		std::vector<ChangesToMakeStruct> sample_per_thread;
		sample_per_thread.push_back(sample);
		partial_changes_threads.push_back(sample_per_thread);
	}

	//if(skinPixels<bgPixels/neighbors)
	{
#pragma omp parallel for
		for (int k = skin_thick; k < dims[2] - skin_thick; k++)
		{
			std::vector<ChangesToMakeStruct> partial_changes;

			Point p;
			for (int j = skin_thick; j < dims[1] - skin_thick; j++)
			{
				for (int i = skin_thick; i < dims[0] - skin_thick; i++)
				{
					int pos = i + j * dims[0];
					if (tissues_vector[k][pos] == skinID)
					{
						for (int z = 0; z < offsets_slices.size(); z++)
						{
							size_t neighbor_slice = z - ((offsets_slices.size() - 1) / 2);

							//iterate through neighbors of pixel
							//if any of these are not
							for (int l = 0; l < offsets_slices[z].size(); l++)
							{
								int idx = pos + offsets_slices[z][l];
								assert(idx >= 0 && idx < m_Area);
								p.px = i;
								p.py = j;
								tissues_size_t value = tissues_vector[k + neighbor_slice][idx];
								if (value == backgroundID)
								{
									for (int y = 0; y < offsets_slices.size(); y++)
									{
										size_t neighbor_slice2 = y - ((offsets_slices.size() - 1) / 2);

										//iterate through neighbors of pixel
										//if any of these are not
										for (int m = 0; m < offsets_slices[y].size(); m++)
										{
											int idx2 = idx + offsets_slices[y][m];
											assert(idx2 >= 0 && idx2 < m_Area);
											if (k + neighbor_slice + neighbor_slice2 < dims[2]) // && k + neighborSlice + neighborSlice2 >= 0 (always true)
											{
												tissues_size_t value2 = tissues_vector[k + neighbor_slice + neighbor_slice2][idx2];
												if (value2 != backgroundID && value2 != skinID)
												{
													ChangesToMakeStruct changes;
													changes.m_SliceNumber = k + neighbor_slice;
													changes.m_PositionConvert = idx;
													if (std::find(partial_changes.begin(), partial_changes.end(), changes) == partial_changes.end())
														partial_changes.push_back(changes);
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

			for (int i = 0; i < partial_changes.size(); i++)
			{
				partial_changes_threads[thread_id].push_back(partial_changes[i]);
			}

			if (0 == thread_id)
			{
				slice_counter++;
				progress.setValue(number_threads * slice_counter);
			}
		}
	}

	for (int i = 0; i < partial_changes_threads.size(); i++)
	{
		for (int j = 0; j < partial_changes_threads[i].size(); j++)
		{
			if (partial_changes_threads[i][j].m_SliceNumber != -1)
			{
				size_t slice = partial_changes_threads[i][j].m_SliceNumber;
				int pos = partial_changes_threads[i][j].m_PositionConvert;
				m_ImageSlices[slice].ReturnWork()[pos] = 255.0f;
			}
		}
	}
	for (int i = dims[2] - 1; i >= 0; i--)
	{
		float* bmp1 = m_ImageSlices[i].ReturnBmp();

		for (unsigned k = 0; k < m_Area; k++)
		{
			if (bmp1[k] < 0)
				bmp1[k] = 0;
			else
				bmp1[k] = 255.0f;
		}

		m_ImageSlices[i].SetMode(2, false);
		m_ImageSlices[i].PopstackBmp();
	}

	progress.setValue(num_tasks);
}

float SlicesHandler::CalculateVolume(Point p, unsigned short slicenr)
{
	Pair p1 = GetPixelsize();
	unsigned long count = 0;
	float f = GetWorkPt(p, slicenr);
	for (unsigned short j = m_Startslice; j < m_Endslice; j++)
		count += m_ImageSlices[j].ReturnWorkpixelcount(f);
	return GetSlicethickness() * p1.high * p1.low * count;
}

float SlicesHandler::CalculateTissuevolume(Point p, unsigned short slicenr)
{
	Pair p1 = GetPixelsize();
	unsigned long count = 0;
	tissues_size_t c = GetTissuePt(p, slicenr);
	for (unsigned short j = m_Startslice; j < m_Endslice; j++)
		count += m_ImageSlices[j].ReturnTissuepixelcount(m_ActiveTissuelayer, c);
	return GetSlicethickness() * p1.high * p1.low * count;
}

void SlicesHandler::Inversesliceorder()
{
	if (m_Nrslices > 0)
	{
		//		Bmphandler dummy;
		unsigned short rcounter = m_Nrslices - 1;
		for (unsigned short i = 0; i < m_Nrslices / 2; i++, rcounter--)
		{
			m_ImageSlices[i].Swap(m_ImageSlices[rcounter]);
			/*dummy=image_slices[i];
			image_slices[i]=image_slices[rcounter];
			image_slices[rcounter]=dummy;*/
		}
		ReverseUndosliceorder();
	}
}

void SlicesHandler::GammaMhd(unsigned short slicenr, short nrtissues, short dim, std::vector<std::string> mhdfiles, float* weights, float** centers, float* tol_f, float* tol_d)
{
	if (mhdfiles.size() + 1 < dim)
		return;
	//	if(slicenr>=startslice&&slicenr<endslice){
	float** bits = new float*[dim];
	for (unsigned short j = 0; j + 1 < dim; j++)
	{
		bits[j + 1] = new float[m_Area];
		if (bits[j + 1] == nullptr)
		{
			for (unsigned short i = 1; i < j; i++)
				delete[] bits[i];
			delete[] bits;
			return;
		}
	}

	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		bits[0] = m_ImageSlices[i].ReturnBmp();
		for (unsigned short k = 0; k + 1 < dim; k++)
		{
			if (!ImageReader::GetSlice(mhdfiles[k].c_str(), bits[k + 1], i, m_Width, m_Height))
			{
				for (unsigned short j = 1; j < dim; j++)
					delete[] bits[j];
				delete[] bits;
				return;
			}
		}
		MultidimensionalGamma mdg;
		Pair pair1 = GetPixelsize();
		mdg.Init(m_Width, m_Height, nrtissues, dim, bits, weights, centers, tol_f, tol_d, pair1.high, pair1.low);
		mdg.Execute();
		mdg.ReturnImage(m_ImageSlices[i].ReturnWork());
		m_ImageSlices[i].SetMode(2, false);
	}

	for (unsigned short j = 1; j < dim; j++)
		delete[] bits[j];
	delete[] bits;
	//	}
}

void SlicesHandler::StepsmoothZ(unsigned short n)
{
	if (n > (m_Endslice - m_Startslice))
		return;
	SmoothSteps stepsm;
	unsigned short linelength1 = m_Endslice - m_Startslice;
	tissues_size_t* line = new tissues_size_t[linelength1];
	stepsm.Init(n, linelength1, TissueInfos::GetTissueCount() + 1);
	for (unsigned long i = 0; i < m_Area; i++)
	{
		unsigned short k = 0;
		for (unsigned short j = m_Startslice; j < m_Endslice; j++, k++)
		{
			line[k] = (m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer))[i];
		}
		stepsm.Dostepsmooth(line);
		k = 0;
		for (unsigned short j = m_Startslice; j < m_Endslice; j++, k++)
		{
			(m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer))[i] = line[k];
		}
	}
	delete[] line;
}

void SlicesHandler::SmoothTissues(unsigned short n)
{
	// TODO: Implement criterion: Cells must be contiguous to the center of the specified filter
	std::map<tissues_size_t, unsigned short> tissuecount;
	tissues_size_t* tissuesnew =
			(tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area);
	if (n % 2 == 0)
		n++;
	unsigned short filtersize = n * n;
	n = (unsigned short)(0.5f * n);

	for (unsigned short j = m_Startslice; j < m_Endslice; j++)
	{
		m_ImageSlices[j].Copyfromtissue(m_ActiveTissuelayer, tissuesnew);
		tissues_size_t* tissuesold =
				m_ImageSlices[j].ReturnTissues(m_ActiveTissuelayer);
		for (unsigned short y = n; y < m_Height - n; y++)
		{
			for (unsigned short x = n; x < m_Width - n; x++)
			{
				// Tissue count within filter size
				tissuecount.clear();
				for (short i = -n; i <= n; i++)
				{
					for (short j = -n; j <= n; j++)
					{
						tissues_size_t currtissue =
								tissuesold[(y + i) * m_Width + (x + j)];
						if (tissuecount.find(currtissue) == tissuecount.end())
						{
							tissuecount.insert(std::pair<tissues_size_t, unsigned short>(currtissue, 1));
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
				tissues_size_t tissuemajor = tissuesold[y * m_Width + x];
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
				tissuesnew[y * m_Width + x] = tissuemajor;
			}
		}
		m_ImageSlices[j].Copy2tissue(m_ActiveTissuelayer, tissuesnew);
	}

	free(tissuesnew);
}

void SlicesHandler::Regrow(unsigned short sourceslicenr, unsigned short targetslicenr, int n)
{
	float* lbmap = (float*)malloc(sizeof(float) * m_Area);
	for (unsigned i = 0; i < m_Area; i++)
		lbmap[i] = 0;

	tissues_size_t* dummy;
	tissues_size_t* results1 =
			(tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area);
	tissues_size_t* results2 =
			(tissues_size_t*)malloc(sizeof(tissues_size_t) * m_Area);
	tissues_size_t* tissues1 =
			m_ImageSlices[sourceslicenr].ReturnTissues(m_ActiveTissuelayer);
	for (unsigned int i = 0; i < m_Area; i++)
	{
		if (tissues1[i] == 0)
			results2[i] = TISSUES_SIZE_MAX;
		else
			results2[i] = tissues1[i];
	}

	for (int l = 0; l < n; l++)
	{
		for (unsigned int i = 0; i < m_Area; i++)
			results1[i] = results2[i];

		int i1 = 0;

		for (unsigned short i = 0; i < (m_Height - 1); i++)
		{
			for (unsigned short j = 0; j < m_Width; j++)
			{
				if (results2[i1] != results2[i1 + m_Width])
					results1[i1] = results1[i1 + m_Width] = 0;

				i1++;
			}
		}

		i1 = 0;

		for (unsigned short i = 0; i < m_Height; i++)
		{
			for (unsigned short j = 0; j < (m_Width - 1); j++)
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

	for (unsigned int i = 0; i < m_Area; i++)
		lbmap[i] = float(results2[i]);

	delete results1;
	delete results2;

	ImageForestingTransformRegionGrowing* if_trg =
			m_ImageSlices[sourceslicenr].IfTrgInit(lbmap);
	float thresh = 0;

	float* f2 = if_trg->ReturnPf();
	for (unsigned i = 0; i < m_Area; i++)
	{
		if (thresh < f2[i])
		{
			thresh = f2[i];
		}
	}
	if (thresh == 0)
		thresh = 1;

	float* f1 = if_trg->ReturnLb();
	tissues_size_t* tissue_bits =
			m_ImageSlices[targetslicenr].ReturnTissues(m_ActiveTissuelayer);

	for (unsigned i = 0; i < m_Area; i++)
	{
		// if(f2[i]<thresh&&f1[i]<255.1f) tissue_bits[i]=(tissues_size_t)(f1[i]+0.1f);
		if (f2[i] < thresh && f1[i] < (float)TISSUES_SIZE_MAX + 0.1f)
			tissue_bits[i] = (tissues_size_t)(f1[i] + 0.1f); // TODO: Correct?
		else
			tissue_bits[i] = 0;
	}

	delete if_trg;
	delete lbmap;
}

bool SlicesHandler::Unwrap(float jumpratio, float shift)
{
	bool ok = true;
	Pair p;
	GetBmprange(&p);
	float range = p.high - p.low;
	float jumpabs = jumpratio * range;
	for (unsigned short i = m_Startslice; i < m_Endslice; i++)
	{
		if (i > m_Startslice)
		{
			if ((m_ImageSlices[i - 1]).ReturnBmp()[0] -
							m_ImageSlices[i].ReturnBmp()[0] >
					jumpabs)
				shift += range;
			else if (m_ImageSlices[i].ReturnBmp()[0] -
									 (m_ImageSlices[i - 1]).ReturnBmp()[0] >
							 jumpabs)
				shift -= range;
		}
		ok &= m_ImageSlices[i].Unwrap(jumpratio, range, shift);
		;
	}
	if (m_Area > 0)
	{
		for (unsigned short i = m_Startslice + 1; i < m_Endslice; i++)
		{
			if ((m_ImageSlices[i - 1]).ReturnWork()[m_Area - 1] -
							m_ImageSlices[i].ReturnWork()[m_Area - 1] >
					jumpabs)
				return false;
			if (m_ImageSlices[i].ReturnWork()[m_Area - 1] -
							(m_ImageSlices[i - 1]).ReturnWork()[m_Area - 1] >
					jumpabs)
				return false;
		}
	}
	return ok;
}

unsigned SlicesHandler::GetNumberOfUndoSteps()
{
	return this->m_UndoQueue.ReturnNrundomax();
}

void SlicesHandler::SetNumberOfUndoSteps(unsigned n)
{
	this->m_UndoQueue.SetNrundo(n);
}

unsigned SlicesHandler::GetNumberOfUndoArrays()
{
	return this->m_UndoQueue.ReturnNrundoarraysmax();
}

void SlicesHandler::SetNumberOfUndoArrays(unsigned n)
{
	this->m_UndoQueue.SetNrundoarraysmax(n);
}

std::vector<iseg::tissues_size_t> SlicesHandler::TissueSelection() const
{
	auto sel_set = TissueInfos::GetSelectedTissues();
	return std::vector<iseg::tissues_size_t>(sel_set.begin(), sel_set.end());
}

void SlicesHandler::SetTissueSelection(const std::vector<tissues_size_t>& sel)
{
	TissueInfos::SetSelectedTissues(std::set<tissues_size_t>(sel.begin(), sel.end()));
	m_OnTissueSelectionChanged(sel);
}

size_t SlicesHandler::NumberOfColors() const
{
	return (m_ColorLookupTable != nullptr) ? m_ColorLookupTable->NumberOfColors() : 0;
}

void SlicesHandler::GetColor(size_t idx, unsigned char& r, unsigned char& g, unsigned char& b) const
{
	if (m_ColorLookupTable != nullptr)
	{
		m_ColorLookupTable->GetColor(idx, r, g, b);
	}
}

bool SlicesHandler::ComputeTargetConnectivity(ProgressInfo* progress)
{
	using input_type = SlicesHandlerITKInterface::image_ref_type;
	using image_type = itk::Image<unsigned, 3>;

	SlicesHandlerITKInterface wrapper(this);
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

		if (!progress || (progress && !progress->WasCanceled()))
		{
			// copy result back
			iseg::Paste<image_type, input_type>(filter->GetOutput(), all_slices);

			// auto-scale target rendering
			SetTargetFixedRange(false);

			return true;
		}
	}
	catch (itk::ExceptionObject& e)
	{
		ISEG_ERROR("Exception occurred: " << e.what());
	}
	return false;
}

bool SlicesHandler::ComputeSplitTissues(tissues_size_t tissue, ProgressInfo* progress)
{
	using input_type = SlicesHandlerITKInterface::tissues_ref_type;
	using internal_type = itk::Image<unsigned char, 3>;
	using image_type = itk::Image<unsigned, 3>;

	SlicesHandlerITKInterface wrapper(this);
	auto all_slices = wrapper.GetTissues(true);

	auto threshold = itk::BinaryThresholdImageFilter<input_type, internal_type>::New();
	threshold->SetLowerThreshold(tissue);
	threshold->SetUpperThreshold(tissue);
	threshold->SetInput(all_slices);

	auto filter = itk::ConnectedComponentImageFilter<internal_type, image_type>::New();
	filter->SetInput(threshold->GetOutput());
	filter->SetFullyConnected(true);
	if (progress)
	{
		auto observer = ItkProgressObserver::New();
		observer->SetProgressInfo(progress);
		filter->AddObserver(itk::ProgressEvent(), observer);
	}
	try
	{
		filter->Update();
		auto components = filter->GetOutput();

		if (!progress || !progress->WasCanceled())
		{
			// add tissue infos
			auto n = filter->GetObjectCount();
			if (n > 1)
			{
				ISEG_INFO("Tissue has " << n << " regions");

				// find which object is largest -> this one will keep its original name & color
				std::vector<size_t> hist(n + 1, 0);
				itk::ImageRegionConstIterator<image_type> it(components, components->GetLargestPossibleRegion());
				for (it.GoToBegin(); !it.IsAtEnd(); ++it)
				{
					hist[it.Get()]++;
				}
				hist[0] = 0;
				const size_t max_label = std::distance(hist.begin(), std::max_element(hist.begin(), hist.end()));

				// mapping from object number to new tissue index
				tissues_size_t ninitial = TissueInfos::GetTissueCount();
				std::vector<tissues_size_t> object2index(n + 1, 0);
				object2index[max_label] = tissue;
				tissues_size_t idx = 1;
				for (tissues_size_t i = 1; i <= n; ++i)
				{
					if (i != max_label)
					{
						TissueInfo info(*TissueInfos::GetTissueInfo(tissue));
						info.m_Name += (boost::format("_%d") % static_cast<int>(idx)).str();
						TissueInfos::AddTissue(info);
						object2index.at(i) = ninitial + idx++;
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

				return true;
			}
			else
			{
				ISEG_INFO("Tissue has only one connected region");
			}
		}
	}
	catch (itk::ExceptionObject& e)
	{
		ISEG_ERROR("Exception occurred: " << e.what());
	}
	return false;
}

} // namespace iseg
