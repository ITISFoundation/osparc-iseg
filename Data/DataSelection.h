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

/// Common definitions for iSeg project.
namespace iseg {

enum EndUndoAction {
	NoUndo,
	EndUndo,
	MergeUndo,
	AbortUndo,
	ClearUndo,
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

	inline bool DataSelected()
	{
		return bmp || work || tissues || vvm || limits || marks || tissueHierarchy;
	}

	bool bmp = false;
	bool work = false;
	bool tissues = false;
	bool vvm = false;
	bool limits = false;
	bool marks = false;
	bool tissueHierarchy = false;

	bool allSlices = false;
	unsigned short sliceNr = 0;
};

} // namespace iseg
