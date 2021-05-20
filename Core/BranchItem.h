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

#include "Data/Point.h"
#include "Data/Vec3.h"

#include <list>
#include <sstream>
#include <vector>

namespace iseg {

//!
class ISEG_CORE_API BranchItem
{
public:
	BranchItem() { Initialize(); }
	BranchItem(BranchItem* parent)
	{
		Initialize();
		m_Parent = parent;
	}

	void Initialize()
	{
		m_Parent = nullptr;
		m_Children.clear();
		m_CenterList.clear();
		m_StartVox[0] = m_StartVox[1] = m_StartVox[2] = 0;
		m_StartVoxWorld[0] = m_StartVoxWorld[1] = m_StartVoxWorld[2] = 0;
		m_EndVox[0] = m_EndVox[1] = m_EndVox[2] = 0;
		m_EndVoxWorld[0] = m_EndVoxWorld[1] = m_EndVoxWorld[2] = 0;
		if (availablelabels.empty())
		{
			for (unsigned i = 60000; i > 0; i--)
				availablelabels.push_back(i);
		}
		m_Label = availablelabels.back();
		availablelabels.pop_back();
	}

	//! Destructor
	~BranchItem()
	{
		if (m_Parent != nullptr)
			m_Parent->RemoveChild(GetLabel());
		m_CenterList.clear();
		for (std::list<BranchItem*>::iterator it = m_Children.begin();
				 it != m_Children.end(); it++)
		{
			(*it)->SetParentDirty(nullptr);
			delete *it;
		}
		m_Children.clear();
		availablelabels.push_back(m_Label);
	} // xxxa delete children?

	BranchItem* GetParent() { return m_Parent; }
	std::list<BranchItem*>* GetChildren() { return &m_Children; }
	void AddChild(BranchItem* child)
	{
		std::list<BranchItem*>::iterator it = m_Children.begin();
		while ((it != m_Children.end()) && ((*it) != child))
			it++;
		if (it == m_Children.end())
			m_Children.push_back(child);
		if (child->GetParent() != this)
			child->SetParent(this);
	}
	void RemoveChild(unsigned label)
	{
		std::list<BranchItem*>::iterator it = m_Children.begin();
		while ((it != m_Children.end()) && ((*it)->GetLabel() != label))
			it++;
		if (it != m_Children.end())
		{
			(*it)->SetParentDirty(nullptr);
			//xxxa delete *it?
			m_Children.erase(it);
		}
	}
	void SetParent(BranchItem* parent)
	{
		if (m_Parent != nullptr)
			m_Parent->RemoveChild(GetLabel());
		m_Parent = parent;
		if (parent != nullptr)
			parent->AddChild(this);
	}

	unsigned GetLabel() const { return m_Label; }
	void SetLabel(unsigned label) { m_Label = label; }

	// Centerlist stuff (centerline of vessel)
	std::vector<Vec3>* GetCenterList() { return &m_CenterList; }
	unsigned GetCenterListSize() { return static_cast<unsigned>(m_CenterList.size()); }
	Vec3 GetCenterPointAt(unsigned index) { return m_CenterList.at(index); }
	void CorrectBranchpoints()
	{
		if (!m_Children.empty())
		{
			*(m_CenterList.rbegin()) = (*m_Children.begin())->GetCenterPointAt(0);
			for (std::list<BranchItem*>::iterator it = m_Children.begin();
					 it != m_Children.end(); it++)
			{
				(*it)->CorrectBranchpoints();
			}
		}
	}

	void AddCenter(Vec3 center) { m_CenterList.push_back(center); }
	void SetStartVox(Vec3 startVox) { m_StartVox = startVox; }
	void SetEndVox(Vec3 endVox) { m_EndVox = endVox; }

	void DougPeuck(float epsilon, float dx, float dy, float dz)
	{
		unsigned int n = (unsigned int)m_CenterList.size();
		if (n > 2)
		{
			std::vector<bool> v1;
			v1.resize(n);

			for (unsigned int i = 0; i < n; i++)
				v1[i] = false;
			v1[0] = true;
			v1[n - 1] = true;
			DougPeuckSub(epsilon, 0, n - 1, &v1, dx, dy, dz);

			std::vector<Vec3>::iterator it = m_CenterList.begin();
			for (unsigned int i = 0; i < n; i++)
			{
				if (!v1[i])
					it = m_CenterList.erase(it);
				else
					it++;
			}
		}
	}
	void DougPeuckInclchildren(float epsilon, float dx, float dy, float dz)
	{
		DougPeuck(epsilon, dx, dy, dz);
		for (std::list<BranchItem*>::iterator it = m_Children.begin();
				 it != m_Children.end(); it++)
		{
			(*it)->DougPeuckInclchildren(epsilon, dx, dy, dz);
		}
	}

