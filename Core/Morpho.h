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

#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkFlatStructuringElement.h>
#include <itkImageRegionIteratorWithIndex.h>
#include <itkPasteImageFilter.h>

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
				itk::FlatStructuringElement<3> structuringElement, eOperation operation, const typename TInputImage::RegionType& requested_region)
{
	using input_image_type = TInputImage;
	using image_type = TOutputImage;
	itkStaticConstMacro(Dimension, unsigned int, TInputImage::ImageDimension);
	using kernel_type = itk::FlatStructuringElement<Dimension>;

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
		erode->SetKernel(structuringElement);
		erode->SetErodeValue(foreground_value);
		erode->SetBackgroundValue(0);
		filters.push_back(typename itk::ImageSource<image_type>::Pointer(erode));

		if (operation == kOpen)
		{
			auto dilate = itk::BinaryDilateImageFilter<image_type, image_type, kernel_type>::New();
			dilate->SetInput(erode->GetOutput());
			dilate->SetKernel(structuringElement);
			dilate->SetDilateValue(foreground_value);
			filters.push_back(typename itk::ImageSource<image_type>::Pointer(dilate));
		}
	}
	else
	{
		auto dilate = itk::BinaryDilateImageFilter<image_type, image_type, kernel_type>::New();
		dilate->SetInput(threshold->GetOutput());
		dilate->SetKernel(structuringElement);
		dilate->SetDilateValue(foreground_value);
		filters.push_back(typename itk::ImageSource<image_type>::Pointer(dilate));

		if (operation == kClose)
		{
			auto erode = itk::BinaryErodeImageFilter<image_type, image_type, kernel_type>::New();
			erode->SetInput(dilate->GetOutput());
			erode->SetKernel(structuringElement);
			erode->SetErodeValue(foreground_value);
			erode->SetBackgroundValue(0);
			filters.push_back(typename itk::ImageSource<image_type>::Pointer(erode));
		}
	}

	filters.back()->GetOutput()->SetRequestedRegion(requested_region);
	filters.back()->Update();
	return filters.back()->GetOutput();
}

template<typename T>
itk::Image<unsigned char, 3>::Pointer
		MorphologicalOperation(typename itk::SliceContiguousImage<T>::Pointer input,
				itk::FlatStructuringElement<3> structuringElement, eOperation operation,
				size_t startslice, size_t endslice)
{
	using input_image_type = itk::SliceContiguousImage<T>;
	using output_image_type = itk::Image<unsigned char, 3>;

	auto start = input->GetLargestPossibleRegion().GetIndex();
	start[2] = startslice;

	auto size = input->GetLargestPossibleRegion().GetSize();
	size[2] = endslice - startslice;

	return MorphologicalOperation<input_image_type, output_image_type>(
			input, structuringElement, operation, itk::ImageBase<3>::RegionType(start, size));
}

} // namespace morpho
