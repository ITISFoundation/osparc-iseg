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
#include "UndoElem.h"

class iSegCore_API UndoQueue
{
public:
	UndoQueue();
	~UndoQueue();
	void add_undo(UndoElem *ue);
	void merge_undo(UndoElem *ue);
	bool add_undo(MultiUndoElem *ue);
	UndoElem *undo();
	UndoElem *redo();
	void clear_undo();
	unsigned return_nrredo();
	unsigned return_nrundo();
	unsigned return_nrundoarraysmax();
	unsigned return_nrundomax();
	void set_nrundoarraysmax(unsigned nr);
	void set_nrundo(unsigned nr);
	void reverse_undosliceorder(unsigned short nrslices);

private:
	unsigned nrundo;
	unsigned nrundoarraysmax;
	void sub_add_undo(UndoElem *ue);
	unsigned nrundoarrays;
	std::vector<UndoElem *>undos;
	unsigned first;
	unsigned nrnow;
	unsigned nrin;
};