	void DougPeuck(float epsilon, float dx, float dy, float dz, std::vector<Vec3>& vp)
	{
		vp.clear();
		unsigned int n = (unsigned int)m_CenterList.size();
		if (n > 2)
		{
			std::vector<bool> v1;
			v1.resize(n);

			for (unsigned int i = 0; i < n; i++)
				v1[i] = false;
			v1[0] = true;
			v1[n - 1] = true;
			DougPeuckSub(epsilon, 0, n - 1, &v1, dx, dy, dz);

			std::vector<Vec3>::iterator it = m_CenterList.begin();
			for (unsigned int i = 0; i < n; i++)
			{
				if (v1[i])
					vp.push_back(*it);
				it++;
			}
		}
		else
		{
			vp = m_CenterList;
		}
	}
	void DougPeuckInclchildren(float epsilon, float dx, float dy, float dz, std::vector<std::vector<Vec3>>& vp)
	{
		std::vector<Vec3> vp2;
		DougPeuck(epsilon, dx, dy, dz, vp2);
		vp.push_back(vp2);
		for (std::list<BranchItem*>::iterator it = m_Children.begin();
				 it != m_Children.end(); it++)
		{
			(*it)->DougPeuckInclchildren(epsilon, dx, dy, dz, vp);
		}
	}
	void GetCenterListSlice(unsigned short slicenr, std::vector<Point>& vp)
	{
		Point p;
		for (std::vector<Vec3>::iterator it = m_CenterList.begin();
				 it != m_CenterList.end(); it++)
		{
			if ((unsigned short)((*it)[2] + 0.0001f) == slicenr)
			{
				p.px = (unsigned short)((*it)[0] + 0.0001f);
				p.py = (unsigned short)((*it)[1] + 0.0001f);
				vp.push_back(p);
			}
		}
	}
	void GetCenterListSliceInclchildren(unsigned short slicenr, std::vector<Point>& vp)
	{
		GetCenterListSlice(slicenr, vp);
		for (std::list<BranchItem*>::iterator it = m_Children.begin();
				 it != m_Children.end(); it++)
		{
			(*it)->GetCenterListSliceInclchildren(slicenr, vp);
		}
	}

protected:
	void SetParentDirty(BranchItem* parent)
	{
		m_Parent = parent;
		if (parent != nullptr)
			parent->AddChild(this);
	}

public:
	static void InitAvailablelabels();

private:
	static std::vector<unsigned> availablelabels;
	// unique label of branch at runtime
	unsigned m_Label;
	// parent branch, root => nullptr
	BranchItem* m_Parent;
	// list of branch's children
	std::list<BranchItem*> m_Children;

	// centerline list in world coordinates
	std::vector<Vec3> m_CenterList;

	Vec3 m_StartVox;			// voxel (in voxel coord) where branch starts
	Vec3 m_StartVoxWorld; // voxel (in world coord) where branch starts
	Vec3
			m_EndVox; // voxel (in voxel coord) where branch ends (end of vessel or begin of other branches)
	Vec3
			m_EndVoxWorld; // voxel (in world coord) where branch ends (end of vessel or begin of other branches)
	void DougPeuckSub(float epsilon, const unsigned int p1, const unsigned int p2, std::vector<bool>* v1_p, float dx, float dy, float dz)
	{
		//	cout << p1<<" "<<p2<<endl;
		if (p2 <= p1 + 1)
			return;
		float dv, l;
		float max_dist = 0;
		Vec3 sij, siv, sih;
		unsigned int max_pos = p1;

		sij[0] = (m_CenterList[p2][0] - m_CenterList[p1][0]) * dx;
		sij[1] = (m_CenterList[p2][1] - m_CenterList[p1][1]) * dy;
		sij[2] = (m_CenterList[p2][2] - m_CenterList[p1][2]) * dz;

		l = sij[0] * sij[0] + sij[1] * sij[1] + sij[2] * sij[2];

		for (unsigned int i = p1 + 1; i < p2; i++)
		{
			siv[0] = (m_CenterList[i][0] - m_CenterList[p1][0]) * dx;
			siv[1] = (m_CenterList[i][1] - m_CenterList[p1][1]) * dy;
			siv[2] = (m_CenterList[i][2] - m_CenterList[p1][2]) * dz;
			sih[0] = siv[1] * sij[2] - siv[2] * sij[1];
			sih[1] = siv[2] * sij[0] - siv[0] * sij[2];
			sih[2] = siv[0] * sij[1] - siv[1] * sij[0];
			dv = (sih[0] * sih[0] + sih[1] * sih[1] + sih[2] * sih[2]) / l;
			if (dv > max_dist)
			{
				max_dist = dv;
				max_pos = i;
			}
		}

		if (max_dist > epsilon * epsilon)
		{
			(*v1_p)[max_pos] = true;
			DougPeuckSub(epsilon, p1, max_pos, v1_p, dx, dy, dz);
			DougPeuckSub(epsilon, max_pos, p2, v1_p, dx, dy, dz);
		}
	}
};

} // namespace iseg
