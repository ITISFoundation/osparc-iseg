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

#include "iSegData.h"

#include <array>
#include <tuple>

namespace iseg {

class ISEG_DATA_API Color
{
public:
	union {
		std::array<float, 3> v;
		struct
		{
			float r, g, b;
		};
	};

	Color() : r(0.f), g(0.f), b(0.f) {}
	Color(float R, float G, float B) : r(R), g(G), b(B) {}
	Color(unsigned char R, unsigned char G, unsigned char B) : r(R / 255.f), g(G / 255.f), b(B / 255.f) {}

	float& operator[](int pos) { return v[pos]; }
	const float& operator[](int pos) const { return v[pos]; }

	/// given any current color, return the next color in a sequency of random colors
	static Color nextRandom(const Color& prev);

	std::tuple<unsigned char, unsigned char, unsigned char> toUChar() const;

	std::tuple<float, float, float> toHSL() const;

	static Color fromHSL(float fh, float fs, float fl);

private:
	static float hue2rgb(float p, float q, float t);
};

} // namespace iseg
