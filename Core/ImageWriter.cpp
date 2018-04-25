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

#include "ImageToITK.h"
#include "ImageWriter.h"
#include "InitializeITKFactory.h"
#include "VtkGlue/itkImageToVTKImageFilter.h"

#include "Data/Transform.h"

#include <itkImage.h>
#include <itkImageFileWriter.h>

#include <vtkNew.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkXMLImageDataWriter.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

using namespace iseg;

template<typename T>
bool ImageWriter::writeVolume(const char* filename, const T** data,
		unsigned width, unsigned height,
		unsigned nrslices, const float spacing[3],
		const Transform& transform)
{
	initializeITKFactory();

	typedef itk::Image<T, 3> image_type;
	typedef itk::ImageFileWriter<image_type> writer_type;

	typename image_type::Pointer image =
			ImageToITK::copy(data, width, height, 0, nrslices, spacing, transform);

	if (image)
	{
		boost::filesystem::path path(filename);
		std::string extension = boost::algorithm::to_lower_copy(
				path.has_extension() ? path.extension().string() : "");
		if (extension == ".vti" || extension == ".vtk")
		{
			typedef itk::ImageToVTKImageFilter<image_type> connector_type;
			auto connector = connector_type::New();
			connector->SetInput(image);
			connector->Update();

			if (extension == ".vti")
			{
				vtkNew<vtkXMLImageDataWriter> writer;
				writer->SetFileName(filename);
				writer->SetInputData(connector->GetOutput());
				writer->SetDataModeToAppended();
				return writer->Write() != 0;
			}
			else
			{
				vtkNew<vtkStructuredPointsWriter> writer;
				writer->SetFileName(filename);
				writer->SetInputData(connector->GetOutput());
				writer->SetFileType(m_Binary ? VTK_BINARY : VTK_ASCII);
				return writer->Write() != 0;
			}
		}

		auto writer = writer_type::New();
		writer->SetInput(image);
		writer->SetFileName(filename);
		try
		{
			writer->Update();
			return true;
		}
		catch (const itk::ExceptionObject& e)
		{
			std::string msg = e.GetDescription();
			auto file = e.GetFile();
			auto line = e.GetLine();
		}
	}
	return false;
}
