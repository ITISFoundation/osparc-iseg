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
    Algorithms and functions that execute on a range of elements.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2011
*/

#ifndef GC_SYSTEM_ALGO_RANGE_H
#define GC_SYSTEM_ALGO_RANGE_H

namespace Gc
{
    namespace System
    {
        namespace Algo
        {
            /** Algorithms and functions that execute on a range of elements. 
        
                @see For math algorithms see also Saf::Math::Range.
            */
            namespace Range
            {
                /** Set all elements to a given value. */
                template <class FwdIter, class Val>
                inline void Fill(FwdIter begin, FwdIter end, const Val& val)
                {
                    while (begin != end)
                    {
                        *begin = val;
                        ++begin;
                    }
                }

                /** Check if all elements are equal to a given value. */
                template <class FwdIter, class Val>
                inline bool AllEqualTo(FwdIter begin, FwdIter end, const Val& val)
                {
                    while (begin != end)
                    {
                        if (*begin != val)
                        {
                            return false;
                        }

                        ++begin;
                    }

                    return true;
                }

                /** Check if any of the elements equals to a given value. */
                template <class FwdIter, class Val>
                inline bool AnyEqualTo(FwdIter begin, FwdIter end, const Val& val)
                {
                    while (begin != end)
                    {
                        if (*begin == val)
                        {
                            return true;
                        }

                        ++begin;
                    }

                    return false;
                }

                /** Check if two ranges contain the same values. */
                template <class FirstFwdIter, class SecondFwdIter>
                inline bool IsEqual(FirstFwdIter r1begin, FirstFwdIter r1end, 
                    SecondFwdIter r2begin)
                {
                    while (r1begin != r1end)
                    {
                        if (*r1begin != *r2begin)
                        {
                            return false;
                        }

                        ++r1begin;
                        ++r2begin;
                    }

                    return true;
                }

                /** Calculate the number of elements in a given sequence. */
                template <class FwdIter>
                inline Size Distance(FwdIter begin, FwdIter end)
                {
                    Size dist = 0;

                    while (begin != end)
                    {
                        ++dist;
                        ++begin;
                    }

                    return dist;
                }

                /** Copy values from one sequence to another. 
            
                    @return Iterator to the end of the destination range.
                */
                template <class InputFwdIter, class OutputFwdIter>
                inline OutputFwdIter Copy(InputFwdIter begin, InputFwdIter end, 
                    OutputFwdIter dest)
                {
                    while (begin != end)
                    {
                        *dest = *begin;
                        ++dest;
                        ++begin;
                    }

                    return dest;
                }

                /** Count elements with a specified value. */
                template <class FwdIter, class Val>
                inline Size Count(FwdIter begin, FwdIter end, const Val& val)
                {
                    Size num = 0;

                    while (begin != end)
                    {
                        if (*begin == val)
                        {
                            ++num;
                        }
                        ++begin;
                    }

                    return num;
                }

                /** Reverse a range of elements. */
                template <class BidirIter>
                inline void Reverse(BidirIter begin, BidirIter end)
                {
                    while (begin != end)
                    {
                        if (begin == --end)
                        {
                            break;
                        }

                        Swap(*begin, *end);
                        ++begin;
                    }
                }            
            }
        }
    }
}

#endif
