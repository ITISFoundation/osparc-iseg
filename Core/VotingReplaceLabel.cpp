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

#include "VotingReplaceLabel.h"

#include "itkLabelVotingBinaryImageFilter.h"

#include "../Data/ItkUtils.h"
#include "../Data/SliceHandlerItkWrapper.h"

#include <itkCastImageFilter.h>

namespace iseg {

size_t VotingReplaceLabel(SliceHandlerInterface* handler,
		tissues_size_t foreground,
		tissues_size_t background,
		std::array<unsigned int, 3> iradius,
		unsigned int majority_threshold,
		unsigned int max_iterations)
{
	using image_type = itk::Image<tissues_size_t, 3>;
	using voting_filter_type = itk::LabelVotingBinaryImageFilter<image_type, image_type>;

	SliceHandlerItkWrapper itkhandler(handler);
	auto tissues = itkhandler.GetTissues(true);

	auto cast = itk::CastImageFilter<SliceHandlerItkWrapper::tissues_ref_type, image_type>::New();
	cast->SetInput(tissues);
	cast->Update();
	auto input = cast->GetOutput();

	voting_filter_type::InputSizeType radius;
	radius[0] = iradius[0];
	radius[1] = iradius[1];
	radius[2] = iradius[2];

	auto voting_filter = voting_filter_type::New();
	voting_filter->SetForegroundValue(foreground);
	voting_filter->SetBackgroundValue(background);
	voting_filter->SetMajorityThreshold(majority_threshold);
	voting_filter->SetRadius(radius);

	image_type::Pointer output;

	// repeat relabeling
	unsigned int iter = 0;
	while (iter < max_iterations)
	{
		voting_filter->SetInput(input);
		voting_filter->Update();

		iter++;

		const auto num_changed = voting_filter->GetNumberOfPixelsChanged();

		output = voting_filter->GetOutput();
		output->DisconnectPipeline();
		input = output;
		if (num_changed == 0)
		{
			break;
		}
	}

	// count how many were not relabeled
	size_t number_remaining = 0;
	itk::ImageRegionConstIterator<image_type> image_it(output, output->GetLargestPossibleRegion());
	for (image_it.GoToBegin(); !image_it.IsAtEnd(); ++image_it)
	{
		if (image_it.Get() == foreground)
		{
			number_remaining += 1;
		}
	}

	// paste into output
	iseg::Paste<image_type, SliceHandlerItkWrapper::tissues_ref_type>(output, tissues);

	return number_remaining;
}

} // namespace iseg
