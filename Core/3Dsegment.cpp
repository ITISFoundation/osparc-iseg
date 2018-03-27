/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
//#include "stdafx.h"
//#include "3Dsegment.h"
//
//int 3Dhandler::ReadRaw(const char *filename, short unsigned w, short unsigned h, unsigned bitdepth, unsigned slicenr, unsigned short nrofslices)
//{
//    FILE             *fp;          /* Open file pointer */
//    int              bitsize;      /* Size of bitmap */
//
//    if ((fp = fopen(filename, "rb")) == NULL)
//        return (NULL);
//
//	width=w;
//	height=h;
//	area=height*(unsigned int)width;
//
//	bitsize = (int) area;
//
//    if ((bmp_bits = (float *) malloc(nrofslices*bitsize*sizeof(float))) == NULL)
//    {
//        fclose(fp);
//        return (NULL);
//    }
//
//	if ((work_bits = (float *) malloc(nrofslices*bitsize*sizeof(float))) == NULL)
//    {
//        fclose(fp);
//		free(bmp_bits);
//        return (NULL);
//    }
//
//	if ((help_bits = (float *) malloc(nrofslices*bitsize*sizeof(float))) == NULL)
//    {
//        fclose(fp);
//		free(bmp_bits);
//		free(work_bits);
//        return (NULL);
//    }
//
// 	unsigned bytedepth=(bitdepth+7)/8;
//
//	if(bytedepth==1){
//		unsigned char	 *bits_tmp;
//
//	    if ((bits_tmp = (unsigned char *) malloc(nrofslices*bitsize)) == NULL)
//		{
//			free(bmp_bits);
//			free(work_bits);
//			free(help_bits);
//			fclose(fp);
//			return (NULL);
//		}
//
//		int result = fseek(fp, (unsigned long)(bitsize)*slicenr, SEEK_SET);
//		if( result ){
//			free(bmp_bits);
//			free(work_bits);
//			free(help_bits);
//			free(bits_tmp);
//			fclose(fp);
//			return (NULL);
//		}
//
//		if(fread(bits_tmp,1,nrofslices*bitsize,fp)<nrofslices*area)
//		{
//			free(bmp_bits);
//			free(work_bits);
//			free(help_bits);
//			free(bits_tmp);
//			fclose(fp);
//			return (NULL);
//		}
//
//		for(int i=0;i<nrofslices*bitsize;i++){
//			work_bits[i]=bmp_bits[i]=(float) bits_tmp[i];
//		}
//
//		free(bits_tmp);
//	}
//	else if(bytedepth==2) {
//		unsigned short	 *bits_tmp;
//
//	    if ((bits_tmp = (unsigned short *) malloc(nrofslices*bitsize*2)) == NULL)
//		{
//			free(bmp_bits);
//			free(work_bits);
//			free(help_bits);
//			fclose(fp);
//			return (NULL);
//		}
//
//		int result = fseek(fp, (unsigned long)(bitsize)*2*slicenr, SEEK_SET);
//		if( result ){
//			free(bmp_bits);
//			free(work_bits);
//			free(help_bits);
//			free(bits_tmp);
//			fclose(fp);
//			return (NULL);
//		}
//
//		if(fread(bits_tmp,1,bitsize*2,fp)<area*2)
//		{
//			free(bmp_bits);
//			free(work_bits);
//			free(help_bits);
//			free(bits_tmp);
//			fclose(fp);
//			return (NULL);
//		}
//
//		for(int i=0;i<bitsize;i++){
//			work_bits[i]=bmp_bits[i]=(float) bits_tmp[i];
//		}
//
//		free(bits_tmp);
//	}
//	else {
//		free(bmp_bits);
//		free(work_bits);
//		free(help_bits);
//		fclose(fp);
//		return (NULL);
//	}
//
//    fclose(fp);
//	return 1;
//}
//
//int 3Dhandler::ReloadRaw(const char *filename, unsigned bitdepth, unsigned slicenr, unsigned short nrofslices)
//{
//    FILE             *fp;          /* Open file pointer */
//    int              bitsize;      /* Size of bitmap */
//
//    if ((fp = fopen(filename, "rb")) == NULL)
//        return (NULL);
//
//	bitsize = (int) area;
//
// 	unsigned bytedepth=(bitdepth+7)/8;
//
//	if(bytedepth==1){
//		unsigned char	 *bits_tmp;
//
//	    if ((bits_tmp = (unsigned char *) malloc(bitsize)) == NULL)
//		{
//			fclose(fp);
//			return (NULL);
//		}
//
//		int result = fseek(fp, (unsigned long)(bitsize)*2*slicenr, SEEK_SET);
//		if( result ){
//			free(bits_tmp);
//			fclose(fp);
//			return (NULL);
//		}
//
//		if(fread(bits_tmp,1,bitsize,fp)<area)
//		{
//			free(bits_tmp);
//			fclose(fp);
//			return (NULL);
//		}
//
//		for(int i=0;i<bitsize;i++){
//			bmp_bits[i]=(float) bits_tmp[i];
//		}
//
//		free(bits_tmp);
//	}
//	else if(bytedepth==2) {
//		unsigned short	 *bits_tmp;
//
//	    if ((bits_tmp = (unsigned short *) malloc(bitsize*2)) == NULL)
//		{
//			fclose(fp);
//			return (NULL);
//		}
//
//		int result = fseek(fp, (unsigned long)(bitsize)*2*slicenr, SEEK_SET);
//		if( result ){
//			free(bits_tmp);
//			fclose(fp);
//			return (NULL);
//		}
//
//		if(fread(bits_tmp,1,bitsize*2,fp)<area*2)
//		{
//			free(bits_tmp);
//			fclose(fp);
//			return (NULL);
//		}
//
//		for(int i=0;i<bitsize;i++){
//			bmp_bits[i]=(float) bits_tmp[i];
//		}
//
//		free(bits_tmp);
//	}
//	else {
//		fclose(fp);
//		return (NULL);
//	}
//
//    fclose(fp);
//	return 1;
//}
//
//int 3Dhandler::SaveBmpRaw(const char *filename)
//{
//	return SaveRaw(filename,bmp_bits);
//}
//
//int 3Dhandler::SaveWorkRaw(const char *filename)
//{
//	return SaveRaw(filename,work_bits);
//}
//
//int 3Dhandler::SaveRaw(const char *filename, float *p_bits)
//{
//	FILE             *fp;
//	unsigned char *	 bits_tmp;
//
//	bits_tmp=(unsigned char *)malloc(area);
//	if(bits_tmp==NULL) return -1;
//
//	for(unsigned int i=0;i<area;i++){
//		bits_tmp[i]=unsigned char(min(255,max(0,p_bits[i]+0.5)));
//	}
//
//	if ((fp = fopen(filename, "wb")) == NULL) return (-1);
//
//	unsigned int bitsize=width*(unsigned)height;
//
//    if (fwrite(bits_tmp, 1, bitsize, fp) < (unsigned int)bitsize) {
//        fclose(fp);
//        return (-1);
//    }
//
//	free(bits_tmp);
//
//    fclose(fp);
//    return 0;
//}
