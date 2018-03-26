/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"
#include "matexport.h"

#include <cstdio>

using namespace std;

bool matexport::print_mat(const char *filename, float *matrix,int nx, int ny, int nz, char* comment, int commentlength, char *varname, int varnamelength, bool complex)
{
	FILE *fp;
	fp=fopen(filename,"wb");
	if(fp==NULL) return false;

	unsigned char header[128];
	{
		unsigned char i=0;
		while(i<116&&i<commentlength) {
			header[i]=comment[i];
			i++;
		}
		while(i<124) {
			header[i]=' ';
			i++;
		}
		header[124]=0;
		header[125]=1;
		header[126]='I';
		header[127]='M';
	}

	fwrite(header,128,1,fp);
	__int32 val=14;
	fwrite(&val,4,1,fp);
	int namefieldlength=varnamelength;
	if(varnamelength%8!=0) namefieldlength+=8-varnamelength%8;
	if(complex)
		val=64+2*4*nx*ny*nz+namefieldlength;
	else
		val=56+4*nx*ny*nz+namefieldlength;
	fwrite(&val,4,1,fp);
	val=6;
	fwrite(&val,4,1,fp);
	val=8;
	fwrite(&val,4,1,fp);
	unsigned char c1=7;
	fwrite(&c1,1,1,fp);
	if(complex) c1=8;
	else c1=0;
	fwrite(&c1,1,1,fp);
	c1=0;
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	val=0;
	fwrite(&val,4,1,fp);
	val=5;
	fwrite(&val,4,1,fp);
	val=12;
	fwrite(&val,4,1,fp);
	val=nx;
	fwrite(&val,4,1,fp);
	val=ny;
	fwrite(&val,4,1,fp);
	val=nz;
	fwrite(&val,4,1,fp);
	val=0;
	fwrite(&val,4,1,fp);
	val=1;
	fwrite(&val,4,1,fp);
	val=varnamelength;
	fwrite(&val,4,1,fp);
	fwrite(varname,varnamelength,1,fp);
	c1=0;
	{
		for(int i=varnamelength;i%8!=0;i++) {
			fwrite(&c1,1,1,fp);
		}
	}
	val=7;
	fwrite(&val,4,1,fp);
	val=nx*ny*nz*4;
	fwrite(&val,4,1,fp);
	fwrite(matrix,nx*ny*nz*4,1,fp);
	if(nx%2==1&&ny%2==1&&nz%2==1) fwrite(&val,4,1,fp);
	if(complex) {
		val=7;
		fwrite(&val,4,1,fp);
		val=nx*ny*nz*4;
		fwrite(&val,4,1,fp);
		fwrite(&matrix[nx*ny*nz],nx*ny*nz*4,1,fp);
		if(nx%2==1&&ny%2==1&&nz%2==1) fwrite(&val,4,1,fp);
	}
	fclose(fp);
	return true;
}

