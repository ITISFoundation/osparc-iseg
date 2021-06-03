/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef dicomread_22Juli05
#define dicomread_22Juli05

#include "Data/Point.h"

#include "Core/Pair.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace iseg {

const unsigned length = 1000; // Arbitrary buffer size

class DicomReader
{
public:
	bool Opendicom(const char* filename);
	void Closedicom();
	unsigned short GetWidth();
	unsigned short GetHeight();
	bool LoadPicture(float* bits);
	bool LoadPictureGdcm(const char* filename, float* bits);
	bool LoadPicture(float* bits, float center, float contrast);
	bool LoadPicture(float* bits, Point p, unsigned short dx, unsigned short dy);
	bool LoadPicture(float* bits, float center, float contrast, Point p, unsigned short dx, unsigned short dy);
	float Slicepos();
	unsigned Seriesnr();
	float Thickness();
	Pair Pixelsize();
	bool Imagepos(float f[3]);
	bool Imageorientation(float f[6]);

private:
	FILE* m_Fp;
	unsigned short m_Width;
	unsigned short m_Height;
	unsigned short m_Bitdepth;
	unsigned short GetBitdepth();
	unsigned char m_Buffer[length];
	unsigned char m_Buffer1[4];
	bool ReadField(unsigned a, unsigned b);
	bool ReadFieldnr(unsigned& a, unsigned& b);
	bool ReadFieldcontent();
	bool NextField();
	unsigned ReadLength();
	void GoStart();
	bool GoLabel(unsigned a, unsigned b);
	float Ds2float();
	void Dsarray2float(float* vals, unsigned short nrvals);
	float Ds2float(int& pos);
	int Is2int();
	int Bin2int();
	unsigned int m_L;
	unsigned int m_Depth;
};

} // namespace iseg

#endif
