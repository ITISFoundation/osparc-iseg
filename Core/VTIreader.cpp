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

#include "VTIreader.h"

#include <vtkDataArray.h>
#include <vtkExecutive.h>
#include <vtkFloatArray.h>
#include <vtkImageData.h>
#include <vtkInformation.h>
#include <vtkPointData.h>
#include <vtkSmartPointer.h>
#include <vtkStreamingDemandDrivenPipeline.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnsignedShortArray.h>
#include <vtkXMLImageDataReader.h>
#include <vtkXMLImageDataWriter.h>
#include <vtkZLibDataCompressor.h>

#pragma warning(disable : 4018) // BL quiet compiler

using namespace iseg;

bool VTIreader::getSlice(const char* filename, float* slice, unsigned slicenr,
						 unsigned width, unsigned height)
{
	vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();
	if (reader->CanReadFile(filename) == 0)
	{
		return false;
	}
	reader->SetFileName(filename);
	reader->Update();

	int ext[6] = {0, 0, 0, 0, 0, 0};
	double org[3] = {0.0, 0.0, 0.0};
	double spc[3] = {0.0, 0.0, 0.0};
	vtkImageData* img = reader->GetOutput();
	img->GetSpacing(spc);
	img->GetExtent(ext);
	img->GetOrigin(org);
	std::string className = img->GetPointData()->GetScalars()->GetClassName();

	if (ext[3] - ext[2] + 1 != (int)height)
		return false;
	if (ext[1] - ext[0] + 1 != (int)width)
		return false;
	if (slicenr > ext[5] - ext[4])
		return false;
	unsigned long long pos = 0;

	if (className == "vtkUnsignedCharArray")
	{
		unsigned char* ptr = (unsigned char*)img->GetScalarPointer(
			ext[0], ext[2], (int)slicenr + ext[4]);
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slice[pos] = (float)ptr[pos];
			}
		}
	}
	else if (className == "vtkFloatArray")
	{
		float* ptr = (float*)img->GetScalarPointer(ext[0], ext[2],
												   (int)slicenr + ext[4]);
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slice[pos] = ptr[pos];
			}
		}
	}
	else if (className == "vtkDoubleArray")
	{
		double* ptr = (double*)img->GetScalarPointer(ext[0], ext[2],
													 (int)slicenr + ext[4]);
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slice[pos] = (float)ptr[pos];
			}
		}
	}
	else
	{
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slice[pos] = img->GetScalarComponentAsFloat(
					i, j, (int)slicenr + ext[4], 0);
			}
		}
	}

	reader->Delete();
	return true;
}

float* VTIreader::getSliceInfo(const char* filename, unsigned slicenr,
							   unsigned& width, unsigned& height)
{
	vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();
	if (reader->CanReadFile(filename) == 0)
	{
		return nullptr;
	}
	reader->SetFileName(filename);
	reader->Update();

	int ext[6] = {0, 0, 0, 0, 0, 0};
	double org[3] = {0.0, 0.0, 0.0};
	double spc[3] = {0.0, 0.0, 0.0};
	vtkImageData* img = reader->GetOutput();
	img->GetSpacing(spc);
	img->GetExtent(ext);
	img->GetOrigin(org);
	std::string className = img->GetPointData()->GetScalars()->GetClassName();

	if (slicenr > ext[5] - ext[4])
		return nullptr;
	height = (unsigned)ext[3] - ext[2] + 1;
	width = (unsigned)ext[1] - ext[0] + 1;

	float* slice = new float[height * (unsigned long)width];
	unsigned long long pos = 0;

	if (className == "vtkUnsignedCharArray")
	{
		unsigned char* ptr = (unsigned char*)img->GetScalarPointer(
			ext[0], ext[2], (int)slicenr + ext[4]);
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slice[pos] = (float)ptr[pos];
			}
		}
	}
	else if (className == "vtkFloatArray")
	{
		float* ptr = (float*)img->GetScalarPointer(ext[0], ext[2],
												   (int)slicenr + ext[4]);
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slice[pos] = ptr[pos];
			}
		}
	}
	else if (className == "vtkDoubleArray")
	{
		double* ptr = (double*)img->GetScalarPointer(ext[0], ext[2],
													 (int)slicenr + ext[4]);
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slice[pos] = (float)ptr[pos];
			}
		}
	}
	else
	{
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slice[pos] = img->GetScalarComponentAsFloat(
					i, j, (int)slicenr + ext[4], 0);
			}
		}
	}

	reader->Delete();
	return slice;
}

