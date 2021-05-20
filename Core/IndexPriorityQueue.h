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

#include "iSegCore.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

namespace iseg {

class ISEG_CORE_API IndexPriorityQueue
{
public:
	IndexPriorityQueue(unsigned size2, float* valuemap1);
	unsigned Pop();
	unsigned Size() const;
	void Insert(unsigned pos);
	void Insert(unsigned pos, float value);
	void Change(unsigned pos, float value);
	void MakeSmaller(unsigned pos, float value);
	void MakeLarger(unsigned pos, float value);
	void Remove(unsigned pos);
	void PrintQueue();
	bool Empty() const;
	void Clear();
	bool InQueue(unsigned pos);
	~IndexPriorityQueue();

private:
	int* m_Indexmap;
	float* m_Valuemap;
	std::vector<unsigned> m_Q;
	unsigned m_L;
	unsigned m_Size1;
};
} // namespace iseg
