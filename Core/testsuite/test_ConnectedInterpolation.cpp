/*
* Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#include <boost/test/unit_test.hpp>

#include "../ConnectedShapeBasedInterpolation.h"

#include <itkMinimumMaximumImageCalculator.h>

#include <string>

namespace iseg {

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(Core_suite);
BOOST_AUTO_TEST_SUITE(Interpolation_suite);

// TestRunner.exe --run_test=iSeg_suite/Interpolation_suite/Interpolation_test --log_level=message
BOOST_AUTO_TEST_CASE(Interpolation_test)
{
	using image_type = itk::Image<float, 2>;

	image_type::IndexType start;
	start.Fill(0);
	image_type::SizeType  size;
	size.Fill(512);
	image_type::RegionType region(start, size);

	auto slice1 = image_type::New();
	slice1->SetRegions(region);
	slice1->Allocate();
	slice1->FillBuffer(0);
	itk::Index<2> idx = {200, 200};
	slice1->SetPixel(idx, 1);

	auto slice2 = image_type::New();
	slice2->SetRegions(region);
	slice2->Allocate();
	slice2->FillBuffer(0);
	idx[0] = 250;
	slice2->SetPixel(idx, 1);

	ConnectedShapeBasedInterpolation myinterpolator;
	auto slices = myinterpolator.interpolate(slice1, slice2, 2);

	for (auto slice: slices)
	{
		auto calc = itk::MinimumMaximumImageCalculator<image_type>::New();
  		calc->SetImage(slice);
		calc->Compute();
		BOOST_CHECK_EQUAL(calc->GetMaximum(), 255);
		BOOST_CHECK_GT(calc->GetIndexOfMaximum()[0], 210);
		BOOST_CHECK_LT(calc->GetIndexOfMaximum()[0], 240);
		BOOST_CHECK_EQUAL(calc->GetIndexOfMaximum()[1], 200);
	}
}

// --run_test=iSeg_suite/Interpolation_suite/ResampleFilterBug --log_level=message
BOOST_AUTO_TEST_CASE(ResampleFilterBug)
{
	using image_type = itk::Image<float,3>;

	image_type::IndexType input_start = {0, 0, 0};
	image_type::SizeType input_size = {512, 512, 3};
	image_type::RegionType input_region(input_start, input_size);
	image_type::SpacingType input_spacing;
	input_spacing.Fill(1.0);
	image_type::PointType input_origin;
	input_origin.Fill(0.0);

	auto input = image_type::New();
	input->SetSpacing(input_spacing);
	input->SetOrigin(input_origin);
	input->SetRegions(input_region);
	input->Allocate();
	input->FillBuffer(3.f);
	input->SetObjectName("My Input");

	auto id = itk::IdentityTransform<double, 3>::New();
	auto origin = input_origin;
	auto spacing = input_spacing;
	auto size = input_region.GetSize();
	size[2] = 2;
	spacing[2] = 1.0 / (size[2] + 1);
	origin[2] = spacing[2];

	auto interpolator = itk::LinearInterpolateImageFunction<image_type, double>::New();

	auto resample_filter = itk::ResampleImageFilter<image_type, image_type>::New();
	resample_filter->SetTransform(id.GetPointer());
	resample_filter->SetInterpolator(interpolator);
	resample_filter->SetOutputOrigin(origin);
	resample_filter->SetOutputSpacing(spacing);
	resample_filter->SetOutputStartIndex(input_region.GetIndex());
	resample_filter->SetSize(size);
	resample_filter->SetInput(input);
	resample_filter->Update();
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

} // namespace iseg
