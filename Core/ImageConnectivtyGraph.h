/*
* Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace iseg {

struct Edge : public std::array<size_t, 2>
{
	Edge(size_t _n1 = 0, size_t _n2 = 0)
	{
		(*this)[0] = _n1 < _n2 ? _n1 : _n2;
		(*this)[1] = _n1 < _n2 ? _n2 : _n1;
	}

	inline size_t operator()(const Edge& e) const { return (*this)[0]; }
};

template<class TImage>
std::vector<Edge> ImageConnectivityGraph(TImage* image, typename TImage::RegionType region)
{
	itkStaticConstMacro(Dimension, unsigned int, TImage::ImageDimension);

	std::unordered_set<Edge, Edge> aligned_set;
	std::unordered_set<Edge, Edge> diag_set;

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
				for (auto i = it.Begin(), i_end = it.End(); i != i_end; ++i)
				{
					if (i.Get() != 0)
					{
						auto n_idx = image->ComputeOffset(it.GetIndex() + i.GetNeighborhoodOffset());
						if (axis_aligned.at(i.GetNeighborhoodIndex()))
						{
							aligned_set.insert(Edge(center_idx, n_idx));
						}
						else
						{
							diag_set.insert(Edge(center_idx, n_idx));
						}
					}
				}
			}
		}
	}

	for (const auto& face_region : faces)
	{
		itk::ConstNeighborhoodIterator<TImage> it(radius, image, face_region);

		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			if (it.GetCenterPixel() != 0)
			{
				auto center_idx = image->ComputeOffset(it.GetIndex());
				for (unsigned int i = 0, i_size = it.Size(); i < i_size; i++)
				{
					bool inbounds = false;
					auto val = it.GetPixel(i, inbounds);
					if (inbounds && val != 0)
					{
						auto n_idx = image->ComputeOffset(it.GetIndex(i));
						if (axis_aligned.at(i))
						{
							aligned_set.insert(Edge(center_idx, n_idx));
						}
						else
						{
							diag_set.insert(Edge(center_idx, n_idx));
						}
					}
				}
			}
		}
	}

	// note: order here is important. first N edges are aligned edges!
	std::vector<Edge> edges(aligned_set.begin(), aligned_set.end());
	edges.insert(edges.end(), diag_set.begin(), diag_set.end());

	// map nodes to compact range so we can use const map
	std::unordered_map<size_t, unsigned int> node_id_map;
	unsigned int id = 0;
	for (auto& e : edges)
	{
		for (int k = 0; k < 2; ++k)
		{
			auto found = node_id_map.find(e[k]);
			if (found == node_id_map.end())
			{
				node_id_map[e[k]] = id;
				e[k] = id++;
			}
			else
			{
				e[k] = found->second;
			}
		}
	}

	int next_region = 0;
	std::vector<int> id_region_map(node_id_map.size(), -1);
	std::vector<int> region_connectivity(node_id_map.size(), -1);

	struct Connectivity
	{
		Connectivity(std::vector<int>& _map) : m_Map(_map) {}

		int BaseConnection(int c)
		{
			if (m_Map[c] == c)
				return c;
			else
				return m_Map[c] = BaseConnection(m_Map[c]);
		}
		std::vector<int>& m_Map;
	} ca(region_connectivity);

	std::vector<Edge> output_edges;
	for (size_t i = 0, i_end = edges.size(); i < i_end; ++i)
	{
		bool add = true;
		auto e = edges[i];

		if (id_region_map[e[0]] == -1)
		{
			if (id_region_map[e[1]] == -1)
			{
				region_connectivity[next_region] = next_region;
				id_region_map[e[1]] = next_region++;
			}
			id_region_map[e[0]] = ca.BaseConnection(id_region_map[e[1]]);
		}
		else
		{
			if (id_region_map[e[1]] == -1)
			{
				id_region_map[e[1]] = ca.BaseConnection(id_region_map[e[0]]);
			}
			else
			{
				int base0 = ca.BaseConnection(id_region_map[e[0]]);
				int base1 = ca.BaseConnection(id_region_map[e[1]]);
				if (base0 != base1)
				{
					// \warning this might break existing indirections
					region_connectivity[base0] = base1;
				}
				else
				{
					// don't add diagonal edges if nodes are already in same region
					add = (i < aligned_set.size());
				}
			}
		}

		if (add)
		{
			output_edges.push_back(e);
		}
	}

	// map back to image ids
	std::vector<size_t> id_node_map(node_id_map.size());
	for (auto p : node_id_map)
	{
		id_node_map.at(p.second) = p.first;
	}
	for (auto& e : output_edges)
	{
		e[0] = id_node_map[e[0]];
		e[1] = id_node_map[e[1]];
	}

	return output_edges;
}

} // namespace iseg
