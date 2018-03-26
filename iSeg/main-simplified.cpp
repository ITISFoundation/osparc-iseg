/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <vector>
#include "world.h"
#include "vec3.h"

void test()
{
	World _world;
	BranchTree _branchTree;
	BranchItem::init_availablelabels();

	// end point for dijkstra in voxel coordinates
	vec3 end;
	vec3 DISTALPOINT1;
	vec3 DISTALPOINT2;
	
	std::vector<vec3> seeds; // only distal seeds
	std::vector<vec3> allSeeds; // seeds + end point
				
	// save all start seeds in s
	seeds.push_back(DISTALPOINT1);
	seeds.push_back(DISTALPOINT2);
	allSeeds.push_back(DISTALPOINT1);
	allSeeds.push_back(DISTALPOINT2);
	allSeeds.push_back(end);

	SlicesHandler h3ds;
	
	vec3 tmpbbStart;
	vec3 tmpbbEnd;
	// TODO compute bounding box for algorithm
	// otherwise the memory consumption is too large
	_world.setBBStart(tmpbbStart);
	_world.setBBEnd(tmpbbEnd);
	_world.init(tmpbbStart,tmpbbEnd,&h3ds);
	_world.dijkstra(seeds,end, &_branchTree);  
}

