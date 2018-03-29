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
    Simple point inspection routines.

    @author Martin Maska <xmaska@fi.muni.cz>
    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include "SimplePoint.h"

namespace Gc
{
    namespace Algo
    {
        namespace Geometry
        {
            ///////////////////////////////////////////
            // 2D
            ///////////////////////////////////////////

            /******************************************************************************/

            static const Uint16 lut_2d_4b_8f[16] = { 53214, 52445, 52993, 52224, 52993, 13277, 
                52993, 13277, 1, 52445, 0, 52224, 52993, 13277, 52993, 13277};

            bool IsSimplePoint2D(Uint32 nb)
            {
                return ((lut_2d_4b_8f[(nb >> 4) & 15]) & (1 << (nb & 15))) != 0;
            }

            /******************************************************************************/

            ///////////////////////////////////////////
            // 3D
            ///////////////////////////////////////////

            /******************************************************************************/

            /** @cond */
            /* Auxiliary graph structure.

               @tparam M Number of vertices in the graph.
               @tparam D Maximum vertex degree.
            
               @author Martin Maska <xmaska@fi.muni.cz>
            */
            template <int M, int D> 
            class NbGraph
            {
            private:
                /* Vertex of given maximal degree. */
                struct Vertex
                {
                    /* Vertex degree. */
                    int degree;
                    /* Indices of adjacent vertices. */
                    int adj[D];

                    /* Constructor. */
                    Vertex() 
                        : degree(0) 
                    {};

                    /* Add adjacent vertex of given index. */
                    void Add(int index) 
                    { 
                        adj[degree] = index; 
                        ++degree; 
                    }

                    /* Remove all adjacent vertices. */
                    void Clear() 
                    { 
                        degree = 0; 
                    }
                };

                /* List of vertices. */
                Vertex m_vertices[M];
                /* List of flags determining whether a vertex has 
                    already been visited or not. */
                bool m_visited[M];
                /* Internal buffer for storing indices of accessible vertices. */
                int m_accessible[M];

            public:
                /* Constructor. */
                NbGraph() 
                {
                    for (int i = 0; i < M; ++i) 
                    { 
                        m_vertices[i].Clear(); 
                        m_visited[i] = false; 
                    }                 
                }

                /* Add an edge between given source and target vertices. */
                void Edge(int source, int target) 
                { 
                    m_vertices[source].Add(target); 
                }

                /* Verify whether the graph has given number of vertices 
                   accessible from the starting one. */
                bool AccessibleNeighbours(int start, int num)
                {
                    // begin breadth-first search algorithm from the starting vertex
                    m_visited[start] = true;
                    m_accessible[0] = start;

                    // the number of visited vertices in a given graph
                    int num_visited = 1;

                    // the index to the list of accessible vertices
                    int index = 0;

                    // execute breadth-first search algorithm
                    while (index < num_visited && num_visited < num)
                    {
                        // the first unprocessed accessible vertex
                        const Vertex &vertex = m_vertices[m_accessible[index]];

                        // go through its adjacent vertices
                        for (int i = 0; i < vertex.degree; ++i)
                        {
                            // the index of the i-th adjacent vertex
                            start = vertex.adj[i];

                            // check whether the i-th adjacent vertex has already been visited
                            if (!m_visited[start])
                            {
                                // add the i-th adjacent vertex to the list of accessible vertices
                                m_visited[start] = true;
                                m_accessible[num_visited] = start;
                                ++num_visited;
                            }
                        }

                        // move to the next accessible vertex
                        ++index;
                    }

                    return (num_visited == num);
                }
                
                /* Verify whether the graph has given number of 6-neighbouring 
                   vertices accessible from the starting one. */
                bool AccessibleSixNeighbours(int start, int num)
                {
                    // begin breadth-first search algorithm from the starting vertex
                    m_visited[start] = true;
                    m_accessible[0] = start;

                    // the number of visited vertices
                    int num_visited = 1;

                    // the index to the list of accessible vertices
                    int index = 0;

                    // decrement the number of 6-neighbouring vertices accessible from the starting one
                    --num;

                    // execute breadth-first search algorithm
                    while (index < num_visited && num > 0)
                    {
                        // the first unprocessed accessible vertex
                        const Vertex &vertex = m_vertices[m_accessible[index]];

                        // go through its adjacent vertices
                        for (int i = 0; i < vertex.degree; ++i)
                        {
                            // the index of the i-th adjacent vertex
                            start = vertex.adj[i];

                            // check whether the i-th adjacent vertex has already been visited
                            if (!m_visited[start])
                            {
                                // add the i-th adjacent vertex to the list of accessible vertices
                                m_visited[start] = true;
                                m_accessible[num_visited] = start;
                                ++num_visited;

                                // check whether the i-th adjacent vertex is a 6-neighbour
                                if (start == 2 || start == 6 || start == 8 || start == 9 || 
                                    start == 11 || start == 15)
                                {
                                    --num;
                                }
                            }
                        }

                        // move to the next accessible vertex
                        ++index;
                    }

                    return (num == 0);
                }
            };
            /** @endcond */

