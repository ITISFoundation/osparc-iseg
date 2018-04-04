/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef vtkEdgeCollapse_h
#define vtkEdgeCollapse_h

#include "vtkPolyDataAlgorithm.h"
#include <map>
#include <vector>

class vtkPolyData;
class vtkEdgeTable;
class vtkIdList;
class vtkPriorityQueue;
class vtkGenericCell;
class vtkAbstractCellLocator;
class vtkFloatArray;

/**
This filter implements a strategy to collapse edges in a surface mesh. Optionally it can flip the edges 
after collapsing in order to improve the angles. Maybe more important is the feature that self-intersection
tests can be performed (if IntersectionCheckLevel > 0).

Further reading (not implemented):
http://www.inf.tu-dresden.de/content/institutes/smt/cg/publications/paper/gumhold-2003-intersection.pdf

Options:

- MinimumEdgeLength: all edges with length < MinimumEdgeLength will be collapsed (if collapse is legal)

- MaximumNormalAngleDeviation: the maximum allowed angle deviation between the triangle normal before & after collapsing

- FlipEdges: flip edges in order to improve angles of triangles (if flip is legal)

- MeshIsManifold: default=0, assumes mesh is closed manifold surface (performs more checks if MeshIsManifold=1)

- IntersectionCheckLevel: self-intersections can occur due to collapsing/flipping edges. This can be avoided by increasing this parameter.

 */
class vtkEdgeCollapse : public vtkPolyDataAlgorithm
{
public:
	vtkTypeMacro(vtkEdgeCollapse, vtkPolyDataAlgorithm);
	void PrintSelf(ostream &os, vtkIndent indent);
	static vtkEdgeCollapse *New();

	// Edges with edge length shorter than MinimumEdgeLength are collapsed
	vtkSetMacro(MinimumEdgeLength, double);
	vtkGetMacro(MinimumEdgeLength, double);

	// This parameter sets the maximum angle in degrees between the original normal
	// and the normal of the resulting new triangles after collapse/flip operations
	vtkSetMacro(MaximumNormalAngleDeviation, double);
	vtkGetMacro(MaximumNormalAngleDeviation, double);

	// Set the level of checks performed to avoid self-intersections
	// Higher levels, e.g. 3 or 4, will cause more triangles to be included
	// in the checks.
	vtkSetClampMacro(IntersectionCheckLevel, int, 0, 4);
	vtkGetMacro(IntersectionCheckLevel, int);

	// This is only used if IntersectionCheckLevel==4, it controls the number of
	// closest points to be included in the self-intersection checks
	vtkSetMacro(NumberOfClosestPoints, int);
	vtkGetMacro(NumberOfClosestPoints, int);

	// Flip edges tries to improve the delaunay property for two adjacent
	// triangles, by flipping the edge.
	vtkSetMacro(FlipEdges, int);
	vtkGetMacro(FlipEdges, int);
	vtkBooleanMacro(FlipEdges, int);

	// Allow splitting of edges, which are larger than MaximumEdgeLength
	vtkSetMacro(UseMaximumEdgeLength, int);
	vtkGetMacro(UseMaximumEdgeLength, int);
	vtkBooleanMacro(UseMaximumEdgeLength, int);

	// Edges with edge length larger than MaximumEdgeLength are divided
	// This parameter is only used if UseMaximumEdgeLength is on.
	vtkSetMacro(MaximumEdgeLength, double);
	vtkGetMacro(MaximumEdgeLength, double);

	// Assume the mesh is closed and manifold or not. More checks are done if you set MeshIsManifold ot off
	vtkSetMacro(MeshIsManifold, int);
	vtkGetMacro(MeshIsManifold, int);
	vtkBooleanMacro(MeshIsManifold, int);

	// You can provide multi-domain surface mesh input, which has duplicate triangles
	// at the interface between two domains. The DomainLabelName is the name of the data array.
	vtkSetStringMacro(DomainLabelName);
	vtkGetStringMacro(DomainLabelName);

