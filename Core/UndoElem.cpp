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
#include "UndoElem.h"

#include <vector>
#include <cstdlib>

using namespace std;

UndoElem::UndoElem()
{
	bmp_old=work_old=bmp_new=work_new=NULL;
	tissue_old=tissue_new=NULL;
	mode1_old=mode1_new=mode2_old=mode2_new=0;
	multi=false;
}

UndoElem::~UndoElem()
{
	if(bmp_old!=NULL) free(bmp_old);
	if(work_old!=NULL) free(work_old);
	if(tissue_old!=NULL) free(tissue_old);
	if(bmp_new!=NULL) free(bmp_new);
	if(work_new!=NULL) free(work_new);
	if(tissue_new!=NULL) free(tissue_new);
}

void UndoElem::merge(UndoElem *ue)
{
	if(dataSelection.sliceNr==ue->dataSelection.sliceNr&&!multi){
		if(ue->dataSelection.bmp){
			if(dataSelection.bmp) free(bmp_new);
			else {
				bmp_old=ue->bmp_old;
				mode1_old=ue->mode1_old;
			}
			mode1_new=ue->mode1_new;
			bmp_new=ue->bmp_new;
		}
		if(ue->dataSelection.work){
			if(dataSelection.work) free(work_new);
			else {
				work_old=ue->work_old;
				mode2_old=ue->mode2_old;
			}
			mode2_new=ue->mode2_new;
			work_new=ue->work_new;
		}
		if(ue->dataSelection.tissues){
			if(dataSelection.tissues) free(tissue_new);
			else tissue_old=ue->tissue_old;
			tissue_new=ue->tissue_new;
		}
		if(ue->dataSelection.vvm){
			vvm_new.clear();
			vvm_new=ue->vvm_new;
			if(!dataSelection.vvm){
				vvm_old.clear();
				vvm_old=ue->vvm_old;
			}
		}
		if(ue->dataSelection.limits){
			limits_new.clear();
			limits_new=ue->limits_new;
			if(!dataSelection.limits){
				limits_old.clear();
				limits_old=ue->limits_old;
			}
		}
		if(ue->dataSelection.marks){
			marks_new.clear();
			marks_new=ue->marks_new;
			if(!dataSelection.marks){
				marks_old.clear();
				marks_old=ue->marks_old;
			}
		}
		dataSelection.CombineSelection(ue->dataSelection);
	} 
/*	if(bmp_new!=NULL) free(bmp_new);
	if(work_new!=NULL) free(work_new);
	if(tissue_new!=NULL) free(tissue_new);
	bmp_new=ue->bmp_new;
	work_new=ue->work_new;
	tissue_new=ue->tissue_new;
	vvm_new.clear();
	limits_new.clear();
	marks_new.clear();
	vvm_new=ue->vvm_new;
	limits_new=ue->limits_new;
	marks_new=ue->marks_new;*/
}

unsigned UndoElem::arraynr()
{
	unsigned i=0;
	const unsigned added=1;

	if(dataSelection.bmp){
		i+=added;
	}
	if(dataSelection.work){
		i+=added;
	}
	if(dataSelection.tissues){
		i+=added;
	}

	return i;
}

MultiUndoElem::MultiUndoElem()
{
	multi=true;
}

MultiUndoElem::~MultiUndoElem()
{
	vector<float *>::iterator itf;
	vector<tissues_size_t *>::iterator it8;
	
	for(itf=vbmp_old.begin();itf!=vbmp_old.end();itf++) free(*itf);
	for(itf=vwork_old.begin();itf!=vwork_old.end();itf++) free(*itf);
	for(it8=vtissue_old.begin();it8!=vtissue_old.end();it8++) free(*it8);
	for(itf=vbmp_new.begin();itf!=vbmp_new.end();itf++) free(*itf);
	for(itf=vwork_new.begin();itf!=vwork_new.end();itf++) free(*itf);
	for(it8=vtissue_new.begin();it8!=vtissue_new.end();it8++) free(*it8);
}

void MultiUndoElem::merge(UndoElem *ue)
{
}

unsigned MultiUndoElem::arraynr()
{
	unsigned i=0;
	const unsigned added=1;

	if(dataSelection.bmp){
		i+=added;
	}
	if(dataSelection.work){
		i+=added;
	}
	if(dataSelection.tissues){
		i+=added;
	}

	return i*vslicenr.size();
}
