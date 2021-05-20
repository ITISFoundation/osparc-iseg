/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

namespace iseg {

class ISEG_CORE_API UndoQueue
{
public:
	UndoQueue();
	~UndoQueue();
	void AddUndo(UndoElem* ue);
	void MergeUndo(UndoElem* ue);
	bool AddUndo(MultiUndoElem* ue);
	UndoElem* Undo();
	UndoElem* Redo();
	void ClearUndo();
	unsigned ReturnNrredo() const;
	unsigned ReturnNrundo() const;
	unsigned ReturnNrundoarraysmax() const;
	unsigned ReturnNrundomax() const;
	void SetNrundoarraysmax(unsigned nr);
	void SetNrundo(unsigned nr);
	void ReverseUndosliceorder(unsigned short nrslices);

private:
	unsigned m_Nrundo;
	unsigned m_Nrundoarraysmax;
	void SubAddUndo(UndoElem* ue);
	unsigned m_Nrundoarrays;
	std::vector<UndoElem*> m_Undos;
	unsigned m_First;
	unsigned m_Nrnow;
	unsigned m_Nrin;
};

} // namespace iseg
