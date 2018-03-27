/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <stdexcept>
#include <vector>

#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QDomText>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>

#include <vtkSmartPointer.h>

#include <NIFTIIO/nifti1_io.h>
//#include <HDF5IO/HDF5Reader.h>

#include "NiftiImageWriter.h"

using namespace std;
using namespace HDF5;

NiftiImageWriter::NiftiImageWriter()
{
	this->NumberOfSlices = 0;
	this->Width = 0;
	this->Height = 0;
	this->PixelSize = 0;
	this->Offset = 0;
	this->Quaternion = 0;
	this->FileName = 0;
}

NiftiImageWriter::~NiftiImageWriter() { delete[] this->FileName; }

template<typename T> int NiftiImageWriter::Write(T **data, bool labelfield)
{
	cerr << "NiftiImageWriter::Write()" << endl;

	// Delete existing file, otherwise nifti_set_filenames will fail
	remove(this->FileName);

	// Create nifti image
	int datatype;
	if (typeid(T) == typeid(unsigned char)) { /* unsigned char (8 bits/voxel) */
		datatype = DT_UINT8;
	}
	else if (typeid(T) == typeid(short)) { /* signed short (16 bits/voxel) */
		datatype = DT_INT16;
	}
	else if (typeid(T) == typeid(int)) { /* signed int (32 bits/voxel)   */
		datatype = DT_INT32;
	}
	else if (typeid(T) == typeid(float)) { /* float (32 bits/voxel)        */
		datatype = DT_FLOAT32;
	}
	else if (typeid(T) == typeid(double)) { /* double (64 bits/voxel)       */
		datatype = DT_FLOAT64;
	}
	else if (typeid(T) == typeid(char)) { /* signed char (8 bits)         */
		datatype = DT_INT8;
	}
	else if (typeid(T) ==
					 typeid(unsigned short)) { /* unsigned short (16 bits)     */
		datatype = DT_UINT16;
	}
	else if (typeid(T) ==
					 typeid(unsigned int)) { /* unsigned int (32 bits)       */
		datatype = DT_UINT32;
	}
	else if (typeid(T) == typeid(long long)) { /* long long (64 bits)          */
		datatype = DT_INT64;
	}
	else if (typeid(T) ==
					 typeid(unsigned long long)) { /* unsigned long long (64 bits) */
		datatype = DT_UINT64;
	}
	else {
		cerr << "error, unsupported grid type: " << typeid(T).name() << endl;
		return 0;
	}

	int dims[8] = {3, this->Width, this->Height, this->NumberOfSlices, 1, 1, 1,
								 1};
	nifti_image *im = nifti_make_new_nim(dims, datatype, 1);
	if (!im) {
		cerr << "error, failed to alloc nifti_image" << endl;
		return 0;
	}

	// TODO: tissue list in im->aux_file

	strcpy(im->descrip, "iSeg");
	im->xyz_units = NIFTI_UNITS_MM;
	im->time_units = NIFTI_UNITS_SEC;

	// Grid spacings
	im->qfac = -1.0;
	im->pixdim[0] = im->qfac;
	im->pixdim[1] = this->PixelSize[0];
	im->pixdim[2] = this->PixelSize[1];
	im->pixdim[3] = this->PixelSize[2];
	im->pixdim[4] = 0.0;
	im->pixdim[5] = 1.0;
	im->pixdim[6] = 1.0;
	im->pixdim[7] = 1.0;
	nifti_update_dims_from_array(im);

	// Datafield intent
	if (labelfield) {
		im->intent_code = NIFTI_INTENT_LABEL;
	}
	else {
		im->intent_code = NIFTI_INTENT_NONE;
	}

	// Dataset transform
	im->sform_code = NIFTI_XFORM_UNKNOWN;
	im->qform_code = NIFTI_XFORM_SCANNER_ANAT;

	im->qoffset_x = this->Offset[0];
	im->qoffset_y = this->Offset[1];
	im->qoffset_z = this->Offset[2];

	// Normalize quaternion
	float tmp = sqrt(this->Quaternion[0] * this->Quaternion[0] +
									 this->Quaternion[1] * this->Quaternion[1] +
									 this->Quaternion[2] * this->Quaternion[2] +
									 this->Quaternion[3] * this->Quaternion[3]);
	for (unsigned short i = 0; i < 4; ++i) {
		this->Quaternion[i] /= tmp;
	}
	im->quatern_b = this->Quaternion[1];
	im->quatern_c = this->Quaternion[2];
	im->quatern_d = this->Quaternion[3];

	if (nifti_set_filenames(im, this->FileName, 1, 1)) {
		cerr << "error, could not set filename" << endl;
		return 0;
	}

	// Copy pixel data
	// Warning: This assumes that im->data has the same data type as the input data.
	T *copyTo = (T *)im->data;
	size_t N = this->Width * this->Height;
	for (int k = 0; k < this->NumberOfSlices; k++) {
		T *copyFrom = data[k];
		for (size_t i = 0; i < N; ++i) {
			*copyTo++ = *copyFrom++;
		}
	}

	cerr << "nifti_image_write..." << endl;
	cerr << this->FileName << endl;

	nifti_image_write(im);

	cerr << "done.\n" << endl;

	nifti_image_free(im);

	return 1;
}

template int NiftiImageWriter::Write<float>(float **data, bool labelfield);
template int NiftiImageWriter::Write<tissues_size_t>(tissues_size_t **data,
																										 bool labelfield);