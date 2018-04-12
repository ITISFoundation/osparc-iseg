/*
    This file is part of Graph Cut (Gc) combinatorial optimization library.
    Copyright (C) 2008-2010 Centre for Biomedical Image Analysis (CBIA)
    Copyright (C) 2008-2010 Ondrej Danek

    This library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published 
    by the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Gc is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with Graph Cut library. If not, see <http://www.gnu.org/licenses/>.
*/

/**
    @file
    Neighbourhood description class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_ENERGY_NEIGHBOURHOOD_H
#define GC_ENERGY_NEIGHBOURHOOD_H

#include "../Type.h"
#include "../Math/Algebra/Number.h"
#include "../Math/Algebra/Vector.h"
#include "../System/Collection/Array.h"
#include "../System/Collection/ArrayBuilder.h"
#include "../System/ArgumentException.h"

namespace Gc
{
    namespace Energy
    {
        /** %Neighbourhood description class. 

            This class holds list of vector offsets to neighbour elements.
        
            @tparam N Dimensionality of the space.
            @tparam T %Data type used to store the offsets.
        */
        template <Size N, class T>
        class Neighbourhood
            : public System::Collection::Array<1,Math::Algebra::Vector<N,T> >
        {
        public:
            using System::Collection::Array<1,Math::Algebra::Vector<N,T> >::m_data;

        public:
	        using System::Collection::Array<1,Math::Algebra::Vector<N,T> >::Elements;
	        using System::Collection::Array<1,Math::Algebra::Vector<N,T> >::Resize;

        public:
            /** Constructor. */
            Neighbourhood()
                : System::Collection::Array<1,Math::Algebra::Vector<N,T> >()
            {}

            /** Constructor. 
            
                Allocates space for \c sz number of neighbour offsets.
            */
            explicit Neighbourhood(Size sz)
                : System::Collection::Array<1,Math::Algebra::Vector<N,T> >(sz)
            {}

            /** Constructor. 
            
                Creates class and initializes it with one of the
                common neighbourhoods.

                @see Common().
            */
            Neighbourhood(Size sz, bool with_center)
                : System::Collection::Array<1,Math::Algebra::Vector<N,T> > ()
            {
                Common(sz, with_center);
            }

            /** Copy constructor. */
            Neighbourhood(const Neighbourhood<N,T> &n)
            {
                *this = n;
            }

            /** Assignment operator. */
            Neighbourhood<N,T>& operator= (const Neighbourhood<N,T> &n)
            {
                System::Collection::Array<1,Math::Algebra::Vector<N,T> >::operator=(n);
                return *this;
            }

            /** Convert %neighbourhood to another type or dimension. 
            
                If \c N2 is greater than \c N, then the neighbour offsets are truncated,
                otherwise they are padded with \c pad_val.
            */
            template <Size CN, class CT>
            Neighbourhood<CN,CT> Convert(CT pad_val) const
            {
                Neighbourhood<CN,CT> n;

                n.Resize(Elements());
                for (Size i = 0; i < Elements(); i++)
                {
                    n[i] = m_data[i].template Convert<CN,CT>(pad_val);
                }

                return n;
            }

            /** Split the neighbourhood into positive and negative halfs.
            
                @see Gc::Math::Algebra::Vector::Sgn()
            */
            void Split(Neighbourhood<N,T> &pos, Neighbourhood<N,T> &neg) const
            {
                System::Collection::ArrayBuilder<Math::Algebra::Vector<N,T> > abn(Elements()), abp(Elements());

                for (Size i = 0; i < Elements(); i++)
                {
                    Int8 sgn = m_data[i].Sgn();
                    if (sgn > 0)
                    {
                        abp.Push(m_data[i]);
                    }
                    else if (sgn < 0)
                    {
                        abn.Push(m_data[i]);
                    }
                }

                abp.CopyToArray(pos);
                abn.CopyToArray(neg);
            }

            /** Returns a new neighbourhood containing only the positive neighbours.
            
                @see Gc::Math::Algebra::Vector::Sgn().
            */
            Neighbourhood<N,T> Positive() const
            {
                System::Collection::ArrayBuilder<Math::Algebra::Vector<N,T> > ab(Elements());

                for (Size i = 0; i < Elements(); i++)
                {
                    if (m_data[i].Sgn() > 0)
                    {
                        ab.Push(m_data[i]);
                    }
                }

                Neighbourhood<N,T> n;   
                ab.CopyToArray(n);

                return n;
            }

            /** Returns a new neighbourhood containing only the negative neighbours.
            
                @see Gc::Math::Algebra::Vector::Sgn().
            */
            Neighbourhood<N,T> Negative() const
            {
                System::Collection::ArrayBuilder<Math::Algebra::Vector<N,T> > ab(Elements());

                for (Size i = 0; i < Elements(); i++)
                {
                    if (m_data[i].Sgn() < 0)
                    {
                        ab.Push(m_data[i]);
                    }
                }

                Neighbourhood<N,T> n;   
                ab.CopyToArray(n);

                return n;
            }

            /** Pointwise multiplication of all elements by given vector (scaling). */
            Neighbourhood<N,T> operator* (const Math::Algebra::Vector<N,T> &v) const
            {
                Neighbourhood<N,T> n;

                n.Resize(Elements());

                for (Size i = 0; i < Elements(); i++)
                {
                    n[i] = m_data[i] * v;
                }

                return n;
            }

            /** Pointwise multiplication of all elements by given vector (scaling). */
            Neighbourhood<N,T>& operator*= (const Math::Algebra::Vector<N,T> &v)
            {
                for (Size i = 0; i < Elements; i++)
                {
                    m_data[i] *= v;
                }

                return *this;
            }

            /** Pointwise division of all elements by given vector (scaling). */
            Neighbourhood<N,T> operator/ (const Math::Algebra::Vector<N,T> &v) const
            {
                Neighbourhood<N,T> n;

                n.Resize(Elements());

                for (Size i = 0; i < Elements(); i++)
                {
                    n[i] = m_data[i] / v;
                }

                return n;
            }

            /** Pointwise division of all elements by given vector (scaling). */
            Neighbourhood<N,T>& operator/= (const Math::Algebra::Vector<N,T> &v)
            {
                for (Size i = 0; i < Elements(); i++)
                {
                    m_data[i] /= v;
                }

                return *this;
            }

            /** Unary minus operator. */
            Neighbourhood<N,T> operator- () const
            {
                Neighbourhood<N,T> n;

                n.Resize(Elements());

                for (Size i = 0; i < Elements(); i++)
                {
                    n[i] = -m_data[i];
                }

                return n;
            }

            /** Generate one of the commonly used neighbourhoods. 
            
                @param[in] sz Size of the %neighbourhood. Currently the following neighbourhoods are
                    supported: 4, 8, 16, 32 in 2D and 6, 18, 26, 98 in 3D.
                @param[in] with_center If \c true zero vector is also included.
                */
            Neighbourhood<N,T>& Common(Size sz, bool with_center)
            {
                if (N == 2 && (sz == 4 || sz == 8 || sz == 16 || sz == 32))
                {
                    // 2D neighbourhoods                    
                    Size k = 0;
                    Resize(with_center ? (sz + 1) : sz);

                    if (with_center)
                    {
                        m_data[k++].Set(0, 0);
                    }

                    T pt[16] = { 1, 0, 3, 1, 2, 1, 3, 2, 1, 1, 2, 3, 1, 2, 1, 3 };
                    for (Size i = 0; i < 16; i += 2 * (32 / sz))
                    {
                        m_data[k++].Set(pt[i], pt[i + 1]);
                        m_data[k++].Set(-pt[i], -pt[i + 1]);
                        m_data[k++].Set(-pt[i + 1], pt[i]);
                        m_data[k++].Set(pt[i + 1], -pt[i]);
                    }
                }
                else if (N == 3 && (sz == 6 || sz == 18 || sz == 26 || sz == 98))
                {
                    // 3D neighbourhoods
                    if (sz == 98)
                    {
                        Box(Math::Algebra::Vector<N,Size>(2), true, with_center);
                    }
                    else
                    {
                        Size k = 0;
                        Resize(with_center ? (sz + 1) : sz);

                        if (with_center)
                        {
                            m_data[k++].Set(0, 0, 0);
                        }

                        m_data[k++].Set(1,0,0);
                        m_data[k++].Set(-1,0,0);
                        m_data[k++].Set(0,1,0);
                        m_data[k++].Set(0,-1,0);
                        m_data[k++].Set(0,0,1);
                        m_data[k++].Set(0,0,-1);

                        if (sz == 18 || sz == 26)
                        {
                            m_data[k++].Set(1,1,0);
                            m_data[k++].Set(-1,-1,0);
                            m_data[k++].Set(-1,1,0);
                            m_data[k++].Set(1,-1,0);

                            m_data[k++].Set(1,0,1);
                            m_data[k++].Set(-1,0,-1);
                            m_data[k++].Set(-1,0,1);
                            m_data[k++].Set(1,0,-1);

                            m_data[k++].Set(0,1,1);
                            m_data[k++].Set(0,-1,-1);
                            m_data[k++].Set(0,-1,1);
                            m_data[k++].Set(0,1,-1);
                        }

                        if (sz == 26)
                        {
                            m_data[k++].Set(1,1,1);
                            m_data[k++].Set(-1,-1,-1);

                            m_data[k++].Set(-1,1,1);
                            m_data[k++].Set(1,-1,-1);

                            m_data[k++].Set(1,-1,1);
                            m_data[k++].Set(-1,1,-1);

                            m_data[k++].Set(1,1,-1);
                            m_data[k++].Set(-1,-1,1);
                        }
                    }
                }
                else
                {
                    throw System::ArgumentException(__FUNCTION__, __LINE__, "Unexpected "
                        "neighbourhood size.");
                }

                return *this;
            }

            /** Generate N-dimensional box %neighbourhood.

                @param[in] sz In each dimension \c k, the %neighbourhood will contain elements
                    with coordinates between \c -sz[k] and \c sz[k].
                @param[in] unique Only neighbours with the greatest common divisor of their coordinates
                    equal to 1 will be added, i.e., the %neighbourhood will contain only unique directions.
                @param[in] with_center Zero (or central) neighbour will be included.

                @return The generated %neighbourhood.
            */
            Neighbourhood<N,T>& Box(const Math::Algebra::Vector<N,Size> &sz, 
                bool unique, bool with_center)
            {
                // Calculate the number of neighbours
                Size num = ((sz * 2) + Math::Algebra::Vector<N,Size>(1)).Product();

                // Array builder
                System::Collection::ArrayBuilder<Math::Algebra::Vector<N,T> > ab(num);

                // Generate neighbours
                Size n = 0, k, zi = with_center ? num : num / 2;
                Math::Algebra::Vector<N,Int32> ncur = -sz.template Convert<N,Int32>();

                while (n < num)
                {
                    if (n++ != zi) // This excludes zero neighbour if requested
                    {
                        if (!unique || ncur.Gcd() == 1)
                        {
                            ab.Push(ncur.template Convert<N,T>());
                        }
                    }
                             
                    k = 0;
                    while (k < N && ++ncur[k] > Int32(sz[k]))
                    {
                        ncur[k] = -Int32(sz[k]);
                        k++;
                    }
                }

                ab.CopyToArray(*this);

                return *this;
            }

            /** Create elementary neighbourhood.

                Elementary neighbourhood is neighbourhood containing only +1 and -1 neighbours in all dimensions.
            */
            Neighbourhood<N,T>& Elementary()
            {
                Resize(2 * N);

                Math::Algebra::Vector<N,Int32> v(0);

                for (Size i = 0; i < N; i++)
                {
                    v[i] = -1;
                    m_data[2*i] = v;
                    v[i] = 1;
                    m_data[2*i+1] = v;
                    v[i] = 0;
                }

                return *this;
            }

            /** Get extent of the %neighbourhood.
            
                Returns the largest positive and negative value in the %neighbourhood
                for each dimension. Useful for example for padding images.

                @param[out] left Largest negative value in each dimension.
                @param[out] right Largest positive value in each dimension.

                @see Gc::Aux::Image::Pad().
            */
            void Extent(Math::Algebra::Vector<N,Size> &left, 
                Math::Algebra::Vector<N,Size> &right) const
            {
                System::Algo::Range::Fill(left.Begin(), left.End(), 0);
                System::Algo::Range::Fill(right.Begin(), right.End(), 0);

                for (Size i = 0; i < Elements(); i++)
                {
                    for (Size j = 0; j < N; j++)
                    {
                        T v = m_data[i][j];

                        if (v < 0)
                        {
                            left[j] = Math::Max(left[j], Size(-v));
                        }
                        else
                        {
                            right[j] = Math::Max(right[j], Size(v));
                        }
                    }
                }
            }
        };
    }
}

#endif