            /******************************************************************************/

            // Auxiliary macros
            /** @cond */
	        #define ADD_2(graph,a,b,val) graph.Edge(a,val); graph.Edge(b,val)
	        #define ADD_4(graph, a,b,c,d,val) ADD_2(graph,a,b,val); ADD_2(graph,c,d,val)
	        #define ADD_6(graph,a,b,c,d,e,f,val) ADD_4(graph,a,b,c,d,val); ADD_2(graph,e,f,val)
	        #define ADD_10(graph,a,b,c,d,e,f,g,h,i,j,val) ADD_6(graph,a,b,c,d,e,f,val); ADD_4(graph,g,h,i,j,val)
	        #define ADD_16(graph,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,val) ADD_10(graph,a,b,c,d,e,f,g,h,i,j,val); ADD_6(graph,k,l,m,n,o,p,val)
            #define NBSET(nb,i) ((nb & (1 << i)) != 0)
            /** @endcond */

            /******************************************************************************/

            static bool VerifyBackground(Uint32 nb)
            {
                NbGraph<18,4> bg_graph;

		        // index of the starting vertex
		        int start = -1;

		        // number of 6-neighbouring vertices
		        int num = 0;

		        // sum of indices of 6-neighbouring vertices
		        int sum = 0;

		        if (NBSET(nb,1)) { ADD_2(bg_graph,2,6,0); }
                if (NBSET(nb,3)) { ADD_2(bg_graph,2,8,1); }
                if (NBSET(nb,4)) { sum += 2; ++num; start = 2; ADD_4(bg_graph,0,1,3,4,2); }
                if (NBSET(nb,5)) { ADD_2(bg_graph,2,9,3); }
                if (NBSET(nb,7)) { ADD_2(bg_graph,2,11,4); }
                if (NBSET(nb,9)) { ADD_2(bg_graph,6,8,5); }
                if (NBSET(nb,10)) { sum += 6; ++num; start = 6; ADD_4(bg_graph,0,5,7,13,6); }
                if (NBSET(nb,11)) { ADD_2(bg_graph,6,9,7); }
                if (NBSET(nb,12)) { sum += 8; ++num; start = 8; ADD_4(bg_graph,1,5,10,14,8); }
                if (NBSET(nb,13)) { sum += 9; ++num; start = 9; ADD_4(bg_graph,3,7,12,16,9); }
                if (NBSET(nb,14)) { ADD_2(bg_graph,8,11,10); }
                if (NBSET(nb,15)) { sum += 11; ++num; start = 11; ADD_4(bg_graph,4,10,12,17,11); }
                if (NBSET(nb,16)) { ADD_2(bg_graph,9,11,12); }
                if (NBSET(nb,18)) { ADD_2(bg_graph,6,15,13); }
                if (NBSET(nb,20)) { ADD_2(bg_graph,8,15,14); }
                if (NBSET(nb,21)) { sum += 15; ++num; start = 15; ADD_4(bg_graph,13,14,16,17,15); }
                if (NBSET(nb,22)) { ADD_2(bg_graph,9,15,16); }
                if (NBSET(nb,24)) { ADD_2(bg_graph,11,15,17); }

                if (num == 0)
                {
                    // the situation number one - no 6-neighbouring vertex
                    return false;
                }
                else if (num == 1)
                {
                    // the situation number two - only one 6-neighbouring vertex
                    return true;
                }
                else if (num == 2)
                {
                    // the situation number three - two 6-neighbouring vertices
                    switch (sum)
                    {
                        // the first case - two opposite 6-neighbouring vertices
                    case 17: return false;
                        // the second case = two 6-neighbouring vertices that share one adjacent 18-neighbour
                    case  8: return NBSET(nb,1);
                    case 10: return NBSET(nb,3);
                    case 11: return NBSET(nb,5);
                    case 13: return NBSET(nb,7);
                    case 14: return NBSET(nb,9);
                    case 15: return NBSET(nb,11);
                    case 19: return NBSET(nb,14);
                    case 20: return NBSET(nb,16);
                    case 21: return NBSET(nb,18);
                    case 23: return NBSET(nb,20);
                    case 24: return NBSET(nb,22);
                    case 26: return NBSET(nb,24);
                    }
                }

                return bg_graph.AccessibleSixNeighbours(start, num);;
            }

