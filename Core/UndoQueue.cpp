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

#include "UndoQueue.h"

using namespace std;
using namespace iseg;

UndoQueue::UndoQueue()
{
	first = nrnow = nrin = nrundoarrays = 0;
	nrundo = 50;
	nrundoarraysmax = 20;
	undos.resize(nrundo);
}

UndoQueue::~UndoQueue()
{
	for (unsigned i = 0; i < nrin; i++)
		delete undos[(first + i) % nrundo];
}

void UndoQueue::sub_add_undo(UndoElem* ue)
{
	if (nrnow == nrundo)
	{
		nrundoarrays = (nrundoarrays + ue->arraynr()) - undos[first]->arraynr();
		delete undos[first];
		undos[first] = ue;
		first = (first + 1) % nrundo;
		while (nrundoarrays > nrundoarraysmax)
		{
			nrundoarrays -= undos[first]->arraynr();
			delete undos[first];
			first = (first + 1) % nrundo;
			nrnow--;
		}
	}
	else
	{
		nrundoarrays += ue->arraynr();

		for (unsigned i = nrnow; i < nrin; i++)
		{
			nrundoarrays -= undos[(first + i) % nrundo]->arraynr();
			delete undos[(first + i) % nrundo];
		}
		undos[(first + nrnow) % nrundo] = ue;
		nrnow++;

		while (nrundoarrays > nrundoarraysmax)
		{
			nrundoarrays -= undos[first]->arraynr();
			delete undos[first];
			first = (first + 1) % nrundo;
			nrnow--;
		}
	}
	nrin = nrnow;
}

void UndoQueue::merge_undo(UndoElem* ue)
{
	if (!ue->multi)
	{
		if (nrin > 0)
		{
			nrundoarrays -= undos[(first + nrin - 1) % nrundo]->arraynr();
			undos[(first + nrin - 1) % nrundo]->merge(ue);
			nrundoarrays += undos[(first + nrin - 1) % nrundo]->arraynr();

			while (nrundoarrays > nrundoarraysmax)
			{
				nrundoarrays -= undos[first]->arraynr();
				delete undos[first];
				first = (first + 1) % nrundo;
				nrnow--;
			}
		}
		nrin = nrnow;
	}
}

void UndoQueue::add_undo(UndoElem* ue) { sub_add_undo(ue); }

bool UndoQueue::add_undo(MultiUndoElem* ue)
{
	if (ue->arraynr() < nrundoarraysmax)
	{
		sub_add_undo(ue);
		return true;
	}
	else
	{
		return false;
	}
}

UndoElem* UndoQueue::undo()
{
	if (nrnow > 0)
	{
		return undos[((--nrnow) + first) % nrundo];
	}
	else
		return nullptr;
}

UndoElem* UndoQueue::redo()
{
	if (nrnow < nrin)
	{
		return undos[((nrnow++) + first) % nrundo];
	}
	else
		return nullptr;
}

void UndoQueue::clear_undo()
{
	for (unsigned i = 0; i < nrin; i++)
		delete undos[(first + i) % nrundo];
	first = nrnow = nrin = nrundoarrays = 0;
	return;
}

unsigned UndoQueue::return_nrundo() { return nrnow; }

unsigned UndoQueue::return_nrredo() { return nrin - nrnow; }

unsigned UndoQueue::return_nrundoarraysmax() { return nrundoarraysmax; }

unsigned UndoQueue::return_nrundomax() { return nrundo; }

void UndoQueue::set_nrundoarraysmax(unsigned nr)
{
	if (nrundoarraysmax != nr)
	{
		nrundoarraysmax = nr;

		while (nrundoarrays > nrundoarraysmax && nrnow > 0)
		{
			nrundoarrays -= undos[first]->arraynr();
			delete undos[first];
			first = (first + 1) % nrundo;
			nrin--;
			nrnow--;
		}

		while (nrundoarrays > nrundoarraysmax && nrin > 0)
		{
			nrin--;
			nrundoarrays -= undos[(first + nrin) % nrundo]->arraynr();
			delete undos[(first + nrin) % nrundo];
		}
	}
}

void UndoQueue::set_nrundo(unsigned nr)
{
	if (nr != nrundo)
	{
		while (nrin > nr && nrnow > 0)
		{
			nrundoarrays -= undos[first]->arraynr();
			delete undos[first];
			first = (first + 1) % nrundo;
			nrnow--;
			nrin--;
		}

		while (nrin > nr)
		{
			nrin--;
			nrundoarrays -= undos[(first + nrin) % nrundo]->arraynr();
			delete undos[(first + nrin) % nrundo];
		}

		vector<UndoElem*> vue;
		vue.clear();
		vue.insert(vue.begin(), undos.begin(), undos.end());

		undos.resize(nr);

		for (unsigned i = 0; i < nrin; i++)
		{
			undos[i] = vue[(first + i) % nrundo];
		}

		this->nrundo = nr;

		first = 0;
	}
}

void UndoQueue::reverse_undosliceorder(unsigned short nrslices)
{
	for (unsigned i = 0; i < nrin; i++)
		undos[i]->dataSelection.sliceNr =
			nrslices - 1 - undos[i]->dataSelection.sliceNr;
}
