/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"
#include "world.h"

#include "Core/branchTree-simplified.h"
#include "Core/branchItem-simplified.h"

#include <math.h>
#include <cmath>

World::World(){
	_isValid = false;
	nodes = NULL;
}
World::~World(){
	clear();
}

//----------------------------------------------------------------------------------
//! Deletes "nodes".
//----------------------------------------------------------------------------------
void World::clear() {
	if(nodes){
		delete[] nodes;
        nodes = NULL;
	}
  	_isValid = false;
}
//----------------------------------------------------------------------------------
//! Initializes volume.
//----------------------------------------------------------------------------------
//void World::init(vec3 bbStart, vec3 bbEnd, ml::TVirtualVolume<MLuint16>* vInVol, ml::TVirtualVolume<MLuint16>* vOutVol){
bool World::init(vec3 bbStart, vec3 bbEnd, SlicesHandler *handler3D){
	

	clear();

//	ml::TimeCounter time2;

	solvingarea=false;
	times=0;
	activelistlowintsize=0;
	firstseedexpanded=false;

	_bbStart = bbStart;
	_bbEnd = bbEnd;
	_handler3D=handler3D;
	
	bbStart[0] = (float)(int)bbStart[0];
	bbStart[1] = (float)(int)bbStart[1];
	bbStart[2] = (float)(int)bbStart[2];

	offx=(unsigned int)bbStart[0];
	offy=(unsigned int)bbStart[1];
	offz=(unsigned int)bbStart[2];

	bbEnd[0] = (float)(int)bbEnd[0];
	bbEnd[1] = (float)(int)bbEnd[1];
	bbEnd[2] = (float)(int)bbEnd[2];

	width= (int)(bbEnd[0]-bbStart[0]+1);
	height= (int)(bbEnd[1]-bbStart[1]+1);
	length= (int)(bbEnd[2]-bbStart[2]+1);

	std::cout << "Width:" << width << " Height:" << height << " Length:" << length << std::endl;

	unsigned total=width*height*length;

	Node* nodes2=NULL;
	try {
		nodes2=new Node[total];
	} catch (std::bad_alloc error) {
		nodes2=NULL;
	}
	
	if (nodes2==NULL){
		std::cerr << "Memory allocation error!" << std::endl;
//		exit(1); // terminate the program
		return false;
    }
	std::cout << "Size of initialized volume: " << total << std::endl;
		
	float px, py, pz;
	float gradient, fgradient; 
	float intens, fintens;
	
	for(unsigned o=0;o<total;o++){
		
		unsigned k = o / (width * height);
		unsigned j = (o - (k * width * height)) / width;
		unsigned i = o - (j * width) - (k * width * height);
				
		//intens
		// ESRA
		// vInVol->getVoxel(x,y,z,0,0,0) reads the voxel intensity
		//intens=(float)(vInVol->getVoxel(ml::Vector(i+bbStart[0], j+bbStart[1], k+bbStart[2], 0, 0, 0)));
		Point p;
		p.px=(short)(i+bbStart[0]);
		p.py=(short)(j+bbStart[1]);
		unsigned short slicenr=(unsigned short)(k+bbStart[2]);
		intens=_handler3D->get_bmp_pt(p,slicenr);

		if(intens>1250 && intens<1300)	fintens=100;
		else if(intens>1150 && intens<=1250)	fintens=10000;
		else if(intens>1000 && intens<=1150)	fintens=1000000;
		else if(intens<=1000)	fintens=10000000000.;
		else{
			intens=1300;
			fintens=0;
		}
			
		
//initialize nodes
		Node nt(o,intens);
		//std::cout << "Node size: " << sizeof(Node) << std::endl;
		nodes2[o]=nt;
	
				//gradient
		if(i>0 && j>0 && k>0){

			unsigned o2=(i-1) + j * width + k * width * height;
			unsigned o3=i + (j-1) * width + k * width * height;
			unsigned o4=i + j * width + (k-1) * width * height;
				
			px=fabs(nodes2[o].intens-nodes2[o2].intens);
			py=fabs(nodes2[o].intens-nodes2[o3].intens);
			pz=fabs(nodes2[o].intens-nodes2[o4].intens);
			gradient=sqrt(px*px+py*py+pz*pz);

			if(gradient>170) fgradient=1000;
					//if(gradient<50) gradient=50;
			else fgradient=((gradient/170)*1000);
		}
		else fgradient=1;	
		nodes2[o].fcost=(fgradient*0.8f+fintens*0.2f);
		
	
	}

	this->nodes=nodes2; 
//	std::cout << "TIME for initialization: " << time2.getRunningTime() << std::endl;

	_isValid = true;
	return true;
}
//----------------------------------------------------------------------------------
//! Modified livewire algorithm.
//----------------------------------------------------------------------------------
void World::dijkstra(std::vector<vec3> seeds, vec3 end, BranchTree* _branchTree){

	if (!_isValid){

     std::cerr << "!_isValid" << std::endl;
     return;
	}

//Declaring seed/end/cross points...

	unsigned short int seedsleft=seeds.size(); 
	unsigned short int n_x=(unsigned short)(seeds[0][0]-offx);
	unsigned short int n_y=(unsigned short)(seeds[0][1]-offy);
	unsigned short int n_z=(unsigned short)(seeds[0][2]-offz);
	
	vec3 endpoint;
		
	endpoint[0]=end[0]-offx;
	endpoint[1]=end[1]-offy;
	endpoint[2]=end[2]-offz;

	unsigned off=n_x + n_y * width + n_z * width * height;
	unsigned short int cross_x;
	unsigned short int cross_y;
	unsigned short int cross_z;

	std::vector<vec3> cross(seeds.size()-1);

//Declaring BranchItems

	// ESRA
	BranchItem* rootOne = _branchTree->addNewBranch();
	rootOne->setParent(NULL);
	std::vector<BranchItem*> children;

//Declaring variables

	bool redfound=false;
	bool first=false;
	bool endt=false;
	bool ends=false;
	bool firstfound=false;
	bool morethanoneseed=true;
	int min;
	int counter=0;
	bool largearea=false;
	int largeareatimes=0;
	typedef std::list<Node*>::iterator li;
	float tmp;
	bool alreadyinlist, alreadyinlistlowint;
	int parentintens;
	int k,j,i;

//Initializing first node to expand (first seed)

	nodes[off].cost=0;
	activelist.push_back(&nodes[off]);
	std::cout << "Expanding seed " << 1 << std::endl;

//----------------------------------------------------------------------------------
//@ Main loop
//----------------------------------------------------------------------------------

	while((activelist.size()!=0 || solvingarea==true) && !endt){

		if(activelist.size()==0){ 
			std::cout << "Activelist empty!!" << std::endl;
			off=solvelargearea2();
			activelist.push_back(&nodes[off]);
		}

	//Reducing activelist if it is too large
		if(activelist.size()>3000 && largearea==false){
			reduceactivelist();
			//std::cout << "Reducing activelist" << std::endl;
		}

	//Skipping seed if it is lasting too much to find the path
		if(counter>150000){
			std::cerr << "Troubles finding the path for seed " << seeds.size()-seedsleft+1 << std::endl;
			seedsfailed.push_back(seeds.size()-seedsleft+1);
			seedsleft=seedsleft-1;
			if(seedsleft>0){
			
				int total=width*height*length;
				int pos=seeds.size()-seedsleft;
				for(int o5=0;o5<total;o5++){
					if(nodes[o5].computed==true){
						nodes[o5].computed=false;
					}
				}
				//std::cout << "Counter: " << counter << std::endl;
				std::cout << "Expanding seed " << pos+1 << std::endl;	
	
				n_x=(unsigned short)(seeds[pos][0]-offx);
				n_y=(unsigned short)(seeds[pos][1]-offy);
				n_z=(unsigned short)(seeds[pos][2]-offz);
		
				unsigned off5=n_x + n_y * width + n_z * width * height;
				std::list<Node*> activelist2;
				activelist=activelist2;
				activelist.push_back(&nodes[off5]);
				
				redfound=false;
				counter=0;
				solvingarea=false;
				largearea=false;
				first=false;
				largeareatimes=0;
				times=0;
			}else{ 
				solvingarea=false;
				firstseedexpanded=false;
				times=0;
				activelist.clear();
				activelistlowint.clear();
				if(firstfound==true) storingtree(children,rootOne);
				paths.clear();
				seedsfailed.clear();
				return;
			}
		}

	//When a path (different from the first one) has been tracked... 

		if(redfound==true){
			
			int pos=seeds.size()-seedsleft;
			//std::cout << "Parentintens: " << parentintens << std::endl;
			int parent=whoistheparent(seeds,parentintens);
			//std::cout << "The parent is: " << parent << std::endl;
	
			if(parent==4000-rootOneintens){
				// ESRA
				BranchItem* child = new BranchItem(rootOne);
				child->setStartVox(seeds[pos-1]);
				child->setEndVox(cross[pos-2]);
				rootOne->addChild(child);
				children.push_back(child);
			}else{
				int prevfailed=0;
				for(size_t s=0;s<seedsfailed.size();s++){
					if(seedsfailed.at(s)<parent+1){ 
						prevfailed++;
						//std::cout << "Prevfailed: " << prevfailed << "Failed seed: " << seedsfailed.at(s) << "Parent: " << parent+1 << std::endl;
					}
				}
				
				// ESRA
				BranchItem* child = new BranchItem(children.at(parent-1-prevfailed));
				//unsigned label= children.at(parent-1-prevfailed)->getLabel();
				//std::cout << "Parentlabel: " << label << std::endl;
				child->setStartVox(seeds[pos-1]);
				child->setEndVox(cross[pos-2]);
				children.at(parent-1-prevfailed)->addChild(child);
				children.push_back(child);
			}
			paint(seeds,cross[pos-2],seedsleft);

		//Set all the nodes to computed=false

			int total=width*height*length;
			for(int o5=0;o5<total;o5++){
				if(nodes[o5].computed==true){
					nodes[o5].computed=false;
					nodes[o5].first=true;
				}
			}
			std::cout << "Counter: " << counter << std::endl;
			std::cout << "Expanding seed " << pos+1 << std::endl;	

			n_x=(unsigned short)(seeds[pos][0]-offx);
			n_y=(unsigned short)(seeds[pos][1]-offy);
			n_z=(unsigned short)(seeds[pos][2]-offz);
		
			unsigned off5=n_x + n_y * width + n_z * width * height;
			std::list<Node*> activelist2;
			activelist=activelist2;
			activelistlowint=activelist2;
			activelist.push_back(&nodes[off5]);
	
			redfound=false;
			counter=0;
			solvingarea=false;
			largearea=false;
			first=false;
			largeareatimes=0;
			times=0;
		}

	//When the first path has been tracked...

		if(ends==true){
			int pos=seeds.size()-seedsleft;
			paint(seeds,endpoint,seedsleft);
			
			rootOne->setStartVox(seeds[pos-1]);
			rootOne->setEndVox(end);

		//Set all the nodes to computed=false
			int total=width*height*length;

			for(int o5=0;o5<total;o5++){
				if(nodes[o5].computed==true){
					nodes[o5].computed=false;
					nodes[o5].first=true;
				}
			}
			std::cout << "Counter: " << counter << std::endl;
			std::cout << "Expanding seed " << pos+1 << std::endl;	

		//Initialize for second seed

			n_x=(unsigned short)(seeds[pos][0]-offx);
			n_y=(unsigned short)(seeds[pos][1]-offy);
			n_z=(unsigned short)(seeds[pos][2]-offz);
	
			unsigned off5=n_x + n_y * width + n_z * width * height;
			std::list<Node*> activelist2;
			activelist=activelist2;
			activelist.push_back(&nodes[off5]);

			ends=false;
			firstfound=true;
			counter=0;
			solvingarea=false;
			largearea=false;
			largeareatimes=0;
			times=0;
			firstseedexpanded=true;
		}

	//Dealing with large structures...

		/*if(solvingarea==true){ 
			if (activelistlowint.size()>0){
			
				off=solvelargearea2();
		
			}
			else{
				//std::cout << "Times:" << times << std::endl;
				solvingarea=false;
				if (times<70){
					off=solvelargearea2();
					times++;
				}
			}
		}*/

		//std::cout << activelist.size() << std::endl;
		
		min=-1;
		li todel;


	//Looking for the node with the mininum cost in activelist

		if(solvingarea==false){
			for(li ii=activelist.begin();ii!=activelist.end();++ii){
						
				Node* e=*ii;
				if(min==-1||min>e->cost){
					min=(int)e->cost;
					off=e->offset;
					todel=ii;
				}
			}
			activelist.erase(todel);
		}
		
		if(nodes[off].computed==true) std::cout << "Node to expand already expanded!!" << std::endl;
		

//Dealing with large structures

//		if (activelist.size()>1 && largeareatimes<1){
//			unsigned o7=nodes[off].prev->offset;
//			if (checkdiameter(off, o7)){ 
//				largearea=true;
//				std::cout << "Large structure found" << std::endl;
//				solvelargearea();
				/*largeareatimes++;
				//if(largeareatimes==20){
					times++;
					if(times==1) solvelargearea();
					off=solvelargearea2();	
					li ia;
					Node* e;
					unsigned o8;
					
					for(ia=activelistlowint.begin();ia!=activelistlowint.end();++ia){
						e=*ia;
						o8=e->offset;
						int z8 = o8 / (width * height);
						int y8 = (o8 - (z8 * width * height)) / width;
						int x8 = o8 - (y8 * width) - (z8 * width * height);
						//std::cout << "NODE: " << "nx:" << x8 << " ny:" << y8 << " nz:" << z8 << std::endl;
						//vOutVol->setVoxel(ml::Vector(x8+offx, y8+offy, z8+offz, 0, 0, 0), 4000);
					}
				}*/
//			}
//		}


	//Expanding the chosen node

		nodes[off].computed=true;
	
		n_z = off / (width * height);
		n_y = (off - (n_z * width * height)) / width;
		n_x = off - (n_y * width) - (n_z * width * height);

  // ----------------------------------------------------------
  //@ The node that is going to be expanded can be painted in the output here.
  // ----------------------------------------------------------
		//vOutVol->setVoxel(ml::Vector(n_x+offx, n_y+offy, n_z+offz, 0, 0, 0), 4000);
		
		counter++; //Counts how many nodes are expanded (iterations of the main loop)
	
		for (k=n_z-1;k<n_z+2;k++){
			for (j=n_y-1;j<n_y+2;j++){
				for (i=n_x-1;i<n_x+2;i++){
						
					unsigned o=i + j * width + k * width * height; //offset of the neighbor
					unsigned o2=n_x + n_y * width + n_z * width * height; //offset of the node that is being expanded
						//std::cout << "Node to expand: " << "x:" << i << " y:" << j << " z:" << k << " o: " << o << " o2: " << o2 << std::endl; 

					if(i>-1 && j>-1 && k>-1 && i<width && j<height && k<length && !nodes[o].computed){

						if(nodes[o].intens>3900 && !endt && !redfound){ //if the node belongs to a previously painted path

							parentintens=(int)nodes[o].intens;
							seedsleft=seedsleft-1;
																					
							if(seedsleft==0){
								endt=true;
								std::cout << "Finished" << std::endl;

							}else{
								redfound=true;
								std::cout << "Red node found (path tracked)" << std::endl;

							}
							cross_z = o / (width * height);
							cross_y = (o - (cross_z * width * height)) / width;
							cross_x = o - (cross_y * width) - (cross_z * width * height);
							int pos=seeds.size()-seedsleft;
							cross[pos-2][2]=cross_z;
							cross[pos-2][1]=cross_y;
							cross[pos-2][0]=cross_x;

									
							std::cout << "Cross node: " << "x:" << cross[pos-2][0]+offx << " y:" << cross[pos-2][1]+offy << " z:" << cross[pos-2][2]+offz << " o:" << o << std::endl; 						
						}
				
				//Dealing with large structures...

						/*if(solvingarea==true){
							if(nodes[o].first==false) changeintens(o);
							else{
								int z1 = o / (width * height);
								int y1 = (o - (z1 * width * height)) / width;
								int x1 = o - (y1 * width) - (z1 * width * height);
								vOutVol->setVoxel(ml::Vector(x1+offx, y1+offy, z1+offz, 0, 0, 0), 4000);
								std::cout << "RootOne area found: " << "x:" << x1 << " y:" << y1 << " z:" << z1 << " Intens:" << nodes[o].intens << " Fcost:" << nodes[o].fcost << " Cost:" << nodes[o].cost << " First:" << nodes[o].first << std::endl; 
								first=true;
								solvingarea=false;
							}
						}*/

				//Tmp=cost of the neighbor + cost of the path from the seed to the current node 

						tmp=nodes[o].fcost+nodes[o2].cost;
				
				//Restarting variables

						alreadyinlist=false;
						alreadyinlistlowint=false;
										
				//First path is found when the endpoint is found 

						if(!firstseedexpanded){
							if (i==endpoint[0] && j==endpoint[1] && k==endpoint[2]){ 
								seedsleft=seedsleft-1;
								if(seedsleft==0){
									endt=true;
									std::cout << "Finished" << std::endl;
									morethanoneseed=false;
								}else{ 
									ends=true;
									std::cout << "First seed expanded (first path tracked)" << std::endl;
								}
							}
						}
				
	

				//Checking if it is in activelistlowint

						li p=activelistlowint.begin();
						if(solvingarea==true){
							while (!alreadyinlistlowint && p!=activelistlowint.end()){
								Node* e=*p;
								if(e->offset==o){
									alreadyinlistlowint=true;
									//activelistlowint.erase(p);
								}
								p++;			
							}
						}

				//Checking if the neighbour is already in activelist	

					li l=activelist.begin();
						while (!alreadyinlist && l!=activelist.end()){
							Node* e=*l;
							if(e->offset==o){
								alreadyinlist=true;
                			}
							l++;
						}

				//If it was in one of the activelists and the new cost is lower, update it

						if(alreadyinlist || alreadyinlistlowint){
							if(nodes[o].cost>tmp+10){ 
								//activelist.erase(activelist.begin()+todel);
									//alreadyinlist=false;
								nodes[o].cost=tmp+10;
								nodes[o].prev=&nodes[o2];
							}
						}

				//Otherwise, save the cost, the path, and put the node in activelist
										
						else{		
							if(nodes[o].intens>1000){
								nodes[o].cost=tmp+10;
								nodes[o].prev=&nodes[o2];
								activelist.push_back(&nodes[o]);
							}						
						}
					}
				}
			}
		}
	}

//Dealing with last path found (painting it, storing it...)
	if(morethanoneseed==true){ //if there were more than one seed
		int pos=seeds.size()-seedsleft;
		int parent=whoistheparent(seeds, parentintens);
		if(parent==4000-rootOneintens){
			// ESRA
			BranchItem* child = new BranchItem(rootOne);
			child->setStartVox(seeds[pos-1]);
			child->setEndVox(cross[pos-2]);
			rootOne->addChild(child);
			children.push_back(child);
		}else{
			int prevfailed=0;
			for(size_t s=0;s<seedsfailed.size();s++){
				if(seedsfailed.at(s)<parent+1){ 
					prevfailed++;
				}
			}		
			// ESRA
			BranchItem* child = new BranchItem(children.at(parent-1-prevfailed));
			child->setStartVox(seeds[pos-1]);
			child->setEndVox(cross[pos-2]);
			children.at(parent-1-prevfailed)->addChild(child);
			children.push_back(child);
		}
		paint(seeds,cross[seeds.size()-2],seedsleft);
	}else {
		paint(seeds,endpoint,seedsleft); //if there was only one seed
	}

//Clearing lists, restarting variables.

	std::cout << "Counter: " << counter << std::endl;
	solvingarea=false;
	firstseedexpanded=false;
	times=0;
	activelist.clear();
	activelistlowint.clear();
	storingtree(children,rootOne);
	paths.clear();
	seedsfailed.clear();
}

