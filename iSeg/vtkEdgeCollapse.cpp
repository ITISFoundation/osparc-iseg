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

#include "vtkEdgeCollapse.h"

#include "config.h"

#include <vtkCell.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDoubleArray.h>
#include <vtkEdgeTable.h>
#include <vtkGenericCell.h>
#include <vtkIdList.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkMath.h>
#include <vtkObjectFactory.h>
#include <vtkPointData.h>
#include <vtkPolyData.h>
#include <vtkPriorityQueue.h>
#include <vtkTriangle.h>

#include <vtkCellLocator.h>
#include <vtkFloatArray.h>
#include <vtkIntArray.h>
#include <vtkPlane.h>
#include <vtkPolygon.h>

#include <vtkSmartPointer.h>
#define vtkNew(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <algorithm>
#include <iterator>
#include <list>
#include <set>

#include "predicates.h"

struct Gp
{
	static double Exactinit()
	{
		static double v = ::exactinit();
		return v;
	}

	static double Getepsilon()
	{
		static double eps = ::getepsilon();
		return eps;
	}
	static double Orient3d(double a[3], double b[3], double c[3], double d[3])
	{
		return ::orient3d(a, b, d, c);
	}
};

//----------------------------------------------------------------------------
namespace MESH {
template<class TElem, class TValue = double>
struct SortElement
{
	SortElement(TElem e, TValue v) : m_Elem(e), m_Value(v) {}
	bool operator<(SortElement const& rhs) const { return m_Value < rhs.value; }
	TElem m_Elem;
	TValue m_Value;
};

class Triangle
{
public:
	Triangle(int i1, int i2, int i3, int lab = 0)
	{
		m_N1 = i1;
		m_N2 = i2;
		m_N3 = i3;
		m_Label = lab;
		m_IsValid = true;
	}
	bool operator<(const Triangle& rhs) const
	{
		int tri0[3] = {m_N1, m_N2, m_N3};
		int tri1[3] = {rhs.m_N1, rhs.m_N2, rhs.m_N3};
		std::sort(tri0, tri0 + 3);
		std::sort(tri1, tri1 + 3);
		if (tri0[0] != tri1[0])
			return (tri0[0] < tri1[0]);
		else if (tri0[1] != tri1[1])
			return (tri0[1] < tri1[1]);
		else
			return (tri0[2] < tri1[2]);
	}
	bool operator==(const Triangle& rhs) const
	{
		int tri0[3] = {m_N1, m_N2, m_N3};
		int tri1[3] = {rhs.m_N1, rhs.m_N2, rhs.m_N3};
		std::sort(tri0, tri0 + 3);
		std::sort(tri1, tri1 + 3);
		return (tri0[0] == tri1[0] && tri0[1] == tri1[1] && tri0[2] == tri1[2]);
	}
	friend std::ostream& operator<<(std::ostream& os, const Triangle& t)
	{
		os << t.m_N1 << " " << t.m_N2 << " " << t.m_N3;
		return os;
	}
	int m_N1, m_N2, m_N3;
	bool m_IsValid;
	int m_Label;
};

bool NoSelfIntersection(vtkPoints* mesh, std::vector<Triangle>& tris);
bool NoSelfIntersection(vtkPoints* mesh, std::vector<Triangle>& changedtris, std::vector<Triangle>& closetris);
} // namespace MESH

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkEdgeCollapse);

//----------------------------------------------------------------------------
vtkEdgeCollapse::vtkEdgeCollapse()
{
	// Objects used frequently or in multiple functions
	this->Edges = vtkEdgeTable::New();
	this->EdgeCosts = vtkPriorityQueue::New();
	this->EndPoint1List = vtkIdList::New();
	this->EndPoint2List = vtkIdList::New();
	this->CollapseCellIds = vtkIdList::New();
	this->Neighbors = vtkIdList::New();
	this->PointIds = vtkIdList::New();
	this->GCell = vtkGenericCell::New();
	this->CellLocator = vtkCellLocator::New();
	this->Normals = nullptr;
	this->Labels = nullptr;

	// Computed values which can be queried by the user
	this->NumberOfEdgeCollapses = 0;
	this->NumberOfEdgeFlips = 0;
	this->NumberOfEdgeDivisions = 0;
	this->ActualReduction = 0.0;
	this->InputIsNonmanifold = 0;

	// Default parameters
	this->MinimumEdgeLength = 0.0;
	this->MaximumNormalAngleDeviation = 15.0;
	this->FlipEdges = 0;
	this->UseMaximumEdgeLength = 0;
	this->MaximumEdgeLength = VTK_DOUBLE_MAX;
	this->MeshIsManifold = 0;
	this->DomainLabelName = nullptr;
	this->IntersectionCheckLevel = 2;
	this->NumberOfClosestPoints = 30; // currently not used
	this->Loud = 0;

	Gp::Exactinit();
}

//----------------------------------------------------------------------------
vtkEdgeCollapse::~vtkEdgeCollapse()
{
	this->Edges->Delete();
	this->EdgeCosts->Delete();
	this->EndPoint1List->Delete();
	this->EndPoint2List->Delete();
	this->CollapseCellIds->Delete();
	this->Neighbors->Delete();
	this->PointIds->Delete();
	this->GCell->Delete();
	this->CellLocator->Delete();
	if (this->Normals != nullptr)
	{
		this->Normals->Delete();
	}
	if (this->DomainLabelName != nullptr)
	{
		delete[] this->DomainLabelName;
	}
}

//----------------------------------------------------------------------------
int vtkEdgeCollapse::RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	// get the info objects
	vtkInformation* in_info = inputVector[0]->GetInformationObject(0);
	vtkInformation* out_info = outputVector->GetInformationObject(0);

	// get the input and output
	vtkPolyData* input =
			vtkPolyData::SafeDownCast(in_info->Get(vtkDataObject::DATA_OBJECT()));
	vtkPolyData* output =
			vtkPolyData::SafeDownCast(out_info->Get(vtkDataObject::DATA_OBJECT()));

	vtkIdType num_pts;
	vtkIdType num_tris;
	vtkIdType edge_id, i;
	int j;
	double cost;
	double x1[3], x2[3];
	vtkIdType end_pt_ids[2];
	vtkIdType npts, *pts;
	vtkIdType num_deleted_tris = 0;

	MinLength2 = this->MinimumEdgeLength * this->MinimumEdgeLength;
	NormalDotProductThreshold =
			std::cos(this->MaximumNormalAngleDeviation * 3.141592654f / 180.0);
	NumberOfEdgeCollapses = 0;
	NumberOfEdgeFlips = 0;

	// check some assumptions about the data
	if (input->GetPolys() == nullptr || input->GetPoints() == nullptr ||
			input->GetPointData() == nullptr || input->GetFieldData() == nullptr)
	{
		vtkErrorMacro("Nothing to decimate");
		return 1;
	}

	if (input->GetPolys()->GetMaxCellSize() > 3)
	{
		vtkErrorMacro("Can only decimate triangles");
		return 1;
	}

	// Copy input to working mesh
	this->Mesh = vtkPolyData::New();

	vtkPoints* points = vtkPoints::New();
	points->DeepCopy(input->GetPoints());
	this->Mesh->SetPoints(points);
	points->Delete();

	vtkCellArray* polys = vtkCellArray::New();
	this->Mesh->SetPolys(polys);
	polys->Delete();
	vtkIdList* ids = vtkIdList::New();
	ids->SetNumberOfIds(input->GetNumberOfCells());
	for (j = 0; j < input->GetNumberOfCells(); j++)
		ids->SetId(j, j);
	this->Mesh->CopyCells(input, ids);
	ids->Delete();

	//this->Mesh->GetFieldData()->PassData(input->GetFieldData());
	this->Mesh->BuildCells();
	this->Mesh->BuildLinks();

	// If duplicate triangles exist in input, map these to single triangle
	if (DomainLabelName != nullptr && strlen(DomainLabelName) > 0 &&
			input->GetCellData()->HasArray(DomainLabelName))
	{
		this->MapDuplicateTriangles(input->GetCellData()->GetArray(DomainLabelName));
		this->Labels = this->Mesh->GetCellData()->GetArray(DomainLabelName);
		assert(input->GetNumberOfCells() == this->Mesh->GetNumberOfCells());
		assert(this->Mesh->GetNumberOfCells() ==
					 this->Labels->GetNumberOfTuples());
	}
	else if (DomainLabelName != nullptr && strlen(DomainLabelName) > 0)
	{
		std::cerr << "WARNING: could not locate cell data array "
							<< DomainLabelName << std::endl;
	}
	num_tris = this->Mesh->GetNumberOfPolys();
	num_pts = this->Mesh->GetNumberOfPoints();
	this->UpdateProgress(0.1);

	// Compute edges and priority for each edge
	this->Edges->InitEdgeInsertion(num_pts, 1); // storing edge id as attribute
	this->EdgeCosts->Allocate(this->Mesh->GetPolys()->GetNumberOfCells() * 3);
	for (i = 0; i < this->Mesh->GetNumberOfCells(); i++)
	{
		if (this->Mesh->GetCellType(i) != VTK_TRIANGLE)
			continue;
		this->Mesh->GetCellPoints(i, npts, pts);

		for (j = 0; j < 3; j++)
		{
			if (this->Edges->IsEdge(pts[j], pts[(j + 1) % 3]) == -1)
			{
				// If this edge has not been processed, get an id for it, add it to
				// the edge list (Edges), and add its endpoints to the EndPoint1List
				// and EndPoint2List (the 2 endpoints to different lists).
				edge_id = this->Edges->GetNumberOfEdges();
				this->Edges->InsertEdge(pts[j], pts[(j + 1) % 3], edge_id);
				this->EndPoint1List->InsertId(edge_id, pts[j]);
				this->EndPoint2List->InsertId(edge_id, pts[(j + 1) % 3]);
			}
		}
	}

	// Compute the cost of and target point for collapsing each edge.
	for (i = 0; i < this->Edges->GetNumberOfEdges(); i++)
	{
		end_pt_ids[0] = this->EndPoint1List->GetId(i);
		end_pt_ids[1] = this->EndPoint2List->GetId(i);

		this->Mesh->GetPoint(end_pt_ids[0], x1);
		this->Mesh->GetPoint(end_pt_ids[1], x2);
		cost = vtkMath::Distance2BetweenPoints(x1, x2);
		if (cost < MinLength2)
		{
			this->EdgeCosts->Insert(cost, i);
		}
	}
	this->UpdateProgress(0.15);

	// Compute normals, used to detect how much surface changes after collapse
	this->ComputeNormals(input);

	// Compute number of triangles at each edge
	this->ComputeManifoldedness();

	this->UpdateProgress(0.2);

	// OK collapse edges until desired reduction is reached
	if (Loud)
		cout << "Starting edge collapse" << endl;
	int num_edges_to_collapse = this->EdgeCosts->GetNumberOfItems() * 0.5;
	int abort = 0, processed = 0;
	edge_id = this->EdgeCosts->Pop(0, cost);
	while (!abort && edge_id >= 0)
	{
		if (!(processed++ % 1000))
		{
			double myprogress =
					std::min(1.0, 0.25 + 0.75 * processed / num_edges_to_collapse);
			printf("\rProgress = %3f", myprogress);
			this->UpdateProgress(myprogress);
			abort = this->GetAbortExecute();
		}

		end_pt_ids[0] = this->EndPoint1List->GetId(edge_id);
		end_pt_ids[1] = this->EndPoint2List->GetId(edge_id);

		// Keep node endPtIds[0] (later remove unused node endPtIds[1])
		if (isboundary[end_pt_ids[1]])
			std::swap(end_pt_ids[0], end_pt_ids[1]);

		if (this->IsCollapseLegal(end_pt_ids[0], end_pt_ids[1]))
		{
			this->NumberOfEdgeCollapses++;

			this->UpdateEdgeData(end_pt_ids[0], end_pt_ids[1]);

			// Update the output triangles.
			num_deleted_tris += this->CollapseEdge(end_pt_ids[0], end_pt_ids[1]);
			this->ActualReduction = (double)num_deleted_tris / num_tris;
		}
		else if (this->IsCollapseLegal(end_pt_ids[1], end_pt_ids[0]))
		{
			this->NumberOfEdgeCollapses++;

			this->UpdateEdgeData(end_pt_ids[1], end_pt_ids[0]);

			// Update the output triangles.
			num_deleted_tris += this->CollapseEdge(end_pt_ids[1], end_pt_ids[0]);
			this->ActualReduction = (double)num_deleted_tris / num_tris;
		}

		edge_id = this->EdgeCosts->Pop(0, cost);
	}
	printf("\n");

	// Perform flipping to improve the angles
	if (this->FlipEdges && !abort)
	{
		if (Loud)
			cout << "Started flipping edges" << endl;
		this->DelaunayFlipEdges();
	}

	// Perform subdivision
	if (this->UseMaximumEdgeLength)
	{
		if (Loud)
			cout << "Started dividing edges" << endl;
		this->SubdivideEdges();
	}

	// copy the simplified mesh from the working mesh to the output mesh
	if (DomainLabelName != nullptr && input->GetCellData()->HasArray(DomainLabelName))
	{
		this->CreateDuplicateTriangles(output);
	}
	else
	{
		vtkIdList* output_cell_list = vtkIdList::New();
		for (i = 0; i < this->Mesh->GetNumberOfCells(); i++)
		{
			if (this->Mesh->GetCell(i)->GetCellType() == VTK_TRIANGLE)
			{
				output_cell_list->InsertNextId(i);
			}
		}
		output->Reset();
		output->Allocate(this->Mesh, output_cell_list->GetNumberOfIds());
		//output->GetPointData()->CopyAllocate(this->Mesh->GetPointData(),1);
		output->CopyCells(this->Mesh, output_cell_list);

		this->Mesh->DeleteLinks();
		this->Mesh->Delete();
		output_cell_list->Delete();
	}

	if (Loud)
	{
		cout << "Finished vtkEdgeCollapse::RequestData()" << endl;
		cout << "Collapsed Edges: " << this->NumberOfEdgeCollapses << endl;
		cout << "Reduction: " << this->ActualReduction << endl;
		cout << "Flipped Edges: " << this->NumberOfEdgeFlips << endl;
		cout << "Split Edges: " << this->NumberOfEdgeDivisions << endl;
	}

	return 1;
}

