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

#if defined(WIN32)
#	if defined(iSegCore_EXPORTS)
#		define ISEG_CORE_API __declspec(dllexport)
#	else
#		define ISEG_CORE_API __declspec(dllimport)
#	endif
#else
#	define ISEG_CORE_API
#endif

#if defined(iSegCore_EXPORTS)
#	define iSegCore_TEMPLATE
#else
#	define iSegCore_TEMPLATE extern
#endif

#if defined(WIN32)
// Disable needs to have dll-interface to be used by clients warning
#	pragma warning(disable : 4251)
#endif
