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

#include "iSegCore.h"

#include <fstream>
#include <string>

namespace iseg {

static std::ofstream flog;

ISEG_CORE_API void quit(const std::string& msg = "", const int code = EXIT_SUCCESS);
ISEG_CORE_API void error(const std::string& msg = "", const int code = EXIT_FAILURE);
ISEG_CORE_API void warning(const std::string& msg);
ISEG_CORE_API bool interceptOutput(const std::string& logname);

} // namespace iseg
