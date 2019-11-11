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
#include "Data/SlicesHandlerITKInterface.h"
#include "Data/Transform.h"

#include <itkCastImageFilter.h>
#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkImageSeriesWriter.h>
#include <itkNumericSeriesFileNames.h>


#include <vtkNew.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkXMLImageDataWriter.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace iseg {

template<typename T>
bool ImageWriter::writeVolume(const std::string& filename, const std::vector<T*>& all_slices, eSliceSelection selection, const SlicesHandlerInterface* handler)
{
	unsigned dims[3] = {handler->width(), handler->height(), handler->num_slices()};
	unsigned start=0, end = handler->num_slices();
	switch(selection)
	{
	case eSliceSelection::kActiveSlices:
		start = handler->start_slice();
		end = handler->end_slice();
		break;
	case eSliceSelection::kSlice:
		start = handler->active_slice();
		end = handler->active_slice() + 1;
		break;
	default:
		break;
	}

	auto image = wrapToITK(all_slices, dims, start, end, handler->spacing(), handler->transform());
	if (image)
	{
		boost::filesystem::path path(filename);
		std::string ext = boost::algorithm::to_lower_copy(path.has_extension() ? path.extension().string() : "");
		if (ext == ".vti" || ext == ".vtk")
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

			if (ext == ".vti")
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
		else if (ext == ".bmp" || ext == ".png" || ext == ".jpg" || ext == ".jpeg")
		{
			using image_type = itk::SliceContiguousImage<T>;
			using contiguous_image_type = itk::Image<T, 3>;
			using image_2d_type = itk::Image<T, 2>;
			//using caster_type = itk::CastImageFilter<image_type, contiguous_image_type>;
			using writer_type = itk::ImageSeriesWriter<image_type, image_2d_type>;
			using names_generator_type = itk::NumericSeriesFileNames;

			std::string format = (path.parent_path() / (path.stem().string() + "%03d" + ext)).string();

			auto region = image->GetLargestPossibleRegion();
			auto start = region.GetIndex();
			auto size = region.GetSize();

			auto names_generator = names_generator_type::New();;
			names_generator->SetSeriesFormat(format.c_str());
			names_generator->SetStartIndex(start[2]);
			names_generator->SetEndIndex(start[2] + size[2] - 1);
			names_generator->SetIncrementIndex(1);

			//auto caster = caster_type::New();
			//caster->SetInput(image);

			auto writer = writer_type::New();
			writer->SetInput(image);// caster->GetOutput());
			writer->SetFileName(filename);
			writer->SetFileNames(names_generator->GetFileNames());

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

bool ImageWriter::writeVolume(const std::string& file_path, SlicesHandlerInterface* handler, eImageSelection img_selection, eSliceSelection slice_selection)
{
	switch (img_selection)
	{
	case eImageSelection::kSource:
		return writeVolume<float>(file_path, handler->source_slices(), slice_selection, handler);
	case eImageSelection::kTarget:
		return writeVolume<float>(file_path, handler->target_slices(), slice_selection, handler);
	case eImageSelection::kTissue:
		return writeVolume<tissues_size_t>(file_path, handler->tissue_slices(handler->active_tissuelayer()), slice_selection, handler);
	}
	return false;
}

}// namespace iseg