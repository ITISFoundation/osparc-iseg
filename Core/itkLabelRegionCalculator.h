/// \file itkFixTopologyCarveOutside.h
///
/// \author Bryn Lloyd
/// \copyright 2020, IT'IS Foundation

#pragma once

#include <itkImage.h>
#include <itkImageRegionConstIterator.h>

#include <climits>
#include <vector>

namespace itk {
template<class TLabelImage>
typename TLabelImage::RegionType GetLabelRegion(const TLabelImage* label_input, typename TLabelImage::PixelType selected_label)
{
	itkStaticConstMacro(imageDimension, unsigned int, TLabelImage::ImageDimension);
	std::vector<size_t> boundingBox(imageDimension * 2);
	for (unsigned int i = 0; i < imageDimension * 2; i += 2)
	{
		boundingBox[i] = std::numeric_limits<size_t>::max();
		boundingBox[i + 1] = std::numeric_limits<size_t>::lowest();
	}

	// do the work
	auto it = itk::ImageRegionConstIterator<TLabelImage>(label_input, label_input->GetBufferedRegion());
	for (it.GoToBegin(); !it.IsAtEnd(); ++it)
	{
		if (it.Get() == selected_label)
		{
			auto idx = it.GetIndex();
			boundingBox[0] = boundingBox[0] > idx[0] ? idx[0] : boundingBox[0];
			boundingBox[1] = boundingBox[1] < idx[0] ? idx[0] : boundingBox[1];

			boundingBox[2] = boundingBox[2] > idx[1] ? idx[1] : boundingBox[2];
			boundingBox[3] = boundingBox[3] < idx[1] ? idx[1] : boundingBox[3];

			boundingBox[4] = boundingBox[4] > idx[2] ? idx[2] : boundingBox[4];
			boundingBox[5] = boundingBox[5] < idx[2] ? idx[2] : boundingBox[5];
		}
	}

	if (boundingBox[0] <= boundingBox[1])
	{
		typename TLabelImage::IndexType index;
		typename TLabelImage::SizeType size;

		for (unsigned int i = 0; i < imageDimension; ++i)
		{
			index[i] = boundingBox[2 * i];
			size[i] = boundingBox[2 * i + 1] - boundingBox[2 * i] + 1;
		}
		return typename TLabelImage::RegionType(index, size);
	}
	return typename TLabelImage::RegionType();
}
} // namespace itk