//----------------------------------------------------------------------------------
//! Paints path from seed when it is found.
//----------------------------------------------------------------------------------
void World::paint(std::vector<vec3> seeds, vec3 cross, unsigned short int seedsleft){

	bool endp2=false;
	int n_x=(int)(cross[0]);
	int n_y=(int)(cross[1]);
	int n_z=(int)(cross[2]);
	unsigned o2=n_x + n_y * width + n_z * width * height;
	unsigned short int t_x, t_y, t_z;
	unsigned o3;
	int pos=seeds.size()-seedsleft-1;
	
//painting the path

	std::cout << "Painting path " << pos+1 << ": x: " << n_x << " y: " << n_y << " z: " << n_z << std::endl;
	std::cout << "Activelistsize: " << activelist.size() << std::endl;
	std::cout << "Seedsleft: " << seedsleft << std::endl;
	int counter=0;
	std::vector<PathElement> newpath;

	while(!endp2){
	
		if(n_x==seeds[pos][0]-offx && n_y==seeds[pos][1]-offy && n_z==seeds[pos][2]-offz){
			
			endp2=true;
			nodes[o2].intens=(float)(4000-pos);

			PathElement pathelement(o2);
			newpath.push_back(pathelement);
					
		}else{
			t_x=n_x;
			t_y=n_y;
			t_z=n_z;
			o3=t_x + t_y * width + t_z * width * height;

			PathElement pathelement(o3);
			newpath.push_back(pathelement);
					
			// ESRA -> setVoxel writes voxel value to output image
			Point p;
			p.px=(short)(n_x+offx);
			p.py=(short)(n_y+offy);
			unsigned short slicenr=(unsigned short)n_z+offz;
			//_handler3D->set_work_pt(p,slicenr,4000-pos);xxxa
			_handler3D->set_work_pt(p,slicenr,4000);
			
			//std::cout << "Path node: " << "x:" << n_x << " y:" << n_y << " z:" << n_z << " Offset:" << o3 << " Intens:" << nodes[o3].intens << " Fcost:" << nodes[o3].fcost << " Cost:" << nodes[o3].cost << " First:" << nodes[o3].first << std::endl; 
			nodes[o3].intens=(float)(4000-pos);
			if(!firstseedexpanded) rootOneintens=4000-pos;
			o2=nodes[o3].prev->offset;
			counter++;
			
			n_z = o2 / (width * height);
			n_y = (o2 - (n_z * width * height)) / width;
			n_x = o2 - (n_y * width) - (n_z * width * height);
		}
	}
	//if(firstseedexpanded) newpath[0].cross=true;
	for(size_t i=0;i<paths.size();i++){
		for(size_t j=0;j<paths[i].size();j++){

			if(paths[i][j].offset==newpath.at(0).offset){
				paths[i][j].cross=true; 
			}
		}
	}
	newpath[counter].seed=true;
	paths.push_back(newpath);
	for(size_t k=0;k<newpath.size();k++){
	
		//std::cout << "Path node: " << "o:" << newpath[k].offset << " cross:" << newpath[k].cross << std::endl; 

	}
	//std::cout << "Path long: " << counter << std::endl;
}
//----------------------------------------------------------------------------------
//! Stores the tree information in BranchTree.
// (When the execution of the main algorithm finishes, there is only one BranchItem for each tracked path;
//  from that information this function creates the right BranchTree.)
//----------------------------------------------------------------------------------
void World::storingtree(std::vector<BranchItem*> children, BranchItem* rootOne){

	//Matrix with the cross points of each path

	std::vector<std::vector<unsigned> > allcrosses;

	for(size_t i=0;i<paths.size();i++){
		std::vector<unsigned> crosses;
		for(size_t j=0;j<paths[i].size();j++){

			if(paths[i][j].cross){
				
				unsigned o=paths[i][j].offset;
				crosses.push_back(o);

			}
		}
		allcrosses.push_back(crosses);
	}

	int i=0;
	bool dup=false;
	int pathcrosses=0;
	// ESRA
	std::vector<std::vector<BranchItem*> > stretches;
	std::vector<BranchItem*> pathstretches;
	stretches.push_back(pathstretches);
		
	for(size_t j=0;j<paths[i].size();j++){

		if(paths[i][j].cross){

			pathcrosses++;
			unsigned o=paths[i][j].offset;
			vec3 cross;
			int crossz = o / (width * height);
			int crossy = (o - (crossz * width * height)) / width;
			int crossx = o - (crossy * width) - (crossz * width * height);
			cross[2]=(float)crossz;
			cross[1]=(float)crossy;
			cross[0]=(float)crossx;	
				
			if(!dup){
				
				//std::cout << "First duplicate branch for path 1: " << pathcrosses << std::endl;

				// ESRA
				BranchItem* dupchild = new BranchItem(rootOne);
				//unsigned label=dupchild->getLabel();
				//std::cout << "Newbranchlabel: " << label << std::endl;
				rootOne->setStartVox(cross);
				dupchild->setEndVox(cross);

				if(pathcrosses<(int)allcrosses[i].size()){
				
					unsigned off=allcrosses[i][pathcrosses];
					int nextcrossz = off / (width * height);
					int nextcrossy = (off - (nextcrossz * width * height)) / width;
					int nextcrossx = off - (nextcrossy * width) - (nextcrossz * width * height);
					vec3 nextcross;
					nextcross[2]=(float)nextcrossz;
					nextcross[1]=(float)nextcrossy;
					nextcross[0]=(float)nextcrossx;
					dupchild->setStartVox(nextcross);

				}else{

					unsigned o=paths[i][j].offset;
					
					int seedz = o / (width * height);
					int seedy = (o - (seedz * width * height)) / width;
					int seedx = o - (seedy * width) - (seedz * width * height);
					vec3 seed;
					seed[2]=(float)seedz;
					seed[1]=(float)seedy;
					seed[0]=(float)seedx;
					dupchild->setStartVox(seed);

				}
				// ESRA
				rootOne->addChild(dupchild);
				rootOne->addCenter(cross);		//<--HERE
				stretches[i].push_back(dupchild);
				dup=true;

			}else{

				//std::cout << "Duplicate branch for path 1: " << pathcrosses << std::endl;

				BranchItem* dupchild = new BranchItem(stretches[i].at(pathcrosses-2));
				//unsigned label=dupchild->getLabel();
				//std::cout << "Newbranchlabel: " << label << std::endl;
				stretches[i].at(pathcrosses-2)->setStartVox(cross);

				for(size_t k=1;k<paths.size();k++){
					
					unsigned off=paths[k][0].offset;
					vec3 first;
					int firstz = off / (width * height);
					int firsty = (off - (firstz * width * height)) / width;
					int firstx = off - (firsty * width) - (firstz * width * height);					
					first[2]=(float)firstz;
					first[1]=(float)firsty;
					first[0]=(float)firstx;
					
					if(first==cross){						

						// ESRA
						unsigned childlabel=children[k-1]->getLabel();
						BranchItem* prevparent=children[k-1]->getParent();
						unsigned prevparentlabel=prevparent->getLabel();
	
						if(rootOne->getLabel()==prevparentlabel){
							rootOne->removeChild(childlabel);
							//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
						}
						for(size_t j=0;j<children.size();j++){
							//std::cout << "Child " << j << " Label " << children[j]->getLabel() << std::endl;
							if(children[j]->getLabel()==prevparentlabel){
								children[j]->removeChild(childlabel);
								//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
							}
						}
						for(size_t p=0;p<stretches.size();p++){
							for(size_t j=0;j<stretches[p].size();j++){
							
								if(stretches[p][j]->getLabel()==prevparentlabel){
									stretches[p][j]->removeChild(childlabel);
									//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
								}
							}
						}
									
						stretches[i].at(pathcrosses-2)->addChild(children[k-1]);
						//std::cout << "Newparent: " << stretches[i].at(pathcrosses-2)->getLabel() << std::endl;
						children[k-1]->setParent(stretches[i].at(pathcrosses-2));
					}
				}

				dupchild->setEndVox(cross);

				if(pathcrosses<(int)allcrosses[i].size()){ 
					
					unsigned off=allcrosses[i][pathcrosses];
					int nextcrossz = off / (width * height);
					int nextcrossy = (off - (nextcrossz * width * height)) / width;
					int nextcrossx = off - (nextcrossy * width) - (nextcrossz * width * height);
					vec3 nextcross;
					nextcross[2]=(float)nextcrossz;
					nextcross[1]=(float)nextcrossy;
					nextcross[0]=(float)nextcrossx;
					dupchild->setStartVox(nextcross);
				
				}else{
				
					unsigned o=paths[i][j].offset;
					int seedz = o / (width * height);
					int seedy = (o - (seedz * width * height)) / width;
					int seedx = o - (seedy * width) - (seedz * width * height);
					vec3 seed;
					seed[2]=(float)seedz;
					seed[1]=(float)seedy;
					seed[0]=(float)seedx;
					dupchild->setStartVox(seed);

				}
				stretches[i].at(pathcrosses-2)->addChild(dupchild);
				stretches[i][pathcrosses-2]->addCenter(cross);		//<--HERE
				stretches[i].push_back(dupchild);
			}
		}	
		
		vec3 node;
		unsigned off=paths[i][j].offset;
		int nodez = off / (width * height);
		int nodey = (off - (nodez * width * height)) / width;
		int nodex = off - (nodey * width) - (nodez * width * height);
		nodez=nodez+offz;
		nodey=nodey+offy;
		nodex=nodex+offx;
		node[0]=(float)nodex;
		node[1]=(float)nodey;
		node[2]=(float)nodez;
		// ESRA
		Point p;
		p.px=(short)(nodex);
		p.py=(short)(nodey);
		unsigned short slicenr=(unsigned short)nodez;
		_handler3D->set_work_pt(p,slicenr,4000);
			
		if(dup){
			stretches[i][pathcrosses-1]->addCenter(node);	//	 				
		}else{
			rootOne->addCenter(node);	//	
		}
	}
	
	for(size_t i=1;i<paths.size();i++){

		// ESRA
		std::vector<BranchItem*> pathstretches;
		stretches.push_back(pathstretches);
		pathcrosses=0;
		dup=false;

		for(size_t j=0;j<paths[i].size();j++){

			if(paths[i][j].cross==true){

				pathcrosses++;
				unsigned o=paths[i][j].offset;
				vec3 cross;
				int crossz = o / (width * height);
				int crossy = (o - (crossz * width * height)) / width;
				int crossx = o - (crossy * width) - (crossz * width * height);
				cross[2]=(float)crossz;
				cross[1]=(float)crossy;
				cross[0]=(float)crossx;
				//std::cout << "Cross " << " x: " << crossx << " y: " << crossy << " z: " << crossz << std::endl;
				
				if(!dup){

					// ESRA
					BranchItem* dupchild = new BranchItem(children.at(i-1));
					//unsigned label=dupchild->getLabel();
					//std::cout << "Newbranchlabel: " << label << std::endl;
					children.at(i-1)->setStartVox(cross);
					dupchild->setEndVox(cross);
					if(pathcrosses<(int)allcrosses[i].size()){
						
						unsigned off=allcrosses[i][pathcrosses];
						int nextcrossz = off / (width * height);
						int nextcrossy = (off - (nextcrossz * width * height)) / width;
						int nextcrossx = off - (nextcrossy * width) - (nextcrossz * width * height);
						vec3 nextcross;
						nextcross[2]=(float)nextcrossz;
						nextcross[1]=(float)nextcrossy;
						nextcross[0]=(float)nextcrossx;
						dupchild->setStartVox(nextcross);
					
					}else{

						unsigned o=paths[i][j].offset;
						int seedz = o / (width * height);
						int seedy = (o - (seedz * width * height)) / width;
						int seedx = o - (seedy * width) - (seedz * width * height);
						vec3 seed;
						seed[2]=(float)seedz;
						seed[1]=(float)seedy;
						seed[0]=(float)seedx;
						children.at(i)->setStartVox(seed);
					
					}
					
					// ESRA
					children.at(i-1)->addChild(dupchild);
					children[i-1]->addCenter(cross);		//<--HERE
					stretches[i].push_back(dupchild);
					dup=true;
									
				}else{
					// ESRA
					BranchItem* dupchild = new BranchItem(stretches[i].at(pathcrosses-2));
					//unsigned label=dupchild->getLabel();
					//std::cout << "Newbranchlabel: " << label << std::endl;
					stretches[i].at(pathcrosses-2)->setStartVox(cross);
					for(size_t k=1;k<paths.size();k++){
					
						unsigned off=paths[k][0].offset;
						vec3 first;
						int firstz = off / (width * height);
						int firsty = (off - (firstz * width * height)) / width;
						int firstx = off - (firsty * width) - (crossz * width * height);
						first[2]=(float)firstz;
						first[1]=(float)firsty;
						first[0]=(float)firstx;
						if(first==cross)
						{
							unsigned childlabel=children[k-1]->getLabel();
							BranchItem* prevparent=children[k-1]->getParent();
							unsigned prevparentlabel=prevparent->getLabel();

							if(rootOne->getLabel()==prevparentlabel){
								rootOne->removeChild(childlabel);
								//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
							}
							for(size_t j=0;j<children.size();j++){
							
								if(children[j]->getLabel()==prevparentlabel){
									children[j]->removeChild(childlabel);
									//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
								}
							}
							for(size_t p=0;p<stretches.size();p++){
								for(size_t j=0;j<stretches[p].size();j++){
								
									if(stretches[p][j]->getLabel()==prevparentlabel){
										stretches[p][j]->removeChild(childlabel);
										//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
									}
								}
							}
									
							stretches[i].at(pathcrosses-2)->addChild(children[k-1]);
							//std::cout << "Newparent: " << stretches[i].at(pathcrosses-2)->getLabel() << std::endl;
							children[k-1]->setParent(stretches[i].at(pathcrosses-2));
						}

					}
	
					dupchild->setEndVox(cross);

					if(pathcrosses<(int)allcrosses[i].size()){ 

						unsigned off=allcrosses[i][pathcrosses];
						int nextcrossz = off / (width * height);
						int nextcrossy = (off - (nextcrossz * width * height)) / width;
						int nextcrossx = off - (nextcrossy * width) - (nextcrossz * width * height);
						vec3 nextcross;
						nextcross[2]=(float)nextcrossz;
						nextcross[1]=(float)nextcrossy;
						nextcross[0]=(float)nextcrossx;
						dupchild->setStartVox(nextcross);

					}else{
						unsigned o=paths[i][j].offset;
						int seedz = o / (width * height);
						int seedy = (o - (seedz * width * height)) / width;
						int seedx = o - (seedy * width) - (seedz * width * height);
						vec3 seed;
						seed[2]=(float)seedz;
						seed[1]=(float)seedy;
						seed[0]=(float)seedx;
						dupchild->setStartVox(seed);
					}
					stretches[i].at(pathcrosses-2)->addChild(dupchild);
					stretches[i][pathcrosses-2]->addCenter(cross);		//<--HERE
					stretches[i].push_back(dupchild);
				}
			}
	
			vec3 node;
			unsigned off=paths[i][j].offset;
			int nodez = off / (width * height);
			int nodey = (off - (nodez * width * height)) / width;
			int nodex = off - (nodey * width) - (nodez * width * height);
			nodez=nodez+offz;
			nodey=nodey+offy;
			nodex=nodex+offx;
			node[0]=(float)nodex;
			node[1]=(float)nodey;
			node[2]=(float)nodez;
			//vOutVol->setVoxel(ml::Vector(nodex, nodey, nodez, 0, 0, 0), 4000);
						
			if(dup){ 
				stretches[i][pathcrosses-1]->addCenter(node);		//
			}else{
				children[i-1]->addCenter(node);		//
			}
		}
	}

	rootOne->correct_branchpoints();
}

