/// \file TopologyInvariants.h
///
/// \author Bryn Lloyd
/// \copyright 2020, IT'IS Foundation

#pragma once

#include <array>
#include <deque>

namespace topology {

template<typename T>
inline int CountIf(const T& v)
{
	static_assert(std::is_same<T, bool>::value, "CountIf argument must be a boolean.");
	return (v != 0) ? 1 : 0;
}

/** \brief True if Euler characteristic of voxel set does not change
*/
template<typename TNeighborhood, typename TLabel>
bool EulerInvariant(const TNeighborhood& neighbors, const TLabel label)
{
	// For a face-connected set, a vertex, edge, or facet is included
	// only if all voxels it is incident to are in the set, and for a
	// vertex-connected set it is included if any incident voxels are in the set

	// Here we count only the changes at the center voxel.
	// ==> If we mark the center as BG, then P, F, E and V are '0'.
	static const int c = 27 / 2;

	assert(neighbors[c] == label);

	// Voxels (parallelepipeds) - count center
	const int P = 1;

	// Faces - count faces around center voxel
	const int F = CountIf(neighbors[c - 1] == label)		// -x
								+ CountIf(neighbors[c + 1] == label)	// +x
								+ CountIf(neighbors[c - 3] == label)	// -y
								+ CountIf(neighbors[c + 3] == label)	// +y
								+ CountIf(neighbors[c - 9] == label)	// -z
								+ CountIf(neighbors[c + 9] == label); // +z

	// Edges - count edges around center voxel
	int E = 0;
	{
		static const short offsets[12][2] = {
				{-1, -3}, {+1, -3}, {-1, +3}, {+1, +3}, {-1, -9}, {+1, -9}, {-1, +9}, {+1, +9}, {-3, -9}, {+3, -9}, {-3, +9}, {+3, +9}};
		for (int i = 0; i < 12; i++)
		{
			short o = offsets[i][0];
			short u = offsets[i][1];

			E += CountIf(neighbors[c + o] == label &&
									 neighbors[c + u] == label &&
									 neighbors[c + o + u] == label);
		}
	}

	// Vertices - count verts around center voxel
	int V = 0;
	{
		static const short offsets[8][3] = {
				{-1, -3, -9}, {+1, -3, -9}, {-1, +3, -9}, {+1, +3, -9}, {-1, -3, +9}, {+1, -3, +9}, {-1, +3, +9}, {+1, +3, +9}};
		for (int i = 0; i < 8; i++)
		{
			short x = offsets[i][0];
			short y = offsets[i][1];
			short z = offsets[i][2];

			V += CountIf(neighbors[c + x] == label &&
									 neighbors[c + y] == label &&
									 neighbors[c + z] == label &&
									 neighbors[c + x + y] == label &&
									 neighbors[c + x + z] == label &&
									 neighbors[c + y + z] == label &&
									 neighbors[c + x + y + z] == label);
		}
	}

	return (V - E + F - P) == 0;
}

/** \brief Returns the number of connected components for selected label
*/
template<typename TNeighborhood, typename TLabel>
unsigned ConnectedComponents(const TNeighborhood& neighbors, const TLabel label)
{
	std::deque<unsigned> queue;
	std::array<bool, 27> visited;
	visited.fill(false);

	static const short ijk_lut[27][3] = {
			{0, 0, 0},
			{1, 0, 0},
			{2, 0, 0},
			{0, 1, 0},
			{1, 1, 0},
			{2, 1, 0},
			{0, 2, 0},
			{1, 2, 0},
			{2, 2, 0}, //
			{0, 0, 1},
			{1, 0, 1},
			{2, 0, 1},
			{0, 1, 1},
			{1, 1, 1},
			{2, 1, 1},
			{0, 2, 1},
			{1, 2, 1},
			{2, 2, 1}, //
			{0, 0, 2},
			{1, 0, 2},
			{2, 0, 2},
			{0, 1, 2},
			{1, 1, 2},
			{2, 1, 2},
			{0, 2, 2},
			{1, 2, 2},
			{2, 2, 2},
	};

	unsigned new_label = 0;
	for (unsigned n = 0; n < 27; ++n)
	{
		// skip other labels and visited voxels
		if (neighbors[n] != label || visited[n])
			continue;

		queue.push_back(n);
		visited[n] = true;
		new_label++;

		while (!queue.empty())
		{
			auto id = queue.front();
			queue.pop_front();
			auto ijk = ijk_lut[id];

			if (ijk[0] < 2 && neighbors[id] == neighbors[id + 1] && !visited[id + 1]) // +x
			{
				queue.push_back(id + 1);
				visited[id + 1] = true;
			}
			if (ijk[0] > 0 && neighbors[id] == neighbors[id - 1] && !visited[id - 1]) // -x
			{
				queue.push_back(id - 1);
				visited[id - 1] = true;
			}
			if (ijk[1] < 2 && neighbors[id] == neighbors[id + 3] && !visited[id + 3]) // +y
			{
				queue.push_back(id + 3);
				visited[id + 3] = true;
			}
			if (ijk[1] > 0 && neighbors[id] == neighbors[id - 3] && !visited[id - 3]) // -y
			{
				queue.push_back(id - 3);
				visited[id - 3] = true;
			}
			if (ijk[2] < 2 && neighbors[id] == neighbors[id + 9] && !visited[id + 9]) // +z
			{
				queue.push_back(id + 9);
				visited[id + 9] = true;
			}
			if (ijk[2] > 0 && neighbors[id] == neighbors[id - 9] && !visited[id - 9]) // -z
			{
				queue.push_back(id - 9);
				visited[id - 9] = true;
			}
		}
	}

	return new_label;
}

/** \brief True if number of connected components of voxel set does not change
*/
template<typename TNeighborhood, typename TLabel>
bool CCInvariant(TNeighborhood neighbors, const TLabel label)
{
	neighbors[27 / 2] = 1;
	unsigned cc_before = ConnectedComponents(neighbors, label);

	neighbors[27 / 2] = 0;
	unsigned cc_after = ConnectedComponents(neighbors, label);
	return (cc_before == cc_after);
}

/** \brief Will voxels be manifold after removing center voxel?
 */
template<typename TNeighborhood, typename TLabel>
bool NonmanifoldRemove(const TNeighborhood& neighbors, const TLabel label)
{
	static const int c = 27 / 2;

	// test for non-manifold edges around center voxel
	return (neighbors[c - 1] == label && neighbors[c - 3] == label && neighbors[c - 1 - 3] != label) || (neighbors[c + 1] == label && neighbors[c - 3] == label && neighbors[c + 1 - 3] != label) || (neighbors[c - 1] == label && neighbors[c + 3] == label && neighbors[c - 1 + 3] != label) || (neighbors[c + 1] == label && neighbors[c + 3] == label && neighbors[c + 1 + 3] != label)
				 //
				 || (neighbors[c - 1] == label && neighbors[c - 9] == label && neighbors[c - 1 - 9] != label) || (neighbors[c + 1] == label && neighbors[c - 9] == label && neighbors[c + 1 - 9] != label) || (neighbors[c - 1] == label && neighbors[c + 9] == label && neighbors[c - 1 + 9] != label) || (neighbors[c + 1] == label && neighbors[c + 9] == label && neighbors[c + 1 + 9] != label)
				 //
				 || (neighbors[c - 3] == label && neighbors[c - 9] == label && neighbors[c - 3 - 9] != label) || (neighbors[c + 3] == label && neighbors[c - 9] == label && neighbors[c + 3 - 9] != label) || (neighbors[c - 3] == label && neighbors[c + 9] == label && neighbors[c - 3 + 9] != label) || (neighbors[c + 3] == label && neighbors[c + 9] == label && neighbors[c + 3 + 9] != label);
}

} // namespace topology
