/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifdef _DEBUG
#	include <cstdio> // TODO: remove this
#endif
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>

#include <hdf5.h>

#include "Tools.h"

#include "HDF5IO.h"
#include "HDF5Reader.h"

using namespace std;
using namespace HDF5;
using namespace Tools;

static herr_t my_info(hid_t loc_id, const char *name, void *opdata)
{
	//	if(loud) cerr << "iterate name = " << name << endl;
	vector<string> *names = (vector<string> *)opdata;
	names->push_back(string(name));
	return 0;
}

HDF5Reader::HDF5Reader()
{
	static_assert(std::is_same<size_type, hsize_t>::value, "type mismatch");
	file = -1;
	loud = 0;
	data = 0;
}

HDF5Reader::~HDF5Reader() { close(); }

void HDF5Reader::decompose(string &filename, string &dataname,
													 const string &pathname)
{
	const std::string::size_type idx = pathname.find(":");
	//   cerr << "idx = " << int(idx==string::npos) << endl;
	filename = (idx != string::npos) ? pathname.substr(0, idx) : "";
	dataname = (idx != string::npos) ? pathname.substr(idx + 1) : pathname;
}

vector<string> HDF5Reader::getGroupInfo(const string &name)
{
	vector<string> names;

	if (file < 0)
	{
		if (loud)
			cerr << "HDF5Reader::groupInfo : warning, no file open" << endl;
		return names;
	}

	//int idx_f = H5Giterate(file, name.c_str(), NULL, my_info, NULL);
	int idx_f = H5Giterate(file, name.c_str(), NULL, my_info, &names);
	if (loud)
		cerr << "HDF5Reader::groupInfo : idx_f = " << idx_f << endl;
	return names;
}

int HDF5Reader::exists(const string &dset)
{
	return (H5Lexists(file, dset.c_str(), H5P_DEFAULT) > 0) ? 1 : 0;
}

HDF5Reader::size_type HDF5Reader::totalSize(const std::vector<size_type> &dims)
{
	if (dims.empty())
		return 0;
	size_type size = dims[0];
	for (size_type i = 1; i < dims.size(); i++)
	{
		size *= dims[i];
	}
	return size;
}

bool HDF5Reader::existsValidHdf5(const string &fname)
{
#ifndef _DEBUG // TODO: temporarily disabled for debug mode
	ifstream test(fname.c_str());
	bool exists = test.is_open();
	test.close();
	if (!exists)
	{
		cerr << "HDF5Reader::existsValidHdf5() : error, '" << fname
				 << "' does not exist" << endl;
		return 0;
	}
#else
	FILE *pFile = fopen(fname.c_str(), "r");
	if (pFile == NULL)
	{
		cerr << "HDF5Reader::existsValidHdf5() : error, '" << fname
				 << "' does not exist" << endl;
		return 0;
	}
	else
	{
		fclose(pFile);
	}
#endif // TODO

	htri_t status = H5Fis_hdf5(fname.c_str());
	if (status > 0)
		return 1;
	return 0;
}

int HDF5Reader::open(const string &fname)
{
	if (file >= 0)
	{
		cerr << "HDF5Reader::open() : a file is already open, close first" << endl;
		return 0;
	}
	if (!HDF5Reader::existsValidHdf5(fname.c_str()))
	{
		cerr << "HDF5Reader::open() : " << fname
				 << " does not exist or is not a valid hdf5 file" << endl;
		return 0;
	}
	file = H5Fopen(fname.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file < 0)
	{
		cerr << "HDF5Reader::open() : could not open " << fname << endl;
		return 0;
	}
	else
		return 1;
}

int HDF5Reader::open(const char *fn) { return this->open(string(fn)); }

int HDF5Reader::close()
{
	if (file >= 0)
	{
		herr_t status = H5Fclose(file);
		if (status < 0)
		{
			cerr << "HDF5Reader::close() : error, closing file failed" << endl;
			return 0;
		}
		file = -1;
	}
	return 1;
}

