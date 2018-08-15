/*
* Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#include "../Logger.h"

#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>

#include <iostream>

namespace iseg {

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(Logger_suite);

// TestRunner.exe --run_test=iSeg_suite/Logger_suite --log_level=message
BOOST_AUTO_TEST_CASE(Logger_test)
{
	namespace fs = boost::filesystem;

	auto fpath = fs::temp_directory_path() / fs::path("_temp.log");

	init_logging(fpath.string(), true);

	ISEG_INFO("This is an info message" << " stream");
	ISEG_WARNING_MSG("This is a warning message");
	ISEG_ERROR_MSG("This is an error message");

	std::cout << "This is a cout message\n"; // not captured
	std::cerr << "This is a cerr message\n";

	Log::close_log_file();

	// check number of lines
	boost::system::error_code ec;
	BOOST_REQUIRE(fs::exists(fpath, ec));
	std::ifstream file(fpath.string().c_str());
	auto n = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
	BOOST_CHECK_EQUAL(n, 5);
	file.close();

	// cleanup
	fs::remove(fpath.string());
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

} // namespace iseg
