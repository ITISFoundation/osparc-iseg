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

#include "LogApi.h"

#include <sstream>

/** \brief Macros used to log a message
*/
#define ISEG_DEBUG_MSG(msg) ::iseg::Log::Debug(msg)
#define ISEG_INFO_MSG(msg) ::iseg::Log::Info(msg)
#define ISEG_WARNING_MSG(msg) ::iseg::Log::Warning(msg)
#define ISEG_ERROR_MSG(msg) ::iseg::Log::Error(msg)

#define ISEG_DEBUG(args)                  \
	{                                       \
		std::stringstream ss;                 \
		ss << args;                           \
		::iseg::Log::Debug(ss.str().c_str()); \
	}
#define ISEG_INFO(args)                  \
	{                                      \
		std::stringstream ss;                \
		ss << args;                          \
		::iseg::Log::Info(ss.str().c_str()); \
	}
#define ISEG_WARNING(args)                  \
	{                                         \
		std::stringstream ss;                   \
		ss << args;                             \
		::iseg::Log::Warning(ss.str().c_str()); \
	}
#define ISEG_ERROR(args)                  \
	{                                       \
		std::stringstream ss;                 \
		ss << args;                           \
		::iseg::Log::Error(ss.str().c_str()); \
	}
