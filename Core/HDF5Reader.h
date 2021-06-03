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
	int Open(const std::string&);
	int Open(const char* fn);
	int Close();
	static bool ExistsValidHdf5(const std::string&);
	static size_type TotalSize(const std::vector<size_type>&);
	std::vector<std::string> GetGroupInfo(const std::string&) const;
	int GetDatasetInfo(std::string&, std::vector<size_type>&, const std::string&) const;
	int Exists(const std::string&) const;

	// Description:
	// Decompose a string of the type 'filename.h5:/dataset' into two strings 'filename.h5' and 'dataset'.
	// In case the input string is of type '/dataset' it will return '' and '/dataset'.
	static void Decompose(std::string&, std::string&, const std::string&);

	int Read(double*, const std::string&);
	int Read(float*, const std::string&);
	int Read(int*, const std::string&);
	int Read(unsigned int*, const std::string&);
	int Read(long*, const std::string&);
	int Read(unsigned char*, const std::string&);
	int Read(unsigned short*, const std::string&);

	int Read(std::vector<double>&, const std::string&);
	int Read(std::vector<float>&, const std::string&);
	int Read(std::vector<int>&, const std::string&);
	int Read(std::vector<unsigned int>&, const std::string&);
	int Read(std::vector<long>&, const std::string&);
	int Read(std::vector<unsigned char>&, const std::string&);
	int Read(std::vector<unsigned short>&, const std::string&);

	int Read(float* data, size_type offset, size_type length, const std::string& name) const;
	int Read(unsigned short* data, size_type offset, size_type length, const std::string& name) const;

	template<class T>
	static int Read2(std::vector<T>& array, const std::string& path)
	{
		std::string filename;
		std::string dataset;

		HDF5Reader::Decompose(filename, dataset, path);
		HDF5Reader reader;
		if (!reader.Open(filename))
		{
			ISEG_ERROR_MSG("opening " + filename);
			return 0;
		}

		if (!reader.Read(array, dataset))
		{
			ISEG_ERROR_MSG("reading " + dataset);
			return 0;
		}

		reader.Close();
		return 1;
	}

	bool m_Loud = false;

private:
	template<typename T>
	int ReadVec(std::vector<T>&, const std::string&);

	template<typename T>
	int ReadData(T* data, const std::string& name);

	hid_type m_File = -1;
};

} // namespace iseg