bool VTIreader::getVolume(const char* filename, float** slices,
						  unsigned nrslices, unsigned width, unsigned height,
						  const std::string& arrayName)
{
	vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();
	if (reader->CanReadFile(filename) == 0)
	{
		cerr << "VTIreader::getVolume() : can not read file " << filename
			 << endl;
		return false;
	}
	reader->SetFileName(filename);
	reader->Update();

	int ext[6] = {0, 0, 0, 0, 0, 0};
	double org[3] = {0.0, 0.0, 0.0};
	double spc[3] = {0.0, 0.0, 0.0};
	vtkImageData* img = reader->GetOutput();
	img->GetSpacing(spc);
	img->GetExtent(ext);
	img->GetOrigin(org);

	img->GetPointData()->SetActiveScalars(
		arrayName.c_str()); // for GetScalarPointer()
	vtkDataArray* scalars = img->GetPointData()->GetScalars(arrayName.c_str());
	if (!scalars)
	{
		cerr << "no such array: " << arrayName << endl;
	}
	std::string className = scalars->GetClassName();

	if (ext[3] + 1 - ext[2] != (int)height)
		return false;
	if (ext[1] + 1 - ext[0] != (int)width)
		return false;
	if (ext[5] + 1 - ext[4] != (int)nrslices)
		return false;

	for (int k = 0; k < nrslices; k++)
	{
		unsigned long long pos = 0;
		if (className == "vtkUnsignedCharArray")
		{
			unsigned char* ptr = (unsigned char*)img->GetScalarPointer(
				ext[0], ext[2], k + ext[4]);
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slices[k][pos] = (float)ptr[pos];
				}
			}
		}
		else if (className == "vtkFloatArray")
		{
			float* ptr =
				(float*)img->GetScalarPointer(ext[0], ext[2], k + ext[4]);
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slices[k][pos] = ptr[pos];
				}
			}
		}
		else if (className == "vtkDoubleArray")
		{
			double* ptr =
				(double*)img->GetScalarPointer(ext[0], ext[2], k + ext[4]);
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slices[k][pos] = (float)ptr[pos];
				}
			}
		}
		else
		{
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slices[k][pos] =
						img->GetScalarComponentAsFloat(i, j, k + ext[4], 0);
				}
			}
		}
	}
	reader->Delete();
	return true;
}

