#include "HDF5IO.h"

#include <hdf5.h>

template<typename T> HDF5IO::handle_id_type HDF5IO::getTypeValue()
{
	if (typeid(T) == typeid(double))
	{
		return H5T_NATIVE_DOUBLE;
	}
	else if (typeid(T) == typeid(float))
	{
		return H5T_NATIVE_FLOAT;
	}
	else if (typeid(T) == typeid(int))
	{
		return H5T_NATIVE_INT;
	}
	else if (typeid(T) == typeid(unsigned int))
	{
		return H5T_NATIVE_UINT;
	}
	else if (typeid(T) == typeid(long))
	{
		return H5T_NATIVE_LONG;
	}
	else if (typeid(T) == typeid(unsigned char))
	{
		return H5T_NATIVE_UCHAR;
	}
	else if (typeid(T) == typeid(unsigned short))
	{
		return H5T_NATIVE_USHORT;
	}
	return -1;
}

template<typename T>
bool HDF5IO::readData(handle_id_type file, const std::string& name,
					  size_t arg_offset, size_t arg_length, T* data_out)
{
	hid_t dataset; /* handles */
	hid_t datatype, dataspace;
	hid_t memspace;

	hsize_t dimsm[1];	/* memory space dimensions */
	hsize_t dims_out[1]; /* dataset dimensions */
	herr_t status;

	hsize_t offset_in[1];  /* size of the hyperslab in the file */
	hsize_t count[1];	  /* size of the hyperslab in the file */
	hsize_t count_out[1];  /* size of the hyperslab in memory */
	hsize_t offset_out[1]; /* size of the hyperslab in memory */
	int status_n, rank;

	/*
	* Open the the dataset.
	*/
	dataset = H5Dopen2(file, name.c_str(), H5P_DEFAULT);

	/*
	* Get datatype and dataspace handles and then query
	* dataset rank and dimensions.
	*/
	datatype = H5Dget_type(dataset);
	dataspace = H5Dget_space(dataset);
	rank = H5Sget_simple_extent_ndims(dataspace);
	status_n = H5Sget_simple_extent_dims(dataspace, dims_out, NULL);

	// TODO: FAIL_IF( rank != 1 )

	/*
	* Define hyperslab in the dataset.
	*/
	offset_in[0] = arg_offset;
	count[0] = arg_length;
	status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, offset_in, NULL,
								 count, NULL);

	/*
	* Define the memory dataspace.
	*/
	dimsm[0] = arg_length;
	memspace = H5Screate_simple(1, dimsm, NULL);

	/*
	* Define memory hyperslab.
	*/
	offset_out[0] = 0;
	count_out[0] = arg_length;
	status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET, offset_out, NULL,
								 count_out, NULL);

	/*
	* Read data from hyperslab in the file into the hyperslab in
	* memory and display.
	*/
	status = H5Dread(dataset, getTypeValue<T>(), memspace, dataspace,
					 H5P_DEFAULT, data_out);

	H5Tclose(datatype);
	H5Dclose(dataset);
	H5Sclose(dataspace);
	H5Sclose(memspace);

	return (status >= 0);
}

template<typename T>
bool HDF5IO::writeData(handle_id_type file, const std::string& name,
					   T** const slice_data, size_t num_slices,
					   size_t slice_size, size_t offset)
{
	hid_t properties = -1, datatype = -1;
	hid_t dataspace = -1, dataset = -1;
	herr_t status = 0;

	/*
	* Create a new dataset within the file using defined dataspace and
	* datatype and default dataset creation properties.
	*/
	if (H5Lexists(file, name.c_str(), H5P_DEFAULT) <= 0)
	{
		/*
		* Describe the size of the array and create the data space for fixed
		* size dataset.
		*/
		int rank = 1;
		hsize_t dimsf[1] = {num_slices * slice_size};
		dataspace = H5Screate_simple(rank, dimsf, 0);

		/*
		* Modify dataset creation properties by enable chunking and gzip compression
		*/
		properties = H5Pcreate(H5P_DATASET_CREATE);

		hsize_t const mega = 1024 * 1024;
		hsize_t const giga = 1024 * mega;
		hsize_t dim_chunks[1] = {
			chunk_size == 0 ? std::min<hsize_t>(slice_size, giga / sizeof(T))
							: chunk_size};
		H5Pset_chunk(properties, rank, dim_chunks);
		if (CompressionLevel >= 0) // if negative: disable compression
		{
			H5Pset_deflate(properties, std::min(CompressionLevel, 9));
		}

		/*
		* Define datatype for the data in the file.
		* We will store little endian numbers.
		*/
		datatype = H5Tcopy(getTypeValue<T>());
		status = H5Tset_order(datatype, H5T_ORDER_LE);

		dataset =
			H5Dcreate(file, name.c_str(), datatype, dataspace, properties);
	}
	else // open existing
	{
		dataset = H5Dopen(file, name.c_str());
		if (dataset >= 0)
		{
			dataspace = H5Dget_space(dataset);
		}
	}

	if (dataset >= 0 && status >= 0 && slice_data)
	{
		size_t current_offset = offset;
		for (size_t i = 0; i < num_slices; i++, current_offset += slice_size)
		{
			if (slice_data[i] == nullptr)
				continue;

			hsize_t dim_offset[1] = {current_offset};
			hsize_t dim_slab[1] = {slice_size};

			status = H5Sselect_hyperslab(dataspace, H5S_SELECT_SET, dim_offset,
										 0, dim_slab, 0);
			if (status >= 0)
			{
				status = H5Sselect_valid(dataspace);
				if (status >= 0)
				{
					int mem_rank = 1;
					hsize_t dim_mem[1] = {slice_size};
					//We create the dataspace in the memory
					hid_t memspace = H5Screate_simple(mem_rank, dim_mem, 0);
					if (memspace >= 0)
					{
						//We select the slab in memory
						hsize_t dim_memoffset[1] = {0};
						hsize_t dim_memslab[1] = {slice_size};
						status = H5Sselect_hyperslab(memspace, H5S_SELECT_SET,
													 dim_memoffset, 0,
													 dim_memslab, 0);
						if (status >= 0)
						{
							status = H5Sselect_valid(memspace);
							if (status >= 0)
							{
								status = H5Dwrite(dataset, getTypeValue<T>(),
												  memspace, dataspace,
												  H5P_DEFAULT, slice_data[i]);
							}
						}

						status = H5Sclose(memspace);
					}
				}
			}
		}
	}

	/*
	* Close/release resources.
	*/
	if (dataspace >= 0)
		H5Sclose(dataspace);
	if (dataset >= 0)
		H5Dclose(dataset);
	if (datatype >= 0)
		H5Tclose(datatype);
	if (properties >= 0)
		H5Pclose(properties);

	return (status >= 0);
}
