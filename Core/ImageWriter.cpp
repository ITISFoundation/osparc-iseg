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
#include <itkRescaleIntensityImageFilter.h>

#include <vtkNew.h>
#include <vtkStructuredPointsWriter.h>
#include <vtkXMLImageDataWriter.h>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>

namespace iseg {

template<typename T>
bool ImageWriter::WriteVolume(const std::string& filename, const std::vector<T*>& all_slices, eSliceSelection selection, const SlicesHandlerInterface* handler)
{
	unsigned dims[3] = {handler->Width(), handler->Height(), handler->NumSlices()};
	unsigned start = 0, end = handler->NumSlices();
	switch (selection)
	{
	case eSliceSelection::kActiveSlices:
		start = handler->StartSlice();
		end = handler->EndSlice();
		break;
	case eSliceSelection::kSlice:
		start = handler->ActiveSlice();
		end = handler->ActiveSlice() + 1;
		break;
	default:
		break;
	}

	auto image = wrapToITK(all_slices, dims, start, end, handler->Spacing(), handler->ImageTransform());
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
			using t_slices_type = itk::SliceContiguousImage<T>;
			using t_image_type = itk::Image<T, 3>;
			using o_image_type = itk::Image<unsigned char, 3>;
			using o_image2_type = itk::Image<unsigned char, 2>;

			std::string format = (path.parent_path() / (path.stem().string() + "%04d" + ext)).string();
			auto names_generator = itk::NumericSeriesFileNames::New();
			names_generator->SetSeriesFormat(format.c_str());
			names_generator->SetStartIndex(start);
			names_generator->SetEndIndex(end - 1);
			names_generator->SetIncrementIndex(1);

			using rescale_type = itk::RescaleIntensityImageFilter<t_slices_type, t_image_type>;
			auto rescale = rescale_type::New();
			rescale->SetInput(image);
			rescale->SetOutputMinimum(0);
			rescale->SetOutputMaximum(itk::NumericTraits<o_image_type::PixelType>::max());

			using filter_type = itk::CastImageFilter<t_image_type, o_image_type>;
			auto filter = filter_type::New();
			filter->SetInput(rescale->GetOutput());

			using writer_type = itk::ImageSeriesWriter<o_image_type, o_image2_type>;
			auto writer = writer_type::New();
			writer->SetFileNames(names_generator->GetFileNames());
			writer->SetInput(filter->GetOutput());

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

bool ImageWriter::WriteVolume(const std::string& file_path, SlicesHandlerInterface* handler, eImageSelection img_selection, eSliceSelection slice_selection)
{
	switch (img_selection)
	{
	case eImageSelection::kSource:
		return WriteVolume<float>(file_path, handler->SourceSlices(), slice_selection, handler);
	case eImageSelection::kTarget:
		return WriteVolume<float>(file_path, handler->TargetSlices(), slice_selection, handler);
	case eImageSelection::kTissue:
		return WriteVolume<tissues_size_t>(file_path, handler->TissueSlices(handler->ActiveTissuelayer()), slice_selection, handler);
	}
	return false;
}

} // namespace iseg