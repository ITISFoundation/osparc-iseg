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
    N-dimensional data border iterator.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_DATA_BORDERITERATOR_H
#define GC_DATA_BORDERITERATOR_H

#include "../Type.h"
#include "../Math/Algebra/Vector.h"

namespace Gc
{
    namespace Data
    {
        /** N-dimensional data border iterator. 
        
            The purpose of this class is to provide fast and easy method to
            iterate through N-dimensional data which have a border of specific
            size in each dimension that has to be treated separately. 
            For instance, imagine you have an N-dimesional
            image (or other data) and you want to set all border elements of
            the image to 0. This can be done extremely easy using the BorderIterator:

            @code
            // Get pointer to the first image voxel
            T *data = img.Begin();
            // Create iterator for data with given size and border size of one voxel
            DataIterator<N> iter(img.Size(), Vector<N,Size>(1), Vector<N,Size>(1));
            // Init iterator - forward direction
            iter.Start(false);

            // Iterate through the data
            while (!iter.IsFinished())
            {
                // Size of the next block of data
                Size bsize;

                // Process next consecutive block of elements
                if (iter.NextBlock(bsize)) 
                {
                    // This block consists of border voxels - set them to 0
                    while (bsize-- > 0)
                    {
                        *data = 0;
                        data++;
                    }
                }
                else
                {
                    // Block of non-border voxels - don't do anything
                    data += bsize;
                }
            }
    
            @endcode

            Other possible applications of this class include data padding or iterating
            through data in which border elements require checking some border conditions, 
            etc.

            @tparam N Dimensionality of the data.

            @see NextBlock().
        */
        template <Size N>
        class BorderIterator
        {
        protected:
            /** Size of the N-dimensional space. */
            Math::Algebra::Vector<N,Size> m_sz;
            /** Number of elements required to move to the next position in given dimension. */
            Math::Algebra::Vector<N,Size> m_stride;
            /** Size of the border at the beginning of each dimension. */
            Math::Algebra::Vector<N,Size> m_left;
            /** Size of the border at the end of each dimension. */
            Math::Algebra::Vector<N,Size> m_right;
            /** Current position in the space. */
            Math::Algebra::Vector<N,Size> m_cur;
            /** Position of the last element. */
            Math::Algebra::Vector<N,Size> m_end;
            /** Number of elements yet to be processed. */
            Size m_count;
            /** Forward/Backward iteration. */
            bool m_is_reversed;
            /** All elements lie in the border. */
            bool m_is_only_border;

        public:
            /** Constructor. 
            
                @param[in] sz Size of the N-dimensional space.
                @param[in] bleft Size of the border at the beginning of each dimension.
                @param[in] bright Size of the border at the end of each dimension.

                @remarks If the size of the border is equal or greater than the size of the
                    array in any of the dimensions than all elements are border elements.
            */
            BorderIterator(const Math::Algebra::Vector<N,Size> &sz, 
                const Math::Algebra::Vector<N,Size> &bleft,
                const Math::Algebra::Vector<N,Size> &bright)
                : m_sz(sz), m_left(bleft), m_right(bright), m_count(0), m_is_reversed(false)
            {
                // Check parameters
                m_is_only_border = false;
                for (Size i = 0; i < N; i++)
                {
                    if (sz[i] <= bleft[i] + bright[i])
                    {
                        // All elements are border elements
                        m_is_only_border = true;
                        break;
                    }
                }

                // Calculate the stride (sub-space volume)
                m_stride[0] = 1;
                for (Size i = 1; i < N; i++)
                {
                    m_stride[i] = m_sz[i-1] * m_stride[i-1];
                }

                // Last elemenet position
                m_end = m_sz - Math::Algebra::Vector<N,Size>(1);
            }

