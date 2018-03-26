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

USE_ITK()
USE_BOOST()
USE_VTK()
USE_OPENMP()

FILE(GLOB HEADERS *.h)
SET(SOURCES
	3D_outlines.cpp
	branchItem-simplified.cpp
	coef.cpp
	ColorLookupTable.cpp
	common.cpp
	contour_class.cpp
	EM.cpp
	feature_extractor.cpp
	fillcontour.cpp
	ImageReader.cpp
	ImageWriter.cpp
	index_priorqueue.cpp
	InitializeITKFactory.cpp
	gamma.cpp
	k_means.cpp
	linedraw.cpp
	LoadPlugin.cpp
	Mark.cpp
	marchingcubeprint.cpp
	matexport.cpp
	RTDoseReader.cpp
	RTDoseWriter.cpp
	RTDoseIODModule.cpp
	Precompiled.cpp
	sliceprovider.cpp
	stepsmooth.cpp
	UndoElem.cpp
	UndoQueue.cpp
	VoxelSurface.cpp
	VTIreader.cpp
)

ADD_LIBRARY(iSegCore ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(iSegCore
	${MY_EXTERNAL_LINK_LIBRARIES}
)

IF(ISEG_BUILD_PRECOMPILED_HEADER)
	ADD_PRECOMPILED_HEADER( iSegCore Precompiled.h )
ELSE()
	LIST(REMOVE_ITEM HEADERS Precompiled.h)
ENDIF()
