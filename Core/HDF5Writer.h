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

#include "iSegCore.h"

#include <string>
#include <vector>

namespace iseg {

class iSegCore_API HDF5Writer
{
public:
	typedef unsigned long long size_type;
	typedef int hid_type;
	HDF5Writer();
	~HDF5Writer();

	static std::vector<std::string> tokenize(std::string&, char = '/');
	int createGroup(const std::string&);
	int open(const std::string&, const std::string& = "overwrite");
	int open(const char* fn, const std::string& = "overwrite");
	int close();
	int write(const double*, const std::vector<size_type>&, const std::string&);
	int write(const float*, const std::vector<size_type>&, const std::string&);
	int write(const int*, const std::vector<size_type>&, const std::string&);
	int write(const unsigned int*, const std::vector<size_type>&,
			  const std::string&);
	int write(const long*, const std::vector<size_type>&, const std::string&);
	int write(const unsigned char*, const std::vector<size_type>&,
			  const std::string&);
	int write(const unsigned short*, const std::vector<size_type>&,
			  const std::string&);
	int write(const std::vector<int>&, const std::string&);
	int write(const std::vector<unsigned int>&, const std::string&);
	int write(const std::vector<long>&, const std::string&);
	int write(const std::vector<double>&, const std::string&);
	int write(const std::vector<float>&, const std::string&);
	int write(const std::vector<unsigned char>&, const std::string&);
	int write(const std::vector<unsigned short>&, const std::string&);
	int write_attribute(const std::string&, const std::string&);
	int write(float** const slices, size_type num_slices, size_type slice_size,
			  const std::string& name, size_t offset = 0);
	int write(unsigned short** const slices, size_type num_slices,
			  size_type slice_size, const std::string& name, size_t offset = 0);
	int flush();

	int compression;
	bool loud;
	std::string ordering;
	std::vector<int> chunkSize;

private:
	int writeData(const void*, const std::string&,
				  const std::vector<size_type>&, const std::string&) const;
	hid_type file;
	size_type bufsize;
};

} // namespace iseg
