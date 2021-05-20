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

namespace iseg {

SliceTransform::SliceTransform(SlicesHandler* hand3D)
		: m_OriginalSource(nullptr), m_OriginalTarget(nullptr), m_OriginalTissues(nullptr)
{
	m_Handler3D = hand3D;
	m_ActiveSlice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	m_TransformSource = m_TransformTarget = m_TransformTissues = true;

	// Reallocate slice data
	ReallocateSliceData();

	// Initialize transform
	Initialize();
}

SliceTransform::~SliceTransform()
{
	if (m_OriginalSource != nullptr)
	{
		free(m_OriginalSource);
	}
	if (m_OriginalTarget != nullptr)
	{
		free(m_OriginalTarget);
	}
	if (m_OriginalTissues != nullptr)
	{
		free(m_OriginalTissues);
	}
}

void SliceTransform::ReallocateSliceData()
{
	if (m_OriginalSource != nullptr)
	{
		free(m_OriginalSource);
	}
	if (m_OriginalTarget != nullptr)
	{
		free(m_OriginalTarget);
	}
	if (m_OriginalTissues != nullptr)
	{
		free(m_OriginalTissues);
	}

	unsigned int area = m_Bmphand->ReturnArea();
	m_OriginalSource = (float*)malloc(sizeof(float) * area);
	m_OriginalTarget = (float*)malloc(sizeof(float) * area);
	m_OriginalTissues = (tissues_size_t*)malloc(sizeof(tissues_size_t) * area);
}

void SliceTransform::SelectTransformData(bool source, bool target, bool tissues)
{
	if (m_TransformSource != source)
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
		m_TransformSource = source;
	}

	if (m_TransformTarget != target)
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
		m_TransformTarget = target;
	}

	if (m_TransformTissues != tissues)
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
		m_TransformTissues = tissues;
	}
}

void SliceTransform::ActiveSliceChanged()
{
	// Undo transform for previously active slice
	CopyToOriginalSlice(m_TransformSource, m_TransformTarget, m_TransformTissues);

	// Get new active slice
	m_ActiveSlice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();
	CopyFromOriginalSlice(m_TransformSource, m_TransformTarget, m_TransformTissues);

	// Apply transform
	ApplyTransform(m_TransformSource, m_TransformTarget, m_TransformTissues);
}

void SliceTransform::NewDataLoaded()
{
	// Get new active slice
	m_ActiveSlice = m_Handler3D->ActiveSlice();
	m_Bmphand = m_Handler3D->GetActivebmphandler();

	// Reallocate slice data
	ReallocateSliceData();

	// Initialize transform
	Initialize();
}

void SliceTransform::Execute(bool allSlices)
{
	if (allSlices)
	{
		for (unsigned short slice = m_Handler3D->StartSlice();
				 slice < m_Handler3D->EndSlice(); ++slice)
		{
			// Active slice has already been transformed
			if (slice == m_ActiveSlice)
			{
				continue;
			}

			// Activate and get current slice
			m_Handler3D->SetActiveSlice(slice);
			m_Bmphand = m_Handler3D->GetActivebmphandler();

			// Get copy of original slice data
			CopyFromOriginalSlice(m_TransformSource, m_TransformTarget, m_TransformTissues);

			// Apply transform
			ApplyTransform(m_TransformSource, m_TransformTarget, m_TransformTissues);
		}

		// Restore previously active slice
		m_Handler3D->SetActiveSlice(m_ActiveSlice);
		m_Bmphand = m_Handler3D->GetActivebmphandler();

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
	CopyToOriginalSlice(m_TransformSource, m_TransformTarget, m_TransformTissues);

	// Reset transformation matrix to identity
	ResetTransformMatrix();
}

void SliceTransform::Translate(int offsetX, int offsetY, bool apply)
{
	// Right multiply backtransform matrix with inverse translation matrix
	m_TransformMatrix[2] -=
			m_TransformMatrix[0] * offsetX + m_TransformMatrix[1] * offsetY;
	m_TransformMatrix[5] -=
			m_TransformMatrix[3] * offsetX + m_TransformMatrix[4] * offsetY;
	m_TransformMatrix[8] -=
			m_TransformMatrix[6] * offsetX + m_TransformMatrix[7] * offsetY;

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(m_TransformSource, m_TransformTarget, m_TransformTissues);
	}
}