bool VTIreader::getVolumeAll(const char* filename, float** slicesbmp,
							 float** sliceswork, tissues_size_t** slicestissue,
							 unsigned nrslices, unsigned width, unsigned height)
{
	vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();
	if (reader->CanReadFile(filename) == 0)
	{
		return false;
	}
	reader->SetFileName(filename);
	reader->Update();

	int ext[6] = {0, 0, 0, 0, 0, 0};
	double org[3] = {0.0, 0.0, 0.0};
	double spc[3] = {0.0, 0.0, 0.0};
	vtkImageData* img = reader->GetOutput();
	int index;
	img->GetSpacing(spc);
	img->GetExtent(ext);
	img->GetOrigin(org);

	if (ext[3] + 1 - ext[2] != (int)height)
		return false;
	if (ext[1] + 1 - ext[0] != (int)width)
		return false;
	if (ext[5] + 1 - ext[4] != (int)nrslices)
		return false;

	vtkDataArray* arraybmp = img->GetPointData()->GetArray("Source", index);
	if (arraybmp == 0)
	{
		reader->Delete();
		cerr << "VTIreader::getVolumeAll() : no Source array" << endl;
		return false;
	}
	vtkDataArray* arraywork = img->GetPointData()->GetArray("Target", index);
	if (arraywork == 0)
	{
		reader->Delete();
		cerr << "VTIreader::getVolumeAll() : no Target array" << endl;
		return false;
	}
	vtkDataArray* arraytissue = img->GetPointData()->GetArray("Tissue", index);
	if (arraytissue == 0)
	{
		reader->Delete();
		cerr << "VTIreader::getVolumeAll() : no Tissue array" << endl;
		return false;
	}
	std::string classNamebmp = arraybmp->GetClassName();
	std::string classNamework = arraywork->GetClassName();
	std::string classNametissue = arraytissue->GetClassName();

	bool tissueUnsignedChar =
		(classNametissue.compare("vtkUnsignedCharArray") == 0);
	bool tissueUnsignedShort =
		(classNametissue.compare("vtkUnsignedShortArray") == 0);
	if (!(tissueUnsignedChar || tissueUnsignedShort) ||
		classNamebmp.compare("vtkFloatArray") != 0 ||
		classNamework.compare("vtkFloatArray") != 0)
	{
		reader->Delete();
		cerr << "VTIreader::getVolumeAll() : unsupported array types for the "
				"arrays"
			 << endl;
		return false;
	}

	img->GetPointData()->SetActiveScalars("Source");
	for (int k = 0; k < nrslices; k++)
	{
		unsigned long long pos = 0;
		float* ptrfloat =
			(float*)img->GetScalarPointer(ext[0], ext[2], k + ext[4]);
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				slicesbmp[k][pos] = ptrfloat[pos];
			}
		}
	}
	img->GetPointData()->SetActiveScalars("Target");
	for (int k = 0; k < nrslices; k++)
	{
		unsigned long long pos = 0;
		float* ptrfloat =
			(float*)img->GetScalarPointer(ext[0], ext[2], k + ext[4]);
		for (int j = ext[2]; j <= ext[3]; j++)
		{
			for (int i = ext[0]; i <= ext[1]; i++, pos++)
			{
				sliceswork[k][pos] = ptrfloat[pos];
			}
		}
	}
	img->GetPointData()->SetActiveScalars("Tissue");
	if (tissueUnsignedChar)
	{
		for (int k = 0; k < nrslices; k++)
		{
			unsigned long long pos = 0;
			unsigned char* ptrchar = (unsigned char*)img->GetScalarPointer(
				ext[0], ext[2], k + ext[4]);
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slicestissue[k][pos] = ptrchar[pos];
				}
			}
		}
	}
	else if (tissueUnsignedShort)
	{
		for (int k = 0; k < nrslices; k++)
		{
			unsigned long long pos = 0;
			unsigned short* ptrshort = (unsigned short*)img->GetScalarPointer(
				ext[0], ext[2], k + ext[4]);
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slicestissue[k][pos] = ptrshort[pos];
				}
			}
		}
	}

	reader->Delete();
	return true;
}

bool VTIreader::getVolume(const char* filename, float** slices,
						  unsigned startslice, unsigned nrslices,
						  unsigned width, unsigned height,
						  const std::string& arrayName)
{
	vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();
	if (reader->CanReadFile(filename) == 0)
	{
		return false;
	}
	reader->SetFileName(filename);
	reader->Update();

	int ext[6] = {0, 0, 0, 0, 0, 0};
	double org[3] = {0.0, 0.0, 0.0};
	double spc[3] = {0.0, 0.0, 0.0};
	vtkImageData* img = reader->GetOutput();
	img->GetSpacing(spc);
	img->GetExtent(ext);
	img->GetOrigin(org);

	img->GetPointData()->SetActiveScalars(
		arrayName.c_str()); // for GetScalarPointer()
	vtkDataArray* scalars = img->GetPointData()->GetScalars(arrayName.c_str());
	if (!scalars)
	{
		cerr << "no such array: " << arrayName << endl;
	}
	std::string className = scalars->GetClassName();

	if (ext[3] + 1 - ext[2] != (int)height)
		return false;
	if (ext[1] + 1 - ext[0] != (int)width)
		return false;
	if (ext[5] + 1 - ext[4] < (int)nrslices + startslice)
		return false;

	for (int k = 0; k < nrslices; k++)
	{
		unsigned long long pos = 0;

		if (className == "vtkUnsignedCharArray")
		{
			unsigned char* ptr = (unsigned char*)img->GetScalarPointer(
				ext[0], ext[2], startslice + k + ext[4]);
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slices[k][pos] = (float)ptr[pos];
				}
			}
		}
		else if (className == "vtkFloatArray")
		{
			float* ptr = (float*)img->GetScalarPointer(ext[0], ext[2],
													   startslice + k + ext[4]);
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slices[k][pos] = ptr[pos];
				}
			}
		}
		else if (className == "vtkDoubleArray")
		{
			double* ptr = (double*)img->GetScalarPointer(
				ext[0], ext[2], startslice + k + ext[4]);
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slices[k][pos] = (float)ptr[pos];
				}
			}
		}
		else
		{
			for (int j = ext[2]; j <= ext[3]; j++)
			{
				for (int i = ext[0]; i <= ext[1]; i++, pos++)
				{
					slices[k][pos] = img->GetScalarComponentAsFloat(
						i, j, startslice + k + ext[4], 0);
				}
			}
		}
	}

	reader->Delete();
	return true;
}

