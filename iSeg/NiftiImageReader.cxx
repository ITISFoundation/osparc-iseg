/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <vector>
#include <stdexcept>

#include <QString>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTextStream>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>

#include <vtkSmartPointer.h>

#include <NIFTIIO/nifti1_io.h>
#include <HDF5IO/HDF5Reader.h>

#include "NiftiImageReader.h"

using namespace std;
using namespace HDF5;

NiftiImageReader::NiftiImageReader()
{
	this->HeaderNotRead = true;
	this->ReadSliceStart = 0;
	this->ReadSliceCount = 0;
	this->NumberOfSlices = 0;
	this->Width = 0;
	this->Height = 0;
	this->Compression = 0;
	this->PixelSize = new float[3];
	this->Offset = new float[3];
	this->Quaternion = new float[6];
	this->ImageSlices = 0;
	this->WorkSlices= 0;
	this->TissueSlices = 0;
	this->FileName = 0;
}



NiftiImageReader::~NiftiImageReader()
{
	delete [] this->FileName;
	delete [] this->Offset;
	delete [] this->PixelSize;
	delete [] this->Quaternion;
}

int NiftiImageReader::ReadHeader()
{
	cerr << "NiftiImageReader::ReadHeader()" << endl;

	cerr << "nifti_image_read..." << endl;
	cerr << this->FileName << endl;
	nifti_image *im = nifti_image_read(this->FileName, false);
	cerr << "done.\n" << endl;

	cerr << "analyze75_orient: " 	<< im->analyze75_orient << endl;
	cerr << "aux_file: " 			<< im->aux_file << endl;
	cerr << "byteorder: " 			<< im->byteorder << endl;
	cerr << "cal_max: "				<< im->cal_max << endl;
	cerr << "cal_min: "				<< im->cal_min << endl;
	cerr << "datatype: "			<< im->datatype << endl;
	cerr << "descrip: "				<< im->descrip << endl;
	cerr << "dim[0..7] = ("			<< im->dim[0] << ", " << im->dim[1] << ", " << im->dim[2] << ", " << im->dim[3] << ", " << im->dim[4] << ", " << im->dim[5] << ", " << im->dim[6] << ", " << im->dim[7] << ")" << endl;
	cerr << "(dx, dy, dz, dt, du, dv, dw) = (" << im->dx << ", " << im->dy << ", " << im->dz << ", " << im->dt << ", " << im->du << ", " << im->dv << ", " << im->dw << ")" << endl;
	cerr << "freq_dim: "			<< im->freq_dim << endl;
	cerr << "intent_code: "			<< im->intent_code << endl;
	cerr << "intent_name: "			<< im->intent_name << endl;
	cerr << "intent_p1: "			<< im->intent_p1 << endl;
	cerr << "intent_p2: "			<< im->intent_p2 << endl;
	cerr << "intent_p3: "			<< im->intent_p3 << endl;
	cerr << "nbyper: "				<< im->nbyper << endl;
	cerr << "ndim: "				<< im->ndim << endl;
	cerr << "nifti_type: "			<< im->nifti_type << endl;
	cerr << "(nx, ny, nz, nt, nu, nv, nw) = (" << im->nx << ", " << im->ny << ", " << im->nz << ", " << im->nt << ", " << im->nu << ", " << im->nv << ", " << im->nw << ")" << endl;
	cerr << "num_ext: "				<< im->num_ext << endl;
	cerr << "nvox: "				<< im->nvox << endl;
	cerr << "phase_dim: "			<< im->phase_dim << endl;
	cerr << "qfac: "				<< im->qfac << endl;
	cerr << "qform_code: "			<< im->qform_code << endl;
	cerr << "qoffset_x: "			<< im->qoffset_x << endl;
	cerr << "qoffset_y: "			<< im->qoffset_y << endl;
	cerr << "qoffset_z: "			<< im->qoffset_z << endl;
	cerr << "scl_inter: "			<< im->scl_inter << endl;
	cerr << "scl_slope: "			<< im->scl_slope << endl;
	cerr << "sform_code: "			<< im->sform_code << endl;
	cerr << "slice_code: "			<< im->slice_code << endl;
	cerr << "slice_dim: "			<< im->slice_dim << endl;
	cerr << "slice_duration: "		<< im->slice_duration << endl;
	cerr << "slice_end: "			<< im->slice_end << endl;
	cerr << "slice_start: "			<< im->slice_start << endl;
	cerr << "swapsize: "			<< im->swapsize << endl;
	cerr << "time_units: "			<< im->time_units << endl;
	cerr << "toffset: "				<< im->toffset << endl;
	cerr << "xyz_units: "			<< im->xyz_units << endl;

	bool is3d = true;
	for (unsigned int i = 4; i <= im->ndim; ++i) {
		is3d &= (im->dim[i] == 1);
	}
	if (!is3d) {
		cerr << "warning, invalid dimensions (ndim = " << im->ndim << ")" << endl;
		cerr << "data read only for (t, u, v, w) = (1, 1, 1, 1)" << endl;
	}

	// Convert pixel size units to mm
	float scaling = 1.0f;
	unsigned short unit = im->xyz_units & 0x07;
	switch (unit)
	{
	case NIFTI_UNITS_METER:
		scaling = 1000.0f;
		break;
	case NIFTI_UNITS_MM:
		scaling = 1.0f;
		break;
	case NIFTI_UNITS_MICRON:
		scaling = 0.001f;
		break;
	}
	cerr << "Unit scaling: " << scaling << endl;

	// Dataset transform
	if (im->qform_code == NIFTI_XFORM_UNKNOWN && im->sform_code == NIFTI_XFORM_UNKNOWN)
	{
		this->Offset[0] = this->Offset[1] = this->Offset[2] = 0.0f;
		this->Quaternion[0] = 1.0f;
		this->Quaternion[1] = this->Quaternion[2] = this->Quaternion[3] = 0.0f;
		this->PixelSize[0] = im->dx * scaling;
		this->PixelSize[1] = im->dy * scaling;
		this->PixelSize[2] = im->dz * scaling;
	}
	else
	{
		this->Offset[0] = im->qoffset_x * scaling;
		this->Offset[1] = im->qoffset_y * scaling;
		this->Offset[2] = im->qoffset_z * scaling;

		this->PixelSize[0] = im->dx * scaling;
		this->PixelSize[1] = im->dy * scaling;
		this->PixelSize[2] = im->dz * scaling;

		float tmp = im->quatern_b*im->quatern_b+im->quatern_c*im->quatern_c+im->quatern_d*im->quatern_d;
		if (tmp < 1.0f) {
			this->Quaternion[0] = sqrt(1.0 - tmp);
		} else {
			this->Quaternion[0] = 0.0f; // tmp can be slightly larger than 1.0 due to precision error
		}
		this->Quaternion[1] = im->quatern_b;
		this->Quaternion[2] = im->quatern_c;
		this->Quaternion[3] = im->quatern_d;

		if (im->qform_code != NIFTI_XFORM_UNKNOWN && im->sform_code == NIFTI_XFORM_UNKNOWN) {
			cerr << "warning, dataset rotation ignored." << endl;
		} else {
			cerr << "warning, dataset transform not supported." << endl;
		}
	}

	// Dataset dimensions
	this->Width = im->nx;
	this->Height = im->ny;
	this->NumberOfSlices = im->nz;
	const size_t N = (size_t)this->Width*(size_t)this->Height*(size_t)this->NumberOfSlices;

	cerr << "Width = " << this->Width << endl;
	cerr << "Height = " << this->Height << endl;
	cerr << "NumberOfSlices = " << this->NumberOfSlices << endl;
	cerr << "Total size = " << N << endl;

	this->HeaderNotRead = false;

	return 1;
}

