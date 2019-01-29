/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "SmoothTissues.h"

#include "../Data/ItkUtils.h"
#include "../Data/SliceHandlerItkWrapper.h"

#include <itkCastImageFilter.h>

namespace iseg {

size_t SmoothTissues(SliceHandlerInterface* handler, size_t start_slice, size_t end_slice, double sigma, bool smooth3d)
{
	using image_type = itk::Image<tissues_size_t, 3>;

	SliceHandlerItkWrapper itkhandler(handler);
	auto tissues = itkhandler.GetTissues(true);

	auto cast = itk::CastImageFilter<SliceHandlerItkWrapper::tissues_ref_type, image_type>::New();
	cast->SetInput(tissues);
	cast->Update();
	auto input = cast->GetOutput();

	//auto is_locked = handler->tissue
	// TODO

	// paste into output
	//iseg::Paste<image_type, SliceHandlerItkWrapper::tissues_ref_type>(output, tissues);
	//
	//return number_remaining;
	throw 1;
}

} // namespace iseg
