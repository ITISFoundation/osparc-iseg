##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_TRACE_TUBES "Build tubular structures tracing plugin" ON)
IF(PLUGIN_TRACE_TUBES)
	ADD_SUBDIRECTORY(testsuite)

	USE_BOOST()

	QT4_WRAP_CPP(MOCSrcsbias TraceTubesWidget.h)
	
	FILE(GLOB PLUGIN_HEADERS *.h)

	ADD_LIBRARY(TraceTubesWidget.ext SHARED 
		TraceTubesWidget.cpp 
		TracingPlugin.cpp 
		${PLUGIN_HEADERS}
		${MOCSrcsbias}
	)
	
	TARGET_LINK_LIBRARIES( TraceTubesWidget.ext PRIVATE
		iSegData
		iSegInterface 
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
	VS_SET_PROPERTY(TraceTubesWidget.ext "Plugins")

	SET(PLUGIN_NAMES ${PLUGIN_NAMES};TraceTubesWidget.ext CACHE INTERNAL "")
ENDIF()