//----------------------------------------------------------------------------
double vtkEdgeCollapse::ComputeAngleAtFirstPoint(vtkIdType i1, vtkIdType i2, vtkIdType i3)
{
	double p1[3], p2[3], p3[3];
	this->Mesh->GetPoint(i1, p1);
	this->Mesh->GetPoint(i2, p2);
	this->Mesh->GetPoint(i3, p3);

	const double v12[3] = {p1[0] - p2[0], p1[1] - p2[1], p1[2] - p2[2]};
	const double v13[3] = {p1[0] - p3[0], p1[1] - p3[1], p1[2] - p3[2]};

	double cosa =
			vtkMath::Dot(v12, v13) / vtkMath::Norm(v12) * vtkMath::Norm(v13);
	return std::acos(std::max(std::min(cosa, 1.0), -1.0));
}

//----------------------------------------------------------------------------
vtkIdType vtkEdgeCollapse::FindThirdNode(vtkIdType i0, vtkIdType i1, const vtkIdType pts[3])
{
	vtkIdType n3 = -1;
	for (int k = 0; k < 3; k++)
	{
		if (pts[k] != i0 && pts[k] != i1)
		{
			n3 = pts[k];
			break;
		}
	}
	return n3;
}

//----------------------------------------------------------------------------
int vtkEdgeCollapse::DelaunayFlipEdges()
{
	vtkIdType i, j, edge_id, n1, n2, n3, n4;
	vtkIdType *pts, *pts2, ids[3];
	vtkIdType npts, npts2;
	double pri;
	vtkNew(vtkPriorityQueue, flipPriority);
	vtkNew(vtkIdList, endPts1);
	vtkNew(vtkIdList, endPts2);
	vtkNew(vtkEdgeTable, edges);

	// compute edges and priority for each edge
	edges->InitEdgeInsertion(this->Mesh->GetNumberOfPoints(), 1); // storing edge id as attribute
	flipPriority->Allocate(this->Mesh->GetPolys()->GetNumberOfCells() * 3);
	for (i = 0; i < this->Mesh->GetNumberOfCells(); i++)
	{
		if (this->Mesh->GetCellType(i) != VTK_TRIANGLE)
			continue;

		this->Mesh->GetCellPoints(i, npts, pts);
		for (j = 0; j < 3; j++)
		{
			if (edges->IsEdge(pts[j], pts[(j + 1) % 3]) == -1)
			{
				// If this edge has not been processed, get an id for it, add it to
				// the edge list (Edges), and add its endpoints to the EndPoint1List
				// and EndPoint2List (the 2 endpoints to different lists).
				n1 = pts[j];
				n2 = pts[(j + 1) % 3];

				edge_id = edges->GetNumberOfEdges();

				this->Mesh->GetCellEdgeNeighbors(i, n1, n2, this->Neighbors);
				if (this->Neighbors->GetNumberOfIds() == 1)
				{
					n3 = pts[(j + 2) % 3];
					this->Mesh->GetCellPoints(this->Neighbors->GetId(0), npts2, pts2);
					n4 = this->FindThirdNode(n1, n2, pts2);
					assert(n4 >= 0 && n3 != n4);
					double angle3 = this->ComputeAngleAtFirstPoint(n3, n1, n2);
					double angle4 = this->ComputeAngleAtFirstPoint(n4, n1, n2);

					pri = angle3 + angle4 - 3.141592654f;
					if (pri > 0.0)
					{
						edges->InsertEdge(n1, n2, edge_id);
						endPts1->InsertId(edge_id, n1);
						endPts2->InsertId(edge_id, n2);
						flipPriority->Insert(-pri, edge_id);
					}
				}
			}
		}
	}

	edge_id = flipPriority->Pop(0, pri);
	while (edge_id >= 0)
	{
		n1 = endPts1->GetId(edge_id);
		n2 = endPts2->GetId(edge_id);

		// fetch the next edge to flip, placed here on purpose
		edge_id = flipPriority->Pop(0, pri);

		if (!this->Mesh->IsEdge(n1, n2))
			continue;

		// Can only get edge neighbors if n1,n2 is an edge
		this->Mesh->GetCellEdgeNeighbors(-1, n1, n2, this->Neighbors);

		// Can only flip manifold edges
		if (!(this->Neighbors->GetNumberOfIds() == 2) ||
				!(this->Neighbors->GetId(0) != this->Neighbors->GetId(1)))
			continue;

		// Find n3,n4, i.e., the third nodes of the two triangles
		this->Mesh->GetCellPoints(this->Neighbors->GetId(0), npts, pts);
		n3 = this->FindThirdNode(n1, n2, pts);
		this->Mesh->GetCellPoints(this->Neighbors->GetId(1), npts, pts);
		n4 = this->FindThirdNode(n1, n2, pts);

		// The other two nodes should not be connected via an edge
		if (n3 < 0 || n4 < 0 || this->Mesh->IsEdge(n3, n4))
			continue;

		// Test if this flip still make sense
		double angle3 = this->ComputeAngleAtFirstPoint(n3, n1, n2);
		double angle4 = this->ComputeAngleAtFirstPoint(n4, n1, n2);
		if (angle3 + angle4 - 3.141592654f < pri)
			continue;

		// Test if normals don't change too much
		double n123[3], n142[3], n143[3], n234[3];

		ids[0] = n1;
		ids[1] = n2;
		ids[2] = n3;
		vtkPolygon::ComputeNormal(this->Mesh->GetPoints(), 3, ids, n123);

		ids[0] = n1;
		ids[1] = n4;
		ids[2] = n2;
		vtkPolygon::ComputeNormal(this->Mesh->GetPoints(), 3, ids, n142);

		ids[0] = n1;
		ids[1] = n4;
		ids[2] = n3;
		vtkPolygon::ComputeNormal(this->Mesh->GetPoints(), 3, ids, n143);

		ids[0] = n2;
		ids[1] = n3;
		ids[2] = n4;
		vtkPolygon::ComputeNormal(this->Mesh->GetPoints(), 3, ids, n234);

		if (vtkMath::Dot(n123, n142) < this->NormalDotProductThreshold ||
				vtkMath::Dot(n143, n234) < this->NormalDotProductThreshold)
			continue;

		// Do Self-Intersection Test
		if (this->IntersectionCheckLevel > 0)
		{
			std::vector<MESH::Triangle> triangles_after_flip;
			std::vector<MESH::Triangle> triangles_nearby;
			// insert flipped triangles
			triangles_after_flip.push_back(MESH::Triangle(n1, n4, n3));
			triangles_after_flip.push_back(MESH::Triangle(n2, n3, n4));

			std::set<vtkIdType> neighbors;
			unsigned short ncells;
			vtkIdType* cells;

			switch (this->IntersectionCheckLevel)
			{
			case 1: {
				this->Mesh->GetPointCells(n1, ncells, cells);
				neighbors.insert(cells, cells + ncells);
				this->Mesh->GetPointCells(n2, ncells, cells);
				neighbors.insert(cells, cells + ncells);
			}
			break;
			default: {
				this->Mesh->GetPointCells(n1, ncells, cells);
				neighbors.insert(cells, cells + ncells);
				this->Mesh->GetPointCells(n2, ncells, cells);
				neighbors.insert(cells, cells + ncells);
				this->Mesh->GetPointCells(n3, ncells, cells);
				neighbors.insert(cells, cells + ncells);
				this->Mesh->GetPointCells(n4, ncells, cells);
				neighbors.insert(cells, cells + ncells);
			}
			}
			// remove the unflipped triangles from the set of candidates
			neighbors.erase(this->Neighbors->GetId(0));
			neighbors.erase(this->Neighbors->GetId(1));
			for (std::set<vtkIdType>::iterator it = neighbors.begin();
					 it != neighbors.end(); ++it)
			{
				this->Mesh->GetCellPoints(*it, npts, pts);
				assert(npts == 3);
				triangles_nearby.push_back(MESH::Triangle(pts[0], pts[1], pts[2]));
			}
			if (!NoSelfIntersection(this->Mesh->GetPoints(), triangles_after_flip, triangles_nearby))
				continue;
		}

		// Finally: Flip triangles
		short unsigned int nc3, nc4;
		vtkIdType* cells;
		this->Mesh->GetPointCells(n3, nc3, cells);
		this->Mesh->GetPointCells(n4, nc4, cells);

		this->Mesh->RemoveCellReference(this->Neighbors->GetId(0));
		this->Mesh->RemoveCellReference(this->Neighbors->GetId(1));

		// Points 3 and 4 will have reference one extra triangle
		this->Mesh->ResizeCellList(n3, nc3 + 1);
		this->Mesh->ResizeCellList(n4, nc4 + 1);

		this->Mesh->ReplaceCellPoint(this->Neighbors->GetId(0), n1, n4);
		this->Mesh->ReplaceCellPoint(this->Neighbors->GetId(1), n2, n3);

		this->Mesh->AddCellReference(this->Neighbors->GetId(0));
		this->Mesh->AddCellReference(this->Neighbors->GetId(1));

		NumberOfEdgeFlips++;
	}

	return (NumberOfEdgeFlips);
}