void SliceTransform::Rotate(double angle, int centerX, int centerY, bool apply)
{
	// Translate center to origin
	Translate(-centerX, -centerY, false);

	// Right multiply backtransform matrix with inverse rotation matrix
	double cos_phi = std::cos(-(M_PI * angle) / 180.0);
	double sin_phi = std::sin(-(M_PI * angle) / 180.0);

	double tmp1 = m_TransformMatrix[0];
	double tmp2 = m_TransformMatrix[1];
	m_TransformMatrix[0] = tmp1 * cos_phi + tmp2 * sin_phi;
	m_TransformMatrix[1] = tmp2 * cos_phi - tmp1 * sin_phi;

	tmp1 = m_TransformMatrix[3];
	tmp2 = m_TransformMatrix[4];
	m_TransformMatrix[3] = tmp1 * cos_phi + tmp2 * sin_phi;
	m_TransformMatrix[4] = tmp2 * cos_phi - tmp1 * sin_phi;

	tmp1 = m_TransformMatrix[6];
	tmp2 = m_TransformMatrix[7];
	m_TransformMatrix[6] = tmp1 * cos_phi + tmp2 * sin_phi;
	m_TransformMatrix[7] = tmp2 * cos_phi - tmp1 * sin_phi;

	// Translate center back
	Translate(centerX, centerY, false);

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(m_TransformSource, m_TransformTarget, m_TransformTissues);
	}
}

void SliceTransform::Scale(double factorX, double factorY, int centerX, int centerY, bool apply)
{
	// Translate center to origin
	Translate(-centerX, -centerY, false);

	// Right multiply backtransform matrix with inverse scale matrix
	m_TransformMatrix[0] /= factorX;
	m_TransformMatrix[1] /= factorY;
	m_TransformMatrix[3] /= factorX;
	m_TransformMatrix[4] /= factorY;
	m_TransformMatrix[6] /= factorX;
	m_TransformMatrix[7] /= factorY;

	// Translate center back
	Translate(centerX, centerY, false);

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(m_TransformSource, m_TransformTarget, m_TransformTissues);
	}
}

void SliceTransform::Shear(double angle, bool shearXAxis, int axisPosition, bool apply)
{
	if (shearXAxis)
	{
		// Translate x-axis to origin
		Translate(-axisPosition, 0, false);

		// Right multiply backtransform matrix with inverse shear matrix
		double k = std::tan(-(M_PI * angle) / 180.0);
		m_TransformMatrix[0] += m_TransformMatrix[1] * k;
		m_TransformMatrix[3] += m_TransformMatrix[4] * k;
		m_TransformMatrix[6] += m_TransformMatrix[7] * k;

		// Translate x-axis back
		Translate(axisPosition, 0, false);
	}
	else
	{
		// Translate y-axis to origin
		Translate(0, -axisPosition, false);

		// Right multiply backtransform matrix with inverse shear matrix
		double k = std::tan(-(M_PI * angle) / 180.0);
		m_TransformMatrix[1] += m_TransformMatrix[0] * k;
		m_TransformMatrix[4] += m_TransformMatrix[3] * k;
		m_TransformMatrix[7] += m_TransformMatrix[6] * k;

		// Translate y-axis back
		Translate(0, axisPosition, false);
	}

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(m_TransformSource, m_TransformTarget, m_TransformTissues);
	}
}

void SliceTransform::Flip(bool flipXAxis, int axisPosition, bool apply)
{
	if (flipXAxis)
	{
		// Translate x-axis to origin
		Translate(-axisPosition, 0, false);

		// Right multiply backtransform matrix with flip matrix
		m_TransformMatrix[0] = -m_TransformMatrix[0];
		m_TransformMatrix[3] = -m_TransformMatrix[3];
		m_TransformMatrix[6] = -m_TransformMatrix[6];

		// Translate x-axis back
		Translate(axisPosition, 0, false);
	}
	else
	{
		// Translate y-axis to origin
		Translate(0, -axisPosition, false);

		// Right multiply backtransform matrix with flip matrix
		m_TransformMatrix[1] = -m_TransformMatrix[1];
		m_TransformMatrix[4] = -m_TransformMatrix[4];
		m_TransformMatrix[7] = -m_TransformMatrix[7];

		// Translate y-axis back
		Translate(0, axisPosition, false);
	}

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(m_TransformSource, m_TransformTarget, m_TransformTissues);
	}
}

