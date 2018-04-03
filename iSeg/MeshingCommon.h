/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef __MeshingCommon_H_
#define __MeshingCommon_H_

#include <algorithm>
#include <cassert>

namespace Meshing {
/**
	This class represents a point, which is copyable, and
	stores it's data as c-style array.
	*/
struct PointType
{
	double x[3];
	PointType() { x[0] = x[1] = x[2] = 0.0; }
	PointType(const PointType &rhs)
	{
		x[0] = rhs.x[0];
		x[1] = rhs.x[1];
		x[2] = rhs.x[2];
	}
	PointType &operator=(const PointType &rhs)
	{
		x[0] = rhs.x[0];
		x[1] = rhs.x[1];
		x[2] = rhs.x[2];
		return (*this);
	}
};

/**
	This class allows finding duplicate edges
	It implements comparison operators.
	*/
template<class TId> struct SegmentType
{
	SegmentType(TId i1, TId i2)
	{
		if (i1 < i2)
		{
			n1 = i1;
			n2 = i2;
		}
		else
		{
			n1 = i2;
			n2 = i1;
		}
		assert(n1 < n2);
	}
	bool operator<(const SegmentType &rhs) const
	{
		if (n1 != rhs.n1)
			return (n1 < rhs.n1);
		else
			return (n2 < rhs.n2);
	}
	TId n1, n2;
};

/**
	This class allows finding duplicate triangles
	It implements comparison operators.
	*/
template<class TId> struct TriangleType
{
	TriangleType(TId i, TId j, TId k)
	{
		n1 = i;
		n2 = j;
		n3 = k;
		if (n1 > n2)
			std::swap(n1, n2);
		if (n1 > n3)
			std::swap(n1, n3);
		if (n2 > n3)
			std::swap(n2, n3);
		assert(n1 < n2);
		assert(n1 < n3);
		assert(n2 < n3);
	}
	bool operator<(const TriangleType &rhs) const
	{
		if (n1 != rhs.n1)
			return (n1 < rhs.n1);
		else if (n2 != rhs.n2)
			return (n2 < rhs.n2);
		else
			return (n3 < rhs.n3);
	}
	TId n1, n2, n3;
};

/**
	This class is used to reorder elements
	*/
template<class TCompare, class TStore> struct SortType
{
	SortType(TStore attachedData, TCompare valueToCompare)
			: _data(attachedData), _val(valueToCompare)
	{
	}
	bool operator<(const SortType &rhs) const { return (this->_val < rhs._val); }
	TStore _data;
	TCompare _val;
};

} // namespace Meshing

#endif
