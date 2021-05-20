/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	double m_X[3];
	PointType() { m_X[0] = m_X[1] = m_X[2] = 0.0; }
	PointType(const PointType& rhs)
	{
		m_X[0] = rhs.m_X[0];
		m_X[1] = rhs.m_X[1];
		m_X[2] = rhs.m_X[2];
	}
	PointType& operator=(const PointType& rhs)
	{
		m_X[0] = rhs.m_X[0];
		m_X[1] = rhs.m_X[1];
		m_X[2] = rhs.m_X[2];
		return (*this);
	}
};

/**
	This class allows finding duplicate edges
	It implements comparison operators.
	*/
template<class TId>
struct SegmentType
{
	SegmentType(TId i1, TId i2)
	{
		if (i1 < i2)
		{
			m_N1 = i1;
			m_N2 = i2;
		}
		else
		{
			m_N1 = i2;
			m_N2 = i1;
		}
		assert(m_N1 < m_N2);
	}
	bool operator<(const SegmentType& rhs) const
	{
		if (m_N1 != rhs.m_N1)
			return (m_N1 < rhs.m_N1);
		else
			return (m_N2 < rhs.m_N2);
	}
	TId m_N1, m_N2;
};

/**
	This class allows finding duplicate triangles
	It implements comparison operators.
	*/
template<class TId>
struct TriangleType
{
	TriangleType(TId i, TId j, TId k)
	{
		m_N1 = i;
		m_N2 = j;
		m_N3 = k;
		if (m_N1 > m_N2)
			std::swap(m_N1, m_N2);
		if (m_N1 > m_N3)
			std::swap(m_N1, m_N3);
		if (m_N2 > m_N3)
			std::swap(m_N2, m_N3);
		assert(m_N1 < m_N2);
		assert(m_N1 < m_N3);
		assert(m_N2 < m_N3);
	}
	bool operator<(const TriangleType& rhs) const
	{
		if (m_N1 != rhs.m_N1)
			return (m_N1 < rhs.m_N1);
		else if (m_N2 != rhs.m_N2)
			return (m_N2 < rhs.m_N2);
		else
			return (m_N3 < rhs.m_N3);
	}
	TId m_N1, m_N2, m_N3;
};

/**
	This class is used to reorder elements
	*/
template<class TCompare, class TStore>
struct SortType
{
	SortType(TStore attachedData, TCompare valueToCompare)
			: m_Data(attachedData), m_Val(valueToCompare)
	{
	}
	bool operator<(const SortType& rhs) const { return (this->_val < rhs._val); }
	TStore m_Data;
	TCompare m_Val;
};

} // namespace Meshing

#endif
