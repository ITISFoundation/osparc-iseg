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

#include "vec3.h"

#include <algorithm>


class Transform
{
public:
	float _m[4][4];

	Transform()
	{
		setIdentity();
	}

	Transform(const float m[4][4])
	{
		setTransform(m);
	}

	template<typename T>
	Transform(const T offset[3], const T dc[6])
	{
		setTransform(offset, dc);
	}

	void setIdentity()
	{
		for (int k = 0; k < 4; k++)
		{
			std::fill_n(_m[k], 4, 0.f);
			_m[k][k] = 1.f;
		}
	}

	static void setIdentity(float offset[3], float dc[6])
	{
		std::fill_n(offset, 3, 0.f);
		std::fill_n(dc, 6, 0.f);
		dc[0] = dc[4] = 1.f;
	}

	void setTransform(const float m[4][4])
	{
		for (int k = 0; k < 4; k++)
		{
			std::copy(m[k], m[k] + 4, _m[k]);
		}
	}

	template<typename T>
	void setTransform(const T offset[3], const T dc[6])
	{
		setIdentity();

		T d1[] = { dc[0], dc[1], dc[2] };
		T d2[] = { dc[3], dc[4], dc[5] };
		T d3[] = {
			d1[1] * d2[2] - d1[2] * d2[1],
			d1[2] * d2[0] - d1[0] * d2[2],
			d1[0] * d2[1] - d1[1] * d2[0]
		};

		for (unsigned int r = 0; r < 3; r++)
		{
			_m[r][0] = static_cast<float>(d1[r]);
			_m[r][1] = static_cast<float>(d2[r]);
			_m[r][2] = static_cast<float>(d3[r]);

			_m[r][3] = static_cast<float>(offset[r]);
		}
	}

	template<typename TVec3>
	void getOffset(TVec3& offset) const
	{
		for (unsigned int r = 0; r < 3; r++)
		{
			offset[r] = _m[r][3];
		}
	}

	template<typename TVec3>
	void setOffset(const TVec3& offset)
	{
		for (unsigned int r = 0; r < 3; r++)
		{
			_m[r][3] = offset[r];
		}
	}

	template<typename TRotation3x3>
	void getRotation(TRotation3x3& rot) const
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
	void setRotation(const TVec3& c0, const TVec3& c1, const TVec3& c2)
	{
		for (unsigned int r = 0; r < 3; r++)
		{
			_m[r][0] = c0[r];
			_m[r][1] = c1[r];
			_m[r][2] = c2[r];
		}
	}

	template<typename TVec3>
	TVec3 rigidTransformPoint(const TVec3& in) const
	{
		typedef typename TVec3::value_type T2;

		TVec3 out;
		out[0] = static_cast<T2>(_m[0][0] * in[0] + _m[0][1] * in[1] + _m[0][2] * in[2] + _m[0][3]);
		out[1] = static_cast<T2>(_m[1][0] * in[0] + _m[1][1] * in[1] + _m[1][2] * in[2] + _m[1][3]);
		out[2] = static_cast<T2>(_m[2][0] * in[0] + _m[2][1] * in[1] + _m[2][2] * in[2] + _m[2][3]);
		return out;
	}

	// for padding, padding_lo[.] is positive
	// for cropping, padding_lo[.] is negative
	void paddingUpdateTransform(const int padding_lo[3], const float spacing[3])
	{
		vec3 new_corner_before_transform(-padding_lo[0] * spacing[0], -padding_lo[1] * spacing[1], -padding_lo[2] * spacing[2]);
		vec3 p1 = rigidTransformPoint(vec3(0, 0, 0));
		vec3 p2 = rigidTransformPoint(new_corner_before_transform);

		vec3 t_before;
		getOffset(t_before);
		setOffset(t_before + (p2 - p1));
	}

	float* operator[](size_t row)
	{
		return _m[row];
	}
	const float* operator[](size_t row) const
	{
		return _m[row];
	}

	bool operator!=(const Transform& other) const
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
};
