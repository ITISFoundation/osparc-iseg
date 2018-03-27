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

#include "Core/Types.h"

class SlicesHandler;
class bmphandler;

class SliceTransform
{
public:
	SliceTransform(SlicesHandler *hand3D);
	~SliceTransform();

	void SelectTransformData(bool source, bool target, bool tissues);

	void ActiveSliceChanged();
	void NewDataLoaded();

	void Initialize();
	void Execute(bool allSlices);
	void Cancel();

	void SwapDataPointers(); // Helper method for starting undo step

	void Translate(int offsetX, int offsetY, bool apply = true);
	void Rotate(double angle, int centerX, int centerY, bool apply = true);
	void Scale(double factorX, double factorY, int centerX, int centerY,
						 bool apply = true);
	void Shear(double angle, bool shearXAxis, int axisPosition,
						 bool apply = true);
	void Flip(bool flipXAxis, int axisPosition, bool apply = true);
	void Matrix(double *inverseMatrix, bool apply = true);

	bool GetIsIdentityTransform();

private:
	void ResetTransformMatrix();
	void GetTransformMatrix(double *copyToMatrix);

	void ReallocateSliceData();
	void CopyFromOriginalSlice(bool source, bool target, bool tissues);
	void CopyToOriginalSlice(bool source, bool target, bool tissues);

	void ApplyTransform(bool source, bool target, bool tissues);
	void ApplyTransformAll();

private:
	// Image data
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeSlice;

	// Original slice data
	float *originalSource;
	float *originalTarget;
	tissues_size_t *originalTissues;

	// Transformation
	double transformMatrix[3 * 3];
	bool transformSource;
	bool transformTarget;
	bool transformTissues;
};
