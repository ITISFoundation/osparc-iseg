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
template<class TLabelImage, typename TPredicate>
typename TLabelImage::RegionType GetSelectedRegion(const TLabelImage* label_input, const TPredicate& is_selected)
{
	itkStaticConstMacro(imageDimension, unsigned int, TLabelImage::ImageDimension);
	using index_t = ::itk::IndexValueType;

	std::vector<index_t> boundingBox(imageDimension * 2);
	for (unsigned int i = 0; i < imageDimension * 2; i += 2)
	{
		boundingBox[i] = std::numeric_limits<index_t>::max();
		boundingBox[i + 1] = std::numeric_limits<index_t>::lowest();
	}

	// do the work
	auto it = itk::ImageRegionConstIterator<TLabelImage>(label_input, label_input->GetBufferedRegion());
	for (it.GoToBegin(); !it.IsAtEnd(); ++it)
	{
		if (is_selected(it.Get()))
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

template<class TLabelImage>
typename TLabelImage::RegionType GetLabelRegion(const TLabelImage* label_input, typename TLabelImage::PixelType selected_label)
{
	return GetSelectedRegion(label_input, [&](typename TLabelImage::PixelType p) { return p == selected_label; });
}

} // namespace itk
