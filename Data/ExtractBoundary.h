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

#include <vector>

namespace iseg {

template<typename TPoint, typename T, typename TComparator>
std::vector<TPoint> extract_boundary(const T* bits, unsigned width, unsigned height, const TPoint& exemplar, const TComparator& compare)
{
	std::vector<TPoint> vp;
	TPoint p = exemplar;

	unsigned pos = 0;

	if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos + width])
	{
		p.px = 0;
		p.py = 0;
		if (compare(bits[pos]))
			vp.push_back(p);
	}
	pos++;
	for (unsigned j = 1; j + 1 < width; j++)
	{
		if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos - 1] || bits[pos] != bits[pos + width])
		{
			p.px = j;
			p.py = 0;
			if (compare(bits[pos]))
				vp.push_back(p);
		}
		pos++;
	}
	if (bits[pos] != bits[pos - 1] || bits[pos] != bits[pos + width])
	{
		p.px = width - 1;
		p.py = 0;
		if (compare(bits[pos]))
			vp.push_back(p);
	}
	pos++;

	for (unsigned i = 1; i + 1 < height; i++)
	{
		if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos + width] || bits[pos] != bits[pos - width])
		{
			p.px = 0;
			p.py = i;
			if (compare(bits[pos]))
				vp.push_back(p);
		}
		pos++;
		for (unsigned j = 1; j + 1 < width; j++)
		{
			if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos - 1] ||
					bits[pos] != bits[pos + width] || bits[pos] != bits[pos - width])
			{
				p.px = j;
				p.py = i;
				if (compare(bits[pos]))
					vp.push_back(p);
			}
			pos++;
		}
		if (bits[pos] != bits[pos - 1] ||
				bits[pos] != bits[pos + width] || bits[pos] != bits[pos - width])
		{
			p.px = width - 1;
			p.py = i;
			if (compare(bits[pos]))
				vp.push_back(p);
		}
		pos++;
	}
	if (bits[pos] != bits[pos + 1] || bits[pos] != bits[pos - width])
	{
		p.px = 0;
		p.py = height - 1;
		if (compare(bits[pos]))
			vp.push_back(p);
	}
	pos++;
	for (unsigned j = 1; j + 1 < width; j++)
	{
		if (bits[pos] != bits[pos + 1] ||
				bits[pos] != bits[pos - 1] || bits[pos] != bits[pos - width])
		{
			p.px = j;
			p.py = height - 1;
			if (compare(bits[pos]))
				vp.push_back(p);
		}
		pos++;
	}
	if (bits[pos] != bits[pos - 1] || bits[pos] != bits[pos - width])
	{
		p.px = width - 1;
		p.py = height - 1;
		if (compare(bits[pos]))
			vp.push_back(p);
	}

	return vp;
}

template<typename TPoint, typename T>
std::vector<TPoint> extract_boundary(const T* bits, unsigned width, unsigned height, const TPoint& exemplar)
{
	return extract_boundary(bits, width, height, exemplar, [](T v) { return (v != 0); });
}

} // namespace iseg
