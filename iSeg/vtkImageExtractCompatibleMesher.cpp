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

#include "vtkImageExtractCompatibleMesher.h"
#include "vtkTemplateTriangulator.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <ctime>
#include <map>
#include <set>
#include <stdexcept>
#include <vector>

#include <vtkExecutive.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkObjectFactory.h>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkIdList.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkShortArray.h>

#include <vtkEdgeTable.h>
#include <vtkGenericCell.h>
#include <vtkIncrementalOctreePointLocator.h>
#include <vtkMath.h>
#include <vtkMergePoints.h>
#include <vtkOrderedTriangulator.h>
#include <vtkPriorityQueue.h>

#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <vtkTimerLog.h>

#include <vtkSmartPointer.h>
#define vtkNew(type, name)                                                     \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

template<int Id> class Timer
{
public:
	Timer() { c0 = clock(); }
	~Timer() { total_clocks += (clock() - c0); }
	static double EllapsedTime() { return total_clocks / CLOCKS_PER_SEC; }
	static void Reset() { total_clocks = 0; }

protected:
	clock_t c0;
	static double total_clocks;
};

template<int Id> double Timer<Id>::total_clocks = 0.0;

static int tet_faces[4][3] = {{0, 1, 3}, {0, 2, 1}, {0, 3, 2}, {1, 2, 3}};

typedef unsigned int IDType;
// triangle with less than operator
struct Triangle
{
	void Sort()
	{
		if (n1 > n2)
			std::swap(n1, n2);
		if (n1 > n3)
			std::swap(n1, n3);
		if (n2 > n3)
			std::swap(n2, n3);
		assert(n1 < n2);
		assert(n1 < n3);
		assert(n2 < n3);
	}
	bool operator<(const Triangle& rhs) const
	{
		if (n1 != rhs.n1)
			return (n1 < rhs.n1);
		else if (n2 != rhs.n2)
			return (n2 < rhs.n2);
		else
			return (n3 < rhs.n3);
	}
	bool operator==(const Triangle& rhs) const
	{
		return (n1 == rhs.n1 && n2 == rhs.n2 && n3 == rhs.n3);
	}
	IDType n1, n2, n3;
};
// store the ids of a tet
class Tetrahedron
{
public:
	Tetrahedron(IDType i0, IDType i1, IDType i2, IDType i3)
	{
		n0 = i0;
		n1 = i1;
		n2 = i2;
		n3 = i3;
	}
	IDType operator[](size_t i) const
	{
		if (i == 0)
			return n0;
		else if (i == 1)
			return n1;
		else if (i == 2)
			return n2;
		else /*(i==3)*/
			return n3;
	}
	int WhichTriangle(Triangle& tri)
	{
		Triangle face;
		for (int k = 0; k < 4; k++)
		{
			face.n1 = (*this)[tet_faces[k][0]];
			face.n2 = (*this)[tet_faces[k][1]];
			face.n3 = (*this)[tet_faces[k][2]];
			face.Sort();
			if (face == tri)
				return k;
		}
		return -1;
	}

private:
	IDType n0, n1, n2, n3;
};
// store the tet face neighbors
class TetNeighbors
{
public:
	TetNeighbors() { cell0 = cell1 = cell2 = cell3 = VTK_UNSIGNED_INT_MAX; };
	void SetNeighbor(int k, IDType id)
	{
		assert(k >= 0 && k < 4);
		if (k == 0)
			cell0 = id;
		else if (k == 1)
			cell1 = id;
		else if (k == 2)
			cell2 = id;
		else /*(k==3)*/
			cell3 = id;
	}
	IDType GetNeighbor(int k) const
	{
		assert(k >= 0 && k < 4);
		if (k == 0)
			return cell0;
		else if (k == 1)
			return cell1;
		else if (k == 2)
			return cell2;
		else /*(k==3)*/
			return cell3;
	}
	bool IsAssigned(IDType id) const { return (id != VTK_UNSIGNED_INT_MAX); }

private:
	IDType cell0, cell1, cell2, cell3;
};
// store the tetrahedra and neighborhood information
class TetContainer
{
public:
	TetContainer(size_t estimatedSize, size_t extensionSize)
	{
		tetra.reserve(estimatedSize);
		ExtensionSize = extensionSize;
	}
	void push_back(IDType i0, IDType i1, IDType i2, IDType i3)
	{
		if (tetra.size() + 1 >= tetra.capacity())
			tetra.reserve(tetra.size() + ExtensionSize);
		tetra.push_back(Tetrahedron(i0, i1, i2, i3));
	}
	size_t GetNumberOfTetrahedra() const { return tetra.size(); }
	const Tetrahedron& GetTetrahedron(size_t cellId) const
	{
		return tetra[cellId];
	}

