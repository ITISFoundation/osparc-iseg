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

#include "SmoothTissues.h"

#include "../Data/ItkUtils.h"
#include "../Data/SliceHandlerItkWrapper.h"

#include <itkDiscreteGaussianImageFilter.h>
#include <itkSignedMaurerDistanceMapImageFilter.h>

namespace iseg {

template<class TInput, class TOutput>
typename TOutput::Pointer _ComputeSDF(const TInput* img, int foreground, double sigma)
{
	using sdf_type = itk::SignedMaurerDistanceMapImageFilter<TInput, TOutput>;

	auto sdf = sdf_type::New();
	sdf->SetInput(img);
	sdf->SetBackgroundValue(foreground); // background is inside
	sdf->SetInsideIsPositive(true);			 // background is inside and is negative
	sdf->SetSquaredDistance(false);			 // \todo test with squared
	sdf->SetUseImageSpacing(true);

	if (sigma <= 0.0)
	{
		sdf->Update();
		return sdf->GetOutput();
	}

	using gaussian_type = itk::DiscreteGaussianImageFilter<TOutput, TOutput>;
	auto gaussian = gaussian_type::New();
	gaussian->SetInput(sdf->GetOutput());
	gaussian->SetVariance(sigma * sigma);
	gaussian->Update();
	return gaussian->GetOutput();
}

template<class TInput>
bool _SmoothTissues(TInput* tissues, const std::vector<bool>& locks, double sigma)
{
	itkStaticConstMacro(ImageDimension, size_t, TInput::ImageDimension);
	using label_image_type = TInput;
	using real_image_type = itk::Image<float, ImageDimension>;

	bool ok = false;

	// compute smooth signed distance function (sdf) for each non-locked tissue
	std::vector<typename real_image_type::Pointer> sdf_images(locks.size(), nullptr);
	{
		itk::ImageRegionIterator<label_image_type> it(tissues, tissues->GetBufferedRegion());
		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			// if not locked and no sdf computed yet
			if (!locks.at(it.Get()) && !sdf_images[it.Get()])
			{
				ok = true; // at least one unlocked tissue found

				// compute sdf to tissue @ current location
				sdf_images[it.Get()] = _ComputeSDF<label_image_type, real_image_type>(
						tissues, it.Get(), sigma);
			}
		}
	}

	// assign non-locked voxels to tissue with most negative sdf
	{
		itk::ImageRegionIteratorWithIndex<label_image_type> it(tissues, tissues->GetBufferedRegion());
		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			// don't overwrite locked tissues
			if (!locks.at(it.Get()))
			{
				auto idx = it.GetIndex();

				// find most negative sdf ("most inside")
				auto min_sdf = std::numeric_limits<float>::max();
				auto min_label = it.Get();
				for (tissues_size_t i = 0; i != sdf_images.size(); ++i)
				{
					if (sdf_images[i])
					{
						auto sdf = sdf_images[i]->GetPixel(idx);
						if (sdf < min_sdf)
						{
							min_sdf = sdf;
							min_label = i;
						}
					}
				}
				it.Set(min_label);
			}
		}
	}

	return ok;
}

bool SmoothTissues(SliceHandlerInterface* handler, size_t start_slice, size_t end_slice, double sigma, bool smooth3d)
{
	SliceHandlerItkWrapper itkhandler(handler);
	auto locks = handler->tissue_locks();

	if (smooth3d)
	{
		using label_image_type = SliceHandlerItkWrapper::tissues_ref_type;

		auto tissues = itkhandler.GetTissues(start_slice, end_slice);

		return _SmoothTissues<label_image_type>(tissues, locks, sigma);
	}
	else
	{
		using label_image_type = itk::Image<unsigned short, 2>;

#pragma omp parallel for
		for (std::int64_t slice = start_slice; slice < end_slice; ++slice)
		{
			// get labelfield at current slice
			auto tissues = itkhandler.GetTissuesSlice(slice);

			_SmoothTissues<label_image_type>(tissues, locks, sigma);
		}
	}

	return true;
}

} // namespace iseg
