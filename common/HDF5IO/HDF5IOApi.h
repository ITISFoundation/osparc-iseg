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
#  if defined(HDF5IO_EXPORTS)
#    define HDF5_EXPORT __declspec( dllexport ) 
#  else
#    define HDF5_EXPORT __declspec( dllimport ) 
#  endif
#  pragma warning( disable : 4251 )
#else
#  define HDF5_EXPORT
#endif
