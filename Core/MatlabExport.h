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

#include "Types.h"
#include "iSegCore.h"

namespace iseg { namespace matexport {

iSegCore_API bool print_mat(const char* filename, float* matrix, int nx, int ny,
							int nz, const char* comment, int commentlength,
							const char* varname, int varnamelength,
							bool complex = false);
iSegCore_API bool print_matslices(const char* filename, float** matrix, int nx,
								  int ny, int nz, const char* comment,
								  int commentlength, const char* varname,
								  int varnamelength, bool complex = false);
iSegCore_API bool print_mat(const char* filename, unsigned char* matrix, int nx,
							int ny, int nz, const char* comment, int commentlength,
							const char* varname, int varnamelength);
iSegCore_API bool print_matslices(const char* filename, unsigned char** matrix,
								  int nx, int ny, int nz, const char* comment,
								  int commentlength, const char* varname,
								  int varnamelength);

#ifdef TISSUES_SIZE_TYPEDEF

iSegCore_API bool print_mat(const char* filename, tissues_size_t* matrix,
							int nx, int ny, int nz, char* comment,
							int commentlength, char* varname,
							int varnamelength);
iSegCore_API bool print_matslices(const char* filename, tissues_size_t** matrix,
								  int nx, int ny, int nz, const char* comment,
								  int commentlength, const char* varname,
								  int varnamelength);

#endif // TISSUES_SIZE_TYPEDEF

}} // namespace iseg::matexport
