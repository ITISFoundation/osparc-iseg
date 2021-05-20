/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#include "Transform.h"

namespace iseg {

Transform::Transform()
{
	SetIdentity();
}

Transform::Transform(const float m[4][4])
{
	SetTransform(m);
}

void Transform::SetIdentity()
{
	for (int k = 0; k < 4; k++)
	{
		std::fill_n(m_M[k], 4, 0.f);
		m_M[k][k] = 1.f;
	}
}

void Transform::SetIdentity(float offset[3], float dc[6])
{
	std::fill_n(offset, 3, 0.f);
	std::fill_n(dc, 6, 0.f);
	dc[0] = dc[4] = 1.f;
}

void Transform::SetTransform(const float m[4][4])
{
	for (int k = 0; k < 4; k++)
	{
		std::copy(m[k], m[k] + 4, m_M[k]);
	}
}

void Transform::PaddingUpdateTransform(const int padding_lo[3], const Vec3& spacing)
{
	Vec3 new_corner_before_transform(-padding_lo[0] * spacing[0], -padding_lo[1] * spacing[1], -padding_lo[2] * spacing[2]);
	Vec3 p1 = RigidTransformPoint(Vec3(0, 0, 0));
	Vec3 p2 = RigidTransformPoint(new_corner_before_transform);

	Vec3 t_before;
	GetOffset(t_before);
	SetOffset(t_before + (p2 - p1));
}

bool Transform::operator!=(const Transform& other) const
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (m_M[i][j] != other.m_M[i][j])
				return true;
		}
	}
	return false;
}

} // namespace iseg