//----------------------------------------------------------------------------------
//! Says who is the parent branch (where is located its startpoint in the vector "seeds")
//----------------------------------------------------------------------------------
int World::whoistheparent(std::vector<vec3> seeds, int parentintens){
	
	int parent=-1;

	//std::cout << "Parentintens: " << parentintens << std::endl;

	for(size_t i=0;i<seeds.size();i++){
		
		if(parentintens==(4000-i)){
			parent=i;
			//std::cout << "Parent: " << i << std::endl;
			return(parent);
		}
	}
	return(parent);
}
//----------------------------------------------------------------------------------
//! Reduce size of activelist by removing every second node
//----------------------------------------------------------------------------------
void World::reduceactivelist(){

	typedef std::list<Node*>::iterator li;
	std::list<Node*> activelistempty;
	Node* e;
	unsigned off;

	int par=0;
	for(li ia=activelist.begin();ia!=activelist.end();++ia){
		e=*ia;
		off=e->offset;
		if(!(par%2)){
			if(nodes[off].intens<3900){
				nodes[off].computed=true;
				//int z = off / (width * height);
				//int y = (off - (z * width * height)) / width;
				//int x = off - (y * width) - (z * width * height);
				//vOutVol->setVoxel(ml::Vector(x+offx, y+offy, z+offz, 0, 0, 0), 4000);
			}
		}else{
			activelistempty.push_back(&nodes[off]);
		}
		par++;
	}
	activelist=activelistempty;
}
//----------------------------------------------------------------------------------
//! Reassigns costs to intensities
//----------------------------------------------------------------------------------
void World::changeintens(unsigned o){

	//int z = o / (width * height);
	//int y = (o - (z * width * height)) / width;
	//int x = o - (y * width) - (z * width * height);
	if(nodes[o].intens>1000 && nodes[o].intens<=1150){
			
		nodes[o].fcost=10;
		//if(nodes[o].computed==true) std::cout << "Node already computed!!" << std::endl;
		//std::cout << "NODE: " << "x:" << x << " y:" << y << " z:" << z << " Computed: " << nodes[off].computed << std::endl;
	
	}
	else if(nodes[o].intens>1150 && nodes[o].intens<=1230){
		if(nodes[o].cost==0 && nodes[o].first==false) nodes[o].fcost=nodes[o].fcost+nodes[o].fcost*10;

	}
	else{
		if(nodes[o].cost==0 && nodes[o].first==false) nodes[o].fcost=100000;
		//nodes[o].cost=nodes[o].cost+nodes[o].fcost;
	}

}

