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
    Numerical algorithm convergence failure exception.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#ifndef GC_MATH_CONVERGENCEEXCEPTION_H
#define GC_MATH_CONVERGENCEEXCEPTION_H

#include "../System/Exception.h"

namespace Gc
{
    namespace Math
    {
        /** Numerical algorithm convergence failure exception. */
        class ConvergenceException 
            : public System::Exception
        {
        public:
            /** Constructor. 
            
                @param[in] func Function name.
                @param[in] line Line number.
            */
            ConvergenceException(const char *func, Int32 line)
                : System::Exception(func, line, "Convergence error.")
            {}

            /** Constructor. 
            
                @param[in] func Function name.
                @param[in] line Line number.
                @param[in] msg String describing the exception and its cause.
            */
            ConvergenceException(const char *func, Int32 line, const std::string &msg)
                : System::Exception(func, line, msg)
            {}
        };
    }
}

#endif
