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

#include "SlicesHandlerInterface.h"
#include "Transform.h"
#include "Vec3.h"
#include "iSegData.h"

#include <itkImage.h>
#include <itkSliceContiguousImage.h>

namespace iseg {

class ISEG_DATA_API SlicesHandlerITKInterface
{
public:
	using pixel_type = SlicesHandlerInterface::pixel_type;
	using tissue_type = SlicesHandlerInterface::tissue_type;

	using image_ref_type = itk::SliceContiguousImage<pixel_type>;
	using tissues_ref_type = itk::SliceContiguousImage<tissue_type>;

	SlicesHandlerITKInterface(SlicesHandlerInterface* sh) : _handler(sh) {}

	template<typename T>
	static typename itk::SliceContiguousImage<T>::Pointer GetITKView(const std::vector<T*>& all_slices, size_t start_slice, size_t end_slice, const SlicesHandlerInterface* handler);

	itk::SliceContiguousImage<pixel_type>::Pointer GetSource(bool active_slices);
	itk::SliceContiguousImage<pixel_type>::Pointer GetSource(size_t start_slice, size_t end_slice);
	itk::SliceContiguousImage<pixel_type>::Pointer GetTarget(bool active_slices);
	itk::SliceContiguousImage<pixel_type>::Pointer GetTarget(size_t start_slice, size_t end_slice);
	itk::SliceContiguousImage<tissue_type>::Pointer GetTissues(bool active_slices);
	itk::SliceContiguousImage<tissue_type>::Pointer GetTissues(size_t start_slice, size_t end_slice);

	enum { kActiveSlice = -1 };
	itk::Image<pixel_type, 2>::Pointer GetSourceSlice(int slice = kActiveSlice);
	itk::Image<pixel_type, 2>::Pointer GetTargetSlice(int slice = kActiveSlice);
	itk::Image<tissue_type, 2>::Pointer GetTissuesSlice(int slice = kActiveSlice);

	/// Get region defined by active slices
	itk::ImageRegion<3> GetActiveRegion() const;

	enum eImageType {
		kSource,
		kTarget
	};

	itk::Image<pixel_type, 3>::Pointer GetImageDeprecated(eImageType type, bool active_slices);
	itk::Image<tissue_type, 3>::Pointer GetTissuesDeprecated(bool active_slices);

private:
	SlicesHandlerInterface* _handler;
};

template<typename T>
typename itk::SliceContiguousImage<T>::Pointer
		SlicesHandlerITKInterface::GetITKView(const std::vector<T*>& all_slices, size_t start_slice, size_t end_slice, const SlicesHandlerInterface* handler)
{
	using SliceContiguousImageType = itk::SliceContiguousImage<T>;

	typename SliceContiguousImageType::IndexType start;
	start.Fill(0);

	typename SliceContiguousImageType::SizeType size;
	size[0] = handler->width();
	size[1] = handler->height();
	size[2] = handler->num_slices();

	typename SliceContiguousImageType::RegionType region(start, size);

	auto spacing = handler->spacing();
	auto transform = handler->transform();

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

	itk::Point<itk::SpacePrecisionType, 3> origin;
	transform.getOffset(origin);

	itk::Matrix<itk::SpacePrecisionType, 3, 3> direction;
	transform.getRotation(direction);

	auto image = SliceContiguousImageType::New();
	image->SetSpacing(spacing.v);
	image->SetOrigin(origin);
	image->SetDirection(direction);
	image->SetRegions(region);
	image->Allocate();

	// Set slice pointers
	std::vector<T*> slices = all_slices;
	auto container = SliceContiguousImageType::PixelContainer::New();
	container->SetImportPointersForSlices(slices, size[0] * size[1], false);
	image->SetPixelContainer(container);

	return image;
}

} // namespace iseg
