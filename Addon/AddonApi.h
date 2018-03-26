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

#ifdef Addon_EXPORTS
#	define ADDON_API __declspec(dllexport)
#else 
#	define ADDON_API __declspec(dllimport)
#endif
