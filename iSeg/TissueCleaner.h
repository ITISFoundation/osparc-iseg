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

#include "Data/Types.h"

#include <vector>

namespace iseg {

class TissueCleaner
{
public:
	TissueCleaner(tissues_size_t** slices1, unsigned short n1, unsigned short width1, unsigned short height1);
	~TissueCleaner();
	bool Allocate();
	void ConnectedComponents();
	void Clean(float ratio, unsigned minsize);
	void MakeStat();

private:
	int BaseConnection(int c);
	std::vector<int> m_Map;
	std::vector<tissues_size_t> m_Tissuemap;
	std::vector<unsigned> m_Volumes;
	unsigned m_Totvolumes[TISSUES_SIZE_MAX + 1];
	int* m_Volume;
	tissues_size_t** m_Slices;
	size_t m_Width, m_Height;
	size_t m_Nrslices;
};

} // namespace iseg
