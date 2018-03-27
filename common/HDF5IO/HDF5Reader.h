/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef HDF5READER_H
#define HDF5READER_H

#include "HDF5IOApi.h"

#include <string>
#include <vector>

namespace HDF5 {
class HDF5_EXPORT HDF5Reader
{
public:
	typedef unsigned long long size_type;
	typedef int hid_type;
	HDF5Reader();
	~HDF5Reader();
	int open(const std::string &);
	int open(const char *fn);
	int close();
	static bool existsValidHdf5(const std::string &);
	static size_type totalSize(const std::vector<size_type> &);
	std::vector<std::string> getGroupInfo(const std::string &);
	int getDatasetInfo(std::string &, std::vector<size_type> &,
										 const std::string &);
	int exists(const std::string &);

	// Description:
	// Decompose a string of the type 'filename.h5:/dataset' into two strings 'filename.h5' and 'dataset'.
	// In case the input string is of type '/dataset' it will return '' and '/dataset'.
	static void decompose(std::string &, std::string &, const std::string &);

	int read(double *, const std::string &);
	int read(float *, const std::string &);
	int read(int *, const std::string &);
	int read(unsigned int *, const std::string &);
	int read(long *, const std::string &);
	int read(unsigned char *, const std::string &);
	int read(unsigned short *, const std::string &);
	int read(std::vector<double> &, const std::string &);
	int read(std::vector<float> &, const std::string &);
	int read(std::vector<int> &, const std::string &);
	int read(std::vector<unsigned int> &, const std::string &);
	int read(std::vector<long> &, const std::string &);
	int read(std::vector<unsigned char> &, const std::string &);
	int read(std::vector<unsigned short> &, const std::string &);

	int read(float *data, size_type offset, size_type length,
					 const std::string &name);
	int read(unsigned short *data, size_type offset, size_type length,
					 const std::string &name);

	template<class T>
	static int read2(std::vector<T> &array, const std::string &path)
	{
		std::string filename;
		std::string dataset;

		HDF5Reader::decompose(filename, dataset, path);
		HDF5Reader reader;
		if (!reader.open(filename))
		{
			std::cerr << "error opening " + filename << std::endl;
			;
			return 0;
		}

		if (!reader.read(array, dataset))
		{
			std::cerr << "error reading " + dataset << std::endl;
			;
			return 0;
		}

		reader.close();
		return 1;
	}

	int loud;

private:
	template<typename T1, typename T2>
	inline int read_cast(std::vector<T2> &v, const std::string &name)
	{
		std::vector<T1> v_temp;
		int ret = read(v_temp, name);
		v.resize(v_temp.size());
		std::transform(v_temp.begin(), v_temp.end(), v.begin(),
									 [](float x) { return static_cast<T2>(x); });
		return ret;
	}

	int readData(const std::string &);

	void *data;
	hid_type file;
};
} // namespace HDF5
#endif
