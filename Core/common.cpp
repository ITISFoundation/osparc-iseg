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
#include "common.h"
#include <vtkMath.h>

int common::CombineTissuesVersion(const int version, const int tissuesVersion)
{
	return (tissuesVersion << 8) + version;
}

void common::ExtractTissuesVersion(const int combinedVersion, int &version, int &tissuesVersion)
{
	tissuesVersion = combinedVersion >> 8;
	version = combinedVersion & 0xFF;
}

void common::QuaternionToDirectionCosines(const float* quaternion, float* directionCosines)
{
	float A[3][3];
	vtkMath::QuaternionToMatrix3x3(quaternion, A);
	directionCosines[0] = A[0][0];
	directionCosines[1] = A[0][1];
	directionCosines[2] = A[0][2];
	directionCosines[3] = A[1][0];
	directionCosines[4] = A[1][1];
	directionCosines[5] = A[1][2];

	// TODO: Necessary to orthonormalize???
	common::Orthonormalize(&directionCosines[0], &directionCosines[3]);
}

#define SWAP(X,Y) { \
		double temp = X ; \
		X = Y ; \
		Y = temp ; \
	}
void common::DirectionCosinesToQuaternion(const float* directionCosines, float* quaternion)
{
	// TODO: Necessary to orthonormalize direction cosines???
	// Construct direction cosine matrix
	float cross[3];
	vtkMath::Cross(&directionCosines[0], &directionCosines[3], cross);

	float A[3][3];
	for (unsigned short i = 0; i < 3; ++i) {
		A[0][i] = directionCosines[i];
		A[1][i] = directionCosines[i+3];
		A[2][i] = cross[i];
	}

	vtkMath::Matrix3x3ToQuaternion(A, quaternion);
}

bool common::Orthonormalize(float* vecA, float* vecB)
{
	// Check whether input valid
	bool ok = true;
	float precision = 1.0e-07;

	// Input vectors non-zero length
	float lenA = std::sqrtf(vtkMath::Dot(vecA, vecA));
	float lenB = std::sqrtf(vtkMath::Dot(vecB, vecB));
	ok &= lenA >= precision;
	ok &= lenB >= precision;

	// Input vectors not collinear
	float tmp = std::abs(vtkMath::Dot(vecA, vecB));
	ok &= std::abs(tmp - lenA*lenB) >= precision;
	if (!ok) {
		return false;
	}

	float cross[3];
	vtkMath::Cross(vecA, vecB, cross);

	float A[3][3];
	for (unsigned short i = 0; i < 3; ++i) {
		A[0][i] = vecA[i];
		A[1][i] = vecB[i];
		A[2][i] = cross[i];
	}

	float B[3][3];
	vtkMath::Orthogonalize3x3(A, B);
	for (unsigned short i = 0; i < 3; ++i) {
		vecA[i] = B[0][i];
		vecB[i] = B[1][i];
	}

	vtkMath::Normalize(vecA);
	vtkMath::Normalize(vecB);

	return true;
}

bool common::Normalize(float* vec)
{
	if (vtkMath::Dot(vec, vec) == 0.0f) {
		return false;
	}
	vtkMath::Normalize(vec);
	return true;
}

bool common::Cross(float* vecA, float* vecB, float* vecOut)
{
	bool ok = true;

	// Input vectors non-zero length
	float lenA = std::sqrtf(vtkMath::Dot(vecA, vecA));
	float lenB = std::sqrtf(vtkMath::Dot(vecB, vecB));
	ok &= lenA > 0.0f;
	ok &= lenB > 0.0f;

	// Input vectors not collinear
	float tmp = std::abs(vtkMath::Dot(vecA, vecB));
	ok &= std::abs(tmp - lenA*lenB) > 0.0f;
	if (!ok) {
		return false;
	}
	
	vtkMath::Cross(vecA, vecB, vecOut);
	return true;
}