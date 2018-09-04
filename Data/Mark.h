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

#include "Point.h"

#include <string>

namespace iseg {

struct Mark
{
	Mark(unsigned label = 0) : mark(label) {}
	Mark(const Mark& r) : p(r.p), mark(r.mark), name(r.name) {}

	static const unsigned RED = -1;
	static const unsigned GREEN = -2;
	static const unsigned BLUE = -3;
	static const unsigned WHITE = -4;

	union {
		Point p;
		struct { short px, py; };
	};
	unsigned mark;
	std::string name;
};

struct augmentedmark
{
	Point p;
	unsigned short slicenr;
	unsigned mark;
	std::string name;
};

inline bool operator!=(augmentedmark a, augmentedmark b)
{
	return (a.p.px != b.p.px) || (a.p.py != b.p.py) ||
				 (a.slicenr != b.slicenr) || (a.mark != b.mark) || (a.name != b.name);
}

} // namespace iseg
