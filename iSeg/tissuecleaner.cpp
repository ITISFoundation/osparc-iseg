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
	n = n1;
	width = width1;
	height = height1;
	volume = NULL;
	map.clear();
}

TissueCleaner::~TissueCleaner() { free(volume); }

bool TissueCleaner::Allocate()
{
	//	volume=(int *)malloc(sizeof(int)*(unsigned)(width+1)*(unsigned)(height+1)*(unsigned)(n+1));
	volume = (int*)malloc(sizeof(int) * (unsigned)(width) * (unsigned)(height) *
						  (unsigned)(n));
	if (volume == NULL)
		return false;
	return true;
}

void TissueCleaner::ConnectedComponents()
{
	map.clear();
	int newest = 0;

	unsigned offset[4];
	offset[0] = 1;
	offset[1] = (unsigned)width;
	offset[2] = (unsigned)width * (unsigned)height;
	offset[3] = offset[2] * n;

	//unsigned offset1[3];
	//unsigned offset2[3];
	//offset1[0]=offset2[0]=1;
	//offset1[1]=(unsigned)width+1;
	//offset2[1]=(unsigned)width;
	//offset1[2]=(unsigned)(width+1)*(unsigned)(height+1);
	//offset2[2]=(unsigned)width*(unsigned)height;

	//for(unsigned i=0;i<offset1[2];i++) volume[i]=-1;
	//for(unsigned short j=0;j<n;j++) {
	//	for(unsigned i=j*offset1[2]+offset1[1];i<(j+1)*offset1[2];i+=offset1[1]) volume[i]=-2;
	//	for(unsigned i=j*offset1[2];i<j*offset1[2]+offset1[1];i++) volume[i]=-3;
	//	unsigned k=0;
	//	for(unsigned l=1;l<=height;l++) {
	//		for(unsigned i=j*offset1[2]+offset1[1]*l+1;i<j*offset1[2]+offset1[1]*(l+1);i++,k++) volume[i]=slices[j][k];
	//	}
	//}

	unsigned long long i1 = 0;
	unsigned i2 = 0;

	map.push_back(newest);
	tissuemap.push_back(slices[0][i1]);
	volume[i1] = newest;
	newest++;
	i1++;
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
	for (unsigned short i = 1; i < n; i++)
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

	for (unsigned long long i = 0; i < offset[3]; i++)
		volume[i] = base_connection(volume[i]);

	//newest=0;
	//for(i1=0;i1<(unsigned)map.size();i1++) if((unsigned)map[i1]==i1){
	//	map[i1]=newest;
	//	newest++;
	//}

	//for(unsigned i=0;i<offset[3];i++) volume[i]=map[volume[i]];

	return;
}

void TissueCleaner::MakeStat()
{
	for (unsigned i = 0; i < TISSUES_SIZE_MAX + 1; i++)
		totvolumes[i] = 0;
	volumes.resize(map.size(), 0);
	if (volume == NULL)
		return;
	unsigned maxi = n * (unsigned)width * unsigned(height);
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
	if (volume == NULL)
		return;
	std::vector<bool> erasemap;
	erasemap.resize(map.size(), false);
	for (size_t i = 0; i < map.size(); i++)
	{
		if (volumes[i] < minsize &&
			volumes[i] < ratio * totvolumes[tissuemap[i]])
			erasemap[i] = true;
	}

	tissues_size_t curchar = 0;
	unsigned long long postot = 0;
	for (unsigned short i = 0; i < n; i++)
	{
		unsigned pos = 0;
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
				unsigned pos1 = pos + k;
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
