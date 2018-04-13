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

#include "itkSliceContiguousImage.h"

#include <itkImage.h>

namespace iseg {

template<typename TInputPixel, typename TOutputPixel>
bool Paste(itk::Image<TInputPixel, 3>* source, itk::SliceContiguousImage<TOutputPixel>* destination,
		size_t startslice, size_t endslice)
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

		typedef itk::ImageRegionIterator<itk::Image<TInputPixel, 3>> image_iterator;
		typedef itk::ImageRegionIterator<itk::SliceContiguousImage<TOutputPixel>> slice_image_iterator;
		image_iterator sit(source, active_region);
		slice_image_iterator dit(destination, active_region);

		for (sit.GoToBegin(), dit.GoToBegin(); !sit.IsAtEnd() && !dit.IsAtEnd(); ++sit, ++dit)
		{
			dit.Set(static_cast<TOutputPixel>(sit.Get()));
		}
		return true;
	}
	return false;
}

} // namespace iseg
