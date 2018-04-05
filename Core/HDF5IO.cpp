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

#include <hdf5.h>

#include <sstream>

namespace iseg {

namespace {

herr_t H5Ewalk_cb(int n, H5E_error_t* err_desc, void* client_data)
{
	std::ostream* stream = static_cast<std::ostream*>(client_data);
	*stream << H5Eget_major(err_desc->maj_num) << " - " << H5Eget_minor(err_desc->min_num);
	if (err_desc->desc)
		*stream << ": " << err_desc->desc;
	*stream << "\n";
	return 0;
}

} // namespace

HDF5IO::HDF5IO(int compression) : CompressionLevel(compression) {}

bool HDF5IO::existsValidHdf5(const std::string& fname)
{
	htri_t status = H5Fis_hdf5(fname.c_str());
	return (status > 0);
}

HDF5IO::handle_id_type HDF5IO::open(const std::string& fname)
{
	if (!existsValidHdf5(fname.c_str()))
	{
		return -1;
	}
	return H5Fopen(fname.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
}

HDF5IO::handle_id_type HDF5IO::create(const std::string& fname, bool append)
{
	static_assert(std::is_same<HDF5IO::handle_id_type, hid_t>::value, "hid_t mismatch. this will lead to runtime errors.");
	if (append && !existsValidHdf5(fname.c_str()))
	{
		append = false;
	}
	if (append)
	{
		return H5Fopen(fname.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
	}
	else
	{
		return H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT,
						 H5P_DEFAULT);
	}
}

bool HDF5IO::close(handle_id_type file)
{
	if (file >= 0)
	{
		herr_t status = H5Fclose(file);
		return (status >= 0);
	}
	return true;
}

std::string HDF5IO::dumpErrorStack()
{
	std::stringstream ss;
	ss << "HDF5 error(s):"
	   << "\r\n";
	H5Ewalk(H5E_WALK_DOWNWARD, H5Ewalk_cb, &ss);
	return ss.str();
}

} // namespace iseg
