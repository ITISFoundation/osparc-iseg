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

#include <iostream>
#include <string>
#include <vector>

namespace iseg {

class ISEG_CORE_API HDF5Reader
{
public:
	using size_type = unsigned long long;
	using hid_type = long long;
	HDF5Reader();
	~HDF5Reader();
	int open(const std::string&);
	int open(const char* fn);
	int close();
	static bool existsValidHdf5(const std::string&);
	static size_type totalSize(const std::vector<size_type>&);
	std::vector<std::string> getGroupInfo(const std::string&);
	int getDatasetInfo(std::string&, std::vector<size_type>&,
			const std::string&);
	int exists(const std::string&);

	// Description:
	// Decompose a string of the type 'filename.h5:/dataset' into two strings 'filename.h5' and 'dataset'.
	// In case the input string is of type '/dataset' it will return '' and '/dataset'.
	static void decompose(std::string&, std::string&, const std::string&);

	int read(double*, const std::string&);
	int read(float*, const std::string&);
	int read(int*, const std::string&);
	int read(unsigned int*, const std::string&);
	int read(long*, const std::string&);
	int read(unsigned char*, const std::string&);
	int read(unsigned short*, const std::string&);

	int read(std::vector<double>&, const std::string&);
	int read(std::vector<float>&, const std::string&);
	int read(std::vector<int>&, const std::string&);
	int read(std::vector<unsigned int>&, const std::string&);
	int read(std::vector<long>&, const std::string&);
	int read(std::vector<unsigned char>&, const std::string&);
	int read(std::vector<unsigned short>&, const std::string&);

	int read(float* data, size_type offset, size_type length,
			const std::string& name);
	int read(unsigned short* data, size_type offset, size_type length,
			const std::string& name);

	template<class T>
	static int read2(std::vector<T>& array, const std::string& path)
	{
		std::string filename;
		std::string dataset;

		HDF5Reader::decompose(filename, dataset, path);
		HDF5Reader reader;
		if (!reader.open(filename))
		{
			ISEG_ERROR_MSG("opening " + filename);
			return 0;
		}

		if (!reader.read(array, dataset))
		{
			ISEG_ERROR_MSG("reading " + dataset);
			return 0;
		}

		reader.close();
		return 1;
	}

	bool loud = false;

private:
	template<typename T>
	int readVec(std::vector<T>&, const std::string&);

	template<typename T>
	int readData(T* data, const std::string& name);

	hid_type file = -1;
};

} // namespace iseg
