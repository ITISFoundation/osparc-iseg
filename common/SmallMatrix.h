/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

#ifndef MATH_SMALL_MATRIX_H
#define MATH_SMALL_MATRIX_H

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <string>
#include <exception>
#include "Definitions.h"
#include "SmallVector.h"

namespace Math
{
	template <typename T, size_t NR, size_t NC> class SmallMatrix
	{
		public:
			// this assumes column-wise ordering
			void outerProduct(const double* u, const double* v)
			{
				T* iter = &data[0];
				for(int c=0; c<NC; ++c)
				{
					for(int r=0; r<NR; ++r)
					{
						*iter = u[r] * v[c];
						++iter;
					}
				}
				assert(iter == &data[0]+NR*NC);
			}

			// this assumes column-wise ordering
			void addScaledOuterProduct(const double* u, const double* v, const double factor=1.0)
			{
				T* iter = &data[0];
				for(int c=0; c<NC; ++c)
				{
					for(int r=0; r<NR; ++r)
					{
						*iter += factor * u[r]*v[c];
						++iter;
					}
				}
				assert(iter == &data[0]+NR*NC);
			}
/*
			void matrixProduct(const SmallMatrix& A, const SmallMatrix& B)
			{
				assert(A.rows()==B.columns());
				assert(A.columns()==B.rows());
				assert(NR==A.rows());
				assert(NC==B.columns());

				T* iter = &data[0];
				for(int c=0; c<NC; ++c)
				{
					for(int r=0; r<NR; ++r)
					{
						for(int n=1; n<=A.columns(); n++)
						{
							*iter = factor * A(r,n)*B(n,c);
							++iter;
						}
					}
				}
				assert(iter == &data[0]+NR*NC);
			}

			template <typename SmallVector> void matrixProduct(const SmallMatrix& A, const SmallVector& B)
			{
				assert(A.columns()==B.size());
				assert(NC==B.columns());

				T* iter = &data[0];
				for(int c=0; c<NC; ++c)
				{
					for(int r=0; r<NR; ++r)
					{
						for(int n=1; n<=A.columns(); n++)
						{
							*iter = factor * A(r,n)*B(n);
							++iter;
						}
					}
				}
				assert(iter == &data[0]+NR*NC);
			}
			void addScaledMatrixProduct(const SmallMatrix& A, const SmallMatrix& B, const double factor=1.0)
			{
				assert(A.rows()==B.columns());
				assert(A.columns()==B.rows());
				assert(NR==A.rows());
				assert(NC==B.columns());

				T* iter = &data[0];
				for(int c=0; c<NC; ++c)
				{
					for(int r=0; r<NR; ++r)
					{
						for(int n=1; n<=A.columns(); n++)
						{
							*iter += factor * A(r,n)*B(n,c);
							++iter;
						}
					}
				}
				assert(iter == &data[0]+NR*NC);
			}
*/

//			template <typename SmallVector> void matrixProduct(const SmallMatrix& A, const SmallVector& B)
//			{
//				assert(A.columns()==B.size());
//				assert(NC==B.columns());
//
//				T* iter = &data[0];
//				for(int c=0; c<NC; ++c)
//				{
//					for(int r=0; r<NR; ++r)
//					{
//						for(int n=1; n<=A.columns(); n++)
//						{
//							*iter += factor * A(r,n)*B(n);
//							++iter;
//						}
//					}
//				}
//				assert(iter == &data[0]+NR*NC);
//			}

			void addScaled(const SmallMatrix& M, const double factor=1.0)
			{
				assert(M.rows()==NR);
				assert(M.columns()==NC);
				for(int i=0; i<data.size(); ++i)
				{
					data[i] += factor * M[i];
				}
			}

			template <typename SmallMatrix> void insert(const SmallMatrix& M1, const int posrow, const int poscol)
			{
				assert(M1.rows()<=NR);
				assert(M1.columns()<=NC);
				assert(posrow>=1);
				assert(poscol>=1);
				assert(posrow<=NR);
				assert(poscol<=NC);
				for(int c1=0; c1<M1.columns(); ++c1)
				{
					const int c = c1+poscol;
					for(int r1=0; r1<M1.rows(); ++r1)
					{
						const int r = r1+posrow;
						(*this)(r,c) = M1(r1+1,c1+1);
					}
				}
			}

			template <typename SmallMatrix> void transposeTo(SmallMatrix& M)
			{
				assert(M.rows()==NC);
				assert(M.columns()==NR);
				for(int c=1; c<=NC; ++c)
				{
					for(int r=1; r<=NR; ++r)
					{
						M(c,r) = (*this)(r,c);
					}
				}
			}

			int numberOfColumns()const{return NC;};
			int numberOfRows()const{return NR;};
			int columns()const{return NC;};
			int rows()const{return NR;};
			int size()const{return data.size();};

			// assign one value to all elements
			void assign(const T& value)
			{
				data.assign(value);
			}

			T* getPointer(const int i=0)
			{
				assert(i>=0 && i<data.size());
				return &data[i];
			}

			const T& operator [] (const int i) const
			{
				assert(i>=0 && i<data.size());
				return data[i];
			}
			T& operator [] (const int i)
			{
				assert(i>=0 && i<data.size());
				return data[i];
			}

			// column-wise storage, 1-based
			const T& operator () (const int r, const int c) const
			{
				assert(c>=1 && r>=1 && c<=NC && r<=NR);
				return data[(c-1)*NR+(r-1)];
			}
			T& operator () (const int r, const int c)
			{
				assert(c>=1 && r>=1 && c<=NC && r<=NR);
				return data[(c-1)*NR+(r-1)];
			}

			friend std::ostream& operator << (std::ostream& os, const SmallMatrix& x)
			{
				for(int i=1; i<=x.rows(); i++)
				{
					for(int j=1; j<=x.columns(); j++)
					{
						os << x(i,j) << " ";
					}
					if(i<x.numberOfRows()) os << std::endl;
				}
				return os;
			}
			// row-wise
			friend std::istream& operator >> (std::istream& is, SmallMatrix& x)
			{
				for(int i=1; i<=x.rows(); i++)
				{
					for(int j=1; j<=x.columns(); j++)
					{
						is >> x(i,j);
					}
				}
				return is;
			}

		private:
			SmallVector<T,NR*NC> data;
// 			SmallMatrix(const SmallMatrix&); // not implemented
// 			void operator=(const SmallMatrix&); // not implemented
	};

}

#endif
