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

#include "iSegData.h"
#include "SlicesHandlerInterface.h"

#include <itkImage.h> // BL TODO get rid of this
#include <itkSliceContiguousImage.h>

namespace iseg {

class ISEG_DATA_API SliceHandlerItkWrapper
{
public:
	using pixel_type = SliceHandlerInterface::pixel_type;
	using tissue_type = SliceHandlerInterface::tissue_type;

	using image_ref_type = itk::SliceContiguousImage<pixel_type>;
	using tissues_ref_type = itk::SliceContiguousImage<tissue_type>;

	SliceHandlerItkWrapper(SliceHandlerInterface* sh) : _handler(sh) {}

	enum eImageType {
		kSource,
		kTarget
	};
	itk::SliceContiguousImage<pixel_type>::Pointer GetImage(eImageType type, bool active_slices);
	itk::SliceContiguousImage<tissue_type>::Pointer GetTissues(bool active_slices);

	itk::Image<pixel_type, 2>::Pointer GetImageSlice(eImageType type);
	itk::Image<tissue_type, 2>::Pointer GetTissuesSlice();

	/// Get region defined by active slices
	itk::ImageRegion<3> GetActiveRegion() const;

	itk::Image<pixel_type, 3>::Pointer GetImageDeprecated(eImageType type, bool active_slices);
	itk::Image<tissue_type, 3>::Pointer GetTissuesDeprecated(bool active_slices);

private:
	SliceHandlerInterface* _handler;
};

} // namespace iseg
