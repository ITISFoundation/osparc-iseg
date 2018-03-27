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

#include "iSegCore.h"

class vtkPolyData;
class Transform;

class iSegCore_API VoxelSurface
{
public:
	VoxelSurface(float fg = 255.f) : m_ForeGroundValue(fg) {}

	enum eSurfaceImageOverlap { kNone, kPartial, kContained };

	eSurfaceImageOverlap Run(const char *filename, const unsigned dims[3],
													 const float spacing[3], const Transform &transform,
													 float **slices, unsigned startslice,
													 unsigned endslice) const;
	eSurfaceImageOverlap Run(vtkPolyData *surface, const unsigned dims[3],
													 const float spacing[3], const Transform &transform,
													 float **slices, unsigned startslice,
													 unsigned endslice) const;

private:
	float m_ForeGroundValue;
};