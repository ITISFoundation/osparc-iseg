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

#include "../itkWeightedDijkstraImageFilter.h"

namespace iseg {

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(TraceTubesWidget_suite);

// TestRunner.exe --run_test=iSeg_suite/TraceTubesWidget_suite/ROI_test --log_level=message
BOOST_AUTO_TEST_CASE(ROI_test)
{
	using image_type = itk::Image<float, 3>;
	auto img = image_type::New();
	{
		itk::Index<3> idx = {0, 0, 0};
		itk::Size<3> size = {10, 20, 30};
		itk::ImageRegion<3> region(idx, size);
		img->SetRegions(region);
	}
	img->Allocate(true);

	{
		itk::Index<3> idx = {0, 5, 7};
		itk::Size<3> size = {3, 10, 10};
		itk::ImageRegion<3> region(idx, size);

		auto output = image_type::New();
		output->Graft(img);
		output->SetLargestPossibleRegion(img->GetLargestPossibleRegion());
		output->SetBufferedRegion(region);

		BOOST_CHECK_EQUAL(output->GetBufferedRegion().GetIndex()[0], 0);
		BOOST_CHECK_EQUAL(output->GetBufferedRegion().GetIndex()[1], 5);
		BOOST_CHECK_EQUAL(output->GetBufferedRegion().GetIndex()[2], 7);

		BOOST_CHECK_EQUAL(output->GetBufferedRegion().GetNumberOfPixels(), size[0] * size[1] * size[2]);
	}
}

// TestRunner.exe --run_test=iSeg_suite/TraceTubesWidget_suite --log_level=message
BOOST_AUTO_TEST_CASE(DijkstraMetric_test)
{
	using image_type = itk::Image<float, 3>;
	using metric_type = itk::MyMetric<image_type>;

	{
		metric_type m;

		BOOST_CHECK_CLOSE(0.0, m.ComputeAngle(1.0, 0.0, 0.0, 1.0, 0.0, 0.0), 1e-3);
		BOOST_CHECK_CLOSE(1.0, m.ComputeAngle(1.0, 0.0, 0.0, 0.0, 1.0, -1.0), 1e-3);
		BOOST_CHECK_CLOSE(2.0, m.ComputeAngle(1.0, 0.0, 0.0, -1.0, 0.0, 0.0), 1e-3);
	}

	{
		image_type::IndexType iprev = {0, 0, 0};
		image_type::IndexType i = {1, 0, 0};
		image_type::IndexType j = {2, 0, 0};

		image_type::RegionType region;
		region.SetIndex(iprev);
		region.SetUpperIndex(j);

		auto img = image_type::New();
		img->SetRegions(region);
		img->Allocate();

		metric_type m;
		m.Initialize(img, iprev, j);

		BOOST_CHECK_CLOSE(3 * 0 + 1.0 + 0.0, m.GetEdgeWeight(i, j, iprev), 1e-3);
	}
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

} // namespace iseg
