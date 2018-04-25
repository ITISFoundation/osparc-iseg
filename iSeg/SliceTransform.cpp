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

#include "SliceTransform.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include <cmath>

#ifndef M_PI
#	define M_PI 3.1415926535
#endif

#ifndef round
#	define round(x) (x < 0 ? ceil((x)-0.5) : floor((x) + 0.5))
#endif

using namespace iseg;

SliceTransform::SliceTransform(SlicesHandler* hand3D)
	: originalSource(NULL), originalTarget(NULL), originalTissues(NULL)
{
	handler3D = hand3D;
	activeSlice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	transformSource = transformTarget = transformTissues = true;

	// Reallocate slice data
	ReallocateSliceData();

	// Initialize transform
	Initialize();
}

SliceTransform::~SliceTransform()
{
	if (originalSource != NULL)
	{
		free(originalSource);
	}
	if (originalTarget != NULL)
	{
		free(originalTarget);
	}
	if (originalTissues != NULL)
	{
		free(originalTissues);
	}
}

void SliceTransform::ReallocateSliceData()
{
	if (originalSource != NULL)
	{
		free(originalSource);
	}
	if (originalTarget != NULL)
	{
		free(originalTarget);
	}
	if (originalTissues != NULL)
	{
		free(originalTissues);
	}

	unsigned int area = bmphand->return_area();
	originalSource = (float*)malloc(sizeof(float) * area);
	originalTarget = (float*)malloc(sizeof(float) * area);
	originalTissues = (tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
}

void SliceTransform::SelectTransformData(bool source, bool target, bool tissues)
{
	if (transformSource != source)
	{
		if (source)
		{
			// Source has been enabled --> apply transform
			CopyFromOriginalSlice(true, false, false);
			ApplyTransform(true, false, false);
		}
		else
		{
			// Source has been disabled --> undo transform
			CopyToOriginalSlice(true, false, false);
		}
		transformSource = source;
	}

	if (transformTarget != target)
	{
		if (target)
		{
			// Target has been enabled --> apply transform
			CopyFromOriginalSlice(false, true, false);
			ApplyTransform(false, true, false);
		}
		else
		{
			// Target has been disabled --> undo transform
			CopyToOriginalSlice(false, true, false);
		}
		transformTarget = target;
	}

	if (transformTissues != tissues)
	{
		if (tissues)
		{
			// Tissues has been enabled --> apply transform
			CopyFromOriginalSlice(false, false, true);
			ApplyTransform(false, false, true);
		}
		else
		{
			// Tissues has been disabled --> undo transform
			CopyToOriginalSlice(false, false, true);
		}
		transformTissues = tissues;
	}
}

void SliceTransform::ActiveSliceChanged()
{
	// Undo transform for previously active slice
	CopyToOriginalSlice(transformSource, transformTarget, transformTissues);

	// Get new active slice
	activeSlice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
	CopyFromOriginalSlice(transformSource, transformTarget, transformTissues);

	// Apply transform
	ApplyTransform(transformSource, transformTarget, transformTissues);
}

void SliceTransform::NewDataLoaded()
{
	// Get new active slice
	activeSlice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	// Reallocate slice data
	ReallocateSliceData();

	// Initialize transform
	Initialize();
}

void SliceTransform::Execute(bool allSlices)
{
	if (allSlices)
	{
		for (unsigned short slice = handler3D->start_slice();
			 slice < handler3D->end_slice(); ++slice)
		{
			// Active slice has already been transformed
			if (slice == activeSlice)
			{
				continue;
			}

			// Activate and get current slice
			handler3D->set_activeslice(slice);
			bmphand = handler3D->get_activebmphandler();

			// Get copy of original slice data
			CopyFromOriginalSlice(transformSource, transformTarget,
								  transformTissues);

			// Apply transform
			ApplyTransform(transformSource, transformTarget, transformTissues);
		}

		// Restore previously active slice
		handler3D->set_activeslice(activeSlice);
		bmphand = handler3D->get_activebmphandler();

		// Reset transformation
		Initialize();
	}
	else
	{
		// Reset transformation
		Initialize();
	}
}

void SliceTransform::Cancel()
{
	// Copy original image data back to slice
	CopyToOriginalSlice(transformSource, transformTarget, transformTissues);

	// Reset transformation matrix to identity
	ResetTransformMatrix();
}

void SliceTransform::Translate(int offsetX, int offsetY, bool apply)
{
	// Right multiply backtransform matrix with inverse translation matrix
	transformMatrix[2] -=
		transformMatrix[0] * offsetX + transformMatrix[1] * offsetY;
	transformMatrix[5] -=
		transformMatrix[3] * offsetX + transformMatrix[4] * offsetY;
	transformMatrix[8] -=
		transformMatrix[6] * offsetX + transformMatrix[7] * offsetY;

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(transformSource, transformTarget, transformTissues);
	}
}

