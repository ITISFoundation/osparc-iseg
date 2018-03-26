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
#include "stepsmooth.h"
#include <cstdlib>
#include <cstdio>

stepsmooth::stepsmooth()
{
	mask=NULL;
	ownmask=false;
	masklength=0;
	weights=NULL;
	return;
}

stepsmooth::~stepsmooth()
{
	for(tissues_size_t i=0;i<nrtissues;i++) {
		delete[] weights[i];
	}
	delete[] weights;
	if(ownmask) delete[] mask;
	return;
}

void stepsmooth::dostepsmooth(tissues_size_t *line)
{
	if(mask==NULL||weights==NULL) return;
	if(linelength<masklength||linelength<2) return;

	/*int *counter=new int[linelength];
	for(unsigned short i=0;i<linelength;i++) counter[i]=0;
	float *counterfloat=new float[linelength];
	for(unsigned short i=0;i<linelength;i++) counterfloat[i]=0;*/

	unsigned short n1=masklength/2;
	for(tissues_size_t c=0;c<nrtissues;c++) {
		for(unsigned short i=0;i<linelength;i++) weights[c][i]=0;//,counter[i]=0,counterfloat[i]=0;
	}
	unsigned short upper=linelength+1-masklength;
	for(unsigned short i=0;i<upper;i++) {
		tissues_size_t index=line[i+n1];
		for(unsigned short j=0;j<masklength;j++) {
			weights[index][i+j]+=mask[j];//,counter[i+j]++,counterfloat[i+j]+=mask[j];
		}

	}
	for(unsigned short i=0;i<n1;i++) {
		tissues_size_t index=line[i];
		unsigned short k=0;
		for(unsigned short j=n1-i;j<masklength;j++,k++) {
			weights[index][k]+=mask[j];//,counter[k]++,counterfloat[k]+=mask[j];
		}

	}
	for(unsigned short i=linelength-masklength+n1+1;i<linelength;i++) {
		tissues_size_t index=line[i];
		unsigned short k=i-n1;
		for(unsigned short j=0;k<linelength;j++,k++) {
			weights[index][k]+=mask[j];//,counter[k]++,counterfloat[k]+=mask[j];
		}
	}
	tissues_size_t index=line[0];
	upper=masklength-n1-1;
	for(unsigned short i=0;i<upper;i++) {
		for(unsigned short j=i+n1+1;j<masklength;j++) {
			weights[index][i]+=mask[j];//,counter[i]++,counterfloat[i]+=mask[j];
		}
	}
	index=line[linelength-1];
	for(unsigned short i=linelength-n1,upper=1;i<linelength;i++,upper++) {
		for(unsigned short j=0;j<upper;j++) {
			weights[index][i]+=mask[j];//,counter[i]++,counterfloat[i]+=mask[j];
		}
	}
	for(unsigned short i=0;i<linelength;i++) {
		float valmax=0;
		for(tissues_size_t c=0;c<nrtissues;c++) {
			if(weights[c][i]>valmax) {
				line[i]=c;
				valmax=weights[c][i];
			}
		}
	}
	//FILE *fp3=fopen("C:\\test.txt","w");
	//for(unsigned short i=0;i<linelength;i++) fprintf(fp3,"%i %f \n",counter[i],counterfloat[i]);
	//fclose(fp3);
	//delete[] counter;
	//delete[] counterfloat;
}

void stepsmooth::init(float *mask1, unsigned short masklength1, unsigned short linelength1, tissues_size_t nrtissues1)
{
	if(ownmask) delete[] mask;
	ownmask=false;
	mask=mask1;
	masklength=masklength1;
	linelength=linelength1;
	if(weights!=NULL) {
		for(tissues_size_t i=0;i<nrtissues;i++) {
			delete[] weights[i];
		}
		delete[] weights;
	}
	nrtissues=nrtissues1;
	weights=new float*[nrtissues];
	for(tissues_size_t i=0;i<nrtissues;i++) {
		weights[i]=new float[linelength];
	}
}

void stepsmooth::init(unsigned short masklength1, unsigned short linelength1, tissues_size_t nrtissues1)
{
	if(ownmask) delete[] mask;
	mask=new float[masklength1];
	masklength=masklength1;
	generate_binommask();
	ownmask=true;
	linelength=linelength1;
	if(weights!=NULL) {
		for(tissues_size_t i=0;i<nrtissues;i++) {
			delete[] weights[i];
		}
		delete[] weights;
	}
	nrtissues=nrtissues1;
	weights=new float*[nrtissues];
	for(tissues_size_t i=0;i<nrtissues;i++) {
		weights[i]=new float[linelength];
	}
}

void stepsmooth::generate_binommask()
{
	mask[0]=1.0f;
	for(unsigned short i=1;i<masklength;i++) {
		mask[i]=1.0;
		for(unsigned short j=i-1;j>0;j--) {
			mask[j]+=mask[j-1];
		}
	}
	mask[masklength/2]+=0.1;
}