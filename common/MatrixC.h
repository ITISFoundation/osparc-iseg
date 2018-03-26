/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */


#ifndef MATH_MATRIXC_H
#define MATH_MATRIXC_H

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <string>
#include <exception>
#include "Definitions.h"

namespace Math
{
typedef T value_type;
//			typedef size_t size_type;
typedef typename std::vector<T>::size_type size_type;
template <class T> class MatrixC
{
public:
	MatrixC(const size_type rows=0, const size_type cols=1)
	{
		NR = rows;
		NC = cols;
		if(rows>0 && cols>0)
			data.resize(rows*cols);
	}

	// typedef typename std::vector<T>::iterator& iterator;
	// typedef typename std::vector<T>::const_iterator& const_iterator;
	// typedef T& reference;
	// typedef const T& const_reference;
	// typedef size_t size_type;

	// iterator support
	// iterator begin() { return &*data.begin(); }
	// const_iterator begin() const { &*return data.begin(); }
	// iterator end() { return &*data.end(); }
	// const_iterator end() const { return &*data.end(); }

	void clear()
	{
		data.clear();
	}

	void copy(const MatrixC& rhs)
	{
		// data = std::vector<T>(rhs.begin(),rhs.end());
		resize(rhs.rows(), rhs.columns());
		for(size_type i=0; i<rhs.size(); i++)
			data[i]=rhs[i];
	};
	void copyMT(const MatrixC& rhs)
	{
#pragma omp single
		resize(rhs.rows(), rhs.columns());
#pragma omp for
		for(size_type i=0; i<rhs.size(); i++)
			data[i]=rhs[i];
	};

	size_type numberOfColumns()const{return NC;};
	size_type numberOfRows()const{return NR;};
	size_type columns()const{return NC;};
	size_type rows()const{return NR;};
	size_type size()const{return data.size();};
	bool empty()const{return !data.size();}

	const T& operator [] (const size_type i) const
	{
		assert(i>=0 && i<size());
		return data[i];
	};
	T& operator [] (const size_type i)
	{
		assert(i>=0 && i<size());
		return data[i];
	};

	// row-wise storage
	const T& operator () (const size_type r, const size_type c) const
	{
		assert(c>=1 && r>=1 && c<=NC && r<=NR);
		return data[(r-1)*NC + (c-1)];
	}
	T& operator () (const size_type r, const size_type c)
	{
		assert(c>=1 && r>=1 && c<=NC && r<=NR);
		return data[(r-1)*NC + (c-1)];
	}

	void assign(const T value)
	{
		std::fill(data.begin(), data.end(), value);
	}
	void assignMT(const T val)
	{
#pragma omp for
		for(size_type i=0; i<this->size(); ++i)
			data[i]=val;
	}

	void resize(const size_type newrows, const size_type newcols)
	{
		NR = newrows;
		NC = newcols;
		data.resize(newrows*newcols);
	}

	// FIXME
	// template <class Vector> void push_back_row(const Vector& v)
	// {
	// for(size_type i=0; i<v.size(); i++)
	// data.push_back();
	// }

private:
	std::vector<T> data;
	size_type NR, NC;
	MatrixC(const MatrixC&); // not implemented
	void operator=(const MatrixC&); // not implemented
};


}

#endif
