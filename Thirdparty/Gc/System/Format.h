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
    Helper text formatting class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_SYSTEM_FORMAT_H
#define GC_SYSTEM_FORMAT_H

#include <string>
#include <sstream>
#include "../Core.h"
#include "../Type.h"

namespace Gc {
namespace System {
/** Auxiliary text formatting class.
        
            This class can be used for simple and type-safe formatting of
            strings. The formatting string is passed to the class in the
            constructor and values are injected into the string using
            << operator. The position of the values in the string is
            specified using the {value_index} syntax.

            Example:
            @code
            int a = 5;
            std::string str = Format("The value of 'a' is {0}.") << a;
            @endcode

            @todo Escaping of the curly braces.
        */
class GC_DLL_EXPORT Format
{
  private:
    /** The formatted string. */
    std::string m_str;
    /** Next value index. */
    Size m_idx;

  public:
    /** Constructor. 
            
                @param[in] format Formatting string.
            */
    Format(const std::string & format)
        : m_str(format)
        , m_idx(0)
    {}

    /** Value insertion operator. */
    template <class T>
    Format & operator<<(const T & v)
    {
        std::ostringstream os;
        os << v;
        InsertString(os.str());
        return *this;
    }

    /** Get formatted string. */
    const std::string & ToString() const
    {
        return m_str;
    }

    /** Automatic conversion operator */
    operator const std::string &() const
    {
        return m_str;
    }

  protected:
    /** Insert string at the current value index position and
                increase the value index counter. 
            */
    void InsertString(const std::string & s);
};
}
} // namespace Gc::System

#endif
