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
    Compact structure for storing array of logical values.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_DATA_BITFIELD_H
#define GC_DATA_BITFIELD_H

#include "../System/Collection/Array.h"

namespace Gc
{
	/** %Auxiliary data encapsulating classes. */
	namespace Data
	{
		/** Structure for storing logical values as single bits.
		 
		    BitField class implements structure similar to array of \c bools. The difference
		    is that in BitField each logical value is stored as a single bit, so the overall
		    memory consumption is <b>8x lower</b>.
		*/
		class BitField
		{
		private:
			/** Data storage. */
            System::Collection::Array<1,Uint8> m_data;
			/** Number of logical values stored in the field. */
			Size m_size;

		public:
			/** Constructor. */
			BitField() 
				: m_size(0)
			{}

			/** Copy constructor. */
			BitField(const BitField &b)
				: m_size(0)
			{
				*this = b;
			}

			/** Assignment operator. */
			BitField& operator= (const BitField &b)
            {
                if (this != &b)
                {
                    m_data = b.m_data;
                    m_size = b.m_size;
                }

				return *this;
			}

			/** Destructor.
			 
                Deallocates the memory.
			*/
			~BitField()
			{
				Dispose();
			}

			/** Get number of bit elements in the field.
            
                @return The size of the field. 
             */
			Size Elements() const
			{
				return m_size;
			}

			/** Get the value of bit at given position.
			  
			    @param[in] pos Index of the element.
			    @return Logical value of the bit at position \c pos.
			*/
			bool Get(Size pos) const
			{
				return (m_data[pos >> 3] & (1 << (pos & 7))) != 0;
			}

			/** Change the logical value of bit at given position.
			 
			    @param[in] pos Bit index.
			    @param[in] val New value.
			*/
			void Set(Size pos, bool val)
			{
				if (val)
				{
					m_data[pos >> 3] |= 1 << (pos & 7);
				}
				else
				{
					m_data[pos >> 3] &= ~(1 << (pos & 7));
				}
			}

			/** Resize the field.
			
			    Resizes the field to a new size. The old content is lost.
			    @param[in] new_size New size of the field.
			*/
			void Resize(Size new_size)
			{
				Dispose();				

				if (m_size)
				{
                    m_data.Resize((m_size >> 3) + 1);
                    m_size = new_size;
				}
			}

			/** Fill the field with a given logical value.
			
			    Using this method all elements in the field are set to value \c logic_val.
           
			    @param[in] logic_val Logical value, \c true or \c false.
			    @see Clear
			*/
			void SetAllElements(bool logic_val = false)
			{
                m_data.SetAllElements(logic_val ? 0xff : 0);
			}

			/** Clear the contents of the field.
			
			    This method sets all bits in the field to \c false.
			    @see Fill
			*/
			void Clear()
			{
				SetAllElements(false);
			}

			/** Dispose the memory occupied by the field.
			
			    Sets the size of the field to 0 and deallocates the memory.
			*/
			void Dispose() 
			{ 
                m_data.Dispose();
				m_size = 0;
			}

			/** Check if the field is empty.
			
			    @return \c True if the size of the field is zero, \c false otherwise.
			*/
			bool IsEmpty() const
			{
				return m_size == 0;
			}
		};
	}
}

#endif
