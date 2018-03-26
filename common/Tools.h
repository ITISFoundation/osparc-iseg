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

#if defined(WIN32)
#if defined(Tools_EXPORTS)
#define TOOLS_EXPORT __declspec( dllexport ) 
#else
#define TOOLS_EXPORT __declspec( dllimport ) 
#endif
#else
#define TOOLS_EXPORT
#endif

#include <cstdlib>
#include <vector>
#include <string>
#include <cstdarg>
#include <fstream>

namespace Tools
{
	static std::ofstream flog;

	TOOLS_EXPORT void quit(const std::string& msg="", const int code=EXIT_SUCCESS);
	TOOLS_EXPORT void error(const std::string& msg = "", const int code=EXIT_FAILURE);
	TOOLS_EXPORT void warning(const std::string& msg);
	TOOLS_EXPORT bool interceptOutput(const std::string& logname);

}
