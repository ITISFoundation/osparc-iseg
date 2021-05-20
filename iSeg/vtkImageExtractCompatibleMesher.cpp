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

#include <vtkTimerLog.h>

#include <vtkSmartPointer.h>
#define vtkNew(type, name) \
	vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

template<int Id>
class Timer
{
public:
	Timer() { m_C0 = clock(); }
	~Timer() { total_clocks += (clock() - m_C0); }
	static double EllapsedTime() { return total_clocks / CLOCKS_PER_SEC; }
	static void Reset() { total_clocks = 0; }

protected:
	clock_t m_C0;
	static double total_clocks;
};

template<int Id>
double Timer<Id>::total_clocks = 0.0;

static int tet_faces[4][3] = {{0, 1, 3}, {0, 2, 1}, {0, 3, 2}, {1, 2, 3}};

using id_type = unsigned int;
// triangle with less than operator
struct Triangle
{
	void Sort()
	{
		if (m_N1 > m_N2)
			std::swap(m_N1, m_N2);
		if (m_N1 > m_N3)
			std::swap(m_N1, m_N3);
		if (m_N2 > m_N3)
			std::swap(m_N2, m_N3);
		assert(m_N1 < m_N2);
		assert(m_N1 < m_N3);
		assert(m_N2 < m_N3);
	}
	bool operator<(const Triangle& rhs) const
	{
		if (m_N1 != rhs.m_N1)
			return (m_N1 < rhs.m_N1);
		else if (m_N2 != rhs.m_N2)
			return (m_N2 < rhs.m_N2);
		else
			return (m_N3 < rhs.m_N3);
	}
	bool operator==(const Triangle& rhs) const
	{
		return (m_N1 == rhs.m_N1 && m_N2 == rhs.m_N2 && m_N3 == rhs.m_N3);
	}
	id_type m_N1, m_N2, m_N3;
};
// store the ids of a tet
class Tetrahedron
{
public:
	Tetrahedron(id_type i0, id_type i1, id_type i2, id_type i3)
	{
		m_N0 = i0;
		m_N1 = i1;
		m_N2 = i2;
		m_N3 = i3;
	}
	id_type operator[](size_t i) const
	{
		if (i == 0)
			return m_N0;
		else if (i == 1)
			return m_N1;
		else if (i == 2)
			return m_N2;
		else /*(i==3)*/
			return m_N3;
	}
	int WhichTriangle(Triangle& tri)
	{
		Triangle face;
		for (int k = 0; k < 4; k++)
		{
			face.m_N1 = (*this)[tet_faces[k][0]];
			face.m_N2 = (*this)[tet_faces[k][1]];
			face.m_N3 = (*this)[tet_faces[k][2]];
			face.Sort();
			if (face == tri)
				return k;
		}
		return -1;
	}

private:
	id_type m_N0, m_N1, m_N2, m_N3;
};
// store the tet face neighbors
class TetNeighbors
{
public:
	TetNeighbors() { m_Cell0 = m_Cell1 = m_Cell2 = m_Cell3 = VTK_UNSIGNED_INT_MAX; };
	void SetNeighbor(int k, id_type id)
	{
		assert(k >= 0 && k < 4);
		if (k == 0)
			m_Cell0 = id;
		else if (k == 1)
			m_Cell1 = id;
		else if (k == 2)
			m_Cell2 = id;
		else /*(k==3)*/
			m_Cell3 = id;
	}
	id_type GetNeighbor(int k) const
	{
		assert(k >= 0 && k < 4);
		if (k == 0)
			return m_Cell0;
		else if (k == 1)
			return m_Cell1;
		else if (k == 2)
			return m_Cell2;
		else /*(k==3)*/
			return m_Cell3;
	}
	bool IsAssigned(id_type id) const { return (id != VTK_UNSIGNED_INT_MAX); }

private:
	id_type m_Cell0, m_Cell1, m_Cell2, m_Cell3;
};
// store the tetrahedra and neighborhood information
class TetContainer
{
public:
	TetContainer(size_t estimatedSize, size_t extensionSize)
	{
		m_Tetra.reserve(estimatedSize);
		m_ExtensionSize = extensionSize;
	}
	void PushBack(id_type i0, id_type i1, id_type i2, id_type i3)
	{
		if (m_Tetra.size() + 1 >= m_Tetra.capacity())
			m_Tetra.reserve(m_Tetra.size() + m_ExtensionSize);
		m_Tetra.push_back(Tetrahedron(i0, i1, i2, i3));
	}
	size_t GetNumberOfTetrahedra() const { return m_Tetra.size(); }
	const Tetrahedron& GetTetrahedron(size_t cellId) const
	{
		return m_Tetra[cellId];
	}

