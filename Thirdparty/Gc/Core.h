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
    Shared library export macro definition.

    @author Ondrej Danek <ondrej.danek@gmail.com>
    @date 2009
*/

/** @mainpage Graph Cut optimization library

    @section introduction Introduction
    Graph Cut optimization library - <b>Gc</b> in short - is a library focusing on the graph cut
    combinatorial optimization and its use in digital image analysis. This research field has
    become very popular in the last decade and many interesting algorithms are built upon
    graph cuts. The library is being developed in <em>C++</em> and places emphasis especially 
    on speed and low memory usage as well as clean and extensible object-oriented design. 

    @section algortihms Algorithms
    The library offers implementation of several popular algorithms from the field. Here is the listed of the
    most interesting ones (click on the algorithm to get a detailed description and references):
    - <b>Maximum flow algorithms</b> - This library includes a wide range of the most 
        popular maximum flow algorithms in image processing all of which are highly optimized and exception safe. 
        The namespace Gc::Flow contains both implementations for general directed graphs (Gc::Flow::General namespace):
        - Gc::Flow::General::BoykovKolmogorov
        - Gc::Flow::General::FordFulkerson
        - Gc::Flow::General::EdmondsKarp
        - Gc::Flow::General::Dinitz
        - Gc::Flow::General::Kohli (dynamic Boykov-Kolmogorov algorithm)
        - Gc::Flow::General::PushRelabel::Fifo
        - Gc::Flow::General::PushRelabel::FifoGap
        - Gc::Flow::General::PushRelabel::HighestLevel
        .
        as well as implementations optimized for grid graphs (Gc::Flow::Grid namespace):
        - Gc::Flow::Grid::Kohli (dynamic Boykov-Kolmogorov algorithm)
        - Gc::Flow::Grid::PushRelabel::Fifo
        - Gc::Flow::Grid::PushRelabel::HighestLevel
        - Gc::Flow::Grid::ZengDanek (topology preserving max-flow algorithm)
        - Gc::Flow::Grid::DanekLabels (label preserving max-flow algorithm)
    - <b>Metric approximation</b>:
        - Gc::Energy::Potential::Metric::RiemannianDanek
        - Gc::Energy::Potential::Metric::RiemannianBoykov
    - <b>Multi-label discrete energy minimization methods</b>:
        - Gc::Energy::Min::Grid::AlphaExpansion
        - Gc::Energy::Min::Grid::AlphaBetaSwap
    - <b>Image segmentation</b>:
        - Gc::Algo::Segmentation::ChanVese
        - Gc::Algo::Segmentation::MumfordShah
        - Gc::Algo::Segmentation::RoussonDeriche

    @section examples Examples

    The directory <em>Examples</em> contains few examples of usage of the library. Mostly these examples are provided
    in the form of a <a href="http://www.mathworks.com">Matlab</a> interface to allow simple testing and fast
    prototyping, but can be easily rewritten for purposes of any library or application.

    @section cguide Compilation guide

    To generate <em>Makefile</em> for your platform and compiler use the <a href="http://www.cmake.org">CMake</a> tool.
    The library has no external dependencies and does not require any special tuning which makes the configuration and
    compilation process really straightforward. Note that creating a static version of the library is not supported.

    @section platforms Supported platforms
    One of the primary goals of the library is to be <b>cross-platform</b> and to avoid ugly platform or compiler specific hacks. 
    Basically, it should be possible to build the library on any platform using a <em>C++ compiler</em> with decent 
    support for modern standards (particularly templates). We have sucesfully compiled and used the library on the following
    platforms:
    - <b>Microsoft Windows XP</b> and newer, both <b>x86</b> and <b>x86-64</b> architectures, using 
        <a href="http://www.microsoft.com/express/">Visual Studio compiler</a> (versions 2008 and 2010 tested).
    - <b>Linux</b>, both <b>x86</b> and <b>x86-64</b> architectures, using 
        <a href="http://gcc.gnu.org/">GCC compiler</a> (several versions including 4.1 and above tested).

    @section License
    The library is licensed under the <a href="http://www.gnu.org/licenses/lgpl.html">GNU Lesser General Public License</a>. 
    Its text is included with the library and is also 
    accessible from the documentation, see \ref gnugpl and \ref gnulgpl. We also encourage you to reference this 
    library if you use it. Finally, note
    that some of the algorithms implemented in this library may have their own licensing rules (like not being available
    for commercial use or requiring citation of some publications) and some may have patents pending. We tried to
    mention these in the detailed documentation of such algorithms, but we can't guarantee this information is complete,
    so be careful.

    @section authors Authors
    The library has been developed at <a href="http://cbia.fi.muni.cz">Centre for Biomedical Image Analysis</a> at
    the <a href="http://www.fi.muni.cz">Faculty of Informatics</a>, <a href="http://www.muni.cz">Masaryk University</a>,
    Brno, Czech Republic. Currently it has only one author:
    - <b>Ondrej Danek</b>, contact: <em>xdanek2@fi.muni.cz</em> or <em>ondrej.danek@gmail.com</em>, web: http://www.ondrej-danek.net
    
    <hr>

    @section disclaimer Disclaimer
    We did our best to offer detailed information on the sources (either publications or implementations) that were used
    during the development of this library and the included algorithms. If any information is wrong or missing
    please feel free to contact us at the address mentioned above so we can fix the issue as soon as possible.
*/

#ifndef GC_CORE_H
#define GC_CORE_H

#if defined(_MSC_VER)
    // Shared library under MS environment, shall we export or import?
    #if defined(GC_SHOULD_EXPORT)
        #define GC_DLL_EXPORT __declspec(dllexport)
    #else
        #define GC_DLL_EXPORT __declspec(dllimport)
    #endif
#else
    #define GC_DLL_EXPORT
#endif

#endif
