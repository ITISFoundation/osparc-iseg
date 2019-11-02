/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	setIdentity();
}

Transform::Transform(const float m[4][4])
{
	setTransform(m);
}

void Transform::setIdentity()
{
	for (int k = 0; k < 4; k++)
	{
		std::fill_n(_m[k], 4, 0.f);
		_m[k][k] = 1.f;
	}
}

void Transform::setIdentity(float offset[3], float dc[6])
{
	std::fill_n(offset, 3, 0.f);
	std::fill_n(dc, 6, 0.f);
	dc[0] = dc[4] = 1.f;
}

void Transform::setTransform(const float m[4][4])
{
	for (int k = 0; k < 4; k++)
	{
		std::copy(m[k], m[k] + 4, _m[k]);
	}
}

void Transform::paddingUpdateTransform(const int padding_lo[3], const Vec3& spacing)
{
	Vec3 new_corner_before_transform(-padding_lo[0] * spacing[0],
			-padding_lo[1] * spacing[1],
			-padding_lo[2] * spacing[2]);
	Vec3 p1 = rigidTransformPoint(Vec3(0, 0, 0));
	Vec3 p2 = rigidTransformPoint(new_corner_before_transform);

	Vec3 t_before;
	getOffset(t_before);
	setOffset(t_before + (p2 - p1));
}

bool Transform::operator!=(const Transform& other) const
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			if (_m[i][j] != other._m[i][j])
				return true;
		}
	}
	return false;
}

} // namespace iseg
