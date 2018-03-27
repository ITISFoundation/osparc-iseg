/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "index_priorqueue.h"
#include "Precompiled.h"

using namespace std;

index_priorqueue::index_priorqueue(unsigned size2, float *valuemap1)
{
	Q.clear();
	indexmap = (int *)malloc(size2 * sizeof(int));
	for (unsigned i = 0; i < size2; i++)
		indexmap[i] = -1;
	valuemap = valuemap1;
	l = 0;
	size1 = size2;
	return;
}

void index_priorqueue::clear()
{
	for (size_t i = 0; i < Q.size(); i++)
		indexmap[Q[i]] = -1;
	Q.clear();
	l = 0;
	return;
}

unsigned index_priorqueue::pop()
{
	if (l <= 1)
	{
		if (l == 1)
		{
			unsigned pos = Q.front();
			indexmap[pos] = -1;
			Q.pop_back();
			l = 0;
			return pos;
		}
		else
			return size1;
	}
	else
	{
		unsigned child, test_node;
		test_node = 0;
		unsigned pos = Q.front();
		indexmap[pos] = -1;

		l--;
		unsigned pos1 = Q[l];
		for (;;)
		{
			child = test_node * 2 + 1;
			if ((child) >= l)
				break;
			if ((child + 1) < l && valuemap[Q[child]] > valuemap[Q[child + 1]])
				child++;
			if (valuemap[pos1] > valuemap[Q[child]])
			{
				//			Q[test_node]=Q[child];
				indexmap[Q[test_node] = Q[child]] = test_node;
				test_node = child;
			}
			else
				break;
		}
		Q[test_node] = pos1;
		indexmap[pos1] = test_node;

		Q.pop_back();
		return pos;
	}
}

void index_priorqueue::insert(unsigned pos, float value)
{
	if (indexmap[pos] == -1)
	{
		unsigned test_node, parent_node;
		Q.push_back(pos);
		test_node = l;
		l++;

		valuemap[pos] = value;

		while (test_node > 0)
		{
			parent_node = (test_node - 1) / 2;
			if (valuemap[Q[parent_node]] > value)
			{
				Q[test_node] = Q[parent_node];
				indexmap[Q[test_node]] = test_node;
				test_node = parent_node;
			}
			else
				break;
		}

		Q[test_node] = pos;
		indexmap[pos] = test_node;
	}
	else
		change(pos, value);

	return;
}

void index_priorqueue::insert(unsigned pos)
{
	if (indexmap[pos] == -1)
	{
		Q.push_back(pos);
		const float value = valuemap[pos];
		unsigned test_node, parent_node;
		test_node = l;

		while (test_node > 0)
		{
			parent_node = (test_node - 1) / 2;
			if (valuemap[Q[parent_node]] > value)
			{
				Q[test_node] = Q[parent_node];
				indexmap[Q[test_node]] = test_node;
				test_node = parent_node;
			}
			else
				break;
		}

		Q[test_node] = pos;
		indexmap[pos] = test_node;

		l++;
	}
	return;
}

void index_priorqueue::remove(unsigned pos)
{
	if (indexmap[pos] >= 0)
	{
		if (l > 1)
		{
			unsigned test_node = indexmap[pos];
			unsigned parent_node, child, pos1;
			pos1 = Q[l - 1];
			float val = valuemap[pos1];

			while (test_node > 0)
			{
				parent_node = (test_node - 1) / 2;
				if (valuemap[Q[parent_node]] > val)
				{
					Q[test_node] = Q[parent_node];
					indexmap[Q[test_node]] = test_node;
					test_node = parent_node;
				}
				else
					break;
			}

			for (;;)
			{
				child = test_node * 2 + 1;
				if ((child + 1) >= l)
					break;
				if ((child + 2) < l && valuemap[Q[child]] > valuemap[Q[child + 1]])
					child++;
				if (val > valuemap[Q[child]])
				{
					//					Q[test_node]=Q[child];
					indexmap[Q[test_node] = Q[child]] = test_node;
					test_node = child;
				}
				else
					break;
			}

			Q[test_node] = pos1;
			indexmap[pos1] = test_node;
			indexmap[pos] = -1;
			l--;
			Q.pop_back();
		}
		else if (l == 1)
		{
			unsigned pos = Q.front();
			indexmap[pos] = -1;
			l--;
			Q.pop_back();
		}
	}
	return;
}

void index_priorqueue::change(unsigned pos, float value)
{
	if (indexmap[pos] >= 0)
	{
		unsigned test_node = indexmap[pos];
		unsigned parent_node, child;
		valuemap[pos] = value;

		while (test_node > 0)
		{
			parent_node = (test_node - 1) / 2;
			if (valuemap[Q[parent_node]] > value)
			{
				Q[test_node] = Q[parent_node];
				indexmap[Q[test_node]] = test_node;
				test_node = parent_node;
			}
			else
				break;
		}

		for (;;)
		{
			child = test_node * 2 + 1;
			if ((child) >= l)
				break;
			if ((child + 1) < l && valuemap[Q[child]] > valuemap[Q[child + 1]])
				child++;
			if (value > valuemap[Q[child]])
			{
				//				Q[test_node]=Q[child];
				indexmap[Q[test_node] = Q[child]] = test_node;
				test_node = child;
			}
			else
				break;
		}

		Q[test_node] = pos;
		indexmap[pos] = test_node;
	}

	return;
}

void index_priorqueue::make_smaller(unsigned pos, float value)
{
	if (indexmap[pos] >= 0)
	{
		unsigned test_node = indexmap[pos];
		unsigned parent_node;
		valuemap[pos] = value;

		while (test_node > 0)
		{
			parent_node = (test_node - 1) / 2;
			if (valuemap[Q[parent_node]] > value)
			{
				Q[test_node] = Q[parent_node];
				indexmap[Q[test_node]] = test_node;
				test_node = parent_node;
			}
			else
				break;
		}

		Q[test_node] = pos;
		indexmap[pos] = test_node;
	}

	return;
}

void index_priorqueue::make_larger(unsigned pos, float value)
{
	if (indexmap[pos] >= 0)
	{
		unsigned test_node = indexmap[pos];
		unsigned child;
		valuemap[pos] = value;

		for (;;)
		{
			child = test_node * 2 + 1;
			if ((child) >= l)
				break;
			if ((child + 1) < l && valuemap[Q[child]] > valuemap[Q[child + 1]])
				child++;
			if (value > valuemap[Q[child]])
			{
				//				Q[test_node]=Q[child];
				indexmap[Q[test_node] = Q[child]] = test_node;
				test_node = child;
			}
			else
				break;
		}

		Q[test_node] = pos;
		indexmap[pos] = test_node;
	}

	return;
}

void index_priorqueue::print_queue()
{
	for (vector<unsigned>::iterator it = Q.begin(); it != Q.end(); it++)
		cout << valuemap[*it] << ", ";
	cout << "." << endl;
}

bool index_priorqueue::empty() { return l == 0; }
bool index_priorqueue::in_queue(unsigned pos) { return indexmap[pos] != -1; }

index_priorqueue::~index_priorqueue()
{
	free(indexmap);
	return;
}

unsigned index_priorqueue::size() { return l; }