//----------------------------------------------------------------------------
int vtkEdgeCollapse::CollapseEdge(vtkIdType pt0Id, vtkIdType pt1Id)
{
	int j, num_deleted = 0;
	vtkIdType* pts;
	vtkIdType npts;
	vtkIdType i;
	vtkIdType cell_id;

	this->Mesh->GetPointCells(pt0Id, this->CollapseCellIds);
	for (i = 0; i < this->CollapseCellIds->GetNumberOfIds(); i++)
	{
		cell_id = this->CollapseCellIds->GetId(i);
		this->Mesh->GetCellPoints(cell_id, npts, pts);
		for (j = 0; j < 3; j++)
		{
			if (pts[j] == pt1Id)
			{
				this->Mesh->RemoveCellReference(cell_id);
				this->Mesh->DeleteCell(cell_id);
				num_deleted++;
			}
		}
	}

	this->Mesh->GetPointCells(pt1Id, this->CollapseCellIds);
	this->Mesh->ResizeCellList(pt0Id, this->CollapseCellIds->GetNumberOfIds());
	for (i = 0; i < this->CollapseCellIds->GetNumberOfIds(); i++)
	{
		cell_id = this->CollapseCellIds->GetId(i);
		this->Mesh->GetCellPoints(cell_id, npts, pts);
		// making sure we don't already have the triangle we're about to
		// change this one to
		if ((pts[0] == pt1Id &&
						this->Mesh->IsTriangle(pt0Id, pts[1], pts[2])) ||
				(pts[1] == pt1Id &&
						this->Mesh->IsTriangle(pts[0], pt0Id, pts[2])) ||
				(pts[2] == pt1Id && this->Mesh->IsTriangle(pts[0], pts[1], pt0Id)))
		{
			this->Mesh->RemoveCellReference(cell_id);
			this->Mesh->DeleteCell(cell_id);
			num_deleted++;
		}
		else
		{
			this->Mesh->AddReferenceToCell(pt0Id, cell_id);
			this->Mesh->ReplaceCellPoint(cell_id, pt1Id, pt0Id);
		}
	}
	this->Mesh->DeletePoint(pt1Id);

	return num_deleted;
}

//----------------------------------------------------------------------------
// FIXME: memory allocation clean up
void vtkEdgeCollapse::UpdateEdgeData(vtkIdType pt0Id, vtkIdType pt1Id)
{ // Assumption is that later CollapseEdge will remove pt1Id
	vtkIdList* changed_edges = vtkIdList::New();
	vtkIdType i, edge_id, edge[2];
	double cost;
	double x1[3], x2[3];

	// Find all edges with exactly either of these 2 endpoints.
	this->FindAffectedEdges(pt0Id, pt1Id, changed_edges);

	// Reset the endpoints for these edges to reflect the new point from the
	// collapsed edge.
	// Add these new edges to the edge table.
	// Remove the the changed edges from the priority queue.
	for (i = 0; i < changed_edges->GetNumberOfIds(); i++)
	{
		edge[0] = this->EndPoint1List->GetId(changed_edges->GetId(i));
		edge[1] = this->EndPoint2List->GetId(changed_edges->GetId(i));

		// Remove all affected edges from the priority queue.
		// This does not include collapsed edge.
		this->EdgeCosts->DeleteId(changed_edges->GetId(i));

		// Determine the new set of edges
		if (edge[0] == pt1Id)
		{
			if (this->Edges->IsEdge(edge[1], pt0Id) == -1)
			{ // The edge will be completely new, add it.
				edge_id = this->Edges->GetNumberOfEdges();
				this->Edges->InsertEdge(edge[1], pt0Id, edge_id);
				this->EndPoint1List->InsertId(edge_id, edge[1]);
				this->EndPoint2List->InsertId(edge_id, pt0Id);
				// Compute cost (target point/data) and add to priority cue.
				this->Mesh->GetPoint(edge[1], x1);
				this->Mesh->GetPoint(pt0Id, x2);
				cost = vtkMath::Distance2BetweenPoints(x1, x2);
				if (cost < this->MinLength2)
				{
					this->EdgeCosts->Insert(cost, edge_id);
				}
			}
		}
		else if (edge[1] == pt1Id)
		{ // The edge will be completely new, add it.
			if (this->Edges->IsEdge(edge[0], pt0Id) == -1)
			{
				edge_id = this->Edges->GetNumberOfEdges();
				this->Edges->InsertEdge(edge[0], pt0Id, edge_id);
				this->EndPoint1List->InsertId(edge_id, edge[0]);
				this->EndPoint2List->InsertId(edge_id, pt0Id);
				// Compute cost (target point/data) and add to priority cue.
				this->Mesh->GetPoint(edge[0], x1);
				this->Mesh->GetPoint(pt0Id, x2);
				cost = vtkMath::Distance2BetweenPoints(x1, x2);
				if (cost < this->MinLength2)
				{
					this->EdgeCosts->Insert(cost, edge_id);
				}
			}
		}
		else
		{ // This edge already has one point as the merged point.
			this->Mesh->GetPoint(edge[0], x1);
			this->Mesh->GetPoint(edge[1], x2);
			cost = vtkMath::Distance2BetweenPoints(x1, x2);
			if (cost < this->MinLength2)
			{
				this->EdgeCosts->Insert(cost, changed_edges->GetId(i));
			}
		}
	}

	changed_edges->Delete();
}

//----------------------------------------------------------------------------
void vtkEdgeCollapse::FindAffectedEdges(vtkIdType p1Id, vtkIdType p2Id, vtkIdList* edges)
{
	unsigned short ncells;
	vtkIdType *cells, npts, *pts, edge_id;
	unsigned short i, j;

	edges->Reset();
	this->Mesh->GetPointCells(p2Id, ncells, cells);
	for (i = 0; i < ncells; i++)
	{
		this->Mesh->GetCellPoints(cells[i], npts, pts);
		for (j = 0; j < 3; j++)
		{
			if (pts[j] != p1Id && pts[j] != p2Id &&
					(edge_id = this->Edges->IsEdge(pts[j], p2Id)) >= 0 &&
					edges->IsId(edge_id) == -1)
			{
				edges->InsertNextId(edge_id);
			}
		}
	}

	this->Mesh->GetPointCells(p1Id, ncells, cells);
	for (i = 0; i < ncells; i++)
	{
		this->Mesh->GetCellPoints(cells[i], npts, pts);
		for (j = 0; j < 3; j++)
		{
			if (pts[j] != p1Id && pts[j] != p2Id &&
					(edge_id = this->Edges->IsEdge(pts[j], p1Id)) >= 0 &&
					edges->IsId(edge_id) == -1)
			{
				edges->InsertNextId(edge_id);
			}
		}
	}
}

//----------------------------------------------------------------------------
bool vtkEdgeCollapse::IsDegenerateTriangle(vtkIdType i0, vtkIdType i1, vtkIdType i2)
{
	return (i0 == i1 || i0 == i2 || i1 == i2);
}

//----------------------------------------------------------------------------
int vtkEdgeCollapse::GetOriginalNormal(double pos[3], double normal[3])
{
	if (this->CellLocator != nullptr && this->Normals != nullptr)
	{
		double cp[3], dist2;
		vtkIdType cellid;
		int subid;
		this->CellLocator->FindClosestPoint(pos, cp, this->GCell, cellid, subid, dist2);
		if (cellid < 0)
			return false;

		assert(cellid < Normals->GetNumberOfTuples());
		Normals->GetTuple(cellid, normal);
		return true;
	}
	return false;
}

//----------------------------------------------------------------------------
void vtkEdgeCollapse::ComputeNormals(vtkPolyData* input)
{
	vtkIdType* pts;
	vtkIdType npts;
	vtkIdType cell_id;
	// build cell locator, to find normal on original surface
	this->CellLocator->SetDataSet(input);
	this->CellLocator->SetMaxLevel(8);
	this->CellLocator->BuildLocator();

	// Setup and compute normals of original surface
	this->Normals = vtkFloatArray::New();
	this->Normals->SetNumberOfComponents(3);
	this->Normals->Allocate(3 * input->GetNumberOfCells());

	double n[3];
	vtkCellArray* polys = input->GetPolys();
	for (cell_id = 0, polys->InitTraversal(); polys->GetNextCell(npts, pts);
			 cell_id++)
	{
		vtkPolygon::ComputeNormal(input->GetPoints(), npts, pts, n);
		this->Normals->InsertTuple(cell_id, n);
	}
}

