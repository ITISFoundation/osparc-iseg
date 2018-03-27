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

#include <cmath>

class vec3
{
public:
	typedef float value_type;

	float v[3];

	vec3()
	{
		v[0] = 0;
		v[1] = 0;
		v[2] = 0;
	}
	vec3(float x, float y, float z)
	{
		v[0] = x;
		v[1] = y;
		v[2] = z;
	}
	vec3 &operator=(const vec3 &invec)
	{
		v[0] = invec[0];
		v[1] = invec[1];
		v[2] = invec[2];
		return *this;
	}
	bool operator==(const vec3 &invec) const
	{
		return ((v[0] == invec[0]) && (v[1] == invec[1]) && (v[2] == invec[2]));
	}
	float &operator[](int pos) { return v[pos]; }
	const float &operator[](int pos) const { return v[pos]; }
	void normalize()
	{
		float n = std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
		v[0] /= n;
		v[1] /= n;
		v[2] /= n;
	}
};

inline vec3 operator+(const vec3 &a, const vec3 &b)
{
	return vec3(a.v[0] + b.v[0], a.v[1] + b.v[1], a.v[2] + b.v[2]);
}

inline vec3 operator-(const vec3 &a, const vec3 &b)
{
	return vec3(a.v[0] - b.v[0], a.v[1] - b.v[1], a.v[2] - b.v[2]);
}

template<typename S> inline vec3 operator*(const S &a, const vec3 &b)
{
	return vec3(a * b.v[0], a * b.v[1], a * b.v[2]);
}

inline float operator*(const vec3 &a, const vec3 &b)
{
	return a.v[0] * b.v[0] + a.v[1] * b.v[1] + a.v[2] * b.v[2];
}

inline float dot(const vec3 &a, const vec3 &b) { return a * b; }

inline vec3 operator^(const vec3 &a, const vec3 &b)
{
	return vec3(a.v[1] * b.v[2] - a.v[2] * b.v[1],
							a.v[2] * b.v[0] - a.v[0] * b.v[2],
							a.v[0] * b.v[1] - a.v[1] * b.v[0]);
}

inline vec3 cross(const vec3 &a, const vec3 &b) { return a ^ b; }
