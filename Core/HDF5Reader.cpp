/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "HDF5IO.h"
#include "HDF5Reader.h"
#include "Log.h"

#include <hdf5.h>

namespace iseg {

static herr_t my_info(hid_t loc_id, const char* name, void* opdata)
{
	//	if(loud) std::cerr << "iterate name = " << name << std::endl;
	std::vector<std::string>* names = (std::vector<std::string>*)opdata;
	names->push_back(std::string(name));
	return 0;
}

HDF5Reader::HDF5Reader()
{
	static_assert(std::is_same<size_type, hsize_t>::value, "type mismatch");
}

HDF5Reader::~HDF5Reader() { close(); }

void HDF5Reader::decompose(std::string& filename, std::string& dataname,
						   const std::string& pathname)
{
	const std::string::size_type idx = pathname.find(':');

	filename = (idx != std::string::npos) ? pathname.substr(0, idx) : "";
	dataname = (idx != std::string::npos) ? pathname.substr(idx + 1) : pathname;
}

std::vector<std::string> HDF5Reader::getGroupInfo(const std::string& name)
{
	std::vector<std::string> names;

	if (file < 0)
	{
		if (loud)
			std::cerr << "HDF5Reader::groupInfo : warning, no file open" << std::endl;
		return names;
	}

	//int idx_f = H5Giterate(file, name.c_str(), nullptr, my_info, nullptr);
	int idx_f = H5Giterate(file, name.c_str(), nullptr, my_info, &names);
	if (loud)
		std::cerr << "HDF5Reader::groupInfo : idx_f = " << idx_f << std::endl;
	return names;
}

int HDF5Reader::exists(const std::string& dset)
{
	return (H5Lexists(file, dset.c_str(), H5P_DEFAULT) > 0) ? 1 : 0;
}

HDF5Reader::size_type HDF5Reader::totalSize(const std::vector<size_type>& dims)
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

bool HDF5Reader::existsValidHdf5(const std::string& fname)
{
#ifndef _DEBUG // TODO: temporarily disabled for debug mode
	std::ifstream test(fname.c_str());
	bool exists = test.is_open();
	test.close();
	if (!exists)
	{
		std::cerr << "HDF5Reader::existsValidHdf5() : error, '" << fname
				  << "' does not exist" << std::endl;
		return false;
	}
#else
	FILE* pFile = fopen(fname.c_str(), "r");
	if (pFile == nullptr)
	{
		std::cerr << "HDF5Reader::existsValidHdf5() : error, '" << fname
				  << "' does not exist" << std::endl;
		return false;
	}
	else
	{
		fclose(pFile);
	}
#endif // TODO

	htri_t status = H5Fis_hdf5(fname.c_str());
	if (status > 0)
		return true;
	return false;
}

int HDF5Reader::open(const std::string& fname)
{
	if (file >= 0)
	{
		std::cerr << "HDF5Reader::open() : a file is already open, close first" << std::endl;
		return 0;
	}
	if (!HDF5Reader::existsValidHdf5(fname))
	{
		std::cerr << "HDF5Reader::open() : " << fname
				  << " does not exist or is not a valid hdf5 file" << std::endl;
		return 0;
	}
	file = H5Fopen(fname.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
	if (file < 0)
	{
		std::cerr << "HDF5Reader::open() : could not open " << fname << std::endl;
		return 0;
	}
	return 1;
}

int HDF5Reader::open(const char* fn) { return this->open(std::string(fn)); }

int HDF5Reader::close()
{
	if (file >= 0)
	{
		herr_t status = H5Fclose(file);
		if (status < 0)
		{
			std::cerr << "HDF5Reader::close() : error, closing file failed" << std::endl;
			return 0;
		}
		file = -1;
	}
	return 1;
}

int HDF5Reader::getDatasetInfo(std::string& type, std::vector<size_type>& extents,
							   const std::string& name)
{
	if (file < 0)
	{
		std::cerr << "HDF5Reader::getDatasetInfo() : error, no files open" << std::endl;
		return 0;
	}

	herr_t status = -1;

	if (loud)
		std::cerr << "HDF5Reader::getDatasetInfo() : checking dataset " << name << std::endl;
	hid_t dataset = H5Dopen2(file, name.c_str(), H5P_DEFAULT);
	if (dataset < 0)
	{
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : no such dataset: " << name
					  << std::endl;
		type = "none";
		return 0;
	}

	hid_t datatype = H5Dget_type(dataset);
	// 	H5T_class_t elclass = H5Tget_class(datatype);
	// 	size_t elsize = H5Tget_size(datatype);
	hid_t native_type = H5Tget_native_type(datatype, H5T_DIR_ASCEND);
	// 	if(loud)
	// 	{
	// 		std::cerr << "HDF5Reader::getDatasetInfo() : element class = " << elclass << std::endl;
	// 		std::cerr << "HDF5Reader::getDatasetInfo() : element size = " << elsize << std::endl;
	// 		std::cerr << "HDF5Reader::getDatasetInfo() : native type = " << native_type << std::endl;
	// 	}

	if (H5Tequal(native_type, H5T_NATIVE_DOUBLE) > 0)
	{
		type = "double";
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : found double" << std::endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_FLOAT) > 0)
	{
		type = "float";
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : found float" << std::endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_INT) > 0)
	{
		type = "int";
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : found int" << std::endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_UINT) > 0)
	{
		type = "unsigned int";
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : found unsigned int" << std::endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_LONG) > 0)
	{
		type = "long";
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : found int" << std::endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_UCHAR) > 0)
	{
		type = "unsigned char";
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : found unsigned char" << std::endl;
	}
	else if (H5Tequal(native_type, H5T_NATIVE_USHORT) > 0)
	{
		type = "unsigned short";
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : found unsigned short" << std::endl;
	}
	else
	{
		std::cerr << "HDF5Reader::getDatasetInfo() : warning, unsupported native type"
				  << std::endl;
		status = H5Tclose(native_type);
		status = H5Tclose(datatype);
		status = H5Dclose(dataset);
		return 0;
	}

	hid_t dataspace = H5Dget_space(dataset);
	int rank = H5Sget_simple_extent_ndims(dataspace);
	if (loud)
		std::cerr << "HDF5Reader::getDatasetInfo() : rank = " << rank << std::endl;
	assert(rank > 0);

	//	hsize_t dims[rank];
	std::vector<hsize_t> dims(rank);
	status = H5Sget_simple_extent_dims(dataspace, dims.data(), nullptr);
	extents.resize(rank);
	for (int i = 0; i < rank; i++)
	{
		extents[i] = dims[i];
		if (loud)
			std::cerr << "HDF5Reader::getDatasetInfo() : extents = " << extents[i] << std::endl;
	}

	status = H5Sclose(dataspace);
	status = H5Tclose(native_type);
	status = H5Tclose(datatype);
	status = H5Dclose(dataset);
	return 1;
}

template<typename T>
struct HdfType
{
    static hid_t type() { return -1; }
};

template <> struct HdfType<double> { static hid_t type() { return H5T_NATIVE_DOUBLE; } };
template <> struct HdfType<float>  { static hid_t type() { return H5T_NATIVE_FLOAT; } };
template <> struct HdfType<std::uint8_t>  { static hid_t type() { return H5T_NATIVE_UINT8; } };
template <> struct HdfType<std::uint16_t> { static hid_t type() { return H5T_NATIVE_UINT16; } };
template <> struct HdfType<std::uint32_t> { static hid_t type() { return H5T_NATIVE_UINT32; } };
template <> struct HdfType<std::uint64_t> { static hid_t type() { return H5T_NATIVE_UINT64; } };
template <> struct HdfType<std::int8_t>  { static hid_t type() { return H5T_NATIVE_INT8; } };
template <> struct HdfType<std::int16_t> { static hid_t type() { return H5T_NATIVE_INT16; } };
template <> struct HdfType<std::int32_t> { static hid_t type() { return H5T_NATIVE_INT32; } };
template <> struct HdfType<std::int64_t> { static hid_t type() { return H5T_NATIVE_INT64; } };

int HDF5Reader::read(double* _data, const std::string& name)
{
    return this->readData(_data, name);
}

int HDF5Reader::read(float* _data, const std::string& name)
{
    return this->readData(_data, name);
}

int HDF5Reader::read(int* _data, const std::string& name)
{
    return this->readData(_data, name);
}

int HDF5Reader::read(unsigned int* _data, const std::string& name)
{
    return this->readData(_data, name);
}

int HDF5Reader::read(long* _data, const std::string& name)
{
    return this->readData(_data, name);
}

int HDF5Reader::read(unsigned char* _data, const std::string& name)
{
    return this->readData(_data, name);
}

int HDF5Reader::read(unsigned short* _data, const std::string& name)
{
	return this->readData(_data, name);
}

int HDF5Reader::read(std::vector<double>& v, const std::string& name)
{
    return readVec(v, name);
}

int HDF5Reader::read(std::vector<float>& v, const std::string& name)
{
    return readVec(v, name);
}

int HDF5Reader::read(std::vector<int>& v, const std::string& name)
{
    return readVec(v, name);
}

int HDF5Reader::read(std::vector<unsigned int>& v, const std::string& name)
{
    return readVec(v, name);
}

int HDF5Reader::read(std::vector<long>& v, const std::string& name)
{
    return readVec(v, name);
}

int HDF5Reader::read(std::vector<unsigned char>& v, const std::string& name)
{
    return readVec(v, name);
}

int HDF5Reader::read(std::vector<unsigned short>& v, const std::string& name)
{
    return readVec(v, name);
}

template<typename T>
int HDF5Reader::readVec(std::vector<T>& v, const std::string& name)
{
    std::string type;
    std::vector<size_type> dims;
    if (!getDatasetInfo(type, dims, name))
        return 0;
    if (dims.size() != 1)
    {
        warning("HDF5Reader::read() : rank must be 1 for std::vectors, returning");
        return 0;
    }
    v.resize(dims[0]);
    return this->readData(v.data(), name);
}

int HDF5Reader::read(float* data, size_type offset, size_type length,
					 const std::string& name)
{
	return HDF5IO().readData(file, name, offset, length, data) ? 1 : 0;
}

int HDF5Reader::read(unsigned short* data, size_type offset,
					 size_type length, const std::string& name)
{
	return HDF5IO().readData(file, name, offset, length, data) ? 1 : 0;
}

template<typename T>
int HDF5Reader::readData(T* Array, const std::string& name)
{
    if (file < 0)
    {
        std::cerr << "HDF5Reader::readData() : no file open" << std::endl;
        return 0;
    }
    if (!Array)
    {
        std::cerr << "HDF5Reader::readData() : no pointer to data" << std::endl;
        return 0;
    }

    if (loud)
        std::cerr << "HDF5Reader::readData() : loading dataset " << name << std::endl;

    bool ok = false;

    hid_t dataset = H5Dopen2(file, name.c_str(), H5P_DEFAULT);
    if (dataset >= 0)
    {
        hid_t dataspace = H5Dget_space(dataset);
        if (dataspace >= 0)
        {
            int rank = H5Sget_simple_extent_ndims(dataspace);
            if (rank >= 0)
            {
                std::vector<hsize_t> dims(rank);
                H5Sget_simple_extent_dims(dataspace, dims.data(), nullptr);
                H5Sselect_all(dataspace);

                if (H5Sselect_valid(dataspace) >= 0)
                {
                    // Define the memory space to read dataset.
                    hid_t memoryspace = H5Screate_simple(rank, dims.data(), nullptr);

                    hid_t type_id = HdfType<T>::type();
                    if (HdfType<T>::type < 0)
                    {
                        // get data type from file
                        type_id = H5Dget_type(dataset);
                    }

                    herr_t status = H5Dread(dataset, type_id, memoryspace, dataspace, H5P_DEFAULT, Array);
                    ok = (status >= 0);

                    if (HdfType<T>::type() < 0)
                        H5Tclose(type_id);
                    H5Sclose(memoryspace);
                }
            }
            H5Sclose(dataspace);
        }
        H5Dclose(dataset);
    }

    return ok ? 1 : 0;
}

} // namespace iseg