bool VTIreader::getInfo(const char* filename, unsigned& width, unsigned& height,
						unsigned& nrslices, float* pixelsize, float* offset,
						std::vector<std::string>& arrayNames)
{
	vtkXMLImageDataReader* reader = vtkXMLImageDataReader::New();
	if (reader->CanReadFile(filename) == 0)
	{
		return false;
	}
	reader->SetFileName(filename);
	reader->UpdateInformation();
	int extent[6];
	double spacing[3];
	double origin[3];
	vtkInformation* outInfo = reader->GetExecutive()->GetOutputInformation(0);

	outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
	width = extent[1] - extent[0] + 1;
	height = extent[3] - extent[2] + 1;
	nrslices = extent[5] - extent[4] + 1;
	pixelsize[0] = 1.0f;
	pixelsize[1] = 1.0f;
	pixelsize[2] = 1.0f;
	outInfo->Get(vtkDataObject::SPACING(), spacing);
	pixelsize[0] = spacing[0];
	pixelsize[1] = spacing[1];
	pixelsize[2] = spacing[2];
	offset[0] = 0.0f;
	offset[1] = 0.0f;
	offset[2] = 0.0f;
	outInfo->Get(vtkDataObject::ORIGIN(), origin);
	offset[0] = origin[0];
	offset[1] = origin[1];
	offset[2] = origin[2];

	const int NPA = reader->GetNumberOfPointArrays();
	// cerr << "VTIreader::getInfo() : number of point arrays: " << NPA << endl;
	arrayNames.resize(NPA);
	for (int i = 0; i < NPA; ++i)
	{
		// cerr << "\tarray id = " << i << ", name = " << reader->GetPointArrayName(i) << endl;
		arrayNames[i] = std::string(reader->GetPointArrayName(i));
	}

	if (!NPA)
		return false;

	return true;
}

