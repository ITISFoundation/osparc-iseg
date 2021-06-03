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

#include <sstream>
#include "Format.h"

namespace Gc {
namespace System {
/***********************************************************************************/

void Format::InsertString(const std::string & s)
{
    std::ostringstream ns;
    ns << '{' << m_idx << '}';

    size_t sof = 0, sp;
    while ((sp = m_str.find(ns.str(), sof)) != std::string::npos)
    {
        m_str.replace(sp, ns.str().size(), s);
        sp++;
    }

    m_idx++;
}

/***********************************************************************************/
}
} // namespace Gc::System
