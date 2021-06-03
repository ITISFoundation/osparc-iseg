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

#include "HDF5Blosc.h"

#include <hdf5.h>
#ifdef USE_HDF5_BLOSC
#	include <blosc_filter.h>
#endif

#include <cstdint>
#include <string>

namespace iseg {

class ISEG_CORE_API HDF5IO
{
public:
	using handle_id_type = hid_t;

	int m_ChunkSize = 0;

	HDF5IO(int compression = -1);

	bool ExistsValidHdf5(const std::string& fname);

	handle_id_type Open(const std::string& fname);

	handle_id_type Create(const std::string& fname, bool append = false);

	bool Close(handle_id_type fid);

	template<typename T>
	handle_id_type GetTypeValue();

	template<typename T>
	bool ReadData(handle_id_type file_id, const std::string& name, size_t arg_offset, size_t arg_length, T* data_out);

	template<typename T>
	bool WriteData(handle_id_type file_id, const std::string& name, T** const slice_data, size_t num_slices, size_t slice_size, size_t offset = 0);

	static std::string DumpErrorStack();

protected:
	int m_CompressionLevel;
};

} // namespace iseg

#include "HDF5IO.inl"