//----------------------------------------------------------------------------------
//! Checks the diameter of the structure at each step
//----------------------------------------------------------------------------------
bool World::checkdiameter(unsigned coff, unsigned poff){

	bool largearea=false;

	int z = coff / (width * height);
	int y = (coff - (z * width * height)) / width;
	int x = coff - (y * width) - (z * width * height);
			
	int pz = poff / (width * height);
	int py = (poff - (pz * width * height)) / width;
	int px = poff - (py * width) - (pz * width * height);

	bool comput=false;

	int v1x=x-px;
	int v1y=y-py;
	int v1z=z-pz;
	int v2x=0;
	int v2y=0;
	int v2z=0;

	if(v1z==0){
		v2z=0;
		if(v1y==0){v2x=0; v2y=1;}
		else if(v1x==0) {v2x=1; v2y=0;}
		else if(v1x==v1y) {v2x=v1x; v2y=-v1y;}
		else{v2x=v1y; v2y=v1y;}
	}else if(v1x==0 && v1y==0){
		v2x=0;
		v2y=1;
		v2z=0;
	}else if(v1x==0){
		v2x=1;
		v2y=0;
		v2z=0;
	}else if(v1y==0){
		v2x=0;
		v2y=1;
		v2z=0;
	}else if(v1x!=0 && v1y!=0){
		v2z=0;
		if(v1x==v1y) {v2x=v1x; v2y=-v1y;}
		else{v2x=v1y; v2y=v1y;}
	}else std::cout << "Unconsidered case!" << std::endl;

	int nx=x;
	int ny=y;
	int nz=z;

	int diameter1=0;
	int diameter2=0;
	int diameter3=0;
	int diameter4=0;

	do{
		unsigned o=nx + ny * width + nz * width * height;
		if(nx>-1 && ny>-1 && nz>-1 && nx<width && ny<height && nz<length){
			//std::cout << "Diameter node: " << "x:" << nx << " y:" << ny << " z:" << nz << " o: " << o << std::endl; 
			if(nodes[o].intens>1150){
				diameter1++;
				comput=true;
				nx=nx+v2x;
				ny=ny+v2y;
				nz=nz+v2z;
			}
			else{
				comput=false;
			}
		}
		else comput=false;	
	}while(comput);

	nx=x;
	ny=y;
	nz=z;

	do{
		unsigned o=nx + ny * width + nz * width * height;
		if(nx>-1 && ny>-1 && nz>-1 && nx<width && ny<height && nz<length){
			//std::cout << "Diameter node: " << "x:" << nx << " y:" << ny << " z:" << nz << " o: " << o << std::endl; 
			if(nodes[o].intens>1150){
				diameter2++;
				comput=true;
				nx=nx-v2x;
				ny=ny-v2y;
				nz=nz-v2z;
			}
			else{
				comput=false;
			}
		}
		else comput=false;	
	}while(comput);

	if(diameter1>15 && diameter2>15){

		int nxf=nx;
		int nyf=ny;
		int nzf=nz;
	
		//std::cout << "Diameter: " << diameter << " Comput: " << comput << std::endl;
		//std::cout << "Diameter node: " << "x:" << x << " y:" << y << " z:" << z << std::endl; 
		//std::cout << "Current node: " << "x:" << x << " y:" << y << " z:" << z << " o: " << coff << std::endl; 
		//std::cout << "Previous node: " << "x:" << px << " y:" << py << " z:" << pz << " o: " << poff << std::endl; 
		//std::cout << "Difference: " << "x:" << cx << " y:" << cy << " z:" << cz << std::endl; 

		int nv2x=v1x;
		int nv2y=v1y;
		int nv2z=v1z;
		
		nx=x;
		ny=y;
		nz=z;

		do{
			unsigned o2=nx + ny * width + nz * width * height;
			if(nx>-1 && ny>-1 && nz>-1 && nx<width && ny<height && nz<length){
				//std::cout << "Diameter node: " << "x:" << nx << " y:" << ny << " z:" << nz << " o: " << o << std::endl; 
				if(nodes[o2].intens>1150){
					diameter3++;
					comput=true;
					nx=nx+nv2x;
					ny=ny+nv2y;
					nz=nz+nv2z;
				}
				else{
					comput=false;
				}
			}
			else comput=false;	
		}while(comput);

		nx=x;
		ny=y;
		nz=z;

		do{
			unsigned o2=nx + ny * width + nz * width * height;
			if(nx>-1 && ny>-1 && nz>-1 && nx<width && ny<height && nz<length ){
				//std::cout << "Diameter node: " << "x:" << nx << " y:" << ny << " z:" << nz << " o: " << o << std::endl; 
				if(nodes[o2].intens>1150){
					diameter4++;
					comput=true;
					nx=nx-nv2x;
					ny=ny-nv2y;
					nz=nz-nv2z;
				}
				else{
					comput=false;
				}
			}
			else comput=false;	
		}while(comput);

		if(diameter3>15 && diameter4>15){
	
			//std::cout << "Diameter: " << diameter << " Comput: " << comput << std::endl;
			//std::cout << "Diameter 2 node: " << "x:" << nx << " y:" << ny << " z:" << nz << std::endl; 
			//std::cout << "Current node: " << "x:" << x << " y:" << y << " z:" << z << " o: " << coff << std::endl; 
			//std::cout << "Previous node: " << "x:" << px << " y:" << py << " z:" << pz << " o: " << poff << std::endl; 
			//std::cout << "Difference: " << "x:" << cx << " y:" << cy << " z:" << cz << std::endl;
			//std::cout << "New difference: " << "x:" << ncx << " y:" << ncy << " z:" << ncz << std::endl;
			//std::cout << "LARGE AREA FOUND" << std::endl;
			//std::cout << "1: " << diameter1 << " 2: " << diameter2 << " 3: " << diameter3 << " 4: " << diameter4 << std::endl;
			largearea=true;
			//zarea=nz;

			//if(times<1){

				for (int i=0; i<diameter1+diameter2; i++){
					Point p;
					p.px=(short)(nxf+i*v2x+offx);
					p.py=(short)(nyf+i*v2y+offy);
					unsigned short slicenr=(unsigned short)nzf+i*v2z+offz;
					_handler3D->set_work_pt(p,slicenr,4000);
					//if(nzf+offz==75) std::cout << "v2: " << "x:" << v2x << " y:" << v2y << " z:" << v2z << std::endl;
				}
		
				for (int j=0; j<diameter3+diameter4; j++){
					Point p;
					p.px=(short)(nx+j*nv2x+offx);
					p.py=(short)(ny+j*nv2y+offy);
					unsigned short slicenr=(unsigned short)nz+j*nv2z+offz;
					_handler3D->set_work_pt(p,slicenr,4000);
					//if(nz+offz==75) std::cout << "nv2: " << "x:" << nv2x << " ny:" << nv2y << " nz:" << nv2z << std::endl;
				}
			//}
		}
	}
	return(largearea);
}	
//----------------------------------------------------------------------------------
//! Deals when the situation when a large structure is found
//	Returns the offset of the node that is going to be expanded (a new node selected trying to skip the large structure)
//----------------------------------------------------------------------------------
unsigned World::solvelargearea2(){
	
	typedef std::list<Node*>::iterator li;
	unsigned newoff;
	unsigned off;
	Node* e;
	li ie, todel;
	int min=-1;

	if(solvingarea==false){

		activelistlowint.clear();
		for(ie=activelist.begin();ie!=activelist.end();++ie){

			e=*ie;
			off=e->offset;
			if(e->intens>1000 && e->intens<=1150){
				
				nodes[off].cost=nodes[off].cost-nodes[off].fcost+10;
				nodes[off].fcost=10;
				if(nodes[off].computed==true) std::cout << "Node already computed!!" << std::endl;
				activelistlowint.push_back(&nodes[off]);
			}else{
				//todel=iu;
				//nodes[off].cost=nodes[off].cost+10000000;
				if(nodes[off].first==false){
					nodes[off].computed=true;
					//vOutVol->setVoxel(ml::Vector(x+offx, y+offy, z+offz, 0, 0, 0), 4000);
				}
			}
		}
		solvingarea=true;
		activelist.clear();
		
		//std::cout << "Activelistsizelowint: " << activelistlowint.size() << std::endl;

		/*for(li ia=activelistlowint.begin();ia!=activelistlowint.end();++ia){
			e=*ia;
			unsigned o8=e->offset;
			int z8 = o8 / (width * height);
			int y8 = (o8 - (z8 * width * height)) / width;
			int x8 = o8 - (y8 * width) - (z8 * width * height);
			//std::cout << "NODE: " << "nx:" << x8 << " ny:" << y8 << " nz:" << z8 << " Computed: " << nodes[o8].computed << std::endl;
			vOutVol->setVoxel(ml::Vector(x8+offx, y8+offy, z8+offz, 0, 0, 0), 4000);
		}*/
		
		/*if(activelistlowint.size()>1500){
			activelist.clear();
			activelist=activelistlowint;
			activelistlowint.clear();
			//std::cout << "Reducing activelistlowint" << std::endl;
			int par=0;
			for(li ia=activelist.begin();ia!=activelist.end();++ia){
				e=*ia;
				off=e->offset;
				if(!(par%2)){
					if(nodes[off].first==false){
						nodes[off].computed=true;
						//vOutVol->setVoxel(ml::Vector(x+offx, y+offy, z+offz, 0, 0, 0), 4000);
					}
				}else{
					activelistlowint.push_back(&nodes[off]);
				}
				par++;
			}
		}*/
		//std::cout << "Activelistsizelowint: " << activelistlowint.size() << std::endl;
		activelist.clear();
		activelistlowintsize=activelistlowint.size();

		for(li ii=activelistlowint.begin();ii!=activelistlowint.end();++ii){
						
			Node* e=*ii;
			if(min==-1||min>e->cost){
				min=(int)e->cost;
				newoff=e->offset;
				todel=ii;
			}
		}

		activelistlowint.erase(todel);
		activelist=activelistlowint;
		return(newoff);
	
	}else{
	
		//std::cout << "Activelistlowint size: " << activelistlowint.size() << std::endl;
		if(activelistlowint.size()==0){ 
			std::cout << "Activelistlowint empty" << std::endl;
		}
					
		for(li ii=activelistlowint.begin();ii!=activelistlowint.end();++ii){
						
			e=*ii;
			//unsigned o8=e->offset;
			//int z8 = o8 / (width * height);
			//int y8 = (o8 - (z8 * width * height)) / width;
			//int x8 = o8 - (y8 * width) - (z8 * width * height);
			//std::cout << "NODE: " << "nx:" << x8 << " ny:" << y8 << " nz:" << z8 << " Computed: " << nodes[o8].computed << std::endl;
			//vOutVol->setVoxel(ml::Vector(x8+offx, y8+offy, z8+offz, 0, 0, 0), 4000);

			if(min==-1||min>e->cost){
				min=(int)e->cost;
				newoff=e->offset;
				todel=ii;
			}
		}

		activelistlowint.erase(todel);
		//std::cout << "Activelistsizelowint: " << activelistlowint.size() << std::endl;

		bool found=false;
		li p=activelist.begin();
		while (!found && p!=activelist.end()){
			Node* e=*p;
			if(e->offset==newoff){
				found=true;
				todel=p;
			}
			p++;			
		}
		activelist.erase(todel);

		//std::cout << "Newoff: " << newoff << std::endl;
		//std::cout << "Largeareatimes: " << largeareatimes << std::endl;
		return(newoff);
	}
}
//----------------------------------------------------------------------------------
//! Diminish cost of dark nodes previously found when a large structure is found
//----------------------------------------------------------------------------------
void World::solvelargearea(){

	int total=width*height*length;
	std::cout << "Solvelargearea1" << std::endl;

	for(int o=0;o<total;o++){
		
		if(nodes[o].computed==true){
			if(nodes[o].intens>1000 && nodes[o].intens<1150){
				nodes[o].cost=nodes[o].cost-nodes[o].fcost;
				nodes[o].fcost=20;

			}
		}
	}
}
//----------------------------------------------------------------------------------
//! Prints the hierarchical information of the tree
//----------------------------------------------------------------------------------
// ESRA
void World::outputBranchTree(BranchItem* branchItem, std::string prefix, FILE *&fp)

