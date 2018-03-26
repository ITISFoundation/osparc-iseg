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
    Convex hull computation algorithms.

    This file implements the Gift Wrap algorithm based on the implementation
    from http://www.cse.unsw.edu.au/~lambert/java/3d/.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#include "../Algebra/Hyperplane.h"
#include "../../System/Collection/Tuple.h"
#include "../../System/Collection/ArrayBuilder.h"
#include "ConvexHull.h"

namespace Gc
{
    namespace Math
    {
        namespace Geometry
        {
            using Algebra::Vector;
            using Algebra::Hyperplane;
            using Algebra::CrossProduct;

            /************************************************************************/

            typedef System::Collection::Tuple<2,Size> Edge;

            /************************************************************************/

            template <Size N, class T>
            static Size FindBottom2D (const System::Collection::Array<1,Vector<N,T> > &points)
            {
                Size bot = 0;

                for (Size i = 1; i < points.Elements(); i++)
                {
                    if (points[i][1] < points[bot][1])
                    {
                        bot = i;
                    }
                }

                return bot;
            }

            /************************************************************************/

            template <Size N, class T>
            static Size Search2D (const System::Collection::Array<1,Vector<N,T> > &p, Size a)
            {
                Size b = (a == 0) ? 1 : 0;
                Vector<N,T> hp_norm(0);

                // Normal vector of projection of the points to xy plane.
                hp_norm[0] = p[b][1] - p[a][1];
                hp_norm[1] = p[a][0] - p[b][0];
                Hyperplane<N,T> h(hp_norm, p[a]);

                for (Size i = b + 1; i < p.Elements(); i++)
                {
                    if (i != a && h.IsAbove(p[i]))
                    {
                        b = i;

                        hp_norm[0] = p[b][1] - p[a][1];
                        hp_norm[1] = p[a][0] - p[b][0];
                        h.Set(hp_norm, p[a]);
                    }
                }

                return b;
            }

            /************************************************************************/

            template <class T>
            static Size Search3D (const System::Collection::Array<1,Vector<3,T> > &p, Size a, Size b)
            {
                Size c;
                for (c = 0; c == a || c == b; c++)
                    ;

                Hyperplane<3,T> h(CrossProduct(p[b]-p[a],p[c]-p[a]), p[a]);

                for (Size i = c + 1; i < p.Elements(); i++) 
                {
                    if (i != a && i != b && h.IsAbove(p[i]))
                    {
                        c = i;
                        h.Set(CrossProduct(p[b]-p[a],p[c]-p[a]), p[a]);
                    }
                }

                return c;
            }

            /************************************************************************/

            static void PushEdge(System::Collection::ArrayBuilder<Edge> &el, Size a, Size b)
            {
                for (Size i = 0; i < el.Elements(); i++)
                {
                    if ((el[i][0] == a && el[i][1] == b) || 
                        (el[i][0] == b && el[i][1] == a))
                    {
                        el.Remove(i);
                        return;
                    }
                }

                el.Push(Edge(a,b));
            }

            /************************************************************************/

            template <class T>
            void ConvexHull (const System::Collection::Array<1,Vector<3,T> > &points, 
                System::Collection::Array<1,System::Collection::Tuple<3,Size> > &hull)
            {
                System::Collection::ArrayBuilder<Edge> es(50);
                System::Collection::ArrayBuilder<System::Collection::Tuple<3,Size> > hl(200);
                
                Size bot = FindBottom2D(points);
                Size bot2 = Search2D(points, bot);

                es.Push(Edge(bot, bot2));
                es.Push(Edge(bot2, bot));

                while (!es.IsEmpty())
                {        
                    // Pop edge
                    Edge e = es.Last();
                    es.Pop();

                    // Find third triangle point
                    Size c = Search3D(points, e[0], e[1]);
                    hl.Push(System::Collection::Tuple<3,Size>(e[0], e[1], c));

                    PushEdge(es, e[0], c);
                    PushEdge(es, c, e[1]);
                }

                hl.CopyToArray(hull);
            }

            /************************************************************************/

            template <class T>
            void ConvexHull (const System::Collection::Array<1,Vector<2,T> > &points, 
                System::Collection::Array<1,System::Collection::Tuple<2,Size> > &hull)
            {
                System::Collection::ArrayBuilder<System::Collection::Tuple<2,Size> > hl(50);

                Size bot = FindBottom2D(points);
                Size a = bot;

                do 
                {
                    Size b = Search2D(points, a);
                    hl.Push(System::Collection::Tuple<2,Size>(a,b));
                    a = b;
                } while (a != bot);

                hl.CopyToArray(hull);
            }

            /************************************************************************/

            // Explicit instantiations
            /** @cond */
            template GC_DLL_EXPORT void ConvexHull(const System::Collection::Array<1,Vector<3,Float32> > &points, 
                System::Collection::Array<1,System::Collection::Tuple<3,Size> > &hull);
            template GC_DLL_EXPORT void ConvexHull(const System::Collection::Array<1,Vector<3,Float64> > &points, 
                System::Collection::Array<1,System::Collection::Tuple<3,Size> > &hull);
            template GC_DLL_EXPORT void ConvexHull(const System::Collection::Array<1,Vector<2,Float32> > &points, 
                System::Collection::Array<1,System::Collection::Tuple<2,Size> > &hull);
            template GC_DLL_EXPORT void ConvexHull(const System::Collection::Array<1,Vector<2,Float64> > &points, 
                System::Collection::Array<1,System::Collection::Tuple<2,Size> > &hull);
            /** @endcond */
        }
    }
}