	bool HasNeighbor(size_t cellId, int k) const
	{
		return (m_Neighbors[cellId].GetNeighbor(k) != VTK_UNSIGNED_INT_MAX);
	}
	id_type GetNeighbor(size_t cellId, int k) const
	{
		return m_Neighbors[cellId].GetNeighbor(k);
	}
	bool BuildNeighbors()
	{
		std::cerr << "Building neighborhood information" << std::endl;
		// allocate memory
		try
		{
			m_Neighbors.resize(m_Tetra.size());
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
		if (m_Tetra.size() >= max_unsigned_int)
		{
			return false;
		}

		// iterate through tetrahedra and create neighbors
		using triangle_id_map = std::map<Triangle, id_type>;
		triangle_id_map tmap;
		triangle_id_map::iterator tmap_it;
		//std::vector<id_type> t1(3), t2(3), t3(3), t4(3);
		Triangle t1, t2, t3, t4;
		std::vector<const Triangle*> faces(4);
		id_type nc = static_cast<id_type>(m_Tetra.size());
		for (id_type cell_id = 0; cell_id < nc; cell_id++)
		{
			const Tetrahedron& tet = m_Tetra[cell_id];
			// get four triangle faces
			t1.m_N1 = tet[tet_faces[0][0]];
			t1.m_N2 = tet[tet_faces[0][1]];
			t1.m_N3 = tet[tet_faces[0][2]];
			t1.Sort();

			t2.m_N1 = tet[tet_faces[1][0]];
			t2.m_N2 = tet[tet_faces[1][1]];
			t2.m_N3 = tet[tet_faces[1][2]];
			t2.Sort();

			t3.m_N1 = tet[tet_faces[2][0]];
			t3.m_N2 = tet[tet_faces[2][1]];
			t3.m_N3 = tet[tet_faces[2][2]];
			t3.Sort();

			t4.m_N1 = tet[tet_faces[3][0]];
			t4.m_N2 = tet[tet_faces[3][1]];
			t4.m_N3 = tet[tet_faces[3][2]];
			t4.Sort();

			//t1
			tmap_it = tmap.find(t1);
			if (tmap_it == tmap.end())
			{ // add first
				tmap[t1] = cell_id;
			}
			else
			{ //found second
				assert(tmap_it->second != cell_id);
				const id_type neighbor_id = tmap_it->second;
				const int j = m_Tetra[neighbor_id].WhichTriangle(t1);
				assert(j >= 0);
				m_Neighbors[cell_id].SetNeighbor(0, neighbor_id);
				m_Neighbors[neighbor_id].SetNeighbor(j, cell_id);
				tmap.erase(tmap_it);
			}

			// t2
			tmap_it = tmap.find(t2);
			if (tmap_it == tmap.end())
			{ // add first
				tmap[t2] = cell_id;
			}
			else
			{ //found second
				assert(tmap_it->second != cell_id);
				const id_type neighbor_id = tmap_it->second;
				const int j = m_Tetra[neighbor_id].WhichTriangle(t2);
				assert(j >= 0);
				m_Neighbors[cell_id].SetNeighbor(1, neighbor_id);
				m_Neighbors[neighbor_id].SetNeighbor(j, cell_id);
				tmap.erase(tmap_it);
			}

			// t3
			tmap_it = tmap.find(t3);
			if (tmap_it == tmap.end())
			{ // add first
				tmap[t3] = cell_id;
			}
			else
			{ //found second
				assert(tmap_it->second != cell_id);
				const id_type neighbor_id = tmap_it->second;
				const int j = m_Tetra[neighbor_id].WhichTriangle(t3);
				assert(j >= 0);
				m_Neighbors[cell_id].SetNeighbor(2, neighbor_id);
				m_Neighbors[neighbor_id].SetNeighbor(j, cell_id);
				tmap.erase(tmap_it);
			}

			// t4
			tmap_it = tmap.find(t4);
			if (tmap_it == tmap.end())
			{ // add first
				tmap[t4] = cell_id;
			}
			else
			{ //found second
				assert(tmap_it->second != cell_id);
				const id_type neighbor_id = tmap_it->second;
				const int j = m_Tetra[neighbor_id].WhichTriangle(t4);
				assert(j >= 0);
				m_Neighbors[cell_id].SetNeighbor(3, neighbor_id);
				m_Neighbors[neighbor_id].SetNeighbor(j, cell_id);
				tmap.erase(tmap_it);
			}
		}

		std::cerr << "tmap size: " << tmap.size() << std::endl;
		for (id_type cell_id = 0; cell_id < nc; cell_id++)
		{
			const Tetrahedron& tet = m_Tetra[cell_id];
			// get four triangle faces
			t1.m_N1 = tet[tet_faces[0][0]];
			t1.m_N2 = tet[tet_faces[0][1]];
			t1.m_N3 = tet[tet_faces[0][2]];
			t1.Sort();

			t2.m_N1 = tet[tet_faces[1][0]];
			t2.m_N2 = tet[tet_faces[1][1]];
			t2.m_N3 = tet[tet_faces[1][2]];
			t2.Sort();

			t3.m_N1 = tet[tet_faces[2][0]];
			t3.m_N2 = tet[tet_faces[2][1]];
			t3.m_N3 = tet[tet_faces[2][2]];
			t3.Sort();

			t4.m_N1 = tet[tet_faces[3][0]];
			t4.m_N2 = tet[tet_faces[3][1]];
			t4.m_N3 = tet[tet_faces[3][2]];
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

				id_type tid1 = tmap_it->second;
				assert(tid1 >= 0 && tid1 < nc);
				if (tid1 != cell_id)
				{
					m_Neighbors[cell_id].SetNeighbor(j, tid1);
				}
				else
				{
					assert(tid1 == cell_id);
				}
			}
		}

		return true;
	}

private:
	size_t m_ExtensionSize;
	std::vector<Tetrahedron> m_Tetra;
	std::vector<TetNeighbors> m_Neighbors;
};

