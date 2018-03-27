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

#include "Core/Types.h"
#include <vector>

class TissueCleaner
{
public:
	TissueCleaner(tissues_size_t **slices1, unsigned short n1,
								unsigned short width1, unsigned short height1);
	~TissueCleaner();
	bool Allocate();
	void ConnectedComponents();
	void Clean(float ratio, unsigned minsize);
	void MakeStat();

private:
	int base_connection(int c);
	std::vector<int> map;
	std::vector<tissues_size_t> tissuemap;
	std::vector<unsigned> volumes;
	unsigned totvolumes[TISSUES_SIZE_MAX + 1];
	int *volume;
	tissues_size_t **slices;
	unsigned short width, height;
	unsigned short n;
};