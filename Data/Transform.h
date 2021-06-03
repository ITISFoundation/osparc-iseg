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

#include "Vec3.h"
#include "iSegData.h"

#include <algorithm>

namespace iseg {

class ISEG_DATA_API Transform
{
	float m_M[4][4];

public:
	/// initialized as identity transform
	Transform();
	Transform(const float m[4][4]);

	template<typename T>
	Transform(const T offset[3], const T dc[6])
	{
		SetTransform(offset, dc);
	}

	void SetIdentity();

	static void SetIdentity(float offset[3], float dc[6]);

	void SetTransform(const float m[4][4]);

	template<typename T>
	void SetTransform(const T offset[3], const T dc[6]);

	template<typename TVec3>
	void GetOffset(TVec3& offset) const;

	template<typename TVec3>
	void SetOffset(const TVec3& offset);

	template<typename T>
	void GetDirectionCosines(T dc[6]) const;

	template<typename TRotation3x3>
	void GetRotation(TRotation3x3& rot) const;

	template<typename TRotation3x3>
	void SetRotation(const TRotation3x3& rot);

	template<typename TVec3>
	void SetRotation(const TVec3& c0, const TVec3& c1, const TVec3& c2);

	template<typename TVec3>
	TVec3 RigidTransformPoint(const TVec3& in) const;

	// for padding, padding_lo[.] is positive
	// for cropping, padding_lo[.] is negative
	void PaddingUpdateTransform(const int padding_lo[3], const Vec3& spacing);

	float* operator[](size_t row) { return m_M[row]; }
	const float* operator[](size_t row) const { return m_M[row]; }

	bool operator!=(const Transform& other) const;
};

template<typename TVec3>
void Transform::SetRotation(const TVec3& c0, const TVec3& c1, const TVec3& c2)
{
	for (unsigned int r = 0; r < 3; r++)
	{
		m_M[r][0] = c0[r];
		m_M[r][1] = c1[r];
		m_M[r][2] = c2[r];
	}
}

template<typename TRotation3x3>
void Transform::SetRotation(const TRotation3x3& rot)
{
	for (unsigned int r = 0; r < 3; r++)
	{
		for (unsigned int c = 0; c < 3; c++)
		{
			m_M[r][c] = rot[r][c];
		}
	}
}

template<typename TRotation3x3>
void Transform::GetRotation(TRotation3x3& rot) const
{
	for (unsigned int r = 0; r < 3; r++)
	{
		for (unsigned int c = 0; c < 3; c++)
		{
			rot[r][c] = m_M[r][c];
		}
	}
}

template<typename TVec3>
void Transform::SetOffset(const TVec3& offset)
{
	for (unsigned int r = 0; r < 3; r++)
	{
		m_M[r][3] = offset[r];
	}
}

template<typename TVec3>
void Transform::GetOffset(TVec3& offset) const
{
	for (unsigned int r = 0; r < 3; r++)
	{
		offset[r] = m_M[r][3];
	}
}

template<typename TVec3>
TVec3 Transform::RigidTransformPoint(const TVec3& in) const
{
	using t2_type = typename TVec3::value_type;

	TVec3 out;
	out[0] = static_cast<t2_type>(m_M[0][0] * in[0] + m_M[0][1] * in[1] + m_M[0][2] * in[2] + m_M[0][3]);
	out[1] = static_cast<t2_type>(m_M[1][0] * in[0] + m_M[1][1] * in[1] + m_M[1][2] * in[2] + m_M[1][3]);
	out[2] = static_cast<t2_type>(m_M[2][0] * in[0] + m_M[2][1] * in[1] + m_M[2][2] * in[2] + m_M[2][3]);
	return out;
}

template<typename T>
void Transform::GetDirectionCosines(T dc[6]) const
{
	dc[0] = m_M[0][0];
	dc[1] = m_M[1][0];
	dc[2] = m_M[2][0];
	dc[3] = m_M[0][1];
	dc[4] = m_M[1][1];
	dc[5] = m_M[2][1];
}

template<typename T>
void Transform::SetTransform(const T offset[3], const T dc[6])
{
	SetIdentity();

	T d1[] = {dc[0], dc[1], dc[2]};
	T d2[] = {dc[3], dc[4], dc[5]};
	T d3[] = {d1[1] * d2[2] - d1[2] * d2[1], d1[2] * d2[0] - d1[0] * d2[2], d1[0] * d2[1] - d1[1] * d2[0]};

	for (unsigned int r = 0; r < 3; r++)
	{
		m_M[r][0] = static_cast<float>(d1[r]);
		m_M[r][1] = static_cast<float>(d2[r]);
		m_M[r][2] = static_cast<float>(d3[r]);

		m_M[r][3] = static_cast<float>(offset[r]);
	}
}

} // namespace iseg