void SliceTransform::Rotate(double angle, int centerX, int centerY, bool apply)
{
	// Translate center to origin
	Translate(-centerX, -centerY, false);

	// Right multiply backtransform matrix with inverse rotation matrix
	double cosPhi = std::cos(-(M_PI * angle) / 180.0);
	double sinPhi = std::sin(-(M_PI * angle) / 180.0);

	double tmp1 = transformMatrix[0];
	double tmp2 = transformMatrix[1];
	transformMatrix[0] = tmp1 * cosPhi + tmp2 * sinPhi;
	transformMatrix[1] = tmp2 * cosPhi - tmp1 * sinPhi;

	tmp1 = transformMatrix[3];
	tmp2 = transformMatrix[4];
	transformMatrix[3] = tmp1 * cosPhi + tmp2 * sinPhi;
	transformMatrix[4] = tmp2 * cosPhi - tmp1 * sinPhi;

	tmp1 = transformMatrix[6];
	tmp2 = transformMatrix[7];
	transformMatrix[6] = tmp1 * cosPhi + tmp2 * sinPhi;
	transformMatrix[7] = tmp2 * cosPhi - tmp1 * sinPhi;

	// Translate center back
	Translate(centerX, centerY, false);

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(transformSource, transformTarget, transformTissues);
	}
}

void SliceTransform::Scale(double factorX, double factorY, int centerX,
						   int centerY, bool apply)
{
	// Translate center to origin
	Translate(-centerX, -centerY, false);

	// Right multiply backtransform matrix with inverse scale matrix
	transformMatrix[0] /= factorX;
	transformMatrix[1] /= factorY;
	transformMatrix[3] /= factorX;
	transformMatrix[4] /= factorY;
	transformMatrix[6] /= factorX;
	transformMatrix[7] /= factorY;

	// Translate center back
	Translate(centerX, centerY, false);

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(transformSource, transformTarget, transformTissues);
	}
}

void SliceTransform::Shear(double angle, bool shearXAxis, int axisPosition,
						   bool apply)
{
	if (shearXAxis)
	{
		// Translate x-axis to origin
		Translate(-axisPosition, 0, false);

		// Right multiply backtransform matrix with inverse shear matrix
		double k = std::tan(-(M_PI * angle) / 180.0);
		transformMatrix[0] += transformMatrix[1] * k;
		transformMatrix[3] += transformMatrix[4] * k;
		transformMatrix[6] += transformMatrix[7] * k;

		// Translate x-axis back
		Translate(axisPosition, 0, false);
	}
	else
	{
		// Translate y-axis to origin
		Translate(0, -axisPosition, false);

		// Right multiply backtransform matrix with inverse shear matrix
		double k = std::tan(-(M_PI * angle) / 180.0);
		transformMatrix[1] += transformMatrix[0] * k;
		transformMatrix[4] += transformMatrix[3] * k;
		transformMatrix[7] += transformMatrix[6] * k;

		// Translate y-axis back
		Translate(0, axisPosition, false);
	}

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(transformSource, transformTarget, transformTissues);
	}
}

void SliceTransform::Flip(bool flipXAxis, int axisPosition, bool apply)
{
	if (flipXAxis)
	{
		// Translate x-axis to origin
		Translate(-axisPosition, 0, false);

		// Right multiply backtransform matrix with flip matrix
		transformMatrix[0] = -transformMatrix[0];
		transformMatrix[3] = -transformMatrix[3];
		transformMatrix[6] = -transformMatrix[6];

		// Translate x-axis back
		Translate(axisPosition, 0, false);
	}
	else
	{
		// Translate y-axis to origin
		Translate(0, -axisPosition, false);

		// Right multiply backtransform matrix with flip matrix
		transformMatrix[1] = -transformMatrix[1];
		transformMatrix[4] = -transformMatrix[4];
		transformMatrix[7] = -transformMatrix[7];

		// Translate y-axis back
		Translate(0, axisPosition, false);
	}

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(transformSource, transformTarget, transformTissues);
	}
}

