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
    Basic logging support.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2010
*/

#include "Log.h"

#include <cstdio>

namespace Gc {
namespace System {
/***********************************************************************************/

// Standard C output by default
Log::ITarget * Log::ms_target = nullptr;
/***********************************************************************************/

void Log::Message(const std::string & str)
{
    if (ms_target != nullptr)
    {
        ms_target->Message(str);
    }
    else
    {
        fprintf(stdout, "%s\n", str.c_str());
    }
}

/***********************************************************************************/

void Log::Warning(const std::string & str)
{
    if (ms_target != nullptr)
    {
        ms_target->Message(str);
    }
    else
    {
        fprintf(stdout, "%s\n", str.c_str());
    }
}

/***********************************************************************************/

void Log::Error(const std::string & str)
{
    if (ms_target != nullptr)
    {
        ms_target->Message(str);
    }
    else
    {
        fprintf(stderr, "%s\n", str.c_str());
    }
}

/***********************************************************************************/
}
} // namespace Gc::System
