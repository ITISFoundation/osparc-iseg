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

#include <unordered_set>
#include <utility>

#include <itkConstShapedNeighborhoodIterator.h>
#include <itkNeighborhoodAlgorithm.h>

namespace iseg {

struct Edge
{
	Edge() {}
	Edge(size_t _n1, size_t _n2) : n1(_n1), n2(_n2)
	{
		if (n1 > n2)
		{
			std::swap(n1, n2);
		}
	}

	inline bool operator==(const Edge& rhs) const
	{
		return n1 == rhs.n1 && n2 == rhs.n2;
	}

	inline size_t operator()(const Edge& e) const { return n1; }

	size_t n1, n2;
};

template<class TImage>
std::unordered_set<Edge, Edge> ImageConnectivityGraph(TImage* image, typename TImage::RegionType region)
{
	itkStaticConstMacro(Dimension, unsigned int, TImage::ImageDimension);

	std::unordered_set<Edge, Edge> edges;

	itk::Size<Dimension> radius;
	radius.Fill(1);

	std::vector<bool> axis_aligned(27, false);
	{
		using neighborhood_type = itk::Neighborhood<char,3>;
		neighborhood_type neighborhood;
		neighborhood.SetRadius(radius);
		for(int k=-1; k<=1; k+=2)
		{
			for(int j=-1; j<=1; j+=2)
			{
				for(int i=-1; i<=1; i+=2)
				{
					neighborhood_type::OffsetType o = {{i, j, k}};
					axis_aligned.at(neighborhood.GetNeighborhoodIndex(o)) = true;
				}
			}
		}
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
						std::cerr << i.GetNeighborhoodIndex() << " = id\n";
						if (axis_aligned.at(i.GetNeighborhoodIndex()))
						{
							std::cerr << "found axis aligned\n";
						}
						edges.insert(Edge(center_idx, n_idx));
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
						edges.insert(Edge(center_idx, n_idx));
					}
				}
			}
		}
	}

	return edges;
}

} // namespace iseg
