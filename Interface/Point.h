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

namespace iseg {

struct Point
{
	short px;
	short py;
};

inline float dist2(Point* p)
{
	return (float)(*p).px * (float)(*p).px + (float)(*p).py * (float)(*p).py;
}
inline float dist(Point* p) { return sqrt(dist2(p)); }
inline float dist2_b(Point* p1, Point* p2)
{
	return (float)((*p1).px - (*p2).px) * (float)((*p1).px - (*p2).px) +
		   (float)((*p1).py - (*p2).py) * (float)((*p1).py - (*p2).py);
}
inline float dist_b(Point* p1, Point* p2) { return sqrt(dist2_b(p1, p2)); }
inline float scalar(Point* p1, Point* p2)
{
	return (float)(*p1).px * (float)(*p2).px +
		   (float)(*p1).py * (float)(*p2).py;
}
inline float cross(Point* p1, Point* p2)
{
	return abs((float)(*p1).px * (float)(*p2).py -
			   (float)(*p1).py * (float)(*p2).px);
}

} // namespace iseg
