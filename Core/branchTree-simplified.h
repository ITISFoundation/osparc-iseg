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

#include "branchItem-simplified.h"

#include <list>

namespace iseg {

//!
class BranchTree
{
public:
	//! Constructor.
	BranchTree(void)
	{
		_branchTree.clear();
		_branchTreeIter = _branchTree.begin();
	};
	//! Destructor.
	~BranchTree(void)
	{
		for (std::list<BranchItem*>::iterator it = _branchTree.begin();
			 it != _branchTree.end(); it++)
		{
			delete *it;
		}
		_branchTree.clear();
	};

	void clear()
	{
		for (std::list<BranchItem*>::iterator it = _branchTree.begin();
			 it != _branchTree.end(); it++)
		{
			delete *it;
		}
		_branchTree.clear();
	}

	unsigned getSize() { return _branchTree.size(); };

	// adds a new branch to _branchTree and returns the created branchItem
	BranchItem* addNewBranch()
	{
		BranchItem* BI = new BranchItem();
		_branchTree.push_back(BI);
		_branchTreeIter = _branchTree.begin();
		return BI;
	};

	void resetIterator() { _branchTreeIter = _branchTree.begin(); };

	// increase _branchTreeIter and return new element
	BranchItem* getItem()
	{
		if (_branchTreeIter == _branchTree.end())
			return NULL;
		else
			return *_branchTreeIter;
	}
	BranchItem* getNextItem()
	{
		if (_branchTreeIter != _branchTree.end())
			_branchTreeIter++;
		if (_branchTreeIter == _branchTree.end())
			return NULL;
		else
			return *_branchTreeIter;
	};

private:
	std::list<BranchItem*>::iterator _branchTreeIter;
	std::list<BranchItem*> _branchTree;
};

} // namespace iseg
