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

#include "ImageWriter.h"
#include "VtkGlue/itkImageToVTKImageFilter.h"

#include "Data/Logger.h"
#include "Data/SliceHandlerItkWrapper.h"
#include "Data/Transform.h"

#include <itkCastImageFilter.h>
#include <itkImage.h>
#include <itkImageFileWriter.h>

#include <vtkNew.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkXMLImageDataWriter.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

using namespace iseg;

template<typename T>
bool ImageWriter::writeVolume(const std::string& filename, const std::vector<T*>& all_slices, bool active_slices, const SliceHandlerInterface* handler)
{
	unsigned dims[3] = {handler->width(), handler->height(), handler->num_slices()};
	auto image = wrapToITK(all_slices, dims, handler->start_slice(), handler->end_slice(), handler->spacing(), handler->transform());

	if (image)
	{
		boost::filesystem::path path(filename);
		std::string extension = boost::algorithm::to_lower_copy(path.has_extension() ? path.extension().string() : "");
		if (extension == ".vti" || extension == ".vtk")
		{
			using image_type = itk::SliceContiguousImage<T>;
			using contiguous_image_type = itk::Image<T, 3>;
			using caster_type = itk::CastImageFilter<image_type, contiguous_image_type>;
			using connector_type = itk::ImageToVTKImageFilter<contiguous_image_type>;

			auto caster = caster_type::New();
			caster->SetInput(image);

			auto connector = connector_type::New();
			connector->SetInput(caster->GetOutput());
			connector->Update();

			if (extension == ".vti")
			{
				vtkNew<vtkXMLImageDataWriter> writer;
				writer->SetFileName(filename.c_str());
				writer->SetInputData(connector->GetOutput());
				writer->SetDataModeToAppended();
				return writer->Write() != 0;
			}
			else
			{
				vtkNew<vtkStructuredPointsWriter> writer;
				writer->SetFileName(filename.c_str());
				writer->SetInputData(connector->GetOutput());
				writer->SetFileType(m_Binary ? VTK_BINARY : VTK_ASCII);
				return writer->Write() != 0;
			}
		}
		else
		{
			using image_type = itk::SliceContiguousImage<T>;
			using contiguous_image_type = itk::Image<T, 3>;
			using caster_type = itk::CastImageFilter<image_type, contiguous_image_type>;
			using writer_type = itk::ImageFileWriter<contiguous_image_type>;

			auto caster = caster_type::New();
			caster->SetInput(image);

			auto writer = writer_type::New();
			writer->SetInput(caster->GetOutput());
			writer->SetFileName(filename);
			try
			{
				writer->Update();
				return true;
			}
			catch (const itk::ExceptionObject& e)
			{
				ISEG_ERROR(e.GetDescription());
			}
		}
	}
	return false;
}
