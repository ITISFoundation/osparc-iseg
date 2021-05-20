##
## Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_KALMAN_TRACER "Build Kalman filter tracing plugin" ON)
IF(PLUGIN_KALMAN_TRACER)
	#ADD_SUBDIRECTORY(testsuite)

	USE_BOOST()
	USE_ITK() # for gdcm
	USE_EIGEN()

	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Thirdparty)
	INCLUDE_DIRECTORIES(../TracingTubularStructures)
  
	QT4_WRAP_CPP(MOCSrcs 
		AutoTubePanel.h
	)
	
	IF(MSVC)
		SET_SOURCE_FILES_PROPERTIES(AutoTubePanel.cpp PROPERTIES COMPILE_FLAGS /bigobj)
	ENDIF()
	
	FILE(GLOB PLUGIN_HEADERS *.h)

	ADD_LIBRARY(KalmanFilterTracingWidget.ext SHARED 
		AutoTubePanel.cpp
		KalmanFilter.cpp
		TracingPlugin.cpp
		${PLUGIN_HEADERS}
		${MOCSrcs}
	)
	
	TARGET_LINK_LIBRARIES( KalmanFilterTracingWidget.ext PRIVATE
		iSegData
		iSegInterface 
		${MY_EXTERNAL_LINK_LIBRARIES}
        Eigen3::Eigen
	)
	VS_SET_PROPERTY(KalmanFilterTracingWidget.ext "Plugins")

	SET(PLUGIN_NAMES ${PLUGIN_NAMES};KalmanFilterTracingWidget.ext CACHE INTERNAL "")
ENDIF()
