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

#ifndef GC_SYSTEM_TIME_STOPWATCH_H
#define GC_SYSTEM_TIME_STOPWATCH_H

#include "../../Core.h"
#include "../../Type.h"
#include "../ArgumentException.h"

#include <ctime>

namespace Gc {
namespace System {
/** Date and time handling. */
namespace Time {
/** %Time interval measuring class, that can be enabled/disabled at runtime. 
            
                The measured interval starts when the object is constructed (or manually
                using the Start() method) and ends when the object is destructed (or manually
                using the End() method). At the end the time interval is put to the standard
                output.
            */
class GC_DLL_EXPORT StopWatch
{
  private:
    /** Global toggle switch of the stopwatch output. */
    static bool ms_is_enabled;

  protected:
    /** Method/function name. */
    std::string m_func;
    /** Line number. */
    Int32 m_line;
    /** Message text. */
    std::string m_msg;
    /** Start timestamp. */
    clock_t m_start;
    /** Start timestamp has been captured. */
    bool m_has_start;

  public:
    /** Constructor. */
    StopWatch()
    {
        m_has_start = false;
    }

    /** Constructor. 
                
                    @param[in] func Method/function name.
                    @param[in] line Line number.
                    @param[in] msg Auxiliary message to be printed. Can be NULL.
                */
    StopWatch(const char * func, Int32 line, const char * msg = nullptr)
    {
        Start(func, line, msg);
    }

    /** Destructor. */
    ~StopWatch()
    {
        End();
    }

    /** Start time measuring manually. 
                
                    @param[in] func Method/function name.
                    @param[in] line Line number.
                    @param[in] msg Auxiliary message to be printed. Can be NULL.
                */
    void Start(const char * func, Int32 line, const char * msg = nullptr);

    /** End time measuring manually. */
    void End();

    /** Globaly enable/disable stopwatch output. */
    static void EnableOutput(bool enable)
    {
        ms_is_enabled = enable;
    }

    /** Check whether stopwatch output is enabled. */
    static bool IsOutputEnabled()
    {
        return ms_is_enabled;
    }
};
} // namespace Time
}
} // namespace Gc::System

#endif
