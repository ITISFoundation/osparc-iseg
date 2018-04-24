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

#include "Transform.h"
#include "VoxelSurface.h"

#include <vtkBoundingBox.h>
#include <vtkCutter.h>
#include <vtkImageData.h>
#include <vtkImageStencil.h>
#include <vtkLinearExtrusionFilter.h>
#include <vtkNew.h>
#include <vtkPlane.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataToImageStencil.h>
#include <vtkSTLReader.h>
#include <vtkStripper.h>
#include <vtkTransform.h>
#include <vtkTriangle.h>
#include <vtkUnsignedCharArray.h>

using namespace iseg;

namespace {

std::vector<double> to_double(const Transform& transform)
{
	auto begin = static_cast<const float*>(transform[0]);
	return std::vector<double>(begin, begin + 16);
}

float round_me_up(float n, const float to_what)
{
	n *= (1 / to_what);
	n = ceil(n);
	n /= (1 / to_what);
	return n;
}

float round_me_dn(float n, const float to_what)
{
	n *= (1 / to_what);
	n = floor(n);
	n /= (1 / to_what);
	return n;
}

} // namespace

VoxelSurface::eSurfaceImageOverlap
		VoxelSurface::Run(const char* filename, const unsigned dims[3],
				const float spacing[3], const Transform& transform,
				float** slices, unsigned startslice,
				unsigned endslice) const
{
	vtkNew<vtkSTLReader> reader;
	reader->SetFileName(filename);
	reader->Update();
	if (reader->GetOutput())
	{
		return Run(reader->GetOutput(), dims, spacing, transform, slices,
				startslice, endslice);
	}
	return eSurfaceImageOverlap::kNone;
}

