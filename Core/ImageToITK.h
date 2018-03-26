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

#include "Transform.h"

#include <itkImage.h>

class ImageToITK
{
public:
	static void copy(const Transform& transform, typename itk::Point<itk::SpacePrecisionType,3>& origin, typename itk::Matrix<itk::SpacePrecisionType,3,3>& direction)
	{
		transform.getRotation(direction);
		transform.getOffset(origin);
	}

	template<typename T>
	static void setup(unsigned width, unsigned height, unsigned startslice, unsigned nrslices, const float spacing[3], const Transform& transform, itk::Image<T, 3>* image)
	{
		typedef itk::Image<T, 3> ImageType;

		ImageType::IndexType start;
		start[0] = 0; // first index on X
		start[1] = 0; // first index on Y
		start[2] = startslice; // first index on Z
		ImageType::SizeType size;
		size[0] = width; // size along X
		size[1] = height; // size along Y
		size[2] = nrslices; // size along Z
		ImageType::RegionType region;
		region.SetSize(size);
		region.SetIndex(start);

		ImageType::PointType origin;
		ImageType::DirectionType direction;
		ImageToITK::copy(transform, origin, direction);

		image->SetRegions(region);
		image->SetSpacing(spacing);
		image->SetOrigin(origin);
		image->SetDirection(direction);
		image->Allocate();
	}

	template<typename T>
	static void copy(const T** data, unsigned width, unsigned height, unsigned startslice, unsigned nrslices, const float spacing[3], const Transform& transform, itk::Image<T, 3>* image)
	{
		setup(width, height, startslice, nrslices, spacing, transform, image);

		typename itk::Image<T, 3>::IndexType pi;
		auto size = image->GetLargestPossibleRegion().GetSize();

		for (unsigned z = 0; z < size[2]; z++)
		{
			const T* slice = data[startslice + z];
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
	}

	template<typename T>
	static typename itk::Image<T, 3>::Pointer copy(const T** data, unsigned width, unsigned height, unsigned startslice, unsigned nrslices, const float spacing[3], const Transform& transform)
	{
		auto image = itk::Image<T, 3>::New();
		copy(data, width, height, startslice, nrslices, spacing, transform, image.GetPointer());
		return image;
	}

};