	// Get some information after running the filter
	vtkGetMacro(NumberOfEdgeCollapses, int);
	vtkGetMacro(NumberOfEdgeFlips, int);
	vtkGetMacro(ActualReduction, double);
	vtkGetMacro(InputIsNonmanifold, int);

	vtkSetMacro(Loud, int);
	vtkBooleanMacro(Loud, int);

protected:
	vtkEdgeCollapse();
	~vtkEdgeCollapse();

	// Implementation of algorithm
	int RequestData(vtkInformation *, vtkInformationVector **,
									vtkInformationVector *);

	// Find all edges that will have an endpoint change ids because of an edge
	// collapse.  p1Id and p2Id are the endpoints of the edge.  p2Id is the
	// pointId being removed.
	void FindAffectedEdges(vtkIdType p1Id, vtkIdType p2Id, vtkIdList *edges);

	// Update the edge priority queue after a collapse
	void UpdateEdgeData(vtkIdType p1Id, vtkIdType p2Id);

	// Test if Collapse is legal (topological & geometrical & intersection checks)
	bool IsCollapseLegal(vtkIdType p1Id, vtkIdType p2Id);

	// Do edge collapse (p2Id is removed)
	int CollapseEdge(vtkIdType p1Id, vtkIdType p2Id);

	// Helper function
	bool IsDegenerateTriangle(vtkIdType i0, vtkIdType i1, vtkIdType i2);

	// Compute triangle normals on input surface
	void ComputeNormals(vtkPolyData *);

	// Get normal at point closest to point
	int GetOriginalNormal(double x[3], double normal[3]);

	// Compute if mesh is manifold and store which points are on a non-manifold edge
	void ComputeManifoldedness();

	// Flip edges to improve triangle angles
	int DelaunayFlipEdges();

	// Returns the angle in degrees at first node of the triangle
	double ComputeAngleAtFirstPoint(vtkIdType i0, vtkIdType i1, vtkIdType i2);

	// Returns the point id not equals i0 or i1
	vtkIdType FindThirdNode(vtkIdType i0, vtkIdType i1, const vtkIdType pts[3]);

	// Subdivide edges longer than MaximumEdgeLength
	// implements a longest edge first subdivision strategy
	int SubdivideEdges();

	// If input surface has duplicate triangles (multi-domain interface triangles)
	// then one copied and the other is set to empty. The 2 domain labels are mapped
	// to a key value, which is later used in CreateDuplicateTriangles to reconstruct
	// the domain-interface
	void MapDuplicateTriangles(vtkDataArray *labels);

	// After doing collapse, create duplicates again
	void CreateDuplicateTriangles(vtkPolyData *);

	double MinimumEdgeLength;
	double MaximumNormalAngleDeviation;
	int FlipEdges;
	int UseMaximumEdgeLength;
	double MaximumEdgeLength;
	int MeshIsManifold;
	int Loud;
	int IntersectionCheckLevel;
	int NumberOfClosestPoints;
	char *DomainLabelName;
	int NumberOfEdgeCollapses;
	int NumberOfEdgeFlips;
	int NumberOfEdgeDivisions;
	double ActualReduction;
	int InputIsNonmanifold;

	vtkPolyData *Mesh;
	vtkDataArray *Labels;
	vtkEdgeTable *Edges;
	vtkPriorityQueue *EdgeCosts;
	vtkIdList *EndPoint1List;
	vtkIdList *EndPoint2List;
	vtkIdList *CollapseCellIds;
	vtkIdList *Neighbors;
	vtkIdList *PointIds;
	vtkFloatArray *Normals;
	vtkGenericCell *GCell;
	vtkAbstractCellLocator *CellLocator;

	double MinLength2;
	double NormalDotProductThreshold;

	//BTX
	std::vector<bool> isboundary;
	typedef std::pair<int, int> DuplicateLabel;
	typedef std::map<DuplicateLabel, int> LabelMapType;
	typedef std::map<int, DuplicateLabel> InverseLabelMapType;
	InverseLabelMapType ilabelmap;
	//ETX

private:
	vtkEdgeCollapse(const vtkEdgeCollapse &); // Not implemented.
	void operator=(const vtkEdgeCollapse &);	// Not implemented.
};

#endif
