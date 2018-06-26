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

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

} // namespace iseg
