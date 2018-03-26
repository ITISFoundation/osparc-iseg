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
    Overflow exception class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_OVERFLOWEXCEPTION_H
#define GC_SYSTEM_OVERFLOWEXCEPTION_H

#include "Exception.h"

namespace Gc
{
    namespace System
    {
        /** Overflow exception class. */
        class OverflowException 
            : public Exception
        {
        public:
            /** Constructor. 
            
                @param[in] func Function name.
                @param[in] line Line number.
            */
            OverflowException(const char *func, int line)
                : Exception(func, line, "Overflow.")
            {}

            /** Constructor. 
            
                @param[in] func Function name.
                @param[in] line Line number.
                @param[in] msg String describing the exception and its cause.
            */
            OverflowException(const char *func, int line, const std::string &msg)
                : Exception(func, line, msg)
            {}
        };
    }
}

#endif
