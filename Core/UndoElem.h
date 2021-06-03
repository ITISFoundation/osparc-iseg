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

#include "Data/DataSelection.h"
#include "Data/Mark.h"
#include "Data/Point.h"
#include "Data/Types.h"

#include <vector>

namespace iseg {

class ISEG_CORE_API UndoElem
{
public:
	UndoElem();
	virtual ~UndoElem();
	virtual void Merge(UndoElem* ue);
	virtual unsigned Arraynr();
	virtual bool Multi() const;

	DataSelection m_DataSelection;
	float* m_BmpOld;
	float* m_WorkOld;
	tissues_size_t* m_TissueOld;
	std::vector<std::vector<Mark>> m_VvmOld;
	std::vector<std::vector<Point>> m_LimitsOld;
	std::vector<Mark> m_MarksOld;
	float* m_BmpNew;
	float* m_WorkNew;
	tissues_size_t* m_TissueNew;
	std::vector<std::vector<Mark>> m_VvmNew;
	std::vector<std::vector<Point>> m_LimitsNew;
	std::vector<Mark> m_MarksNew;
	unsigned char m_Mode1Old;
	unsigned char m_Mode1New;
	unsigned char m_Mode2Old;
	unsigned char m_Mode2New;
};

class ISEG_CORE_API MultiUndoElem : public UndoElem
{
public:
	MultiUndoElem();
	~MultiUndoElem() override;
	void Merge(UndoElem* ue) override;
	unsigned Arraynr() override;
	bool Multi() const override;

	std::vector<unsigned> m_Vslicenr;
	std::vector<float*> m_VbmpOld;
	std::vector<float*> m_VworkOld;
	std::vector<tissues_size_t*> m_VtissueOld;
	std::vector<std::vector<std::vector<Mark>>> m_VvvmOld;
	std::vector<std::vector<std::vector<Point>>> m_VlimitsOld;
	std::vector<std::vector<Mark>> m_VmarksOld;
	std::vector<float*> m_VbmpNew;
	std::vector<float*> m_VworkNew;
	std::vector<tissues_size_t*> m_VtissueNew;
	std::vector<std::vector<std::vector<Mark>>> m_VvvmNew;
	std::vector<std::vector<std::vector<Point>>> m_VlimitsNew;
	std::vector<std::vector<Mark>> m_VmarksNew;
	std::vector<unsigned char> m_Vmode1Old;
	std::vector<unsigned char> m_Vmode1New;
	std::vector<unsigned char> m_Vmode2Old;
	std::vector<unsigned char> m_Vmode2New;
};

} // namespace iseg
