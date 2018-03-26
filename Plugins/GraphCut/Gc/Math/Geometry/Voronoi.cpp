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
    Voronoi diagram computation algorithms.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#include "../../System/ArgumentException.h"
#include "../../System/Collection/Pair.h"
#include "../../System/Algo/Sort/Heap.h"
#include "../Constant.h"
#include "Voronoi.h"
#include "ConvexHull.h"

namespace Gc
{
    namespace Math
    {
        namespace Geometry
        {
            using Algebra::Vector;

            /************************************************************************/

            /** For triangle specified by three points lying on a unit sphere calculates its
                circumcenter.
            */
            template <class T>
            static Vector<3,T> SphericalTriangleCircumcenter (const Vector<3,T> &a, 
                const Vector<3,T> &b, const Vector<3,T> &c)
            {
                Vector<3,T> v = CrossProduct(b-a,c-a).Normalized();
                return (a-v).Length() < (a+v).Length() ? v : -v;
            }

            /************************************************************************/

            /** Calculate spherical angle (at point a) specified by points a, b and c 
                lying on a unit sphere. 
            */
            template <class T>
            static T SphericalAngle (const Vector<3,T> &a, const Vector<3,T> &b, 
                const Vector<3,T> &c)
            {
                return CrossProduct(c-a,-a).Angle(CrossProduct(b-a,-a));
            }

            /************************************************************************/

            /** Calculate the area of a spherical triangle. */
            template <class T>
            static T SphericalTriangleArea (const Vector<3,T> &a, const Vector<3,T> &b, 
                const Vector<3,T> &c)
            { 
                return SphericalAngle(a,b,c) + 
                       SphericalAngle(b,a,c) + 
                       SphericalAngle(c,a,b) - T(Constant::Pi);
            }

            /************************************************************************/

            /** Among the unprocessed triangles find triangle containing edge a - b. */
            static Size FindNeighbourTriangle (System::Collection::Array<1,System::Collection::Tuple<3,Size> > &tl, 
                System::Collection::Array<1,int> &tem, Size i, Size a, Size b)
            {
                for (Size j = i + 1; j < tl.Elements(); j++)
                {
                    Size ta = tl[j][0], tb = tl[j][1], tc = tl[j][2];

                    if (!(tem[j] & 1))
                    {
                        if ((ta == a && tb == b) || (tb == a && ta == b))
                        {
                            tem[j] |= 1;
                            return j;
                        }
                    }
                    
                    if (!(tem[j] & 2))
                    {
                        if ((tb == a && tc == b) || (tc == a && tb == b))
                        {
                            tem[j] |= 2;
                            return j;
                        }
                    }

                    if (!(tem[j] & 4))
                    {
                        if ((ta == a && tc == b) || (tc == a && ta == b))
                        {
                            tem[j] |= 4;
                            return j;
                        }
                    }
                }

                return (Size)-1;
            }

            /************************************************************************/

