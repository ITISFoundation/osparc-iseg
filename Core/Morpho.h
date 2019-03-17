/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#pragma once

#include "Data/ItkUtils.h"
#include "Data/SliceHandlerItkWrapper.h"

#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkPasteImageFilter.h>

#include <boost/variant.hpp>

namespace morpho {

template<unsigned int Dimension>
itk::FlatStructuringElement<Dimension> MakeBall(const typename itk::ImageBase<Dimension>::SpacingType& spacing, double radius)
{
	auto ball = iseg::MakeBall<bool, Dimension>(spacing, radius);
	return itk::FlatStructuringElement<Dimension>::FromImage(ball);
}

template<unsigned int Dimension>
itk::FlatStructuringElement<Dimension> MakeBall(const itk::Size<Dimension>& radius)
{
	bool radiusIsParametric = true;
	return itk::FlatStructuringElement<Dimension>::Ball(radius, radiusIsParametric);
}

enum eOperation {
	kErode,
	kDilate,
	kClose,
	kOpen
};

template<class TInputImage, class TOutputImage = itk::Image<unsigned char, TInputImage::ImageDimension>>
typename TOutputImage::Pointer
		MorphologicalOperation(typename TInputImage::Pointer input,
				boost::variant<int, float> radius, eOperation operation,
				const typename TInputImage::RegionType& requested_region)
{
	using input_image_type = TInputImage;
	using image_type = TOutputImage;
	itkStaticConstMacro(Dimension, unsigned int, TInputImage::ImageDimension);
	using kernel_type = itk::FlatStructuringElement<Dimension>;
	using spacing_type = typename itk::ImageBase<Dimension>::SpacingType;

			class MyVisitor : public boost::static_visitor<itk::FlatStructuringElement<Dimension>>
	{
	public:
		MyVisitor(const spacing_type& spacing) : _spacing(spacing) {}

		itk::FlatStructuringElement<Dimension> operator()(int r) const
		{
			itk::Size<Dimension> radius;
			radius.Fill(r);

			return morpho::MakeBall<Dimension>(radius);
		}

		itk::FlatStructuringElement<Dimension> operator()(float r) const
		{
			return morpho::MakeBall<Dimension>(_spacing, static_cast<double>(r));
		}

	private:
		spacing_type _spacing;
	};

	auto ball = boost::apply_visitor(MyVisitor(input->GetSpacing()), radius);

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

	filters.back()->GetOutput()->SetRequestedRegion(requested_region);
	filters.back()->Update();
	return filters.back()->GetOutput();
}

/** \brief Do morpological operation on target image
*/
void MorphologicalOperation(iseg::SliceHandlerInterface* handler,
		boost::variant<int, float> radius, eOperation operation, bool true3d)
{
	iseg::SliceHandlerItkWrapper itkhandler(handler);
	if (true3d)
	{
		using input_type = iseg::SliceHandlerItkWrapper::image_ref_type;
		using output_type = itk::Image<unsigned char, 3>;

		auto target = itkhandler.GetTarget(true); // get active slices
		auto region = target->GetBufferedRegion();
		auto output = MorphologicalOperation<input_type>(target, radius, operation, region);
		iseg::Paste<output_type, input_type>(output, target);
	}
	else
	{
		using input_type = itk::Image<float, 2>;
		using output_type = itk::Image<unsigned char, 2>;

		size_t startslice = handler->start_slice();
		size_t endslice = handler->end_slice();

#pragma omp parallel for
		for (std::int64_t slice = startslice; slice < endslice; ++slice)
		{
			auto target = itkhandler.GetTargetSlice(slice);
			auto region = target->GetBufferedRegion();
			auto output = MorphologicalOperation<input_type>(target, radius, operation, region);
			iseg::Paste<output_type, input_type>(output, target);
		}
	}
}

} // namespace morpho
