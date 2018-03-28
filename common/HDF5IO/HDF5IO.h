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

#include <cstdint>
#include <string>

#include "HDF5IOApi.h"

class HDF5_EXPORT HDF5IO
{
public:
	typedef long handle_id_type;

	int chunk_size = 0;

	HDF5IO(int compression = -1);

	bool existsValidHdf5(const std::string& fname);

	handle_id_type open(const std::string& fname);

	handle_id_type create(const std::string& fname, bool append = false);

	bool close(handle_id_type fid);

	template<typename T> handle_id_type getTypeValue();

	template<typename T>
	bool readData(handle_id_type file_id, const std::string& name,
				  size_t arg_offset, size_t arg_length, T* data_out);

	template<typename T>
	bool writeData(handle_id_type file_id, const std::string& name,
				   T** const slice_data, size_t num_slices, size_t slice_size,
				   size_t offset = 0);

protected:
	int CompressionLevel;
};

#include "HDF5IO.inl"