	bool HasNeighbor(size_t cellId, int k) const
	{
		return (neighbors[cellId].GetNeighbor(k) != VTK_UNSIGNED_INT_MAX);
	}
	IDType GetNeighbor(size_t cellId, int k) const
	{
		return neighbors[cellId].GetNeighbor(k);
	}
	bool BuildNeighbors()
	{
		std::cerr << "Building neighborhood information" << std::endl;
		// allocate memory
		try
		{
			neighbors.resize(tetra.size());
		}
		catch (std::length_error& /*e*/)
		{
			return false;
		}
		catch (std::bad_alloc& /*e*/)
		{
			return false;
		}
		catch (...)
		{
			return false;
		}
		size_t max_unsigned_int = 4294967295;
		if (tetra.size() >= max_unsigned_int)
		{
			return false;
		}

		// iterate through tetrahedra and create neighbors
		typedef std::map<Triangle, IDType> triangle_id_map;
		triangle_id_map tmap;
		triangle_id_map::iterator tmap_it;
		//std::vector<IDType> t1(3), t2(3), t3(3), t4(3);
		Triangle t1, t2, t3, t4;
		std::vector<const Triangle*> faces(4);
		IDType NC = static_cast<IDType>(tetra.size());
		for (IDType cellId = 0; cellId < NC; cellId++)
		{
			const Tetrahedron& tet = tetra[cellId];
			// get four triangle faces
			t1.n1 = tet[tet_faces[0][0]];
			t1.n2 = tet[tet_faces[0][1]];
			t1.n3 = tet[tet_faces[0][2]];
			t1.Sort();

			t2.n1 = tet[tet_faces[1][0]];
			t2.n2 = tet[tet_faces[1][1]];
			t2.n3 = tet[tet_faces[1][2]];
			t2.Sort();

			t3.n1 = tet[tet_faces[2][0]];
			t3.n2 = tet[tet_faces[2][1]];
			t3.n3 = tet[tet_faces[2][2]];
			t3.Sort();

			t4.n1 = tet[tet_faces[3][0]];
			t4.n2 = tet[tet_faces[3][1]];
			t4.n3 = tet[tet_faces[3][2]];
			t4.Sort();

			//t1
			tmap_it = tmap.find(t1);
			if (tmap_it == tmap.end())
			{ // add first
				tmap[t1] = cellId;
			}
			else
			{ //found second
				assert(tmap_it->second != cellId);
				const IDType neighborId = tmap_it->second;
				const int j = tetra[neighborId].WhichTriangle(t1);
				assert(j >= 0);
				neighbors[cellId].SetNeighbor(0, neighborId);
				neighbors[neighborId].SetNeighbor(j, cellId);
				tmap.erase(tmap_it);
			}

			// t2
			tmap_it = tmap.find(t2);
			if (tmap_it == tmap.end())
			{ // add first
				tmap[t2] = cellId;
			}
			else
			{ //found second
				assert(tmap_it->second != cellId);
				const IDType neighborId = tmap_it->second;
				const int j = tetra[neighborId].WhichTriangle(t2);
				assert(j >= 0);
				neighbors[cellId].SetNeighbor(1, neighborId);
				neighbors[neighborId].SetNeighbor(j, cellId);
				tmap.erase(tmap_it);
			}

			// t3
			tmap_it = tmap.find(t3);
			if (tmap_it == tmap.end())
			{ // add first
				tmap[t3] = cellId;
			}
			else
			{ //found second
				assert(tmap_it->second != cellId);
				const IDType neighborId = tmap_it->second;
				const int j = tetra[neighborId].WhichTriangle(t3);
				assert(j >= 0);
				neighbors[cellId].SetNeighbor(2, neighborId);
				neighbors[neighborId].SetNeighbor(j, cellId);
				tmap.erase(tmap_it);
			}

			// t4
			tmap_it = tmap.find(t4);
			if (tmap_it == tmap.end())
			{ // add first
				tmap[t4] = cellId;
			}
			else
			{ //found second
				assert(tmap_it->second != cellId);
				const IDType neighborId = tmap_it->second;
				const int j = tetra[neighborId].WhichTriangle(t4);
				assert(j >= 0);
				neighbors[cellId].SetNeighbor(3, neighborId);
				neighbors[neighborId].SetNeighbor(j, cellId);
				tmap.erase(tmap_it);
			}
		}

		std::cerr << "tmap size: " << tmap.size() << std::endl;
		for (IDType cellId = 0; cellId < NC; cellId++)
		{
			const Tetrahedron& tet = tetra[cellId];
			// get four triangle faces
			t1.n1 = tet[tet_faces[0][0]];
			t1.n2 = tet[tet_faces[0][1]];
			t1.n3 = tet[tet_faces[0][2]];
			t1.Sort();

			t2.n1 = tet[tet_faces[1][0]];
			t2.n2 = tet[tet_faces[1][1]];
			t2.n3 = tet[tet_faces[1][2]];
			t2.Sort();

			t3.n1 = tet[tet_faces[2][0]];
			t3.n2 = tet[tet_faces[2][1]];
			t3.n3 = tet[tet_faces[2][2]];
			t3.Sort();

			t4.n1 = tet[tet_faces[3][0]];
			t4.n2 = tet[tet_faces[3][1]];
			t4.n3 = tet[tet_faces[3][2]];
			t4.Sort();

			faces[0] = &t1;
			faces[1] = &t2;
			faces[2] = &t3;
			faces[3] = &t4;

			triangle_id_map::iterator tmap_it;
			for (int j = 0; j < 4; j++)
			{
				// internal triangles were already removed from map above
				if ((tmap_it = tmap.find(*faces[j])) == tmap.end())
					continue;

				IDType tid1 = tmap_it->second;
				assert(tid1 >= 0 && tid1 < NC);
				if (tid1 != cellId)
				{
					neighbors[cellId].SetNeighbor(j, tid1);
				}
				else
				{
					assert(tid1 == cellId);
				}
			}
		}

		return true;
	}

private:
	size_t ExtensionSize;
	std::vector<Tetrahedron> tetra;
	std::vector<TetNeighbors> neighbors;
};

class vtkTriangulatorImpl : public vtkTemplateTriangulator
{
public:
	static vtkTriangulatorImpl* New();
	vtkTypeMacro(vtkTriangulatorImpl, vtkTemplateTriangulator);
	void PrintSelf(ostream& os, vtkIndent indent)
	{
		Superclass::PrintSelf(os, indent);
	}

	/// Override
	void GetPoint(vtkIdType v, double p[3])
	{
		assert(Points != NULL);
		assert(v >= 0 && v < Points->GetNumberOfPoints());
		Points->GetPoint(v, p);
	}

	/// Override
	virtual vtkIdType AddPoint(double x, double y, double z)
	{
		assert(Locator != NULL);
		double p[3] = {x, y, z};
		vtkIdType ptId;
		Locator->InsertUniquePoint(p, ptId);
		return ptId;
	}

	/// Override
	virtual void AddTetrahedron(vtkIdType v1, vtkIdType v2, vtkIdType v3,
								vtkIdType v4, int domain)
	{
		assert(Tetrahedra);
		assert(CellDomainArray);
		assert(Tetrahedra->GetNumberOfTetrahedra() ==
			   CellDomainArray->GetNumberOfTuples());
		Tetrahedra->push_back(v1, v2, v3, v4);
		CellDomainArray->InsertNextValue(domain);
	}