VoxelSurface::eSurfaceImageOverlap
		VoxelSurface::Run(vtkPolyData* surface, const unsigned dims[3],
				const float spacing[3], const Transform& transform,
				float** slices, unsigned startslice,
				unsigned endslice) const
{
	// instead of applying the transform to the image, we inverse transform the surface
	auto m = to_double(transform);
	vtkNew<vtkTransform> inverse_transform;
	inverse_transform->SetMatrix(m.data());
	inverse_transform->Inverse();

	vtkNew<vtkPoints> points;
	inverse_transform->TransformPoints(surface->GetPoints(), points.Get());

	vtkBoundingBox surface_bb(points->GetBounds());

	// image wo transform
	vtkBoundingBox image_bb(0, (dims[0] - 1) * spacing[0], 0,
			(dims[1] - 1) * spacing[1], 0,
			(dims[2] - 1) * spacing[2]);

	eSurfaceImageOverlap result = kNone;
	if (image_bb.Contains(surface_bb))
	{
		result = eSurfaceImageOverlap::kContained;
	}
	else if (image_bb.Intersects(surface_bb))
	{
		result = eSurfaceImageOverlap::kPartial;
	}
	else
	{
		return eSurfaceImageOverlap::kNone;
	}

	vtkNew<vtkPolyData> surface_inverse_transformed;
	surface_inverse_transformed->CopyStructure(surface);
	surface_inverse_transformed->SetPoints(points.Get());
	surface_inverse_transformed->BuildCells();

	// loop through all the slices in the dataset
	int zbegin = static_cast<int>(startslice);
	int zend = static_cast<int>(endslice);

	//#pragma omp parallel for -> leads to crashes, because input->GetBounds is called in Cutter
	for (int z = zbegin; z < zend; z++)
	{
		// REMEMBER: image is NOT transformed
		auto zPos = z * spacing[2];

		// do not even try cutting if we are out of the surface bounding box
		if (zPos < surface_bb.GetBound(4) || zPos > surface_bb.GetBound(5))
			continue;

		// place the cutter plane
		vtkNew<vtkPlane> plane;
		plane->SetNormal(0, 0, 1);
		plane->SetOrigin(0, 0, zPos);

		// cut the surface with the plane
		vtkNew<vtkCutter> cutter;
		cutter->SetCutFunction(plane.Get());
		cutter->SetInputData(surface_inverse_transformed.Get());

		// extract the contour from the plane
		vtkNew<vtkStripper> stripper;
		stripper->SetInputConnection(cutter->GetOutputPort()); // valid circle
		stripper->Update();

		vtkPolyData* contour = stripper->GetOutput();

		// check whether there is any contour
		auto c_numberOfLines = contour->GetNumberOfLines();
		if (c_numberOfLines > 0)
		{
			// create a 2D black image of the same size of the contour
			vtkNew<vtkImageData> imageData;

			vtkBoundingBox ct_bounds;
			{
				// enlarge the bounding box of the contour to fit the indices of the dataset
				double bb[6];
				contour->GetBounds(bb);
				bb[0] = round_me_dn(bb[0], spacing[0]);
				bb[1] = round_me_up(bb[1], spacing[0]);
				bb[2] = round_me_dn(bb[2], spacing[1]);
				bb[3] = round_me_up(bb[3], spacing[1]);
				ct_bounds.SetBounds(bb);
			}
			// crop if surface is partially outside image
			ct_bounds.IntersectBox(image_bb);
			int ct_dim[3] = {};
			for (int d = 0; d < 3; d++)
			{
				ct_dim[d] = static_cast<int>(std::ceil(ct_bounds.GetLength(d) / spacing[d])) + 1;
			}
			const double* ct_origin = ct_bounds.GetMinPoint();

			imageData->SetDimensions(ct_dim[0], ct_dim[1], 1);
			imageData->SetSpacing(spacing[0], spacing[1], spacing[2]);
			imageData->SetOrigin(ct_origin[0], ct_origin[1], ct_origin[2]);
			imageData->AllocateScalars(VTK_UNSIGNED_CHAR, 1);
			vtkUnsignedCharArray* scalars = vtkUnsignedCharArray::SafeDownCast(imageData->GetPointData()->GetScalars());

			// Fill the image with foreground voxel values
			unsigned char inval = 255;
			unsigned char outval = 0;
			scalars->FillComponent(0, inval);

			// Take the Contour PolyData and fill it
			// Sweep polygonal data (this is the important thing with contours!)
			vtkNew<vtkLinearExtrusionFilter> extruder;
			extruder->SetInputData(contour);
			extruder->SetScaleFactor(1.);
			extruder->SetExtrusionTypeToNormalExtrusion();
			extruder->SetVector(0, 0, 1);
			extruder->Update();

			// polygonal data --> image stencil:
			vtkNew<vtkPolyDataToImageStencil> pol2stenc;
			pol2stenc->SetTolerance(0); // important if extruder->SetVector(0, 0, 1) !!!
			pol2stenc->SetInputConnection(extruder->GetOutputPort());
			pol2stenc->SetOutputOrigin(imageData->GetOrigin());
			pol2stenc->SetOutputSpacing(imageData->GetSpacing());
			pol2stenc->SetOutputWholeExtent(imageData->GetExtent());
			pol2stenc->Update();

			// Paste the Contour PolyData on the black image
			// Cut the corresponding white image and set the background:
			vtkNew<vtkImageStencil> imgstenc;
			imgstenc->SetInputData(imageData.Get());
			imgstenc->SetStencilConnection(pol2stenc->GetOutputPort());
			imgstenc->ReverseStencilOff();
			imgstenc->SetBackgroundValue(outval);
			imgstenc->Update();

			// Add or overwrite the white pixels in the working array
			vtkImageData* out = imgstenc->GetOutput();
			float* workBits = slices[z];
			if (out->GetNumberOfPoints() > 0)
			{
				int offset_x = (int)floor(abs(ct_origin[0] / spacing[0]) + 0.5);
				int offset_y = (int)floor(abs(ct_origin[1] / spacing[1]) + 0.5);

				for (int ct_y = 0; ct_y < ct_dim[1] - 1; ct_y++)
				{
					for (int ct_x = 0; ct_x < ct_dim[0] - 1; ct_x++)
					{
						unsigned char* pixel_val = static_cast<unsigned char*>(out->GetScalarPointer(ct_x, ct_y, 0));
						if (pixel_val[0] == inval)
						{
							// Convert the relative x,y from the contour to the x,y of the dataset
							int ds_x = offset_x + ct_x;
							int ds_y = offset_y + ct_y;

							int index = ds_x + ds_y * (dims[0]);
							workBits[index] = m_ForeGroundValue;
						}
					}
				}
			}
		}
	}

	return result;
}

