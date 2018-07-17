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

#include <algorithm>
#include <cassert>
#include <deque>
#include <tuple>
#include <vector>


namespace iseg {

/** \brief Function which receives topological edges and returns polylines, which are split at non-manifold verts
	*/
template<typename Index, typename Tuple>
void EdgesToPolylines(const std::vector<Tuple>& edges, std::vector<std::vector<Index>>& polylines);

template<typename Index, typename Tuple>
void EdgesConnectivity(const std::vector<Tuple>& edges, std::vector<std::vector<Index>>& connected_regions);

/** \brief Function to extract edges from polylines
	*/
template<typename Index, typename Tuple>
void PolylinesToEdges(const std::vector<std::vector<Index>>& polylines, std::vector<Tuple>& edges);

/** \brief Remove duplicate edges (order does not matter)
	*/
template<typename Index, typename Tuple>
void RemoveDuplicateEdges(std::vector<Tuple>& edges);

template<typename TVec3>
void WritePolylinesToVtk(std::vector<std::vector<TVec3>>& polylines, const std::string& file_name);
} // namespace iseg

#include "PolyLines.inl"