void SliceTransform::Matrix(double* inverseMatrix, bool apply)
{
	// Right multiply backtransform matrix with input matrix
	double tmp1 = transformMatrix[0];
	double tmp2 = transformMatrix[1];
	double tmp3 = transformMatrix[2];
	transformMatrix[0] = tmp1 * inverseMatrix[0] + tmp2 * inverseMatrix[3] +
						 tmp3 * inverseMatrix[6];
	transformMatrix[1] = tmp1 * inverseMatrix[1] + tmp2 * inverseMatrix[4] +
						 tmp3 * inverseMatrix[7];
	transformMatrix[2] = tmp1 * inverseMatrix[2] + tmp2 * inverseMatrix[5] +
						 tmp3 * inverseMatrix[8];

	tmp1 = transformMatrix[3];
	tmp2 = transformMatrix[4];
	tmp3 = transformMatrix[5];
	transformMatrix[3] = tmp1 * inverseMatrix[0] + tmp2 * inverseMatrix[3] +
						 tmp3 * inverseMatrix[6];
	transformMatrix[4] = tmp1 * inverseMatrix[1] + tmp2 * inverseMatrix[4] +
						 tmp3 * inverseMatrix[7];
	transformMatrix[5] = tmp1 * inverseMatrix[2] + tmp2 * inverseMatrix[5] +
						 tmp3 * inverseMatrix[8];

	tmp1 = transformMatrix[6];
	tmp2 = transformMatrix[7];
	tmp3 = transformMatrix[8];
	transformMatrix[6] = tmp1 * inverseMatrix[0] + tmp2 * inverseMatrix[3] +
						 tmp3 * inverseMatrix[6];
	transformMatrix[7] = tmp1 * inverseMatrix[1] + tmp2 * inverseMatrix[4] +
						 tmp3 * inverseMatrix[7];
	transformMatrix[8] = tmp1 * inverseMatrix[2] + tmp2 * inverseMatrix[5] +
						 tmp3 * inverseMatrix[8];

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(transformSource, transformTarget, transformTissues);
	}
}

bool SliceTransform::GetIsIdentityTransform()
{
	// Returns whether the transform is approximately an identity transform
	double sqrSum = 0.0;
	sqrSum += transformMatrix[1] * transformMatrix[1];
	sqrSum += transformMatrix[2] * transformMatrix[2];
	sqrSum += transformMatrix[3] * transformMatrix[3];
	sqrSum += transformMatrix[5] * transformMatrix[5];
	sqrSum += transformMatrix[6] * transformMatrix[6];
	sqrSum += transformMatrix[7] * transformMatrix[7];
	sqrSum += (transformMatrix[0] / transformMatrix[8] - 1.0) *
			  (transformMatrix[0] / transformMatrix[8] - 1.0);
	sqrSum += (transformMatrix[4] / transformMatrix[8] - 1.0) *
			  (transformMatrix[4] / transformMatrix[8] - 1.0);
	return (sqrSum < 1.0E-06);
}

void SliceTransform::Initialize()
{
	// Reset the transformation matrix to identity
	ResetTransformMatrix();

	// Copy original slice to internal data structures
	CopyFromOriginalSlice(transformSource, transformTarget, transformTissues);
}

void SliceTransform::ResetTransformMatrix()
{
	// Initialize transform matrix (homogeneous coordinates)
	for (unsigned int i = 0; i < 9; ++i)
	{
		transformMatrix[i] = 0.0;
	}
	transformMatrix[0] = transformMatrix[4] = transformMatrix[8] = 1.0;
}

void SliceTransform::GetTransformMatrix(double* copyToMatrix)
{
	for (unsigned short i = 0; i < 9; ++i)
	{
		copyToMatrix[i] = transformMatrix[i];
	}
}

void SliceTransform::CopyFromOriginalSlice(bool source, bool target,
										   bool tissues)
{
	// Copy original slice data
	if (source)
	{
		bmphand->copy_bmp(originalSource);
	}
	if (target)
	{
		bmphand->copy_work(originalTarget);
	}
	if (tissues)
	{
		bmphand->copy_tissue(handler3D->active_tissuelayer(),
							 originalTissues);
	}
}