            /******************************************************************************/

            static bool VerifyForeground(Uint32 nb)
            {
                NbGraph<26,16> fg_graph;

                // index of the starting vertex
                int start = -1;

                // the number of vertices
                int num = 0;

                if (!NBSET(nb,0)) { ++num; start = 0; ADD_6(fg_graph,1,3,4,9,10,12,0); };
                if (!NBSET(nb,1)) { ++num; start = 1; ADD_10(fg_graph,0,2,3,4,5,9,10,11,12,13,1); };
                if (!NBSET(nb,2)) { ++num; start = 2; ADD_6(fg_graph,1,4,5,10,11,13,2); };
                if (!NBSET(nb,3)) { ++num; start = 3; ADD_10(fg_graph,0,1,4,6,7,9,10,12,14,15,3); };
                if (!NBSET(nb,4)) { ++num; start = 4; ADD_16(fg_graph,0,1,2,3,5,6,7,8,9,10,11,12,13,14,15,16,4); };
                if (!NBSET(nb,5)) { ++num; start = 5; ADD_10(fg_graph,1,2,4,7,8,10,11,13,15,16,5); };
                if (!NBSET(nb,6)) { ++num; start = 6; ADD_6(fg_graph,3,4,7,12,14,15,6); };
                if (!NBSET(nb,7)) { ++num; start = 7; ADD_10(fg_graph,3,4,5,6,8,12,13,14,15,16,7); };
                if (!NBSET(nb,8)) { ++num; start = 8; ADD_6(fg_graph,4,5,7,13,15,16,8); };

                if (!NBSET(nb,9))  { ++num; start =  9; ADD_10(fg_graph,0,1,3,4,10,12,17,18,20,21,9); };
                if (!NBSET(nb,10)) { ++num; start = 10; ADD_16(fg_graph,0,1,2,3,4,5,9,11,12,13,17,18,19,20,21,22,10); };
                if (!NBSET(nb,11)) { ++num; start = 11; ADD_10(fg_graph,1,2,4,5,10,13,18,19,21,22,11); };
                if (!NBSET(nb,12)) { ++num; start = 12; ADD_16(fg_graph,0,1,3,4,6,7,9,10,14,15,17,18,20,21,23,24,12); };
                if (!NBSET(nb,13)) { ++num; start = 13; ADD_16(fg_graph,1,2,4,5,7,8,10,11,15,16,18,19,21,22,24,25,13); };
                if (!NBSET(nb,14)) { ++num; start = 14; ADD_10(fg_graph,3,4,6,7,12,15,20,21,23,24,14); };
                if (!NBSET(nb,15)) { ++num; start = 15; ADD_16(fg_graph,3,4,5,6,7,8,12,13,14,16,20,21,22,23,24,25,15); };
                if (!NBSET(nb,16)) { ++num; start = 16; ADD_10(fg_graph,4,5,7,8,13,15,21,22,24,25,16); };

                if (!NBSET(nb,17)) { ++num; start = 17; ADD_6(fg_graph,9,10,12,18,20,21,17); };
                if (!NBSET(nb,18)) { ++num; start = 18; ADD_10(fg_graph,9,10,11,12,13,17,19,20,21,22,18); };
                if (!NBSET(nb,19)) { ++num; start = 19; ADD_6(fg_graph,10,11,13,18,21,22,19); };
                if (!NBSET(nb,20)) { ++num; start = 20; ADD_10(fg_graph,9,10,12,14,15,17,18,21,23,24,20); };
                if (!NBSET(nb,21)) { ++num; start = 21; ADD_16(fg_graph,9,10,11,12,13,14,15,16,17,18,19,20,22,23,24,25,21); };
                if (!NBSET(nb,22)) { ++num; start = 22; ADD_10(fg_graph,10,11,13,15,16,18,19,21,24,25,22); };
                if (!NBSET(nb,23)) { ++num; start = 23; ADD_6(fg_graph,12,14,15,20,21,24,23); };
                if (!NBSET(nb,24)) { ++num; start = 24; ADD_10(fg_graph,12,13,14,15,16,20,21,22,23,25,24); };
                if (!NBSET(nb,25)) { ++num; start = 25; ADD_6(fg_graph,13,15,16,21,22,24,25); };

                return (num != 0 && fg_graph.AccessibleNeighbours(start, num));
            }

            /******************************************************************************/

            bool IsSimplePoint3D(Uint32 nb)
            {
                if (!VerifyBackground(nb))
                {
                    return false;
                }

                return VerifyForeground(nb);
            }

            /******************************************************************************/
        }
    }
}