//----------------------------------------------------------------------------
void vtkEdgeCollapse::ComputeManifoldedness()
{
	isboundary.resize(this->Mesh->GetNumberOfPoints(), false);
	if (this->MeshIsManifold)
		return;

	vtkIdType* pts;
	vtkIdType npts;
	vtkIdType cell_id;
	vtkCellArray* polys = this->Mesh->GetPolys();
	for (cell_id = 0, polys->InitTraversal(); polys->GetNextCell(npts, pts);
			 cell_id++)
	{
		// skip this cell if is e.g. VTK_EMPTY_CELL
		if (Mesh->GetCellType(cell_id) != VTK_TRIANGLE)
			continue;

		for (int i = 0; i < npts; i++)
		{
			vtkIdType i1 = pts[i];
			vtkIdType i2 = pts[(i + 1) % npts];

			this->Mesh->GetCellEdgeNeighbors(cell_id, i1, i2, this->Neighbors);
			int num_nei = this->Neighbors->GetNumberOfIds();

			// at a manifold edge, there should be one neighboring triangle
			// 0   : boundary
			// 1   : ordinary
			// 2++ : non-manifold edge
			if (num_nei != 1)
			{
				//edgemap[Edge(i1,i2)] = numNei+1;
				isboundary[i1] = true;
				isboundary[i2] = true;
				InputIsNonmanifold = 1;
			}
		}
	}
}

//----------------------------------------------------------------------------
bool vtkEdgeCollapse::IsCollapseLegal(vtkIdType i1, vtkIdType i2)
{ // Assumption is that i2 will be removed (moved to i1)
	vtkIdType* pts;
	vtkIdType npts;
	// Check if either i1 or i2 is deleted, i.e. has no cells
	this->Mesh->GetPointCells(i1, this->Neighbors);
	if (this->Neighbors->GetNumberOfIds() == 0)
		return false;
	this->Mesh->GetPointCells(i2, this->Neighbors);
	if (this->Neighbors->GetNumberOfIds() == 0)
		return false;

	if (i1 == i2)
		return false;

	this->Mesh->GetCellEdgeNeighbors(-1, i1, i2, this->CollapseCellIds);
	int edge_tris = this->CollapseCellIds->GetNumberOfIds();
	if (edge_tris == 0)
		return false;

	// If both end points are on boundary then the connecting edge should be nonmanifold
	bool nonmanifold = (edge_tris != 2);
	if (!nonmanifold && (isboundary[i1] && isboundary[i2]))
		return false;

	//
	// There should only be 2 degenerate triangles (if mesh is manifold)
	//
	// this->Mesh->GetPointCells(i2, this->Neighbors); called above
	int count_degenerate = 0;
	double normal_new[3];
	double normal[3];
	std::list<vtkIdType> degenerate;
	for (int k = 0; k < this->Neighbors->GetNumberOfIds(); k++)
	{
		vtkIdType cell_id = this->Neighbors->GetId(k);
		this->Mesh->GetCellPoints(cell_id, this->PointIds);
		vtkIdType* ids = this->PointIds->GetPointer(0);
		assert(!this->IsDegenerateTriangle(ids[0], ids[1], ids[2]));
		for (int i = 0; i < 3; i++)
			if (ids[i] == i2)
				ids[i] = i1;
		if (this->IsDegenerateTriangle(ids[0], ids[1], ids[2]))
		{
			count_degenerate++;
			degenerate.push_back(cell_id);
		}
		else
		{
			// Compute area
			vtkPolygon::ComputeNormal(this->Mesh->GetPoints(), 3, this->PointIds->GetPointer(0), normal_new);

			// The normals of the new triangles should point
			// more or less in the same direction as the old triangles
			/*
			this->Mesh->GetPoint(this->PointIds->GetId(0), x0);
			this->Mesh->GetPoint(this->PointIds->GetId(1), x1);
			this->Mesh->GetPoint(this->PointIds->GetId(2), x2);
			for (int k=0; k<3; k++)
				center[k] = 0.333 * (x0[k] + x1[k] + x2[k]);

			if (GetOriginalNormal(center, normal))
			{
			*/
			//TODO: clean-up this code
			// Assumes that the cell ids don't change ordering
			// (deleted cells are left in same position)
			this->Normals->GetTuple(cell_id, normal);
			if ((true))
			{
				double cost = vtkMath::Dot(normal, normal_new);
				if (cost < NormalDotProductThreshold)
				{
					// angle deviation is larger than MaximumNormalAngleDeviation
					return false;
				}
			}
			else
			{
				cout << "Could not find closest point on original surface"
						 << endl;
				return false;
			}
		}
	} // end for all i2 triangles

	// Only those triangles at edge i1,i2 should be degenerate
	if (count_degenerate > edge_tris)
	{
		return false;
	}

	//
	// Test if not exactly 2 nodes are connected to the edge i1,i2
	//
	/* Why does this code not work, but the one below does?
	std::set<vtkIdType> inter;
	this->PointIds->Reset();
	this->PointIds->InsertNextId(i1);
	this->PointIds->InsertNextId(i2);
	this->Mesh->GetCellNeighbors(-1, this->PointIds, this->Neighbors);
	for (int k=0; k<this->Neighbors->GetNumberOfIds(); k++)
	{
		vtkIdType cellId = this->Neighbors->GetId(k);
		this->Mesh->GetCellPoints(cellId, this->PointIds);
		inter.insert(this->PointIds->GetId(0));
		inter.insert(this->PointIds->GetId(1));
		inter.insert(this->PointIds->GetId(2));
	}
	if (inter.size() != 2+edge_tris)
	{
		if(Loud>1) cout << "inter.size() = " << inter.size() << endl;
		return false;
	}
	*/
	std::list<vtkIdType> nodes_i1, nodes_i2;
	this->Mesh->GetPointCells(i1, this->Neighbors);
	for (int k = 0; k < this->Neighbors->GetNumberOfIds(); k++)
	{
		this->Mesh->GetCellPoints(this->Neighbors->GetId(k), npts, pts);
		nodes_i1.push_back(pts[0]);
		nodes_i1.push_back(pts[1]);
		nodes_i1.push_back(pts[2]);
	}
	this->Mesh->GetPointCells(i2, this->Neighbors);
	for (int k = 0; k < this->Neighbors->GetNumberOfIds(); k++)
	{
		this->Mesh->GetCellPoints(this->Neighbors->GetId(k), npts, pts);
		nodes_i2.push_back(pts[0]);
		nodes_i2.push_back(pts[1]);
		nodes_i2.push_back(pts[2]);
	}
	nodes_i1.sort();
	nodes_i2.sort();
	std::list<vtkIdType> inter;
	std::set_intersection(nodes_i1.begin(), nodes_i1.end(), nodes_i2.begin(), nodes_i2.end(), std::back_inserter(inter));
	inter.unique();
	if (inter.size() != 2 + edge_tris)
	{
		if (Loud > 1)
			cout << "inter.size() = " << inter.size() << endl;
		return false;
	}

	//
	// Surface approximation: if normal at center of collapsed edge changes, don't allow collapse
	//

	//
	// Self-Intersection Checks
	//
	if (IntersectionCheckLevel > 0)
	{
		std::list<vtkIdType> nodes_i;
		std::list<vtkIdType>::iterator it;
		unsigned short ncells;
		vtkIdType* cells;
		std::set<vtkIdType> triangles_after_collapse;

		switch (IntersectionCheckLevel)
		{
		case 1: {
			this->Mesh->GetPointCells(i2, ncells, cells);
			for (int k = 0; k < this->Neighbors->GetNumberOfIds(); k++)
			{
				triangles_after_collapse.insert(cells, cells + ncells);
			}
		}
		break;
		case 2: {
			for (it = inter.begin(); it != inter.end(); ++it)
			{
				this->Mesh->GetPointCells(*it, ncells, cells);
				for (int k = 0; k < this->Neighbors->GetNumberOfIds(); k++)
				{
					triangles_after_collapse.insert(cells, cells + ncells);
				}
			}
		}
		break;
		case 3: {
			nodes_i1.unique(); // TODO: is this necessary
			nodes_i2.unique();
			set_union(nodes_i1.begin(), nodes_i1.end(), nodes_i2.begin(), nodes_i2.end(), std::back_inserter(nodes_i));
			for (it = nodes_i.begin(); it != nodes_i.end(); ++it)
			{
				this->Mesh->GetPointCells(*it, ncells, cells);
				for (int k = 0; k < this->Neighbors->GetNumberOfIds(); k++)
				{
					triangles_after_collapse.insert(cells, cells + ncells);
				}
			}
		}
		break;
		default: {
			double x[3];
			this->Mesh->GetPoint(i2, x);
			double bounds[6] = {x[0], x[0], x[1], x[1], x[2], x[2]};
			for (it = inter.begin(); it != inter.end(); ++it)
			{
				this->Mesh->GetPoint(*it, x);
				bounds[0] = std::min(x[0], bounds[0]);
				bounds[1] = std::max(x[0], bounds[1]);
				bounds[2] = std::min(x[1], bounds[2]);
				bounds[3] = std::max(x[1], bounds[3]);
				bounds[4] = std::min(x[2], bounds[4]);
				bounds[5] = std::max(x[2], bounds[5]);
			}
			if (IntersectionCheckLevel > 4)
			{
				double grow = (IntersectionCheckLevel - 4) * 0.5;
				for (int d = 0; d < 3; d++)
				{
					double dx = grow * (bounds[2 * d + 1] - bounds[2 * d]);
					assert(dx >= 0.0);
					bounds[2 * d] -= dx;
					bounds[2 * d + 1] += dx;
				}
			}

			this->CellLocator->FindCellsWithinBounds(bounds, this->Neighbors);
			for (int i = 0; i < this->Neighbors->GetNumberOfIds(); i++)
			{
				if (this->CellLocator->GetDataSet()->GetCellType(i) ==
						VTK_TRIANGLE)
				{
					triangles_after_collapse.insert(this->Neighbors->GetId(i));
				}
			}
		}
		break;
		}

		// Do self-intersection test
		assert(degenerate.size() == count_degenerate);
		for (it = degenerate.begin(); it != degenerate.end(); ++it)
		{
			triangles_after_collapse.erase(*it);
		}
		std::vector<MESH::Triangle> tris;
		for (std::set<vtkIdType>::iterator sit =
						 triangles_after_collapse.begin();
				 sit != triangles_after_collapse.end(); ++sit)
		{
			this->Mesh->GetCellPoints(*sit, npts, pts);
			if (npts != 3)
				continue;

			MESH::Triangle tri(pts[0], pts[1], pts[2]);
			if (tri.m_N1 == i2)
				tri.m_N1 = i1;
			if (tri.m_N2 == i2)
				tri.m_N2 = i1;
			if (tri.m_N3 == i2)
				tri.m_N3 = i1;
			assert(!this->IsDegenerateTriangle(tri.m_N1, tri.m_N2, tri.m_N3));
			tris.push_back(tri);
		}
		if (!NoSelfIntersection(this->Mesh->GetPoints(), tris))
			return false;
	}

	return true;
}