            template <class T>
            void HypersphereVoronoiDiagram (const System::Collection::Array<1,Vector<3,T> > &points, 
                System::Collection::Array<1,T> &vd)
            {
                System::Collection::Array<1,System::Collection::Tuple<3,Size> > tl;
                ConvexHull (points, tl);

                System::Collection::Array<1,int> tem(tl.Elements(), 0);
                vd.Resize(points.Elements(), 0);

                for (Size i = 0; i < tl.Elements(); i++)
                {
                    if (tem[i] == 7)
                    {
                        continue;
                    }

                    Vector<3,T> c1, c2;
                    Size a = tl[i][0], b = tl[i][1], c = tl[i][2];

                    c1 = SphericalTriangleCircumcenter(points[a],points[b],points[c]);

                    if (!(tem[i] & 1)) // First triangle edge not processed
                    {
                        Size j = FindNeighbourTriangle(tl, tem, i, a, b);
                        c2 = SphericalTriangleCircumcenter(points[tl[j][0]],points[tl[j][1]],points[tl[j][2]]);
                        vd[a] += SphericalTriangleArea(points[a],c1,c2);
                        vd[b] += SphericalTriangleArea(points[b],c1,c2);
                        tem[i] |= 1;
                    }
                    if (!(tem[i] & 2)) // Second triangle edge not processed
                    {
                        Size j = FindNeighbourTriangle(tl, tem, i, b, c);
                        c2 = SphericalTriangleCircumcenter(points[tl[j][0]],points[tl[j][1]],points[tl[j][2]]);
                        vd[b] += SphericalTriangleArea(points[b],c1,c2);
                        vd[c] += SphericalTriangleArea(points[c],c1,c2);
                        tem[i] |= 2;
                    }
                    if (!(tem[i] & 4)) // Third triangle edge not processed
                    {
                        Size j = FindNeighbourTriangle(tl, tem, i, a, c);
                        c2 = SphericalTriangleCircumcenter(points[tl[j][0]],points[tl[j][1]],points[tl[j][2]]);
                        vd[a] += SphericalTriangleArea(points[a],c1,c2);
                        vd[c] += SphericalTriangleArea(points[c],c1,c2);
                        tem[i] |= 4;
                    }
                }
            }

            /************************************************************************/

            template <class T>
            struct OrientationPred
            {                
                bool operator()(const System::Collection::Pair<T,Size>& v1, 
                    const System::Collection::Pair<T,Size>& v2) const
                {
                    return (v1.m_first < v2.m_first);
                }
            };

            /************************************************************************/

            template <class T>
            void HypersphereVoronoiDiagram (const System::Collection::Array<1,Vector<2,T> > &points, 
                System::Collection::Array<1,T> &vd)
            {
                if (points.Elements() < 2)
                {
                    throw System::ArgumentException(__FUNCTION__, __LINE__, "Unable to calculate Voronoi diagram"
                        " for less than 2 points.");
                }

                // Sort vectors according to their angular orientation
                System::Collection::Array<1,System::Collection::Pair<T,Size> > p(points.Elements());
                
                for (Size i = 0; i < points.Elements(); i++)
                {
                    // Calculate the angular orientation of the vector and save its index
                    p[i].m_first = Math::Algebra::AngularOrientation(points[i]);
                    p[i].m_second = i;
                }

                // Sort angular orientations
                System::Algo::Sort::Heap(p.Begin(), p.End(), OrientationPred<T>());
                
                // Calculate Voronoi partitioning (average of neighbouring angular deltas)
                vd.Resize(points.Elements());
                Size li = points.Elements() - 1;

                // First point
                vd[p[0].m_second] = T(Constant::Pi) + (p[1].m_first - p[li].m_first) / 2;

                // Middle points                
                for (Size i = 1; i < li; i++)
                {
                    vd[p[i].m_second] = (p[i+1].m_first - p[i-1].m_first) / 2;
                }

                // Last point
                vd[p[li].m_second] = T(Constant::Pi) + (p[0].m_first - p[li-1].m_first) / 2;
            }

            /************************************************************************/

            // Explicit instantiations
            /** @cond */
            template GC_DLL_EXPORT void HypersphereVoronoiDiagram 
                (const System::Collection::Array<1,Vector<3,Float32> > &points, 
                System::Collection::Array<1,Float32> &vd);
            template GC_DLL_EXPORT void HypersphereVoronoiDiagram 
                (const System::Collection::Array<1,Vector<3,Float64> > &points, 
                System::Collection::Array<1,Float64> &vd);
            template GC_DLL_EXPORT void HypersphereVoronoiDiagram 
                (const System::Collection::Array<1,Vector<2,Float32> > &points, 
                System::Collection::Array<1,Float32> &vd);
            template GC_DLL_EXPORT void HypersphereVoronoiDiagram 
                (const System::Collection::Array<1,Vector<2,Float64> > &points, 
                System::Collection::Array<1,Float64> &vd);
            /** @endcond */
        }
    }
}
