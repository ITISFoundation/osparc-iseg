/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "TissueCleaner.h"
#include "TissueInfos.h"

#include <cstdlib>

using namespace iseg;

int TissueCleaner::base_connection(int c)
{
	if (c == map[c])
		return c;
	else
		return map[c] = base_connection(map[c]);
}

TissueCleaner::TissueCleaner(tissues_size_t** slices1, unsigned short n1,
		unsigned short width1, unsigned short height1)
{
	slices = slices1;
	nrslices = static_cast<size_t>(n1);
	width = static_cast<size_t>(width1);
	height = static_cast<size_t>(height1);
	volume = nullptr;
	map.clear();
}

TissueCleaner::~TissueCleaner() { free(volume); }

bool TissueCleaner::Allocate()
{
	volume = (int*)malloc(sizeof(int) * width * height * nrslices);
	if (volume == nullptr)
		return false;
	return true;
}

void TissueCleaner::ConnectedComponents()
{
	map.clear();
	int newest = 0;

	size_t offset[4];
	offset[0] = 1;
	offset[1] = width;
	offset[2] = width * height;
	offset[3] = offset[2] * nrslices;

	size_t i1 = 0;
	size_t i2 = 0;

	// init first pixel
	map.push_back(newest);
	tissuemap.push_back(slices[0][i1]);
	volume[i1] = newest;
	newest++;
	i1++;
	// init first row
	for (unsigned short k = 1; k < width; k++)
	{
		if (slices[0][i1] == slices[0][i1 - 1])
		{
			volume[i1] = base_connection(volume[i1 - 1]);
		}
		else
		{
			map.push_back(newest);
			tissuemap.push_back(slices[0][i1]);
			volume[i1] = newest;
			newest++;
		}
		i1++;
	}
	// init first slice
	for (unsigned short j = 1; j < height; j++)
	{
		if (slices[0][i1] == slices[0][i1 - width])
		{
			volume[i1] = base_connection(volume[i1 - width]);
		}
		else
		{
			map.push_back(newest);
			tissuemap.push_back(slices[0][i1]);
			volume[i1] = newest;
			newest++;
		}
		i1++;
		for (unsigned short k = 1; k < width; k++)
		{
			if (slices[0][i1] == slices[0][i1 - 1])
			{
				volume[i1] = base_connection(volume[i1 - 1]);
				if (slices[0][i1] == slices[0][i1 - width])
				{
					map[(int)base_connection(volume[i1 - width])] = volume[i1];
				}
			}
			else
			{
				if (slices[0][i1] == slices[0][i1 - width])
				{
					volume[i1] = base_connection(volume[i1 - width]);
				}
				else
				{
					map.push_back(newest);
					tissuemap.push_back(slices[0][i1]);
					volume[i1] = newest;
					newest++;
				}
			}
			i1++;
		}
	}
	// for each slice >= 1
	for (unsigned short i = 1; i < nrslices; i++)
	{
		i2 = 0;
		if (slices[i][i2] == slices[i - 1][i2])
			volume[i1] = base_connection(volume[i1 - offset[2]]);
		else
		{
			map.push_back(newest);
			tissuemap.push_back(slices[i][i2]);
			volume[i1] = newest;
			newest++;
		}
		i1++;
		i2++;
		for (unsigned short k = 1; k < width; k++)
		{
			if (slices[i][i2] == slices[i][i2 - 1])
			{
				volume[i1] = base_connection(volume[i1 - 1]);
				if (slices[i][i2] == slices[i - 1][i2])
				{
					map[(int)base_connection(volume[i1 - offset[2]])] =
							volume[i1];
				}
			}
			else
			{
				if (slices[i][i2] == slices[i - 1][i2])
					volume[i1] = base_connection(volume[i1 - offset[2]]);
				else
				{
					map.push_back(newest);
					tissuemap.push_back(slices[i][i2]);
					volume[i1] = newest;
					newest++;
				}
			}
			i1++;
			i2++;
		}
		for (unsigned short j = 1; j < height; j++)
		{
			if (slices[i][i2] == slices[i][i2 - width])
			{
				volume[i1] = base_connection(volume[i1 - width]);
				if (slices[i][i2] == slices[i - 1][i2])
				{
					map[(int)base_connection(volume[i1 - offset[2]])] =
							volume[i1];
				}
			}
			else
			{
				if (slices[i][i2] == slices[i - 1][i2])
					volume[i1] = base_connection(volume[i1 - offset[2]]);
				else
				{
					map.push_back(newest);
					tissuemap.push_back(slices[i][i2]);
					volume[i1] = newest;
					newest++;
				}
			}
			i1++;
			i2++;
			for (unsigned short k = 1; k < width; k++)
			{
				if (slices[i][i2] == slices[i][i2 - 1])
				{
					volume[i1] = base_connection(volume[i1 - 1]);
					if (slices[i][i2] == slices[i][i2 - width])
					{
						map[(int)base_connection(volume[i1 - width])] =
								volume[i1];
					}
					if (slices[i][i2] == slices[i - 1][i2])
					{
						map[(int)base_connection(volume[i1 - offset[2]])] =
								volume[i1];
					}
				}
				else
				{
					if (slices[i][i2] == slices[i][i2 - width])
					{
						volume[i1] = base_connection(volume[i1 - width]);
						if (slices[i][i2] == slices[i - 1][i2])
						{
							map[(int)base_connection(volume[i1 - offset[2]])] =
									volume[i1];
						}
					}
					else
					{
						if (slices[i][i2] == slices[i - 1][i2])
							volume[i1] =
									base_connection(volume[i1 - offset[2]]);
						else
						{
							map.push_back(newest);
							tissuemap.push_back(slices[i][i2]);
							volume[i1] = newest;
							newest++;
						}
					}
				}
				i1++;
				i2++;
			}
		}
	}

	// map connected regions to single ID
	for (size_t i = 0; i < offset[3]; i++)
		volume[i] = base_connection(volume[i]);
}