class vtkTriangulatorImpl : public vtkTemplateTriangulator
{
public:
	static vtkTriangulatorImpl* New();
	vtkTypeMacro(vtkTriangulatorImpl, vtkTemplateTriangulator);
	void PrintSelf(ostream& os, vtkIndent indent) override
	{
		Superclass::PrintSelf(os, indent);
	}

	/// Override
	void GetPoint(vtkIdType v, double p[3]) override
	{
		assert(m_Points != nullptr);
		assert(v >= 0 && v < m_Points->GetNumberOfPoints());
		m_Points->GetPoint(v, p);
	}

	/// Override
	vtkIdType AddPoint(double x, double y, double z) override
	{
		assert(m_Locator != nullptr);
		double p[3] = {x, y, z};
		vtkIdType pt_id;
		m_Locator->InsertUniquePoint(p, pt_id);
		return pt_id;
	}

	/// Override
	void AddTetrahedron(vtkIdType v1, vtkIdType v2, vtkIdType v3, vtkIdType v4, int domain) override
	{
		assert(m_Tetrahedra);
		assert(m_CellDomainArray);
		assert(m_Tetrahedra->GetNumberOfTetrahedra() ==
					 m_CellDomainArray->GetNumberOfTuples());
		m_Tetrahedra->PushBack(v1, v2, v3, v4);
		m_CellDomainArray->InsertNextValue(domain);
	}

	vtkIncrementalPointLocator* m_Locator;
	vtkPoints* m_Points;
	TetContainer* m_Tetrahedra;
	vtkShortArray* m_CellDomainArray;

protected:
	vtkTriangulatorImpl() = default;
	~vtkTriangulatorImpl() override = default;
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
	this->OutputScalarName = nullptr;
	this->SetOutputScalarName("Material");
	this->GenerateTetMeshOutput = false;
	this->NumberOfUnassignedLabels = -1;

	this->Input = nullptr;
	//this->Grid = nullptr;
	this->Points = nullptr;
	this->ClipScalars = nullptr;
	this->Triangulator = nullptr;
	this->Locator = nullptr;
	this->Connectivity = nullptr;
	this->PointDomainArray = nullptr;
	this->Tetrahedra = nullptr;
	this->MyTriangulator = nullptr;

	// by default process active point scalars
	this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
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
int vtkImageExtractCompatibleMesher::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
	info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
	return 1;
}

