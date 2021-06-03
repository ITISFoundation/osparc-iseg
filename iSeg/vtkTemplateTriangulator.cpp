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

#include "MeshingCommon.h"
#include "vtkTemplateTriangulator.h"

#include <cassert>
#include <map>

namespace {
// order convention for tetrahedron edges
short tet_edges[6][2] = {{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}};

// order convention for tetrahedron faces
short tet_triangles[4][3] = {{1, 2, 3}, {0, 3, 2}, {0, 1, 3}, {0, 2, 1}};

// this corresponds to the convention used by vtkHexahedron
short cube_faces[6][4] = {{0, 4, 7, 3}, {1, 2, 6, 5}, {0, 1, 5, 4}, {3, 7, 6, 2}, {0, 3, 2, 1}, {4, 5, 6, 7}};

// needed for DetermineTetraSubdivisionCase
short node_trafo[64][4] = {
		0, 1, 2, 3, 0, 1, 2, 3, 2, 0, 1, 3, 0, 1, 2, 3, 3, 0, 2, 1, 0, 3, 1, 2, 0, 2, 3, 1, 0, 1, 2, 3, 1, 2, 0, 3, 1, 2, 0, 3, 2, 0, 1, 3, 0, 1, 2, 3, 1, 2, 0, 3, 0, 1, 3, 2, 2, 0, 1, 3, 0, 1, 2, 3, 1, 3, 2, 0, 1, 0, 3, 2, 2, 0, 1, 3, 0, 1, 2, 3, 3, 1, 0, 2, 0, 3, 1, 2, 3, 0, 1, 2, 0, 3, 1, 2, 1, 3, 2, 0, 1, 2, 0, 3, 1, 2, 3, 0, 1, 2, 0, 3, 1, 3, 2, 0, 1, 0, 3, 2, 2, 0, 1, 3, 0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 2, 3, 0, 1, 2, 0, 3, 1, 3, 0, 2, 1, 3, 0, 2, 1, 0, 2, 3, 1, 0, 2, 3, 1, 2, 1, 3, 0, 1, 2, 0, 3, 2, 0, 1, 3, 2, 0, 1, 3, 3, 2, 0, 1, 1, 2, 0, 3, 2, 3, 0, 1, 2, 0, 1, 3, 3, 2, 1, 0, 1, 3, 0, 2, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0, 3, 1, 0, 2, 3, 0, 2, 1, 3, 0, 2, 1, 1, 3, 2, 0, 1, 3, 2, 0, 2, 1, 3, 0, 1, 2, 0, 3, 3, 2, 1, 0, 1, 3, 2, 0, 3, 2, 1, 0, 0, 1, 2, 3};

// needed for DetermineTetraSubdivisionCase
short edge_trafo[64][6] = {
		0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 1, 3, 5, 0, 2, 4, 0, 1, 2, 3, 4, 5, 2, 5, 4, 1, 0, 3, 2, 0, 1, 4, 5, 3, 1, 2, 0, 5, 3, 4, 0, 1, 2, 3, 4, 5, 3, 0, 4, 1, 5, 2, 3, 0, 4, 1, 5, 2, 1, 3, 5, 0, 2, 4, 0, 1, 2, 3, 4, 5, 3, 0, 4, 1, 5, 2, 0, 2, 1, 4, 3, 5, 1, 3, 5, 0, 2, 4, 0, 1, 2, 3, 4, 5, 4, 3, 0, 5, 2, 1, 0, 4, 3, 2, 1, 5, 1, 3, 5, 0, 2, 4, 0, 1, 2, 3, 4, 5, 4, 2, 5, 0, 3, 1, 2, 0, 1, 4, 5, 3, 2, 4, 5, 0, 1, 3, 2, 0, 1, 4, 5, 3, 4, 3, 0, 5, 2, 1, 3, 0, 4, 1, 5, 2, 3, 4, 0, 5, 1, 2, 3, 0, 4, 1, 5, 2, 4, 3, 0, 5, 2, 1, 0, 4, 3, 2, 1, 5, 1, 3, 5, 0, 2, 4, 0, 1, 2, 3, 4, 5, 5, 4, 2, 3, 1, 0, 0, 1, 2, 3, 4, 5, 5, 1, 3, 2, 4, 0, 1, 5, 3, 2, 0, 4, 2, 5, 4, 1, 0, 3, 2, 5, 4, 1, 0, 3, 1, 2, 0, 5, 3, 4, 1, 2, 0, 5, 3, 4, 3, 5, 1, 4, 0, 2, 3, 0, 4, 1, 5, 2, 1, 3, 5, 0, 2, 4, 1, 3, 5, 0, 2, 4, 5, 2, 4, 1, 3, 0, 3, 0, 4, 1, 5, 2, 5, 1, 3, 2, 4, 0, 1, 3, 5, 0, 2, 4, 5, 4, 2, 3, 1, 0, 4, 0, 3, 2, 5, 1, 5, 4, 2, 3, 1, 0, 0, 1, 2, 3, 4, 5, 5, 4, 2, 3, 1, 0, 4, 2, 5, 0, 3, 1, 2, 5, 4, 1, 0, 3, 2, 5, 4, 1, 0, 3, 4, 3, 0, 5, 2, 1, 4, 3, 0, 5, 2, 1, 3, 5, 1, 4, 0, 2, 3, 0, 4, 1, 5, 2, 5, 4, 2, 3, 1, 0, 4, 3, 0, 5, 2, 1, 5, 4, 2, 3, 1, 0, 0, 1, 2, 3, 4, 5};

short tri_trafo[64][4] = {
		0, 1, 2, 3, 0, 1, 2, 3, 2, 0, 1, 3, 0, 1, 2, 3, 3, 0, 2, 1, 0, 3, 1, 2, 0, 2, 3, 1, 0, 1, 2, 3, 1, 2, 0, 3, 1, 2, 0, 3, 2, 0, 1, 3, 0, 1, 2, 3, 1, 2, 0, 3, 0, 1, 3, 2, 2, 0, 1, 3, 0, 1, 2, 3, 1, 3, 2, 0, 1, 0, 3, 2, 2, 0, 1, 3, 0, 1, 2, 3, 3, 1, 0, 2, 0, 3, 1, 2, 3, 0, 1, 2, 0, 3, 1, 2, 1, 3, 2, 0, 1, 2, 0, 3, 1, 2, 3, 0, 1, 2, 0, 3, 1, 3, 2, 0, 1, 0, 3, 2, 2, 0, 1, 3, 0, 1, 2, 3, 3, 2, 1, 0, 0, 1, 2, 3, 2, 3, 0, 1, 2, 0, 3, 1, 3, 0, 2, 1, 3, 0, 2, 1, 0, 2, 3, 1, 0, 2, 3, 1, 2, 1, 3, 0, 1, 2, 0, 3, 2, 0, 1, 3, 2, 0, 1, 3, 3, 2, 0, 1, 1, 2, 0, 3, 2, 3, 0, 1, 2, 0, 1, 3, 3, 2, 1, 0, 1, 3, 0, 2, 3, 2, 1, 0, 0, 1, 2, 3, 3, 2, 1, 0, 3, 1, 0, 2, 3, 0, 2, 1, 3, 0, 2, 1, 1, 3, 2, 0, 1, 3, 2, 0, 2, 1, 3, 0, 1, 2, 0, 3, 3, 2, 1, 0, 1, 3, 2, 0, 3, 2, 1, 0, 0, 1, 2, 3};

// needed for DetermineTetraSubdivisionCase
short casenr[64] = {
		10,
		0,
		0,
		1,
		0,
		1,
		1,
		3,
		0,
		1,
		1,
		4,
		2,
		-5,
		5,
		7,
		0,
		1,
		2,
		5,
		1,
		4,
		-5,
		7,
		1,
		3,
		-5,
		7,
		5,
		7,
		6,
		8,
		0,
		2,
		1,
		-5,
		1,
		5,
		4,
		7,
		1,
		5,
		3,
		7,
		-5,
		6,
		7,
		8,
		1,
		-5,
		5,
		6,
		3,
		7,
		7,
		8,
		4,
		7,
		7,
		8,
		7,
		8,
		8,
		9,
};

} // namespace

