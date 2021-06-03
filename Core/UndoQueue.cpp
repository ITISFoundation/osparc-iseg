/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "UndoQueue.h"

namespace iseg {

UndoQueue::UndoQueue()
{
	m_First = m_Nrnow = m_Nrin = m_Nrundoarrays = 0;
	m_Nrundo = 50;
	m_Nrundoarraysmax = 20;
	m_Undos.resize(m_Nrundo);
}

UndoQueue::~UndoQueue()
{
	for (unsigned i = 0; i < m_Nrin; i++)
		delete m_Undos[(m_First + i) % m_Nrundo];
}

void UndoQueue::SubAddUndo(UndoElem* ue)
{
	if (m_Nrnow == m_Nrundo)
	{
		m_Nrundoarrays = (m_Nrundoarrays + ue->Arraynr()) - m_Undos[m_First]->Arraynr();
		delete m_Undos[m_First];
		m_Undos[m_First] = ue;
		m_First = (m_First + 1) % m_Nrundo;
		while (m_Nrundoarrays > m_Nrundoarraysmax)
		{
			m_Nrundoarrays -= m_Undos[m_First]->Arraynr();
			delete m_Undos[m_First];
			m_First = (m_First + 1) % m_Nrundo;
			m_Nrnow--;
		}
	}
	else
	{
		m_Nrundoarrays += ue->Arraynr();

		for (unsigned i = m_Nrnow; i < m_Nrin; i++)
		{
			m_Nrundoarrays -= m_Undos[(m_First + i) % m_Nrundo]->Arraynr();
			delete m_Undos[(m_First + i) % m_Nrundo];
		}
		m_Undos[(m_First + m_Nrnow) % m_Nrundo] = ue;
		m_Nrnow++;

		while (m_Nrundoarrays > m_Nrundoarraysmax)
		{
			m_Nrundoarrays -= m_Undos[m_First]->Arraynr();
			delete m_Undos[m_First];
			m_First = (m_First + 1) % m_Nrundo;
			m_Nrnow--;
		}
	}
	m_Nrin = m_Nrnow;
}

void UndoQueue::MergeUndo(UndoElem* ue)
{
	if (!ue->Multi())
	{
		if (m_Nrin > 0)
		{
			m_Nrundoarrays -= m_Undos[(m_First + m_Nrin - 1) % m_Nrundo]->Arraynr();
			m_Undos[(m_First + m_Nrin - 1) % m_Nrundo]->Merge(ue);
			m_Nrundoarrays += m_Undos[(m_First + m_Nrin - 1) % m_Nrundo]->Arraynr();

			while (m_Nrundoarrays > m_Nrundoarraysmax)
			{
				m_Nrundoarrays -= m_Undos[m_First]->Arraynr();
				delete m_Undos[m_First];
				m_First = (m_First + 1) % m_Nrundo;
				m_Nrnow--;
			}
		}
		m_Nrin = m_Nrnow;
	}
}

void UndoQueue::AddUndo(UndoElem* ue) { SubAddUndo(ue); }

bool UndoQueue::AddUndo(MultiUndoElem* ue)
{
	if (ue->Arraynr() < m_Nrundoarraysmax)
	{
		SubAddUndo(ue);
		return true;
	}
	else
	{
		return false;
	}
}

UndoElem* UndoQueue::Undo()
{
	if (m_Nrnow > 0)
	{
		return m_Undos[((--m_Nrnow) + m_First) % m_Nrundo];
	}
	else
		return nullptr;
}

UndoElem* UndoQueue::Redo()
{
	if (m_Nrnow < m_Nrin)
	{
		return m_Undos[((m_Nrnow++) + m_First) % m_Nrundo];
	}
	else
		return nullptr;
}

void UndoQueue::ClearUndo()
{
	for (unsigned i = 0; i < m_Nrin; i++)
		delete m_Undos[(m_First + i) % m_Nrundo];
	m_First = m_Nrnow = m_Nrin = m_Nrundoarrays = 0;
}

unsigned UndoQueue::ReturnNrundo() const { return m_Nrnow; }

unsigned UndoQueue::ReturnNrredo() const { return m_Nrin - m_Nrnow; }

unsigned UndoQueue::ReturnNrundoarraysmax() const { return m_Nrundoarraysmax; }

unsigned UndoQueue::ReturnNrundomax() const { return m_Nrundo; }

void UndoQueue::SetNrundoarraysmax(unsigned nr)
{
	if (m_Nrundoarraysmax != nr)
	{
		m_Nrundoarraysmax = nr;

		while (m_Nrundoarrays > m_Nrundoarraysmax && m_Nrnow > 0)
		{
			m_Nrundoarrays -= m_Undos[m_First]->Arraynr();
			delete m_Undos[m_First];
			m_First = (m_First + 1) % m_Nrundo;
			m_Nrin--;
			m_Nrnow--;
		}

		while (m_Nrundoarrays > m_Nrundoarraysmax && m_Nrin > 0)
		{
			m_Nrin--;
			m_Nrundoarrays -= m_Undos[(m_First + m_Nrin) % m_Nrundo]->Arraynr();
			delete m_Undos[(m_First + m_Nrin) % m_Nrundo];
		}
	}
}

void UndoQueue::SetNrundo(unsigned nr)
{
	if (nr != m_Nrundo)
	{
		while (m_Nrin > nr && m_Nrnow > 0)
		{
			m_Nrundoarrays -= m_Undos[m_First]->Arraynr();
			delete m_Undos[m_First];
			m_First = (m_First + 1) % m_Nrundo;
			m_Nrnow--;
			m_Nrin--;
		}

		while (m_Nrin > nr)
		{
			m_Nrin--;
			m_Nrundoarrays -= m_Undos[(m_First + m_Nrin) % m_Nrundo]->Arraynr();
			delete m_Undos[(m_First + m_Nrin) % m_Nrundo];
		}

		std::vector<UndoElem*> vue;
		vue.clear();
		vue.insert(vue.begin(), m_Undos.begin(), m_Undos.end());

		m_Undos.resize(nr);

		for (unsigned i = 0; i < m_Nrin; i++)
		{
			m_Undos[i] = vue[(m_First + i) % m_Nrundo];
		}

		this->m_Nrundo = nr;

		m_First = 0;
	}
}

void UndoQueue::ReverseUndosliceorder(unsigned short nrslices)
{
	for (unsigned i = 0; i < m_Nrin; i++)
		m_Undos[i]->m_DataSelection.sliceNr = nrslices - 1 - m_Undos[i]->m_DataSelection.sliceNr;
}

} // namespace iseg
