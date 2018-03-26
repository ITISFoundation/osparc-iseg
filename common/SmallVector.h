/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef MATH_SMALL_VECTOR_H
#define MATH_SMALL_VECTOR_H

#include <cstdarg>

#include <cstdio>
#include <cmath>
#include <cassert>
#include <iostream>
#include <vector>

#include "Definitions.h"

namespace Math
{

	template <typename T, size_t N> class SmallVector
	{
		public:
			typedef T value_type;
			typedef T* iterator;
			typedef const T* const_iterator;
			typedef T& reference;
			typedef const T& const_reference;
			typedef size_t size_type;

			// iterator support
			iterator begin() { return data; }
			const_iterator begin() const { return data; }
			iterator end() { return data+N; }
			const_iterator end() const { return data+N; }

			// size is constant
			size_type size() const { return N; }
			bool empty() const { return false; }
			size_type max_size() const { return N; }

			// assign one value to all elements
			void assign(const T& value)
			{
				std::fill(begin(), end(), value);
			}

		// 0-based (C style)
			const T& operator [] (const int i) const
			{
				assert(i>=0 && i<N);
				return data[i];
			};
			T& operator [] (const int i)
			{
				assert(i>=0 && i<N);
				return data[i];
			};

		// 1-based (FORTRAN style)
			const T& operator () (const int i) const
			{
				assert(i>0 && i<=N);
				return data[i-1];
			};
			T& operator () (const int i)
			{
				assert(i>0 && i<=N);
				return data[i-1];
			};
			
			friend std::ostream& operator << (std::ostream& os, const SmallVector& v)
			{
				for(int i=0; i<v.size(); i++)
					os << v.data[i] << " ";
				return os;
			}
			friend std::istream& operator >> (std::istream& is, SmallVector& v)
			{
				for(int i=0; i<v.size(); i++)
					is >> v.data[i];
				return is;
			}

			T norm2()
			{
				T n2 = 0.0;
				for(int i=0; i<N; i++)
					n2 += data[i]*data[i];
				return n2;
			}

			T norm()
			{
				T n2 = norm2();
				//n2 = std::max(0.0, n2);//accuracy problems?
				return std::sqrt(n2);
			}

			void normalize()
			{
				const value_type _norm = norm();
				for(int i=0; i<N; i++)
					data[i] /= _norm;
			}

			void cross(const SmallVector& A, const SmallVector& B)
			{
				assert(N==3 && A.size()==3 && B.size()==3);
				data[0] = A(2)*B(3)-A(3)*B(2);
				data[1] = A(3)*B(1)-A(1)*B(3);
				data[2] = A(1)*B(2)-A(2)*B(1);
			}

			void scale(const T s)
			{
				for(int i=0; i<N; i++)
					data[i] *= s;
			}

		private:
			T data[N];
// 			SmallVector(const SmallVector&); // not implemented
// 			void operator=(const SmallVector&); // not implemented
	};

}

#endif
