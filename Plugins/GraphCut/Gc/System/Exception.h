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
    Generic exception class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_EXCEPTION_H
#define GC_SYSTEM_EXCEPTION_H

#include <exception>
#include <string>
#include "../Type.h"

namespace Gc
{
    /** %System related classes. 
    
        The reason behind the existence of this namespace is that I don't like
        STL, especially the way it is structured and the functionality it
        offers. This namespace may be in the future separataed into a standalone
        library.
    */
    namespace System
    {
        /** Generic exception class. */
        class Exception 
            : public std::exception
        {
        private:
            /** Function in which the exception was thrown. */
            std::string m_func;
            /** Line number on which the exception was thrown. */
            Int32 m_line;
            /** String describing the exception and its cause. */
            std::string m_msg;

        public:
            /** Constructor. 
            
                @param[in] func Function name.
                @param[in] line Line number.
                @param[in] msg String describing the exception and its cause.
            */
            Exception(const char *func, Int32 line, const std::string &msg)
                : m_func(func), m_line(line), m_msg(msg)
            {}
            
            /** Destructor */
            ~Exception() throw()
            {}

            /** Get the file in which the exception was thrown. */
            const std::string &Function() const
            {
                return m_func;
            }

            /** Get the line number on which the exception was thrown. */
            Int32 LineNumber() const
            {
                return m_line;
            }

            /** Get the exception error message. */
            const std::string &Message() const
            {
                return m_msg;
            }

            /** Standard std::exception::what() method overload. */
            const char *what() const throw()
            {
                return m_msg.c_str();
            }
        };
    }
}

#endif
