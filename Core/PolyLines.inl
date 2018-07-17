#pragma once

#include <fstream>

namespace iseg {

namespace detail {
template<unsigned int V, typename Index>
const Index& get(const std::tuple<Index, Index>& t)
{
	return std::get<V>(t);
}
template<unsigned int V, typename Index>
const Index& get(const std::array<Index, 2>& t)
{
	return t[V];
}
} // namespace detail

template<typename Index, typename Tuple>
void EdgesConnectivity(const std::vector<Tuple>& edges, std::vector<std::vector<Index>>& connected_regions)
{
	connected_regions.clear();

	struct MaxPair
	{
		MaxPair() : Max(0) {}
		void operator()(const Tuple& t) { Max = std::max<Index>(Max, std::max<Index>(detail::get<0>(t), detail::get<1>(t))); }
		Index Max;
	};
	Index const NN = std::for_each(edges.begin(), edges.end(), MaxPair()).Max + 1;
	Index const NE = (Index)edges.size();

	std::vector<std::vector<Index>> node_edge_list(NN);
	for (Index i = 0; i < NE; i++)
	{
		auto n1 = detail::get<0>(edges[i]);
		auto n2 = detail::get<1>(edges[i]);
		if (n1 == n2)
			continue;

		node_edge_list[n1].push_back(i);
		node_edge_list[n2].push_back(i);
	}

	//std::vector<bool> is_node_visited(NN,false);
	std::vector<bool> is_edge_visited(NE, false);
	std::vector<Index> edge_queue;

	for (Index i = 0; i < NE; i++)
	{
		if (is_edge_visited[i])
			continue;

		// grow along connected edges
		std::vector<Index> edge_list;
		edge_queue.push_back(i);
		is_edge_visited[i] = true;
		while (!edge_queue.empty())
		{
			auto current_edge = edge_queue.back();
			edge_queue.pop_back();

			edge_list.push_back(current_edge);

			// add neighboring edges at nodes
			for (int n = 0; n < 2; n++)
			{
				Index nid = (n == 0) ? detail::get<0>(edges[current_edge]) : detail::get<1>(edges[current_edge]);

				if (node_edge_list[nid].size() >= 2) // difference compared to "EdgesToPolylines"
				{
					auto found = std::find_if(node_edge_list[nid].begin(), node_edge_list[nid].end(),
							[&is_edge_visited](Index eid) { return !is_edge_visited[eid]; });
					if (found != node_edge_list[nid].end())
					{
						edge_queue.push_back(*found);
						is_edge_visited[*found] = true;
					}
				}
			}
		}
		assert(!edge_list.empty());

		// sort edges to form a proper polyline
		std::vector<Index> connected_nodes;
		for (auto i: edge_list)
		{
			connected_nodes.push_back(detail::get<0>(edges[edge_list[i]]));
			connected_nodes.push_back(detail::get<1>(edges[edge_list[i]]));
		}
		std::sort(connected_nodes.begin(), connected_nodes.end());
		auto it = std::unique(connected_nodes.begin(), connected_nodes.end());
		connected_nodes.resize(std::distance(connected_nodes.begin(),it));
		connected_regions.push_back(connected_nodes);
	}
}

template<typename Index, typename Tuple>
void EdgesToPolylines(const std::vector<Tuple>& edges, std::vector<std::vector<Index>>& polylines)
{
	polylines.clear();

	struct MaxPair
	{
		MaxPair() : Max(0) {}
		void operator()(const Tuple& t) { Max = std::max<Index>(Max, std::max<Index>(detail::get<0>(t), detail::get<1>(t))); }
		Index Max;
	};
	Index const NN = std::for_each(edges.begin(), edges.end(), MaxPair()).Max + 1;
	Index const NE = (Index)edges.size();

	std::vector<std::vector<Index>> node_edge_list(NN);
	for (Index i = 0; i < NE; i++)
	{
		auto n1 = detail::get<0>(edges[i]);
		auto n2 = detail::get<1>(edges[i]);
		if (n1 == n2)
			continue;

		node_edge_list[n1].push_back(i);
		node_edge_list[n2].push_back(i);
	}

	//std::vector<bool> is_node_visited(NN,false);
	std::vector<bool> is_edge_visited(NE, false);
	std::vector<Index> edge_queue;

	for (Index i = 0; i < NE; i++)
	{
		if (is_edge_visited[i])
			continue;

		// grow edge until end points are non-manifold
		std::vector<Index> edge_list;
		edge_queue.push_back(i);
		is_edge_visited[i] = true;
		while (!edge_queue.empty())
		{
			auto current_edge = edge_queue.back();
			edge_queue.pop_back();

			edge_list.push_back(current_edge);

			// add neighboring edges at nodes
			for (int n = 0; n < 2; n++)
			{
				Index nid = (n == 0) ? detail::get<0>(edges[current_edge]) : detail::get<1>(edges[current_edge]);

				if (node_edge_list[nid].size() == 2)
				{
					auto found = std::find_if(node_edge_list[nid].begin(), node_edge_list[nid].end(),
							[&is_edge_visited](Index eid) { return !is_edge_visited[eid]; });
					if (found != node_edge_list[nid].end())
					{
						edge_queue.push_back(*found);
						is_edge_visited[*found] = true;
					}
				}
			}
		}
		assert(!edge_list.empty());

		// sort edges to form a proper polyline
		std::deque<Index> polyline;
		std::vector<bool> processed_edges(edge_list.size(), false);
		// add seed, defines direction
		polyline.push_back(detail::get<0>(edges[edge_list[0]]));
		polyline.push_back(detail::get<1>(edges[edge_list[0]]));
		processed_edges[0] = true;
		while (true)
		{
			size_t countProcessed = std::count(processed_edges.begin(), processed_edges.end(), true);
			for (size_t i = 0; i < edge_list.size(); i++)
			{
				if (!processed_edges[i])
				{
					processed_edges[i] = true;

					auto n1 = detail::get<0>(edges[edge_list[i]]);
					auto n2 = detail::get<1>(edges[edge_list[i]]);
					if (n1 == polyline.front())
						polyline.push_front(n2);
					else if (n1 == polyline.back())
						polyline.push_back(n2);
					else if (n2 == polyline.front())
						polyline.push_front(n1);
					else if (n2 == polyline.back())
						polyline.push_back(n1);
					else // if cannot append edge, leave unprocessed
						processed_edges[i] = false;
				}
			}
			// stop while loop if we are not making progress (because all are processed, or disconnected edges)
			if (countProcessed == std::count(processed_edges.begin(), processed_edges.end(), true))
			{
				assert(countProcessed == processed_edges.size());
				break;
			}
		}

		if (polyline.size() >= 2)
		{
			std::vector<Index> polyline_vector(polyline.size());
			std::transform(polyline.begin(), polyline.end(), polyline_vector.begin(),
					[](Index id) { return id; });
			polylines.push_back(polyline_vector);
		}
	}
}

template<typename Index, typename Tuple>
void PolylinesToEdges(const std::vector<std::vector<Index>>& polylines, std::vector<Tuple>& edges)
{
	edges.resize(0);
	for (auto& line : polylines)
	{
		for (size_t i = 1; i < line.size(); i++)
		{
			if (line[i - 1] != line[i])
				edges.push_back(Tuple(line[i - 1], line[i]));
		}
	}
}

template<typename Index, typename Tuple>
void RemoveDuplicateEdges(std::vector<Tuple>& edges)
{
	std::vector<Tuple> new_edges;
	std::set<Tuple> unique_keys;
	for (size_t i = 0; i < edges.size(); i++)
	{
		Tuple e = edges[i];
		if (detail::get<0>(e) == detail::get<1>(e))
			continue;
		if (detail::get<0>(e) > detail::get<1>(e))
		{
			std::swap(detail::get<0>(e), detail::get<1>(e));
		}

		if (unique_keys.find(e) == unique_keys.end())
		{
			unique_keys.insert(e);
			new_edges.push_back(edges[i]);
		}
	}

	std::swap(new_edges, edges);
}

template<typename TVec3>
void WritePolylinesToVtk(std::vector<std::vector<TVec3>>& polylines, const std::string& file_name)
{
	using bit32 = unsigned int;
	std::vector<std::vector<bit32>> polylines_idx;
	std::vector<TVec3> points;
	bit32 point_count = 0;
	size_t lines_counts = 0;
	for (auto pline : polylines)
	{
		bool is_closed = (pline.front() == pline.back());

		std::vector<bit32> iline;
		for (size_t i = 0; i < (is_closed ? pline.size() - 1 : pline.size()); i++)
		{
			iline.push_back(point_count++);
			points.push_back(pline[i]);
		}
		if (is_closed)
		{
			iline.push_back(iline.front());
		}
		polylines_idx.push_back(iline);
		lines_counts += iline.size();
	}

	const bit32 num_points = (bit32)points.size();
	const bit32 num_lines = (bit32)polylines_idx.size();

	std::ofstream ofile;
	ofile.open(file_name.c_str());

	ofile << "# vtk DataFile Version 3.0\n";
	ofile << "vtk output\n";
	ofile << "ASCII\n";
	ofile << "DATASET POLYDATA\n";

	ofile << "POINTS " << num_points << " float\n";
	for (bit32 i = 0; i < num_points; i++)
	{
		auto p = points[i];
		ofile << p[0] << " " << p[1] << " " << p[2] << "\n";
	}

	ofile << "LINES " << num_lines << " " << lines_counts + num_lines << "\n";
	for (bit32 i = 0; i < num_lines; i++)
	{
		auto line = polylines_idx[i];
		ofile << line.size();
		for (auto id : line)
			ofile << " " << id;
		ofile << "\n";
	}

	ofile.close();
}

} // namespace iseg
