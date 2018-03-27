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

#include "Point.h"
#include "Vec3.h"

#include <list>
#include <sstream>
#include <vector>

namespace iseg {

//!
class iSegCore_API BranchItem
{
public:
	BranchItem(void) { Initialize(); }
	BranchItem(BranchItem* parent)
	{
		Initialize();
		_parent = parent;
	}

	void Initialize()
	{
		_parent = NULL;
		_children.clear();
		_centerList.clear();
		_startVox[0] = _startVox[1] = _startVox[2] = 0;
		_startVoxWorld[0] = _startVoxWorld[1] = _startVoxWorld[2] = 0;
		_endVox[0] = _endVox[1] = _endVox[2] = 0;
		_endVoxWorld[0] = _endVoxWorld[1] = _endVoxWorld[2] = 0;
		if (availablelabels.empty())
		{
			for (unsigned i = 60000; i > 0; i--)
				availablelabels.push_back(i);
		}
		_label = availablelabels.back();
		availablelabels.pop_back();
	}

	//! Destructor
	~BranchItem(void)
	{
		if (_parent != NULL)
			_parent->removeChild(getLabel());
		_centerList.clear();
		for (std::list<BranchItem*>::iterator it = _children.begin();
			 it != _children.end(); it++)
		{
			(*it)->setParentDirty(NULL);
			delete *it;
		}
		_children.clear();
		availablelabels.push_back(_label);
	} // xxxa delete children?

	BranchItem* getParent() { return _parent; }
	std::list<BranchItem*>* getChildren() { return &_children; }
	void addChild(BranchItem* child)
	{
		std::list<BranchItem*>::iterator it = _children.begin();
		while ((it != _children.end()) && ((*it) != child))
			it++;
		if (it == _children.end())
			_children.push_back(child);
		if (child->getParent() != this)
			child->setParent(this);
	}
	void removeChild(unsigned label)
	{
		std::list<BranchItem*>::iterator it = _children.begin();
		while ((it != _children.end()) && ((*it)->getLabel() != label))
			it++;
		if (it != _children.end())
		{
			(*it)->setParentDirty(NULL);
			//xxxa delete *it?
			_children.erase(it);
		}
	}
	void setParent(BranchItem* parent)
	{
		if (_parent != NULL)
			_parent->removeChild(getLabel());
		_parent = parent;
		if (parent != NULL)
			parent->addChild(this);
	}

	unsigned getLabel() { return _label; }
	void setLabel(unsigned label) { _label = label; }

	// Centerlist stuff (centerline of vessel)
	std::vector<Vec3>* getCenterList() { return &_centerList; }
	unsigned getCenterListSize() { return _centerList.size(); }
	Vec3 getCenterPointAt(unsigned index) { return _centerList.at(index); }
	void correct_branchpoints()
	{
		if (!_children.empty())
		{
			*(_centerList.rbegin()) = (*_children.begin())->getCenterPointAt(0);
			for (std::list<BranchItem*>::iterator it = _children.begin();
				 it != _children.end(); it++)
			{
				(*it)->correct_branchpoints();
			}
		}
	}

	void addCenter(Vec3 center) { _centerList.push_back(center); }
	void setStartVox(Vec3 startVox) { _startVox = startVox; }
	void setEndVox(Vec3 endVox) { _endVox = endVox; }

	void doug_peuck(float epsilon, float dx, float dy, float dz)
	{
		unsigned int n = (unsigned int)_centerList.size();
		if (n > 2)
		{
			std::vector<bool> v1;
			v1.resize(n);

			for (unsigned int i = 0; i < n; i++)
				v1[i] = false;
			v1[0] = true;
			v1[n - 1] = true;
			doug_peuck_sub(epsilon, 0, n - 1, &v1, dx, dy, dz);

			std::vector<Vec3>::iterator it = _centerList.begin();
			for (unsigned int i = 0; i < n; i++)
			{
				if (!v1[i])
					it = _centerList.erase(it);
				else
					it++;
			}
		}
		return;
	}
	void doug_peuck_inclchildren(float epsilon, float dx, float dy, float dz)
	{
		doug_peuck(epsilon, dx, dy, dz);
		for (std::list<BranchItem*>::iterator it = _children.begin();
			 it != _children.end(); it++)
		{
			(*it)->doug_peuck_inclchildren(epsilon, dx, dy, dz);
		}
	}

