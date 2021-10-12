/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#pragma once

#include "Data/ItkProgressObserver.h"
#include "Data/ItkUtils.h"
#include "Data/ScopeExit.h"
#include "Data/SlicesHandlerITKInterface.h"

#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkPasteImageFilter.h>

#ifndef NO_OPENMP_SUPPORT
#	include <omp.h>
#endif

#include <boost/variant.hpp>

namespace iseg {

namespace details {
template<unsigned int Dimension>
itk::FlatStructuringElement<Dimension> MakeBall(const typename itk::ImageBase<Dimension>::SpacingType& spacing, double radius)
{
	auto ball = iseg::MakeBall<bool, Dimension>(spacing, radius);
	return itk::FlatStructuringElement<Dimension>::FromImage(ball);
}

template<unsigned int Dimension>
itk::FlatStructuringElement<Dimension> MakeBall(const itk::Size<Dimension>& radius)
{
	bool radius_is_parametric = true;
	return itk::FlatStructuringElement<Dimension>::Ball(radius, radius_is_parametric);
}
}

template<class TInputImage>
itk::FlatStructuringElement<TInputImage::ImageDimension> MakeBall(typename TInputImage::Pointer input, boost::variant<int, float> radius)
{
	itkStaticConstMacro(Dimension, unsigned int, TInputImage::ImageDimension);
	using spacing_type = typename TInputImage::SpacingType;

	class MyVisitor : public boost::static_visitor<itk::FlatStructuringElement<Dimension>>
	{
	public:
		explicit MyVisitor(const spacing_type& spacing) : m_Spacing(spacing) {}

		itk::FlatStructuringElement<Dimension> operator()(int r) const
		{
			itk::Size<Dimension> radius;
			radius.Fill(r);

			return details::MakeBall<Dimension>(radius);
		}

		itk::FlatStructuringElement<Dimension> operator()(float r) const
		{
			return details::MakeBall<Dimension>(m_Spacing, static_cast<double>(r));
		}

	private:
		spacing_type m_Spacing;
	};

	auto ball = boost::apply_visitor(MyVisitor(input->GetSpacing()), radius);
	return ball;
}

enum eOperation {
	kErode,
	kDilate,
	kClose,
	kOpen
};

template<class TInputImage, class TOutputImage = itk::Image<unsigned char, TInputImage::ImageDimension>>
typename TOutputImage::Pointer
		MorphologicalOperation(typename TInputImage::Pointer input, boost::variant<int, float> radius, eOperation operation, const typename TInputImage::RegionType& requested_region, iseg::ProgressInfo* progress = nullptr)
{
	using input_image_type = TInputImage;
	using image_type = TOutputImage;
	itkStaticConstMacro(Dimension, unsigned int, TInputImage::ImageDimension);
	using kernel_type = itk::FlatStructuringElement<Dimension>;

	auto ball = MakeBall<TInputImage>(input, radius);

	unsigned char foreground_value = 255;

	auto threshold = itk::BinaryThresholdImageFilter<input_image_type, image_type>::New();
	threshold->SetInput(input);
	threshold->SetLowerThreshold(0.001f); // background is '0'
	threshold->SetInsideValue(foreground_value);

	std::vector<typename itk::ImageSource<image_type>::Pointer> filters;
	if (operation == eOperation::kErode || operation == eOperation::kOpen)
	{
		auto erode = itk::BinaryErodeImageFilter<image_type, image_type, kernel_type>::New();
		erode->SetInput(threshold->GetOutput());
		erode->SetKernel(ball);
		erode->SetErodeValue(foreground_value);
		erode->SetBackgroundValue(0);
		filters.push_back(typename itk::ImageSource<image_type>::Pointer(erode));

		if (operation == kOpen)
		{
			auto dilate = itk::BinaryDilateImageFilter<image_type, image_type, kernel_type>::New();
			dilate->SetInput(erode->GetOutput());
			dilate->SetKernel(ball);
			dilate->SetDilateValue(foreground_value);
			filters.push_back(typename itk::ImageSource<image_type>::Pointer(dilate));
		}
	}
	else
	{
		auto dilate = itk::BinaryDilateImageFilter<image_type, image_type, kernel_type>::New();
		dilate->SetInput(threshold->GetOutput());
		dilate->SetKernel(ball);
		dilate->SetDilateValue(foreground_value);
		filters.push_back(typename itk::ImageSource<image_type>::Pointer(dilate));

		if (operation == kClose)
		{
			auto erode = itk::BinaryErodeImageFilter<image_type, image_type, kernel_type>::New();
			erode->SetInput(dilate->GetOutput());
			erode->SetKernel(ball);
			erode->SetErodeValue(foreground_value);
			erode->SetBackgroundValue(0);
			filters.push_back(typename itk::ImageSource<image_type>::Pointer(erode));
		}
	}

	if (progress)
	{
		auto observer = iseg::ItkProgressObserver::New();
		observer->SetProgressInfo(progress);
		for (const auto& filter : filters)
			filter->AddObserver(itk::ProgressEvent(), observer);
	}

	filters.back()->GetOutput()->SetRequestedRegion(requested_region);
	filters.back()->Update();
	return filters.back()->GetOutput();
}

/** \brief Do morpological operation on target image
*/
void MorphologicalOperation(iseg::SlicesHandlerInterface* handler, boost::variant<int, float> radius, eOperation operation, bool true3d, iseg::ProgressInfo* progress)
{
	iseg::SlicesHandlerITKInterface itkhandler(handler);
	if (true3d)
	{
		using input_type = iseg::SlicesHandlerITKInterface::image_ref_type;
		using output_type = itk::Image<unsigned char, 3>;

		auto target = itkhandler.GetTarget(true); // get active slices
		auto region = target->GetBufferedRegion();
		auto output = MorphologicalOperation<input_type>(target, radius, operation, region, progress);
		iseg::Paste<output_type, input_type>(output, target);
	}
	else
	{
		using input_type = itk::Image<float, 2>;
		using output_type = itk::Image<unsigned char, 2>;

		std::int64_t startslice = handler->StartSlice();
		std::int64_t endslice = handler->EndSlice();

		if (progress)
		{
			progress->SetNumberOfSteps(endslice - startslice);
		}

#ifndef NO_OPENMP_SUPPORT
		const auto guard = MakeScopeExit([nthreads = omp_get_num_threads()]() { omp_set_num_threads(nthreads); });
		omp_set_num_threads(std::max<int>(2, std::thread::hardware_concurrency() / 2));
#endif

#pragma omp parallel for
		for (std::int64_t slice = startslice; slice < endslice; ++slice)
		{
			auto target = itkhandler.GetTargetSlice(slice);
			auto region = target->GetBufferedRegion();
			auto output = MorphologicalOperation<input_type>(target, radius, operation, region, nullptr);
			iseg::Paste<output_type, input_type>(output, target);

			if (progress)
			{
				progress->Increment();
			}
		}
	}
}

} // namespace iseg
