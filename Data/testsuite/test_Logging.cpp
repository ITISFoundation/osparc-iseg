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

namespace iseg {

BOOST_AUTO_TEST_SUITE(iSeg_suite);
BOOST_AUTO_TEST_SUITE(Logger_suite);

// TestRunner.exe --run_test=iSeg_suite/Logger_suite --log_level=all
BOOST_AUTO_TEST_CASE(Logger_test)
{
	namespace fs = boost::filesystem;

	auto fpath = fs::temp_directory_path() / fs::path("_temp.log");

	auto file_sink = init_logging(fpath.string(), eSeverityLevel::debug);

	ISEG_DEBUG() << "This is a debug message";
	ISEG_INFO() << "This is an info message";
	ISEG_WARNING() << "This is a warning message";
	ISEG_ERROR() << "This is an error message";

	logging::core::get()->remove_sink(file_sink);
	file_sink.reset();
	
	BOOST_REQUIRE(fs::exists(fpath.string()));

	// check number of lines
	std::ifstream file(fpath.string().c_str());
	auto n = std::count(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>(), '\n');
	BOOST_CHECK_EQUAL(n, 4);
	file.close();

	// cleanup
	fs::remove(fpath.string());
}

BOOST_AUTO_TEST_SUITE_END();
BOOST_AUTO_TEST_SUITE_END();

} // namespace iseg