/**	Pointer to implementation class, which stores the maps to make sure 
	edge and triangle points are created only once.
*/
class vtkTemplateTriangulator::vtkPimple
{
public:
	using triangle_type = Meshing::TriangleType<vtkIdType>;
	using TriangleNodeMap = std::map<triangle_type, vtkIdType>;

	using edge_type = Meshing::SegmentType<vtkIdType>;
	using EdgeNodeMap = std::map<edge_type, vtkIdType>;

	EdgeNodeMap m_EdgeTable;
	TriangleNodeMap m_TriangleTable;
};

//////////////////////////////////////////////////////////////////////////
vtkTemplateTriangulator::vtkTemplateTriangulator() { m_Pimple = new vtkPimple; }

vtkTemplateTriangulator::~vtkTemplateTriangulator() { delete m_Pimple; }

void vtkTemplateTriangulator::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);
}

int vtkTemplateTriangulator::AddPyramid(vtkIdType v1, vtkIdType v2, vtkIdType v3, vtkIdType v4, vtkIdType v5, int dom)
{
	int num_tets = 2;
	bool look_up_table = (std::max(v1, v3) < std::max(v2, v4));

	// diagonal v1,v3
	if (look_up_table)
	{
		//v1v2v3v5
		AddTetrahedron(v1, v2, v3, v5, dom);

		//v1v3v4v5
		AddTetrahedron(v1, v3, v4, v5, dom);
	}
	// diagonal v2,v4
	else
	{
		//v2v3v4v5
		AddTetrahedron(v2, v3, v4, v5, dom);

		//v2v4v1v5
		AddTetrahedron(v1, v2, v4, v5, dom);
	}

	return num_tets;
}

