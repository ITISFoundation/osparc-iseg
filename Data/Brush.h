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

namespace iseg {

template<typename T, typename F>
void brush(T* slice_data, unsigned slice_width, unsigned slice_height, float dx, float dy,
		Point p, float const radius, bool draw_or_modify, T f, T f1, F is_locked)
{
	float const radius_corrected = dx > dy ? std::floor(radius / dx + 0.5f) * dx : std::floor(radius / dy + 0.5f) * dy;
	float const radius_corrected2 = radius_corrected * radius_corrected;

	int const xradius = static_cast<int>(std::ceil(radius_corrected / dx));
	int const yradius = static_cast<int>(std::ceil(radius_corrected / dy));
	for (int x = std::max(0, p.px - xradius); x <= std::min(static_cast<int>(slice_width) - 1, p.px + xradius); x++)
	{
		for (int y = std::max(0, p.py - yradius); y <= std::min(static_cast<int>(slice_height) - 1, p.py + yradius); y++)
		{
			// don't modify locked pixels
			if (is_locked(slice_data[y * slice_width + x]))
				continue;

			if (std::pow(dx * (p.px - x), 2.f) + std::pow(dy * (p.py - y), 2.f) <= radius_corrected2)
			{
				if (draw_or_modify)
				{
					slice_data[y * slice_width + x] = f;
				}
				else
				{
					if (slice_data[y * slice_width + x] == f)
						slice_data[y * slice_width + x] = f1;
				}
			}
		}
	}
}

template<typename T, typename F>
void brush(T* slice_data, unsigned slice_width, unsigned slice_height,
		Point p, int radius, bool draw_or_modify, T f, T f1, F is_locked)
{
	unsigned short dist = radius * radius;

	int xmin, xmax, ymin, ymax, d;
	if (p.px < radius)
		xmin = 0;
	else
		xmin = int(p.px - radius);
	if (p.px + radius >= slice_width)
		xmax = int(slice_width - 1);
	else
		xmax = int(p.px + radius);

	for (int x = xmin; x <= xmax; x++)
	{
		d = int(std::floor(std::sqrt(float(dist - (x - p.px) * (x - p.px)))));
		ymin = std::max(0, int(p.py) - d);
		ymax = std::min(int(height - 1), d + p.py);

		for (int y = ymin; y <= ymax; y++)
		{
			// don't modify locked pixels
			if (is_locked(slice_data[y * unsigned(slice_width) + x]))
				continue;

			if (draw_or_modify)
			{
				slice_data[y * unsigned(slice_width) + x] = f;
			}
			else
			{
				if (slice_data[y * unsigned(slice_width) + x] == f)
					slice_data[y * unsigned(slice_width) + x] = f1;
			}
		}
	}
}

} // namespace iseg
