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


/** \brief Functions used to log a message
*/
ISEG_DATA_API void debug(const std::string& msg);
ISEG_DATA_API void info(const std::string& msg);
ISEG_DATA_API void warning(const std::string& msg);
ISEG_DATA_API void error(const std::string& msg);

}