//----------------------------------------------------------------------------
int vtkEdgeCollapse::SubdivideEdges()
{
	vtkIdType i, j, edge_id, n1, n2, n3;
	vtkIdType* pts;
	vtkIdType npts, cell_id;
	double length2, x1[3], x2[3], x3[3], new_x[3];
	double max_length2 = MaximumEdgeLength * MaximumEdgeLength;
	vtkNew(vtkPriorityQueue, splitPriority);
	vtkNew(vtkIdList, endPts1);
	vtkNew(vtkIdList, endPts2);
	vtkNew(vtkEdgeTable, edges);

	// compute edges and priority for each edge
	edges->InitEdgeInsertion(this->Mesh->GetNumberOfPoints(), 1); // storing edge id as attribute
	splitPriority->Allocate(this->Mesh->GetPolys()->GetNumberOfCells() * 3);
	for (i = 0; i < this->Mesh->GetNumberOfCells(); i++)
	{
		if (this->Mesh->GetCellType(i) != VTK_TRIANGLE)
			continue;

		this->Mesh->GetCellPoints(i, npts, pts);
		for (j = 0; j < 3; j++)
		{
			if (edges->IsEdge(pts[j], pts[(j + 1) % 3]) == -1)
			{
				// If this edge has not been processed, get an id for it, add it to
				// the edge list (Edges), and add its endpoints to the EndPoint1List
				// and EndPoint2List (the 2 endpoints to different lists).
				n1 = pts[j];
				n2 = pts[(j + 1) % 3];

				edge_id = edges->GetNumberOfEdges();
				this->Mesh->GetPoint(n1, x1);
				this->Mesh->GetPoint(n2, x2);
				length2 = vtkMath::Distance2BetweenPoints(x1, x2);
				if (length2 > max_length2)
				{
					edges->InsertEdge(n1, n2, edge_id);
					endPts1->InsertId(edge_id, n1);
					endPts2->InsertId(edge_id, n2);
					// lower values are popped first, so negative sign pops longest edges first
					splitPriority->Insert(-length2, edge_id);
				}
			}
		}
	}

	edge_id = splitPriority->Pop(0, length2);
	while (edge_id >= 0)
	{
		n1 = endPts1->GetId(edge_id);
		n2 = endPts2->GetId(edge_id);
		assert(this->Mesh->IsEdge(n1, n2));

		// find adjacent triangles
		this->Mesh->GetCellEdgeNeighbors(-1, n1, n2, this->Neighbors);
		assert(this->Neighbors->GetNumberOfIds() > 0);

		// create point at center of edge (or somewhere else?)
		this->Mesh->GetPoint(n1, x1);
		this->Mesh->GetPoint(n2, x2);
		new_x[0] = 0.5 * (x1[0] + x2[0]);
		new_x[1] = 0.5 * (x1[1] + x2[1]);
		new_x[2] = 0.5 * (x1[2] + x2[2]);
		vtkIdType new_pt_id = this->Mesh->InsertNextLinkedPoint(new_x, 2 * this->Neighbors->GetNumberOfIds());

		for (i = 0; i < Neighbors->GetNumberOfIds(); i++)
		{
			cell_id = Neighbors->GetId(i);

			// get third node of triangle
			this->Mesh->GetCellPoints(cell_id, this->PointIds);
			vtkIdType new_cell_id = this->Mesh->InsertNextLinkedCell(VTK_TRIANGLE, 3, this->PointIds->GetPointer(0));
			if (this->Labels)
			{
				this->Labels->InsertNextTuple1(this->Labels->GetTuple1(cell_id));
				assert(this->Labels->GetNumberOfTuples() ==
							 this->Mesh->GetNumberOfCells());
			}

			// modify old triangle to use newPtId instead of n2, i.e. (n1,n3,newPtId)
			this->Mesh->RemoveReferenceToCell(n2, cell_id);
			this->Mesh->ReplaceCellPoint(cell_id, n2, new_pt_id);
			// should have already created enough space for point cells in InsertNextLinkedPoint
			this->Mesh->ResizeCellList(new_pt_id, 1);
			this->Mesh->AddReferenceToCell(new_pt_id, cell_id);

			// add other half of triangle
			// modify old triangle to use newPtId instead of n1, i.e. (n2,n3,newPtId)
			this->Mesh->RemoveReferenceToCell(n1, new_cell_id);
			this->Mesh->ReplaceCellPoint(new_cell_id, n1, new_pt_id);
			// should have already created enough space for point cells in InsertNextLinkedPoint
			this->Mesh->ResizeCellList(new_pt_id, 1);
			this->Mesh->AddReferenceToCell(new_pt_id, new_cell_id);

			// add edge between newPtId and third node of triangle to queue
			n3 = FindThirdNode(n1, n2, this->PointIds->GetPointer(0));
			this->Mesh->GetPoint(n3, x3);
			double len2 = vtkMath::Distance2BetweenPoints(new_x, x3);
			if (len2 > max_length2)
			{
				edge_id = edges->GetNumberOfEdges();
				edges->InsertEdge(n3, new_pt_id, edge_id);
				endPts1->InsertId(edge_id, n3);
				endPts2->InsertId(edge_id, new_pt_id);
				splitPriority->Insert(-len2, edge_id);
			}
		}

		// half length, quarter squared length
		this->Mesh->GetPoint(n1, x1);
		this->Mesh->GetPoint(new_pt_id, x2);
		length2 = vtkMath::Distance2BetweenPoints(x1, x2);
		// add new edge to Priority Queue
		if (length2 > max_length2)
		{
			edge_id = edges->GetNumberOfEdges();
			edges->InsertEdge(n1, new_pt_id, edge_id);
			endPts1->InsertId(edge_id, n1);
			endPts2->InsertId(edge_id, new_pt_id);
			splitPriority->Insert(-length2, edge_id);

			edge_id = edges->GetNumberOfEdges();
			edges->InsertEdge(n2, new_pt_id, edge_id);
			endPts1->InsertId(edge_id, n2);
			endPts2->InsertId(edge_id, new_pt_id);
			splitPriority->Insert(-length2, edge_id);
		}

		this->NumberOfEdgeDivisions++;

		// fetch the next edge to split
		edge_id = splitPriority->Pop(0, length2);
		assert(endPts1->GetNumberOfIds() == edges->GetNumberOfEdges());
	}

	return 0;
}

//----------------------------------------------------------------------------
/*
void vtkEdgeCollapse::MapDuplicateTriangles(vtkDataArray* labels)
{   // Assumes Mesh has been created, now just need to find duplicates
	// and remove reference to them. Additionally, builds a mapping of a key to
	// the DuplicateLabel
	double* range = labels->GetRange();
	int newLabel = range[1]+2;
	vtkIdType numTris = this->Mesh->GetNumberOfPolys();

	vtkIntArray *newlabels = vtkIntArray::New();
	newlabels->SetNumberOfTuples(numTris);
	newlabels->SetName(DomainLabelName);

	// Check for duplicates, create a special label for duplicates
	if(Loud) cout << "Check for duplicates, creating new labels" << endl;
	using SortTriangle = MESH::SortElement<vtkIdType, MESH::Triangle>;
	std::set<SortTriangle> unique;
	std::set<SortTriangle>::iterator it;
	std::vector<bool> isduplicate(numTris,0);
	for(vtkIdType i=0 ; i<numTris; ++i)
	{
		this->Mesh->GetCellPoints(i, this->PointIds);
		vtkIdType ids[] = {PointIds->GetId(0),PointIds->GetId(1),PointIds->GetId(2)};
		std::sort(ids,ids+3);
		MESH::Triangle tri(ids[0],ids[1],ids[2],labels->GetTuple1(i));
		SortTriangle stri(i,tri);
		if( (it=unique.find(stri)) == unique.end() )
		{
			unique.insert(stri);
			newlabels->SetTuple1(i, labels->GetTuple1(i));
		}
		else
		{
			MESH::Triangle otri = it->value;
			assert(tri.label != otri.label);
			DuplicateLabel duplicate(std::min(tri.label,otri.label),std::max(tri.label,otri.label));
			LabelMap::iterator labelit = labelmap.find(duplicate);
			if(labelit != labelmap.end())
			{
				assert(it->elem >= 0 && it->elem < numTris);
				newlabels->SetTuple1(it->elem, labelit->second);
				newlabels->SetTuple1(i, labelit->second);
				isduplicate[i] = 1;
			}
			else
			{
				labelmap[duplicate] = ++newLabel;
				ilabelmap[newLabel] = duplicate;
				assert(it->elem >= 0 && it->elem < numTris);
				newlabels->SetTuple1(it->elem, newLabel);
				newlabels->SetTuple1(i, newLabel);
				isduplicate[i] = 1;
			}
		}
	}
	this->Mesh->GetCellData()->AddArray(newlabels);
	newlabels->Delete();

	// Now remove the duplicate triangles
	int NumberOfDuplicateTriangles = 0;
	for(vtkIdType i = 0; i < numTris; i++)
	{
		if(isduplicate[i])
		{
			this->Mesh->RemoveCellReference(i);
			this->Mesh->DeleteCell(i);
			assert(this->Mesh->GetCellType(i) == VTK_EMPTY_CELL);
			NumberOfDuplicateTriangles++;
		}
	}
	std::cerr << "NumberOfDuplicateTriangles " << NumberOfDuplicateTriangles << std::endl;
}
*/