{

	fprintf(fp,"%s%u ( ",prefix.c_str(),branchItem->getLabel());
      //std::cout << prefix << branchItem->getLabel() << " ( ";

      prefix.append("----");

      if (branchItem->getParent() == NULL){

		  fprintf(fp,"NULL )\n");
            //std::cout << "NULL )" << std::endl;
 
	  }else{

		  fprintf(fp,"%u )\n",branchItem->getParent()->getLabel());
            //std::cout << branchItem->getParent()->getLabel() << " )" << std::endl;

      }

	  std::vector<vec3> *center = branchItem->getCenterList();
	  std::vector<vec3>::iterator iter1;

	  fprintf(fp,"size(%i):",center->size());
      for (iter1 = center->begin(); iter1 != center->end(); iter1++){
		  fprintf(fp,"(%f %f %f) ",(*iter1)[0],(*iter1)[1],(*iter1)[2]);
      }
	  fprintf(fp,"\n");

	  std::list<BranchItem*> *children = branchItem->getChildren();
	  std::list<BranchItem*>::iterator iter;

      for (iter = children->begin(); iter != children->end(); iter++){

		  BranchItem* item = *iter;
            outputBranchTree(item, prefix,fp);
      }

}

void World::set3dslicehandler(SlicesHandler *handler3D)
{
	_handler3D=handler3D;
}

