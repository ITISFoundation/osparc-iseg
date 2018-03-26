/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef MATH_VECTOR_H
#define MATH_VECTOR_H

#include <cstdarg>

#include <cstdio>
#include <cmath>
#include <cassert>
#include <iostream>
#include <vector>

#include "Definitions.h"

namespace Math
{

template <class T> class Vector
{
public:
	// iterator support
	typedef typename std::vector<T>::iterator iterator;
	typedef typename std::vector<T>::const_iterator const_iterator;
	iterator begin() { return _data.begin(); }
	const_iterator begin() const { return _data.begin(); }
	iterator end() { return _data.end(); }
	const_iterator end() const { return _data.end(); }
	typedef typename std::vector<T>::size_type size_type;
	//		typedef size_t size_type;
	// iterator begin() { return data; }
	// const_iterator begin() const { return data; }
	// iterator end() { return data+N; }
	// const_iterator end() const { return data+N; }

	// default constructor
	Vector(size_type N=0)
	{
		if(N>0) _data.resize(N);
	}

	void clear()
	{
		_data.clear();
	}

	// copy in values
	void copy(const Vector& rhs)
	{
		_data = std::vector<T>(rhs.begin(),rhs.end());
	}
	// copy in values
	void copyMT(const Vector& rhs)
	{
		_data.resize(rhs.size());
#pragma omp for
		for(size_type i=0; i<this->size(); i++)
			_data[i]=rhs[i];
	}

	T* data() { return _data.data(); }
	const T* data() const { return _data.data(); }

	const T& operator [] (const size_type i) const
	{
		assert(i>=0 && i<size());
		return _data[i];
	};

	T& operator [] (const size_type i)
	{
		assert(i>=0 && i<size());
		return _data[i];
	};

	// 1-based (FORTRAN style)
	const T& operator () (const size_type i) const
	{
		assert(i>0 && i<=size());
		return _data[i-1];
	};

	T& operator () (const int i)
	{
		assert(i>0 && i<=size());
		return _data[i-1];
	};

	void assign(const T value)
	{
		std::fill(_data.begin(), _data.end(), value);
	}

	void assignMT(const T val)
	{
		// 			assert(size()>0);
#pragma omp for
		for(size_type i=0; i<this->size(); ++i)
			_data[i]=val;
	}

	void resize(const size_type newsize)
	{
		_data.resize(newsize);
	}

	void push_back(const T& val)
	{
		_data.push_back(val);
	}

	size_type size()const
	{
		return _data.size();
	}

	bool empty()const{return !_data.size();}

	// TESTME
	//T max() const
	//{
	//	assert(!data.empty());
	//	T val = data[0];
	//	for(size_type i=1; i<this->size(); i++)
	//		val = max(val,data[i]);
	//	return val;
	//}

private:
	std::vector<T> _data;
	Vector<T>(const Vector<T>&); // not implemented
	void operator =(const Vector<T>&); // not implemented
};

}

#endif