bool matexport::print_matslices(const char *filename, float **matrix,int nx, int ny, int nz, char* comment, int commentlength, char *varname, int varnamelength, bool complex)
{
	FILE *fp;
	fp=fopen(filename,"wb");
	if(fp==NULL) return false;

	unsigned char header[128];
	{
		unsigned char i=0;
		while(i<116&&i<commentlength) {
			header[i]=comment[i];
			i++;
		}
		while(i<124) {
			header[i]=' ';
			i++;
		}
		header[124]=0;
		header[125]=1;
		header[126]='I';
		header[127]='M';
	}

	fwrite(header,128,1,fp);
	__int32 val=14;
	fwrite(&val,4,1,fp);
	int namefieldlength=varnamelength;
	if(varnamelength%8!=0) namefieldlength+=8-varnamelength%8;
	if(complex)
		val=64+2*4*nx*ny*nz+namefieldlength;
	else
		val=56+4*nx*ny*nz+namefieldlength;
	fwrite(&val,4,1,fp);
	val=6;
	fwrite(&val,4,1,fp);
	val=8;
	fwrite(&val,4,1,fp);
	unsigned char c1=7;
	fwrite(&c1,1,1,fp);
	if(complex) c1=8;
	else c1=0;
	fwrite(&c1,1,1,fp);
	c1=0;
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	val=0;
	fwrite(&val,4,1,fp);
	val=5;
	fwrite(&val,4,1,fp);
	val=12;
	fwrite(&val,4,1,fp);
	val=nx;
	fwrite(&val,4,1,fp);
	val=ny;
	fwrite(&val,4,1,fp);
	val=nz;
	fwrite(&val,4,1,fp);
	val=0;
	fwrite(&val,4,1,fp);
	val=1;
	fwrite(&val,4,1,fp);
	val=varnamelength;
	fwrite(&val,4,1,fp);
	fwrite(varname,varnamelength,1,fp);
	c1=0;
	{
		for(int i=varnamelength;i%8!=0;i++) {
			fwrite(&c1,1,1,fp);
		}
	}
	val=7;
	fwrite(&val,4,1,fp);
	val=nx*ny*nz*4;
	fwrite(&val,4,1,fp);
	for(int i=0;i<nz;i++) {
		fwrite(matrix[i],nx*ny*4,1,fp);
	}
	if(nx%2==1&&ny%2==1&&nz%2==1) fwrite(&val,4,1,fp);
	if(complex) {
		val=7;
		fwrite(&val,4,1,fp);
		val=nx*ny*nz*4;
		fwrite(&val,4,1,fp);
		fwrite(&matrix[nx*ny*nz],nx*ny*nz*4,1,fp);
		if(nx%2==1&&ny%2==1&&nz%2==1) fwrite(&val,4,1,fp);
	}
	fclose(fp);
	return true;
}

bool matexport::print_mat(const char *filename, unsigned char *matrix,int nx, int ny, int nz, char* comment, int commentlength, char *varname, int varnamelength)
{
	FILE *fp;
	fp=fopen(filename,"wb");
	if(fp==NULL) return false;

	unsigned char header[128];
	{
		unsigned char i=0;
		while(i<116&&i<commentlength) {
			header[i]=comment[i];
			i++;
		}
		while(i<124) {
			header[i]=' ';
			i++;
		}
		header[124]=0;
		header[125]=1;
		header[126]='I';
		header[127]='M';
	}

	fwrite(header,128,1,fp);
	__int32 val=14;
	fwrite(&val,4,1,fp);
	int namefieldlength=varnamelength;
	if(varnamelength%8!=0) namefieldlength+=8-varnamelength%8;
	int padding=0;
	if(nx*ny*nz%8!=0) padding=8-nx*ny*nz%8;
	val=56+nx*ny*nz+namefieldlength+padding;
	fwrite(&val,4,1,fp);
	val=6;
	fwrite(&val,4,1,fp);
	val=8;
	fwrite(&val,4,1,fp);
	unsigned char c1=9;
	fwrite(&c1,1,1,fp);
	c1=0;
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	val=0;
	fwrite(&val,4,1,fp);
	val=5;
	fwrite(&val,4,1,fp);
	val=12;
	fwrite(&val,4,1,fp);
	val=nx;
	fwrite(&val,4,1,fp);
	val=ny;
	fwrite(&val,4,1,fp);
	val=nz;
	fwrite(&val,4,1,fp);
	val=0;
	fwrite(&val,4,1,fp);
	val=1;
	fwrite(&val,4,1,fp);
	val=varnamelength;
	fwrite(&val,4,1,fp);
	fwrite(varname,varnamelength,1,fp);
	c1=0;
	{
		for(int i=varnamelength;i%8!=0;i++) {
			fwrite(&c1,1,1,fp);
		}
	}
	val=2;
	fwrite(&val,4,1,fp);
	val=nx*ny*nz;
	fwrite(&val,4,1,fp);
	fwrite(matrix,nx*ny*nz,1,fp);
	c1=0;
	{
		for(int i=nx*ny*nz;i%8!=0;i++) {
			fwrite(&c1,1,1,fp);
		}
	}
	fclose(fp);
	return true;
}
bool matexport::print_matslices(const char *filename, unsigned char **matrix,int nx, int ny, int nz, char* comment, int commentlength, char *varname, int varnamelength)
{
	FILE *fp;
	fp=fopen(filename,"wb");
	if(fp==NULL) return false;

	unsigned char header[128];
	{
		unsigned char i=0;
		while(i<116&&i<commentlength) {
			header[i]=comment[i];
			i++;
		}
		while(i<124) {
			header[i]=' ';
			i++;
		}
		header[124]=0;
		header[125]=1;
		header[126]='I';
		header[127]='M';
	}

	fwrite(header,128,1,fp);
	__int32 val=14;
	fwrite(&val,4,1,fp);
	int namefieldlength=varnamelength;
	if(varnamelength%8!=0) namefieldlength+=8-varnamelength%8;
	int padding=0;
	if(nx*ny*nz%8!=0) padding=8-nx*ny*nz%8;
	val=56+nx*ny*nz+namefieldlength+padding;
	fwrite(&val,4,1,fp);
	val=6;
	fwrite(&val,4,1,fp);
	val=8;
	fwrite(&val,4,1,fp);
	unsigned char c1=9;
	fwrite(&c1,1,1,fp);
	c1=0;
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	val=0;
	fwrite(&val,4,1,fp);
	val=5;
	fwrite(&val,4,1,fp);
	val=12;
	fwrite(&val,4,1,fp);
	val=nx;
	fwrite(&val,4,1,fp);
	val=ny;
	fwrite(&val,4,1,fp);
	val=nz;
	fwrite(&val,4,1,fp);
	val=0;
	fwrite(&val,4,1,fp);
	val=1;
	fwrite(&val,4,1,fp);
	val=varnamelength;
	fwrite(&val,4,1,fp);
	fwrite(varname,varnamelength,1,fp);
	c1=0;
	for(int i=varnamelength;i%8!=0;i++) {
		fwrite(&c1,1,1,fp);
	}
	val=2;
	fwrite(&val,4,1,fp);
	val=nx*ny*nz;
	fwrite(&val,4,1,fp);
	for(int i=0;i<nz;i++) {
		fwrite(matrix[i],nx*ny,1,fp);
	}
	c1=0;
	for(int i=nx*ny*nz;i%8!=0;i++) {
		fwrite(&c1,1,1,fp);
	}
	fclose(fp);
	return true;
}

