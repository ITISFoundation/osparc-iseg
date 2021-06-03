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

#include "Data/Transform.h"

#include <vector>

class vtkPolyData; // NOLINT

namespace iseg {

class Transform;

class ISEG_CORE_API VoxelSurface
{
public:
	VoxelSurface(float fg = 255.f) : m_ForeGroundValue(fg) {}

	enum eSurfaceImageOverlap {
		kNone = 0,
		kContained = 1,
		kPartial = 2
	};

	eSurfaceImageOverlap Voxelize(vtkPolyData* polydata, std::vector<float*>& all_slices, const unsigned dims[3], const Vec3& spacing, const Transform& transform, unsigned startslice, unsigned endslice) const;

	eSurfaceImageOverlap Intersect(vtkPolyData* surface, std::vector<float*>& all_slices, const unsigned dims[3], const Vec3& spacing, const Transform& transform, unsigned startslice, unsigned endslice, double relative_tolerance = 0.1) const;

private:
	float m_ForeGroundValue;
};

} // namespace iseg
