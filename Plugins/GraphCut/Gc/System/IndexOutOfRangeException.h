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
    Index out of range exception class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_INDEXOUTOFRANGEEXCEPTION_H
#define GC_SYSTEM_INDEXOUTOFRANGEEXCEPTION_H

#include "ArgumentException.h"

namespace Gc
{
    namespace System
    {
        /** Index out of range exception class. */
        class IndexOutOfRangeException 
            : public ArgumentException
        {
        public:
            /** Constructor. 
            
                @param[in] func Function name.
                @param[in] line Line number.
            */
            IndexOutOfRangeException(const char *func, int line)
                : ArgumentException(func, line, "Index out of range.")
            {}

            /** Constructor. 
            
                @param[in] func Function name.
                @param[in] line Line number.
                @param[in] msg String describing the exception and its cause.
            */
            IndexOutOfRangeException(const char *func, int line, const std::string &msg)
                : ArgumentException(func, line, msg)
            {}
        };
    }
}

#endif