            /** Initialize the iterator. 
            
                @param[in] reverse If \c true the data will be iterated from the end.
                    Otherwise from the beginning.
            */
            void Start(bool reverse)
            {
                m_cur.SetZero();
                m_count = m_sz.Product();

                if (reverse != m_is_reversed)
                {
                    std::swap(m_left, m_right);
                }
                m_is_reversed = reverse;
            }

            /** Check if the end of the data was reached. */
            bool IsFinished()
            {
                return (m_count == 0);
            }

            /** Returns whether forward or backward iteration is performed. */
            bool IsReversed()
            {
                return m_is_reversed;
            }

            /** Get the coordinates of the element at the beginning of current block. */
            Math::Algebra::Vector<N,Size> CurPos() const
            {
                if (m_is_reversed)
                {
                    return m_end - m_cur;
                }

                return m_cur;
            }

            /** Get forward/backward neighbour coordinates. */
            void NextPos(Math::Algebra::Vector<N,Size> &elpos)
            {
                if (m_is_reversed)
                {
                    Size k = 0;
                    while (k < N && --elpos[k] > m_end[k])
                    {
                        elpos[k] = m_end[k];
                        k++;
                    }
                }
                else
                {
                    Size k = 0;
                    while (k < N && ++elpos[k] > m_end[k])
                    {
                        elpos[k] = 0;
                        k++;
                    }
                }
            }

            /** Get next block of data.

                When iterating through the data, blocks of border and non-border elements
                alternate. This method tells what is the length of the next consecutive 
                block of elements and whether these elements lie in the specified border or
                not.

                The use of this method is very effective because it counts the border or
                non-border elements across all dimensions. For example having a 30x30x30
                volumetric image with border of 1 on each side of each dimension, the first
                call to NextBlock() will return \c true (=border) and size of the block
                will be 931 (=whole first slice + whole first line of the second slice + first
                voxel on the second line of the second slice). Second call will return
                \c false with the block size equal to 28. The third will return \c true with
                block size equal to 2 (last voxel on the second line of the second slice +
                first voxel on the third line of the second slice) and so on.

                @param[out] num Size of the block.
                @return \c True if the next consecutive block of elements of size \c num 
                    lies in the border. \c False otherwise.
            */
            bool NextBlock(Size &num)
            {
                if (m_is_only_border)
                {
                    num = m_count;
                    m_count = 0;
                    return true;
                }

                // Count left or right border elements
                for (Size i = N; i-- > 0; )
                {
                    if (m_cur[i] < m_left[i])
                    {
                        num = CountLeftBorderElements(i);
                        m_count -= num;
                        return true;
                    }
                    else if (m_cur[i] >= m_sz[i] - m_right[i])
                    {
                        num = CountRightBorderElements(i);
                        m_count -= num;
                        return true;
                    }
                }

                // Count non-border elements
                Size i = 0;
                while (!m_left[i] && !m_right[i] && i + 1 < N)
                {
                    i++;
                }

                Size nbs = m_sz[i] - m_left[i] - m_right[i]; 
                num = nbs * m_stride[i];
                m_cur[i] += nbs - 1;
                while (i < N && ++m_cur[i] >= m_sz[i])
                {
                    m_cur[i] = 0;
                    i++;
                }

                m_count -= num;
                return false;
            }

        private:
            Size CountLeftBorderElements(Size p)
            {
                Size num = 0;

                for (Size i = 0; i <= p; i++)
                {
                    num += m_left[i] * m_stride[i];
                    m_cur[i] = m_left[i];
                }

                return num;
            }

            Size CountRightBorderElements(Size p)
            {
                Size num = 0;

                while (p < N)
                {
                    if (++m_cur[p] >= m_sz[p] - m_right[p])
                    {
                        num += m_right[p] * m_stride[p];
                        m_cur[p] = 0;
                    }
                    else
                    {
                        num += CountLeftBorderElements(p-1);
                        break;
                    }
                    p++;
                }
                
                return num;
            }
        };
    }
}

#endif
