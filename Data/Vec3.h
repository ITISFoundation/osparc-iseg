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

#include <cmath>

namespace iseg {

class Vec3
{
public:
	using value_type = float;

	union {
		float v[3]; // NOLINT
		struct
		{
			float x, y, z; // NOLINT
		};
	};

	Vec3()
	{
		v[0] = 0;
		v[1] = 0;
		v[2] = 0;
	}
	Vec3(float ix, float iy, float iz)
	{
		v[0] = ix;
		v[1] = iy;
		v[2] = iz;
	}
	Vec3(const Vec3&) = default;
	Vec3& operator=(const Vec3&) = default;

	bool operator==(const Vec3& r) const
	{
		return v[0] == r.v[0] && v[1] == r.v[1] && v[2] == r.v[2];
	}
	float& operator[](int pos) { return v[pos]; }
	const float& operator[](int pos) const { return v[pos]; }
	void Normalize()
	{
		float l = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		v[0] /= l;
		v[1] /= l;
		v[2] /= l;
	}
};

inline Vec3 operator+(const Vec3& a, const Vec3& b)
{
	return Vec3(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2]);
}

inline Vec3 operator-(const Vec3& a, const Vec3& b)
{
	return Vec3(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2]);
}

template<typename S>
inline Vec3 operator*(const S& a, const Vec3& b)
{
	return Vec3(a * b.v[0], a * b.v[1], a * b.v[2]);
}

inline float operator*(const Vec3& a, const Vec3& b)
{
	return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2];
}

inline float dot(const Vec3& a, const Vec3& b) { return a * b; }

inline Vec3 operator^(const Vec3& a, const Vec3& b)
{
	return Vec3(a.v[1] * b.v[2] - a.v[2] * b.v[1], a.v[2] * b.v[0] - a.v[0] * b.v[2], a.v[0] * b.v[1] - a.v[1] * b.v[0]);
}

inline Vec3 cross(const Vec3& a, const Vec3& b) { return a ^ b; }

} // namespace iseg
