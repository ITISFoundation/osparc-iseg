##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_CONFIDENCE "Build confidence connected segmentation plugin" OFF)
IF(PLUGIN_CONFIDENCE)
	QT4_WRAP_CPP(MOCSrcscon ConfidenceWidget.h)

	ADD_LIBRARY(Confidence.ext SHARED 
		ConfidencePlugin.cpp 
		ConfidencePlugin.h 
		ConfidenceWidget.cpp 
		ConfidenceWidget.h 
		${MOCSrcscon}
	)

	TARGET_LINK_LIBRARIES( Confidence.ext 
		iSegData
		iSegInterface 
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
	VS_SET_PROPERTY(Confidence.ext "Plugins")

	SET(PLUGIN_NAMES ${PLUGIN_NAMES};Confidence.ext CACHE INTERNAL "")
ENDIF()