int vtkTemplateTriangulator::AddPrism(vtkIdType v1, vtkIdType v2, vtkIdType v3, vtkIdType v4, vtkIdType v5, vtkIdType v6, int dom)
{
	int num_tets = 0;

	bool look_up_table[] = {(std::max(v2, v4) < std::max(v1, v5)), (std::max(v1, v6) < std::max(v4, v3)), (std::max(v3, v5) < std::max(v2, v6))};
	/*
	bool lookUpTable[] = {
		(std::min(v2, v4) < std::min(v1, v5)),
		(std::min(v1, v6) < std::min(v4, v3)),
		(std::min(v3, v5) < std::min(v2, v6))
	};
	*/

	//CASE 0-3
	if (look_up_table[0])
	{
		if (look_up_table[1])
		{
			if (look_up_table[2])
			{
				// Add Steiner point
				double p1[3], p2[3], p3[3], p4[3], p5[3], p6[3];
				GetPoint(v1, p1);
				GetPoint(v2, p2);
				GetPoint(v3, p3);
				GetPoint(v4, p4);
				GetPoint(v5, p5);
				GetPoint(v6, p6);
				double pnew[3] = {
						(p1[0] + p2[0] + p3[0] + p4[0] + p5[0] + p6[0]) / 6.0, (p1[1] + p2[1] + p3[1] + p4[1] + p5[1] + p6[1]) / 6.0, (p1[2] + p2[2] + p3[2] + p4[2] + p5[2] + p6[2]) / 6.0};
				vtkIdType v7 = AddPoint(pnew);

				//v1v2v3v7
				AddTetrahedron(v1, v2, v3, v7, dom);

				//v1v2v4v7
				AddTetrahedron(v1, v2, v4, v7, dom);

				//v2v4v5v7
				AddTetrahedron(v2, v4, v5, v7, dom);

				//v2v3v5v7
				AddTetrahedron(v2, v3, v5, v7, dom);

				//v3v5v6v7
				AddTetrahedron(v3, v5, v6, v7, dom);

				//v1v3v6v7
				AddTetrahedron(v1, v3, v6, v7, dom);

				//v1v4v6v7
				AddTetrahedron(v1, v4, v6, v7, dom);

				//v4v5v6v7
				AddTetrahedron(v4, v5, v6, v7, dom);

				num_tets = 8;
			}
			else
			{
				//CASE 1:

				//v1v2v4v6
				AddTetrahedron(v1, v2, v6, v4, dom);

				//v4v6v2v5
				AddTetrahedron(v2, v4, v5, v6, dom);

				//v1v2v3v6
				AddTetrahedron(v1, v2, v3, v6, dom);

				num_tets = 3;
			}
		}
		else
		{
			if (look_up_table[2])
			{
				//CASE 2:
				//v1v2v3v4
				AddTetrahedron(v1, v2, v3, v4, dom);

				//v2v3v4v5
				AddTetrahedron(v2, v3, v4, v5, dom);

				//v4v5v6v3
				AddTetrahedron(v3, v4, v5, v6, dom);

				num_tets = 3;
			}
			else
			{
				//CASE 3:

				//v1v2v3v4
				AddTetrahedron(v1, v2, v3, v4, dom);

				//v4v5v6v2
				AddTetrahedron(v2, v4, v5, v6, dom);

				//v4v6v2v3
				AddTetrahedron(v2, v3, v4, v6, dom);

				num_tets = 3;
			}
		}
	}
	//CASE 4-7
	else
	{
		if (look_up_table[1])
		{
			if (look_up_table[2])
			{
				//CASE 4:
				//v1v4v5v6
				AddTetrahedron(v1, v4, v5, v6, dom);

				//v1v2v3v5
				AddTetrahedron(v1, v2, v3, v5, dom);

				//v1v3v5v6
				AddTetrahedron(v1, v3, v6, v5, dom);

				num_tets = 3;
			}
			else
			{
				//CASE 5:
				//v1v4v5v6
				AddTetrahedron(v1, v4, v5, v6, dom);

				//v1v2v5v6
				AddTetrahedron(v1, v2, v6, v5, dom);

				//v1v2v3v6
				AddTetrahedron(v1, v2, v3, v6, dom);

				num_tets = 3;
			}
		}
		else
		{
			if (look_up_table[2])
			{
				//CASE 6:
				//v1v3v4v5
				AddTetrahedron(v1, v3, v4, v5, dom);

				//v1v2v3v5
				AddTetrahedron(v1, v2, v3, v5, dom);

				//v4v5v6v3
				AddTetrahedron(v3, v4, v5, v6, dom);

				num_tets = 3;
			}
			else
			{
				//CASE 7:

				// Add Steiner point
				double p1[3], p2[3], p3[3], p4[3], p5[3], p6[3];
				GetPoint(v1, p1);
				GetPoint(v2, p2);
				GetPoint(v3, p3);
				GetPoint(v4, p4);
				GetPoint(v5, p5);
				GetPoint(v6, p6);
				double pnew[3] = {
						(p1[0] + p2[0] + p3[0] + p4[0] + p5[0] + p6[0]) / 6.0, (p1[1] + p2[1] + p3[1] + p4[1] + p5[1] + p6[1]) / 6.0, (p1[2] + p2[2] + p3[2] + p4[2] + p5[2] + p6[2]) / 6.0};
				vtkIdType v7 = AddPoint(pnew);

				//v1v2v3v7
				AddTetrahedron(v1, v2, v3, v7, dom);

				//v1v4v5v7
				AddTetrahedron(v1, v4, v5, v7, dom);

				//v1v2v5v7
				AddTetrahedron(v1, v2, v5, v7, dom);

				//v2v5v6v7
				AddTetrahedron(v1, v5, v6, v7, dom);

				//v2v3v6v7
				AddTetrahedron(v2, v3, v6, v7, dom);

				//v3v4v6v7
				AddTetrahedron(v3, v4, v6, v7, dom);

				//v1v3v4v7
				AddTetrahedron(v1, v3, v4, v7, dom);

				//v4v5v6v7
				AddTetrahedron(v4, v5, v6, v7, dom);

				num_tets = 8;
			}
		}
	}
	return num_tets;
}

