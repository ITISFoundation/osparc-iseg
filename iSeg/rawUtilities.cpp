/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "rawUtilities.hpp"
#include "GPUMC.h"

unsigned char *readRawFile(const char *filename, int sizeX, int sizeY,
													 int sizeZ, int stepSizeX, int stepSizeY,
													 int stepSizeZ)
{
	// Parse the specified raw file
	int rawDataSize = sizeX * sizeY * sizeZ;
	unsigned char *rawVoxels = new unsigned char[rawDataSize];
	FILE *file = fopen(filename, "rb");
	if (file == NULL)
		return NULL;

	fread(rawVoxels, sizeof(unsigned char), rawDataSize, file);
	if (stepSizeX == 1 && stepSizeY == 1 && stepSizeZ == 1)
		return rawVoxels;

	unsigned char *voxels =
			new unsigned char[rawDataSize / (stepSizeX * stepSizeY * stepSizeZ)];
	int i = 0;
	for (int z = 0; z < sizeZ; z += stepSizeZ)
	{
		for (int y = 0; y < sizeY; y += stepSizeY)
		{
			for (int x = 0; x < sizeX; x += stepSizeX)
			{
				voxels[i] = rawVoxels[x + y * sizeX + z * sizeX * sizeY];
				i++;
			}
		}
	}
	delete rawVoxels;
	return voxels;
}
