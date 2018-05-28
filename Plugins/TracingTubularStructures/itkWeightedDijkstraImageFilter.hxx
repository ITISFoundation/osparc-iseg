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

namespace itk {

template<typename TInputImageType, typename TMetric>
WeightedDijkstraImageFilter<TInputImageType, TMetric>::
		WeightedDijkstraImageFilter()
{
}

template<typename TInputImageType, typename TMetric>
void WeightedDijkstraImageFilter<TInputImageType, TMetric>::
		SetStartIndex(const typename TInputImageType::IndexType& StartIndex)
{
	m_StartIndex = StartIndex;
}

template<typename TInputImageType, typename TMetric>
void WeightedDijkstraImageFilter<TInputImageType, TMetric>::
		SetEndIndex(const typename TInputImageType::IndexType& EndIndex)
{
	m_EndIndex = EndIndex;
}

template<typename TInputImageType, typename TMetric>
void WeightedDijkstraImageFilter<TInputImageType, TMetric>::
		GenerateData()
{
	using neighborhood_iterator_type = ConstNeighborhoodIterator<ImageType>;

	using vertex_t = IndexType;
	using weight_t = float;
	using weight_vertex_pair_t = std::pair<weight_t, vertex_t>;

	typename ImageType::ConstPointer image = this->GetInput();
	m_Metric.Initialize(image, m_StartIndex, m_EndIndex);

	using offset_table_t = typename ImageType::RegionType::OffsetTableType;
	offset_table_t offset_table;
	m_Region.ComputeOffsetTable(offset_table);

	auto ijk2index = [&](const typename ImageType::IndexType& ijk) {
		typename ImageType::OffsetValueType offset = 0;
		ImageHelper<InputImageDimension, InputImageDimension>::ComputeOffset(m_Region.GetIndex(), ijk, offset_table, offset);
		return offset;
	};

	size_t source = ijk2index(m_StartIndex);
	size_t target = ijk2index(m_EndIndex);
	size_t N = m_Region.GetNumberOfPixels();

	auto dummy_img = ImageType::New();
	dummy_img->SetLargestPossibleRegion(image->GetLargestPossibleRegion());
	dummy_img->SetBufferedRegion(m_Region);

	neighborhood_iterator_type::RadiusType radius;
	radius.Fill(1);
	neighborhood_iterator_type nit(radius, dummy_img, m_Region);

	std::vector<bool> is_boundary(N, false);
	NeighborhoodAlgorithm::ImageBoundaryFacesCalculator<ImageType> face_calculator;
	auto faces = face_calculator(dummy_img.GetPointer(), m_Region, radius);
	if (!faces.empty())
		faces.erase(faces.begin());
	for (auto face_region : faces)
	{
		ImageRegionConstIterator<ImageType> it(image, face_region);
		for (it.GoToBegin(); !it.IsAtEnd(); ++it)
		{
			is_boundary.at(ijk2index(it.GetIndex())) = true;
		}
	}
	bool any_boundary = std::count(is_boundary.begin(), is_boundary.end(), true);

	std::vector<weight_t> min_distance(N, std::numeric_limits<weight_t>::max());
	static const vertex_t kInvalidIndex = {-1, -1, -1};
	std::vector<vertex_t> previous(N, kInvalidIndex);

	min_distance[source] = 0;

	// we use greater instead of less to turn max-heap into min-heap
	struct Greater
	{
		constexpr bool operator()(const weight_vertex_pair_t& l, const weight_vertex_pair_t& r) const
		{
			return (l.first > r.first);
		}
	};
	std::priority_queue<weight_vertex_pair_t,
			std::vector<weight_vertex_pair_t>, Greater>
			vertex_queue;
	vertex_queue.push(std::make_pair(min_distance[source], m_StartIndex));

	while (!vertex_queue.empty())
	{
		weight_t dist = vertex_queue.top().first;
		vertex_t u = vertex_queue.top().second;
		size_t uid = ijk2index(u);
		if (uid == target)
			break;

		vertex_t uprev = (uid == source) ? m_StartIndex : previous[uid];
		vertex_queue.pop();

		// Because we leave old copies of the vertex in the priority queue
		// (with outdated higher distances), we need to ignore it when we come
		// across it again, by checking its distance against the minimum distance
		if (dist > min_distance[uid])
			continue;

		// Visit each edge exiting u
		nit.SetLocation(u);

		for (unsigned int i = 0; i < nit.Size(); i++)
		{
			if (is_boundary[uid] && !nit.IndexInBounds(i))
				continue;

			vertex_t v = nit.GetIndex(i);
			size_t vid = ijk2index(v);

			weight_t weight = m_Metric.GetEdgeWeight(u, v, uprev);
			weight_t distance_through_u = dist + weight;

			if (distance_through_u < min_distance[vid])
			{
				min_distance[vid] = distance_through_u;
				previous[vid] = u;
				vertex_queue.push(std::make_pair(min_distance[vid], v));
			}
		}
	}
	std::list<vertex_t> path;
	for (vertex_t vertex = m_EndIndex; vertex != kInvalidIndex;)
	{
		path.push_front(vertex);
		auto index = ijk2index(vertex);
		vertex = previous[index];
	}

	PathType::Pointer output = this->GetOutput(0);
	if (output.IsNull())
	{
		output = static_cast<PathType*>(this->MakeOutput(0).GetPointer());
		this->SetNthOutput(0, output.GetPointer());
	}
	PathType::VertexListType::Pointer output_path = output->GetVertexList();
	output_path->Initialize();
	output_path->reserve(path.size()); // use std::vector version of 'reserve()'

	for (auto vertex : path)
	{
		output_path->push_back(vertex);
	}

	output->Modified();
}

template<typename TInputImageType, typename TMetric>
void WeightedDijkstraImageFilter<TInputImageType, TMetric>::
		PrintSelf(std::ostream& os, Indent indent) const
{
	Superclass::PrintSelf(os, indent);
}

} // namespace itk