int NiftiImageReader::Read()
{
	// Read Nifti header
	if (this->HeaderNotRead && !ReadHeader()) {
		return 0;
	}

	return Read(0, this->NumberOfSlices);
}

int NiftiImageReader::Read(unsigned short startslice, unsigned short nrslices)
{
	cerr << "NiftiImageReader::Read()" << endl;
	this->ReadSliceStart = startslice;
	this->ReadSliceCount = nrslices;

	// Read Nifti header
	if (this->HeaderNotRead && !ReadHeader()) {
		return 0;
	}

	if (this->ReadSliceStart + this->ReadSliceCount > this->NumberOfSlices) {
		return 0;
	}

	// Read image data
	cerr << "nifti_image_read..." << endl;
	nifti_image *im = nifti_image_read(this->FileName, true);

	// https://brainder.org/2012/09/23/the-nifti-file-format/
	if( im->dim[0] > 3 )
	{
		bool thereIsMore = true;
		thereIsMore = true;
	}

	cerr << "done.\n" << endl;

	if (im->intent_code == NIFTI_INTENT_LABEL)
	{
		// Tissue
		cerr << "reading tissue data of grid type: " << nifti_datatype_string(im->datatype) << "..." << endl;
		switch(im->datatype)
		{
		case DT_UINT8: /* unsigned char (8 bits/voxel) */
			ReadData((unsigned char*) im->data, this->TissueSlices);
			break;
		case DT_INT16: /* signed short (16 bits/voxel) */
			ReadData((short*) im->data, this->TissueSlices);
			break;
		case DT_INT32: /* signed int (32 bits/voxel)   */
			ReadData((int*) im->data, this->TissueSlices);
			break;
		case DT_FLOAT32: /* float (32 bits/voxel)        */
			ReadData((float*) im->data, this->TissueSlices);
			break;
		case DT_FLOAT64: /* double (64 bits/voxel)       */
			ReadData((double*) im->data, this->TissueSlices);
			break;
		case DT_INT8: /* signed char (8 bits)         */
			ReadData((char*) im->data, this->TissueSlices);
			break;
		case DT_UINT16: /* unsigned short (16 bits)     */
			ReadData((unsigned short*) im->data, this->TissueSlices);
			break;
		case DT_UINT32: /* unsigned int (32 bits)       */
			ReadData((unsigned int*) im->data, this->TissueSlices);
			break;
		case DT_INT64: /* long long (64 bits)          */
			ReadData((long long*) im->data, this->TissueSlices);
			break;
		case DT_UINT64: /* unsigned long long (64 bits) */
			ReadData((unsigned long long*) im->data, this->TissueSlices);
			break;
		default:
			cerr << "error, unsupported grid type: " << nifti_datatype_string(im->datatype) << endl;
			return 0;
		}
		// TODO: Load tissue list from im->aux_file
	}
	else
	{
		// Source
		cerr << "reading image data of grid type: " << nifti_datatype_string(im->datatype) << "..." << endl;
		switch(im->datatype)
		{
		case DT_UINT8: /* unsigned char (8 bits/voxel) */
			ReadData((unsigned char*) im->data, this->ImageSlices);
			break;
		case DT_INT16: /* signed short (16 bits/voxel) */
			ReadData((short*) im->data, this->ImageSlices);
			break;
		case DT_INT32: /* signed int (32 bits/voxel)   */
			ReadData((int*) im->data, this->ImageSlices);
			break;
		case DT_FLOAT32: /* float (32 bits/voxel)        */
			ReadData((float*) im->data, this->ImageSlices);
			break;
		case DT_FLOAT64: /* double (64 bits/voxel)       */
			ReadData((double*) im->data, this->ImageSlices);
			break;
		case DT_INT8: /* signed char (8 bits)         */
			ReadData((char*) im->data, this->ImageSlices);
			break;
		case DT_UINT16: /* unsigned short (16 bits)     */
			ReadData((unsigned short*) im->data, this->ImageSlices);
			break;
		case DT_UINT32: /* unsigned int (32 bits)       */
			ReadData((unsigned int*) im->data, this->ImageSlices);
			break;
		case DT_INT64: /* long long (64 bits)          */
			ReadData((long long*) im->data, this->ImageSlices);
			break;
		case DT_UINT64: /* unsigned long long (64 bits) */
			ReadData((unsigned long long*) im->data, this->ImageSlices);
			break;
		case DT_RGB: /* RGB triple (24 bits) */
			ReadDataRGB((unsigned char*) im->data, this->ImageSlices);
			break;
		default:
			cerr << "error, unsupported grid type: " << nifti_datatype_string(im->datatype) << endl;
			return 0;
		}
	}
	cerr << "done.\n" << endl;

	nifti_image_free(im);

	return 1;
}

template <typename InType, typename OutType>
void NiftiImageReader::ReadData(const InType *in, OutType **out)
{
	size_t n = this->ReadSliceStart * this->Height * this->Width;
	for(int k=0; k<this->ReadSliceCount; k++)
	{
		size_t pos = 0;
		for(int j=0; j<this->Height; j++)
		{
			for(int i=0; i<this->Width; i++,pos++)
			{
				out[k][pos] = in[n];
				n++;
			}
		}
	}
}

void NiftiImageReader::ReadDataRGB(const unsigned char *in, float **out)
{
	size_t n = 3 * this->ReadSliceStart * this->Height * this->Width;
	for(int k=0; k<this->ReadSliceCount; k++)
	{
		size_t pos = 0;
		for(int j=0; j<this->Height; j++)
		{
			for(int i=0; i<this->Width; i++,pos++)
			{
				// Combine RGB triple to single value of OutType
				float tmp = in[n++] << 16;
				tmp += in[n++] << 8;
				tmp += in[n++];
				out[k][pos] = tmp;
			}
		}
	}
}