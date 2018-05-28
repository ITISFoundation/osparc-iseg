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
