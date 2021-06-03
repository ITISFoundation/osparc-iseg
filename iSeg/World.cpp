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

#include "World.h"

#include "Core/BranchItem.h"

#include <cmath>

namespace iseg {

World::World()
{
	m_IsValid = false;
	m_Nodes = nullptr;
}
World::~World() { Clear(); }

//----------------------------------------------------------------------------------
//! Deletes "nodes".
//----------------------------------------------------------------------------------
void World::Clear()
{
	if (m_Nodes)
	{
		delete[] m_Nodes;
		m_Nodes = nullptr;
	}
	m_IsValid = false;
}
//----------------------------------------------------------------------------------
//! Initializes volume.
//----------------------------------------------------------------------------------
//void World::init(vec3 bbStart, vec3 bbEnd, ml::TVirtualVolume<MLuint16>* vInVol, ml::TVirtualVolume<MLuint16>* vOutVol){
bool World::Init(Vec3 bbStart, Vec3 bbEnd, SlicesHandler* handler3D)
{
	Clear();

	//	ml::TimeCounter time2;

	m_Solvingarea = false;
	m_Times = 0;
	m_Activelistlowintsize = 0;
	m_Firstseedexpanded = false;

	m_BbStart = bbStart;
	m_BbEnd = bbEnd;
	m_Handler3D = handler3D;

	bbStart[0] = (float)(int)bbStart[0];
	bbStart[1] = (float)(int)bbStart[1];
	bbStart[2] = (float)(int)bbStart[2];

	m_Offx = (unsigned int)bbStart[0];
	m_Offy = (unsigned int)bbStart[1];
	m_Offz = (unsigned int)bbStart[2];

	bbEnd[0] = (float)(int)bbEnd[0];
	bbEnd[1] = (float)(int)bbEnd[1];
	bbEnd[2] = (float)(int)bbEnd[2];

	m_Width = (int)(bbEnd[0] - bbStart[0] + 1);
	m_Height = (int)(bbEnd[1] - bbStart[1] + 1);
	m_Length = (int)(bbEnd[2] - bbStart[2] + 1);

	std::cout << "Width:" << m_Width << " Height:" << m_Height
						<< " Length:" << m_Length << std::endl;

	unsigned total = m_Width * m_Height * m_Length;

	Node* nodes2 = nullptr;
	try
	{
		nodes2 = new Node[total];
	}
	catch (std::bad_alloc error)
	{
		nodes2 = nullptr;
	}

	if (nodes2 == nullptr)
	{
		std::cerr << "Memory allocation error!" << std::endl;
		//		exit(1); // terminate the program
		return false;
	}
	std::cout << "Size of initialized volume: " << total << std::endl;

	float px, py, pz;
	float gradient, fgradient;
	float intens, fintens;

	for (unsigned o = 0; o < total; o++)
	{
		unsigned k = o / (m_Width * m_Height);
		unsigned j = (o - (k * m_Width * m_Height)) / m_Width;
		unsigned i = o - (j * m_Width) - (k * m_Width * m_Height);

		//intens
		// ESRA
		// vInVol->getVoxel(x,y,z,0,0,0) reads the voxel intensity
		//intens=(float)(vInVol->getVoxel(ml::Vector(i+bbStart[0], j+bbStart[1], k+bbStart[2], 0, 0, 0)));
		Point p;
		p.px = (short)(i + bbStart[0]);
		p.py = (short)(j + bbStart[1]);
		unsigned short slicenr = (unsigned short)(k + bbStart[2]);
		intens = m_Handler3D->GetBmpPt(p, slicenr);

		if (intens > 1250 && intens < 1300)
			fintens = 100;
		else if (intens > 1150 && intens <= 1250)
			fintens = 10000;
		else if (intens > 1000 && intens <= 1150)
			fintens = 1000000;
		else if (intens <= 1000)
			fintens = 10000000000.;
		else
		{
			intens = 1300;
			fintens = 0;
		}

		//initialize nodes
		Node nt(o, intens);
		//std::cout << "Node size: " << sizeof(Node) << std::endl;
		nodes2[o] = nt;

		//gradient
		if (i > 0 && j > 0 && k > 0)
		{
			unsigned o2 = (i - 1) + j * m_Width + k * m_Width * m_Height;
			unsigned o3 = i + (j - 1) * m_Width + k * m_Width * m_Height;
			unsigned o4 = i + j * m_Width + (k - 1) * m_Width * m_Height;

			px = fabs(nodes2[o].m_Intens - nodes2[o2].m_Intens);
			py = fabs(nodes2[o].m_Intens - nodes2[o3].m_Intens);
			pz = fabs(nodes2[o].m_Intens - nodes2[o4].m_Intens);
			gradient = sqrt(px * px + py * py + pz * pz);

			if (gradient > 170)
				fgradient = 1000;
			//if(gradient<50) gradient=50;
			else
				fgradient = ((gradient / 170) * 1000);
		}
		else
			fgradient = 1;
		nodes2[o].m_Fcost = (fgradient * 0.8f + fintens * 0.2f);
	}

	this->m_Nodes = nodes2;
	//	std::cout << "TIME for initialization: " << time2.getRunningTime() << std::endl;

	m_IsValid = true;
	return true;
}
//----------------------------------------------------------------------------------
//! Modified livewire algorithm.
//----------------------------------------------------------------------------------
void World::Dijkstra(std::vector<Vec3> seeds, Vec3 end, BranchTree* _branchTree)
{
	if (!m_IsValid)
	{
		std::cerr << "!_isValid" << std::endl;
		return;
	}

	//Declaring seed/end/cross points...

	unsigned short int seedsleft = seeds.size();
	unsigned short int n_x = (unsigned short)(seeds[0][0] - m_Offx);
	unsigned short int n_y = (unsigned short)(seeds[0][1] - m_Offy);
	unsigned short int n_z = (unsigned short)(seeds[0][2] - m_Offz);

	Vec3 endpoint;

	endpoint[0] = end[0] - m_Offx;
	endpoint[1] = end[1] - m_Offy;
	endpoint[2] = end[2] - m_Offz;

	unsigned off = n_x + n_y * m_Width + n_z * m_Width * m_Height;
	unsigned short int cross_x;
	unsigned short int cross_y;
	unsigned short int cross_z;

	std::vector<Vec3> cross(seeds.size() - 1);

	//Declaring BranchItems

	// ESRA
	BranchItem* root_one = _branchTree->AddNewBranch();
	root_one->SetParent(nullptr);
	std::vector<BranchItem*> children;

	//Declaring variables

	bool redfound = false;
	bool first = false;
	bool endt = false;
	bool ends = false;
	bool firstfound = false;
	bool morethanoneseed = true;
	int min;
	int counter = 0;
	bool largearea = false;
	int largeareatimes = 0;
	using li = std::list<Node*>::iterator;
	float tmp;
	bool alreadyinlist, alreadyinlistlowint;
	int parentintens;
	int k, j, i;

	//Initializing first node to expand (first seed)

	m_Nodes[off].m_Cost = 0;
	m_Activelist.push_back(&m_Nodes[off]);
	std::cout << "Expanding seed " << 1 << std::endl;

	//----------------------------------------------------------------------------------
	//@ Main loop
	//----------------------------------------------------------------------------------

	while ((!m_Activelist.empty() || m_Solvingarea == true) && !endt)
	{
		if (m_Activelist.empty())
		{
			std::cout << "Activelist empty!!" << std::endl;
			off = Solvelargearea2();
			m_Activelist.push_back(&m_Nodes[off]);
		}

		//Reducing activelist if it is too large
		if (m_Activelist.size() > 3000 && largearea == false)
		{
			Reduceactivelist();
			//std::cout << "Reducing activelist" << std::endl;
		}

		//Skipping seed if it is lasting too much to find the path
		if (counter > 150000)
		{
			std::cerr << "Troubles finding the path for seed "
								<< seeds.size() - seedsleft + 1 << std::endl;
			m_Seedsfailed.push_back(seeds.size() - seedsleft + 1);
			seedsleft = seedsleft - 1;
			if (seedsleft > 0)
			{
				int total = m_Width * m_Height * m_Length;
				int pos = seeds.size() - seedsleft;
				for (int o5 = 0; o5 < total; o5++)
				{
					if (m_Nodes[o5].m_Computed == true)
					{
						m_Nodes[o5].m_Computed = false;
					}
				}
				//std::cout << "Counter: " << counter << std::endl;
				std::cout << "Expanding seed " << pos + 1 << std::endl;

				n_x = (unsigned short)(seeds[pos][0] - m_Offx);
				n_y = (unsigned short)(seeds[pos][1] - m_Offy);
				n_z = (unsigned short)(seeds[pos][2] - m_Offz);

				unsigned off5 = n_x + n_y * m_Width + n_z * m_Width * m_Height;
				std::list<Node*> activelist2;
				m_Activelist = activelist2;
				m_Activelist.push_back(&m_Nodes[off5]);

				redfound = false;
				counter = 0;
				m_Solvingarea = false;
				largearea = false;
				first = false;
				largeareatimes = 0;
				m_Times = 0;
			}
			else
			{
				m_Solvingarea = false;
				m_Firstseedexpanded = false;
				m_Times = 0;
				m_Activelist.clear();
				m_Activelistlowint.clear();
				if (firstfound == true)
					Storingtree(children, root_one);
				m_Paths.clear();
				m_Seedsfailed.clear();
				return;
			}
		}

		//When a path (different from the first one) has been tracked...

		if (redfound == true)
		{
			int pos = seeds.size() - seedsleft;
			//std::cout << "Parentintens: " << parentintens << std::endl;
			int parent = Whoistheparent(seeds, parentintens);
			//std::cout << "The parent is: " << parent << std::endl;

			if (parent == 4000 - m_RootOneintens)
			{
				// ESRA
				BranchItem* child = new BranchItem(root_one);
				child->SetStartVox(seeds[pos - 1]);
				child->SetEndVox(cross[pos - 2]);
				root_one->AddChild(child);
				children.push_back(child);
			}
			else
			{
				int prevfailed = 0;
				for (size_t s = 0; s < m_Seedsfailed.size(); s++)
				{
					if (m_Seedsfailed.at(s) < parent + 1)
					{
						prevfailed++;
						//std::cout << "Prevfailed: " << prevfailed << "Failed seed: " << seedsfailed.at(s) << "Parent: " << parent+1 << std::endl;
					}
				}

				// ESRA
				BranchItem* child =
						new BranchItem(children.at(parent - 1 - prevfailed));
				//unsigned label= children.at(parent-1-prevfailed)->getLabel();
				//std::cout << "Parentlabel: " << label << std::endl;
				child->SetStartVox(seeds[pos - 1]);
				child->SetEndVox(cross[pos - 2]);
				children.at(parent - 1 - prevfailed)->AddChild(child);
				children.push_back(child);
			}
			Paint(seeds, cross[pos - 2], seedsleft);

			//Set all the nodes to computed=false

			int total = m_Width * m_Height * m_Length;
			for (int o5 = 0; o5 < total; o5++)
			{
				if (m_Nodes[o5].m_Computed == true)
				{
					m_Nodes[o5].m_Computed = false;
					m_Nodes[o5].m_First = true;
				}
			}
			std::cout << "Counter: " << counter << std::endl;
			std::cout << "Expanding seed " << pos + 1 << std::endl;

			n_x = (unsigned short)(seeds[pos][0] - m_Offx);
			n_y = (unsigned short)(seeds[pos][1] - m_Offy);
			n_z = (unsigned short)(seeds[pos][2] - m_Offz);

			unsigned off5 = n_x + n_y * m_Width + n_z * m_Width * m_Height;
			std::list<Node*> activelist2;
			m_Activelist = activelist2;
			m_Activelistlowint = activelist2;
			m_Activelist.push_back(&m_Nodes[off5]);

			redfound = false;
			counter = 0;
			m_Solvingarea = false;
			largearea = false;
			first = false;
			largeareatimes = 0;
			m_Times = 0;
		}

		//When the first path has been tracked...

		if (ends == true)
		{
			int pos = seeds.size() - seedsleft;
			Paint(seeds, endpoint, seedsleft);

			root_one->SetStartVox(seeds[pos - 1]);
			root_one->SetEndVox(end);

			//Set all the nodes to computed=false
			int total = m_Width * m_Height * m_Length;

			for (int o5 = 0; o5 < total; o5++)
			{
				if (m_Nodes[o5].m_Computed == true)
				{
					m_Nodes[o5].m_Computed = false;
					m_Nodes[o5].m_First = true;
				}
			}
			std::cout << "Counter: " << counter << std::endl;
			std::cout << "Expanding seed " << pos + 1 << std::endl;

			//Initialize for second seed

			n_x = (unsigned short)(seeds[pos][0] - m_Offx);
			n_y = (unsigned short)(seeds[pos][1] - m_Offy);
			n_z = (unsigned short)(seeds[pos][2] - m_Offz);

			unsigned off5 = n_x + n_y * m_Width + n_z * m_Width * m_Height;
			std::list<Node*> activelist2;
			m_Activelist = activelist2;
			m_Activelist.push_back(&m_Nodes[off5]);

			ends = false;
			firstfound = true;
			counter = 0;
			m_Solvingarea = false;
			largearea = false;
			largeareatimes = 0;
			m_Times = 0;
			m_Firstseedexpanded = true;
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

		min = -1;
		li todel;

		//Looking for the node with the mininum cost in activelist

		if (m_Solvingarea == false)
		{
			for (li ii = m_Activelist.begin(); ii != m_Activelist.end(); ++ii)
			{
				Node* e = *ii;
				if (min == -1 || min > e->m_Cost)
				{
					min = (int)e->m_Cost;
					off = e->m_Offset;
					todel = ii;
				}
			}
			m_Activelist.erase(todel);
		}

		if (m_Nodes[off].m_Computed == true)
			std::cout << "Node to expand already expanded!!" << std::endl;

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

		m_Nodes[off].m_Computed = true;

		n_z = off / (m_Width * m_Height);
		n_y = (off - (n_z * m_Width * m_Height)) / m_Width;
		n_x = off - (n_y * m_Width) - (n_z * m_Width * m_Height);

		// ----------------------------------------------------------
		//@ The node that is going to be expanded can be painted in the output here.
		// ----------------------------------------------------------
		//vOutVol->setVoxel(ml::Vector(n_x+offx, n_y+offy, n_z+offz, 0, 0, 0), 4000);

		counter++; //Counts how many nodes are expanded (iterations of the main loop)

		for (k = n_z - 1; k < n_z + 2; k++)
		{
			for (j = n_y - 1; j < n_y + 2; j++)
			{
				for (i = n_x - 1; i < n_x + 2; i++)
				{
					unsigned o = i + j * m_Width +
											 k * m_Width * m_Height; //offset of the neighbor
					unsigned o2 =
							n_x + n_y * m_Width +
							n_z * m_Width *
									m_Height; //offset of the node that is being expanded
					//std::cout << "Node to expand: " << "x:" << i << " y:" << j << " z:" << k << " o: " << o << " o2: " << o2 << std::endl;

					if (i > -1 && j > -1 && k > -1 && i < m_Width && j < m_Height &&
							k < m_Length && !m_Nodes[o].m_Computed)
					{
						if (m_Nodes[o].m_Intens > 3900 && !endt && !redfound)
						{ //if the node belongs to a previously painted path

							parentintens = (int)m_Nodes[o].m_Intens;
							seedsleft = seedsleft - 1;

							if (seedsleft == 0)
							{
								endt = true;
								std::cout << "Finished" << std::endl;
							}
							else
							{
								redfound = true;
								std::cout << "Red node found (path tracked)"
													<< std::endl;
							}
							cross_z = o / (m_Width * m_Height);
							cross_y = (o - (cross_z * m_Width * m_Height)) / m_Width;
							cross_x = o - (cross_y * m_Width) -
												(cross_z * m_Width * m_Height);
							int pos = seeds.size() - seedsleft;
							cross[pos - 2][2] = cross_z;
							cross[pos - 2][1] = cross_y;
							cross[pos - 2][0] = cross_x;

							std::cout << "Cross node: "
												<< "x:" << cross[pos - 2][0] + m_Offx
												<< " y:" << cross[pos - 2][1] + m_Offy
												<< " z:" << cross[pos - 2][2] + m_Offz
												<< " o:" << o << std::endl;
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

						tmp = m_Nodes[o].m_Fcost + m_Nodes[o2].m_Cost;

						//Restarting variables

						alreadyinlist = false;
						alreadyinlistlowint = false;

						//First path is found when the endpoint is found

						if (!m_Firstseedexpanded)
						{
							if (i == endpoint[0] && j == endpoint[1] &&
									k == endpoint[2])
							{
								seedsleft = seedsleft - 1;
								if (seedsleft == 0)
								{
									endt = true;
									std::cout << "Finished" << std::endl;
									morethanoneseed = false;
								}
								else
								{
									ends = true;
									std::cout << "First seed expanded (first "
															 "path tracked)"
														<< std::endl;
								}
							}
						}

						//Checking if it is in activelistlowint

						li p = m_Activelistlowint.begin();
						if (m_Solvingarea == true)
						{
							while (!alreadyinlistlowint &&
										 p != m_Activelistlowint.end())
							{
								Node* e = *p;
								if (e->m_Offset == o)
								{
									alreadyinlistlowint = true;
									//activelistlowint.erase(p);
								}
								p++;
							}
						}

						//Checking if the neighbour is already in activelist

						li l = m_Activelist.begin();
						while (!alreadyinlist && l != m_Activelist.end())
						{
							Node* e = *l;
							if (e->m_Offset == o)
							{
								alreadyinlist = true;
							}
							l++;
						}

						//If it was in one of the activelists and the new cost is lower, update it

						if (alreadyinlist || alreadyinlistlowint)
						{
							if (m_Nodes[o].m_Cost > tmp + 10)
							{
								//activelist.erase(activelist.begin()+todel);
								//alreadyinlist=false;
								m_Nodes[o].m_Cost = tmp + 10;
								m_Nodes[o].m_Prev = &m_Nodes[o2];
							}
						}

						//Otherwise, save the cost, the path, and put the node in activelist

						else
						{
							if (m_Nodes[o].m_Intens > 1000)
							{
								m_Nodes[o].m_Cost = tmp + 10;
								m_Nodes[o].m_Prev = &m_Nodes[o2];
								m_Activelist.push_back(&m_Nodes[o]);
							}
						}
					}
				}
			}
		}
	}

	//Dealing with last path found (painting it, storing it...)
	if (morethanoneseed == true)
	{ //if there were more than one seed
		int pos = seeds.size() - seedsleft;
		int parent = Whoistheparent(seeds, parentintens);
		if (parent == 4000 - m_RootOneintens)
		{
			// ESRA
			BranchItem* child = new BranchItem(root_one);
			child->SetStartVox(seeds[pos - 1]);
			child->SetEndVox(cross[pos - 2]);
			root_one->AddChild(child);
			children.push_back(child);
		}
		else
		{
			int prevfailed = 0;
			for (size_t s = 0; s < m_Seedsfailed.size(); s++)
			{
				if (m_Seedsfailed.at(s) < parent + 1)
				{
					prevfailed++;
				}
			}
			// ESRA
			BranchItem* child =
					new BranchItem(children.at(parent - 1 - prevfailed));
			child->SetStartVox(seeds[pos - 1]);
			child->SetEndVox(cross[pos - 2]);
			children.at(parent - 1 - prevfailed)->AddChild(child);
			children.push_back(child);
		}
		Paint(seeds, cross[seeds.size() - 2], seedsleft);
	}
	else
	{
		Paint(seeds, endpoint, seedsleft); //if there was only one seed
	}

	//Clearing lists, restarting variables.

	std::cout << "Counter: " << counter << std::endl;
	m_Solvingarea = false;
	m_Firstseedexpanded = false;
	m_Times = 0;
	m_Activelist.clear();
	m_Activelistlowint.clear();
	Storingtree(children, root_one);
	m_Paths.clear();
	m_Seedsfailed.clear();
}

//----------------------------------------------------------------------------------
//! Paints path from seed when it is found.
//----------------------------------------------------------------------------------
void World::Paint(std::vector<Vec3> seeds, Vec3 cross, unsigned short int seedsleft)
{
	bool endp2 = false;
	int n_x = (int)(cross[0]);
	int n_y = (int)(cross[1]);
	int n_z = (int)(cross[2]);
	unsigned o2 = n_x + n_y * m_Width + n_z * m_Width * m_Height;
	unsigned short int t_x, t_y, t_z;
	unsigned o3;
	int pos = seeds.size() - seedsleft - 1;

	//painting the path

	std::cout << "Painting path " << pos + 1 << ": x: " << n_x << " y: " << n_y
						<< " z: " << n_z << std::endl;
	std::cout << "Activelistsize: " << m_Activelist.size() << std::endl;
	std::cout << "Seedsleft: " << seedsleft << std::endl;
	int counter = 0;
	std::vector<PathElement> newpath;

	while (!endp2)
	{
		if (n_x == seeds[pos][0] - m_Offx && n_y == seeds[pos][1] - m_Offy &&
				n_z == seeds[pos][2] - m_Offz)
		{
			endp2 = true;
			m_Nodes[o2].m_Intens = (float)(4000 - pos);

			PathElement pathelement(o2);
			newpath.push_back(pathelement);
		}
		else
		{
			t_x = n_x;
			t_y = n_y;
			t_z = n_z;
			o3 = t_x + t_y * m_Width + t_z * m_Width * m_Height;

			PathElement pathelement(o3);
			newpath.push_back(pathelement);

			// ESRA -> setVoxel writes voxel value to output image
			Point p;
			p.px = (short)(n_x + m_Offx);
			p.py = (short)(n_y + m_Offy);
			unsigned short slicenr = (unsigned short)n_z + m_Offz;
			//_handler3D->set_work_pt(p,slicenr,4000-pos);xxxa
			m_Handler3D->SetWorkPt(p, slicenr, 4000);

			//std::cout << "Path node: " << "x:" << n_x << " y:" << n_y << " z:" << n_z << " Offset:" << o3 << " Intens:" << nodes[o3].intens << " Fcost:" << nodes[o3].fcost << " Cost:" << nodes[o3].cost << " First:" << nodes[o3].first << std::endl;
			m_Nodes[o3].m_Intens = (float)(4000 - pos);
			if (!m_Firstseedexpanded)
				m_RootOneintens = 4000 - pos;
			o2 = m_Nodes[o3].m_Prev->m_Offset;
			counter++;

			n_z = o2 / (m_Width * m_Height);
			n_y = (o2 - (n_z * m_Width * m_Height)) / m_Width;
			n_x = o2 - (n_y * m_Width) - (n_z * m_Width * m_Height);
		}
	}
	//if(firstseedexpanded) newpath[0].cross=true;
	for (size_t i = 0; i < m_Paths.size(); i++)
	{
		for (size_t j = 0; j < m_Paths[i].size(); j++)
		{
			if (m_Paths[i][j].m_Offset == newpath.at(0).m_Offset)
			{
				m_Paths[i][j].m_Cross = true;
			}
		}
	}
	newpath[counter].m_Seed = true;
	m_Paths.push_back(newpath);
	for (size_t k = 0; k < newpath.size(); k++)
	{
		//std::cout << "Path node: " << "o:" << newpath[k].offset << " cross:" << newpath[k].cross << std::endl;
	}
	//std::cout << "Path long: " << counter << std::endl;
}
//----------------------------------------------------------------------------------
//! Stores the tree information in BranchTree.
// (When the execution of the main algorithm finishes, there is only one BranchItem for each tracked path;
//  from that information this function creates the right BranchTree.)
//----------------------------------------------------------------------------------
void World::Storingtree(std::vector<BranchItem*> children, BranchItem* rootOne)
{
	//Matrix with the cross points of each path

	std::vector<std::vector<unsigned>> allcrosses;

	for (size_t i = 0; i < m_Paths.size(); i++)
	{
		std::vector<unsigned> crosses;
		for (size_t j = 0; j < m_Paths[i].size(); j++)
		{
			if (m_Paths[i][j].m_Cross)
			{
				unsigned o = m_Paths[i][j].m_Offset;
				crosses.push_back(o);
			}
		}
		allcrosses.push_back(crosses);
	}

	int i = 0;
	bool dup = false;
	int pathcrosses = 0;
	// ESRA
	std::vector<std::vector<BranchItem*>> stretches;
	std::vector<BranchItem*> pathstretches;
	stretches.push_back(pathstretches);

	for (size_t j = 0; j < m_Paths[i].size(); j++)
	{
		if (m_Paths[i][j].m_Cross)
		{
			pathcrosses++;
			unsigned o = m_Paths[i][j].m_Offset;
			Vec3 cross;
			int crossz = o / (m_Width * m_Height);
			int crossy = (o - (crossz * m_Width * m_Height)) / m_Width;
			int crossx = o - (crossy * m_Width) - (crossz * m_Width * m_Height);
			cross[2] = (float)crossz;
			cross[1] = (float)crossy;
			cross[0] = (float)crossx;

			if (!dup)
			{
				//std::cout << "First duplicate branch for path 1: " << pathcrosses << std::endl;

				// ESRA
				BranchItem* dupchild = new BranchItem(rootOne);
				//unsigned label=dupchild->getLabel();
				//std::cout << "Newbranchlabel: " << label << std::endl;
				rootOne->SetStartVox(cross);
				dupchild->SetEndVox(cross);

				if (pathcrosses < (int)allcrosses[i].size())
				{
					unsigned off = allcrosses[i][pathcrosses];
					int nextcrossz = off / (m_Width * m_Height);
					int nextcrossy =
							(off - (nextcrossz * m_Width * m_Height)) / m_Width;
					int nextcrossx = off - (nextcrossy * m_Width) -
													 (nextcrossz * m_Width * m_Height);
					Vec3 nextcross;
					nextcross[2] = (float)nextcrossz;
					nextcross[1] = (float)nextcrossy;
					nextcross[0] = (float)nextcrossx;
					dupchild->SetStartVox(nextcross);
				}
				else
				{
					unsigned o = m_Paths[i][j].m_Offset;

					int seedz = o / (m_Width * m_Height);
					int seedy = (o - (seedz * m_Width * m_Height)) / m_Width;
					int seedx = o - (seedy * m_Width) - (seedz * m_Width * m_Height);
					Vec3 seed;
					seed[2] = (float)seedz;
					seed[1] = (float)seedy;
					seed[0] = (float)seedx;
					dupchild->SetStartVox(seed);
				}
				// ESRA
				rootOne->AddChild(dupchild);
				rootOne->AddCenter(cross); //<--HERE
				stretches[i].push_back(dupchild);
				dup = true;
			}
			else
			{
				//std::cout << "Duplicate branch for path 1: " << pathcrosses << std::endl;

				BranchItem* dupchild =
						new BranchItem(stretches[i].at(pathcrosses - 2));
				//unsigned label=dupchild->getLabel();
				//std::cout << "Newbranchlabel: " << label << std::endl;
				stretches[i].at(pathcrosses - 2)->SetStartVox(cross);

				for (size_t k = 1; k < m_Paths.size(); k++)
				{
					unsigned off = m_Paths[k][0].m_Offset;
					Vec3 first;
					int firstz = off / (m_Width * m_Height);
					int firsty = (off - (firstz * m_Width * m_Height)) / m_Width;
					int firstx =
							off - (firsty * m_Width) - (firstz * m_Width * m_Height);
					first[2] = (float)firstz;
					first[1] = (float)firsty;
					first[0] = (float)firstx;

					if (first == cross)
					{
						// ESRA
						unsigned childlabel = children[k - 1]->GetLabel();
						BranchItem* prevparent = children[k - 1]->GetParent();
						unsigned prevparentlabel = prevparent->GetLabel();

						if (rootOne->GetLabel() == prevparentlabel)
						{
							rootOne->RemoveChild(childlabel);
							//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
						}
						for (size_t j = 0; j < children.size(); j++)
						{
							//std::cout << "Child " << j << " Label " << children[j]->getLabel() << std::endl;
							if (children[j]->GetLabel() == prevparentlabel)
							{
								children[j]->RemoveChild(childlabel);
								//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
							}
						}
						for (size_t p = 0; p < stretches.size(); p++)
						{
							for (size_t j = 0; j < stretches[p].size(); j++)
							{
								if (stretches[p][j]->GetLabel() ==
										prevparentlabel)
								{
									stretches[p][j]->RemoveChild(childlabel);
									//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
								}
							}
						}

						stretches[i]
								.at(pathcrosses - 2)
								->AddChild(children[k - 1]);
						//std::cout << "Newparent: " << stretches[i].at(pathcrosses-2)->getLabel() << std::endl;
						children[k - 1]->SetParent(stretches[i].at(pathcrosses - 2));
					}
				}

				dupchild->SetEndVox(cross);

				if (pathcrosses < (int)allcrosses[i].size())
				{
					unsigned off = allcrosses[i][pathcrosses];
					int nextcrossz = off / (m_Width * m_Height);
					int nextcrossy =
							(off - (nextcrossz * m_Width * m_Height)) / m_Width;
					int nextcrossx = off - (nextcrossy * m_Width) -
													 (nextcrossz * m_Width * m_Height);
					Vec3 nextcross;
					nextcross[2] = (float)nextcrossz;
					nextcross[1] = (float)nextcrossy;
					nextcross[0] = (float)nextcrossx;
					dupchild->SetStartVox(nextcross);
				}
				else
				{
					unsigned o = m_Paths[i][j].m_Offset;
					int seedz = o / (m_Width * m_Height);
					int seedy = (o - (seedz * m_Width * m_Height)) / m_Width;
					int seedx = o - (seedy * m_Width) - (seedz * m_Width * m_Height);
					Vec3 seed;
					seed[2] = (float)seedz;
					seed[1] = (float)seedy;
					seed[0] = (float)seedx;
					dupchild->SetStartVox(seed);
				}
				stretches[i].at(pathcrosses - 2)->AddChild(dupchild);
				stretches[i][pathcrosses - 2]->AddCenter(cross); //<--HERE
				stretches[i].push_back(dupchild);
			}
		}

		Vec3 node;
		unsigned off = m_Paths[i][j].m_Offset;
		int nodez = off / (m_Width * m_Height);
		int nodey = (off - (nodez * m_Width * m_Height)) / m_Width;
		int nodex = off - (nodey * m_Width) - (nodez * m_Width * m_Height);
		nodez = nodez + m_Offz;
		nodey = nodey + m_Offy;
		nodex = nodex + m_Offx;
		node[0] = (float)nodex;
		node[1] = (float)nodey;
		node[2] = (float)nodez;
		// ESRA
		Point p;
		p.px = (short)(nodex);
		p.py = (short)(nodey);
		unsigned short slicenr = (unsigned short)nodez;
		m_Handler3D->SetWorkPt(p, slicenr, 4000);

		if (dup)
		{
			stretches[i][pathcrosses - 1]->AddCenter(node); //
		}
		else
		{
			rootOne->AddCenter(node); //
		}
	}

	for (size_t i = 1; i < m_Paths.size(); i++)
	{
		// ESRA
		std::vector<BranchItem*> pathstretches;
		stretches.push_back(pathstretches);
		pathcrosses = 0;
		dup = false;

		for (size_t j = 0; j < m_Paths[i].size(); j++)
		{
			if (m_Paths[i][j].m_Cross == true)
			{
				pathcrosses++;
				unsigned o = m_Paths[i][j].m_Offset;
				Vec3 cross;
				int crossz = o / (m_Width * m_Height);
				int crossy = (o - (crossz * m_Width * m_Height)) / m_Width;
				int crossx = o - (crossy * m_Width) - (crossz * m_Width * m_Height);
				cross[2] = (float)crossz;
				cross[1] = (float)crossy;
				cross[0] = (float)crossx;
				//std::cout << "Cross " << " x: " << crossx << " y: " << crossy << " z: " << crossz << std::endl;

				if (!dup)
				{
					// ESRA
					BranchItem* dupchild = new BranchItem(children.at(i - 1));
					//unsigned label=dupchild->getLabel();
					//std::cout << "Newbranchlabel: " << label << std::endl;
					children.at(i - 1)->SetStartVox(cross);
					dupchild->SetEndVox(cross);
					if (pathcrosses < (int)allcrosses[i].size())
					{
						unsigned off = allcrosses[i][pathcrosses];
						int nextcrossz = off / (m_Width * m_Height);
						int nextcrossy =
								(off - (nextcrossz * m_Width * m_Height)) / m_Width;
						int nextcrossx = off - (nextcrossy * m_Width) -
														 (nextcrossz * m_Width * m_Height);
						Vec3 nextcross;
						nextcross[2] = (float)nextcrossz;
						nextcross[1] = (float)nextcrossy;
						nextcross[0] = (float)nextcrossx;
						dupchild->SetStartVox(nextcross);
					}
					else
					{
						unsigned o = m_Paths[i][j].m_Offset;
						int seedz = o / (m_Width * m_Height);
						int seedy = (o - (seedz * m_Width * m_Height)) / m_Width;
						int seedx =
								o - (seedy * m_Width) - (seedz * m_Width * m_Height);
						Vec3 seed;
						seed[2] = (float)seedz;
						seed[1] = (float)seedy;
						seed[0] = (float)seedx;
						children.at(i)->SetStartVox(seed);
					}

					// ESRA
					children.at(i - 1)->AddChild(dupchild);
					children[i - 1]->AddCenter(cross); //<--HERE
					stretches[i].push_back(dupchild);
					dup = true;
				}
				else
				{
					// ESRA
					BranchItem* dupchild =
							new BranchItem(stretches[i].at(pathcrosses - 2));
					//unsigned label=dupchild->getLabel();
					//std::cout << "Newbranchlabel: " << label << std::endl;
					stretches[i].at(pathcrosses - 2)->SetStartVox(cross);
					for (size_t k = 1; k < m_Paths.size(); k++)
					{
						unsigned off = m_Paths[k][0].m_Offset;
						Vec3 first;
						int firstz = off / (m_Width * m_Height);
						int firsty = (off - (firstz * m_Width * m_Height)) / m_Width;
						int firstx =
								off - (firsty * m_Width) - (crossz * m_Width * m_Height);
						first[2] = (float)firstz;
						first[1] = (float)firsty;
						first[0] = (float)firstx;
						if (first == cross)
						{
							unsigned childlabel = children[k - 1]->GetLabel();
							BranchItem* prevparent =
									children[k - 1]->GetParent();
							unsigned prevparentlabel = prevparent->GetLabel();

							if (rootOne->GetLabel() == prevparentlabel)
							{
								rootOne->RemoveChild(childlabel);
								//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
							}
							for (size_t j = 0; j < children.size(); j++)
							{
								if (children[j]->GetLabel() == prevparentlabel)
								{
									children[j]->RemoveChild(childlabel);
									//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
								}
							}
							for (size_t p = 0; p < stretches.size(); p++)
							{
								for (size_t j = 0; j < stretches[p].size(); j++)
								{
									if (stretches[p][j]->GetLabel() ==
											prevparentlabel)
									{
										stretches[p][j]->RemoveChild(childlabel);
										//std::cout << "Removedchild: " << childlabel << " Prevparent: " << prevparentlabel << std::endl;
									}
								}
							}

							stretches[i]
									.at(pathcrosses - 2)
									->AddChild(children[k - 1]);
							//std::cout << "Newparent: " << stretches[i].at(pathcrosses-2)->getLabel() << std::endl;
							children[k - 1]->SetParent(stretches[i].at(pathcrosses - 2));
						}
					}

					dupchild->SetEndVox(cross);

					if (pathcrosses < (int)allcrosses[i].size())
					{
						unsigned off = allcrosses[i][pathcrosses];
						int nextcrossz = off / (m_Width * m_Height);
						int nextcrossy =
								(off - (nextcrossz * m_Width * m_Height)) / m_Width;
						int nextcrossx = off - (nextcrossy * m_Width) -
														 (nextcrossz * m_Width * m_Height);
						Vec3 nextcross;
						nextcross[2] = (float)nextcrossz;
						nextcross[1] = (float)nextcrossy;
						nextcross[0] = (float)nextcrossx;
						dupchild->SetStartVox(nextcross);
					}
					else
					{
						unsigned o = m_Paths[i][j].m_Offset;
						int seedz = o / (m_Width * m_Height);
						int seedy = (o - (seedz * m_Width * m_Height)) / m_Width;
						int seedx =
								o - (seedy * m_Width) - (seedz * m_Width * m_Height);
						Vec3 seed;
						seed[2] = (float)seedz;
						seed[1] = (float)seedy;
						seed[0] = (float)seedx;
						dupchild->SetStartVox(seed);
					}
					stretches[i].at(pathcrosses - 2)->AddChild(dupchild);
					stretches[i][pathcrosses - 2]->AddCenter(cross); //<--HERE
					stretches[i].push_back(dupchild);
				}
			}

			Vec3 node;
			unsigned off = m_Paths[i][j].m_Offset;
			int nodez = off / (m_Width * m_Height);
			int nodey = (off - (nodez * m_Width * m_Height)) / m_Width;
			int nodex = off - (nodey * m_Width) - (nodez * m_Width * m_Height);
			nodez = nodez + m_Offz;
			nodey = nodey + m_Offy;
			nodex = nodex + m_Offx;
			node[0] = (float)nodex;
			node[1] = (float)nodey;
			node[2] = (float)nodez;
			//vOutVol->setVoxel(ml::Vector(nodex, nodey, nodez, 0, 0, 0), 4000);

			if (dup)
			{
				stretches[i][pathcrosses - 1]->AddCenter(node); //
			}
			else
			{
				children[i - 1]->AddCenter(node); //
			}
		}
	}

	rootOne->CorrectBranchpoints();
}

//----------------------------------------------------------------------------------
//! Says who is the parent branch (where is located its startpoint in the vector "seeds")
//----------------------------------------------------------------------------------
int World::Whoistheparent(std::vector<Vec3> seeds, int parentintens)
{
	int parent = -1;

	//std::cout << "Parentintens: " << parentintens << std::endl;

	for (size_t i = 0; i < seeds.size(); i++)
	{
		if (parentintens == (4000 - i))
		{
			parent = i;
			//std::cout << "Parent: " << i << std::endl;
			return (parent);
		}
	}
	return (parent);
}
//----------------------------------------------------------------------------------
//! Reduce size of activelist by removing every second node
//----------------------------------------------------------------------------------
void World::Reduceactivelist()
{
	using li = std::list<Node*>::iterator;
	std::list<Node*> activelistempty;
	Node* e;
	unsigned off;

	int par = 0;
	for (li ia = m_Activelist.begin(); ia != m_Activelist.end(); ++ia)
	{
		e = *ia;
		off = e->m_Offset;
		if (!(par % 2))
		{
			if (m_Nodes[off].m_Intens < 3900)
			{
				m_Nodes[off].m_Computed = true;
				//int z = off / (width * height);
				//int y = (off - (z * width * height)) / width;
				//int x = off - (y * width) - (z * width * height);
				//vOutVol->setVoxel(ml::Vector(x+offx, y+offy, z+offz, 0, 0, 0), 4000);
			}
		}
		else
		{
			activelistempty.push_back(&m_Nodes[off]);
		}
		par++;
	}
	m_Activelist = activelistempty;
}
//----------------------------------------------------------------------------------
//! Reassigns costs to intensities
//----------------------------------------------------------------------------------
void World::Changeintens(unsigned o)
{
	//int z = o / (width * height);
	//int y = (o - (z * width * height)) / width;
	//int x = o - (y * width) - (z * width * height);
	if (m_Nodes[o].m_Intens > 1000 && m_Nodes[o].m_Intens <= 1150)
	{
		m_Nodes[o].m_Fcost = 10;
		//if(nodes[o].computed==true) std::cout << "Node already computed!!" << std::endl;
		//std::cout << "NODE: " << "x:" << x << " y:" << y << " z:" << z << " Computed: " << nodes[off].computed << std::endl;
	}
	else if (m_Nodes[o].m_Intens > 1150 && m_Nodes[o].m_Intens <= 1230)
	{
		if (m_Nodes[o].m_Cost == 0 && m_Nodes[o].m_First == false)
			m_Nodes[o].m_Fcost = m_Nodes[o].m_Fcost + m_Nodes[o].m_Fcost * 10;
	}
	else
	{
		if (m_Nodes[o].m_Cost == 0 && m_Nodes[o].m_First == false)
			m_Nodes[o].m_Fcost = 100000;
		//nodes[o].cost=nodes[o].cost+nodes[o].fcost;
	}
}

//----------------------------------------------------------------------------------
//! Checks the diameter of the structure at each step
//----------------------------------------------------------------------------------
bool World::Checkdiameter(unsigned coff, unsigned poff)
{
	bool largearea = false;

	int z = coff / (m_Width * m_Height);
	int y = (coff - (z * m_Width * m_Height)) / m_Width;
	int x = coff - (y * m_Width) - (z * m_Width * m_Height);

	int pz = poff / (m_Width * m_Height);
	int py = (poff - (pz * m_Width * m_Height)) / m_Width;
	int px = poff - (py * m_Width) - (pz * m_Width * m_Height);

	bool comput = false;

	int v1x = x - px;
	int v1y = y - py;
	int v1z = z - pz;
	int v2x = 0;
	int v2y = 0;
	int v2z = 0;

	if (v1z == 0)
	{
		v2z = 0;
		if (v1y == 0)
		{
			v2x = 0;
			v2y = 1;
		}
		else if (v1x == 0)
		{
			v2x = 1;
			v2y = 0;
		}
		else if (v1x == v1y)
		{
			v2x = v1x;
			v2y = -v1y;
		}
		else
		{
			v2x = v1y;
			v2y = v1y;
		}
	}
	else if (v1x == 0 && v1y == 0)
	{
		v2x = 0;
		v2y = 1;
		v2z = 0;
	}
	else if (v1x == 0)
	{
		v2x = 1;
		v2y = 0;
		v2z = 0;
	}
	else if (v1y == 0)
	{
		v2x = 0;
		v2y = 1;
		v2z = 0;
	}
	else if (v1x != 0 && v1y != 0)
	{
		v2z = 0;
		if (v1x == v1y)
		{
			v2x = v1x;
			v2y = -v1y;
		}
		else
		{
			v2x = v1y;
			v2y = v1y;
		}
	}
	else
		std::cout << "Unconsidered case!" << std::endl;

	int nx = x;
	int ny = y;
	int nz = z;

	int diameter1 = 0;
	int diameter2 = 0;
	int diameter3 = 0;
	int diameter4 = 0;

	do
	{
		unsigned o = nx + ny * m_Width + nz * m_Width * m_Height;
		if (nx > -1 && ny > -1 && nz > -1 && nx < m_Width && ny < m_Height &&
				nz < m_Length)
		{
			//std::cout << "Diameter node: " << "x:" << nx << " y:" << ny << " z:" << nz << " o: " << o << std::endl;
			if (m_Nodes[o].m_Intens > 1150)
			{
				diameter1++;
				comput = true;
				nx = nx + v2x;
				ny = ny + v2y;
				nz = nz + v2z;
			}
			else
			{
				comput = false;
			}
		}
		else
			comput = false;
	} while (comput);

	nx = x;
	ny = y;
	nz = z;

	do
	{
		unsigned o = nx + ny * m_Width + nz * m_Width * m_Height;
		if (nx > -1 && ny > -1 && nz > -1 && nx < m_Width && ny < m_Height &&
				nz < m_Length)
		{
			//std::cout << "Diameter node: " << "x:" << nx << " y:" << ny << " z:" << nz << " o: " << o << std::endl;
			if (m_Nodes[o].m_Intens > 1150)
			{
				diameter2++;
				comput = true;
				nx = nx - v2x;
				ny = ny - v2y;
				nz = nz - v2z;
			}
			else
			{
				comput = false;
			}
		}
		else
			comput = false;
	} while (comput);

	if (diameter1 > 15 && diameter2 > 15)
	{
		int nxf = nx;
		int nyf = ny;
		int nzf = nz;

		//std::cout << "Diameter: " << diameter << " Comput: " << comput << std::endl;
		//std::cout << "Diameter node: " << "x:" << x << " y:" << y << " z:" << z << std::endl;
		//std::cout << "Current node: " << "x:" << x << " y:" << y << " z:" << z << " o: " << coff << std::endl;
		//std::cout << "Previous node: " << "x:" << px << " y:" << py << " z:" << pz << " o: " << poff << std::endl;
		//std::cout << "Difference: " << "x:" << cx << " y:" << cy << " z:" << cz << std::endl;

		int nv2x = v1x;
		int nv2y = v1y;
		int nv2z = v1z;

		nx = x;
		ny = y;
		nz = z;

		do
		{
			unsigned o2 = nx + ny * m_Width + nz * m_Width * m_Height;
			if (nx > -1 && ny > -1 && nz > -1 && nx < m_Width && ny < m_Height &&
					nz < m_Length)
			{
				//std::cout << "Diameter node: " << "x:" << nx << " y:" << ny << " z:" << nz << " o: " << o << std::endl;
				if (m_Nodes[o2].m_Intens > 1150)
				{
					diameter3++;
					comput = true;
					nx = nx + nv2x;
					ny = ny + nv2y;
					nz = nz + nv2z;
				}
				else
				{
					comput = false;
				}
			}
			else
				comput = false;
		} while (comput);

		nx = x;
		ny = y;
		nz = z;

		do
		{
			unsigned o2 = nx + ny * m_Width + nz * m_Width * m_Height;
			if (nx > -1 && ny > -1 && nz > -1 && nx < m_Width && ny < m_Height &&
					nz < m_Length)
			{
				//std::cout << "Diameter node: " << "x:" << nx << " y:" << ny << " z:" << nz << " o: " << o << std::endl;
				if (m_Nodes[o2].m_Intens > 1150)
				{
					diameter4++;
					comput = true;
					nx = nx - nv2x;
					ny = ny - nv2y;
					nz = nz - nv2z;
				}
				else
				{
					comput = false;
				}
			}
			else
				comput = false;
		} while (comput);

		if (diameter3 > 15 && diameter4 > 15)
		{
			//std::cout << "Diameter: " << diameter << " Comput: " << comput << std::endl;
			//std::cout << "Diameter 2 node: " << "x:" << nx << " y:" << ny << " z:" << nz << std::endl;
			//std::cout << "Current node: " << "x:" << x << " y:" << y << " z:" << z << " o: " << coff << std::endl;
			//std::cout << "Previous node: " << "x:" << px << " y:" << py << " z:" << pz << " o: " << poff << std::endl;
			//std::cout << "Difference: " << "x:" << cx << " y:" << cy << " z:" << cz << std::endl;
			//std::cout << "New difference: " << "x:" << ncx << " y:" << ncy << " z:" << ncz << std::endl;
			//std::cout << "LARGE AREA FOUND" << std::endl;
			//std::cout << "1: " << diameter1 << " 2: " << diameter2 << " 3: " << diameter3 << " 4: " << diameter4 << std::endl;
			largearea = true;
			//zarea=nz;

			//if(times<1){

			for (int i = 0; i < diameter1 + diameter2; i++)
			{
				Point p;
				p.px = (short)(nxf + i * v2x + m_Offx);
				p.py = (short)(nyf + i * v2y + m_Offy);
				unsigned short slicenr = (unsigned short)nzf + i * v2z + m_Offz;
				m_Handler3D->SetWorkPt(p, slicenr, 4000);
				//if(nzf+offz==75) std::cout << "v2: " << "x:" << v2x << " y:" << v2y << " z:" << v2z << std::endl;
			}

			for (int j = 0; j < diameter3 + diameter4; j++)
			{
				Point p;
				p.px = (short)(nx + j * nv2x + m_Offx);
				p.py = (short)(ny + j * nv2y + m_Offy);
				unsigned short slicenr = (unsigned short)nz + j * nv2z + m_Offz;
				m_Handler3D->SetWorkPt(p, slicenr, 4000);
				//if(nz+offz==75) std::cout << "nv2: " << "x:" << nv2x << " ny:" << nv2y << " nz:" << nv2z << std::endl;
			}
			//}
		}
	}
	return (largearea);
}
//----------------------------------------------------------------------------------
//! Deals when the situation when a large structure is found
//	Returns the offset of the node that is going to be expanded (a new node selected trying to skip the large structure)
//----------------------------------------------------------------------------------
unsigned World::Solvelargearea2()
{
	using li = std::list<Node*>::iterator;
	unsigned newoff;
	unsigned off;
	Node* e;
	li ie, todel;
	int min = -1;

	if (m_Solvingarea == false)
	{
		m_Activelistlowint.clear();
		for (ie = m_Activelist.begin(); ie != m_Activelist.end(); ++ie)
		{
			e = *ie;
			off = e->m_Offset;
			if (e->m_Intens > 1000 && e->m_Intens <= 1150)
			{
				m_Nodes[off].m_Cost = m_Nodes[off].m_Cost - m_Nodes[off].m_Fcost + 10;
				m_Nodes[off].m_Fcost = 10;
				if (m_Nodes[off].m_Computed == true)
					std::cout << "Node already computed!!" << std::endl;
				m_Activelistlowint.push_back(&m_Nodes[off]);
			}
			else
			{
				//todel=iu;
				//nodes[off].cost=nodes[off].cost+10000000;
				if (m_Nodes[off].m_First == false)
				{
					m_Nodes[off].m_Computed = true;
					//vOutVol->setVoxel(ml::Vector(x+offx, y+offy, z+offz, 0, 0, 0), 4000);
				}
			}
		}
		m_Solvingarea = true;
		m_Activelist.clear();

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
		m_Activelist.clear();
		m_Activelistlowintsize = m_Activelistlowint.size();

		for (li ii = m_Activelistlowint.begin(); ii != m_Activelistlowint.end();
				 ++ii)
		{
			Node* e = *ii;
			if (min == -1 || min > e->m_Cost)
			{
				min = (int)e->m_Cost;
				newoff = e->m_Offset;
				todel = ii;
			}
		}

		m_Activelistlowint.erase(todel);
		m_Activelist = m_Activelistlowint;
		return (newoff);
	}
	else
	{
		//std::cout << "Activelistlowint size: " << activelistlowint.size() << std::endl;
		if (m_Activelistlowint.empty())
		{
			std::cout << "Activelistlowint empty" << std::endl;
		}

		for (li ii = m_Activelistlowint.begin(); ii != m_Activelistlowint.end();
				 ++ii)
		{
			e = *ii;
			//unsigned o8=e->offset;
			//int z8 = o8 / (width * height);
			//int y8 = (o8 - (z8 * width * height)) / width;
			//int x8 = o8 - (y8 * width) - (z8 * width * height);
			//std::cout << "NODE: " << "nx:" << x8 << " ny:" << y8 << " nz:" << z8 << " Computed: " << nodes[o8].computed << std::endl;
			//vOutVol->setVoxel(ml::Vector(x8+offx, y8+offy, z8+offz, 0, 0, 0), 4000);

			if (min == -1 || min > e->m_Cost)
			{
				min = (int)e->m_Cost;
				newoff = e->m_Offset;
				todel = ii;
			}
		}

		m_Activelistlowint.erase(todel);
		//std::cout << "Activelistsizelowint: " << activelistlowint.size() << std::endl;

		bool found = false;
		li p = m_Activelist.begin();
		while (!found && p != m_Activelist.end())
		{
			Node* e = *p;
			if (e->m_Offset == newoff)
			{
				found = true;
				todel = p;
			}
			p++;
		}
		m_Activelist.erase(todel);

		//std::cout << "Newoff: " << newoff << std::endl;
		//std::cout << "Largeareatimes: " << largeareatimes << std::endl;
		return (newoff);
	}
}
//----------------------------------------------------------------------------------
//! Diminish cost of dark nodes previously found when a large structure is found
//----------------------------------------------------------------------------------
void World::Solvelargearea()
{
	int total = m_Width * m_Height * m_Length;
	std::cout << "Solvelargearea1" << std::endl;

	for (int o = 0; o < total; o++)
	{
		if (m_Nodes[o].m_Computed == true)
		{
			if (m_Nodes[o].m_Intens > 1000 && m_Nodes[o].m_Intens < 1150)
			{
				m_Nodes[o].m_Cost = m_Nodes[o].m_Cost - m_Nodes[o].m_Fcost;
				m_Nodes[o].m_Fcost = 20;
			}
		}
	}
}
//----------------------------------------------------------------------------------
//! Prints the hierarchical information of the tree
//----------------------------------------------------------------------------------
// ESRA
void World::OutputBranchTree(BranchItem* branchItem, std::string prefix, FILE*& fp)

{
	fprintf(fp, "%s%u ( ", prefix.c_str(), branchItem->GetLabel());
	//std::cout << prefix << branchItem->getLabel() << " ( ";

	prefix.append("----");

	if (branchItem->GetParent() == nullptr)
	{
		fprintf(fp, "nullptr )\n");
		//std::cout << "nullptr )" << std::endl;
	}
	else
	{
		fprintf(fp, "%u )\n", branchItem->GetParent()->GetLabel());
		//std::cout << branchItem->getParent()->getLabel() << " )" << std::endl;
	}

	std::vector<Vec3>* center = branchItem->GetCenterList();
	std::vector<Vec3>::iterator iter1;

	fprintf(fp, "size(%i):", static_cast<int>(center->size()));
	for (iter1 = center->begin(); iter1 != center->end(); iter1++)
	{
		fprintf(fp, "(%f %f %f) ", (*iter1)[0], (*iter1)[1], (*iter1)[2]);
	}
	fprintf(fp, "\n");

	std::list<BranchItem*>* children = branchItem->GetChildren();
	std::list<BranchItem*>::iterator iter;

	for (iter = children->begin(); iter != children->end(); iter++)
	{
		BranchItem* item = *iter;
		OutputBranchTree(item, prefix, fp);
	}
}

void World::Set3dslicehandler(SlicesHandler* handler3D)
{
	m_Handler3D = handler3D;
}

Node::Node(unsigned offset, float intens)
{
	this->m_Offset = offset;
	this->m_Intens = intens;

	m_Computed = false;
	m_First = false;
}


PathElement::PathElement(unsigned offset)
{
	this->m_Offset = offset;

	m_Cross = false;
	m_Seed = false;
}


void World::GetSeedBoundingBox(std::vector<Vec3>* seeds, Vec3& bbStart, Vec3& bbEnd, SlicesHandler* handler3D)
{
	const long memory_max_voxels = 18060000;
	bbStart = Vec3(10000, 10000, 10000);
	bbEnd = Vec3(0, 0, 0);
	int inc_voi = 2;
	int margin = 15;

	// compute bounding box for seeds
	for (size_t i = 0; i < seeds->size(); i++)
	{
		Vec3 pos_vox = seeds->at(i);
		for (int j = 0; j < 3; j++)
		{
			if (pos_vox[j] < bbStart[j])
			{
				bbStart[j] = pos_vox[j];
			}
			if (pos_vox[j] > bbEnd[j])
			{
				bbEnd[j] = pos_vox[j];
			}
		}
	}

	// add margin
	Vec3 img_ext((float)handler3D->Width(), (float)handler3D->Height(), (float)handler3D->NumSlices());
	//	Vector imgExt = getInImg(0)->getImgExt();

	for (int j = 0; j < 3; j++)
	{
		if (bbStart[j] > margin)
		{
			bbStart[j] -= margin;
		}
		else
		{
			bbStart[j] = 0;
		}

		if (bbEnd[j] + margin < img_ext[j] - 1)
		{
			bbEnd[j] += margin;
		}
		else
		{
			bbEnd[j] = img_ext[j] - 1;
		}
	}

	int remaining = int((bbEnd[2] - bbStart[2]) * (bbEnd[1] - bbStart[1]) *
											(bbEnd[0] - bbStart[0]));
	remaining = memory_max_voxels - remaining;
	remaining /= int(bbEnd[2] - bbStart[2] + 1);

	bool is_stop = false;

	if (remaining <= 0)
	{
		is_stop = true;
	}

	while (!is_stop)
	{
		bbEnd[0] += inc_voi;
		bbEnd[1] += inc_voi;
		bbStart[0] -= inc_voi;
		bbStart[1] -= inc_voi;

		if (bbEnd[0] > img_ext[0] - 1)
		{
			bbEnd[0] = img_ext[0] - 1;
		}
		if (bbEnd[1] > img_ext[1] - 1)
		{
			bbEnd[1] = img_ext[1] - 1;
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
		remaining = int((bbEnd[2] - bbStart[2]) * (bbEnd[1] - bbStart[1]) *
										(bbEnd[0] - bbStart[0]));
		remaining = memory_max_voxels - remaining;

		if ((bbStart[0] == 0) && (bbStart[1] == 0) &&
				(bbEnd[0] == img_ext[0] - 1) && (bbEnd[1] == img_ext[1] - 1))
		{
			is_stop = true;
		}

		if (remaining < 0)
		{
			is_stop = true;
		}
	}

	bbStart[0] += inc_voi;
	bbStart[1] += inc_voi;
	bbEnd[0] -= inc_voi;
	bbEnd[1] -= inc_voi;

	// check if extension in z-direction is possible with remaining number of voxels
	if (remaining > 0)
	{
		int num_per_slice =
				int((bbEnd[0] - bbStart[0]) * (bbEnd[1] - bbStart[1]));
		int num_slices = remaining / num_per_slice;

		// do not consider the fact, that numSlices might be odd
		int ext = num_slices / 2;
		bbStart[2] -= ext;
		bbEnd[2] += ext;

		if (bbStart[2] < 0)
		{
			bbStart[2] = 0;
		}
		if (bbEnd[2] > img_ext[2] - 1)
		{
			bbEnd[2] = img_ext[2] - 1;
		}
	}
}

} // namespace iseg
