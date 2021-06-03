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

#include "BranchItem.h"

#include <list>

namespace iseg {

//!
class BranchTree
{
public:
	//! Constructor.
	BranchTree()
	{
		m_BranchTree.clear();
		m_BranchTreeIter = m_BranchTree.begin();
	};
	//! Destructor.
	~BranchTree()
	{
		for (std::list<BranchItem*>::iterator it = m_BranchTree.begin();
				 it != m_BranchTree.end(); it++)
		{
			delete *it;
		}
		m_BranchTree.clear();
	};

	void Clear()
	{
		for (std::list<BranchItem*>::iterator it = m_BranchTree.begin();
				 it != m_BranchTree.end(); it++)
		{
			delete *it;
		}
		m_BranchTree.clear();
	}

	unsigned GetSize() { return static_cast<unsigned>(m_BranchTree.size()); };

	// adds a new branch to _branchTree and returns the created branchItem
	BranchItem* AddNewBranch()
	{
		BranchItem* bi = new BranchItem();
		m_BranchTree.push_back(bi);
		m_BranchTreeIter = m_BranchTree.begin();
		return bi;
	};

	void ResetIterator() { m_BranchTreeIter = m_BranchTree.begin(); };

	// increase _branchTreeIter and return new element
	BranchItem* GetItem()
	{
		if (m_BranchTreeIter == m_BranchTree.end())
			return nullptr;
		else
			return *m_BranchTreeIter;
	}
	BranchItem* GetNextItem()
	{
		if (m_BranchTreeIter != m_BranchTree.end())
			m_BranchTreeIter++;
		if (m_BranchTreeIter == m_BranchTree.end())
			return nullptr;
		else
			return *m_BranchTreeIter;
	};

private:
	std::list<BranchItem*>::iterator m_BranchTreeIter;
	std::list<BranchItem*> m_BranchTree;
};

} // namespace iseg
