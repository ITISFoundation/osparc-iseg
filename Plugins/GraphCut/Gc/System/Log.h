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

#ifndef GC_SYSTEM_LOG_H
#define GC_SYSTEM_LOG_H

#include <string>
#include "../Core.h"
#include "../Type.h"

namespace Gc
{
    namespace System
    {
        /** Logging functionality. 
        
            This class is used by the library for logging purposes and
            all its output goes through it. It can be used to redirect
            the output by specifying user log target. When no logging 
            target is specified standard C output is used.
        */
        class GC_DLL_EXPORT Log
        {
        public:
            /** %Log target interface. */
            class ITarget
            {
            public:
                /** Virtual destructor. */
                virtual ~ITarget()
                {}

                /** %Log message. */
                virtual void Message(const std::string &str) = 0;

                /** %Log warning. */
                virtual void Warning(const std::string &str) = 0;

                /** %Log error. */
                virtual void Error(const std::string &str) = 0;
            };

        private:
            /** Active logging target. */
            static ITarget *ms_target;

        public:
            /** %Log message. */
            static void Message(const std::string &str);

            /** %Log warning. */
            static void Warning(const std::string &str);

            /** %Log error. */
            static void Error(const std::string &str);

            /** Get current target. */
            static ITarget *Target()
            {
                return ms_target;    
            }

            /** Set the logging target. 
            
                @param[in] target New logging target. Use NULL to
                    redirect the logging back to standard C streams.

                @warning The target is not deleted automatically.
            */
            static void SetTarget(ITarget *target)
            {
                ms_target = target;
            }
        };
    }
}

#endif
