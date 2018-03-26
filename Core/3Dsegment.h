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

#include <iostream>
#include <algorithm>

class handler_3D
{
public:
	int ReadRaw(const char *filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, unsigned short nrofslices);
	int ReloadRaw(const char *filename, unsigned bitdepth, unsigned short slicenr);
	int SaveBmpRaw(const char *filename);
	int SaveWorkRaw(const char *filename);

private:
	short unsigned width;
	short unsigned height;
	short unsigned nrslices;
	unsigned int area;

	int SaveRaw(const char *filename, float *p_bits);

	unsigned int histogram[256];
	float *bmp_bits;
	float *work_bits;
	float *help_bits;
};


int handler_3D::ReadRaw(const char *filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned short slicenr, unsigned short nrofslices)
{
	FILE             *fp;          /* Open file pointer */
	int              bitsize;      /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == NULL)
		return (NULL);

	width = w;
	height = h;
	area = height*(unsigned int)width;
	nrslices = nrofslices;

	bitsize = (int)area;

	if ((bmp_bits = (float *)malloc(nrofslices*bitsize * sizeof(float))) == NULL)
	{
		fclose(fp);
		return (NULL);
	}

	if ((work_bits = (float *)malloc(nrofslices*bitsize * sizeof(float))) == NULL)
	{
		fclose(fp);
		free(bmp_bits);
		return (NULL);
	}

	if ((help_bits = (float *)malloc(nrofslices*bitsize * sizeof(float))) == NULL)
	{
		fclose(fp);
		free(bmp_bits);
		free(work_bits);
		return (NULL);
	}

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1) {
		unsigned char	 *bits_tmp;

		if ((bits_tmp = (unsigned char *)malloc(nrofslices*bitsize)) == NULL)
		{
			free(bmp_bits);
			free(work_bits);
			free(help_bits);
			fclose(fp);
			return (NULL);
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (unsigned long)(bitsize)*slicenr, SEEK_SET);
#else
		int result = fseek(fp, (unsigned long)(bitsize)*slicenr, SEEK_SET);
#endif
		if (result) {
			free(bmp_bits);
			free(work_bits);
			free(help_bits);
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		if (fread(bits_tmp, 1, nrofslices*bitsize, fp) < nrofslices*area)
		{
			free(bmp_bits);
			free(work_bits);
			free(help_bits);
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		for (int i = 0; i < nrofslices*bitsize; i++) {
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2) {
		unsigned short	 *bits_tmp;

		if ((bits_tmp = (unsigned short *)malloc(nrofslices*bitsize * 2)) == NULL)
		{
			free(bmp_bits);
			free(work_bits);
			free(help_bits);
			fclose(fp);
			return (NULL);
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (unsigned long)(bitsize) * 2 * slicenr, SEEK_SET);
#else
		int result = fseek(fp, (unsigned long)(bitsize) * 2 * slicenr, SEEK_SET);
#endif
		if (result) {
			free(bmp_bits);
			free(work_bits);
			free(help_bits);
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		if (fread(bits_tmp, 1, nrofslices*bitsize * 2, fp) < nrofslices*area * 2)
		{
			free(bmp_bits);
			free(work_bits);
			free(help_bits);
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		for (int i = 0; i < nrofslices*bitsize; i++) {
			work_bits[i] = bmp_bits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else {
		free(bmp_bits);
		free(work_bits);
		free(help_bits);
		fclose(fp);
		return (NULL);
	}

	fclose(fp);
	return 1;
}

int handler_3D::ReloadRaw(const char *filename, unsigned bitdepth, unsigned short slicenr)
{
	FILE             *fp;          /* Open file pointer */
	int              bitsize;      /* Size of bitmap */

	if ((fp = fopen(filename, "rb")) == NULL)
		return (NULL);

	bitsize = (int)area;

	unsigned bytedepth = (bitdepth + 7) / 8;

	if (bytedepth == 1) {
		unsigned char	 *bits_tmp;

		if ((bits_tmp = (unsigned char *)malloc(nrslices*bitsize)) == NULL)
		{
			fclose(fp);
			return (NULL);
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (unsigned long)(bitsize) * 2 * slicenr, SEEK_SET);
#else
		int result = fseek(fp, (unsigned long)(bitsize) * 2 * slicenr, SEEK_SET);
#endif
		if (result) {
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		if (fread(bits_tmp, 1, nrslices*bitsize, fp) < area)
		{
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		for (int i = 0; i < nrslices*bitsize; i++) {
			bmp_bits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else if (bytedepth == 2) {
		unsigned short	 *bits_tmp;

		if ((bits_tmp = (unsigned short *)malloc(nrslices*bitsize * 2)) == NULL)
		{
			fclose(fp);
			return (NULL);
		}

#ifdef _MSC_VER
		int result = _fseeki64(fp, (unsigned long)(bitsize) * 2 * slicenr, SEEK_SET);
#else
		int result = fseek(fp, (unsigned long)(bitsize) * 2 * slicenr, SEEK_SET);
#endif
		if (result) {
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		if (fread(bits_tmp, 1, nrslices*bitsize * 2, fp) < area * 2)
		{
			free(bits_tmp);
			fclose(fp);
			return (NULL);
		}

		for (int i = 0; i < nrslices*bitsize; i++) {
			bmp_bits[i] = (float)bits_tmp[i];
		}

		free(bits_tmp);
	}
	else {
		fclose(fp);
		return (NULL);
	}

	fclose(fp);
	return 1;
}

int handler_3D::SaveBmpRaw(const char *filename)
{
	return SaveRaw(filename, bmp_bits);
}

int handler_3D::SaveWorkRaw(const char *filename)
{
	return SaveRaw(filename, work_bits);
}

int handler_3D::SaveRaw(const char *filename, float *p_bits)
{
	FILE             *fp;
	unsigned char *	 bits_tmp;

	bits_tmp = (unsigned char *)malloc(nrslices*area);
	if (bits_tmp == NULL) return -1;

	for (unsigned int i = 0; i < nrslices*area; i++) {
		bits_tmp[i] = (unsigned char)(std::min(255.0f, std::max(0.0f, p_bits[i] + 0.5f)));
	}

	if ((fp = fopen(filename, "wb")) == NULL) return (-1);

	unsigned int bitsize = width*(unsigned)height;

	if (fwrite(bits_tmp, 1, bitsize, fp) < (unsigned int)bitsize) {
		fclose(fp);
		return (-1);
	}

	free(bits_tmp);

	fclose(fp);
	return 0;
}