	vtkIncrementalPointLocator* Locator;
	vtkPoints* Points;
	TetContainer* Tetrahedra;
	vtkShortArray* CellDomainArray;

protected:
	vtkTriangulatorImpl() {}
	~vtkTriangulatorImpl() {}
};

vtkStandardNewMacro(vtkTriangulatorImpl);

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkImageExtractCompatibleMesher);

vtkImageExtractCompatibleMesher::vtkImageExtractCompatibleMesher()
{
	// optional tetrahedral mesh output
	this->SetNumberOfOutputPorts(2);
	vtkNew(vtkUnstructuredGrid, output2);
	this->GetExecutive()->SetOutputData(1, output2);

	this->FiveTetrahedraPerVoxel = true;
	this->CreateVoxelCenterPoint = false;
	this->MaxNumberOfIterations = 10;
	this->BackgroundLabel = 0;
	this->UseTemplates = true;
	this->UseOctreeLocator = true;
	this->OutputScalarName = 0;
	this->SetOutputScalarName("Material");
	this->GenerateTetMeshOutput = false;
	this->NumberOfUnassignedLabels = -1;

	this->Input = NULL;
	//this->Grid = NULL;
	this->Points = NULL;
	this->ClipScalars = NULL;
	this->Triangulator = NULL;
	this->Locator = NULL;
	this->Connectivity = NULL;
	this->PointDomainArray = NULL;
	this->Tetrahedra = NULL;
	this->MyTriangulator = NULL;

	// by default process active point scalars
	this->SetInputArrayToProcess(0, 0, 0,
								 vtkDataObject::FIELD_ASSOCIATION_POINTS,
								 vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkImageExtractCompatibleMesher::~vtkImageExtractCompatibleMesher()
{
	if (this->OutputScalarName)
		delete[] this->OutputScalarName;
}

//----------------------------------------------------------------------------
void vtkImageExtractCompatibleMesher::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkImageExtractCompatibleMesher::FillInputPortInformation(
	int vtkNotUsed(port), vtkInformation* info)
{
	info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
	return 1;
}

//----------------------------------------------------------------------------
int vtkImageExtractCompatibleMesher::FillOutputPortInformation(
	int port, vtkInformation* info)
{
	//The input should be two vtkPolyData
	if (port == 0)
	{
		info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
		return 1;
	}
	//The input should be two vtkPolyData
	if (port == 1)
	{
		info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkUnstructuredGrid");
		return 1;
	}
	return 1;
}

//----------------------------------------------------------------------------
int vtkImageExtractCompatibleMesher::RequestData(
	vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector,
	vtkInformationVector* outputVector)
{
	return this->ContourSurface(inputVector, outputVector);
}

int vtkImageExtractCompatibleMesher::ContourSurface(
	vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
	this->Input =
		vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));

	if (this->Input->GetNumberOfCells() == 0)
	{
		vtkErrorMacro(<< "Cannot run with zero cells");
		return 1;
	}
	vtkIdType estimatedSize = this->Input->GetNumberOfCells();
	estimatedSize = estimatedSize / 1024 * 1024; //multiple of 1024
	if (estimatedSize < 1024)
	{
		estimatedSize = 1024;
	}
	this->Points = vtkPoints::New();
	Points->SetDataTypeToFloat();
	Points->Allocate(estimatedSize / 2, estimatedSize / 2);
	if (this->UseOctreeLocator)
	{
		vtkIncrementalOctreePointLocator* loc =
			vtkIncrementalOctreePointLocator::New();
		loc->SetTolerance(0.0);
		loc->SetMaxLevel(12); // does this have any effect?
		// loc->SetMaxPointsPerLeaf(128);
		this->Locator = loc;
	}
	else
	{
		vtkMergePoints* loc = vtkMergePoints::New();
		int div[3];
		this->Input->GetDimensions(div);
		div[0] = std::max(div[0] / 7, 50);
		div[1] = std::max(div[1] / 7, 50);
		div[2] = std::max(div[2] / 7, 50);
		loc->SetDivisions(div);
		loc->SetMaxLevel(12); // does this have any effect?
		this->Locator = loc;
	}
	this->Locator->InitPointInsertion(Points, this->Input->GetBounds());
	this->Connectivity = vtkCellArray::New();
	this->Tetrahedra = new TetContainer(estimatedSize, estimatedSize / 2);

	this->CellDomainArray = vtkShortArray::New();
	this->CellDomainArray->SetName(this->OutputScalarName);
	this->CellDomainArray->SetNumberOfComponents(1);
	this->PointDomainArray = vtkShortArray::New();
	this->PointDomainArray->SetName(this->OutputScalarName);
	this->Triangulator = vtkOrderedTriangulator::New();
	this->Triangulator->PreSortedOn();
	this->Triangulator->UseTwoSortIdsOn();
	this->Triangulator->SetUseTemplates(this->UseTemplates);

	MyTriangulator = vtkTriangulatorImpl::New();
	MyTriangulator->CellDomainArray = CellDomainArray;
	MyTriangulator->Locator = Locator;
	MyTriangulator->Points = Points;
	MyTriangulator->Tetrahedra = Tetrahedra;

	int dims[3];
	double spacing[3], origin[3];
	this->Input->GetDimensions(dims);
	this->Input->GetOrigin(origin);
	this->Input->GetSpacing(spacing);

	vtkIdType numICells = dims[0] - 1;
	vtkIdType numJCells = dims[1] - 1;
	vtkIdType numKCells = dims[2] - 1;
	vtkIdType sliceSize = numICells * numJCells;
	vtkIdType extOffset = this->Input->GetExtent()[0] +
						  this->Input->GetExtent()[2] +
						  this->Input->GetExtent()[4];

	vtkPoints* cellPts;
	vtkIdList* cellIds;
	vtkGenericCell* cell = vtkGenericCell::New();
	this->ClipScalars = this->GetInputArrayToProcess(0, inputVector);
	if (!this->ClipScalars)
	{
		this->ClipScalars = this->Input->GetPointData()->GetArray(0);
		if (!this->ClipScalars)
		{
			this->Input->Print(std::cerr);
			vtkErrorMacro(<< "Cannot run without input scalars");
			return 1;
		}
	}
	vtkShortArray* cellScalars = vtkShortArray::New();
	cellScalars->SetNumberOfComponents(1);
	cellScalars->SetNumberOfTuples(8);

	// Traverse through all all voxels, compute tetrahedra on the go
	int abort = 0;
	for (vtkIdType k = 0; k < numKCells && !abort; k++)
	{
		// Check for progress and abort on every z-slice
		this->UpdateProgress(static_cast<double>(k) / numKCells);
		abort = this->GetAbortExecute();
		for (vtkIdType j = 0; j < numJCells; j++)
		{
			for (vtkIdType i = 0; i < numICells; i++)
			{
				int flip = (extOffset + i + j + k) & 0x1;
				vtkIdType cellId = i + j * numICells + k * sliceSize;

				this->Input->GetCell(cellId, cell);
				if (cell->GetCellType() == VTK_EMPTY_CELL)
				{
					continue;
				}
				cellPts = cell->GetPoints();
				cellIds = cell->GetPointIds();

				// Check if this cell is at surface/interface
				bool isClipped = false;
				int s0 = this->ClipScalars->GetComponent(cellIds->GetId(0), 0);
				for (int ii = 0; ii < 8; ii++)
				{
					int s =
						this->ClipScalars->GetComponent(cellIds->GetId(ii), 0);
					cellScalars->SetValue(ii, s);
					if (s != s0)
					{
						isClipped = true;
					}
				}

				//
				if (isClipped == true)
				{
					this->ClipVoxel(cellScalars, flip, spacing, cellIds,
									cellPts);
				}
				else if (s0 != this->BackgroundLabel)
				{
					this->TriangulateVoxel(s0, flip, spacing, cellIds, cellPts);
				}
			}
		}
		if (k % 10 == 0)
		{
			std::cerr << "Time for full voxels: " << Timer<1>::EllapsedTime()
					  << std::endl;
			std::cerr << "Time for clipped voxels: " << Timer<2>::EllapsedTime()
					  << std::endl;
		}
	}
	std::cerr << "Final Time for full voxels: " << Timer<1>::EllapsedTime()
			  << std::endl;
	std::cerr << "Final Time for clipped voxels: " << Timer<2>::EllapsedTime()
			  << std::endl;

	// Start cleaning up
	this->Triangulator->Delete();
	this->Locator->Delete();
	cell->Delete();
	cellScalars->Delete();

	std::cerr << "Number of tetra: "
			  << this->Tetrahedra->GetNumberOfTetrahedra() << std::endl;
	std::cerr << "Number of points: " << this->Points->GetNumberOfPoints()
			  << std::endl;

	// This is needed if using the ClipVoxel_not_used function instead of the newer ClipVoxel
	// this->LabelTetra_not_used();

	if (this->GenerateTetMeshOutput)
	{
		// Create & Write tetrahedral mesh
		vtkInformation* outInfo2 = outputVector->GetInformationObject(1);
		vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(
			outInfo2->Get(vtkDataObject::DATA_OBJECT()));
		this->GenerateTetMesh(grid);
	}

	Tetrahedra->BuildNeighbors();

	this->ExtractMeshDomainInterfaces(outputVector); // TODO

	// Cleanup
	this->Input = NULL;
	this->Points->Delete();
	this->Connectivity->Delete();
	this->CellDomainArray->Delete();
	this->ClipScalars = NULL;
	delete this->Tetrahedra;

	return 1;
}