void SliceTransform::Matrix(double* inverseMatrix, bool apply)
{
	// Right multiply backtransform matrix with input matrix
	double tmp1 = m_TransformMatrix[0];
	double tmp2 = m_TransformMatrix[1];
	double tmp3 = m_TransformMatrix[2];
	m_TransformMatrix[0] = tmp1 * inverseMatrix[0] + tmp2 * inverseMatrix[3] +
												 tmp3 * inverseMatrix[6];
	m_TransformMatrix[1] = tmp1 * inverseMatrix[1] + tmp2 * inverseMatrix[4] +
												 tmp3 * inverseMatrix[7];
	m_TransformMatrix[2] = tmp1 * inverseMatrix[2] + tmp2 * inverseMatrix[5] +
												 tmp3 * inverseMatrix[8];

	tmp1 = m_TransformMatrix[3];
	tmp2 = m_TransformMatrix[4];
	tmp3 = m_TransformMatrix[5];
	m_TransformMatrix[3] = tmp1 * inverseMatrix[0] + tmp2 * inverseMatrix[3] +
												 tmp3 * inverseMatrix[6];
	m_TransformMatrix[4] = tmp1 * inverseMatrix[1] + tmp2 * inverseMatrix[4] +
												 tmp3 * inverseMatrix[7];
	m_TransformMatrix[5] = tmp1 * inverseMatrix[2] + tmp2 * inverseMatrix[5] +
												 tmp3 * inverseMatrix[8];

	tmp1 = m_TransformMatrix[6];
	tmp2 = m_TransformMatrix[7];
	tmp3 = m_TransformMatrix[8];
	m_TransformMatrix[6] = tmp1 * inverseMatrix[0] + tmp2 * inverseMatrix[3] +
												 tmp3 * inverseMatrix[6];
	m_TransformMatrix[7] = tmp1 * inverseMatrix[1] + tmp2 * inverseMatrix[4] +
												 tmp3 * inverseMatrix[7];
	m_TransformMatrix[8] = tmp1 * inverseMatrix[2] + tmp2 * inverseMatrix[5] +
												 tmp3 * inverseMatrix[8];

	// Apply transform to data
	if (apply)
	{
		ApplyTransform(m_TransformSource, m_TransformTarget, m_TransformTissues);
	}
}

bool SliceTransform::GetIsIdentityTransform()
{
	// Returns whether the transform is approximately an identity transform
	double sqr_sum = 0.0;
	sqr_sum += m_TransformMatrix[1] * m_TransformMatrix[1];
	sqr_sum += m_TransformMatrix[2] * m_TransformMatrix[2];
	sqr_sum += m_TransformMatrix[3] * m_TransformMatrix[3];
	sqr_sum += m_TransformMatrix[5] * m_TransformMatrix[5];
	sqr_sum += m_TransformMatrix[6] * m_TransformMatrix[6];
	sqr_sum += m_TransformMatrix[7] * m_TransformMatrix[7];
	sqr_sum += (m_TransformMatrix[0] / m_TransformMatrix[8] - 1.0) *
						 (m_TransformMatrix[0] / m_TransformMatrix[8] - 1.0);
	sqr_sum += (m_TransformMatrix[4] / m_TransformMatrix[8] - 1.0) *
						 (m_TransformMatrix[4] / m_TransformMatrix[8] - 1.0);
	return (sqr_sum < 1.0E-06);
}

void SliceTransform::Initialize()
{
	// Reset the transformation matrix to identity
	ResetTransformMatrix();

	// Copy original slice to internal data structures
	CopyFromOriginalSlice(m_TransformSource, m_TransformTarget, m_TransformTissues);
}

void SliceTransform::ResetTransformMatrix()
{
	// Initialize transform matrix (homogeneous coordinates)
	for (unsigned int i = 0; i < 9; ++i)
	{
		m_TransformMatrix[i] = 0.0;
	}
	m_TransformMatrix[0] = m_TransformMatrix[4] = m_TransformMatrix[8] = 1.0;
}

void SliceTransform::GetTransformMatrix(double* copyToMatrix)
{
	for (unsigned short i = 0; i < 9; ++i)
	{
		copyToMatrix[i] = m_TransformMatrix[i];
	}
}

void SliceTransform::CopyFromOriginalSlice(bool source, bool target, bool tissues)
{
	// Copy original slice data
	if (source)
	{
		m_Bmphand->CopyBmp(m_OriginalSource);
	}
	if (target)
	{
		m_Bmphand->CopyWork(m_OriginalTarget);
	}
	if (tissues)
	{
		m_Bmphand->CopyTissue(m_Handler3D->ActiveTissuelayer(), m_OriginalTissues);
	}
}

