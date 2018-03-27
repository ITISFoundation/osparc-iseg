/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "HDF5IO.h"

#include <hdf5.h>

HDF5IO::HDF5IO(int compression) : CompressionLevel(compression) {}

bool HDF5IO::existsValidHdf5(const std::string &fname)
{
	htri_t status = H5Fis_hdf5(fname.c_str());
	return (status > 0);
}

HDF5IO::handle_id_type HDF5IO::open(const std::string &fname)
{
	if (!existsValidHdf5(fname.c_str()))
	{
		return -1;
	}
	return H5Fopen(fname.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
}

HDF5IO::handle_id_type HDF5IO::create(const std::string &fname, bool append)
{
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
		return H5Fcreate(fname.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
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
