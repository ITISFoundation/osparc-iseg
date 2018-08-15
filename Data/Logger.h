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

#include "LogApi.h"

#include <sstream>

/** \brief Macros used to log a message
*/
#define ISEG_DEBUG_MSG(msg)   ::iseg::Log::debug(msg)
#define ISEG_INFO_MSG(msg)    ::iseg::Log::info(msg)
#define ISEG_WARNING_MSG(msg) ::iseg::Log::warning(msg)
#define ISEG_ERROR_MSG(msg)  ::iseg::Log::error(msg)

#define ISEG_DEBUG(args)   { std::stringstream ss; ss << args; ::iseg::Log::debug(ss.str().c_str()); }
#define ISEG_INFO(args)    { std::stringstream ss; ss << args; ::iseg::Log::info(ss.str().c_str()); }
#define ISEG_WARNING(args) { std::stringstream ss; ss << args; ::iseg::Log::warning(ss.str().c_str()); }
#define ISEG_ERROR(args)   { std::stringstream ss; ss << args; ::iseg::Log::error(ss.str().c_str()); }
