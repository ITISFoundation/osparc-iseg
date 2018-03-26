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

#include "iSegCore.h"

#include <algorithm>
#include <functional>
#include <vector>
#include <iostream>

class iSegCore_API index_priorqueue
{
	public:
		index_priorqueue(unsigned size2,float *valuemap1);
		unsigned pop();
		unsigned size();
		void insert(unsigned pos);
		void insert(unsigned pos, float value);
		void change(unsigned pos, float value);
		void make_smaller(unsigned pos, float value);
		void make_larger(unsigned pos, float value);
		void remove(unsigned pos);
		void print_queue();
		bool empty();
		void clear();
		bool in_queue(unsigned pos);
		~index_priorqueue();
	private:
		int *indexmap;
		float *valuemap;
		std::vector<unsigned> Q;
		unsigned l;
		unsigned size1;
};
