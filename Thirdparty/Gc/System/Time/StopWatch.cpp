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
    Time interval measuring class.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include "../Log.h"
#include "../Format.h"
#include "StopWatch.h"

namespace Gc {
namespace System {
namespace Time {
/***********************************************************************************/

// Time measuring is disabled by default
bool StopWatch::ms_is_enabled = false;

/***********************************************************************************/

void StopWatch::Start(const char * func, Int32 line, const char * msg)
{
    m_has_start = ms_is_enabled;

    if (ms_is_enabled)
    {
        if (func == nullptr)
        {
            throw ArgumentException(__FUNCTION__, __LINE__, "Method name must be non-NULL.");
        }

        m_func = func;
        if (msg != nullptr)
        {
            m_msg = msg;
        }
        else
        {
            m_msg.clear();
        }
        m_line = line;
        m_start = clock();
    }
}

/***********************************************************************************/

void StopWatch::End()
{
    if (m_has_start && ms_is_enabled)
    {
        clock_t end = clock();
        if (m_msg.empty())
        {
            Log::Message(Format("{0}[{1}]: {2} sec") << m_func << m_line
                                                     << (end - m_start) / float(CLOCKS_PER_SEC));
        }
        else
        {
            Log::Message(Format("{0}[{1}] ({2}): {3} sec") << m_func.c_str()
                                                           << m_line << m_msg << (end - m_start) / float(CLOCKS_PER_SEC));
        }
        m_has_start = false;
    }
}

/***********************************************************************************/
}
}
} // namespace Gc::System::Time
