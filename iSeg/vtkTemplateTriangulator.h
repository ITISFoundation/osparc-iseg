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

#include <vtkObject.h>

class vtkEdgeTable;

/**	\brief Base class for algorithms which use subdivision templates to 
	create compatible tetrahedral meshes.

	Classes, which inherit from vtkTemplateTriangulator should implement, 
	where and how the subdivided tetrahedra are inserted. This class is not 
	a filter, since some places where it can be used might not want to copy 
	the full mesh.

	\todo Allow user to select criterion to choose diagonal for splitting 
	(in unique way), e.g. the ordered Delaunay triangulator (not at max id), 
	the "classical" approach (at min id), or using the domain labels in
	addition to the node ids.

	\todo Add API to do general edge division
	\todo Add API to do division for multi-domain meshes, where corner is on 
	interface between domains
*/
class vtkTemplateTriangulator : public vtkObject
{
public:
	vtkTypeMacro(vtkTemplateTriangulator, vtkObject);
	void PrintSelf(ostream &os, vtkIndent indent);

	/// Divide tetrahedron given domain labels at corners
	/// \note does not support nodes on interface (with more than one label)
	void AddMultipleDomainTetrahedron(const vtkIdType currentTetra[4],
																		const int domain[4]);

protected:
	vtkTemplateTriangulator();
	virtual ~vtkTemplateTriangulator();

	/// Get point in mesh
	virtual void GetPoint(vtkIdType v, double p[3]) = 0;

	/// Add point to mesh
	virtual vtkIdType AddPoint(double x, double y, double z) = 0;
	vtkIdType AddPoint(double p[3]) { return AddPoint(p[0], p[1], p[2]); }

	/// Add tetrahedron to mesh
	virtual void AddTetrahedron(vtkIdType v0, vtkIdType v1, vtkIdType v2,
															vtkIdType v3, int domain) = 0;

	/// Subdivide pyramid: the quad is defined by nodes 1-4, the tip by node 5
	int AddPyramid(vtkIdType v0, vtkIdType v1, vtkIdType v2, vtkIdType v3,
								 vtkIdType v4, int domain);

	/// Subdivide prism: nodes 1-3 bottom triangle, 4-6 top triangle
	int AddPrism(vtkIdType v0, vtkIdType v1, vtkIdType v2, vtkIdType v3,
							 vtkIdType v4, vtkIdType v5, int domain);

	/// Subdivide hexahedron: nodes 1-4 bottom quad, 5-8 top quad, single domain
	/// \note The convention corresponds to VTK_HEXAHEDRON (not VTK_VOXEL)
	int AddHexahedron(vtkIdType v0, vtkIdType v1, vtkIdType v2, vtkIdType v3,
										vtkIdType v4, vtkIdType v5, vtkIdType v6, vtkIdType v7,
										int domain);

	/// Determine subdivision case from lookup table
	short DetermineTetraSubdivisionCase(short code[6], short edgeMap[6] = NULL,
																			short vertexMap[4] = NULL,
																			short triangleMap[4] = NULL);

	class vtkPimple;
	vtkPimple *Pimple;

private:
	vtkTemplateTriangulator(const vtkTemplateTriangulator &); // Not implemented.
	void operator=(const vtkTemplateTriangulator &);					// Not implemented.
};
