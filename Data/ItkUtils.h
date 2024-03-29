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

#include "Logger.h"

#include "itkSliceContiguousImage.h"

#include <itkImage.h>
#include <itkImageFileWriter.h>
#include <itkImageRegionIterator.h>
#include <itkImageRegionIteratorWithIndex.h>

namespace iseg {

template<typename TInputPixel, typename TOutputPixel>
bool Paste(const itk::Image<TInputPixel, 3>* source, itk::SliceContiguousImage<TOutputPixel>* destination, size_t startslice, size_t endslice)
{
	if (source->GetLargestPossibleRegion() ==
			destination->GetLargestPossibleRegion())
	{
		// copy only startslice-endslice from source
		auto start = source->GetLargestPossibleRegion().GetIndex();
		start[2] = startslice;

		auto size = source->GetLargestPossibleRegion().GetSize();
		size[2] = endslice - startslice;

		// copy active slices into destination, starting at startslice
		auto active_region = itk::ImageBase<3>::RegionType(start, size);

		using image_iterator_type = itk::ImageRegionConstIterator<itk::Image<TInputPixel, 3>>;
		using slice_image_iterator_type = itk::ImageRegionIterator<itk::SliceContiguousImage<TOutputPixel>>;
		image_iterator_type sit(source, active_region);
		slice_image_iterator_type dit(destination, active_region);

		for (sit.GoToBegin(), dit.GoToBegin(); !sit.IsAtEnd() && !dit.IsAtEnd(); ++sit, ++dit)
		{
			dit.Set(static_cast<TOutputPixel>(sit.Get()));
		}
		return true;
	}
	return false;
}

template<class TInputImage, class TOutputImage>
bool Paste(const TInputImage* source, TOutputImage* destination)
{
	using output_pixel_type = typename TOutputImage::PixelType;

	if (source->GetBufferedRegion().GetSize() == destination->GetBufferedRegion().GetSize())
	{
		itk::ImageRegionConstIterator<TInputImage> sit(source, source->GetBufferedRegion());
		itk::ImageRegionIterator<TOutputImage> dit(destination, destination->GetBufferedRegion());

		for (sit.GoToBegin(), dit.GoToBegin(); !sit.IsAtEnd() && !dit.IsAtEnd(); ++sit, ++dit)
		{
			dit.Set(static_cast<output_pixel_type>(sit.Get()));
		}
		return true;
	}
	return false;
}

template<class TInputImage, class TOutputImage>
bool Paste(const TInputImage* source, TOutputImage* destination, const typename TInputImage::RegionType& region)
{
	using output_pixel_type = typename TOutputImage::PixelType;

	if (source->GetBufferedRegion().IsInside(region) && destination->GetBufferedRegion().IsInside(region))
	{
		itk::ImageRegionConstIterator<TInputImage> sit(source, region);
		itk::ImageRegionIterator<TOutputImage> dit(destination, region);

		for (sit.GoToBegin(), dit.GoToBegin(); !sit.IsAtEnd() && !dit.IsAtEnd(); ++sit, ++dit)
		{
			dit.Set(static_cast<output_pixel_type>(sit.Get()));
		}
		return true;
	}
	return false;
}

namespace Functor {

template<class TInput, class TOutput = TInput>
class MapLabels
{
public:
	MapLabels(const std::vector<TOutput>& m = std::vector<TOutput>()) : m_Map(m) {}

	bool operator!=(const MapLabels& other) const
	{
		return !(*this == other);
	}

	bool operator==(const MapLabels& other) const
	{
		return other.m_Map == m_Map;
	}

	inline TOutput operator()(const TInput& A) const
	{
		return static_cast<TOutput>(m_Map.at(A));
	}

	std::vector<TOutput> m_Map;
};

} // namespace Functor

template<class T>
void dump_image(T* img, const std::string& file_name)
{
//#ifdef ENABLE_DUMP_IMAGE
	if (!file_name.empty())
	{
		auto writer = itk::ImageFileWriter<T>::New();
		writer->SetInput(img);
		writer->SetFileName(file_name);
		writer->Update();
	}
//#endif
}

namespace detail {
inline unsigned NextOddInteger(double v)
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
} // namespace detail

template<typename T, unsigned int Dimension>
typename itk::Image<T, Dimension>::Pointer MakeBall(const typename itk::ImageBase<Dimension>::SpacingType& spacing, double radius)
{
	using image_type = typename itk::Image<T, Dimension>;
	using iterator_type = itk::ImageRegionIteratorWithIndex<image_type>;

	typename image_type::SizeType size;
	int center[Dimension];
	for (unsigned int d = 0; d < Dimension; ++d)
	{
		size[d] = detail::NextOddInteger(radius / spacing[d]);
		center[d] = static_cast<int>(size[d] / 2);
	}

	typename image_type::RegionType region;
	region.SetSize(size);

	auto img = image_type::New();
	img->SetRegions(region);
	img->Allocate();
	img->FillBuffer(0);
	img->SetSpacing(spacing);

	double radius2 = radius * radius;

	iterator_type it(img, img->GetLargestPossibleRegion());
	for (it.GoToBegin(); !it.IsAtEnd(); ++it)
	{
		auto idx = it.GetIndex();
		double distance = 0;
		for (unsigned int d = 0; d < Dimension; ++d)
		{
			double dx = spacing[d] * (static_cast<int>(idx[d]) - center[d]);
			distance += dx * dx;
		}

		if (distance < radius2)
		{
			it.Set(1);
		}
	}
	return img;
}

#define SAFE_UPDATE(filter, expr) \
	try                             \
	{                               \
		filter->Update();             \
	}                               \
	catch (itk::ExceptionObject e)  \
	{                               \
		ISEG_ERROR_MSG(e.what());     \
		expr;                         \
	}

} // namespace iseg
