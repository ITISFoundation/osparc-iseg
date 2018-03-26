/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
*/

#ifndef MATH_MATRIX_H
#define MATH_MATRIX_H

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
template <class T> class Matrix
{
public:
	typedef T value_type;
	typedef typename std::vector<T>::size_type size_type;
	Matrix(const size_type rows=0, const size_type cols=1)
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
		this->NR = 0;
		this->NC = 0;
	}

	void copy(const Matrix& rhs)
	{
		// data = std::vector<T>(rhs.begin(),rhs.end());
		resize(rhs.rows(), rhs.columns());
		for(size_type i=0; i<rhs.size(); i++)
			data[i]=rhs[i];
	};
	void copyMT(const Matrix& rhs)
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

	// pointer
	T* pointer(const size_type i=0)
	{
		assert(i>=0 && i<size());
		return &data[i];
	};

	// column-wise storage
	const T& operator () (const size_type r, const size_type c) const
	{
		assert(c>=1 && r>=1 && c<=NC && r<=NR);
		return data[(c-1)*NR + (r-1)];
	}
	T& operator () (const size_type r, const size_type c)
	{
		assert(c>=1 && r>=1 && c<=NC && r<=NR);
		return data[(c-1)*NR + (r-1)];
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

	void resize_preserve(const size_type newrows, const size_type newcols)
	{
		Matrix<T> m1(newrows,newcols);
		for(size_type j=1; j<=std::min(this->rows(),newrows); j++)
			for(size_type i=1; i<=std::min(this->columns(),newcols); i++)
				m1(j,i) = (*this)(j,i);
		this->resize(newrows,newcols);
		for(size_type i=0; i<this->size(); i++)
			data[i] = m1[i];
	}

	// FIXME
	// template <class Vector> void push_back_row(const Vector& v)
	// {
	// for(size_type i=0; i<v.size(); i++)
	// data.push_back();
	// }

	friend std::ostream& operator << (std::ostream& os, const Matrix& m)
	{
		for(int i=1; i<=m.rows(); ++i)
		{
			for(int j=1; j<=m.columns(); ++j)
			{
				os << " " << m(i,j);
			}
			os << std::endl;
		}

	  return os;
	}

private:
	std::vector<T> data;
	size_type NR, NC;
	Matrix(const Matrix&); // not implemented
	void operator=(const Matrix&); // not implemented
};


}

#endif