//----------------------------------------------------------------------------
void vtkEdgeCollapse::MapDuplicateTriangles(vtkDataArray* labels)
{ // Assumes Mesh has been created, now just need to find duplicates
	// and remove reference to them. Additionally, builds a mapping of a key to
	// the DuplicateLabel
	double* range = labels->GetRange();
	if (Loud)
		std::cout << "[" << range[0] << "," << range[1] << "]" << std::endl;
	int new_label = range[1] + 2;
	vtkIdType num_tris = this->Mesh->GetNumberOfPolys();

	vtkIntArray* newlabels = vtkIntArray::New();
	newlabels->SetNumberOfTuples(num_tris);
	newlabels->SetName(DomainLabelName);

	// Check for duplicates, create a special label for duplicates
	if (Loud)
		cout << "Check for duplicates, creating new labels" << endl;
	LabelMapType labelmap;
	std::vector<bool> isvisited(num_tris, false);
	std::vector<bool> isduplicate(num_tris, false);
	for (vtkIdType i = 0; i < num_tris; ++i)
	{
		if (isvisited[i])
			continue;

		this->Mesh->GetCellPoints(i, this->PointIds);
		this->Mesh->GetCellNeighbors(i, this->PointIds, this->Neighbors);

		if (this->Neighbors->GetNumberOfIds() == 0)
		{
			// This triangle is unique
			newlabels->SetTuple1(i, labels->GetTuple1(i));
		}
		else
		{
			// This triangle is duplicate
			int label_i = labels->GetTuple1(i);
			int label_other = labels->GetTuple1(this->Neighbors->GetId(0));
			assert(label_i != label_other);
			DuplicateLabel duplicate(std::min(label_i, label_other), std::max(label_i, label_other));
			// Check if this interface already has been mapped to a label
			LabelMapType::iterator labelit = labelmap.find(duplicate);
			if (labelit != labelmap.end())
			{
				assert(this->Neighbors->GetId(0) >= 0 &&
							 this->Neighbors->GetId(0) < num_tris);
				newlabels->SetTuple1(this->Neighbors->GetId(0), labelit->second);
				newlabels->SetTuple1(i, labelit->second);
				isvisited[i] = isvisited[this->Neighbors->GetId(0)] = true;
				// mark triangle with larger label as duplicate
				if (label_i < label_other)
					isduplicate[this->Neighbors->GetId(0)] = true;
				else
					isduplicate[i] = true;
			}
			// Not yet, now create a new label for this interface
			else
			{
				labelmap[duplicate] = ++new_label;
				ilabelmap[new_label] = duplicate;
				assert(this->Neighbors->GetId(0) >= 0 &&
							 this->Neighbors->GetId(0) < num_tris);
				newlabels->SetTuple1(this->Neighbors->GetId(0), new_label);
				newlabels->SetTuple1(i, new_label);
				isvisited[i] = isvisited[this->Neighbors->GetId(0)] = true;
				// mark triangle with larger label as duplicate
				if (label_i < label_other)
					isduplicate[this->Neighbors->GetId(0)] = true;
				else
					isduplicate[i] = true;
			}
		}
	}
	this->Mesh->GetCellData()->AddArray(newlabels);
	newlabels->Delete();

	// Now remove the duplicate triangles
	int number_of_duplicate_triangles = 0;
	for (vtkIdType i = 0; i < num_tris; i++)
	{
		if (isduplicate[i])
		{
			this->Mesh->RemoveCellReference(i);
			this->Mesh->DeleteCell(i);
			assert(this->Mesh->GetCellType(i) == VTK_EMPTY_CELL);
			number_of_duplicate_triangles++;
		}
	}
	std::cerr << "NumberOfDuplicateTriangles " << number_of_duplicate_triangles
						<< std::endl;
}

//----------------------------------------------------------------------------
void vtkEdgeCollapse::CreateDuplicateTriangles(vtkPolyData* output)
{
	vtkIdType i;
	output->Reset();
	vtkIdList* output_cell_list = vtkIdList::New();
	vtkIdList* flip_cell_list = vtkIdList::New();

	vtkDataArray* inlabels =
			this->Mesh->GetCellData()->GetArray(DomainLabelName);
	assert(inlabels);
	assert(this->Mesh->GetNumberOfCells() == inlabels->GetNumberOfTuples());

	vtkIntArray* labels = vtkIntArray::New();
	labels->SetName(DomainLabelName);

	for (i = 0; i < this->Mesh->GetNumberOfCells(); i++)
	{
		if (this->Mesh->GetCell(i)->GetCellType() == VTK_TRIANGLE)
		{
			int label = inlabels->GetTuple1(i);
			InverseLabelMapType::iterator it = ilabelmap.find(label);
			if (it != ilabelmap.end())
			{
				// This triangle represents two domains, i.e. interface
				DuplicateLabel duplicate = it->second;
				labels->InsertNextTuple1(duplicate.first);	//domain 1
				labels->InsertNextTuple1(duplicate.second); //domain 2
				output_cell_list->InsertNextId(i);
				output_cell_list->InsertNextId(i);
				flip_cell_list->InsertNextId(output_cell_list->GetNumberOfIds() -
																		 1);
			}
			else
			{
				labels->InsertNextTuple1(label);
				output_cell_list->InsertNextId(i);
			}
		}
	}
	output->Allocate(this->Mesh, output_cell_list->GetNumberOfIds());
	output->CopyCells(this->Mesh, output_cell_list);

	for (i = 0; i < flip_cell_list->GetNumberOfIds(); i++)
	{
		vtkIdType id = flip_cell_list->GetId(i);
		assert(id >= 0 && id < output->GetNumberOfPolys());
		output->ReverseCell(id);
	}

	output->GetCellData()->AddArray(labels);
	labels->Delete();
	output_cell_list->Delete();
	flip_cell_list->Delete();
}

//----------------------------------------------------------------------------
void vtkEdgeCollapse::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------

using REAL = double;
// Labels that signify the result of triangle-triangle intersection test.
enum eInterresult {
	DISJOINT,
	INTERSECT,
	SHAREVERTEX,
	SHAREEDGE,
	SHAREFACE,
	TOUCHEDGE,
	TOUCHFACE,
	INTERVERT,
	INTEREDGE,
	INTERFACE,
	INTERTET,
	TRIEDGEINT,
	EDGETRIINT,
	COLLISIONFACE,
	INTERSUBSEG,
	INTERSUBFACE,
	BELOWHULL2
};
// Triangle-triangle intersection test
enum eInterresult edge_vert_col_inter(REAL*, REAL*, REAL*);
enum eInterresult edge_edge_cop_inter(REAL*, REAL*, REAL*, REAL*, REAL*);
enum eInterresult tri_vert_cop_inter(REAL*, REAL*, REAL*, REAL*, REAL*);
enum eInterresult tri_edge_cop_inter(REAL*, REAL*, REAL*, REAL*, REAL*, REAL*);
enum eInterresult tri_edge_inter_tail(REAL*, REAL*, REAL*, REAL*, REAL*, REAL, REAL);
enum eInterresult tri_edge_inter(REAL*, REAL*, REAL*, REAL*, REAL*);
enum eInterresult tri_tri_inter(REAL*, REAL*, REAL*, REAL*, REAL*, REAL*);

bool MESH::NoSelfIntersection(vtkPoints* mesh, std::vector<Triangle>& tris)
{
	double x1[3], x2[3], x3[3];
	double y1[3], y2[3], y3[3];

	vtkIdType ntris = static_cast<vtkIdType>(tris.size());
	for (vtkIdType i = 0; i < ntris - 1; i++)
	{
		mesh->GetPoint(tris[i].m_N1, x1);
		mesh->GetPoint(tris[i].m_N2, x2);
		mesh->GetPoint(tris[i].m_N3, x3);

		// the test is assumed to symmetric
		for (vtkIdType j = i + 1; j < ntris; j++)
		{
			std::set<int> nunion;
			nunion.insert(tris[i].m_N1);
			nunion.insert(tris[i].m_N2);
			nunion.insert(tris[i].m_N3);
			nunion.insert(tris[j].m_N1);
			nunion.insert(tris[j].m_N2);
			nunion.insert(tris[j].m_N3);
			// Assumption is that two triangles, which share an edge cannot self-intersect
			// -> why not avoid adding these triangles in the first place?
			if (nunion.size() == 4)
				continue;

			mesh->GetPoint(tris[j].m_N1, y1);
			mesh->GetPoint(tris[j].m_N2, y2);
			mesh->GetPoint(tris[j].m_N3, y3);

			eInterresult intersect = tri_tri_inter(x1, x2, x3, y1, y2, y3);

			if (intersect == INTERSECT || intersect == SHAREFACE)
			{
				return false;
			}
		} //for j
	}		//for i
	return true;
}

