##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
ADD_SUBDIRECTORY(testsuite)

USE_BOOST()
USE_HDF5()
USE_BLOSC()
USE_OPENMP()
USE_ITK()
USE_VTK()

FILE(GLOB HEADERS *.h)
SET(SOURCES
	BranchItem.cpp
	ColorLookupTable.cpp
	Contour.cpp
	ExpectationMaximization.cpp
	FeatureExtractor.cpp
	fillcontour.cpp
	HDF5IO.cpp
	HDF5Reader.cpp
	HDF5Writer.cpp
	ImageReader.cpp
	ImageWriter.cpp
	IndexPriorityQueue.cpp
	InitializeITKFactory.cpp
	KMeans.cpp
	LoadPlugin.cpp
	Log.cpp
	MarchingCubes.cpp
	MatlabExport.cpp
	MultidimensionalGamma.cpp
	Outline.cpp
	Precompiled.cpp
	ProjectVersion.cpp
	RTDoseIODModule.cpp
	RTDoseReader.cpp
	RTDoseWriter.cpp
	SliceProvider.cpp
	SmoothSteps.cpp
	UndoElem.cpp
	UndoQueue.cpp
	VoxelSurface.cpp
	VTIreader.cpp
)

ADD_LIBRARY(iSegCore ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(iSegCore
	iSegData
	${MY_EXTERNAL_LINK_LIBRARIES}
)

IF(ISEG_BUILD_PRECOMPILED_HEADER)
	ADD_PRECOMPILED_HEADER( iSegCore Precompiled.h )
ELSE()
	LIST(REMOVE_ITEM HEADERS Precompiled.h)
ENDIF()
