/*
* Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	static void Debug(const char* format, ...);
	template<class C, typename... Args>
	static void Debug(const std::basic_string<C>& format, Args... args) { debug(format.c_str(), args...); }

	static void Info(const char* format, ...);
	template<class C, typename... Args>
	static void Info(const std::basic_string<C>& format, Args... args) { info(format.c_str(), args...); }

	static void Warning(const char* format, ...);
	template<class C, typename... Args>
	static void Warning(const std::basic_string<C>& format, Args... args) { warning(format.c_str(), args...); }

	static void Error(const char* format, ...);
	template<class C, typename... Args>
	static void Error(const std::basic_string<C>& format, Args... args) { error(format.c_str(), args...); }

	static void Note(const std::string& channel, const char* format, ...);

	static bool AttachLogFile(std::string const& logFileName, bool createNewFile = true);
	static bool AttachConsole(bool on);
	static bool InterceptCerr();

	static void CloseLogFile();

	static std::ostream& LogStream();
};

ISEG_DATA_API void init_logging(const std::string& log_file_name, bool print_to_clog = true, bool print_debug_log = false, bool intercept_cerr = true);
} // namespace iseg