void TissueCleaner::MakeStat()
{
	for (unsigned i = 0; i < TISSUES_SIZE_MAX + 1; i++)
		totvolumes[i] = 0;
	volumes.resize(map.size(), 0);
	if (volume == nullptr)
		return;
	size_t maxi = nrslices * (size_t)width * size_t(height);
	for (unsigned long long i = 0; i < maxi; i++)
	{
		volumes[volume[i]]++;
	}
	for (size_t i = 0; i < map.size(); i++)
	{
		if (map[i] == i)
			totvolumes[tissuemap[i]] += volumes[i];
	}
}

void TissueCleaner::Clean(float ratio, unsigned minsize)
{
	if (volume == nullptr)
		return;
	std::vector<bool> erasemap;
	erasemap.resize(map.size(), false);
	for (size_t i = 0; i < map.size(); i++)
	{
		if (volumes[i] < minsize && volumes[i] < ratio * totvolumes[tissuemap[i]])
		{
			// only remove small components if tissue is NOT locked!
			if (!TissueInfos::GetTissueLocked(tissuemap[i]))
			{
				erasemap[i] = true;
			}
		}
	}

	tissues_size_t curchar = 0;
	size_t postot = 0;
	for (unsigned short i = 0; i < nrslices; i++)
	{
		size_t pos = 0;
		for (unsigned short j = 0; j < height; j++)
		{
			unsigned short k = 0;
			while (k < width && erasemap[volume[postot]])
			{
				postot++;
				k++;
			}
			if (k == width)
			{
				pos += width;
				//xxxa I should treat this case, but it is unlikely and I am tired...
			}
			else
			{
				size_t pos1 = pos + k;
				curchar = slices[i][pos1];
				for (; pos < pos1; pos++)
					slices[i][pos] = curchar;
				for (; k < width; k++, pos++, postot++)
				{
					if (erasemap[volume[postot]])
						slices[i][pos] = curchar;
					else
						curchar = slices[i][pos];
				}
			}
		}
	}
}
