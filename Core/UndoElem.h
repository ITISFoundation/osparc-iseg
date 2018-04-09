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

#include "iSegCore.h"

#include "Mark.h"
#include "Types.h"

#include "Plugin/Point.h"
#include "Plugin/DataSelection.h"

#include <vector>

namespace iseg {

class iSegCore_API UndoElem
{
public:
	bool multi;
	DataSelection dataSelection;
	float* bmp_old;
	float* work_old;
	tissues_size_t* tissue_old;
	std::vector<std::vector<Mark>> vvm_old;
	std::vector<std::vector<Point>> limits_old;
	std::vector<Mark> marks_old;
	float* bmp_new;
	float* work_new;
	tissues_size_t* tissue_new;
	std::vector<std::vector<Mark>> vvm_new;
	std::vector<std::vector<Point>> limits_new;
	std::vector<Mark> marks_new;
	unsigned char mode1_old;
	unsigned char mode1_new;
	unsigned char mode2_old;
	unsigned char mode2_new;
	UndoElem();
	virtual ~UndoElem();
	void merge(UndoElem* ue);
	virtual unsigned arraynr();
};

class iSegCore_API MultiUndoElem : public UndoElem
{
public:
	//abcd vector<unsigned short> vslicenr;
	std::vector<unsigned> vslicenr;
	std::vector<float*> vbmp_old;
	std::vector<float*> vwork_old;
	std::vector<tissues_size_t*> vtissue_old;
	std::vector<std::vector<std::vector<Mark>>> vvvm_old;
	std::vector<std::vector<std::vector<Point>>> vlimits_old;
	std::vector<std::vector<Mark>> vmarks_old;
	std::vector<float*> vbmp_new;
	std::vector<float*> vwork_new;
	std::vector<tissues_size_t*> vtissue_new;
	std::vector<std::vector<std::vector<Mark>>> vvvm_new;
	std::vector<std::vector<std::vector<Point>>> vlimits_new;
	std::vector<std::vector<Mark>> vmarks_new;
	std::vector<unsigned char> vmode1_old;
	std::vector<unsigned char> vmode1_new;
	std::vector<unsigned char> vmode2_old;
	std::vector<unsigned char> vmode2_new;
	MultiUndoElem();
	virtual ~MultiUndoElem();
	void merge(UndoElem* ue);
	virtual unsigned arraynr();
};

} // namespace iseg
