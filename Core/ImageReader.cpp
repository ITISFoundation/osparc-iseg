/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "ImageReader.h"
#include "Precompiled.h"

#include "InitializeITKFactory.h"
#include "Transform.h"
#include "VTIreader.h"

#include <itkImage.h>
#include <itkImageFileReader.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

typedef itk::Image<float, 3> image_type;
typedef itk::ImageFileReader<image_type> reader_type;

bool ImageReader::getSlice(const char *filename, float *slice, unsigned slicenr,
													 unsigned width, unsigned height)
{
	return getVolume(filename, &slice, slicenr, 1, width, height);
}

float *ImageReader::getSliceInfo(const char *filename, unsigned slicenr,
																 unsigned &width, unsigned &height)
{
	unsigned nrslices;
	float spacing[3];
	Transform tr;
	if (getInfo(filename, width, height, nrslices, spacing, tr))
	{
		float *slice = new float[height * (unsigned long)width];
		if (getSlice(filename, slice, slicenr, width, height))
		{
			return slice;
		}
		delete[] slice;
	}
	return nullptr;
}

bool ImageReader::getVolume(const char *filename, float **slices,
														unsigned nrslices, unsigned width, unsigned height)
{
	return getVolume(filename, slices, 0, nrslices, width, height);
}

bool ImageReader::getVolume(const char *filename, float **slices,
														unsigned startslice, unsigned nrslices,
														unsigned width, unsigned height)
{
	initializeITKFactory();

	// ITK does not know how to load VTI
	boost::filesystem::path path(filename);
	std::string extension = boost::algorithm::to_lower_copy(
			path.has_extension() ? path.extension().string() : "");
	if (extension == ".vti")
	{
		unsigned w, h, n;
		float s[3], o[3];
		std::vector<std::string> arrayNames;
		if (VTIreader::getInfo(filename, w, h, n, s, o, arrayNames) &&
				!arrayNames.empty())
		{
			return VTIreader::getVolume(filename, slices, startslice, nrslices, width,
																	height, arrayNames[0]);
		}
	}

	auto reader = reader_type::New();
	reader->SetFileName(filename);
	try
	{
		reader->UpdateLargestPossibleRegion();
	}
	catch (itk::ExceptionObject &)
	{
		return false;
	}

	auto image = reader->GetOutput();
	auto container = image->GetPixelContainer();
	image_type::PixelType *buffer = container->GetImportPointer();

	for (unsigned k = 0; k < nrslices; k++)
	{
		size_t pos3d = (k + startslice) * width * height, pos = 0;
		for (unsigned j = 0; j < height; j++)
		{
			for (unsigned i = 0; i < width; i++, pos++, pos3d++)
			{
				slices[k][pos] = buffer[pos3d];
			}
		}
	}
	return true;
}

bool ImageReader::getInfo(const char *filename, unsigned &width,
													unsigned &height, unsigned &nrslices,
													float spacing[3], Transform &transform)
{
	initializeITKFactory();

	auto imageIO = itk::ImageIOFactory::CreateImageIO(
			filename, itk::ImageIOFactory::ReadMode);
	if (imageIO)
	{
		imageIO->SetFileName(filename);
		imageIO->ReadImageInformation();
		unsigned int N = imageIO->GetNumberOfDimensions();

		width = static_cast<unsigned>(imageIO->GetDimensions(0));
		height = static_cast<unsigned>(imageIO->GetDimensions(1));
		nrslices = static_cast<unsigned>(imageIO->GetDimensions(2));

		transform.setIdentity();
		for (unsigned int r = 0; r < N; r++)
		{
			spacing[r] = static_cast<float>(imageIO->GetSpacing(r));

			for (unsigned int c = 0; c < N; c++)
				transform[r][c] = imageIO->GetDirection(c)[r];

			transform[r][3] = imageIO->GetOrigin(r);
		}

		return true;
	}
	else
	{
		boost::filesystem::path path(filename);
		std::string extension = boost::algorithm::to_lower_copy(
				path.has_extension() ? path.extension().string() : "");
		if (extension == ".vti")
		{
			float offset[3];
			std::vector<std::string> arrayNames;
			VTIreader::getInfo(filename, width, height, nrslices, spacing, offset,
												 arrayNames);

			transform.setIdentity();
			transform.setOffset(offset);
			return true;
		}
	}
	return false;
}
