##
## Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
#########################################################################
#
# Gc library
#  
#########################################################################
# set the debug/release version
if(UNIX)
    option(GC_DEBUG_VERSION "Shall I add debug information?" ON)
    if(GC_DEBUG_VERSION) 
	   set(CMAKE_BUILD_TYPE "DEBUG" CACHE STATIC "Compilation for debug or release version?" FORCE)
    else(GC_DEBUG_VERSION)
	   set(CMAKE_BUILD_TYPE "RELEASE" CACHE STATIC "Compilation for debug or release version?" FORCE)	
    endif(GC_DEBUG_VERSION)
endif(UNIX)

#########################################################################
# compilation details
#########################################################################

# export symbols
if(WIN32)
    # Turn off some annoying warnings
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /nologo /wd4251 /wd4275")    
    # Fast math
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Oi /fp:fast")
    
    add_definitions(-DGC_SHOULD_EXPORT) 
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_DEPRECATE)
endif(WIN32)

# all warnings under gcc
if(UNIX)
	# gnu compiler
	if(CMAKE_COMPILER_IS_GNUCXX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -mfpmath=sse -msse2")
	endif(CMAKE_COMPILER_IS_GNUCXX)
endif(UNIX)


# set the list of source files
set(GC_SOURCES
	# Core
	Core.h
	Type.h
	
	# System
	System/Exception.h
	System/OutOfMemoryException.h
	System/ArgumentException.h
	System/IndexOutOfRangeException.h
	System/InvalidOperationException.h
	System/OverflowException.h
	System/NotImplementedException.h
	System/Format.cpp
	System/Format.h
	System/Log.h   
	System/Log.cpp    
	System/Collection/Tuple.h
	System/Collection/Array.h
	System/Collection/Pair.h
	System/Collection/ArrayBuilder.h
	System/Collection/IArrayMask.h
	System/Collection/BoolArrayMask.h    
	System/Collection/EmptyArrayMask.h
	System/Time/StopWatch.h         
	System/Time/StopWatch.cpp
	System/Algo/Basic.h
	System/Algo/Function.h
	System/Algo/Predicate.h
	System/Algo/Range.h    
	System/Algo/Iterator.h    
	System/Algo/Sort/Heap.h    
	
	# Math
	Math/Basic.h
	Math/Constant.h
	Math/ConvergenceException.h
	Math/Algebra/Number.h
	Math/Algebra/Vector.h
	Math/Algebra/Hyperplane.h
	Math/Algebra/Matrix.h
	Math/Algebra/SquareMatrix.h
	Math/Algebra/Eigenvectors.cpp
	Math/Algebra/Eigenvectors.h        
	Math/Geometry/ConvexHull.cpp
	Math/Geometry/ConvexHull.h
	Math/Geometry/Voronoi.cpp
	Math/Geometry/Voronoi.h
		
	# Maximum Flow
	Flow/Type.h
	Flow/IMaxFlow.h        
	Flow/IGridMaxFlow.h
	Flow/General/FordFulkerson.cpp    
	Flow/General/FordFulkerson.h
	Flow/General/EdmondsKarp.cpp    
	Flow/General/EdmondsKarp.h
	Flow/General/Dinitz.cpp    
	Flow/General/Dinitz.h
	Flow/General/BoykovKolmogorov.cpp
	Flow/General/BoykovKolmogorov.h    
	Flow/General/PushRelabel/Fifo.cpp    
	Flow/General/PushRelabel/Fifo.h
	Flow/General/PushRelabel/FifoGap.cpp    
	Flow/General/PushRelabel/FifoGap.h    
	Flow/General/PushRelabel/HighestLevel.cpp    
	Flow/General/PushRelabel/HighestLevel.h    
	Flow/General/Kohli.cpp
	Flow/General/Kohli.h
	Flow/Grid/CommonBase.h
	Flow/Grid/CommonBase.cpp    
	Flow/Grid/Kohli.cpp
	Flow/Grid/Kohli.h
	Flow/Grid/ZengDanek.cpp
	Flow/Grid/ZengDanek.h
	Flow/Grid/DanekLabels.cpp
	Flow/Grid/DanekLabels.h
	Flow/Grid/PushRelabel/HighestLevel.cpp    
	Flow/Grid/PushRelabel/HighestLevel.h    
	Flow/Grid/PushRelabel/Fifo.cpp    
	Flow/Grid/PushRelabel/Fifo.h    
		
	# Energy
	Energy/Neighbourhood.h
	Energy/IEnergy.h
	Energy/IGridEnergy.h
	Energy/Potential/Metric/RiemannianDanek.cpp
	Energy/Potential/Metric/RiemannianDanek.h
	Energy/Potential/Metric/RiemannianBoykov.cpp
	Energy/Potential/Metric/RiemannianBoykov.h    
	Energy/Min/Grid/AlphaBetaSwap.h
	Energy/Min/Grid/AlphaBetaSwap.cpp
	Energy/Min/Grid/AlphaExpansion.h
	Energy/Min/Grid/AlphaExpansion.cpp
	Energy/Min/Grid/LabelingEnergy.h
		
	# Data
	Data/Image.cpp
	Data/Image.h
	Data/BitField.h
	Data/BorderIterator.h
	
	# Algo
	Algo/Segmentation/Tools.cpp
	Algo/Segmentation/Tools.h
	Algo/Segmentation/Mask.h
	Algo/Segmentation/ChanVese.cpp
	Algo/Segmentation/ChanVese.h
	Algo/Segmentation/RoussonDeriche.cpp
	Algo/Segmentation/RoussonDeriche.h
	Algo/Segmentation/MumfordShah.cpp
	Algo/Segmentation/MumfordShah.h
	Algo/Geometry/DistanceTransform.cpp
	Algo/Geometry/DistanceTransform.h
	Algo/Geometry/SimplePoint.cpp
	Algo/Geometry/SimplePoint.h
	Algo/Clustering/KMeans.cpp
	Algo/Clustering/KMeans.h
)

add_library(Gc SHARED ${GC_SOURCES})
set_target_properties(Gc PROPERTIES VERSION 1.0.0)
VS_SET_PROPERTY(Gc Thirdparty)