int vtkTemplateTriangulator::AddHexahedron(vtkIdType v0, vtkIdType v1, vtkIdType v2, vtkIdType v3, vtkIdType v4, vtkIdType v5, vtkIdType v6, vtkIdType v7, int dom)
{
	vtkIdType vi[8] = {v0, v1, v2, v3, v4, v5, v6, v7};
	int num_tets = 0;
	bool lookup_table[6];
	for (int k = 0; k < 6; k++)
	{
		const short* face = cube_faces[k];

		// Note: lookupTable[k] is true if there is a diagonal edge
		// between cube_faces[k][0] and cube_faces[k][2]

		// diagonal never is connected to largest id (since this is the last added
		// in the point insertion Delaunay algorithm used by vtkOrderedTriangulator)
		lookup_table[k] = (std::max(vi[face[0]], vi[face[2]]) <
											 std::max(vi[face[1]], vi[face[3]]));

		/*
		// alternative definition:
		// diagonal is added starting from smallest id
		lookupTable[k] = (std::min(vi[face[0]], vi[face[2]]) < std::min(vi[face[1]], vi[face[3]]));
		*/
	}

	if (!lookup_table[0])
	{
		if (!lookup_table[1])
		{
			if (!lookup_table[2])
			{
				if (!lookup_table[3])
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 0:
							AddTetrahedron(vi[1], vi[2], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[2], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[2], vi[7], dom);
							AddTetrahedron(vi[4], vi[5], vi[2], vi[7], dom);
							num_tets = 6;
						}
						else
						{
							// case 32:
							AddTetrahedron(vi[1], vi[4], vi[2], vi[3], dom);
							AddTetrahedron(vi[1], vi[4], vi[3], vi[0], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[4], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[2], vi[7], dom);
							AddTetrahedron(vi[4], vi[7], vi[2], vi[3], dom);
							num_tets = 6;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 16:
							AddTetrahedron(vi[1], vi[2], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[2], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[2], vi[7], dom);
							AddTetrahedron(vi[4], vi[5], vi[2], vi[7], dom);
							num_tets = 6;
						}
						else
						{
							// case 48:
							assert(0 && "This case (48) should not exist!");
						}
					}
				}
				else
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 8:
							AddTetrahedron(vi[1], vi[4], vi[3], vi[0], dom);
							AddTetrahedron(vi[1], vi[5], vi[3], vi[4], dom);
							AddTetrahedron(vi[2], vi[3], vi[1], vi[5], dom);
							AddTetrahedron(vi[3], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[5], vi[6], vi[3], vi[7], dom);
							AddTetrahedron(vi[5], vi[7], vi[3], vi[4], dom);
							num_tets = 6;
						}
						else
						{
							// case 40:
							AddTetrahedron(vi[1], vi[2], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[3], vi[4], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[3], vi[7], dom);
							num_tets = 6;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 24:
							assert(0 && "This case (24) should not exist!");
						}
						else
						{
							// case 56:
							AddTetrahedron(vi[1], vi[2], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[3], vi[4], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[3], vi[7], dom);
							num_tets = 6;
						}
					}
				}
			}
			else
			{
				if (!lookup_table[3])
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 4:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[2], vi[3], vi[5], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[5], vi[7], dom);
							AddTetrahedron(vi[3], vi[5], vi[2], vi[7], dom);
							num_tets = 6;
						}
						else
						{
							// case 36:
							assert(0 && "This case (36) should not exist!");
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 20:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[5], vi[7], dom);
							AddTetrahedron(vi[3], vi[5], vi[2], vi[7], dom);
							num_tets = 6;
						}
						else
						{
							// case 52:
							AddTetrahedron(vi[2], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[5], vi[0], vi[1], dom);
							AddTetrahedron(vi[2], vi[6], vi[4], vi[5], dom);
							AddTetrahedron(vi[2], vi[7], vi[4], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[2], vi[7], dom);
							num_tets = 6;
						}
					}
				}
				else
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 12:
							assert(0 && "This case (12) should not exist!");
						}
						else
						{
							// case 44:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[2], vi[3], vi[5], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[3], vi[4], vi[5], vi[6], dom);
							AddTetrahedron(vi[3], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[3], vi[7], dom);
							num_tets = 6;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 28:
							AddTetrahedron(vi[0], vi[2], vi[5], vi[1], dom);
							AddTetrahedron(vi[3], vi[4], vi[0], vi[5], dom);
							AddTetrahedron(vi[3], vi[5], vi[0], vi[2], dom);
							AddTetrahedron(vi[3], vi[6], vi[5], vi[2], dom);
							AddTetrahedron(vi[3], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[4], vi[5], vi[3], vi[7], dom);
							num_tets = 6;
						}
						else
						{
							// case 60:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[3], vi[4], vi[5], vi[6], dom);
							AddTetrahedron(vi[3], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[3], vi[7], dom);
							num_tets = 6;
						}
					}
				}
			}
		}
		else
		{
			if (!lookup_table[2])
			{
				if (!lookup_table[3])
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 2:
							AddTetrahedron(vi[1], vi[4], vi[3], vi[0], dom);
							AddTetrahedron(vi[1], vi[4], vi[7], vi[3], dom);
							AddTetrahedron(vi[1], vi[5], vi[6], vi[7], dom);
							AddTetrahedron(vi[1], vi[5], vi[7], vi[4], dom);
							AddTetrahedron(vi[1], vi[6], vi[2], vi[7], dom);
							AddTetrahedron(vi[1], vi[7], vi[2], vi[3], dom);
							num_tets = 6;
						}
						else
						{
							// case 34:
							AddTetrahedron(vi[1], vi[2], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[1], vi[6], vi[4], vi[5], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[6], dom);
							AddTetrahedron(vi[2], vi[7], vi[4], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[2], vi[7], dom);
							num_tets = 6;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 18:
							assert(0 && "This case (18) should not exist!");
						}
						else
						{
							// case 50:
							AddTetrahedron(vi[1], vi[2], vi[0], vi[4], dom);
							AddTetrahedron(vi[1], vi[6], vi[4], vi[5], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[6], dom);
							AddTetrahedron(vi[2], vi[7], vi[4], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[2], vi[7], dom);
							num_tets = 6;
						}
					}
				}
				else
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 10:
							AddTetrahedron(vi[1], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[3], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[3], vi[5], vi[1], vi[6], dom);
							AddTetrahedron(vi[3], vi[6], vi[1], vi[2], dom);
							AddTetrahedron(vi[3], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[4], vi[5], vi[3], vi[7], dom);
							num_tets = 6;
						}
						else
						{
							// case 42:
							AddTetrahedron(vi[3], vi[1], vi[4], vi[0], dom);
							AddTetrahedron(vi[4], vi[1], vi[6], vi[5], dom);
							AddTetrahedron(vi[4], vi[3], vi[6], vi[1], dom);
							AddTetrahedron(vi[6], vi[1], vi[3], vi[2], dom);
							AddTetrahedron(vi[6], vi[4], vi[7], vi[3], dom);
							num_tets = 5;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 26:
							assert(0 && "This case (26) should not exist!");
						}
						else
						{
							// case 58:
							AddTetrahedron(vi[1], vi[2], vi[0], vi[4], dom);
							AddTetrahedron(vi[1], vi[6], vi[4], vi[5], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[3], vi[7], dom);
							num_tets = 6;
						}
					}
				}
			}
			else
			{
				if (!lookup_table[3])
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 6:
							assert(0 && "This case (6) should not exist!");
						}
						else
						{
							// case 38:
							assert(0 && "This case (38) should not exist!");
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 22:
							assert(0 && "This case (22) should not exist!");
						}
						else
						{
							// case 54:
							assert(0 && "This case (54) should not exist!");
						}
					}
				}
				else
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 14:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[3], vi[4], vi[5], vi[7], dom);
							AddTetrahedron(vi[3], vi[5], vi[1], vi[6], dom);
							AddTetrahedron(vi[3], vi[6], vi[1], vi[2], dom);
							AddTetrahedron(vi[3], vi[7], vi[5], vi[6], dom);
							num_tets = 6;
						}
						else
						{
							// case 46:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[3], vi[4], vi[5], vi[6], dom);
							AddTetrahedron(vi[3], vi[5], vi[1], vi[6], dom);
							AddTetrahedron(vi[3], vi[6], vi[1], vi[2], dom);
							AddTetrahedron(vi[4], vi[6], vi[3], vi[7], dom);
							num_tets = 6;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 30:
							assert(0 && "This case (30) should not exist!");
						}
						else
						{
							// case 62:
							AddTetrahedron(vi[0], vi[5], vi[6], vi[4], dom);
							AddTetrahedron(vi[0], vi[6], vi[3], vi[4], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[6], dom);
							AddTetrahedron(vi[1], vi[6], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[6], dom);
							AddTetrahedron(vi[3], vi[4], vi[6], vi[7], dom);
							num_tets = 6;
						}
					}
				}
			}
		}
	}
	else
	{
		if (!lookup_table[1])
		{
			if (!lookup_table[2])
			{
				if (!lookup_table[3])
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 1:
							AddTetrahedron(vi[1], vi[2], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[7], dom);
							AddTetrahedron(vi[1], vi[7], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[7], vi[1], vi[5], dom);
							AddTetrahedron(vi[7], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[7], vi[5], vi[2], vi[6], dom);
							num_tets = 6;
						}
						else
						{
							// case 33:
							assert(0 && "This case (33) should not exist!");
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 17:
							AddTetrahedron(vi[0], vi[4], vi[2], vi[7], dom);
							AddTetrahedron(vi[0], vi[7], vi[2], vi[3], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[2], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[4], vi[5], vi[2], vi[7], dom);
							num_tets = 6;
						}
						else
						{
							// case 49:
							AddTetrahedron(vi[0], vi[4], vi[2], vi[7], dom);
							AddTetrahedron(vi[0], vi[7], vi[2], vi[3], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[4], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[2], vi[7], dom);
							num_tets = 6;
						}
					}
				}
				else
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 9:
							assert(0 && "This case (9) should not exist!");
						}
						else
						{
							// case 41:
							assert(0 && "This case (41) should not exist!");
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 25:
							assert(0 && "This case (25) should not exist!");
						}
						else
						{
							// case 57:
							assert(0 && "This case (57) should not exist!");
						}
					}
				}
			}
			else
			{
				if (!lookup_table[3])
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 5:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[2], vi[3], vi[5], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[3], vi[5], vi[2], vi[7], dom);
							AddTetrahedron(vi[5], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
						else
						{
							// case 37:
							assert(0 && "This case (37) should not exist!");
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 21:
							AddTetrahedron(vi[0], vi[2], vi[5], vi[1], dom);
							AddTetrahedron(vi[5], vi[0], vi[7], vi[2], dom);
							AddTetrahedron(vi[5], vi[2], vi[7], vi[6], dom);
							AddTetrahedron(vi[7], vi[0], vi[5], vi[4], dom);
							AddTetrahedron(vi[7], vi[2], vi[0], vi[3], dom);
							num_tets = 5;
						}
						else
						{
							// case 53:
							AddTetrahedron(vi[0], vi[4], vi[2], vi[7], dom);
							AddTetrahedron(vi[0], vi[5], vi[2], vi[4], dom);
							AddTetrahedron(vi[0], vi[7], vi[2], vi[3], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[4], vi[5], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[2], vi[7], dom);
							num_tets = 6;
						}
					}
				}
				else
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 13:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[2], vi[3], vi[5], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[3], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[5], vi[6], vi[3], vi[7], dom);
							AddTetrahedron(vi[5], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
						else
						{
							// case 45:
							assert(0 && "This case (45) should not exist!");
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 29:
							AddTetrahedron(vi[0], vi[5], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[5], dom);
							AddTetrahedron(vi[3], vi[5], vi[2], vi[6], dom);
							AddTetrahedron(vi[5], vi[6], vi[3], vi[7], dom);
							AddTetrahedron(vi[5], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
						else
						{
							// case 61:
							AddTetrahedron(vi[0], vi[6], vi[3], vi[7], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[6], dom);
							AddTetrahedron(vi[2], vi[5], vi[0], vi[1], dom);
							AddTetrahedron(vi[2], vi[6], vi[0], vi[5], dom);
							AddTetrahedron(vi[6], vi[4], vi[0], vi[5], dom);
							AddTetrahedron(vi[6], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
					}
				}
			}
		}
		else
		{
			if (!lookup_table[2])
			{
				if (!lookup_table[3])
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 3:
							assert(0 && "This case (3) should not exist!");
						}
						else
						{
							// case 35:
							AddTetrahedron(vi[1], vi[2], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[7], dom);
							AddTetrahedron(vi[1], vi[6], vi[4], vi[5], dom);
							AddTetrahedron(vi[1], vi[7], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[7], vi[1], vi[6], dom);
							AddTetrahedron(vi[7], vi[4], vi[1], vi[6], dom);
							num_tets = 6;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 19:
							AddTetrahedron(vi[0], vi[4], vi[1], vi[7], dom);
							AddTetrahedron(vi[0], vi[7], vi[1], vi[2], dom);
							AddTetrahedron(vi[0], vi[7], vi[2], vi[3], dom);
							AddTetrahedron(vi[1], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[4], vi[5], vi[1], vi[7], dom);
							AddTetrahedron(vi[7], vi[6], vi[1], vi[2], dom);
							num_tets = 6;
						}
						else
						{
							// case 51:
							AddTetrahedron(vi[0], vi[4], vi[2], vi[7], dom);
							AddTetrahedron(vi[0], vi[7], vi[2], vi[3], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[4], dom);
							AddTetrahedron(vi[1], vi[6], vi[4], vi[5], dom);
							AddTetrahedron(vi[2], vi[4], vi[1], vi[6], dom);
							AddTetrahedron(vi[4], vi[6], vi[2], vi[7], dom);
							num_tets = 6;
						}
					}
				}
				else
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 11:
							AddTetrahedron(vi[0], vi[4], vi[1], vi[7], dom);
							AddTetrahedron(vi[0], vi[7], vi[1], vi[3], dom);
							AddTetrahedron(vi[1], vi[3], vi[6], vi[2], dom);
							AddTetrahedron(vi[1], vi[3], vi[7], vi[6], dom);
							AddTetrahedron(vi[1], vi[7], vi[5], vi[6], dom);
							AddTetrahedron(vi[4], vi[5], vi[1], vi[7], dom);
							num_tets = 6;
						}
						else
						{
							// case 43:
							AddTetrahedron(vi[0], vi[6], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[2], vi[3], vi[6], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[6], dom);
							AddTetrahedron(vi[1], vi[6], vi[0], vi[4], dom);
							AddTetrahedron(vi[6], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[6], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 27:
							assert(0 && "This case (27) should not exist!");
						}
						else
						{
							// case 59:
							AddTetrahedron(vi[0], vi[6], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[6], dom);
							AddTetrahedron(vi[1], vi[6], vi[0], vi[4], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[6], dom);
							AddTetrahedron(vi[6], vi[4], vi[1], vi[5], dom);
							AddTetrahedron(vi[6], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
					}
				}
			}
			else
			{
				if (!lookup_table[3])
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 7:
							AddTetrahedron(vi[0], vi[5], vi[7], vi[4], dom);
							AddTetrahedron(vi[1], vi[2], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[7], dom);
							AddTetrahedron(vi[1], vi[6], vi[7], vi[5], dom);
							AddTetrahedron(vi[1], vi[7], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[7], vi[1], vi[6], dom);
							num_tets = 6;
						}
						else
						{
							// case 39:
							assert(0 && "This case (39) should not exist!");
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 23:
							AddTetrahedron(vi[0], vi[5], vi[6], vi[7], dom);
							AddTetrahedron(vi[0], vi[6], vi[2], vi[7], dom);
							AddTetrahedron(vi[0], vi[7], vi[2], vi[3], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[6], dom);
							AddTetrahedron(vi[1], vi[6], vi[0], vi[5], dom);
							AddTetrahedron(vi[5], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
						else
						{
							// case 55:
							AddTetrahedron(vi[0], vi[4], vi[5], vi[6], dom);
							AddTetrahedron(vi[0], vi[4], vi[6], vi[7], dom);
							AddTetrahedron(vi[0], vi[5], vi[1], vi[6], dom);
							AddTetrahedron(vi[0], vi[6], vi[1], vi[2], dom);
							AddTetrahedron(vi[0], vi[6], vi[2], vi[7], dom);
							AddTetrahedron(vi[0], vi[7], vi[2], vi[3], dom);
							num_tets = 6;
						}
					}
				}
				else
				{
					if (!lookup_table[4])
					{
						if (!lookup_table[5])
						{
							// case 15:
							AddTetrahedron(vi[0], vi[5], vi[6], vi[7], dom);
							AddTetrahedron(vi[0], vi[6], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[2], vi[3], vi[6], dom);
							AddTetrahedron(vi[1], vi[3], vi[0], vi[6], dom);
							AddTetrahedron(vi[1], vi[6], vi[0], vi[5], dom);
							AddTetrahedron(vi[5], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
						else
						{
							// case 47:
							AddTetrahedron(vi[0], vi[4], vi[5], vi[6], dom);
							AddTetrahedron(vi[0], vi[4], vi[6], vi[7], dom);
							AddTetrahedron(vi[0], vi[5], vi[1], vi[6], dom);
							AddTetrahedron(vi[0], vi[6], vi[1], vi[3], dom);
							AddTetrahedron(vi[0], vi[7], vi[6], vi[3], dom);
							AddTetrahedron(vi[1], vi[3], vi[6], vi[2], dom);
							num_tets = 6;
						}
					}
					else
					{
						if (!lookup_table[5])
						{
							// case 31:
							AddTetrahedron(vi[0], vi[5], vi[6], vi[7], dom);
							AddTetrahedron(vi[0], vi[6], vi[3], vi[7], dom);
							AddTetrahedron(vi[1], vi[2], vi[0], vi[6], dom);
							AddTetrahedron(vi[1], vi[6], vi[0], vi[5], dom);
							AddTetrahedron(vi[2], vi[3], vi[0], vi[6], dom);
							AddTetrahedron(vi[5], vi[7], vi[0], vi[4], dom);
							num_tets = 6;
						}
						else
						{
							// case 63:
							assert(0 && "This case (63) should not exist!");
						}
					}
				}
			}
		}
	}
	return num_tets;
}