	void doug_peuck(float epsilon, float dx, float dy, float dz,
					std::vector<Vec3>& vp)
	{
		vp.clear();
		unsigned int n = (unsigned int)_centerList.size();
		if (n > 2)
		{
			std::vector<bool> v1;
			v1.resize(n);

			for (unsigned int i = 0; i < n; i++)
				v1[i] = false;
			v1[0] = true;
			v1[n - 1] = true;
			doug_peuck_sub(epsilon, 0, n - 1, &v1, dx, dy, dz);

			std::vector<Vec3>::iterator it = _centerList.begin();
			for (unsigned int i = 0; i < n; i++)
			{
				if (v1[i])
					vp.push_back(*it);
				it++;
			}
		}
		else
		{
			vp = _centerList;
		}
		return;
	}
	void doug_peuck_inclchildren(float epsilon, float dx, float dy, float dz,
								 std::vector<std::vector<Vec3>>& vp)
	{
		std::vector<Vec3> vp2;
		doug_peuck(epsilon, dx, dy, dz, vp2);
		vp.push_back(vp2);
		for (std::list<BranchItem*>::iterator it = _children.begin();
			 it != _children.end(); it++)
		{
			(*it)->doug_peuck_inclchildren(epsilon, dx, dy, dz, vp);
		}
	}
	void getCenterListSlice(unsigned short slicenr, std::vector<Point>& vp)
	{
		Point p;
		for (std::vector<Vec3>::iterator it = _centerList.begin();
			 it != _centerList.end(); it++)
		{
			if ((unsigned short)((*it)[2] + 0.0001f) == slicenr)
			{
				p.px = (unsigned short)((*it)[0] + 0.0001f);
				p.py = (unsigned short)((*it)[1] + 0.0001f);
				vp.push_back(p);
			}
		}
	}
	void getCenterListSlice_inclchildren(unsigned short slicenr,
										 std::vector<Point>& vp)
	{
		getCenterListSlice(slicenr, vp);
		for (std::list<BranchItem*>::iterator it = _children.begin();
			 it != _children.end(); it++)
		{
			(*it)->getCenterListSlice_inclchildren(slicenr, vp);
		}
	}

protected:
	void setParentDirty(BranchItem* parent)
	{
		_parent = parent;
		if (parent != NULL)
			parent->addChild(this);
	}

public:
	static void init_availablelabels();

private:
	static std::vector<unsigned> availablelabels;
	// unique label of branch at runtime
	unsigned _label;
	// parent branch, root => NULL
	BranchItem* _parent;
	// list of branch's children
	std::list<BranchItem*> _children;

	// centerline list in world coordinates
	std::vector<Vec3> _centerList;

	Vec3 _startVox;		 // voxel (in voxel coord) where branch starts
	Vec3 _startVoxWorld; // voxel (in world coord) where branch starts
	Vec3
		_endVox; // voxel (in voxel coord) where branch ends (end of vessel or begin of other branches)
	Vec3
		_endVoxWorld; // voxel (in world coord) where branch ends (end of vessel or begin of other branches)
	void doug_peuck_sub(float epsilon, const unsigned int p1,
						const unsigned int p2, std::vector<bool>* v1_p,
						float dx, float dy, float dz)
	{
		//	cout << p1<<" "<<p2<<endl;
		if (p2 <= p1 + 1)
			return;
		float dv, l;
		float max_dist = 0;
		Vec3 SIJ, SIV, SIH;
		unsigned int max_pos = p1;

		SIJ[0] = (_centerList[p2][0] - _centerList[p1][0]) * dx;
		SIJ[1] = (_centerList[p2][1] - _centerList[p1][1]) * dy;
		SIJ[2] = (_centerList[p2][2] - _centerList[p1][2]) * dz;

		l = SIJ[0] * SIJ[0] + SIJ[1] * SIJ[1] + SIJ[2] * SIJ[2];

		for (unsigned int i = p1 + 1; i < p2; i++)
		{
			SIV[0] = (_centerList[i][0] - _centerList[p1][0]) * dx;
			SIV[1] = (_centerList[i][1] - _centerList[p1][1]) * dy;
			SIV[2] = (_centerList[i][2] - _centerList[p1][2]) * dz;
			SIH[0] = SIV[1] * SIJ[2] - SIV[2] * SIJ[1];
			SIH[1] = SIV[2] * SIJ[0] - SIV[0] * SIJ[2];
			SIH[2] = SIV[0] * SIJ[1] - SIV[1] * SIJ[0];
			dv = (SIH[0] * SIH[0] + SIH[1] * SIH[1] + SIH[2] * SIH[2]) / l;
			if (dv > max_dist)
			{
				max_dist = dv;
				max_pos = i;
			}
		}

		if (max_dist > epsilon * epsilon)
		{
			(*v1_p)[max_pos] = true;
			doug_peuck_sub(epsilon, p1, max_pos, v1_p, dx, dy, dz);
			doug_peuck_sub(epsilon, max_pos, p2, v1_p, dx, dy, dz);
		}

		return;
	}
};

} // namespace iseg
