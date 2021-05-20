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

#include "Transform.h"

#include <itkImage.h>
#include <itkSliceContiguousImage.h>

#include <vector>

namespace iseg {

inline void copyToITK(const Transform& transform, typename itk::Point<itk::SpacePrecisionType, 3>& origin, typename itk::Matrix<itk::SpacePrecisionType, 3, 3>& direction)
{
	transform.GetRotation(direction);
	transform.GetOffset(origin);
}

template<typename T>
typename itk::Image<T, 3>::Pointer allocateImage(const unsigned dimensions[3], unsigned start_slice, unsigned end_slice, const Vec3& spacing, const Transform& transform)
{
	using image_type = itk::Image<T, 3>;

	typename image_type::IndexType start;
	start[0] = 0;						// first index on X
	start[1] = 0;						// first index on Y
	start[2] = start_slice; // first index on Z
	typename image_type::SizeType size;
	size[0] = dimensions[0]; // size along X
	size[1] = dimensions[1]; // size along Y
	size[2] = dimensions[2]; // size along Z
	typename image_type::RegionType region;
	region.SetSize(size);
	region.SetIndex(start);

	typename image_type::PointType origin;
	typename image_type::DirectionType direction;
	toITK(transform, origin, direction);

	auto image = image_type::New();
	image->SetRegions(region);
	image->SetSpacing(spacing);
	image->SetOrigin(origin);
	image->SetDirection(direction);
	image->Allocate();
	return image;
}

template<typename T>
typename itk::Image<T, 3>::Pointer copyToITK(const std::vector<T*>& all_slices, const unsigned dimensions[3], unsigned start_slice, unsigned end_slice, const Vec3& spacing, const Transform& transform)
{
	auto image = allocateImage<T>(dimensions, start_slice, end_slice, spacing, transform);

	typename itk::Image<T, 3>::IndexType pi;
	auto size = image->GetLargestPossibleRegion().GetSize();

	// \todo copy using std::copy per slice
	for (unsigned z = 0; z < size[2]; z++)
	{
		const T* slice = all_slices[start_slice + z];
		pi.SetElement(2, z);
		size_t idx = 0;
		for (unsigned y = 0; y < size[1]; y++)
		{
			pi.SetElement(1, y);
			for (unsigned x = 0; x < size[0]; x++, idx++)
			{
				pi.SetElement(0, x);
				image->SetPixel(pi, slice[idx]);
			}
		}
	}
	return image;
}

template<typename T>
typename itk::SliceContiguousImage<T>::Pointer wrapToITK(const std::vector<T*>& all_slices, const unsigned dims[3], unsigned start_slice, unsigned end_slice, const Vec3& spacing, const Transform& transform)
{
	using slice_contiguous_image_type = itk::SliceContiguousImage<T>;
	using region_type = typename slice_contiguous_image_type::RegionType;

	typename slice_contiguous_image_type::IndexType start;
	start.Fill(0);

	typename slice_contiguous_image_type::SizeType size;
	size[0] = dims[0];
	size[1] = dims[1];
	size[2] = dims[2];

	region_type total_region(start, size);

	assert(end_slice > start_slice);
	if (end_slice > start_slice)
	{
		// \note deactivated, because instead I set the extent above to start-end
		//	// correct transform if we are exporting active slices
		//	int cropping[] = { 0,0,handler->start_slice() };
		//	transform.paddingUpdateTransform(cropping, spacing.v);

		start[2] = start_slice;
		size[2] = end_slice - start_slice;
	}

	region_type buffered_region(start, size);

	itk::Point<itk::SpacePrecisionType, 3> origin;
	transform.GetOffset(origin);

	itk::Matrix<itk::SpacePrecisionType, 3, 3> direction;
	transform.GetRotation(direction);

	auto image = slice_contiguous_image_type::New();
	image->SetOrigin(origin);
	image->SetSpacing(spacing.v);
	image->SetDirection(direction);
	image->SetLargestPossibleRegion(total_region);
	image->SetBufferedRegion(buffered_region);
	image->SetRequestedRegion(buffered_region);

	// Set slice pointers
	std::vector<T*> slices;
	for (size_t i = 0; i < size[2]; ++i)
	{
		slices.push_back(all_slices[i + start[2]]);
	}
	auto container = slice_contiguous_image_type::PixelContainer::New();
	container->SetImportPointersForSlices(slices, size[0] * size[1], false);
	image->SetPixelContainer(container);

	return image;
}

} // namespace iseg