int HDF5Reader::getDatasetInfo(string &type, vector<size_type> &extents,
															 const string &name)
{
	if (file < 0)
	{
		cerr << "HDF5Reader::getDatasetInfo() : error, no files open" << endl;
		return 0;
	}

	herr_t status = -1;

	if (loud)
		cerr << "HDF5Reader::getDatasetInfo() : checking dataset " << name << endl;
	hid_t dataset = H5Dopen2(file, name.c_str(), H5P_DEFAULT);
	if (dataset < 0)
	{
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : no such dataset: " << name
					 << endl;
		type = "none";
		return 0;
	}

	hid_t datatype = H5Dget_type(dataset);
	// 	H5T_class_t elclass = H5Tget_class(datatype);
	// 	size_t elsize = H5Tget_size(datatype);
	hid_t native_type = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
	// 	if(loud)
	// 	{
	// 		cerr << "HDF5Reader::getDatasetInfo() : element class = " << elclass << endl;
	// 		cerr << "HDF5Reader::getDatasetInfo() : element size = " << elsize << endl;
	// 		cerr << "HDF5Reader::getDatasetInfo() : native type = " << native_type << endl;
	// 	}

	if (H5Tequal(native_type, H5T_NATIVE_DOUBLE) > 0)
	{
		type = "double";
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : found double" << endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_FLOAT) > 0)
	{
		type = "float";
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : found float" << endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_INT) > 0)
	{
		type = "int";
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : found int" << endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_UINT) > 0)
	{
		type = "unsigned int";
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : found unsigned int" << endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_LONG) > 0)
	{
		type = "long";
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : found int" << endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_UCHAR) > 0)
	{
		type = "unsigned char";
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : found unsigned char" << endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_USHORT) > 0)
	{
		type = "unsigned short";
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : found unsigned short" << endl;
	}
	else
	{
		cerr << "HDF5Reader::getDatasetInfo() : warning, unsupported native type"
				 << endl;
		status = H5Tclose(native_type);
		status = H5Tclose(datatype);
		status = H5Dclose(dataset);
		return 0;
	}

	hid_t dataspace = H5Dget_space(dataset);
	int rank = H5Sget_simple_extent_ndims(dataspace);
	if (loud)
		cerr << "HDF5Reader::getDatasetInfo() : rank = " << rank << endl;
	assert(rank > 0);

	//	hsize_t dims[rank];
	std::vector<hsize_t> dims(rank);
	status = H5Sget_simple_extent_dims(dataspace, dims.data(), NULL);
	extents.resize(rank);
	for (int i = 0; i < rank; i++)
	{
		extents[i] = dims[i];
		if (loud)
			cerr << "HDF5Reader::getDatasetInfo() : extents = " << extents[i] << endl;
	}

	// 	if(elclass == H5T_FLOAT && elsize==8)
	// 		type = "double";
	// 	else if(elclass == H5T_FLOAT && elsize==4)
	// 		type = "float";
	// 	else if(elclass == H5T_INTEGER && elsize==4)
	// 		type = "int";
	// 	else if(elclass == H5T_INTEGER && elsize==1)
	// 		type = "unsigned char";
	// 	else if(loud)
	// 		cerr << "HDF5Reader::getDatasetInfo() : warning, unrecognized data type" << endl;

	status = H5Sclose(dataspace);
	status = H5Tclose(native_type);
	status = H5Tclose(datatype);
	status = H5Dclose(dataset);
	return 1;
}

int HDF5Reader::read(double *_data, const string &name)
{
	data = (void *)_data;
	return this->readData(name);
}

int HDF5Reader::read(float *_data, const string &name)
{
	data = (void *)_data;
	return this->readData(name);
}

int HDF5Reader::read(int *_data, const string &name)
{
	data = (void *)_data;
	return this->readData(name);
}

int HDF5Reader::read(unsigned int *_data, const string &name)
{
	data = (void *)_data;
	return this->readData(name);
}

int HDF5Reader::read(long *_data, const string &name)
{
	data = (void *)_data;
	return this->readData(name);
}

int HDF5Reader::read(unsigned char *_data, const string &name)
{
	data = (void *)_data;
	return this->readData(name);
}

int HDF5Reader::read(unsigned short *_data, const string &name)
{
	data = (void *)_data;
	return this->readData(name);
}

int HDF5Reader::read(vector<double> &v, const std::string &name)
{
	string type;
	vector<size_type> dims;
	if (!getDatasetInfo(type, dims, name))
		return 0;
	if (type != "double")
	{
		if (type == "float")
		{
			return read_cast<float>(v, name);
		}
		else
		{
			warning("HDF5Reader::read() : data must be float, returning");
			return 0;
		}
		warning("HDF5Reader::read() : data must be double, returning");
		return 0;
	}
	if (dims.size() != 1)
	{
		warning("HDF5Reader::read() : rank must be 1 for vectors, returning");
		return 0;
	}
	v.resize(dims[0]);
	data = (void *)v.data();
	return this->readData(name);
}

int HDF5Reader::read(vector<float> &v, const std::string &name)
{
	std::string type;
	std::vector<size_type> dims;
	if (!getDatasetInfo(type, dims, name))
		return 0;
	if (type != "float")
	{
		if (type == "double")
		{
			return read_cast<double>(v, name);
		}
		else
		{
			warning("HDF5Reader::read() : data must be float, returning");
			return 0;
		}
	}
	else
	{
		if (dims.size() != 1)
		{
			warning("HDF5Reader::read() : rank must be 1 for vectors, returning");
			return 0;
		}
		v.resize(dims[0]);
		data = (void *)v.data();
		return this->readData(name);
	}
}

int HDF5Reader::read(vector<int> &v, const std::string &name)
{
	string type;
	vector<size_type> dims;
	if (!getDatasetInfo(type, dims, name))
		return 0;
	if (dims.size() != 1)
	{
		warning("HDF5Reader::read() : rank must be 1 for vectors, returning");
		return 0;
	}
	if (type != "int")
	{
		warning("HDF5Reader::read() : data must be int, returning");
		return 0;
	}
	v.resize(dims[0]);
	data = (void *)v.data();
	return this->readData(name);
}

int HDF5Reader::read(vector<unsigned int> &v, const std::string &name)
{
	string type;
	vector<size_type> dims;
	if (!getDatasetInfo(type, dims, name))
		return 0;
	if (dims.size() != 1)
	{
		warning("HDF5Reader::read() : rank must be 1 for vectors, returning");
		return 0;
	}
	if (type != "unsigned int")
	{
		warning("HDF5Reader::read() : data must be unsigned int, returning");
		return 0;
	}
	v.resize(dims[0]);
	data = (void *)v.data();
	return this->readData(name);
}

int HDF5Reader::read(vector<long> &v, const std::string &name)
{
	string type;
	vector<size_type> dims;
	if (!getDatasetInfo(type, dims, name))
		return 0;
	if (dims.size() != 1)
	{
		warning("HDF5Reader::read() : rank must be 1 for vectors, returning");
		return 0;
	}
	if (type != "long")
	{
		warning("HDF5Reader::read() : data must be long, returning");
		return 0;
	}
	v.resize(dims[0]);
	data = (void *)v.data();
	return this->readData(name);
}

int HDF5Reader::read(vector<unsigned char> &v, const std::string &name)
{
	string type;
	vector<size_type> dims;
	if (!getDatasetInfo(type, dims, name))
		return 0;
	if (dims.size() != 1)
	{
		warning("HDF5Reader::read() : rank must be 1 for vectors, returning");
		return 0;
	}
	if (type != "unsigned char")
	{
		warning("HDF5Reader::read() : data must be unsigned char, returning");
		return 0;
	}
	v.resize(dims[0]);
	data = (void *)v.data();
	return this->readData(name);
}

int HDF5Reader::read(vector<unsigned short> &v, const std::string &name)
{
	string type;
	vector<size_type> dims;
	if (!getDatasetInfo(type, dims, name))
		return 0;
	if (dims.size() != 1)
	{
		warning("HDF5Reader::read() : rank must be 1 for vectors, returning");
		return 0;
	}
	if (type != "unsigned short")
	{
		warning("HDF5Reader::read() : data must be unsigned short, returning");
		return 0;
	}
	v.resize(dims[0]);
	data = (void *)v.data();
	return this->readData(name);
}

int HDF5::HDF5Reader::read(float *data, size_type offset, size_type length,
													 const std::string &name)
{
	return HDF5IO().readData(file, name, offset, length, data) ? 1 : 0;
}

int HDF5::HDF5Reader::read(unsigned short *data, size_type offset,
													 size_type length, const std::string &name)
{
	return HDF5IO().readData(file, name, offset, length, data) ? 1 : 0;
}

int HDF5Reader::readData(const string &name)
{
	if (file < 0)
	{
		cerr << "HDF5Reader::readData() : no file open" << endl;
		return 0;
	}

	if (!data)
	{
		cerr << "HDF5Reader::readData() : no pointer to data" << endl;
		return 0;
	}

	hid_t dataset, datatype, dataspace;
	herr_t status;

	if (loud)
		cerr << "HDF5Reader::readData() : loading dataset " << name << endl;
	dataset = H5Dopen2(file, name.c_str(), H5P_DEFAULT);
	if (dataset < 0)
	{
		cerr << "HDF5Reader::readData() : no such dataset: " << name << endl;
		return 0;
	}

	datatype = H5Dget_type(dataset);
	hid_t native_type = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
	dataspace = H5Dget_space(dataset);
	int rank = H5Sget_simple_extent_ndims(dataspace);
	std::vector<hsize_t> dims(rank);
	if (loud)
		cerr << "HDF5Reader::readData() : rank = " << rank << endl;
	if (rank <= 0)
		error("invalid rank");
	status = H5Sget_simple_extent_dims(dataspace, dims.data(), NULL);
	//if(loud) cerr << "HDF5Reader::read() : reading " << names[i] << " of type " << types[i] << " and total size " << totsize << endl;

	// 	if(elclass == H5T_FLOAT && elsize==8)
	if (H5Tequal(native_type, H5T_NATIVE_DOUBLE) > 0)
	{
		if (loud)
			cerr << "HDF5Reader::readData() : double format detected" << endl;
		status = H5Dread(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL, H5P_DEFAULT,
										 (double *)data);
	}
	// 	else if(elclass == H5T_INTEGER && elsize==4)
	else if (H5Tequal(native_type, H5T_NATIVE_FLOAT) > 0)
	{
		if (loud)
			cerr << "HDF5Reader::readData() : float format detected" << endl;
		status = H5Dread(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
										 (float *)data);
	}
	else if (H5Tequal(native_type, H5T_NATIVE_INT) > 0)
	{
		if (loud)
			cerr << "HDF5Reader::readData() : int format detected" << endl;
		status = H5Dread(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
										 (int *)data);
	}
	else if (H5Tequal(native_type, H5T_NATIVE_UINT) > 0)
	{
		if (loud)
			cerr << "HDF5Reader::readData() : unsigned int format detected" << endl;
		status = H5Dread(dataset, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
										 (unsigned int *)data);
	}
	// 	else if(elclass == H5T_INTEGER && elsize==1)
	else if (H5Tequal(native_type, H5T_NATIVE_LONG) > 0)
	{
		if (loud)
			cerr << "HDF5Reader::readData() : long format detected" << endl;
		status = H5Dread(dataset, H5T_NATIVE_LONG, H5S_ALL, H5S_ALL, H5P_DEFAULT,
										 (long *)data);
	}
	else if (H5Tequal(native_type, H5T_NATIVE_UCHAR) > 0)
	{
		if (loud)
			cerr << "HDF5Reader::readData() : unsigned char format detected" << endl;
		status = H5Dread(dataset, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL, H5P_DEFAULT,
										 (unsigned char *)data);
	}
	else if (H5Tequal(native_type, H5T_NATIVE_USHORT) > 0)
	{
		if (loud)
			cerr << "HDF5Reader::readData() : unsigned short format detected" << endl;
		status = H5Dread(dataset, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
										 (unsigned short *)data);
	}

	status = H5Tclose(native_type);
	status = H5Tclose(datatype);
	status = H5Sclose(dataspace);
	status = H5Dclose(dataset);

	if (status < 0)
		return 0;
	else
		return 1;
}
