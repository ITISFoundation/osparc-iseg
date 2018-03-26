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

#include "../Addon/itkSliceContiguousImage.h"

#include <itkFlatStructuringElement.h>
#include <itkBinaryThresholdImageFilter.h>
#include <itkBinaryDilateImageFilter.h>
#include <itkBinaryErodeImageFilter.h>
#include <itkPasteImageFilter.h>

namespace morpho
{
	namespace detail
	{
		unsigned NextOddInteger(double v)
		{
			int vint = static_cast<int>(v);
			// make odd
			if (vint % 2 == 0)
			{
				vint++;
			}
			// increase by 2 to next odd
			if (v > vint)
			{
				vint += 2;
			}
			return vint;
		}

		template<typename T>
		typename itk::Image<T, 3>::Pointer MakeBall(const itk::ImageBase<3>::SpacingType& spacing, double radius)
		{
			typedef typename itk::Image<T, 3> image_type;

			image_type::SizeType size;
			size[0] = NextOddInteger(radius / spacing[0]);
			size[1] = NextOddInteger(radius / spacing[1]);
			size[2] = NextOddInteger(radius / spacing[2]);

			image_type::RegionType region;
			region.SetSize(size);

			auto img = image_type::New();
			img->SetRegions(region);
			img->Allocate();
			img->FillBuffer(0);
			img->SetSpacing(spacing);

			int center[3] = {
				static_cast<int>(size[0] / 2),
				static_cast<int>(size[1] / 2),
				static_cast<int>(size[2] / 2),
			};

			double radius2 = radius * radius;
			image_type::IndexType idx;
			for (int k = 0; k < size[2]; ++k)
			{
				for (int j = 0; j < size[1]; ++j)
				{
					for (int i = 0; i < size[0]; ++i)
					{
						double dx = spacing[0] * (i - center[0]);
						double dy = spacing[1] * (j - center[1]);
						double dz = spacing[2] * (k - center[2]);
						if (dx*dx + dy*dy + dz*dz < radius2)
						{
							idx[0] = i;
							idx[1] = j;
							idx[2] = k;
							img->SetPixel(idx, 1);
						}
					}
				}
			}
			return img;
		}
	}

	itk::FlatStructuringElement<3> MakeBall(const itk::ImageBase<3>::SpacingType& spacing, double radius)
	{
		auto ball = detail::MakeBall<bool>(spacing, radius);
		return itk::FlatStructuringElement<3>::FromImage(ball);
	}

	itk::FlatStructuringElement<3> MakeBall(const itk::Size<3>& radius)
	{
		bool radiusIsParametric = true;
		return itk::FlatStructuringElement<3>::Ball(radius, radiusIsParametric);
	}

	enum eOperation { kErode, kDilate, kClose, kOpen };

	template<typename T>
	itk::Image<unsigned char,3>::Pointer MorphologicalOperation(
		typename itk::SliceContiguousImage<T>::Pointer input, itk::FlatStructuringElement<3> structuringElement, eOperation operation, size_t startslice, size_t endslice)
	{
		typedef itk::SliceContiguousImage<T>	input_image_type;
		typedef itk::Image<unsigned char, 3>	image_type;
		typedef itk::FlatStructuringElement<3>	kernel_type;

		unsigned char foreground_value = 255;

		auto threshold = itk::BinaryThresholdImageFilter<input_image_type, image_type>::New();
		threshold->SetInput(input);
		threshold->SetLowerThreshold(0.5f); // background is '0'
		threshold->SetInsideValue(foreground_value);

		std::vector<itk::ImageSource<image_type>::Pointer> filters;
		if (operation == eOperation::kErode || operation == eOperation::kOpen)
		{
			auto erode = itk::BinaryErodeImageFilter<image_type, image_type, kernel_type>::New();
			erode->SetInput(threshold->GetOutput());
			erode->SetKernel(structuringElement);
			erode->SetErodeValue(foreground_value);
			erode->SetBackgroundValue(0);
			filters.push_back(itk::ImageSource<image_type>::Pointer(erode));

			if (operation == kOpen)
			{
				auto dilate = itk::BinaryDilateImageFilter<image_type, image_type, kernel_type>::New();
				dilate->SetInput(erode->GetOutput());
				dilate->SetKernel(structuringElement);
				dilate->SetDilateValue(foreground_value);
				filters.push_back(itk::ImageSource<image_type>::Pointer(dilate));
			}
		}
		else
		{
			auto dilate = itk::BinaryDilateImageFilter<image_type, image_type, kernel_type>::New();
			dilate->SetInput(threshold->GetOutput());
			dilate->SetKernel(structuringElement);
			dilate->SetDilateValue(foreground_value);
			filters.push_back(itk::ImageSource<image_type>::Pointer(dilate));

			if (operation == kClose)
			{
				auto erode = itk::BinaryErodeImageFilter<image_type, image_type, kernel_type>::New();
				erode->SetInput(erode->GetOutput());
				erode->SetKernel(structuringElement);
				erode->SetErodeValue(foreground_value);
				erode->SetBackgroundValue(0);
				filters.push_back(itk::ImageSource<image_type>::Pointer(erode));
			}
		}
		
		auto start = input->GetLargestPossibleRegion().GetIndex();
		start[2] = startslice;

		auto size = input->GetLargestPossibleRegion().GetSize();
		size[2] = endslice - startslice;

		filters.back()->GetOutput()->SetRequestedRegion(image_type::RegionType(start, size));
		filters.back()->Update();
		return filters.back()->GetOutput();
	}

	template<typename T>
	bool Paste(itk::Image<unsigned char, 3>* source, itk::SliceContiguousImage<T>* destination, size_t startslice, size_t endslice)
	{
		if (source->GetLargestPossibleRegion() == destination->GetLargestPossibleRegion())
		{
			// copy only startslice-endslice from source
			auto start = source->GetLargestPossibleRegion().GetIndex();
			start[2] = startslice;

			auto size = source->GetLargestPossibleRegion().GetSize();
			size[2] = endslice - startslice;

			// copy active slices into destination, starting at startslice
			auto active_region = itk::ImageBase<3>::RegionType(start, size);

			typedef itk::ImageRegionIterator<itk::SliceContiguousImage<T>> slice_image_iterator;
			typedef itk::ImageRegionIterator<itk::Image<unsigned char, 3>> image_iterator;
			image_iterator sit(source, active_region);
			slice_image_iterator dit(destination, active_region);

			for (sit.GoToBegin(), dit.GoToBegin(); !sit.IsAtEnd() && !dit.IsAtEnd(); ++sit, ++dit)
			{
				dit.Set(sit.Get());
			}
			return true;
		}
		return false;
	}
}