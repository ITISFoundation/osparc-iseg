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

#include "iSegData.h"

#include <string>

namespace iseg {

class ISEG_DATA_API Log
{
public:
	static void debug(const char* format, ...);
	template<class C, typename ...Args>
	static void debug(const std::basic_string<C>& format, Args...args) { debug(format.c_str(), args...); }

	static void info(const char* format, ...);
	template<class C, typename ...Args>
	static void info(const std::basic_string<C>& format, Args...args) { info(format.c_str(), args...); }

	static void warning(const char* format, ...);
	template<class C, typename ...Args>
	static void warning(const std::basic_string<C>& format, Args...args) { warning(format.c_str(), args...); }

	static void error(const char* format, ...);
	template<class C, typename ...Args>
	static void error(const std::basic_string<C>& format, Args...args) { error(format.c_str(), args...); }

	static void note(const std::string& channel, const char* format, ...);

	static bool attach_log_file(std::string const & logFileName, bool createNewFile = true);
	static bool attach_console(bool on);
	static bool intercept_cerr();

	static void close_log_file();

	static std::ostream& log_stream();
};

ISEG_DATA_API void init_logging(const std::string& log_file_name, bool print_to_clog = true, bool print_debug_log = false, bool intercept_cerr = true);
}