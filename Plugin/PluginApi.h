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
#	ifdef Plugin_EXPORTS
#		define ISEG_PLUGIN_API __declspec(dllexport)
#	else
#		define ISEG_PLUGIN_API __declspec(dllimport)
#	endif
#else
#	define ISEG_PLUGIN_API
#endif
