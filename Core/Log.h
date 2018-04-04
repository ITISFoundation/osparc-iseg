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

#include "iSegCore.h"

#include <fstream>
#include <string>

namespace iseg {

static std::ofstream flog;

iSegCore_API void quit(const std::string& msg = "", const int code = EXIT_SUCCESS);
iSegCore_API void error(const std::string& msg = "", const int code = EXIT_FAILURE);
iSegCore_API void warning(const std::string& msg);
iSegCore_API bool interceptOutput(const std::string& logname);

} // namespace iseg
