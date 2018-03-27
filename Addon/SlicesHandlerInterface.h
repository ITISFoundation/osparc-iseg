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

#include "AddonApi.h"

#include "itkSliceContiguousImage.h"
#include <itkImage.h> // BL TODO get rid of this

namespace iseg {

class CSliceHandlerInterface
{
public:
	virtual unsigned short return_startslice() = 0;
	virtual unsigned short return_endslice() = 0;
	virtual unsigned short get_activeslice() = 0;

	virtual void GetITKImage(itk::Image<float, 3> *) = 0;
	virtual void GetITKImageFB(itk::Image<float, 3> *) = 0;
	virtual void GetITKImageGM(itk::Image<float, 3> *) = 0;
	virtual void ModifyWork(itk::Image<unsigned int, 3> *output) = 0;
	virtual void GetITKImage(itk::Image<float, 3> *, int, int) = 0;
	virtual void GetITKImageFB(itk::Image<float, 3> *, int, int) = 0;
	virtual void GetITKImageGM(itk::Image<float, 3> *, int, int) = 0;
	virtual void ModifyWork(itk::Image<unsigned int, 3> *output, int, int) = 0;
	virtual void ModifyWorkFloat(itk::Image<float, 3> *output) = 0;
	virtual void ModifyWorkFloat(itk::Image<float, 3> *output, int, int) = 0;
	virtual void GetSeed(itk::Image<float, 3>::IndexType *) = 0;

	enum eImageType { kSource, kTarget };
	virtual itk::SliceContiguousImage<float>::Pointer
			GetImage(eImageType type, bool active_slices) = 0;
	virtual itk::SliceContiguousImage<unsigned short>::Pointer
			GetTissues(bool active_slices) = 0;
};

} // namespace iseg
