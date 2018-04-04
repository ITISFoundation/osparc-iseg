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

//Disable annoying warning no 4786 that nobody understands why appears and MS says that can be ignored
#pragma warning(disable : 4786)

#include "SlicesHandler.h"

#include "Core/BranchTree.h"
#include "Core/Vec3.h"

#include <iostream>
#include <list>
#include <vector>

namespace iseg {

class Node
{
public:
	unsigned offset;
	float intens;
	float cost;
	float fcost;
	bool computed;
	bool first;
	Node* prev;
	Node(unsigned offset, float intens);
	Node() {}
	~Node();
};

class PathElement
{
public:
	unsigned offset;
	bool cross;
	bool seed;
	Node* prev;
	PathElement(unsigned offset);
	PathElement() {}
	~PathElement();
};

class World
{
	Node* nodes;
	std::list<Node*>
		activelist; //list with the active nodes (the ones that have been "touched" but not expanded yet)
	std::list<Node*> activelistlowint;
	std::vector<std::vector<PathElement>> paths;
	int width;
	int height;
	int length;
	unsigned offx;
	unsigned offy;
	unsigned offz;
	bool solvingarea;
	int times;
	std::vector<int> seedsfailed;
	int rootOneintens;
	bool firstseedexpanded;
	int activelistlowintsize;
	//		ml::TVirtualVolume<MLuint16>* vInVol;
	//		ml::TVirtualVolume<MLuint16>* vOutVol;

public:
	World();
	~World();

	Vec3 getBBStart() { return _bbStart; };
	Vec3 getBBEnd() { return _bbEnd; };
	void setBBStart(Vec3 bbStart) { _bbStart = bbStart; };
	void setBBEnd(Vec3 bbEnd) { _bbEnd = bbEnd; };
	bool isValid() { return _isValid; };
	//		void init(vec3 bbStart, vec3 bbEnd, ml::TVirtualVolume<MLuint16>* vInVol, ml::TVirtualVolume<MLuint16>* vOutVol);
	void getSeedBoundingBox(std::vector<Vec3>* seeds, Vec3& bbStart,
							Vec3& bbEnd, SlicesHandler* handler3D);
	bool init(Vec3 bbStart, Vec3 bbEnd, SlicesHandler* handler3D);
	void clear();
	void dijkstra(std::vector<Vec3> seeds, Vec3 end, BranchTree* _branchTree);
	void paint(std::vector<Vec3> seeds, Vec3 cross,
			   unsigned short int numofseeds);
	bool checkdiameter(unsigned currentoff, unsigned prevoff);
	void solvelargearea();
	unsigned solvelargearea2();
	void changeintens(unsigned o);
	void reduceactivelist();
	int whoistheparent(std::vector<Vec3> seeds, int parentintens);
	void outputBranchTree(BranchItem* branchItem, std::string prefix,
						  FILE*& fp);
	void storingtree(std::vector<BranchItem*> children, BranchItem* rootOne);
	void set3dslicehandler(SlicesHandler* handler3D);

private:
	SlicesHandler* _handler3D;
	// start of bounding box
	Vec3 _bbStart;
	// end of bounding box
	Vec3 _bbEnd;
	// is class valid/initialized
	bool _isValid;
};

} // namespace iseg