//----------------------------------------------------------------------------
int vtkImageExtractCompatibleMesher::FillOutputPortInformation(int port, vtkInformation* info)
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
int vtkImageExtractCompatibleMesher::RequestData(vtkInformation* vtkNotUsed(request), vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	return this->ContourSurface(inputVector, outputVector);
}

int vtkImageExtractCompatibleMesher::ContourSurface(vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
	vtkInformation* in_info = inputVector[0]->GetInformationObject(0);
	this->Input =
			vtkImageData::SafeDownCast(in_info->Get(vtkDataObject::DATA_OBJECT()));

	if (this->Input->GetNumberOfCells() == 0)
	{
		vtkErrorMacro(<< "Cannot run with zero cells");
		return 1;
	}
	vtkIdType estimated_size = this->Input->GetNumberOfCells();
	estimated_size = estimated_size / 1024 * 1024; //multiple of 1024
	if (estimated_size < 1024)
	{
		estimated_size = 1024;
	}
	this->Points = vtkPoints::New();
	Points->SetDataTypeToFloat();
	Points->Allocate(estimated_size / 2, estimated_size / 2);
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
	this->Tetrahedra = new TetContainer(estimated_size, estimated_size / 2);

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
	MyTriangulator->m_CellDomainArray = CellDomainArray;
	MyTriangulator->m_Locator = Locator;
	MyTriangulator->m_Points = Points;
	MyTriangulator->m_Tetrahedra = Tetrahedra;

	int dims[3];
	double spacing[3], origin[3];
	this->Input->GetDimensions(dims);
	this->Input->GetOrigin(origin);
	this->Input->GetSpacing(spacing);

	vtkIdType num_i_cells = dims[0] - 1;
	vtkIdType num_j_cells = dims[1] - 1;
	vtkIdType num_k_cells = dims[2] - 1;
	vtkIdType slice_size = num_i_cells * num_j_cells;
	vtkIdType ext_offset = this->Input->GetExtent()[0] +
												 this->Input->GetExtent()[2] +
												 this->Input->GetExtent()[4];

	vtkPoints* cell_pts;
	vtkIdList* cell_ids;
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
	vtkShortArray* cell_scalars = vtkShortArray::New();
	cell_scalars->SetNumberOfComponents(1);
	cell_scalars->SetNumberOfTuples(8);

	// Traverse through all all voxels, compute tetrahedra on the go
	int abort = 0;
	for (vtkIdType k = 0; k < num_k_cells && !abort; k++)
	{
		// Check for progress and abort on every z-slice
		this->UpdateProgress(static_cast<double>(k) / num_k_cells);
		abort = this->GetAbortExecute();
		for (vtkIdType j = 0; j < num_j_cells; j++)
		{
			for (vtkIdType i = 0; i < num_i_cells; i++)
			{
				int flip = (ext_offset + i + j + k) & 0x1;
				vtkIdType cell_id = i + j * num_i_cells + k * slice_size;

				this->Input->GetCell(cell_id, cell);
				if (cell->GetCellType() == VTK_EMPTY_CELL)
				{
					continue;
				}
				cell_pts = cell->GetPoints();
				cell_ids = cell->GetPointIds();

				// Check if this cell is at surface/interface
				bool is_clipped = false;
				int s0 = this->ClipScalars->GetComponent(cell_ids->GetId(0), 0);
				for (int ii = 0; ii < 8; ii++)
				{
					int s =
							this->ClipScalars->GetComponent(cell_ids->GetId(ii), 0);
					cell_scalars->SetValue(ii, s);
					if (s != s0)
					{
						is_clipped = true;
					}
				}

				//
				if (is_clipped == true)
				{
					this->ClipVoxel(cell_scalars, flip, spacing, cell_ids, cell_pts);
				}
				else if (s0 != this->BackgroundLabel)
				{
					this->TriangulateVoxel(s0, flip, spacing, cell_ids, cell_pts);
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
	cell_scalars->Delete();

	std::cerr << "Number of tetra: "
						<< this->Tetrahedra->GetNumberOfTetrahedra() << std::endl;
	std::cerr << "Number of points: " << this->Points->GetNumberOfPoints()
						<< std::endl;

	// This is needed if using the ClipVoxel_not_used function instead of the newer ClipVoxel
	// this->LabelTetra_not_used();

	if (this->GenerateTetMeshOutput)
	{
		// Create & Write tetrahedral mesh
		vtkInformation* out_info2 = outputVector->GetInformationObject(1);
		vtkUnstructuredGrid* grid = vtkUnstructuredGrid::SafeDownCast(out_info2->Get(vtkDataObject::DATA_OBJECT()));
		this->GenerateTetMesh(grid);
	}

	Tetrahedra->BuildNeighbors();

	this->ExtractMeshDomainInterfaces(outputVector); // TODO

	// Cleanup
	this->Input = nullptr;
	this->Points->Delete();
	this->Connectivity->Delete();
	this->CellDomainArray->Delete();
	this->ClipScalars = nullptr;
	delete this->Tetrahedra;

	return 1;
}

void vtkImageExtractCompatibleMesher::ClipVoxel(vtkShortArray* cellScalars, int flip, double spacing[3], vtkIdList* cellIds, vtkPoints* cellPts)
{
	Timer<2> timer;

	double x[3], voxel_origin[3];
	double bounds[6];
	vtkIdType id, pt_id;
	static int order_flip[2][8] = {
			{0, 3, 5, 6, 1, 2, 4, 7}, {1, 2, 4, 7, 0, 3, 5, 6}}; //injection order based on flip
	static int order_noflip[8] = {0, 1, 2, 3, 4, 5, 6, 7};	 //injection order without flip

	// compute bounds for voxel and initialize
	cellPts->GetPoint(0, voxel_origin);
	for (int i = 0; i < 3; i++)
	{
		bounds[2 * i] = voxel_origin[i];
		bounds[2 * i + 1] = voxel_origin[i] + spacing[i];
	}

	// Initialize Delaunay insertion process with voxel triangulation.
	// No more than 8 points (8 corners) may be inserted.
	this->Triangulator->InitTriangulation(bounds, 8);

	// Inject ordered voxel corner points into triangulation. Recall
	// that the PreSortedOn() flag was set in the triangulator.
	for (int num_pts = 0; num_pts < 8; num_pts++)
	{
		if (this->FiveTetrahedraPerVoxel)
			pt_id = order_flip[flip][num_pts];
		else
			pt_id = order_noflip[num_pts];

		cellPts->GetPoint(pt_id, x);
		if (this->Locator->InsertUniquePoint(x, id))
		{
			this->PointDomainArray->InsertValue(id, cellScalars->GetValue(pt_id));
		}
		this->Triangulator->InsertPoint(id, id, 0 /*cellScalar*/, x, x, 0);
	}

	// triangulate the points
	if (UseTemplates)
		this->Triangulator->TemplateTriangulate(VTK_NUMBER_OF_CELL_TYPES + flip, 8, 12);
	else
		this->Triangulator->Triangulate();

	// Add the triangulation to the mesh
	this->Connectivity->Initialize();
	this->Triangulator->AddTetras(0, this->Connectivity);
	vtkIdType num_new = this->Connectivity->GetNumberOfCells();

	vtkIdType npts, *pts;
	this->Connectivity->InitTraversal();
	for (vtkIdType i = 0; i < num_new; i++)
	{
		this->Connectivity->GetNextCell(npts, pts);

		// now check colors at nodes and subdivide the tetrahedra accordingly
		int doms[4] = {PointDomainArray->GetValue(pts[0]), PointDomainArray->GetValue(pts[1]), PointDomainArray->GetValue(pts[2]), PointDomainArray->GetValue(pts[3])};
		MyTriangulator->AddMultipleDomainTetrahedron(pts, doms);
	}
}

void vtkImageExtractCompatibleMesher::TriangulateVoxel(int cellScalar, int flip, double spacing[3], vtkIdList* cellIds, vtkPoints* cellPts)
{
	Timer<1> timer;

	double x[3], voxel_origin[3];
	double bounds[6];
	vtkIdType id, pt_id;
	static int order_flip[2][8] = {
			{0, 3, 5, 6, 1, 2, 4, 7}, {1, 2, 4, 7, 0, 3, 5, 6}}; //injection order based on flip
	static int order_noflip[8] = {0, 1, 2, 3, 4, 5, 6, 7};	 //injection order without flip

	// compute bounds for voxel and initialize
	cellPts->GetPoint(0, voxel_origin);
	for (int i = 0; i < 3; i++)
	{
		bounds[2 * i] = voxel_origin[i];
		bounds[2 * i + 1] = voxel_origin[i] + spacing[i];
	}

	// Initialize Delaunay insertion process with voxel triangulation.
	// No more than 8 points (8 corners) may be inserted.
	this->Triangulator->InitTriangulation(bounds, 8);

	// Inject ordered voxel corner points into triangulation. Recall
	// that the PreSortedOn() flag was set in the triangulator.
	for (int num_pts = 0; num_pts < 8; num_pts++)
	{
		if (this->FiveTetrahedraPerVoxel)
			pt_id = order_flip[flip][num_pts];
		else
			pt_id = order_noflip[num_pts];

		cellPts->GetPoint(pt_id, x);
		if (this->Locator->InsertUniquePoint(x, id))
		{
			this->PointDomainArray->InsertValue(id, cellScalar);
		}
		this->Triangulator->InsertPoint(id, id, cellScalar, x, x, 0);
	} //for eight voxel corner points

	// triangulate the points
	if (UseTemplates)
		this->Triangulator->TemplateTriangulate(VTK_NUMBER_OF_CELL_TYPES + flip, 8, 12);
	else
		this->Triangulator->Triangulate();

	// Add the triangulation to the mesh
	this->Connectivity->Initialize();
	this->Triangulator->AddTetras(0, this->Connectivity);
	vtkIdType num_new = this->Connectivity->GetNumberOfCells();

	vtkIdType npts, *pts;
	int doms[4] = {cellScalar, cellScalar, cellScalar, cellScalar};
	this->Connectivity->InitTraversal();
	for (vtkIdType i = 0; i < num_new; i++)
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
	vtkIdType num_tets = this->Tetrahedra->GetNumberOfTetrahedra();
	for (vtkIdType i = 0; i < num_tets; i++)
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

	vtkIdType num_i_cells = dims[0];
	vtkIdType num_j_cells = dims[1];
	vtkIdType slice_size = num_i_cells * num_j_cells;

	int i = (x[0] - origin[0] + 0.5 * spacing[0]) / spacing[0] - extent[0];
	int j = (x[1] - origin[1] + 0.5 * spacing[1]) / spacing[1] - extent[2];
	int k = (x[2] - origin[2] + 0.5 * spacing[2]) / spacing[2] - extent[4];

	if (i >= 0 && i < dims[0] && j >= 0 && j < dims[1] && k >= 0 && k < dims[2])
	{
		vtkIdType cell_id = i + j * num_i_cells + k * slice_size;

		assert(cell_id >= 0 && cell_id < this->ClipScalars->GetNumberOfTuples());
		int dom = this->ClipScalars->GetTuple1(cell_id);
		return dom;
	}
	std::cout << "Warning: probing outside image " << i << " " << j << " " << k
						<< std::endl;
	return this->BackgroundLabel;
}

void vtkImageExtractCompatibleMesher::ExtractMeshDomainInterfaces(vtkInformationVector* outputVector)
{
	vtkInformation* out_info = outputVector->GetInformationObject(0);
	vtkPolyData* output =
			vtkPolyData::SafeDownCast(out_info->Get(vtkDataObject::DATA_OBJECT()));

	if (!this->CellDomainArray)
	{
		vtkErrorMacro(<< "Cannot run without cell labels");
		return;
	}
	vtkIdType n_cells = this->Tetrahedra->GetNumberOfTetrahedra();
	vtkIdType n_pts = this->Points->GetNumberOfPoints();

	// setup algorithm data structures
	vtkIdType neighbor_id;
	vtkNew(vtkIdList, facePtIds);
	facePtIds->SetNumberOfIds(3);

	vtkNew(vtkShortArray, domainArray);
	domainArray->SetName(OutputScalarName);
	domainArray->Allocate(n_cells, n_cells / 2);
	output->GetCellData()->AddArray(domainArray);
	vtkNew(vtkCellArray, cells);
	cells->Allocate(n_cells / 2, n_cells / 2);
	vtkNew(vtkPoints, points);
	output->SetPoints(points);

	std::vector<bool> visited(n_cells, false);
	std::vector<vtkIdType> pt_id_mapping(n_pts, -1);
	vtkIdType tri1_ids[3], tri2_ids[3];
	double x[3];
	int abort = 0;
	vtkIdType check_every = n_cells / 20;
	if (check_every == 0)
		check_every = 1;
	for (vtkIdType cell_id = 0; cell_id < n_cells && !abort; cell_id++)
	{
		if (static_cast<int>(this->CellDomainArray->GetTuple1(cell_id)) ==
				BackgroundLabel)
			continue;

		// Check for progress and abort
		if (cell_id % check_every == 0)
		{
			this->UpdateProgress(static_cast<double>(cell_id) / n_cells);
			abort = this->GetAbortExecute();
		}

		visited[cell_id] = true;
		const Tetrahedron& tet = this->Tetrahedra->GetTetrahedron(cell_id);
		for (int k = 0; k < 4; k++)
		{
			facePtIds->SetId(0, tet[tet_faces[k][0]]);
			facePtIds->SetId(1, tet[tet_faces[k][1]]);
			facePtIds->SetId(2, tet[tet_faces[k][2]]);

			if (!this->Tetrahedra->HasNeighbor(cell_id, k))
			{
				// boundary tet: add face
				if (pt_id_mapping[facePtIds->GetId(0)] < 0)
				{
					this->Points->GetPoint(facePtIds->GetId(0), x);
					pt_id_mapping[facePtIds->GetId(0)] =
							points->InsertNextPoint(x);
				}
				if (pt_id_mapping[facePtIds->GetId(1)] < 0)
				{
					this->Points->GetPoint(facePtIds->GetId(1), x);
					pt_id_mapping[facePtIds->GetId(1)] =
							points->InsertNextPoint(x);
				}
				if (pt_id_mapping[facePtIds->GetId(2)] < 0)
				{
					this->Points->GetPoint(facePtIds->GetId(2), x);
					pt_id_mapping[facePtIds->GetId(2)] =
							points->InsertNextPoint(x);
				}

				tri1_ids[0] = pt_id_mapping[facePtIds->GetId(0)];
				tri1_ids[1] = pt_id_mapping[facePtIds->GetId(1)];
				tri1_ids[2] = pt_id_mapping[facePtIds->GetId(2)];
				cells->InsertNextCell(3, tri1_ids);
				domainArray->InsertNextValue(this->CellDomainArray->GetTuple1(cell_id));
			}
			else if (visited[neighbor_id = this->Tetrahedra->GetNeighbor(cell_id, k)] == false)
			{
				// neighbor has not been visited yet
				if (this->CellDomainArray->GetTuple1(cell_id) !=
						this->CellDomainArray->GetTuple1(neighbor_id))
				{
					// label is different: add face
					if (pt_id_mapping[facePtIds->GetId(0)] < 0)
					{
						this->Points->GetPoint(facePtIds->GetId(0), x);
						pt_id_mapping[facePtIds->GetId(0)] =
								points->InsertNextPoint(x);
					}
					if (pt_id_mapping[facePtIds->GetId(1)] < 0)
					{
						this->Points->GetPoint(facePtIds->GetId(1), x);
						pt_id_mapping[facePtIds->GetId(1)] =
								points->InsertNextPoint(x);
					}
					if (pt_id_mapping[facePtIds->GetId(2)] < 0)
					{
						this->Points->GetPoint(facePtIds->GetId(2), x);
						pt_id_mapping[facePtIds->GetId(2)] =
								points->InsertNextPoint(x);
					}
					tri1_ids[0] = pt_id_mapping[facePtIds->GetId(0)];
					tri1_ids[1] = pt_id_mapping[facePtIds->GetId(1)];
					tri1_ids[2] = pt_id_mapping[facePtIds->GetId(2)];
					cells->InsertNextCell(3, tri1_ids);
					domainArray->InsertNextValue(this->CellDomainArray->GetTuple1(cell_id));

					// add duplicate, unless the neighbor is background
					if (static_cast<int>(this->CellDomainArray->GetTuple1(neighbor_id)) != BackgroundLabel)
					{
						tri2_ids[0] = pt_id_mapping[facePtIds->GetId(0)];
						tri2_ids[1] = pt_id_mapping[facePtIds->GetId(2)];
						tri2_ids[2] = pt_id_mapping[facePtIds->GetId(1)];
						cells->InsertNextCell(3, tri2_ids);
						domainArray->InsertNextValue(this->CellDomainArray->GetTuple1(neighbor_id));
					}
				}
			}
		}
	}
	output->SetPolys(cells);
	std::cerr << "Number of tetra: " << n_cells << std::endl;
	std::cerr << "Number of triangles: " << cells->GetNumberOfCells()
						<< std::endl;
}