bool MESH::NoSelfIntersection(vtkPoints* mesh, std::vector<Triangle>& changedtris, std::vector<Triangle>& closetris)
{
	double x1[3], x2[3], x3[3];
	double y1[3], y2[3], y3[3];

	for (int i = 0; i < changedtris.size(); i++)
	{
		mesh->GetPoint(changedtris[i].m_N1, x1);
		mesh->GetPoint(changedtris[i].m_N2, x2);
		mesh->GetPoint(changedtris[i].m_N3, x3);

		for (int j = 0; j < closetris.size(); j++)
		{
			std::set<int> nunion;
			nunion.insert(changedtris[i].m_N1);
			nunion.insert(changedtris[i].m_N2);
			nunion.insert(changedtris[i].m_N3);
			nunion.insert(closetris[j].m_N1);
			nunion.insert(closetris[j].m_N2);
			nunion.insert(closetris[j].m_N3);
			// Assumption is that two triangles, which share an edge cannot self-intersect
			// -> why not avoid adding these triangles in the first place?
			if (nunion.size() == 4)
				continue;

			mesh->GetPoint(closetris[j].m_N1, y1);
			mesh->GetPoint(closetris[j].m_N2, y2);
			mesh->GetPoint(closetris[j].m_N3, y3);

			eInterresult intersect = tri_tri_inter(x1, x2, x3, y1, y2, y3);

			if (intersect == INTERSECT || intersect == SHAREFACE)
			{
				return false;
			}
		} //for j
	}		//for i
	return true;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Triangle-triangle intersection test                                       //
//                                                                           //
// The triangle-triangle intersection test is implemented with exact arithm- //
// etic. It exactly tells whether or not two triangles in three dimensions   //
// intersect.  Before implementing this test myself,  I tried two C codes    //
// (implemented by Thomas Moeller and Philippe Guigue, respectively), which  //
// are all public available. However both of them failed frequently. Another //
// unconvenience is both codes only tell whether or not the two triangles    //
// intersect without distinguishing the cases whether they exactly intersect //
// in interior or they just share a vertex or share an edge. The two latter  //
// cases are acceptable and should return not intersection in TetGen.        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// All the following routines require the input objects are not degenerate.
//   i.e., a triangle must has three non-collinear corners; an edge must
//   has two identical endpoints.  Degenerate cases should have to detect
//   first and then handled as special cases.

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// edge_vert_col_inter()    Test whether an edge (ab) and a collinear vertex //
//                          (p) are intersecting or not.                     //
//                                                                           //
// Possible cases are p is coincident to a (p = a), or to b (p = b), or p is //
// inside ab (a < p < b), or outside ab (p < a or p > b). These cases can be //
// quickly determined by comparing the corresponding coords of a, b, and p   //
// (which are not all equal).                                                //
//                                                                           //
// The return value indicates one of the three cases: DISJOINT, SHAREVERTEX  //
// (p = a or p = b), and INTERSECT (a < p < b).                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

enum eInterresult edge_vert_col_inter(REAL* A, REAL* B, REAL* P)
{
	int i = 0;
	do
	{
		if (A[i] < B[i])
		{
			if (P[i] < A[i])
			{
				return DISJOINT;
			}
			else if (P[i] > A[i])
			{
				if (P[i] < B[i])
				{
					return INTERSECT;
				}
				else if (P[i] > B[i])
				{
					return DISJOINT;
				}
				else
				{
					// assert(P[i] == B[i]);
					return SHAREVERTEX;
				}
			}
			else
			{
				// assert(P[i] == A[i]);
				return SHAREVERTEX;
			}
		}
		else if (A[i] > B[i])
		{
			if (P[i] < B[i])
			{
				return DISJOINT;
			}
			else if (P[i] > B[i])
			{
				if (P[i] < A[i])
				{
					return INTERSECT;
				}
				else if (P[i] > A[i])
				{
					return DISJOINT;
				}
				else
				{
					// assert(P[i] == A[i]);
					return SHAREVERTEX;
				}
			}
			else
			{
				// assert(P[i] == B[i]);
				return SHAREVERTEX;
			}
		}
		// i-th coordinates are equal, try i+1-th;
		i++;
	} while (i < 3);
	// Should never be here.
	return DISJOINT;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// edge_edge_cop_inter()    Test whether two coplanar edges (ab, and pq) are //
//                          intersecting or not.                             //
//                                                                           //
// Possible cases are ab and pq are disjointed, or proper intersecting (int- //
// ersect at a point other than their endpoints), or both collinear and int- //
// ersecting, or sharing at a common endpoint, or are coincident.            //
//                                                                           //
// A reference point R is required, which is exactly not coplanar with these //
// two edges.  Since the caller knows these two edges are coplanar, it must  //
// be able to provide (or calculate) such a point.                           //
//                                                                           //
// The return value indicates one of the four cases: DISJOINT, SHAREVERTEX,  //
// SHAREEDGE, and INTERSECT.                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

enum eInterresult edge_edge_cop_inter(REAL* A, REAL* B, REAL* P, REAL* Q, REAL* R)
{
	REAL s1, s2, s3, s4;

#ifdef SELF_CHECK
	assert(R != nullptr);
#endif
	s1 = Gp::Orient3d(A, B, R, P);
	s2 = Gp::Orient3d(A, B, R, Q);
	if (s1 * s2 > 0.0)
	{
		// Both p and q are at the same side of ab.
		return DISJOINT;
	}
	s3 = Gp::Orient3d(P, Q, R, A);
	s4 = Gp::Orient3d(P, Q, R, B);
	if (s3 * s4 > 0.0)
	{
		// Both a and b are at the same side of pq.
		return DISJOINT;
	}

	// Possible degenerate cases are:
	//   (1) Only one of p and q is collinear with ab;
	//   (2) Both p and q are collinear with ab;
	//   (3) Only one of a and b is collinear with pq.
	enum eInterresult abp, abq;
	enum eInterresult pqa, pqb;

	if (s1 == 0.0)
	{
		// p is collinear with ab.
		abp = edge_vert_col_inter(A, B, P);
		if (abp == INTERSECT)
		{
			// p is inside ab.
			return INTERSECT;
		}
		if (s2 == 0.0)
		{
			// q is collinear with ab. Case (2).
			abq = edge_vert_col_inter(A, B, Q);
			if (abq == INTERSECT)
			{
				// q is inside ab.
				return INTERSECT;
			}
			if (abp == SHAREVERTEX && abq == SHAREVERTEX)
			{
				// ab and pq are identical.
				return SHAREEDGE;
			}
			pqa = edge_vert_col_inter(P, Q, A);
			if (pqa == INTERSECT)
			{
				// a is inside pq.
				return INTERSECT;
			}
			pqb = edge_vert_col_inter(P, Q, B);
			if (pqb == INTERSECT)
			{
				// b is inside pq.
				return INTERSECT;
			}
			if (abp == SHAREVERTEX || abq == SHAREVERTEX)
			{
				// either p or q is coincident with a or b.
#ifdef SELF_CHECK
				// ONLY one case is possible, otherwise, shoule be SHAREEDGE.
				assert(abp ^ abq);
#endif
				return SHAREVERTEX;
			}
			// The last case. They are disjointed.
#ifdef SELF_CHECK
			assert((abp == DISJOINT) &&
						 (abp == abq && abq == pqa && pqa == pqb));
#endif
			return DISJOINT;
		}
		else
		{
			// p is collinear with ab. Case (1).
#ifdef SELF_CHECK
			assert(abp == SHAREVERTEX || abp == DISJOINT);
#endif
			return abp;
		}
	}
	// p is NOT collinear with ab.
	if (s2 == 0.0)
	{
		// q is collinear with ab. Case (1).
		abq = edge_vert_col_inter(A, B, Q);
#ifdef SELF_CHECK
		assert(abq == SHAREVERTEX || abq == DISJOINT || abq == INTERSECT);
#endif
		return abq;
	}

	// We have found p and q are not collinear with ab. However, it is still
	//   possible that a or b is collinear with pq (ONLY one of a and b).
	if (s3 == 0.0)
	{
		// a is collinear with pq. Case (3).
#ifdef SELF_CHECK
		assert(s4 != 0.0);
#endif
		pqa = edge_vert_col_inter(P, Q, A);
#ifdef SELF_CHECK
		// This case should have been detected in above.
		assert(pqa != SHAREVERTEX);
		assert(pqa == INTERSECT || pqa == DISJOINT);
#endif
		return pqa;
	}
	if (s4 == 0.0)
	{
		// b is collinear with pq. Case (3).
#ifdef SELF_CHECK
		assert(s3 != 0.0);
#endif
		pqb = edge_vert_col_inter(P, Q, B);
#ifdef SELF_CHECK
		// This case should have been detected in above.
		assert(pqb != SHAREVERTEX);
		assert(pqb == INTERSECT || pqb == DISJOINT);
#endif
		return pqb;
	}

	// ab and pq are intersecting properly.
	return INTERSECT;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Notations                                                                 //
//                                                                           //
// Let ABC be the plane passes through a, b, and c;  ABC+ be the halfspace   //
// including the set of all points x, such that gp::orient3d(a, b, c, x) > 0;    //
// ABC- be the other halfspace, such that for each point x in ABC-,          //
// gp::orient3d(a, b, c, x) < 0.  For the set of x which are on ABC, gp::orient3d(a, //
// b, c, x) = 0.                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// tri_vert_copl_inter()    Test whether a triangle (abc) and a coplanar     //
//                          point (p) are intersecting or not.               //
//                                                                           //
// Possible cases are p is inside abc, or on an edge of, or coincident with  //
// a vertex of, or outside abc.                                              //
//                                                                           //
// A reference point R is required. R is exactly not coplanar with abc and p.//
// Since the caller knows they are coplanar, it must be able to provide (or  //
// calculate) such a point.                                                  //
//                                                                           //
// The return value indicates one of the four cases: DISJOINT, SHAREVERTEX,  //
// and INTERSECT.                                                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

enum eInterresult tri_vert_cop_inter(REAL* A, REAL* B, REAL* C, REAL* P, REAL* R)
{
	REAL s1, s2, s3;
	int sign;

#ifdef SELF_CHECK
	assert(R != (REAL*)nullptr);
#endif
	// Adjust the orientation of a, b, c and r, so that we can assume that
	//   r is strictly in ABC- (i.e., r is above ABC wrt. right-hand rule).
	s1 = Gp::Orient3d(A, B, C, R);
#ifdef SELF_CHECK
	assert(s1 != 0.0);
#endif
	sign = s1 < 0.0 ? 1 : -1;

	// Test starts from here.
	s1 = Gp::Orient3d(A, B, R, P) * sign;
	if (s1 < 0.0)
	{
		// p is in ABR-.
		return DISJOINT;
	}
	s2 = Gp::Orient3d(B, C, R, P) * sign;
	if (s2 < 0.0)
	{
		// p is in BCR-.
		return DISJOINT;
	}
	s3 = Gp::Orient3d(C, A, R, P) * sign;
	if (s3 < 0.0)
	{
		// p is in CAR-.
		return DISJOINT;
	}
	if (s1 == 0.0)
	{
		// p is on ABR.
		if (s2 == 0.0)
		{
			// p is on BCR.
#ifdef SELF_CHECK
			assert(s3 > 0.0);
#endif
			// p is coincident with b.
			return SHAREVERTEX;
		}
		if (s3 == 0.0)
		{
			// p is on CAR.
			// p is coincident with a.
			return SHAREVERTEX;
		}
		// p is on edge ab.
		return INTERSECT;
	}
	// p is in ABR+.
	if (s2 == 0.0)
	{
		// p is on BCR.
		if (s3 == 0.0)
		{
			// p is on CAR.
			// p is coincident with c.
			return SHAREVERTEX;
		}
		// p is on edge bc.
		return INTERSECT;
	}
	if (s3 == 0.0)
	{
		// p is on CAR.
		// p is on edge ca.
		return INTERSECT;
	}

	// p is strictly inside abc.
	return INTERSECT;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// tri_edge_cop_inter()    Test whether a triangle (abc) and a coplanar edge //
//                         (pq) are intersecting or not.                     //
//                                                                           //
// A reference point R is required. R is exactly not coplanar with abc and   //
// pq.  Since the caller knows they are coplanar, it must be able to provide //
// (or calculate) such a point.                                              //
//                                                                           //
// The return value indicates one of the four cases: DISJOINT, SHAREVERTEX,  //
// SHAREEDGE, and INTERSECT.                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

enum eInterresult tri_edge_cop_inter(REAL* A, REAL* B, REAL* C, REAL* P, REAL* Q, REAL* R)
{
	enum eInterresult abpq, bcpq, capq;
	enum eInterresult abcp, abcq;

	// Test if pq is intersecting one of edges of abc.
	abpq = edge_edge_cop_inter(A, B, P, Q, R);
	if (abpq == INTERSECT || abpq == SHAREEDGE)
	{
		return abpq;
	}
	bcpq = edge_edge_cop_inter(B, C, P, Q, R);
	if (bcpq == INTERSECT || bcpq == SHAREEDGE)
	{
		return bcpq;
	}
	capq = edge_edge_cop_inter(C, A, P, Q, R);
	if (capq == INTERSECT || capq == SHAREEDGE)
	{
		return capq;
	}

	// Test if p and q is inside abc.
	abcp = tri_vert_cop_inter(A, B, C, P, R);
	if (abcp == INTERSECT)
	{
		return INTERSECT;
	}
	abcq = tri_vert_cop_inter(A, B, C, Q, R);
	if (abcq == INTERSECT)
	{
		return INTERSECT;
	}

	// Combine the test results of edge intersectings and triangle insides
	//   to detect whether abc and pq are sharing vertex or disjointed.
	if (abpq == SHAREVERTEX)
	{
		// p or q is coincident with a or b.
#ifdef SELF_CHECK
		assert(abcp ^ abcq);
#endif
		return SHAREVERTEX;
	}
	if (bcpq == SHAREVERTEX)
	{
		// p or q is coincident with b or c.
#ifdef SELF_CHECK
		assert(abcp ^ abcq);
#endif
		return SHAREVERTEX;
	}
	if (capq == SHAREVERTEX)
	{
		// p or q is coincident with c or a.
#ifdef SELF_CHECK
		assert(abcp ^ abcq);
#endif
		return SHAREVERTEX;
	}

	// They are disjointed.
	return DISJOINT;
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// tri_edge_inter_tail()    Test whether a triangle (abc) and an edge (pq)   //
//                          are intersecting or not.                         //
//                                                                           //
// s1 and s2 are results of pre-performed orientation tests. s1 = gp::orient3d(  //
// a, b, c, p); s2 = gp::orient3d(a, b, c, q).  To separate this routine from    //
// tri_edge_inter() can save two orientation tests in tri_tri_inter().       //
//                                                                           //
// The return value indicates one of the four cases: DISJOINT, SHAREVERTEX,  //
// SHAREEDGE, and INTERSECT.                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

enum eInterresult tri_edge_inter_tail(REAL* A, REAL* B, REAL* C, REAL* P, REAL* Q, REAL s1, REAL s2)
{
	REAL s3, s4, s5;
	int sign;

	if (s1 * s2 > 0.0)
	{
		// p, q are at the same halfspace of ABC, no intersection.
		return DISJOINT;
	}

	if (s1 * s2 < 0.0)
	{
		// p, q are both not on ABC (and not sharing vertices, edges of abc).
		// Adjust the orientation of a, b, c and p, so that we can assume that
		//   p is strictly in ABC-, and q is strictly in ABC+.
		sign = s1 < 0.0 ? 1 : -1;
		s3 = Gp::Orient3d(A, B, P, Q) * sign;
		if (s3 < 0.0)
		{
			// q is at ABP-.
			return DISJOINT;
		}
		s4 = Gp::Orient3d(B, C, P, Q) * sign;
		if (s4 < 0.0)
		{
			// q is at BCP-.
			return DISJOINT;
		}
		s5 = Gp::Orient3d(C, A, P, Q) * sign;
		if (s5 < 0.0)
		{
			// q is at CAP-.
			return DISJOINT;
		}
		if (s3 == 0.0)
		{
			// q is on ABP.
			if (s4 == 0.0)
			{
				// q is on BCP (and q must in CAP+).
#ifdef SELF_CHECK
				assert(s5 > 0.0);
#endif
				// pq intersects abc at vertex b.
				return SHAREVERTEX;
			}
			if (s5 == 0.0)
			{
				// q is on CAP (and q must in BCP+).
				// pq intersects abc at vertex a.
				return SHAREVERTEX;
			}
			// q in both BCP+ and CAP+.
			// pq crosses ab properly.
			return INTERSECT;
		}
		// q is in ABP+;
		if (s4 == 0.0)
		{
			// q is on BCP.
			if (s5 == 0.0)
			{
				// q is on CAP.
				// pq intersects abc at vertex c.
				return SHAREVERTEX;
			}
			// pq crosses bc properly.
			return INTERSECT;
		}
		// q is in BCP+;
		if (s5 == 0.0)
		{
			// q is on CAP.
			// pq crosses ca properly.
			return INTERSECT;
		}
		// q is in CAP+;
		// pq crosses abc properly.
		return INTERSECT;
	}

	if (s1 != 0.0 || s2 != 0.0)
	{
		// Either p or q is coplanar with abc. ONLY one of them is possible.
		if (s1 == 0.0)
		{
			// p is coplanar with abc, q can be used as reference point.
#ifdef SELF_CHECK
			assert(s2 != 0.0);
#endif
			return tri_vert_cop_inter(A, B, C, P, Q);
		}
		else
		{
			// q is coplanar with abc, p can be used as reference point.
#ifdef SELF_CHECK
			assert(s2 == 0.0);
#endif
			return tri_vert_cop_inter(A, B, C, Q, P);
		}
	}

	// pq is coplanar with abc.  Calculate a point which is exactly not
	//   coplanar with a, b, and c.
	REAL r[3], n[3];
	REAL ax, ay, az, bx, by, bz;
	REAL epsilon = Gp::Getepsilon();

	ax = A[0] - B[0];
	ay = A[1] - B[1];
	az = A[2] - B[2];
	bx = A[0] - C[0];
	by = A[1] - C[1];
	bz = A[2] - C[2];
	n[0] = ay * bz - by * az;
	n[1] = az * bx - bz * ax;
	n[2] = ax * by - bx * ay;
	// The normal should not be a zero vector (otherwise, abc are collinear).
#ifdef SELF_CHECK
	assert((fabs(N[0]) + fabs(N[1]) + fabs(N[2])) > 0.0);
#endif
	// The reference point R is lifted from A to the normal direction with
	//   a distance d = average edge length of the triangle abc.
	r[0] = n[0] + A[0];
	r[1] = n[1] + A[1];
	r[2] = n[2] + A[2];
	// Becareful the case: if the non-zero component(s) in N is smaller than
	//   the machine epsilon (i.e., 2^(-16) for double), R will exactly equal
	//   to A due to the round-off error.  Do check if it is.
	if (r[0] == A[0] && r[1] == A[1] && r[2] == A[2])
	{
		int i, j;
		for (i = 0; i < 3; i++)
		{
#ifdef SELF_CHECK
			assert(R[i] == A[i]);
#endif
			j = 2;
			do
			{
				if (n[i] > 0.0)
				{
					n[i] += (j * epsilon);
				}
				else
				{
					n[i] -= (j * epsilon);
				}
				r[i] = n[i] + A[i];
				j *= 2;
			} while (r[i] == A[i]);
		}
	}

	return tri_edge_cop_inter(A, B, C, P, Q, r);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// tri_edge_inter()    Test whether a triangle (abc) and an edge (pq) are    //
//                     intersecting or not.                                  //
//                                                                           //
// The return value indicates one of the four cases: DISJOINT, SHAREVERTEX,  //
// SHAREEDGE, and INTERSECT.                                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

enum eInterresult tri_edge_inter(REAL* A, REAL* B, REAL* C, REAL* P, REAL* Q)
{
	REAL s1, s2;

	// Test the locations of p and q with respect to ABC.
	s1 = Gp::Orient3d(A, B, C, P);
	s2 = Gp::Orient3d(A, B, C, Q);

	return tri_edge_inter_tail(A, B, C, P, Q, s1, s2);
}

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// tri_tri_inter()    Test whether two triangle (abc) and (opq) are          //
//                    intersecting or not.                                   //
//                                                                           //
// The return value indicates one of the five cases: DISJOINT, SHAREVERTEX,  //
// SHAREEDGE, SHAREFACE, and INTERSECT.                                      //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

enum eInterresult tri_tri_inter(REAL* A, REAL* B, REAL* C, REAL* O, REAL* P, REAL* Q)
{
	REAL s_o, s_p, s_q;
	REAL s_a, s_b, s_c;

	s_o = Gp::Orient3d(A, B, C, O);
	s_p = Gp::Orient3d(A, B, C, P);
	s_q = Gp::Orient3d(A, B, C, Q);
	if ((s_o * s_p > 0.0) && (s_o * s_q > 0.0))
	{
		// o, p, q are all in the same halfspace of ABC.
		return DISJOINT;
	}

	s_a = Gp::Orient3d(O, P, Q, A);
	s_b = Gp::Orient3d(O, P, Q, B);
	s_c = Gp::Orient3d(O, P, Q, C);
	if ((s_a * s_b > 0.0) && (s_a * s_c > 0.0))
	{
		// a, b, c are all in the same halfspace of OPQ.
		return DISJOINT;
	}

	enum eInterresult abcop, abcpq, abcqo;
	int shareedge = 0;

	abcop = tri_edge_inter_tail(A, B, C, O, P, s_o, s_p);
	if (abcop == INTERSECT)
	{
		return INTERSECT;
	}
	else if (abcop == SHAREEDGE)
	{
		shareedge++;
	}
	abcpq = tri_edge_inter_tail(A, B, C, P, Q, s_p, s_q);
	if (abcpq == INTERSECT)
	{
		return INTERSECT;
	}
	else if (abcpq == SHAREEDGE)
	{
		shareedge++;
	}
	abcqo = tri_edge_inter_tail(A, B, C, Q, O, s_q, s_o);
	if (abcqo == INTERSECT)
	{
		return INTERSECT;
	}
	else if (abcqo == SHAREEDGE)
	{
		shareedge++;
	}
	if (shareedge == 3)
	{
		// opq are coincident with abc.
		return SHAREFACE;
	}
#ifdef SELF_CHECK
	// It is only possible either no share edge or one.
	assert(shareedge == 0 || shareedge == 1);
#endif

	// Continue to detect whether opq and abc are intersecting or not.
	enum eInterresult opqab, opqbc, opqca;

	opqab = tri_edge_inter_tail(O, P, Q, A, B, s_a, s_b);
	if (opqab == INTERSECT)
	{
		return INTERSECT;
	}
	opqbc = tri_edge_inter_tail(O, P, Q, B, C, s_b, s_c);
	if (opqbc == INTERSECT)
	{
		return INTERSECT;
	}
	opqca = tri_edge_inter_tail(O, P, Q, C, A, s_c, s_a);
	if (opqca == INTERSECT)
	{
		return INTERSECT;
	}

	// At this point, two triangles are not intersecting and not coincident.
	//   They may be share an edge, or share a vertex, or disjoint.
	if (abcop == SHAREEDGE)
	{
#ifdef SELF_CHECK
		assert(abcpq == SHAREVERTEX && abcqo == SHAREVERTEX);
#endif
		// op is coincident with an edge of abc.
		return SHAREEDGE;
	}
	if (abcpq == SHAREEDGE)
	{
#ifdef SELF_CHECK
		assert(abcop == SHAREVERTEX && abcqo == SHAREVERTEX);
#endif
		// pq is coincident with an edge of abc.
		return SHAREEDGE;
	}
	if (abcqo == SHAREEDGE)
	{
#ifdef SELF_CHECK
		assert(abcop == SHAREVERTEX && abcpq == SHAREVERTEX);
#endif
		// qo is coincident with an edge of abc.
		return SHAREEDGE;
	}

	// They may share a vertex or disjoint.
	if (abcop == SHAREVERTEX)
	{
		// o or p is coincident with a vertex of abc.
		if (abcpq == SHAREVERTEX)
		{
			// p is the coincident vertex.
#ifdef SELF_CHECK
			assert(abcqo != SHAREVERTEX);
#endif
		}
		else
		{
			// o is the coincident vertex.
#ifdef SELF_CHECK
			assert(abcqo == SHAREVERTEX);
#endif
		}
		return SHAREVERTEX;
	}
	if (abcpq == SHAREVERTEX)
	{
		// q is the coincident vertex.
#ifdef SELF_CHECK
		assert(abcqo == SHAREVERTEX);
#endif
		return SHAREVERTEX;
	}

	// They are disjoint.
	return DISJOINT;
}
