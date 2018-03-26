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
#	if defined(iSegCore_EXPORTS)
#		define iSegCore_API __declspec( dllexport ) 
#		define iSegCore_TEMPLATE 
#	else
#		define iSegCore_API __declspec( dllimport ) 
#		define iSegCore_TEMPLATE extern
#	endif
#else
#	define iSegCore_API
#endif

#if defined(WIN32)
// Disable needs to have dll-interface to be used by clients warning
#  pragma warning( disable : 4251 )
#endif