VoxelSurface::eSurfaceImageOverlap iseg::VoxelSurface::Intersect(const char* filename, const unsigned dims[3], const float spacing[3], const Transform& transform, float** slices, unsigned startslice, unsigned endslice)
{
	vtkNew<vtkSTLReader> reader;
	reader->SetFileName(filename);
	reader->Update();
	auto surface = reader->GetOutput();

	// instead of applying the transform to the image, we inverse transform the surface
	auto m = to_double(transform);
	vtkNew<vtkTransform> inverse_transform;
	inverse_transform->SetMatrix(m.data());
	inverse_transform->Inverse();

	vtkNew<vtkPoints> points;
	inverse_transform->TransformPoints(surface->GetPoints(), points.Get());
	surface->SetPoints(points.Get());

	vtkBoundingBox surface_bb(points->GetBounds());

	// image wo transform
	vtkBoundingBox image_bb(0, (dims[0] - 1) * spacing[0], 0,
			(dims[1] - 1) * spacing[1], 0,
			(dims[2] - 1) * spacing[2]);

	eSurfaceImageOverlap result = kNone;
	if (image_bb.Contains(surface_bb))
	{
		result = eSurfaceImageOverlap::kContained;
	}
	else if (image_bb.Intersects(surface_bb))
	{
		result = eSurfaceImageOverlap::kPartial;
	}
	else
	{
		return eSurfaceImageOverlap::kNone;
	}

	surface->BuildCells();
	auto num_cells = surface->GetNumberOfCells();

	double tol = (*std::min_element(spacing, spacing+3)) / 10.0;
	vtkIdType *pts, npts;
	double bb[6];
	int bbi[6];
	for (vtkIdType id = 0; id < num_cells; ++id)
	{
		surface->GetCellPoints(id, npts, pts);
		if (npts == 3)
		{
			surface->GetCellBounds(id, bb);
			// round up/down to gridlines
			for (int d = 0; d < 3; ++d)
			{
				bbi[2 * d] = std::floor(std::max(0.0, bb[2 * d]) / spacing[d]);
				bbi[2 * d + 1] = std::min<int>(static_cast<int>(dims[d]), bb[2 * d + 1] / spacing[d] + 1);
			}

			vtkCell* cell = surface->GetCell(id);

			// iterate over grid lines (X-lines, Y-lines, Z-lines)
			auto intersect = [&](int const dir[3]) {
				double pa[3] = {};
				double pe[3] = {};
				unsigned int ijk[3] = {};
				int subId;
				double t, x[3], pcoords[3];

				pa[dir[2]] = bbi[dir[2] * 2] * spacing[dir[2]];
				pe[dir[2]] = bbi[dir[2] * 2 + 1] * spacing[dir[2]];
				double const scale = bbi[dir[2] * 2 + 1] - bbi[dir[2] * 2];

				for (int i = bbi[dir[0] * 2]; i < bbi[dir[0] * 2 + 1]; ++i)
				{
					for (int j = bbi[dir[1] * 2]; j < bbi[dir[1] * 2 + 1]; ++j)
					{
						pa[dir[0]] = pe[dir[0]] = i * spacing[dir[0]];
						pa[dir[1]] = pe[dir[1]] = j * spacing[dir[1]];

						if (cell->IntersectWithLine(pa, pe, tol, t, x, pcoords, subId) != 0)
						{
							ijk[dir[0]] = i;
							ijk[dir[1]] = j;
							ijk[dir[2]] = bbi[dir[2] * 2] + std::floor(scale * t);

							if (ijk[0] < dims[0] && 
								ijk[1] < dims[1] &&
								ijk[2] < dims[2])
							{
								float* slice = slices[ijk[2]];
								slice[ijk[0] + dims[0] * ijk[1]] = m_ForeGroundValue;
							}
						}
					}
				}
			};

			int along_z[3] = {0, 1, 2};
			intersect(along_z);

			int along_y[3] = {0, 2, 1};
			intersect(along_y);

			int along_x[3] = {1, 2, 0};
			intersect(along_x);
		}
	}
	return result;
}
