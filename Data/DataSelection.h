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

/// Common definitions for iSeg project.
namespace iseg {

enum eEndUndoAction {
	NoUndo,
	EndUndo,
	MergeUndo,
	AbortUndo,
	ClearUndo
};

struct DataSelection
{
	inline void CombineSelection(DataSelection& other)
	{
		bmp = bmp || other.bmp;
		work = work || other.work;
		tissues = tissues || other.tissues;
		vvm = vvm || other.vvm;
		limits = limits || other.limits;
		marks = marks || other.marks;
		tissueHierarchy = tissueHierarchy || other.tissueHierarchy;
	}

	inline bool DataSelected() const
	{
		return bmp || work || tissues || vvm || limits || marks || tissueHierarchy;
	}

	bool bmp = false;							// NOLINT
	bool work = false;						// NOLINT
	bool tissues = false;					// NOLINT
	bool vvm = false;							// NOLINT
	bool limits = false;					// NOLINT
	bool marks = false;						// NOLINT
	bool tissueHierarchy = false; // NOLINT

	bool allSlices = false;			// NOLINT
	unsigned short sliceNr = 0; // NOLINT
};

} // namespace iseg
