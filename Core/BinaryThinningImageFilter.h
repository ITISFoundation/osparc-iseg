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

#include <itkBinaryThinningImageFilter.h>
#include <itkBinaryThinningImageFilter3D.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkMedialAxisImageFilter.h>
#include <itkRescaleIntensityImageFilter.h>

namespace iseg {
template<typename TInputImage, typename TOutputImage, unsigned int Dimension>
class BinaryThinningImageFilter : public itk::BinaryThinningImageFilter<TInputImage, TOutputImage>
{
};

template<typename TInputImage, typename TOutputImage>
class BinaryThinningImageFilter<TInputImage, TOutputImage, 3> : public itk::MedialAxisImageFilter<TInputImage, TOutputImage>
{
};

template<typename TInputImage, typename TOutputImage>
typename TOutputImage::Pointer BinaryThinning(TInputImage* input, typename TInputImage::PixelType lower = 1)
{
	itkStaticConstMacro(ImageDimension, size_t, TInputImage::ImageDimension);
	using value_type = typename TInputImage::PixelType;

	using input_type = TInputImage;
	using output_type = TOutputImage;
	using mask_type = itk::Image<unsigned char, ImageDimension>;

	using threshold_filter_type = itk::BinaryThresholdImageFilter<input_type, mask_type>;
	using thinnning_filter_type = iseg::BinaryThinningImageFilter<mask_type, mask_type, ImageDimension>;
	using rescale_filter_type = itk::RescaleIntensityImageFilter<mask_type, output_type>;

	auto threshold = threshold_filter_type::New();
	threshold->SetInput(input);
	threshold->SetLowerThreshold(lower);

	auto thinning = thinnning_filter_type::New();
	thinning->SetInput(threshold->GetOutput());

	auto rescale = rescale_filter_type::New();
	rescale->SetInput(thinning->GetOutput());
	rescale->SetOutputMinimum(0);
	rescale->SetOutputMaximum(255);
	rescale->InPlaceOn();

	try
	{
		rescale->Update();
		return rescale->GetOutput();
	}
	catch (itk::ExceptionObject& e)
	{
		ISEG_ERROR() << "exception occurred " << e.what();
	}
	return nullptr;
}
} // namespace iseg
