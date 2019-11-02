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

#include "Vec3.h"
#include "iSegData.h"

#include <algorithm>

namespace iseg {

class ISEG_DATA_API Transform
{
public:
	float _m[4][4];

	/// initialized as identity transform
	Transform();
	Transform(const float m[4][4]);

	template<typename T>
	Transform(const T offset[3], const T dc[6])
	{
		setTransform(offset, dc);
	}

	void setIdentity();

	static void setIdentity(float offset[3], float dc[6]);

	void setTransform(const float m[4][4]);

	template<typename T>
	void setTransform(const T offset[3], const T dc[6]);

	template<typename TVec3>
	void getOffset(TVec3& offset) const;

	template<typename TVec3>
	void setOffset(const TVec3& offset);

	template<typename T>
	void getDirectionCosines(T dc[6]) const;

	template<typename TRotation3x3>
	void getRotation(TRotation3x3& rot) const;

	template<typename TRotation3x3>
	void setRotation(const TRotation3x3& rot);

	template<typename TVec3>
	void setRotation(const TVec3& c0, const TVec3& c1, const TVec3& c2);

	template<typename TVec3>
	TVec3 rigidTransformPoint(const TVec3& in) const;

	// for padding, padding_lo[.] is positive
	// for cropping, padding_lo[.] is negative
	void paddingUpdateTransform(const int padding_lo[3], const Vec3& spacing);

	float* operator[](size_t row) { return _m[row]; }
	const float* operator[](size_t row) const { return _m[row]; }

	bool operator!=(const Transform& other) const;
};

template<typename TVec3>
void Transform::setRotation(const TVec3& c0, const TVec3& c1, const TVec3& c2)
{
	for (unsigned int r = 0; r < 3; r++)
	{
		_m[r][0] = c0[r];
		_m[r][1] = c1[r];
		_m[r][2] = c2[r];
	}
}

template<typename TRotation3x3>
void Transform::setRotation(const TRotation3x3& rot)
{
	for (unsigned int r = 0; r < 3; r++)
	{
		for (unsigned int c = 0; c < 3; c++)
		{
			_m[r][c] = rot[r][c];
		}
	}
}

template<typename TRotation3x3>
void Transform::getRotation(TRotation3x3& rot) const
{
	for (unsigned int r = 0; r < 3; r++)
	{
		for (unsigned int c = 0; c < 3; c++)
		{
			rot[r][c] = _m[r][c];
		}
	}
}

template<typename TVec3>
void Transform::setOffset(const TVec3& offset)
{
	for (unsigned int r = 0; r < 3; r++)
	{
		_m[r][3] = offset[r];
	}
}

template<typename TVec3>
void Transform::getOffset(TVec3& offset) const
{
	for (unsigned int r = 0; r < 3; r++)
	{
		offset[r] = _m[r][3];
	}
}

template<typename TVec3>
TVec3 Transform::rigidTransformPoint(const TVec3& in) const
{
	typedef typename TVec3::value_type T2;

	TVec3 out;
	out[0] = static_cast<T2>(_m[0][0] * in[0] + _m[0][1] * in[1] + _m[0][2] * in[2] + _m[0][3]);
	out[1] = static_cast<T2>(_m[1][0] * in[0] + _m[1][1] * in[1] + _m[1][2] * in[2] + _m[1][3]);
	out[2] = static_cast<T2>(_m[2][0] * in[0] + _m[2][1] * in[1] + _m[2][2] * in[2] + _m[2][3]);
	return out;
}

template<typename T>
void Transform::getDirectionCosines(T dc[6]) const
{
	dc[0] = _m[0][0];
	dc[1] = _m[1][0];
	dc[2] = _m[2][0];
	dc[3] = _m[0][1];
	dc[4] = _m[1][1];
	dc[5] = _m[2][1];
}

template<typename T>
void Transform::setTransform(const T offset[3], const T dc[6])
{
	setIdentity();

	T d1[] = {dc[0], dc[1], dc[2]};
	T d2[] = {dc[3], dc[4], dc[5]};
	T d3[] = {d1[1] * d2[2] - d1[2] * d2[1], d1[2] * d2[0] - d1[0] * d2[2], d1[0] * d2[1] - d1[1] * d2[0]};

	for (unsigned int r = 0; r < 3; r++)
	{
		_m[r][0] = static_cast<float>(d1[r]);
		_m[r][1] = static_cast<float>(d2[r]);
		_m[r][2] = static_cast<float>(d3[r]);

		_m[r][3] = static_cast<float>(offset[r]);
	}
}

} // namespace iseg
