/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "IndexPriorityQueue.h"

namespace iseg {

IndexPriorityQueue::IndexPriorityQueue(unsigned size2, float* valuemap1)
{
	m_Q.clear();
	m_Indexmap = (int*)malloc(size2 * sizeof(int));
	for (unsigned i = 0; i < size2; i++)
		m_Indexmap[i] = -1;
	m_Valuemap = valuemap1;
	m_L = 0;
	m_Size1 = size2;
}

void IndexPriorityQueue::Clear()
{
	for (size_t i = 0; i < m_Q.size(); i++)
		m_Indexmap[m_Q[i]] = -1;
	m_Q.clear();
	m_L = 0;
}

unsigned IndexPriorityQueue::Pop()
{
	if (m_L <= 1)
	{
		if (m_L == 1)
		{
			unsigned pos = m_Q.front();
			m_Indexmap[pos] = -1;
			m_Q.pop_back();
			m_L = 0;
			return pos;
		}
		else
			return m_Size1;
	}
	else
	{
		unsigned child, test_node;
		test_node = 0;
		unsigned pos = m_Q.front();
		m_Indexmap[pos] = -1;

		m_L--;
		unsigned pos1 = m_Q[m_L];
		for (;;)
		{
			child = test_node * 2 + 1;
			if ((child) >= m_L)
				break;
			if ((child + 1) < m_L && m_Valuemap[m_Q[child]] > m_Valuemap[m_Q[child + 1]])
				child++;
			if (m_Valuemap[pos1] > m_Valuemap[m_Q[child]])
			{
				//			Q[test_node]=Q[child];
				m_Indexmap[m_Q[test_node] = m_Q[child]] = test_node;
				test_node = child;
			}
			else
				break;
		}
		m_Q[test_node] = pos1;
		m_Indexmap[pos1] = test_node;

		m_Q.pop_back();
		return pos;
	}
}

void IndexPriorityQueue::Insert(unsigned pos, float value)
{
	if (m_Indexmap[pos] == -1)
	{
		unsigned test_node, parent_node;
		m_Q.push_back(pos);
		test_node = m_L;
		m_L++;

		m_Valuemap[pos] = value;

		while (test_node > 0)
		{
			parent_node = (test_node - 1) / 2;
			if (m_Valuemap[m_Q[parent_node]] > value)
			{
				m_Q[test_node] = m_Q[parent_node];
				m_Indexmap[m_Q[test_node]] = test_node;
				test_node = parent_node;
			}
			else
				break;
		}

		m_Q[test_node] = pos;
		m_Indexmap[pos] = test_node;
	}
	else
		Change(pos, value);
}

void IndexPriorityQueue::Insert(unsigned pos)
{
	if (m_Indexmap[pos] == -1)
	{
		m_Q.push_back(pos);
		const float value = m_Valuemap[pos];
		unsigned test_node, parent_node;
		test_node = m_L;

		while (test_node > 0)
		{
			parent_node = (test_node - 1) / 2;
			if (m_Valuemap[m_Q[parent_node]] > value)
			{
				m_Q[test_node] = m_Q[parent_node];
				m_Indexmap[m_Q[test_node]] = test_node;
				test_node = parent_node;
			}
			else
				break;
		}

		m_Q[test_node] = pos;
		m_Indexmap[pos] = test_node;

		m_L++;
	}
}

void IndexPriorityQueue::Remove(unsigned pos)
{
	if (m_Indexmap[pos] >= 0)
	{
		if (m_L > 1)
		{
			unsigned test_node = m_Indexmap[pos];
			unsigned parent_node, child, pos1;
			pos1 = m_Q[m_L - 1];
			float val = m_Valuemap[pos1];

			while (test_node > 0)
			{
				parent_node = (test_node - 1) / 2;
				if (m_Valuemap[m_Q[parent_node]] > val)
				{
					m_Q[test_node] = m_Q[parent_node];
					m_Indexmap[m_Q[test_node]] = test_node;
					test_node = parent_node;
				}
				else
					break;
			}

			for (;;)
			{
				child = test_node * 2 + 1;
				if ((child + 1) >= m_L)
					break;
				if ((child + 2) < m_L &&
						m_Valuemap[m_Q[child]] > m_Valuemap[m_Q[child + 1]])
					child++;
				if (val > m_Valuemap[m_Q[child]])
				{
					//					Q[test_node]=Q[child];
					m_Indexmap[m_Q[test_node] = m_Q[child]] = test_node;
					test_node = child;
				}
				else
					break;
			}

			m_Q[test_node] = pos1;
			m_Indexmap[pos1] = test_node;
			m_Indexmap[pos] = -1;
			m_L--;
			m_Q.pop_back();
		}
		else if (m_L == 1)
		{
			unsigned pos = m_Q.front();
			m_Indexmap[pos] = -1;
			m_L--;
			m_Q.pop_back();
		}
	}
}

void IndexPriorityQueue::Change(unsigned pos, float value)
{
	if (m_Indexmap[pos] >= 0)
	{
		unsigned test_node = m_Indexmap[pos];
		unsigned parent_node, child;
		m_Valuemap[pos] = value;

		while (test_node > 0)
		{
			parent_node = (test_node - 1) / 2;
			if (m_Valuemap[m_Q[parent_node]] > value)
			{
				m_Q[test_node] = m_Q[parent_node];
				m_Indexmap[m_Q[test_node]] = test_node;
				test_node = parent_node;
			}
			else
				break;
		}

		for (;;)
		{
			child = test_node * 2 + 1;
			if ((child) >= m_L)
				break;
			if ((child + 1) < m_L && m_Valuemap[m_Q[child]] > m_Valuemap[m_Q[child + 1]])
				child++;
			if (value > m_Valuemap[m_Q[child]])
			{
				//				Q[test_node]=Q[child];
				m_Indexmap[m_Q[test_node] = m_Q[child]] = test_node;
				test_node = child;
			}
			else
				break;
		}

		m_Q[test_node] = pos;
		m_Indexmap[pos] = test_node;
	}
}

void IndexPriorityQueue::MakeSmaller(unsigned pos, float value)
{
	if (m_Indexmap[pos] >= 0)
	{
		unsigned test_node = m_Indexmap[pos];
		unsigned parent_node;
		m_Valuemap[pos] = value;

		while (test_node > 0)
		{
			parent_node = (test_node - 1) / 2;
			if (m_Valuemap[m_Q[parent_node]] > value)
			{
				m_Q[test_node] = m_Q[parent_node];
				m_Indexmap[m_Q[test_node]] = test_node;
				test_node = parent_node;
			}
			else
				break;
		}

		m_Q[test_node] = pos;
		m_Indexmap[pos] = test_node;
	}
}

void IndexPriorityQueue::MakeLarger(unsigned pos, float value)
{
	if (m_Indexmap[pos] >= 0)
	{
		unsigned test_node = m_Indexmap[pos];
		unsigned child;
		m_Valuemap[pos] = value;

		for (;;)
		{
			child = test_node * 2 + 1;
			if ((child) >= m_L)
				break;
			if ((child + 1) < m_L && m_Valuemap[m_Q[child]] > m_Valuemap[m_Q[child + 1]])
				child++;
			if (value > m_Valuemap[m_Q[child]])
			{
				//				Q[test_node]=Q[child];
				m_Indexmap[m_Q[test_node] = m_Q[child]] = test_node;
				test_node = child;
			}
			else
				break;
		}

		m_Q[test_node] = pos;
		m_Indexmap[pos] = test_node;
	}
}

void IndexPriorityQueue::PrintQueue()
{
	for (auto it = m_Q.begin(); it != m_Q.end(); it++)
		std::cout << m_Valuemap[*it] << ", ";
	std::cout << "." << std::endl;
}

bool IndexPriorityQueue::Empty() const { return m_L == 0; }
bool IndexPriorityQueue::InQueue(unsigned pos) { return m_Indexmap[pos] != -1; }

IndexPriorityQueue::~IndexPriorityQueue()
{
	free(m_Indexmap);
}

unsigned IndexPriorityQueue::Size() const { return m_L; }

} // namespace iseg