#ifdef TISSUES_SIZE_TYPEDEF
bool matexport::print_mat(const char *filename, tissues_size_t *matrix,int nx, int ny, int nz, char* comment, int commentlength, char *varname, int varnamelength)
{
	FILE *fp;
	fp=fopen(filename,"wb");
	if(fp==NULL) return false;

	// Header
	unsigned char header[128];
	unsigned char i=0;
	while(i<116&&i<commentlength) {
		header[i]=comment[i];
		i++;
	}
	while(i<124) {
		header[i]=' ';
		i++;
	}
	header[124]=0;
	header[125]=1;
	header[126]='I';
	header[127]='M';
	fwrite(header,128,1,fp);

	// Data type: miMATRIX
	__int32 val=14;
	fwrite(&val,4,1,fp);

	// Number of bytes
	int namefieldlength=varnamelength;
	if(varnamelength%8!=0) namefieldlength+=8-varnamelength%8;
	int padding=0;
	if(nx*ny*nz*sizeof(tissues_size_t)%8!=0) padding=8-nx*ny*nz*sizeof(tissues_size_t)%8;
	val=56+nx*ny*nz*sizeof(tissues_size_t)+namefieldlength+padding;
	fwrite(&val,4,1,fp);
	
	// Array flags
	val=6; // miUINT32
	fwrite(&val,4,1,fp);
	val=8; // Number of bytes
	fwrite(&val,4,1,fp);
	unsigned char c1;
	switch(sizeof(tissues_size_t)) {
	case 1:
		c1=9; // mxUINT8_CLASS
		break;
	case 2:
		c1=11; // mxUINT16_CLASS
		break;
	default:
		return false;
	}
	fwrite(&c1,1,1,fp);
	c1=0;
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	val=0;
	fwrite(&val,4,1,fp);

	// Dimensions array
	val=5; // miINT32
	fwrite(&val,4,1,fp);
	val=12; // Number of bytes
	fwrite(&val,4,1,fp);
	val=nx;
	fwrite(&val,4,1,fp);
	val=ny;
	fwrite(&val,4,1,fp);
	val=nz;
	fwrite(&val,4,1,fp);
	val=0;
	fwrite(&val,4,1,fp);

	// Array name
	val=1; // miINT8
	fwrite(&val,4,1,fp);
	val=varnamelength; // Number of bytes
	fwrite(&val,4,1,fp);
	fwrite(varname,varnamelength,1,fp);
	c1=0;
	for(int i=varnamelength;i%8!=0;i++) {
		fwrite(&c1,1,1,fp);
	}

	// Real part
	switch(sizeof(tissues_size_t)) {
	case 1:
		val=2; // miUINT8
		break;
	case 2:
		val=4; // miUINT16
		break;
	default:
		return false;
	}
	fwrite(&val,4,1,fp);
	val=nx*ny*nz*sizeof(tissues_size_t);
	fwrite(&val,4,1,fp); // Number of bytes
	fwrite(matrix,nx*ny*nz*sizeof(tissues_size_t),1,fp);
	c1=0;
	for(int i=nx*ny*nz*sizeof(tissues_size_t);i%8!=0;i++) {
		fwrite(&c1,1,1,fp);
	}

	fclose(fp);
	return true;
}
bool matexport::print_matslices(const char *filename, tissues_size_t **matrix,int nx, int ny, int nz, char* comment, int commentlength, char *varname, int varnamelength)
{
	FILE *fp;
	fp=fopen(filename,"wb");
	if(fp==NULL) return false;

	// Header
	unsigned char header[128];
	unsigned char i=0;
	while(i<116&&i<commentlength) {
		header[i]=comment[i];
		i++;
	}
	while(i<124) {
		header[i]=' ';
		i++;
	}
	header[124]=0;
	header[125]=1;
	header[126]='I';
	header[127]='M';
	fwrite(header,128,1,fp);

	// Data type: miMATRIX
	__int32 val=14;
	fwrite(&val,4,1,fp);

	// Number of bytes
	int namefieldlength=varnamelength;
	if(varnamelength%8!=0) namefieldlength+=8-varnamelength%8;
	int padding=0;
	if(nx*ny*nz*sizeof(tissues_size_t)%8!=0) padding=8-nx*ny*nz*sizeof(tissues_size_t)%8;
	val=56+nx*ny*nz*sizeof(tissues_size_t)+namefieldlength+padding;
	fwrite(&val,4,1,fp);

	// Array flags
	val=6; // miUINT32
	fwrite(&val,4,1,fp);
	val=8; // Number of bytes
	fwrite(&val,4,1,fp);
	unsigned char c1;
	switch(sizeof(tissues_size_t)) {
	case 1:
		c1=9; // mxUINT8_CLASS
		break;
	case 2:
		c1=11; // mxUINT16_CLASS
		break;
	default:
		return false;
	}
	fwrite(&c1,1,1,fp);
	c1=0;
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	fwrite(&c1,1,1,fp);
	val=0;
	fwrite(&val,4,1,fp);

	// Dimensions array
	val=5; // miINT32
	fwrite(&val,4,1,fp);
	val=12; // Number of bytes
	fwrite(&val,4,1,fp);
	val=nx;
	fwrite(&val,4,1,fp);
	val=ny;
	fwrite(&val,4,1,fp);
	val=nz;
	fwrite(&val,4,1,fp);
	val=0;
	fwrite(&val,4,1,fp);

	// Array name
	val=1; // miINT8
	fwrite(&val,4,1,fp);
	val=varnamelength; // Number of bytes
	fwrite(&val,4,1,fp);
	fwrite(varname,varnamelength,1,fp);
	c1=0;
	for(int i=varnamelength;i%8!=0;i++) {
		fwrite(&c1,1,1,fp);
	}

	// Real part
	switch(sizeof(tissues_size_t)) {
	case 1:
		val=2; // miUINT8
		break;
	case 2:
		val=4; // miUINT16
		break;
	default:
		return false;
	}
	fwrite(&val,4,1,fp);
	val=nx*ny*nz*sizeof(tissues_size_t);
	fwrite(&val,4,1,fp); // Number of bytes
	for(int i=0;i<nz;i++) {
		fwrite(matrix[i],nx*ny*sizeof(tissues_size_t),1,fp);
	}
	c1=0;
	for(int i=nx*ny*nz*sizeof(tissues_size_t);i%8!=0;i++) {
		fwrite(&c1,1,1,fp);
	}
	fclose(fp);
	return true;
}
#endif // TISSUES_SIZE_TYPEDEF