void SliceTransform::CopyToOriginalSlice(bool source, bool target, bool tissues)
{
	// Copy original image data back to slice
	if (source)
	{
		bmphand->copy2bmp(originalSource, bmphand->return_mode(true));
	}
	if (target)
	{
		bmphand->copy2work(originalTarget, bmphand->return_mode(false));
	}
	if (tissues)
	{
		bmphand->copy2tissue(handler3D->active_tissuelayer(),
							 originalTissues);
	}
}

void SliceTransform::SwapDataPointers()
{
	if (transformSource)
	{
		originalSource = bmphand->swap_bmp_pointer(originalSource);
	}
	if (transformTarget)
	{
		originalTarget = bmphand->swap_work_pointer(originalTarget);
	}
	if (transformTissues)
	{
		originalTissues = bmphand->swap_tissues_pointer(
			handler3D->active_tissuelayer(), originalTissues);
	}
}

void SliceTransform::ApplyTransform(bool source, bool target, bool tissues)
{
	if (source && target && tissues)
	{
		// Faster version without if-statements in the loop
		ApplyTransformAll();
		return;
	}

	// transformMatrix * original --> preview
	double a, b, c;
	int xTr, yTr;
	unsigned short width = bmphand->return_width();
	unsigned short height = bmphand->return_height();
	float* sourcePtr = bmphand->return_bmp();
	float* targetPtr = bmphand->return_work();
	tissues_size_t* tissuesPtr =
		bmphand->return_tissues(handler3D->active_tissuelayer());
	for (unsigned int y = 0; y < height; ++y)
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			// Transform in homogeneous coordinates
			a = transformMatrix[0] * x + transformMatrix[1] * y +
				transformMatrix[2];
			b = transformMatrix[3] * x + transformMatrix[4] * y +
				transformMatrix[5];
			c = transformMatrix[6] * x + transformMatrix[7] * y +
				transformMatrix[8];

			xTr = (int)round(a / c);
			yTr = (int)round(b / c);
			if (xTr >= 0 && yTr >= 0 && xTr < width && yTr < height)
			{
				if (source)
				{
					sourcePtr[y * width + x] =
						originalSource[yTr * width + xTr];
				}
				if (target)
				{
					targetPtr[y * width + x] =
						originalTarget[yTr * width + xTr];
				}
				if (tissues)
				{
					tissuesPtr[y * width + x] =
						originalTissues[yTr * width + xTr];
				}
			}
			else
			{
				if (source)
				{
					sourcePtr[y * width + x] = 0.0f;
				}
				if (target)
				{
					targetPtr[y * width + x] = 0.0f;
				}
				if (tissues)
				{
					tissuesPtr[y * width + x] = 0;
				}
			}
		}
	}
}

void SliceTransform::ApplyTransformAll()
{
	// transformMatrix * original --> preview
	double a, b, c;
	int xTr, yTr;
	unsigned short width = bmphand->return_width();
	unsigned short height = bmphand->return_height();
	float* sourcePtr = bmphand->return_bmp();
	float* targetPtr = bmphand->return_work();
	tissues_size_t* tissuesPtr =
		bmphand->return_tissues(handler3D->active_tissuelayer());
	for (unsigned int y = 0; y < height; ++y)
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			// Transform in homogeneous coordinates
			a = transformMatrix[0] * x + transformMatrix[1] * y +
				transformMatrix[2];
			b = transformMatrix[3] * x + transformMatrix[4] * y +
				transformMatrix[5];
			c = transformMatrix[6] * x + transformMatrix[7] * y +
				transformMatrix[8];

			xTr = (int)round(a / c);
			yTr = (int)round(b / c);
			if (xTr >= 0 && yTr >= 0 && xTr < width && yTr < height)
			{
				sourcePtr[y * width + x] = originalSource[yTr * width + xTr];
				targetPtr[y * width + x] = originalTarget[yTr * width + xTr];
				tissuesPtr[y * width + x] = originalTissues[yTr * width + xTr];
			}
			else
			{
				sourcePtr[y * width + x] = 0.0f;
				targetPtr[y * width + x] = 0.0f;
				tissuesPtr[y * width + x] = 0;
			}
		}
	}
}