// Method to triangulate and clip voxel using ordered Delaunay
// triangulation to produce tetrahedra. Voxel is initially triangulated
// using 8 voxel corner points inserted in order (to control direction
// of face diagonals). Then edge intersection points are injected into the
// triangulation. The ordering controls the orientation of any face
// diagonals.
void vtkImageExtractCompatibleMesher::ClipVoxel_not_used(
	vtkShortArray* cellScalars, int flip, double spacing[3], vtkIdList* cellIds,
	vtkPoints* cellPts)
{
	Timer<2> timer;

	int s1, s2;
	double x[3], voxelOrigin[3];
	double bounds[6], p1[3], p2[3];
	vtkIdType id, ptId;
	static int edges[12][2] = {{0, 1}, {2, 3}, {4, 5}, {6, 7}, {0, 2}, {1, 3},
							   {4, 6}, {5, 7}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
	static int order_flip[2][8] = {
		{0, 3, 5, 6, 1, 2, 4, 7},
		{1, 2, 4, 7, 0, 3, 5, 6}}; //injection order based on flip
	static int order_noflip[8] = {0, 1, 2, 3,
								  4, 5, 6, 7}; //injection order without flip

	// compute bounds for voxel and initialize
	cellPts->GetPoint(0, voxelOrigin);
	for (int i = 0; i < 3; i++)
	{
		bounds[2 * i] = voxelOrigin[i];
		bounds[2 * i + 1] = voxelOrigin[i] + spacing[i];
	}

	// Initialize Delaunay insertion process with voxel triangulation.
	// No more than 21 points (8 corners + 12 edges + 1 center) may be inserted.
	this->Triangulator->InitTriangulation(bounds, 21);

	// Inject ordered voxel corner points into triangulation. Recall
	// that the PreSortedOn() flag was set in the triangulator.
	int type;
	std::set<int> labelset;
	for (int numPts = 0; numPts < 8; numPts++)
	{
		if (this->FiveTetrahedraPerVoxel)
			ptId = order_flip[flip][numPts];
		else
			ptId = order_noflip[numPts];

		s1 = cellScalars->GetValue(ptId);
		if (s1 != this->BackgroundLabel)
		{
			type = 0; //inside
		}
		else
		{
			type = 4; //don't insert
		}

		if (type == 4)
			continue; // TODO: this might be wrong, in vtkClipVolume all points are added to locator

		labelset.insert(s1);
		cellPts->GetPoint(ptId, x);
		if (this->Locator->InsertUniquePoint(x, id))
		{
			this->PointDomainArray->InsertValue(id, s1);
		}
		this->Triangulator->InsertPoint(id, id, s1, x, x, type);
	} //for eight voxel corner points

	// For each edge intersection point, insert into triangulation. Edge
	// intersections exist where edge points have different scalar values (labels)
	for (int edgeNum = 0; edgeNum < 12; edgeNum++)
	{
		s1 = cellScalars->GetValue(edges[edgeNum][0]);
		s2 = cellScalars->GetValue(edges[edgeNum][1]);

		if (s1 != s2)
		{
			// generate edge intersection point
			cellPts->GetPoint(edges[edgeNum][0], p1);
			cellPts->GetPoint(edges[edgeNum][1], p2);
			for (int i = 0; i < 3; i++)
			{
				x[i] = 0.5 * (p1[i] + p2[i]);
			}

			// Incorporate point into output and interpolate edge data as necessary
			if (this->Locator->InsertUniquePoint(x, ptId))
			{
				this->PointDomainArray->InsertValue(
					ptId,
					SURFACE_DOMAIN); // reuse background for interface/surface label
			}

			//Insert into Delaunay triangulation (type 2 = "boundary")
			this->Triangulator->InsertPoint(ptId, ptId, 0, x, x, 2);
		} //if edge intersects value
	}	 //for all edges

	// Create center point
	if (this->CreateVoxelCenterPoint && labelset.size() > 2)
	{
		// generate edge intersection point
		cellPts->GetPoint(0, p1);
		cellPts->GetPoint(7, p2);
		for (int i = 0; i < 3; i++)
		{
			assert(p2[i] - p1[i] > spacing[i] * 0.5);
			x[i] = 0.5 * (p1[i] + p2[i]);
		}

		// Incorporate point into output and interpolate edge data as necessary
		if (this->Locator->InsertUniquePoint(x, ptId))
		{
			this->PointDomainArray->InsertValue(
				ptId,
				SURFACE_DOMAIN); // reuse background for interface/surface label
		}

		//Insert into Delaunay triangulation (type 2 = "boundary")
		this->Triangulator->InsertPoint(ptId, ptId, 0, x, x, 2);
	}

	// triangulate the points
	this->Triangulator->Triangulate();
	//this->Triangulator->TemplateTriangulate(cellType, numPts, numEdges);

	// Add the triangulation to the mesh
	this->Connectivity->Initialize();
	this->Triangulator->AddTetras(0, this->Connectivity);
	vtkIdType numNew = this->Connectivity->GetNumberOfCells();

	vtkIdType npts, *pts;
	this->Connectivity->InitTraversal();
	for (vtkIdType i = 0; i < numNew; i++)
	{
		this->Connectivity->GetNextCell(npts, pts);
		this->Tetrahedra->push_back(pts[0], pts[1], pts[2], pts[3]);
	}
}

void vtkImageExtractCompatibleMesher::ClipVoxel(vtkShortArray* cellScalars,
												int flip, double spacing[3],
												vtkIdList* cellIds,
												vtkPoints* cellPts)
{
	Timer<2> timer;

	double x[3], voxelOrigin[3];
	double bounds[6];
	vtkIdType id, ptId;
	static int order_flip[2][8] = {
		{0, 3, 5, 6, 1, 2, 4, 7},
		{1, 2, 4, 7, 0, 3, 5, 6}}; //injection order based on flip
	static int order_noflip[8] = {0, 1, 2, 3,
								  4, 5, 6, 7}; //injection order without flip

	// compute bounds for voxel and initialize
	cellPts->GetPoint(0, voxelOrigin);
	for (int i = 0; i < 3; i++)
	{
		bounds[2 * i] = voxelOrigin[i];
		bounds[2 * i + 1] = voxelOrigin[i] + spacing[i];
	}

	// Initialize Delaunay insertion process with voxel triangulation.
	// No more than 8 points (8 corners) may be inserted.
	this->Triangulator->InitTriangulation(bounds, 8);

	// Inject ordered voxel corner points into triangulation. Recall
	// that the PreSortedOn() flag was set in the triangulator.
	for (int numPts = 0; numPts < 8; numPts++)
	{
		if (this->FiveTetrahedraPerVoxel)
			ptId = order_flip[flip][numPts];
		else
			ptId = order_noflip[numPts];

		cellPts->GetPoint(ptId, x);
		if (this->Locator->InsertUniquePoint(x, id))
		{
			this->PointDomainArray->InsertValue(id,
												cellScalars->GetValue(ptId));
		}
		this->Triangulator->InsertPoint(id, id, 0 /*cellScalar*/, x, x, 0);
	}

	// triangulate the points
	if (UseTemplates)
		this->Triangulator->TemplateTriangulate(VTK_NUMBER_OF_CELL_TYPES + flip,
												8, 12);
	else
		this->Triangulator->Triangulate();

	// Add the triangulation to the mesh
	this->Connectivity->Initialize();
	this->Triangulator->AddTetras(0, this->Connectivity);
	vtkIdType numNew = this->Connectivity->GetNumberOfCells();

	vtkIdType npts, *pts;
	this->Connectivity->InitTraversal();
	for (vtkIdType i = 0; i < numNew; i++)
	{
		this->Connectivity->GetNextCell(npts, pts);

		// now check colors at nodes and subdivide the tetrahedra accordingly
		int doms[4] = {PointDomainArray->GetValue(pts[0]),
					   PointDomainArray->GetValue(pts[1]),
					   PointDomainArray->GetValue(pts[2]),
					   PointDomainArray->GetValue(pts[3])};
		MyTriangulator->AddMultipleDomainTetrahedron(pts, doms);
	}
}

void vtkImageExtractCompatibleMesher::TriangulateVoxel(int cellScalar, int flip,
													   double spacing[3],
													   vtkIdList* cellIds,
													   vtkPoints* cellPts)
{
	Timer<1> timer;

	double x[3], voxelOrigin[3];
	double bounds[6];
	vtkIdType id, ptId;
	static int order_flip[2][8] = {
		{0, 3, 5, 6, 1, 2, 4, 7},
		{1, 2, 4, 7, 0, 3, 5, 6}}; //injection order based on flip
	static int order_noflip[8] = {0, 1, 2, 3,
								  4, 5, 6, 7}; //injection order without flip

	// compute bounds for voxel and initialize
	cellPts->GetPoint(0, voxelOrigin);
	for (int i = 0; i < 3; i++)
	{
		bounds[2 * i] = voxelOrigin[i];
		bounds[2 * i + 1] = voxelOrigin[i] + spacing[i];
	}

	// Initialize Delaunay insertion process with voxel triangulation.
	// No more than 8 points (8 corners) may be inserted.
	this->Triangulator->InitTriangulation(bounds, 8);

	// Inject ordered voxel corner points into triangulation. Recall
	// that the PreSortedOn() flag was set in the triangulator.
	for (int numPts = 0; numPts < 8; numPts++)
	{
		if (this->FiveTetrahedraPerVoxel)
			ptId = order_flip[flip][numPts];
		else
			ptId = order_noflip[numPts];

		cellPts->GetPoint(ptId, x);
		if (this->Locator->InsertUniquePoint(x, id))
		{
			this->PointDomainArray->InsertValue(id, cellScalar);
		}
		this->Triangulator->InsertPoint(id, id, cellScalar, x, x, 0);
	} //for eight voxel corner points

	// triangulate the points
	if (UseTemplates)
		this->Triangulator->TemplateTriangulate(VTK_NUMBER_OF_CELL_TYPES + flip,
												8, 12);
	else
		this->Triangulator->Triangulate();

	// Add the triangulation to the mesh
	this->Connectivity->Initialize();
	this->Triangulator->AddTetras(0, this->Connectivity);
	vtkIdType numNew = this->Connectivity->GetNumberOfCells();

	vtkIdType npts, *pts;
	int doms[4] = {cellScalar, cellScalar, cellScalar, cellScalar};
	this->Connectivity->InitTraversal();
	for (vtkIdType i = 0; i < numNew; i++)
	{
		this->Connectivity->GetNextCell(npts, pts);
		// this->Tetrahedra->push_back(pts[0],pts[1],pts[2],pts[3]);
		MyTriangulator->AddMultipleDomainTetrahedron(pts, doms);
	}
}

void vtkImageExtractCompatibleMesher::GenerateTetMesh(vtkUnstructuredGrid* grid)
{
	assert(Tetrahedra->GetNumberOfTetrahedra() ==
		   CellDomainArray->GetNumberOfTuples());
	assert(grid);

	grid->Allocate(Tetrahedra->GetNumberOfTetrahedra());
	grid->SetPoints(this->Points);
	vtkIdType numTets = this->Tetrahedra->GetNumberOfTetrahedra();
	for (vtkIdType i = 0; i < numTets; i++)
	{
		vtkIdType pts[4];
		Tetrahedron tet = this->Tetrahedra->GetTetrahedron(i);
		pts[0] = tet[0];
		pts[1] = tet[1];
		pts[2] = tet[2];
		pts[3] = tet[3];
		grid->InsertNextCell(VTK_TETRA, 4, pts);
	}

	grid->GetCellData()->AddArray(this->CellDomainArray);
}

int vtkImageExtractCompatibleMesher::LabelTetra_not_used()
{
	assert(this->PointDomainArray && this->Tetrahedra);
	assert(this->PointDomainArray->GetNumberOfTuples() ==
		   this->Points->GetNumberOfPoints());
	int OUTSIDE_DOMAIN = this->BackgroundLabel;

	double x[3];

	this->CellDomainArray->SetNumberOfTuples(
		this->Tetrahedra->GetNumberOfTetrahedra());

	this->Tetrahedra->BuildNeighbors();

	// Try to compute unambiguous domain using tetrahedron vertices
	int number4PointsOnSurface = 0;
	NumberOfUnassignedLabels = 0;
	vtkIdType numberOfCells = this->CellDomainArray->GetNumberOfTuples();
	for (vtkIdType i = 0; i < numberOfCells; i++)
	{
		// if (this->Grid->GetCellType(i) != VTK_TETRA)
		const Tetrahedron& pts = this->Tetrahedra->GetTetrahedron(i);
		int numberInside = 0;
		int numberOutside = 0;
		int numberSurface = 0;
		int dom_cell = UNSURE_DOMAIN;
		for (int j = 0; j < 4; j++)
		{
			if (this->PointDomainArray->GetValue(pts[j]) == SURFACE_DOMAIN)
			{
				numberSurface++;
			}
			else if (this->PointDomainArray->GetValue(pts[j]) == OUTSIDE_DOMAIN)
			{
				dom_cell = this->PointDomainArray->GetValue(pts[j]);
				numberOutside++;
			}
			else // if INSIDE_DOMAIN_k
			{
				dom_cell = this->PointDomainArray->GetValue(pts[j]);
				numberInside++;
			}
		}

		// set default value
		this->CellDomainArray->SetValue(i, UNSURE_DOMAIN);

		// at least not on surface
		if (numberSurface < 4)
		{
			if (numberInside + numberSurface == 4)
			{
				assert(dom_cell != UNSURE_DOMAIN);
				this->CellDomainArray->SetValue(i, dom_cell);
			}
			// at least one outside
			else if (numberOutside + numberSurface == 4)
			{
				assert(dom_cell != UNSURE_DOMAIN);
				this->CellDomainArray->SetValue(i, dom_cell);
			}
			else
			{
				std::cout << "Tetrahedron with both inside and outside nodes ("
						  << numberInside << "," << numberOutside << ")."
						  << std::endl;
			}
		}
		// all points are on surface
		else
		{
			number4PointsOnSurface++;

			int dom[4];
			std::map<int, int> hist;
			std::map<int, int>::iterator hist_it;
			for (int k = 0; k < 4; k++)
			{
				this->Points->GetPoint(pts[k], x);
				dom[k] = EvaluateLabel(x);
				if ((hist_it = hist.find(dom[k])) == hist.end())
					hist[dom[k]] = 1;
				else
					hist_it->second++;
			}
			if (hist.size() == 1)
			{
				this->CellDomainArray->SetValue(i, dom[0]);
			}
			else
			{
				int bestdom = dom[0];
				int frequency = 0;
				for (hist_it = hist.begin(); hist_it != hist.end(); ++hist_it)
				{
					if (hist_it->second > frequency)
					{
						bestdom = hist_it->first;
						frequency = hist_it->second;
					}
				}
				if (frequency > 2)
					this->CellDomainArray->SetValue(i, bestdom);
			}
		}

		if (this->CellDomainArray->GetValue(i) == UNSURE_DOMAIN)
			NumberOfUnassignedLabels++;
	}
	this->PointDomainArray->Delete();

	// Neighborhood based labeling
	int dom;
	int iter = 0;
	int assigned = 0;
	int abort = 0;
	do
	{
		std::cerr << "Iteration: " << iter << std::endl;
		assigned = 0;
		vtkShortArray* domainCopy = vtkShortArray::New();
		char name[32];
		sprintf(name, "Material_%d", iter++);
		domainCopy->SetName(name);
		domainCopy->DeepCopy(this->CellDomainArray);

		for (vtkIdType cellId = 0; cellId < numberOfCells; cellId++)
		{
			if (domainCopy->GetValue(cellId) == UNSURE_DOMAIN)
			{
				// check domain of each neighbor
				std::map<int, int> hist;
				std::map<int, int>::iterator hist_it;
				for (int k = 0; k < 4; k++)
				{
					if (this->Tetrahedra->HasNeighbor(cellId, k))
					{
						vtkIdType neighborId =
							this->Tetrahedra->GetNeighbor(cellId, k);
						if ((dom = domainCopy->GetValue(neighborId)) !=
							UNSURE_DOMAIN)
						{
							if ((hist_it = hist.find(dom)) == hist.end())
								hist[dom] = 1;
							else
								hist_it->second++;
						}
					}
				}
				if (hist.size() > 0)
				{
					int cell_domain = UNSURE_DOMAIN;
					int maxhist = 0;
					int max2hist = 0;
					for (hist_it = hist.begin(); hist_it != hist.end();
						 ++hist_it)
					{
						if (hist_it->second >= 2)
						{
							maxhist = 0;
							cell_domain = hist_it->first;
							break;
						}
						if (hist_it->second >= maxhist)
						{
							max2hist = maxhist;
							maxhist = hist_it->second;
							cell_domain = hist_it->first;
						}
					}
					if (maxhist == 0)
						this->CellDomainArray->SetValue(cellId, cell_domain);
					else if (maxhist > 1 && maxhist > max2hist)
						this->CellDomainArray->SetValue(cellId, cell_domain);
					else if (iter == MaxNumberOfIterations)
						this->CellDomainArray->SetValue(cellId, cell_domain);
					if (this->CellDomainArray->GetValue(cellId) !=
						UNSURE_DOMAIN)
						assigned++;
				}
			}
		}

		domainCopy->Delete();

		abort = this->GetAbortExecute();
		NumberOfUnassignedLabels -= assigned;
		std::cout << "Number of assigned cells: " << assigned << std::endl;

	} while (assigned > 0 && !abort && iter < MaxNumberOfIterations);

	// Label unassigned cells (UNSURE_DOMAIN)
	for (vtkIdType cellId = 0;
		 cellId < numberOfCells && NumberOfUnassignedLabels > 0; cellId++)
	{
		if (this->CellDomainArray->GetValue(cellId) == UNSURE_DOMAIN)
		{
			// check domain of each neighbor
			std::map<int, int> hist;
			for (int k = 0; k < 4; k++)
			{
				if (this->Tetrahedra->HasNeighbor(cellId, k))
				{
					vtkIdType neighborId =
						this->Tetrahedra->GetNeighbor(cellId, k);
					if ((dom = this->CellDomainArray->GetValue(neighborId)) !=
						UNSURE_DOMAIN)
					{
						if (hist.find(dom) == hist.end())
							hist[dom] = 1;
						else
							hist[dom]++;
					}
				}
			}
			int cell_domain = UNSURE_DOMAIN;
			int maxhist = 0;
			std::map<int, int>::iterator it;
			for (it = hist.begin(); it != hist.end(); ++it)
			{
				if (it->second >= 2)
				{
					maxhist = 0;
					cell_domain = it->first;
					break;
				}
				if (it->second >= maxhist)
				{
					maxhist = it->second;
					cell_domain = it->first;
				}
			}
			this->CellDomainArray->SetValue(cellId, cell_domain);
			if (this->CellDomainArray->GetValue(cellId) != UNSURE_DOMAIN)
				NumberOfUnassignedLabels--;
		}
	}

	std::cout << "Number of cells with 4 points on surface: "
			  << number4PointsOnSurface << std::endl;
	std::cout << "Number of unassigned cells: " << NumberOfUnassignedLabels
			  << std::endl;
	return 1;
}

int vtkImageExtractCompatibleMesher::EvaluateLabel(double x[3])
{
	assert(this->Input && this->ClipScalars);
	int dims[3];
	int extent[6];
	double origin[6], spacing[3];
	this->Input->GetDimensions(dims);
	this->Input->GetSpacing(spacing);
	this->Input->GetOrigin(origin);
	this->Input->GetExtent(extent);

	vtkIdType numICells = dims[0];
	vtkIdType numJCells = dims[1];
	vtkIdType sliceSize = numICells * numJCells;

	int i = (x[0] - origin[0] + 0.5 * spacing[0]) / spacing[0] - extent[0];
	int j = (x[1] - origin[1] + 0.5 * spacing[1]) / spacing[1] - extent[2];
	int k = (x[2] - origin[2] + 0.5 * spacing[2]) / spacing[2] - extent[4];

	if (i >= 0 && i < dims[0] && j >= 0 && j < dims[1] && k >= 0 && k < dims[2])
	{
		vtkIdType cellId = i + j * numICells + k * sliceSize;

		assert(cellId >= 0 && cellId < this->ClipScalars->GetNumberOfTuples());
		int dom = this->ClipScalars->GetTuple1(cellId);
		return dom;
	}
	std::cout << "Warning: probing outside image " << i << " " << j << " " << k
			  << std::endl;
	return this->BackgroundLabel;
}

void vtkImageExtractCompatibleMesher::ExtractMeshDomainInterfaces(
	vtkInformationVector* outputVector)
{
	vtkInformation* outInfo = outputVector->GetInformationObject(0);
	vtkPolyData* output =
		vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

	if (!this->CellDomainArray)
	{
		vtkErrorMacro(<< "Cannot run without cell labels");
		return;
	}
	vtkIdType NCells = this->Tetrahedra->GetNumberOfTetrahedra();
	vtkIdType NPts = this->Points->GetNumberOfPoints();

	// setup algorithm data structures
	vtkIdType neighborId;
	vtkNew(vtkIdList, facePtIds);
	facePtIds->SetNumberOfIds(3);

	vtkNew(vtkShortArray, domainArray);
	domainArray->SetName(OutputScalarName);
	domainArray->Allocate(NCells, NCells / 2);
	output->GetCellData()->AddArray(domainArray);
	vtkNew(vtkCellArray, cells);
	cells->Allocate(NCells / 2, NCells / 2);
	vtkNew(vtkPoints, points);
	output->SetPoints(points);

	std::vector<bool> visited(NCells, false);
	std::vector<vtkIdType> ptIdMapping(NPts, -1);
	vtkIdType tri1Ids[3], tri2Ids[3];
	double x[3];
	int abort = 0;
	vtkIdType checkEvery = NCells / 20;
	if (checkEvery == 0)
		checkEvery = 1;
	for (vtkIdType cellId = 0; cellId < NCells && !abort; cellId++)
	{
		if (static_cast<int>(this->CellDomainArray->GetTuple1(cellId)) ==
			BackgroundLabel)
			continue;

		// Check for progress and abort
		if (cellId % checkEvery == 0)
		{
			this->UpdateProgress(static_cast<double>(cellId) / NCells);
			abort = this->GetAbortExecute();
		}

		visited[cellId] = true;
		const Tetrahedron& tet = this->Tetrahedra->GetTetrahedron(cellId);
		for (int k = 0; k < 4; k++)
		{
			facePtIds->SetId(0, tet[tet_faces[k][0]]);
			facePtIds->SetId(1, tet[tet_faces[k][1]]);
			facePtIds->SetId(2, tet[tet_faces[k][2]]);

			if (!this->Tetrahedra->HasNeighbor(cellId, k))
			{
				// boundary tet: add face
				if (ptIdMapping[facePtIds->GetId(0)] < 0)
				{
					this->Points->GetPoint(facePtIds->GetId(0), x);
					ptIdMapping[facePtIds->GetId(0)] =
						points->InsertNextPoint(x);
				}
				if (ptIdMapping[facePtIds->GetId(1)] < 0)
				{
					this->Points->GetPoint(facePtIds->GetId(1), x);
					ptIdMapping[facePtIds->GetId(1)] =
						points->InsertNextPoint(x);
				}
				if (ptIdMapping[facePtIds->GetId(2)] < 0)
				{
					this->Points->GetPoint(facePtIds->GetId(2), x);
					ptIdMapping[facePtIds->GetId(2)] =
						points->InsertNextPoint(x);
				}

				tri1Ids[0] = ptIdMapping[facePtIds->GetId(0)];
				tri1Ids[1] = ptIdMapping[facePtIds->GetId(1)];
				tri1Ids[2] = ptIdMapping[facePtIds->GetId(2)];
				cells->InsertNextCell(3, tri1Ids);
				domainArray->InsertNextValue(
					this->CellDomainArray->GetTuple1(cellId));
			}
			else if (visited[neighborId = this->Tetrahedra->GetNeighbor(
								 cellId, k)] == false)
			{
				// neighbor has not been visited yet
				if (this->CellDomainArray->GetTuple1(cellId) !=
					this->CellDomainArray->GetTuple1(neighborId))
				{
					// label is different: add face
					if (ptIdMapping[facePtIds->GetId(0)] < 0)
					{
						this->Points->GetPoint(facePtIds->GetId(0), x);
						ptIdMapping[facePtIds->GetId(0)] =
							points->InsertNextPoint(x);
					}
					if (ptIdMapping[facePtIds->GetId(1)] < 0)
					{
						this->Points->GetPoint(facePtIds->GetId(1), x);
						ptIdMapping[facePtIds->GetId(1)] =
							points->InsertNextPoint(x);
					}
					if (ptIdMapping[facePtIds->GetId(2)] < 0)
					{
						this->Points->GetPoint(facePtIds->GetId(2), x);
						ptIdMapping[facePtIds->GetId(2)] =
							points->InsertNextPoint(x);
					}
					tri1Ids[0] = ptIdMapping[facePtIds->GetId(0)];
					tri1Ids[1] = ptIdMapping[facePtIds->GetId(1)];
					tri1Ids[2] = ptIdMapping[facePtIds->GetId(2)];
					cells->InsertNextCell(3, tri1Ids);
					domainArray->InsertNextValue(
						this->CellDomainArray->GetTuple1(cellId));

					// add duplicate, unless the neighbor is background
					if (static_cast<int>(this->CellDomainArray->GetTuple1(
							neighborId)) != BackgroundLabel)
					{
						tri2Ids[0] = ptIdMapping[facePtIds->GetId(0)];
						tri2Ids[1] = ptIdMapping[facePtIds->GetId(2)];
						tri2Ids[2] = ptIdMapping[facePtIds->GetId(1)];
						cells->InsertNextCell(3, tri2Ids);
						domainArray->InsertNextValue(
							this->CellDomainArray->GetTuple1(neighborId));
					}
				}
			}
		}
	}
	output->SetPolys(cells);
	std::cerr << "Number of tetra: " << NCells << std::endl;
	std::cerr << "Number of triangles: " << cells->GetNumberOfCells()
			  << std::endl;
}