Node::Node(unsigned offset, float intens){ 

	this->offset = offset;
	this->intens = intens;
	
	computed=false;
	first=false;
}
Node::~Node(){}

PathElement::PathElement(unsigned offset){
	
	this->offset = offset;
	
	cross=false;
	seed=false;
}
PathElement::~PathElement(){}



void World::getSeedBoundingBox(std::vector<vec3> *seeds, vec3 &bbStart, vec3 &bbEnd, SlicesHandler *handler3D)
{
	const long MEMORY_MAX_VOXELS = 18060000;	
	bbStart = vec3(10000, 10000, 10000);
	bbEnd   = vec3(0, 0, 0);
	int incVOI = 2;
	int margin = 15;

	// compute bounding box for seeds
	for (size_t i=0; i<seeds->size(); i++)
	{
		vec3 posVox = seeds->at(i);
		for (int j=0; j<3; j++)
		{
			if (posVox[j] < bbStart[j])
			{
				bbStart[j] = posVox[j];
			}
			if (posVox[j] > bbEnd[j])
			{
				bbEnd[j] = posVox[j];
			}
		}
	}

	// add margin
	vec3 imgExt((float)handler3D->return_width(),(float)handler3D->return_height(),(float)handler3D->return_nrslices());
//	Vector imgExt = getInImg(0)->getImgExt();


	for (int j=0; j<3; j++)
	{
		if (bbStart[j] > margin)  
		{
			bbStart[j] -= margin;
		}
		else
		{
			bbStart[j] = 0;
		}

		if (bbEnd[j]+margin < imgExt[j]-1)
		{
			bbEnd[j] += margin;
		}
		else
		{
			bbEnd[j] = imgExt[j]-1;
		}
	}

	int remaining = int((bbEnd[2] - bbStart[2]) * (bbEnd[1] - bbStart[1]) * (bbEnd[0] - bbStart[0]));
	remaining = MEMORY_MAX_VOXELS - remaining;
	remaining /= int(bbEnd[2] - bbStart[2]+1);

	bool isStop = false;

	if (remaining <= 0)
	{
		isStop = true;
	}

	while (!isStop)
	{
		bbEnd[0]   += incVOI;
		bbEnd[1]   += incVOI;
		bbStart[0] -= incVOI;
		bbStart[1] -= incVOI;

		if (bbEnd[0] > imgExt[0]-1)
		{
			bbEnd[0] = imgExt[0]-1;
		}
		if (bbEnd[1] > imgExt[1]-1)
		{
			bbEnd[1] = imgExt[1]-1;
		}
		if (bbStart[0] < 0)
		{
			bbStart[0] = 0;
		}
		if (bbStart[1] < 0)
		{
			bbStart[1] = 0;
		}

		/*remaining -= (2 * (bbEnd[0]-bbStart[0]));
		remaining -= (2 * (bbEnd[1]-bbStart[1]));
		remaining += 4;*/
		remaining = int((bbEnd[2] - bbStart[2]) * (bbEnd[1] - bbStart[1]) * (bbEnd[0] - bbStart[0]));
		remaining = MEMORY_MAX_VOXELS - remaining;

	    if ( (bbStart[0] == 0) && (bbStart[1] == 0) && (bbEnd[0] == imgExt[0]-1) && (bbEnd[1] == imgExt[1]-1) )
		{
			isStop = true;
		}

		if (remaining < 0)
		{
			isStop = true;
		}
	}

	bbStart[0] += incVOI;
	bbStart[1] += incVOI;
	bbEnd[0]   -= incVOI;
	bbEnd[1]   -= incVOI;

	// check if extension in z-direction is possible with remaining number of voxels
	if (remaining > 0)
	{
		int numPerSlice = int((bbEnd[0]-bbStart[0])*(bbEnd[1]-bbStart[1]));
		int numSlices = remaining / numPerSlice;
		
		// do not consider the fact, that numSlices might be odd
		int ext = numSlices /2;
		bbStart[2] -= ext;
		bbEnd[2] += ext;

		if (bbStart[2] < 0)
		{
			bbStart[2] = 0;
		}
		if (bbEnd[2] > imgExt[2]-1)
		{
			bbEnd[2] = imgExt[2]-1;
		}
	}
}
