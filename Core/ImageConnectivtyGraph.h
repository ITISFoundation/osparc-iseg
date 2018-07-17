/*
* Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
*
* This file is part of iSEG
* (see https://github.com/ITISFoundation/osparc-iseg).
*
* This software is released under the MIT License.
*  https://opensource.org/licenses/MIT
*/
#pragma

#include "PolyLines.h"

#include <itkConstShapedNeighborhoodIterator.h>
#include <itkNeighborhoodAlgorithm.h>

#include <unordered_set>
#include <utility>
#include <vector>

namespace iseg {

struct Edge : public std::array<size_t,2>
{
	Edge(size_t _n1=0, size_t _n2=0)
	{
		(*this)[0] = n1 < n2 ? n1 : n2;
		(*this)[1] = n1 < n2 ? n2 : n1;
	}

	inline size_t operator()(const Edge& e) const { return n1; }

	size_t n1, n2;
};

struct Edges
{
	std::vector<Edge> aligned_edges;
	std::vector<Edge> diag_edges;
};

template<class TImage>
Edges ImageConnectivityGraph(TImage* image, typename TImage::RegionType region)
{
	itkStaticConstMacro(Dimension, unsigned int, TImage::ImageDimension);

	std::unordered_set<Edge, Edge> aligned_edges;
	std::unordered_set<Edge, Edge> diag_edges;

	itk::Size<Dimension> radius;
	radius.Fill(1);

	std::vector<bool> axis_aligned(27, false);
	{
		using neighborhood_type = itk::Neighborhood<char, 3>;
		neighborhood_type neighborhood;
		neighborhood.SetRadius(radius);
		axis_aligned.at(neighborhood.GetNeighborhoodIndex({0, 0, 1})) = true;
		axis_aligned.at(neighborhood.GetNeighborhoodIndex({0, 0, -1})) = true;
		axis_aligned.at(neighborhood.GetNeighborhoodIndex({0, 1, 0})) = true;
		axis_aligned.at(neighborhood.GetNeighborhoodIndex({0, -1, 0})) = true;
		axis_aligned.at(neighborhood.GetNeighborhoodIndex({1, 0, 0})) = true;
		axis_aligned.at(neighborhood.GetNeighborhoodIndex({-1, 0, 0})) = true;
	}

	itk::NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<TImage> face_calculator;
	auto faces = face_calculator(image, region, radius);
	if (!faces.empty())
	{
		auto inner_region = faces.front();
		faces.erase(faces.begin());

		using shaped_iterator_type = itk::ConstShapedNeighborhoodIterator<TImage>;
		typename shaped_iterator_type::OffsetType o1 = {{1, 0, 0}};
		typename shaped_iterator_type::OffsetType o2 = {{0, 1, 0}};
		typename shaped_iterator_type::OffsetType o3 = {{0, 0, 1}};
		typename shaped_iterator_type::OffsetType o4 = {{1, 1, 0}};
		typename shaped_iterator_type::OffsetType o5 = {{1, 0, 1}};
		typename shaped_iterator_type::OffsetType o6 = {{0, 1, 1}};
		typename shaped_iterator_type::OffsetType o7 = {{1, 1, 1}};

		shaped_iterator_type it(radius, image, inner_region);
		it.ActivateOffset(o1);
		it.ActivateOffset(o2);
		it.ActivateOffset(o3);
		it.ActivateOffset(o4);
		it.ActivateOffset(o5);
		it.ActivateOffset(o6);
		it.ActivateOffset(o7);

		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			if (it.GetCenterPixel() != 0)
			{
				auto center_idx = image->ComputeOffset(it.GetIndex());
				for (auto i = it.Begin(); i != it.End(); ++i)
				{
					if (i.Get() != 0)
					{
						auto n_idx = image->ComputeOffset(it.GetIndex() + i.GetNeighborhoodOffset());
						if (axis_aligned.at(i.GetNeighborhoodIndex()))
						{
							aligned_edges.insert(Edge(center_idx, n_idx));
						}
						else
						{
							diag_edges.insert(Edge(center_idx, n_idx));
						}
					}
				}
			}
		}
	}

	for (auto face_region : faces)
	{
		itk::ConstNeighborhoodIterator<TImage> it(radius, image, face_region);

		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			if (it.GetCenterPixel() != 0)
			{
				auto center_idx = image->ComputeOffset(it.GetIndex());
				for (unsigned int i = 0; i < it.Size(); i++)
				{
					bool inbounds = false;
					auto val = it.GetPixel(i, inbounds);
					if (inbounds && val != 0)
					{
						auto n_idx = image->ComputeOffset(it.GetIndex(i));
						if (axis_aligned.at(i))
						{
							aligned_edges.insert(Edge(center_idx, n_idx));
						}
						else
						{
							diag_edges.insert(Edge(center_idx, n_idx));
						}
					}
				}
			}
		}
	}

	std::vector<std::vector<size_t>> lines;
	EdgesToPolylines(std::vector<Edge>(aligned_edges.begin(), aligned_edges.end()), lines);

	// next line id = lines.size()
	// map node id to line id

	// for each edge, check nodes
	//   if nodes belong to same line, skip edge
	//   if nodes belong to different lines, merge lines and add edge
	//   if exactly one node belongs to line id, add other node to same line
	//   if nodes don't belong to any line, add new line

	Edges edges;
	edges.aligned_edges.assign(aligned_edges.begin(), aligned_edges.end());
	edges.diag_edges.assign(diag_edges.begin(), diag_edges.end());
	return edges;
}

} // namespace iseg
