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

#include "Point.h"
#include "Types.h"

#include <string>

namespace iseg {

struct Mark
{
	Mark(unsigned label = 0) : mark(label) {}
	Mark(const Mark& r) : p(r.p), mark(r.mark), name(r.name) {}

	static const tissues_size_t red = -1;
	static const tissues_size_t green = -2;
	static const tissues_size_t blue = -3;
	static const tissues_size_t white = -4;

	union {
		Point p; // NOLINT
		struct
		{
			short px, py; // NOLINT
		};
	};
	unsigned mark;		// NOLINT
	std::string name; // NOLINT
};

struct AugmentedMark
{
	Point p;								// NOLINT
	unsigned short slicenr; // NOLINT
	unsigned mark;					// NOLINT
	std::string name;				// NOLINT
};

inline bool operator!=(AugmentedMark a, AugmentedMark b)
{
	return (a.p.px != b.p.px) || (a.p.py != b.p.py) || (a.slicenr != b.slicenr) || (a.mark != b.mark) || (a.name != b.name);
}

} // namespace iseg
