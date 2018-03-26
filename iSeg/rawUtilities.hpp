/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef RAW_UTILITIES
#define RAW_UTILITIES

#ifdef GPUMC_EXPORTS
# define GPUMC_API __declspec(dllexport)
#else
# define GPUMC_API __declspec(dllimport)
#endif

#include <fstream>

GPUMC_API unsigned char * readRawFile(const char *, int, int, int, int, int, int);

#endif
