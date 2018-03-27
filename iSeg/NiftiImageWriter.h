/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef NIFTIIMAGEWRITER_H
#define NIFTIIMAGEWRITER_H

#include "common.h"
#include <SetGetMacros.h>

class NiftiImageWriter
{
public:
	NiftiImageWriter();
	~NiftiImageWriter();
	SetStringMacro(FileName);
	GetStringMacro(FileName);
	SetMacro(NumberOfSlices, unsigned);
	GetMacro(NumberOfSlices, unsigned);
	SetMacro(Width, unsigned);
	GetMacro(Width, unsigned);
	SetMacro(Height, unsigned);
	GetMacro(Height, unsigned);
	SetMacro(PixelSize, float *);
	GetMacro(PixelSize, float *);
	SetMacro(Offset, float *);
	GetMacro(Offset, float *);
	SetMacro(Quaternion, float *);
	GetMacro(Quaternion, float *);

	template<typename T> int Write(T **data, bool labelfield = false);

protected:
	char *FileName;
	unsigned NumberOfSlices;
	unsigned Width;
	unsigned Height;
	float *PixelSize;
	float *Offset;
	float *Quaternion;
};

#endif // NIFTIIMAGEWRITER_H
