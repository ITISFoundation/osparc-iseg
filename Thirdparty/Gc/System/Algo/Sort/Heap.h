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
    Heap sort algorithm.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_SYSTEM_ALGO_SORT_HEAP_H
#define GC_SYSTEM_ALGO_SORT_HEAP_H

#include "../../../Type.h"
#include "../Iterator.h"
#include "../Predicate.h"

namespace Gc
{
    namespace System
    {
        namespace Algo
        {
            /** %Data sorting algorithms. */
            namespace Sort
            {
                /** Heap sort algorithm. 
                
                    Sorts elements from the smallest to the largest using heap sort
                    algorithm.

                    @param[in] begin Random access iterator pointing to the first element.
                    @param[in] end Random access iterator pointing to the end of the sequence.
                    @param[in] pred Comparison predicate.
                */
                template <class RanIter, class Pred>
                void Heap(RanIter begin, RanIter end, Pred pred)
                {
                    Size n = (end - begin), parent = n/2, index, child;
                    typename IteratorTraits<RanIter>::ValueType temp; 

                    while (n > 1)
                    { 
                        if (parent) 
                        { 
                            temp = begin[--parent];
                        } 
                        else 
                        {            
                            n--;
                            temp = begin[n];
                            begin[n] = begin[0];
                        }

                        index = parent;
                        child = index * 2 + 1;
                        while (child < n) 
                        {
                            if (child + 1 < n && pred(begin[child], begin[child + 1])) 
                            {
                                child++;
                            }

                            if (pred(temp, begin[child])) 
                            {
                                begin[index] = begin[child];
                                index = child;
                                child = index * 2 + 1;
                            } 
                            else 
                            {
                                break;
                            }
                        }

                        begin[index] = temp; 
                    }
                }

                /** Heap sort algorithm. 
                
                    Sorts elements from the smallest to the largest using heap sort
                    algorithm and System::Algo::Predicate::Less.

                    @param[in] begin Random access iterator pointing to the first element.
                    @param[in] end Random access iterator pointing to the end of the sequence.
                */
                template <class RanIter>
                void Heap(RanIter begin, RanIter end)
                {                    
                    Heap(begin, end, Predicate::Less<typename IteratorTraits<RanIter>::ValueType>());
                }
            }
        }
    }
}

#endif