void SliceTransform::CopyToOriginalSlice(bool source, bool target, bool tissues)
{
	// Copy original image data back to slice
	if (source)
	{
		m_Bmphand->Copy2bmp(m_OriginalSource, m_Bmphand->ReturnMode(true));
	}
	if (target)
	{
		m_Bmphand->Copy2work(m_OriginalTarget, m_Bmphand->ReturnMode(false));
	}
	if (tissues)
	{
		m_Bmphand->Copy2tissue(m_Handler3D->ActiveTissuelayer(), m_OriginalTissues);
	}
}

void SliceTransform::SwapDataPointers()
{
	if (m_TransformSource)
	{
		m_OriginalSource = m_Bmphand->SwapBmpPointer(m_OriginalSource);
	}
	if (m_TransformTarget)
	{
		m_OriginalTarget = m_Bmphand->SwapWorkPointer(m_OriginalTarget);
	}
	if (m_TransformTissues)
	{
		m_OriginalTissues = m_Bmphand->SwapTissuesPointer(m_Handler3D->ActiveTissuelayer(), m_OriginalTissues);
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
	int x_tr, y_tr;
	unsigned short width = m_Bmphand->ReturnWidth();
	unsigned short height = m_Bmphand->ReturnHeight();
	float* source_ptr = m_Bmphand->ReturnBmp();
	float* target_ptr = m_Bmphand->ReturnWork();
	tissues_size_t* tissues_ptr =
			m_Bmphand->ReturnTissues(m_Handler3D->ActiveTissuelayer());
	for (unsigned int y = 0; y < height; ++y)
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			// Transform in homogeneous coordinates
			a = m_TransformMatrix[0] * x + m_TransformMatrix[1] * y +
					m_TransformMatrix[2];
			b = m_TransformMatrix[3] * x + m_TransformMatrix[4] * y +
					m_TransformMatrix[5];
			c = m_TransformMatrix[6] * x + m_TransformMatrix[7] * y +
					m_TransformMatrix[8];

			x_tr = (int)round(a / c);
			y_tr = (int)round(b / c);
			if (x_tr >= 0 && y_tr >= 0 && x_tr < width && y_tr < height)
			{
				if (source)
				{
					source_ptr[y * width + x] =
							m_OriginalSource[y_tr * width + x_tr];
				}
				if (target)
				{
					target_ptr[y * width + x] =
							m_OriginalTarget[y_tr * width + x_tr];
				}
				if (tissues)
				{
					tissues_ptr[y * width + x] =
							m_OriginalTissues[y_tr * width + x_tr];
				}
			}
			else
			{
				if (source)
				{
					source_ptr[y * width + x] = 0.0f;
				}
				if (target)
				{
					target_ptr[y * width + x] = 0.0f;
				}
				if (tissues)
				{
					tissues_ptr[y * width + x] = 0;
				}
			}
		}
	}
}

void SliceTransform::ApplyTransformAll()
{
	// transformMatrix * original --> preview
	double a, b, c;
	int x_tr, y_tr;
	unsigned short width = m_Bmphand->ReturnWidth();
	unsigned short height = m_Bmphand->ReturnHeight();
	float* source_ptr = m_Bmphand->ReturnBmp();
	float* target_ptr = m_Bmphand->ReturnWork();
	tissues_size_t* tissues_ptr =
			m_Bmphand->ReturnTissues(m_Handler3D->ActiveTissuelayer());
	for (unsigned int y = 0; y < height; ++y)
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			// Transform in homogeneous coordinates
			a = m_TransformMatrix[0] * x + m_TransformMatrix[1] * y +
					m_TransformMatrix[2];
			b = m_TransformMatrix[3] * x + m_TransformMatrix[4] * y +
					m_TransformMatrix[5];
			c = m_TransformMatrix[6] * x + m_TransformMatrix[7] * y +
					m_TransformMatrix[8];

			x_tr = (int)round(a / c);
			y_tr = (int)round(b / c);
			if (x_tr >= 0 && y_tr >= 0 && x_tr < width && y_tr < height)
			{
				source_ptr[y * width + x] = m_OriginalSource[y_tr * width + x_tr];
				target_ptr[y * width + x] = m_OriginalTarget[y_tr * width + x_tr];
				tissues_ptr[y * width + x] = m_OriginalTissues[y_tr * width + x_tr];
			}
			else
			{
				source_ptr[y * width + x] = 0.0f;
				target_ptr[y * width + x] = 0.0f;
				tissues_ptr[y * width + x] = 0;
			}
		}
	}
}

} // namespace iseg
