##
## Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_TRACE_TUBES "Build tubular structures tracing plugin" OFF)
IF(PLUGIN_TRACE_TUBES)
	ADD_SUBDIRECTORY(testsuite)

	USE_BOOST()
	USE_VTK()
	USE_ITK() # for gdcm

	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Thirdparty)
  
	QT4_WRAP_CPP(MOCSrcs
		AutoTubeWidget.h
		TraceTubesWidget.h)
	
	FILE(GLOB PLUGIN_HEADERS *.h)

	ADD_LIBRARY(TraceTubesWidget.ext SHARED 
		AutoTubeWidget.cpp
		TraceTubesWidget.cpp 
		TracingPlugin.cpp
		${PLUGIN_HEADERS}
		${MOCSrcs}
	)
	
	TARGET_LINK_LIBRARIES( TraceTubesWidget.ext PRIVATE
		iSegData
		iSegInterface 
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
	VS_SET_PROPERTY(TraceTubesWidget.ext "Plugins")

	SET(PLUGIN_NAMES ${PLUGIN_NAMES};TraceTubesWidget.ext CACHE INTERNAL "")
ENDIF()