bool VTIwriter::writeVolumeAll(const char* filename, float** slicesbmp,
							   float** sliceswork,
							   tissues_size_t** slicestissue,
							   tissues_size_t nrtissues, unsigned nrslices,
							   unsigned width, unsigned height,
							   float* pixelsize, float* offset, bool binary,
							   bool compress)
{
	vtkSmartPointer<vtkImageData> input = vtkSmartPointer<vtkImageData>::New();
	input->SetExtent(0, (int)width - 1, 0, (int)height - 1, 0,
					 (int)nrslices - 1);
	input->SetSpacing(pixelsize[0], pixelsize[1], pixelsize[2]);
	input->SetOrigin((double)offset[0], (double)offset[1], (double)offset[2]);

	vtkSmartPointer<vtkFloatArray> bmparray =
		vtkSmartPointer<vtkFloatArray>::New();
	bmparray->SetNumberOfValues((vtkIdType)width * (vtkIdType)height *
								(vtkIdType)nrslices);
	bmparray->SetNumberOfComponents(1);
	bmparray->SetName("Source");
	//bmparray->SetNumberOfTuples(xxx);
	vtkSmartPointer<vtkFloatArray> workarray =
		vtkSmartPointer<vtkFloatArray>::New();
	workarray->SetNumberOfValues((vtkIdType)width * (vtkIdType)height *
								 (vtkIdType)nrslices);
	workarray->SetNumberOfComponents(1);
	workarray->SetName("Target");
	input->GetPointData()->AddArray(bmparray);
	input->GetPointData()->AddArray(workarray);
	if (nrtissues <= 255)
	{
		vtkSmartPointer<vtkUnsignedCharArray> tissuearray =
			vtkSmartPointer<vtkUnsignedCharArray>::New();
		tissuearray->SetNumberOfValues((vtkIdType)width * (vtkIdType)height *
									   (vtkIdType)nrslices);
		tissuearray->SetNumberOfComponents(1);
		tissuearray->SetName("Tissue");
		input->GetPointData()->AddArray(tissuearray);
	}
	else if (sizeof(tissues_size_t) == sizeof(unsigned short))
	{
		vtkSmartPointer<vtkUnsignedShortArray> tissuearray =
			vtkSmartPointer<vtkUnsignedShortArray>::New();
		tissuearray->SetNumberOfValues((vtkIdType)width * (vtkIdType)height *
									   (vtkIdType)nrslices);
		tissuearray->SetNumberOfComponents(1);
		tissuearray->SetName("Tissue");
		input->GetPointData()->AddArray(tissuearray);
	}
	else
	{
		cerr << "VTIwriter::writeVolumeAll: Error, tissues_size_t not "
				"implemented."
			 << endl;
		return false;
	}
	//BL? input->Update();

	int info = input->GetPointData()->SetActiveScalars("Source");
	for (int k = 0; k < nrslices; k++)
	{
		unsigned long long pos = 0;
		float* ptrfloat = (float*)input->GetScalarPointer(0, 0, k);
		for (int j = 0; j < height; j++)
		{
			for (int i = 0; i < width; i++, pos++)
			{
				ptrfloat[pos] = slicesbmp[k][pos];
			}
		}
	}
	input->GetPointData()->SetActiveScalars("Target");
	for (int k = 0; k < nrslices; k++)
	{
		unsigned long long pos = 0;
		float* ptrfloat = (float*)input->GetScalarPointer(0, 0, k);
		for (int j = 0; j < height; j++)
		{
			for (int i = 0; i < width; i++, pos++)
			{
				ptrfloat[pos] = sliceswork[k][pos];
			}
		}
	}
	input->GetPointData()->SetActiveScalars("Tissue");
	if (nrtissues <= 255)
	{
		for (int k = 0; k < nrslices; k++)
		{
			unsigned long long pos = 0;
			unsigned char* ptrchar =
				(unsigned char*)input->GetScalarPointer(0, 0, k);
			for (int j = 0; j < height; j++)
			{
				for (int i = 0; i < width; i++, pos++)
				{
					ptrchar[pos] = slicestissue[k][pos];
				}
			}
		}
	}
	else
	{
		for (int k = 0; k < nrslices; k++)
		{
			unsigned long long pos = 0;
			unsigned short* ptrshort =
				(unsigned short*)input->GetScalarPointer(0, 0, k);
			for (int j = 0; j < height; j++)
			{
				for (int i = 0; i < width; i++, pos++)
				{
					ptrshort[pos] = slicestissue[k][pos];
				}
			}
		}
	}

	vtkSmartPointer<vtkZLibDataCompressor> compressor =
		vtkSmartPointer<vtkZLibDataCompressor>::New();
	vtkSmartPointer<vtkXMLImageDataWriter> writer =
		vtkSmartPointer<vtkXMLImageDataWriter>::New();
	writer->SetInputData(input);
	if (compress)
		writer->SetCompressor(compressor);
	else
		writer->SetCompressorTypeToNone();
	writer->SetDataModeToAppended();
	if (binary)
		writer->EncodeAppendedDataOff();
	writer->SetFileName(filename);
	writer->Write();

	return true;
}
