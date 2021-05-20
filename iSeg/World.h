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

//Disable annoying warning no 4786 that nobody understands why appears and MS says that can be ignored
#pragma warning(disable : 4786)

#include "SlicesHandler.h"

#include "Data/Vec3.h"

#include "Core/BranchTree.h"

#include <iostream>
#include <list>
#include <vector>

namespace iseg {

class Node
{
public:
	unsigned m_Offset;
	float m_Intens;
	float m_Cost;
	float m_Fcost;
	bool m_Computed;
	bool m_First;
	Node* m_Prev;
	Node(unsigned offset, float intens);
	Node() = default;
	~Node() = default;
};

class PathElement
{
public:
	unsigned m_Offset;
	bool m_Cross;
	bool m_Seed;
	Node* m_Prev;
	PathElement(unsigned offset);
	PathElement() = default;
	~PathElement() = default;
};

class World
{
	Node* m_Nodes;
	std::list<Node*>
			m_Activelist; //list with the active nodes (the ones that have been "touched" but not expanded yet)
	std::list<Node*> m_Activelistlowint;
	std::vector<std::vector<PathElement>> m_Paths;
	int m_Width;
	int m_Height;
	int m_Length;
	unsigned m_Offx;
	unsigned m_Offy;
	unsigned m_Offz;
	bool m_Solvingarea;
	int m_Times;
	std::vector<int> m_Seedsfailed;
	int m_RootOneintens;
	bool m_Firstseedexpanded;
	int m_Activelistlowintsize;
	//		ml::TVirtualVolume<MLuint16>* vInVol;
	//		ml::TVirtualVolume<MLuint16>* vOutVol;

public:
	World();
	~World();

	Vec3 GetBbStart() { return m_BbStart; };
	Vec3 GetBbEnd() { return m_BbEnd; };
	void SetBbStart(Vec3 bbStart) { m_BbStart = bbStart; };
	void SetBbEnd(Vec3 bbEnd) { m_BbEnd = bbEnd; };
	bool IsValid() const { return m_IsValid; };
	//		void init(vec3 bbStart, vec3 bbEnd, ml::TVirtualVolume<MLuint16>* vInVol, ml::TVirtualVolume<MLuint16>* vOutVol);
	void GetSeedBoundingBox(std::vector<Vec3>* seeds, Vec3& bbStart, Vec3& bbEnd, SlicesHandler* handler3D);
	bool Init(Vec3 bbStart, Vec3 bbEnd, SlicesHandler* handler3D);
	void Clear();
	void Dijkstra(std::vector<Vec3> seeds, Vec3 end, BranchTree* _branchTree);
	void Paint(std::vector<Vec3> seeds, Vec3 cross, unsigned short int numofseeds);
	bool Checkdiameter(unsigned currentoff, unsigned prevoff);
	void Solvelargearea();
	unsigned Solvelargearea2();
	void Changeintens(unsigned o);
	void Reduceactivelist();
	int Whoistheparent(std::vector<Vec3> seeds, int parentintens);
	void OutputBranchTree(BranchItem* branchItem, std::string prefix, FILE*& fp);
	void Storingtree(std::vector<BranchItem*> children, BranchItem* rootOne);
	void Set3dslicehandler(SlicesHandler* handler3D);

private:
	SlicesHandler* m_Handler3D;
	// start of bounding box
	Vec3 m_BbStart;
	// end of bounding box
	Vec3 m_BbEnd;
	// is class valid/initialized
	bool m_IsValid;
};

} // namespace iseg
