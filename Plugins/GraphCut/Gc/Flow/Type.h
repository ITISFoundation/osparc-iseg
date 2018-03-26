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
    Definitions of types used in maximum flow computations.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

#ifndef GC_FLOW_TYPE_H
#define GC_FLOW_TYPE_H

namespace Gc
{
    /** Namespace containing all methods for finding a maximum flow in a network. */
    namespace Flow
    {
        /** Node origin after the min-cut/max-flow computation. */
		enum Origin
		{
            /** Free node. Free nodes can be assigned to either the source 
                or the sink without changing the capacity of the minimal cut.
            */
			Free = 0,
            /** Node connected to the source. */
			Source = 1,
            /** Node connected to the sink. */
			Sink = 2
		};
    }
}

#endif
