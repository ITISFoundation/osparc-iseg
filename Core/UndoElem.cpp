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

#include "UndoElem.h"

#include <cstdlib>
#include <vector>

namespace iseg {

UndoElem::UndoElem()
{
	m_BmpOld = m_WorkOld = m_BmpNew = m_WorkNew = nullptr;
	m_TissueOld = m_TissueNew = nullptr;
	m_Mode1Old = m_Mode1New = m_Mode2Old = m_Mode2New = 0;
}

UndoElem::~UndoElem()
{
	if (m_BmpOld != nullptr)
		free(m_BmpOld);
	if (m_WorkOld != nullptr)
		free(m_WorkOld);
	if (m_TissueOld != nullptr)
		free(m_TissueOld);
	if (m_BmpNew != nullptr)
		free(m_BmpNew);
	if (m_WorkNew != nullptr)
		free(m_WorkNew);
	if (m_TissueNew != nullptr)
		free(m_TissueNew);
}

void UndoElem::Merge(UndoElem* ue)
{
	if (m_DataSelection.sliceNr == ue->m_DataSelection.sliceNr && !Multi())
	{
		if (ue->m_DataSelection.bmp)
		{
			if (m_DataSelection.bmp)
				free(m_BmpNew);
			else
			{
				m_BmpOld = ue->m_BmpOld;
				m_Mode1Old = ue->m_Mode1Old;
			}
			m_Mode1New = ue->m_Mode1New;
			m_BmpNew = ue->m_BmpNew;
		}
		if (ue->m_DataSelection.work)
		{
			if (m_DataSelection.work)
				free(m_WorkNew);
			else
			{
				m_WorkOld = ue->m_WorkOld;
				m_Mode2Old = ue->m_Mode2Old;
			}
			m_Mode2New = ue->m_Mode2New;
			m_WorkNew = ue->m_WorkNew;
		}
		if (ue->m_DataSelection.tissues)
		{
			if (m_DataSelection.tissues)
				free(m_TissueNew);
			else
				m_TissueOld = ue->m_TissueOld;
			m_TissueNew = ue->m_TissueNew;
		}
		if (ue->m_DataSelection.vvm)
		{
			m_VvmNew.clear();
			m_VvmNew = ue->m_VvmNew;
			if (!m_DataSelection.vvm)
			{
				m_VvmOld.clear();
				m_VvmOld = ue->m_VvmOld;
			}
		}
		if (ue->m_DataSelection.limits)
		{
			m_LimitsNew.clear();
			m_LimitsNew = ue->m_LimitsNew;
			if (!m_DataSelection.limits)
			{
				m_LimitsOld.clear();
				m_LimitsOld = ue->m_LimitsOld;
			}
		}
		if (ue->m_DataSelection.marks)
		{
			m_MarksNew.clear();
			m_MarksNew = ue->m_MarksNew;
			if (!m_DataSelection.marks)
			{
				m_MarksOld.clear();
				m_MarksOld = ue->m_MarksOld;
			}
		}
		m_DataSelection.CombineSelection(ue->m_DataSelection);
	}
	/*	if(bmp_new!=nullptr) free(bmp_new);
		if(work_new!=nullptr) free(work_new);
		if(tissue_new!=nullptr) free(tissue_new);
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

unsigned UndoElem::Arraynr()
{
	unsigned i = 0;
	const unsigned added = 1;

	if (m_DataSelection.bmp)
	{
		i += added;
	}
	if (m_DataSelection.work)
	{
		i += added;
	}
	if (m_DataSelection.tissues)
	{
		i += added;
	}

	return i;
}

bool UndoElem::Multi() const
{
	return false;
}

MultiUndoElem::MultiUndoElem() = default;

MultiUndoElem::~MultiUndoElem()
{
	std::vector<float*>::iterator itf;
	std::vector<tissues_size_t*>::iterator it8;

	for (itf = m_VbmpOld.begin(); itf != m_VbmpOld.end(); itf++)
		free(*itf);
	for (itf = m_VworkOld.begin(); itf != m_VworkOld.end(); itf++)
		free(*itf);
	for (it8 = m_VtissueOld.begin(); it8 != m_VtissueOld.end(); it8++)
		free(*it8);
	for (itf = m_VbmpNew.begin(); itf != m_VbmpNew.end(); itf++)
		free(*itf);
	for (itf = m_VworkNew.begin(); itf != m_VworkNew.end(); itf++)
		free(*itf);
	for (it8 = m_VtissueNew.begin(); it8 != m_VtissueNew.end(); it8++)
		free(*it8);
}

void MultiUndoElem::Merge(UndoElem* ue) {}

unsigned MultiUndoElem::Arraynr()
{
	unsigned i = 0;
	const unsigned added = 1;

	if (m_DataSelection.bmp)
	{
		i += added;
	}
	if (m_DataSelection.work)
	{
		i += added;
	}
	if (m_DataSelection.tissues)
	{
		i += added;
	}

	return i * m_Vslicenr.size();
}

bool MultiUndoElem::Multi() const
{
	return true;
}

} // namespace iseg
