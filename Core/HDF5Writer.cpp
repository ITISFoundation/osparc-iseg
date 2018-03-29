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
#include "HDF5Writer.h"
#include "Log.h"

#include <boost/algorithm/string.hpp>

#include <hdf5.h>

namespace iseg {

HDF5Writer::HDF5Writer()
{
	static_assert(std::is_same<size_type, hsize_t>::value, "type mismatch");
	compression = 1;
	loud = 0;
	file = -1;
	bufsize = 1024 * 1024;
}

HDF5Writer::~HDF5Writer() { close(); }

int HDF5Writer::open(const std::string& fname, const std::string& fileMode)
{
	if (loud)
	{
		std::cerr << "HDF5Writer::open() : opening file " << fname << " in "
				  << fileMode << " mode" << std::endl;
	}
	if (file >= 0)
	{
		std::cerr << "HDF5Writer::open() : error, file " << fname
				  << " is already open, close first" << std::endl;
		return 0;
	}
	if (fileMode == "overwrite")
	{
		if (loud)
		{
			std::cerr << "HDF5Writer::write() : creating " << fname << std::endl;
		}
		file = H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
		// 		std::cerr << "file = " << file << std::endl;
	}
	else if (fileMode == "append")
	{
		if (loud)
		{
			std::cerr << "HDF5Writer::write() : appending " << fname << std::endl;
		}
		file = H5Fopen(fname.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
	}
	else
	{
		std::cerr << "HDF5Writer::write() : error, unknown mode" << std::endl;
		return 0;
	}
	if (file < 0)
	{
		std::cerr << "HDF5Writer::write() : error, no files were opened"
				  << std::endl;
		return 0;
	}
	else
	{
		if (loud)
		{
			std::cerr << "HDF5Writer::write() : created file id = " << file
					  << std::endl;
		}
		return 1;
	}
}

int HDF5Writer::open(const char* fn, const std::string& m)
{
	return this->open(std::string(fn), m);
}

int HDF5Writer::flush()
{
	if (file < 0)
	{
		if (loud)
		{
			std::cerr << "HDF5Writer::flush() : no files open" << std::endl;
		}
		return 0;
	}
	if (loud)
	{
		std::cerr << "HDF5Writer::flush() : flushing file id " << file << std::endl;
	}
	herr_t err = H5Fflush(file, H5F_SCOPE_GLOBAL);
	if (err < 0)
	{
		return 0;
	}
	return 1;
}

int HDF5Writer::close()
{
	if (file >= 0)
	{
		if (loud)
		{
			std::cerr << "HDF5Writer::close() : closing file id " << file
					  << std::endl;
		}
		herr_t status = H5Fclose(file);
		if (status < 0)
		{
			std::cerr << "HDF5Writer::close() : error, closing file failed"
					  << std::endl;
			return 0;
		}
		file = -1;
	}
	else if (loud)
	{
		std::cerr << "HDF5Writer::close() : no open files to close" << std::endl;
	}

	return 1;
}

int HDF5Writer::createGroup(const std::string& gname)
{
	if (file < 0)
	{
		if (loud)
		{
			std::cerr << "HDF5Writer::createGroup() : no files open" << std::endl;
		}
		return 0;
	}

	hid_t group =
		H5Gcreate2(file, gname.c_str(), H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	if (group < 0)
	{
		std::cerr << "HDF5Writer::createGroup() : creating group failed"
				  << std::endl;
		return 0;
	}
	herr_t status = H5Gclose(group);
	if (status < 0)
	{
		std::cerr << "HDF5Writer::createGroup() : closing group failed"
				  << std::endl;
		return 0;
	}
	return 1;
}

int HDF5Writer::write(float** const slice_data, size_type num_slices,
					  size_type slice_size, const std::string& name,
					  size_t offset)
{
	return HDF5IO(compression)
				   .writeData(file, name, slice_data, num_slices, slice_size,
							  offset)
			   ? 1
			   : 0;
}

int HDF5Writer::write(unsigned short** const slice_data,
					  size_type num_slices, size_type slice_size,
					  const std::string& name, size_t offset)
{
	return HDF5IO(compression)
				   .writeData(file, name, slice_data, num_slices, slice_size,
							  offset)
			   ? 1
			   : 0;
}

int HDF5Writer::write(const double* data, const std::vector<size_type>& dims,
					  const std::string& name)
{
	const std::string type = "double";
	return this->writeData(data, type, dims, name);
}

int HDF5Writer::write(const float* data, const std::vector<size_type>& dims,
					  const std::string& name)
{
	const std::string type = "float";
	return this->writeData(data, type, dims, name);
}

int HDF5Writer::write(const int* data, const std::vector<size_type>& dims,
					  const std::string& name)
{
	const std::string type = "int";
	return this->writeData(data, type, dims, name);
}

int HDF5Writer::write(const unsigned int* data,
					  const std::vector<size_type>& dims,
					  const std::string& name)
{
	const std::string type = "unsigned int";
	return this->writeData(data, type, dims, name);
}

int HDF5Writer::write(const long* data, const std::vector<size_type>& dims,
					  const std::string& name)
{
	const std::string type = "long";
	return this->writeData(data, type, dims, name);
}

int HDF5Writer::write(const unsigned char* data,
					  const std::vector<size_type>& dims,
					  const std::string& name)
{
	const std::string type = "unsigned char";
	return this->writeData(data, type, dims, name);
}

int HDF5Writer::write(const unsigned short* data,
					  const std::vector<size_type>& dims,
					  const std::string& name)
{
	const std::string type = "unsigned short";
	return this->writeData(data, type, dims, name);
}

int HDF5Writer::write(const std::vector<int>& v, const std::string& name)
{
	std::vector<size_type> dims(1);
	dims[0] = v.size();
	const std::string type = "int";
	return this->writeData(&v[0], type, dims, name);
}

int HDF5Writer::write(const std::vector<unsigned int>& v,
					  const std::string& name)
{
	std::vector<size_type> dims(1);
	dims[0] = v.size();
	const std::string type = "unsigned int";
	return this->writeData(&v[0], type, dims, name);
}

int HDF5Writer::write(const std::vector<long>& v, const std::string& name)
{
	std::vector<size_type> dims(1);
	dims[0] = v.size();
	const std::string type = "long";
	return this->writeData(&v[0], type, dims, name);
}

int HDF5Writer::write(const std::vector<double>& v, const std::string& name)
{
	std::vector<size_type> dims(1);
	dims[0] = v.size();
	const std::string type = "double";
	return this->writeData(&v[0], type, dims, name);
}

int HDF5Writer::write(const std::vector<float>& v, const std::string& name)
{
	std::vector<size_type> dims(1);
	dims[0] = v.size();
	const std::string type = "float";
	return this->writeData(&v[0], type, dims, name);
}

int HDF5Writer::write(const std::vector<unsigned char>& v,
					  const std::string& name)
{
	std::vector<size_type> dims(1);
	dims[0] = v.size();
	const std::string type = "unsigned char";
	return this->writeData(&v[0], type, dims, name);
}

int HDF5Writer::write(const std::vector<unsigned short>& v,
					  const std::string& name)
{
	std::vector<size_type> dims(1);
	dims[0] = v.size();
	const std::string type = "unsigned short";
	return this->writeData(&v[0], type, dims, name);
}

int HDF5Writer::write_attribute(const std::string& value,
								const std::string& name)
{
	int result = 0;

	std::vector<std::string> path;
	boost::algorithm::split(path, name, boost::algorithm::is_any_of("/"),
							boost::algorithm::token_compress_off);

	std::string attribute_name = path.back();
	path.resize(path.size() - 1);
	std::string dataset_name;
	for (size_t i = 0; i < path.size(); ++i)
	{
		if (path[i].empty())
			continue;
		dataset_name += "/" + path[i];
	}
	if (dataset_name.empty())
	{
		dataset_name = "/";
	}

	hid_t parent_handle = file;
	hid_t group_andle;
	std::string parent_path = "";
	for (size_t iter_path = 0; iter_path < path.size(); ++iter_path)
	{
		if (!path[iter_path].empty())
		{
			parent_path += path[iter_path];
			if (H5Lexists(parent_handle, parent_path.c_str(), H5P_DEFAULT) <= 0)
			{
				if (group_andle < 0)
				{
					break;
				}

				if (iter_path < path.size() - 1)
				{
					// close it again except the last compartment
					return 0;
				}
			}
			else
			{
				// compartment exists
				if (iter_path == path.size() - 1)
				{
					// open compartment if last location exists
					group_andle = H5Gopen(parent_handle, parent_path.c_str());
				}
			}
		}
		parent_path += "/";
	}

	//hid_t dataset_id = H5Dopen2(file, "/Tissues", H5P_DEFAULT);
	if (group_andle >= 0)
	{
		//  Note: HDF5 insists on writing a 0 byte when given an
		//  empty string. Reading will then return a size of 1!
		//
		hid_t dataspace_id = H5Screate(H5S_SCALAR);
		if (dataspace_id >= 0)
		{
			hid_t tid1 = H5Tcopy(H5T_C_S1);
			herr_t ret = H5Tset_size(tid1, value.length() + 1);
			&ret;
			hid_t attribute_id = H5Acreate(group_andle, attribute_name.c_str(), tid1,
										   dataspace_id, H5P_DEFAULT);
			if (attribute_id >= 0)
			{
				herr_t status = H5Awrite(attribute_id, tid1, value.c_str());
				result = (status == 0);
				status = H5Aclose(attribute_id);
			}
			else
			{
				return 0;
			}
			H5Tclose(tid1);
			H5Sclose(dataspace_id);
		}
		else
		{
			return 0;
		}

		herr_t status = H5Gclose(group_andle);
	}
	else
	{
		return 0;
	}

	return result;
}

int HDF5Writer::writeData(const void* data, const std::string& type,
						  const std::vector<size_type>& extents,
						  const std::string& name) const
{
	if (name.empty())
	{
		std::cerr << "HDF5Writer::write() : error, no dataname given" << std::endl;
		return 0;
	}
	hid_t datatype, plist;
	herr_t status;

	if (file < 0)
	{
		std::cerr << "HDF5Writer::write() : error, no files open" << std::endl;
		return 0;
	}
	else if (loud)
	{
		std::cerr << "HDF5Writer::write() : writing to file " << file << std::endl;
	}

	if (!data)
	{
		std::cerr << "HDF5Writer::write() : error, no pointer to data : " << name
				  << std::endl;
		return 0;
	}

	int ok = 1;
	int elemsize = 0;
	// write data
	const int rank = static_cast<int>(extents.size());
	if (type == "double")
	{
		datatype = H5Tcopy(H5T_NATIVE_DOUBLE);
		elemsize = sizeof(double);
	}
	else if (type == "float")
	{
		datatype = H5Tcopy(H5T_NATIVE_FLOAT);
		elemsize = sizeof(float);
	}
	else if (type == "int")
	{
		datatype = H5Tcopy(H5T_NATIVE_INT);
		elemsize = sizeof(int);
	}
	else if (type == "unsigned int")
	{
		datatype = H5Tcopy(H5T_NATIVE_UINT);
		elemsize = sizeof(unsigned int);
	}
	else if (type == "long")
	{
		datatype = H5Tcopy(H5T_NATIVE_LONG);
		elemsize = sizeof(long);
	}
	else if (type == "unsigned char")
	{
		datatype = H5Tcopy(H5T_NATIVE_UCHAR);
		elemsize = sizeof(unsigned char);
	}
	else if (type == "unsigned short")
	{
		datatype = H5Tcopy(H5T_NATIVE_USHORT);
		elemsize = sizeof(unsigned short);
	}
	else if (type == "char")
	{
		datatype = H5Tcopy(H5T_NATIVE_CHAR);
		elemsize = sizeof(char);
	}
	else
	{
		std::cerr << "HDF5Writer::write() : error, unsupported data type"
				  << std::endl;
		throw std::runtime_error(
			"HDF5Writer::write() : error, unsupported data type");
	}

	status = H5Tset_order(datatype, H5T_ORDER_LE);
	std::vector<hsize_t> dimsf(rank);
	if (loud)
	{
		std::cerr << "HDF5Writer::write() rank = " << extents.size() << std::endl;
	}
	for (int j = 0; j < rank; j++)
	{
		if (loud)
		{
			std::cerr << "HDF5Writer::write() extents[" << j << "] = " << extents[j]
					  << std::endl;
		}
		if (ordering == "reverse" || ordering == "matlab")
		{
			dimsf[j] = extents[rank - 1 - j];
		}
		else //natural
		{
			dimsf[j] = extents[j];
		}
	}
	if (compression)
	{
		// hsize_t cdims[rank];
		std::vector<hsize_t> cdims(rank);
		bool chunk_ok = !chunkSize.empty();
		if (!chunkSize.empty())
		{
			assert(chunkSize.size() == rank);
			for (int j = 0; j < rank; j++)
			{
				if (chunkSize[j] <= 0 || chunkSize[j] > dimsf[j])
				{
					if (loud)
						std::cerr << "HDF5Writer::write() : invalid chunk size"
								  << std::endl;
					chunk_ok = false;
				}
				cdims[j] = chunkSize[j];
			}
		}
		if (!chunk_ok)
		{
			if (loud)
			{
				std::cerr << "HDF5Writer::write() : using automatic chunk size with "
							 "buffer size "
						  << bufsize << std::endl;
			}
			for (int j = 0; j < rank; j++)
			{
				cdims[j] = std::min(dimsf[j], hsize_t(bufsize / elemsize));
			}
		}
		plist = H5Pcreate(H5P_DATASET_CREATE);
		H5Pset_chunk(plist, rank, cdims.data());
		H5Pset_deflate(plist, compression);
	}
	hid_t dataspace = H5Screate_simple(rank, dimsf.data(), NULL);
	if (dataspace >= 0)
	{
		hid_t dataset;
		if (compression)
		{
			dataset = H5Dcreate2(file, name.c_str(), datatype, dataspace, H5P_DEFAULT,
								 plist, H5P_DEFAULT);
		}
		else
		{
			dataset = H5Dcreate2(file, name.c_str(), datatype, dataspace, H5P_DEFAULT,
								 H5P_DEFAULT, H5P_DEFAULT);
		}
		if (dataset < 0)
		{
			ok = 0;
		}
		else if (dataset >= 0 && data != 0)
		{
			if (loud)
			{
				std::cerr << "HDF5Writer : writing " << name << " of type " << type
						  << std::endl;
			}

			if (type == "double")
			{
				status = H5Dwrite(dataset, H5T_NATIVE_DOUBLE, H5S_ALL, H5S_ALL,
								  H5P_DEFAULT, (double*)data);
			}
			else if (type == "float")
			{
				status = H5Dwrite(dataset, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL,
								  H5P_DEFAULT, (float*)data);
			}
			else if (type == "int")
			{
				status = H5Dwrite(dataset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL,
								  H5P_DEFAULT, (int*)data);
			}
			else if (type == "unsigned int")
			{
				status = H5Dwrite(dataset, H5T_NATIVE_UINT, H5S_ALL, H5S_ALL,
								  H5P_DEFAULT, (unsigned int*)data);
			}
			else if (type == "long")
			{
				status = H5Dwrite(dataset, H5T_NATIVE_LONG, H5S_ALL, H5S_ALL,
								  H5P_DEFAULT, (long*)data);
			}
			else if (type == "unsigned char")
			{
				status = H5Dwrite(dataset, H5T_NATIVE_UCHAR, H5S_ALL, H5S_ALL,
								  H5P_DEFAULT, (unsigned char*)data);
			}
			else if (type == "unsigned short")
			{
				status = H5Dwrite(dataset, H5T_NATIVE_USHORT, H5S_ALL, H5S_ALL,
								  H5P_DEFAULT, (unsigned short*)data);
			}
			else if (type == "char")
			{
				status = H5Dwrite(dataset, H5T_NATIVE_CHAR, H5S_ALL, H5S_ALL,
								  H5P_DEFAULT, (char*)data);
			}
			else
			{
				std::cerr << "HDF5Writer::write() : error, unsupported data type"
						  << std::endl;
				throw std::runtime_error(
					"HDF5Writer::write() : error, unsupported data type");
			}

			if (status < 0)
			{
				std::cerr << "HDF5Writer::write() : writing dataset failed"
						  << std::endl;
				ok = 0;
			}
			else if (loud)
			{
				std::cerr << "HDF5Writer::write() : " << name << " writen succesfully"
						  << std::endl;
			}

			H5Dclose(dataset);
		}
		H5Sclose(dataspace);
	}

	if (compression)
	{
		H5Pclose(plist);
	}
	H5Tclose(datatype);

	return ok;
}

std::vector<std::string> HDF5Writer::tokenize(std::string& line, char delim)
{
	std::stringstream ss(line); // Insert the std::string into a stream

	std::vector<std::string> tokens;
	char buf[256];
	while (ss.getline(buf, 256, delim))
	{
		std::string ts(buf);
		if (!ts.empty())
		{
			tokens.push_back(ts);
		}
	}
	//	if(loud)
	//	{
	std::cerr << "HDF5Writer::tokenize() : " << tokens.size() << std::endl;
	for (int i = 0; i < tokens.size(); i++)
	{
		std::cerr << "\t" << tokens[i].length() << " " << tokens[i] << std::endl;
	}
	//	}
	return tokens;
}

} // namespace iseg