short vtkTemplateTriangulator::DetermineTetraSubdivisionCase(short code[6], short edgeMap[6] /*= nullptr*/, short vertexMap[4] /*= nullptr*/, short triangleMap[4] /*= nullptr*/)
{
	int bitvalue = 0;
	int factor = 1;
	for (int i = 0; i < 6; i++)
	{
		bitvalue += code[i] * factor;
		factor *= 2;
	}

	if (edgeMap)
	{
		edgeMap[0] = edge_trafo[bitvalue][0];
		edgeMap[1] = edge_trafo[bitvalue][1];
		edgeMap[2] = edge_trafo[bitvalue][2];
		edgeMap[3] = edge_trafo[bitvalue][3];
		edgeMap[4] = edge_trafo[bitvalue][4];
		edgeMap[5] = edge_trafo[bitvalue][5];
	}

	if (vertexMap)
	{
		vertexMap[0] = node_trafo[bitvalue][0];
		vertexMap[1] = node_trafo[bitvalue][1];
		vertexMap[2] = node_trafo[bitvalue][2];
		vertexMap[3] = node_trafo[bitvalue][3];
	}
	if (triangleMap)
	{
		triangleMap[0] = tri_trafo[bitvalue][0];
		triangleMap[1] = tri_trafo[bitvalue][1];
		triangleMap[2] = tri_trafo[bitvalue][2];
		triangleMap[3] = tri_trafo[bitvalue][3];
	}

	return (casenr[bitvalue]);
}

