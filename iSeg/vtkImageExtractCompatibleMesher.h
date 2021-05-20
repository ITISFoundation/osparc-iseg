/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef __vtkImageExtractCompatibleMesher_h
#define __vtkImageExtractCompatibleMesher_h

#include "vtkPolyDataAlgorithm.h"

class vtkImageData;
class vtkUnstructuredGrid;
class vtkOrderedTriangulator;
class vtkIncrementalPointLocator;
class vtkCellArray;
class vtkShortArray;
class vtkIdList;
class vtkPoints;
//BTX
class TetContainer;
class vtkTriangulatorImpl;
//ETX

/**	\brief Extract compatible multi-domain surface mesh from label field

	The main difference to e.g. marching cubes for multiple domains, is that it does
	not produce any 'holes' at junctions between more than 2 materials.

	This class implements a similar algorithm to the vtkImageExtractCompatibleDomainSurfaces.
	The main difference is that no iterations are needed; everything is computed using subdivision
	templates. It uses a leaner data-structure (unsigned int for indices, different technique 
	for tet neighbor computation).

	\note You should set the output name of the scalars. It will be used to differentiate
	between different materials (e.g. in vtkEdgeCollapse)

	\see vtkEdgeCollapse
*/
class vtkImageExtractCompatibleMesher : public vtkPolyDataAlgorithm
{
public:
	static vtkImageExtractCompatibleMesher* New();
	vtkTypeMacro(vtkImageExtractCompatibleMesher, vtkPolyDataAlgorithm);
	void PrintSelf(ostream& os, vtkIndent indent) override;

	// If you don't set the name, by default the input name "Material" will be used.
	vtkSetStringMacro(OutputScalarName) vtkGetStringMacro(OutputScalarName);

	// The background value will be ignored, i.e. it will be the outside domain
	vtkSetMacro(BackgroundLabel, int) vtkGetMacro(BackgroundLabel, int);

	// Set the ordering for odd/even voxels, such that each voxel
	// is triangulated using 5 tetrahedra: Default On
	// Honestly, I am not sure if there is any reason to switch this off
	vtkSetMacro(FiveTetrahedraPerVoxel, bool);
	vtkGetMacro(FiveTetrahedraPerVoxel, bool);
	vtkBooleanMacro(FiveTetrahedraPerVoxel, bool);

	// Create voxel center point if there are more than N=2 labels per
	// voxel: Default Off
	//vtkSetMacro(CreateVoxelCenterPoint,bool)
	//vtkGetMacro(CreateVoxelCenterPoint,bool)
	//vtkBooleanMacro(CreateVoxelCenterPoint,bool)

	// Use templates for triangulation: Default On
	vtkSetMacro(UseTemplates, bool) vtkGetMacro(UseTemplates, bool);
	vtkBooleanMacro(UseTemplates, bool);

	// Use octree point locator: Default On
	vtkSetMacro(UseOctreeLocator, bool) vtkGetMacro(UseOctreeLocator, bool);
	vtkBooleanMacro(UseOctreeLocator, bool);

	// Generate tetrahedral in second output: Default Off
	// Attention: this will consume lots of memory
	vtkSetMacro(GenerateTetMeshOutput, bool);
	vtkGetMacro(GenerateTetMeshOutput, bool);
	vtkBooleanMacro(GenerateTetMeshOutput, bool);

	// Number of iterations used to label tetrahedra
	//vtkSetMacro(MaxNumberOfIterations,int)
	//vtkGetMacro(MaxNumberOfIterations,int)

	// Test if there are any unassigned labels after running the filter
	// vtkGetMacro(NumberOfUnassignedLabels,int);

protected:
	vtkImageExtractCompatibleMesher();
	~vtkImageExtractCompatibleMesher() override;

	int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
	int FillInputPortInformation(int port, vtkInformation* info) override;
	int FillOutputPortInformation(int port, vtkInformation* info) override;

	// Use a contouring method (similar to discrete marching cubes)
	// to extract the surfaces between different material regions
	int ContourSurface(vtkInformationVector**, vtkInformationVector*);

	void ClipVoxel(vtkShortArray* cellScalars, int flip, double spacing[3], vtkIdList* cellIds, vtkPoints* cellPts);

	// Helper function for ContourSurface
	void TriangulateVoxel(int cellScalar, int flip, double spacing[3], vtkIdList* cellIds, vtkPoints* cellPts);

	// Helper function for ContourSurface
	int EvaluateLabel(double x[3]);

	// Helper function for ContourSurface:
	// Extract domain interfaces/surfaces from tetrahedral mesh
	void ExtractMeshDomainInterfaces(vtkInformationVector*);

	void GenerateTetMesh(vtkUnstructuredGrid* grid);

	//BTX
	enum {
		SURFACE_DOMAIN = VTK_SHORT_MIN,
		UNSURE_DOMAIN = VTK_SHORT_MIN + 1,
	};
	//ETX

	// Parameters
	bool FiveTetrahedraPerVoxel;
	bool CreateVoxelCenterPoint;
	int BackgroundLabel;
	bool UseTemplates;
	bool UseOctreeLocator;
	char* OutputScalarName;
	bool GenerateTetMeshOutput;
	int MaxNumberOfIterations;

	// Data
	vtkImageData* Input;
	vtkDataArray* ClipScalars;
	vtkPoints* Points;
	vtkOrderedTriangulator* Triangulator;
	vtkIncrementalPointLocator* Locator;
	vtkCellArray* Connectivity;
	vtkShortArray* PointDomainArray;
	vtkShortArray* CellDomainArray;
	//BTX
	TetContainer* Tetrahedra;
	vtkTriangulatorImpl* MyTriangulator;
	//ETX

	int NumberOfUnassignedLabels;

	//BTX
	// For testing, the class is defined in the corresponding unit test
	friend class vtkUnitTesting;
	//ETX

private:
	vtkImageExtractCompatibleMesher(const vtkImageExtractCompatibleMesher&) = delete; // Not implemented
	void operator=(const vtkImageExtractCompatibleMesher&) = delete;									 // Not implemented
};

#endif
