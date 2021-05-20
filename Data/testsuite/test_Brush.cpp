/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <boost/test/unit_test.hpp>

#include "../Brush.h"

namespace iseg {

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(Brush_suite);
// TestRunner.exe --run_test=iSeg_suite/Brush_suite --log_level=message

BOOST_AUTO_TEST_CASE(brush_test)
{
	unsigned w = 512, h = 512;
	unsigned char bg = 0;
	unsigned char fg = 1;

	// uniform spacing, radius = 0
	{
		Point p = {120, 120};
		float radius = 0.0;
		std::vector<unsigned char> data(w * h, 0);
		brush(data.data(), w, h, 1.f, 1.f, p, radius, true, fg, bg, [](unsigned char) { return false; });
		BOOST_CHECK_EQUAL(std::count(data.begin(), data.end(), fg), 1);
	}
	// uniform spacing, radius = 1
	{
		Point p = {120, 120};
		float radius = 1.0;
		std::vector<unsigned char> data(w * h, 0);
		brush(data.data(), w, h, 1.f, 1.f, p, radius, true, fg, bg, [](unsigned char) { return false; });
		BOOST_CHECK_EQUAL(std::count(data.begin(), data.end(), fg), 5);
		BOOST_TEST_MESSAGE("FG pixels: " << std::count(data.begin(), data.end(), fg));
	}
	// non-uniform spacing, radius = 0
	{
		Point p = {120, 120};
		float radius = 0.0;
		std::vector<unsigned char> data(w * h, 0);
		brush(data.data(), w, h, 1.f, 0.2f, p, radius, true, fg, bg, [](unsigned char) { return false; });
		BOOST_CHECK_EQUAL(std::count(data.begin(), data.end(), fg), 1);
		BOOST_TEST_MESSAGE("FG pixels: " << std::count(data.begin(), data.end(), fg));
	}
	// non-uniform spacing, radius = 1
	{
		Point p = {120, 120};
		float radius = 1.0;
		std::vector<unsigned char> data(w * h, 0);
		brush(data.data(), w, h, 1.f, 0.2f, p, radius, true, fg, bg, [](unsigned char) { return false; });
		BOOST_CHECK_GT(std::count(data.begin(), data.end(), fg), 5);
		BOOST_TEST_MESSAGE("FG pixels: " << std::count(data.begin(), data.end(), fg));
	}
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

} // namespace iseg
