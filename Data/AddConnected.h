/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#pragma

#include <vector>

namespace iseg {

template<typename T1, typename T2, typename TWritable>
void add_connected_2d(T1* source, T2* target, unsigned width, unsigned height, unsigned position, T2 new_value, const TWritable& writable)
{
	//T1 f = work_bits[position];

	const unsigned char k_object = 1;
	const unsigned char k_background = 0;
	const unsigned char k_writable = -1;
	const unsigned area = width * height;
	std::vector<unsigned char> results(area + 2 * width + 2 * height + 4);

	unsigned i = width + 3;
	unsigned i1 = 0;
	for (unsigned j = 0; j < height; j++, i += 2)
	{
		for (unsigned k = 0; k < width; k++, i++, i1++)
		{
			//if (work_bits[i1] == f && (tissues[i1] == 0 || (override && !TissueInfos::GetTissueLocked(tissues[i1]))))
			if (writable(i1))
			{
				results[i] = k_writable;
			}
			else
			{
				results[i] = k_background;
			}
		}
	}

	for (unsigned j = 0; j < width + 2; j++)
		results[j] = results[j + (width + 2) * (height + 1)] = 0;
	for (unsigned j = 0; j <= (width + 2) * (height + 1); j += width + 2)
		results[j] = results[j + width + 1] = 0;

	std::vector<unsigned> s;
	s.push_back(position % width + 1 + (position / width + 1) * (width + 2));
	if (results[s.back()] == k_writable)
		results[s.back()] = k_object;

	unsigned w = width + 2;

	while (!s.empty())
	{
		i = s.back();
		s.pop_back();
		if (results[i - 1] == k_writable)
		{
			results[i - 1] = k_object;
			s.push_back(i - 1);
		}
		if (results[i + 1] == k_writable)
		{
			results[i + 1] = k_object;
			s.push_back(i + 1);
		}
		if (results[i - w] == k_writable)
		{
			results[i - w] = k_object;
			s.push_back(i - w);
		}
		if (results[i + w] == k_writable)
		{
			results[i + w] = k_object;
			s.push_back(i + w);
		}
	}

	i = width + 3;
	unsigned i2 = 0;
	for (unsigned j = 0; j < height; j++, i += 2)
	{
		for (unsigned k = 0; k < width; k++, i++, i2++)
		{
			if (results[i] == k_object)
			{
				target[i2] = new_value;
			}
		}
	}
}

} // namespace iseg
