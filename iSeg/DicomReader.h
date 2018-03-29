/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef dicomread_22Juli05
#define dicomread_22Juli05

#include "Core/Pair.h"
#include "Core/Point.h"

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace iseg {

const unsigned length = 1000; // Arbitrary buffer size

class dicomread
{
public:
	bool opendicom(const char* filename);
	void closedicom();
	unsigned short get_width();
	unsigned short get_height();
	bool load_picture(float* bits);
	bool load_pictureGDCM(const char* filename, float* bits);
	bool load_picture(float* bits, float center, float contrast);
	bool load_picture(float* bits, Point p, unsigned short dx,
					  unsigned short dy);
	bool load_picture(float* bits, float center, float contrast, Point p,
					  unsigned short dx, unsigned short dy);
	float slicepos();
	unsigned seriesnr();
	float thickness();
	Pair pixelsize();
	bool imagepos(float f[3]);
	bool imageorientation(float f[6]);

private:
	FILE* fp;
	unsigned short width;
	unsigned short height;
	unsigned short bitdepth;
	unsigned short get_bitdepth();
	unsigned char buffer[length];
	unsigned char buffer1[4];
	bool read_field(unsigned a, unsigned b);
	bool read_fieldnr(unsigned& a, unsigned& b);
	bool read_fieldcontent();
	bool next_field();
	unsigned read_length();
	void go_start();
	bool go_label(unsigned a, unsigned b);
	float ds2float();
	void dsarray2float(float* vals, unsigned short nrvals);
	float ds2float(int& pos);
	int is2int();
	int bin2int();
	unsigned int l;
	unsigned int depth;
};

} // namespace iseg

#endif
