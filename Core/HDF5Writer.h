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

#include "iSegCore.h"

#include <string>
#include <vector>

namespace iseg {

class ISEG_CORE_API HDF5Writer
{
public:
	using size_type = unsigned long long;
	using hid_type = long long;
	HDF5Writer();
	~HDF5Writer();

	static std::vector<std::string> Tokenize(std::string&, char = '/');
	int CreateGroup(const std::string&);
	int Open(const std::string&, const std::string& = "overwrite");
	int Open(const char* fn, const std::string& = "overwrite");
	int Close();
	int Write(const double*, const std::vector<size_type>&, const std::string&);
	int Write(const float*, const std::vector<size_type>&, const std::string&);
	int Write(const int*, const std::vector<size_type>&, const std::string&);
	int Write(const unsigned int*, const std::vector<size_type>&, const std::string&);
	int Write(const long*, const std::vector<size_type>&, const std::string&);
	int Write(const unsigned char*, const std::vector<size_type>&, const std::string&);
	int Write(const unsigned short*, const std::vector<size_type>&, const std::string&);
	int Write(const std::vector<int>&, const std::string&);
	int Write(const std::vector<unsigned int>&, const std::string&);
	int Write(const std::vector<long>&, const std::string&);
	int Write(const std::vector<double>&, const std::string&);
	int Write(const std::vector<float>&, const std::string&);
	int Write(const std::vector<unsigned char>&, const std::string&);
	int Write(const std::vector<unsigned short>&, const std::string&);
	int WriteAttribute(const std::string&, const std::string&);
	int Write(float** const slices, size_type num_slices, size_type slice_size, const std::string& name, size_t offset = 0);
	int Write(unsigned short** const slices, size_type num_slices, size_type slice_size, const std::string& name, size_t offset = 0);
	int Flush();

	int m_Compression;
	bool m_Loud;
	std::string m_Ordering;
	std::vector<int> m_ChunkSize;

private:
	int WriteData(const void*, const std::string&, const std::vector<size_type>&, const std::string&) const;
	hid_type m_File;
	size_type m_Bufsize;
};

} // namespace iseg