void vtkTemplateTriangulator::AddMultipleDomainTetrahedron(const vtkIdType vi[4], const int domain[4])
{
	short code[6];
	vtkIdType edge_nodes[6];

	// create vertices on edges if necessary
	for (short j = 0; j < 6; j++)
	{
		code[j] = domain[tet_edges[j][0]] != domain[tet_edges[j][1]];
		edge_nodes[j] = -1;

		if (code[j])
		{
			vtkIdType n1 = vi[tet_edges[j][0]];
			vtkIdType n2 = vi[tet_edges[j][1]];
			assert(n1 != n2);

			vtkPimple::edge_type edge(n1, n2);
			vtkPimple::EdgeNodeMap::iterator edge_it =
					m_Pimple->m_EdgeTable.find(edge);
			if (edge_it == m_Pimple->m_EdgeTable.end())
			{
				double x1[3], x2[3];
				GetPoint(n1, x1);
				GetPoint(n2, x2);

				// create point and add to edge table
				vtkIdType new_id =
						AddPoint(0.5 * (x1[0] + x2[0]), 0.5 * (x1[1] + x2[1]), 0.5 * (x1[2] + x2[2]));

				m_Pimple->m_EdgeTable[edge] = new_id;
				edge_nodes[j] = new_id;
			}
			else
			{
				edge_nodes[j] = edge_it->second;
			}
		}
	}

	short edge_map[6];
	short vertex_map[4];
	short face_map[4];
	const short casenr =
			DetermineTetraSubdivisionCase(code, edge_map, vertex_map, face_map);

	// create vertices on triangles if necessary
	vtkIdType face_nodes[4] = {-1, -1, -1, -1};
	if (casenr == 8 || casenr == 9)
	{
		for (short j = 0; j < 4; j++)
		{
			if (domain[tet_triangles[j][0]] != domain[tet_triangles[j][1]] &&
					domain[tet_triangles[j][0]] != domain[tet_triangles[j][2]] &&
					domain[tet_triangles[j][1]] != domain[tet_triangles[j][2]])
			{
				vtkIdType n1 = vi[tet_triangles[j][0]];
				vtkIdType n2 = vi[tet_triangles[j][1]];
				vtkIdType n3 = vi[tet_triangles[j][2]];

				vtkPimple::triangle_type tri(n1, n2, n3);
				vtkPimple::TriangleNodeMap::iterator tri_it =
						m_Pimple->m_TriangleTable.find(tri);
				if (tri_it == m_Pimple->m_TriangleTable.end())
				{
					double x1[3], x2[3], x3[3];
					GetPoint(n1, x1);
					GetPoint(n2, x2);
					GetPoint(n3, x3);

					// create point and add to edge table
					vtkIdType new_id = AddPoint((x1[0] + x2[0] + x3[0]) / 3.0, (x1[1] + x2[1] + x3[1]) / 3.0, (x1[2] + x2[2] + x3[2]) / 3.0);

					m_Pimple->m_TriangleTable[tri] = new_id;
					face_nodes[j] = new_id;
				}
				else
				{
					face_nodes[j] = tri_it->second;
				}
			}
		}
	}

	// divide tetrahedron
	switch (casenr)
	{
	case 3: {
		assert(edge_nodes[edge_map[0]] >= 0);
		assert(edge_nodes[edge_map[1]] >= 0);
		assert(edge_nodes[edge_map[2]] >= 0);
		assert(edge_nodes[edge_map[3]] < 0);
		assert(edge_nodes[edge_map[4]] < 0);
		assert(edge_nodes[edge_map[5]] < 0);

		AddTetrahedron(edge_nodes[edge_map[0]], edge_nodes[edge_map[2]], edge_nodes[edge_map[1]], vi[vertex_map[0]], domain[vertex_map[0]]);

		AddPrism(edge_nodes[edge_map[0]], edge_nodes[edge_map[1]], edge_nodes[edge_map[2]], vi[vertex_map[1]], vi[vertex_map[2]], vi[vertex_map[3]], domain[vertex_map[1]]);
	}
	break;
	case 6: {
		assert(edge_nodes[edge_map[0]] >= 0);
		assert(edge_nodes[edge_map[1]] >= 0);
		assert(edge_nodes[edge_map[2]] < 0);
		assert(edge_nodes[edge_map[3]] < 0);
		assert(edge_nodes[edge_map[4]] >= 0);
		assert(edge_nodes[edge_map[5]] >= 0);
		assert(domain[vertex_map[0]] == domain[vertex_map[3]]);
		assert(domain[vertex_map[1]] == domain[vertex_map[2]]);

		AddPrism(vi[vertex_map[0]], edge_nodes[edge_map[0]], edge_nodes[edge_map[1]], vi[vertex_map[3]], edge_nodes[edge_map[4]], edge_nodes[edge_map[5]], domain[vertex_map[0]]);

		AddPrism(edge_nodes[edge_map[0]], edge_nodes[edge_map[4]], vi[vertex_map[1]], edge_nodes[edge_map[1]], edge_nodes[edge_map[5]], vi[vertex_map[2]], domain[vertex_map[1]]);
	}
	break;
	case 8: {
		assert(edge_nodes[edge_map[0]] >= 0);
		assert(edge_nodes[edge_map[1]] >= 0);
		assert(edge_nodes[edge_map[2]] >= 0);
		assert(edge_nodes[edge_map[3]] >= 0);
		assert(edge_nodes[edge_map[4]] >= 0);
		assert(edge_nodes[edge_map[5]] < 0);
		assert(domain[vertex_map[2]] == domain[vertex_map[3]]);

		AddHexahedron(vi[vertex_map[2]], edge_nodes[edge_map[1]], face_nodes[face_map[3]], edge_nodes[edge_map[3]], vi[vertex_map[3]], edge_nodes[edge_map[2]], face_nodes[face_map[2]], edge_nodes[edge_map[4]], domain[vertex_map[2]]);

		AddPrism(vi[vertex_map[1]], edge_nodes[edge_map[4]], edge_nodes[edge_map[3]], edge_nodes[edge_map[0]], face_nodes[face_map[2]], face_nodes[face_map[3]], domain[vertex_map[1]]);

		AddPrism(vi[vertex_map[0]], edge_nodes[edge_map[1]], edge_nodes[edge_map[2]], edge_nodes[edge_map[0]], face_nodes[face_map[3]], face_nodes[face_map[2]], domain[vertex_map[0]]);
	}
	break;
	case 9: {
		assert(edge_nodes[edge_map[0]] >= 0);
		assert(edge_nodes[edge_map[1]] >= 0);
		assert(edge_nodes[edge_map[2]] >= 0);
		assert(edge_nodes[edge_map[3]] >= 0);
		assert(edge_nodes[edge_map[4]] >= 0);
		assert(edge_nodes[edge_map[5]] >= 0);

		// add central point
		double x1[3], x2[3], x3[3], x4[3];
		GetPoint(vi[0], x1);
		GetPoint(vi[1], x2);
		GetPoint(vi[2], x3);
		GetPoint(vi[3], x4);
		vtkIdType c0 = AddPoint(0.25 * (x1[0] + x2[0] + x3[0] + x4[0]), 0.25 * (x1[1] + x2[1] + x3[1] + x4[1]), 0.25 * (x1[2] + x2[2] + x3[2] + x4[2]));

		// v0,e0,f3,e1, e2,f2,c0,f1
		AddHexahedron(vi[vertex_map[0]], edge_nodes[edge_map[0]], face_nodes[face_map[3]], edge_nodes[edge_map[1]], edge_nodes[edge_map[2]], face_nodes[face_map[2]], c0, face_nodes[face_map[1]], domain[vertex_map[0]]);

		// v1,e3,f3,e0, e4,f0,c0,f2
		AddHexahedron(vi[vertex_map[1]], edge_nodes[edge_map[3]], face_nodes[face_map[3]], edge_nodes[edge_map[0]], edge_nodes[edge_map[4]], face_nodes[face_map[0]], c0, face_nodes[face_map[2]], domain[vertex_map[1]]);

		// v2,e1,f3,e3, e5,f1,c0,f0
		AddHexahedron(vi[vertex_map[2]], edge_nodes[edge_map[1]], face_nodes[face_map[3]], edge_nodes[edge_map[3]], edge_nodes[edge_map[5]], face_nodes[face_map[1]], c0, face_nodes[face_map[0]], domain[vertex_map[2]]);

		// v3,e2,f1,e5, e4,f2,c0,f0
		AddHexahedron(vi[vertex_map[3]], edge_nodes[edge_map[2]], face_nodes[face_map[1]], edge_nodes[edge_map[5]], edge_nodes[edge_map[4]], face_nodes[face_map[2]], c0, face_nodes[face_map[0]], domain[vertex_map[3]]);
	}
	break;
	case 10: {
		AddTetrahedron(vi[0], vi[1], vi[2], vi[3], domain[0]);
	}
	break;
	default: assert(0);
	}
}
