/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef COMMON
#define COMMON

#include "Types.h"
#include "iSegCore.h"

/*
 * common.h
 * Common definitions for iSeg project.
 */

namespace common {

struct iSegCore_API DataSelection
{
	DataSelection()
	{
		bmp = work = tissues = vvm = limits = marks = tissueHierarchy = false;
		allSlices = false;
		sliceNr = 0;
	}

	void CombineSelection(DataSelection &other)
	{
		bmp = bmp || other.bmp;
		work = work || other.work;
		tissues = tissues || other.tissues;
		vvm = vvm || other.vvm;
		limits = limits || other.limits;
		marks = marks || other.marks;
		tissueHierarchy = tissueHierarchy || other.tissueHierarchy;
	}

	bool DataSelected()
	{
		return bmp || work || tissues || vvm || limits || marks || tissueHierarchy;
	}

	bool bmp;
	bool work;
	bool tissues;
	bool vvm;
	bool limits;
	bool marks;
	bool tissueHierarchy;

	bool allSlices;
	unsigned short sliceNr;
};

enum EndUndoAction {
	NoUndo,
	EndUndo,
	MergeUndo,
	AbortUndo,
	ClearUndo,
};

iSegCore_API int CombineTissuesVersion(const int version,
																			 const int tissuesVersion);
iSegCore_API void ExtractTissuesVersion(const int combinedVersion, int &version,
																				int &tissuesVersion);

// Quaternion <--> Direction Cosines Conversion
// quaternion[4] = {w, x, y, z}, directionCosines[6] = {a, b, c, x, y, z}
void QuaternionToDirectionCosines(const float *quaternion,
																	float *directionCosines);
void DirectionCosinesToQuaternion(const float *directionCosines,
																	float *quaternion);
void QuaternionFromMatrix(const float mat[3][3], float *quaternion);

iSegCore_API bool Normalize(float *vec);
iSegCore_API bool Cross(float *vecA, float *vecB, float *vecOut);
iSegCore_API bool Orthonormalize(float *vecA, float *vecB);
} // namespace common

#endif