##
## Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_BIAS "Build MRI bias correction plugin" ON)
IF(PLUGIN_BIAS)
	QT4_WRAP_CPP(MOCSrcs BiasCorrection.h)

	FILE(GLOB PLUGIN_HEADERS *.h)
	ADD_LIBRARY(BiasCorrection.ext SHARED 
		BiasCorrectionPlugin.cpp 
		BiasCorrection.cpp 
		${PLUGIN_HEADERS}
		${MOCSrcs}
	)
	
	TARGET_LINK_LIBRARIES( BiasCorrection.ext PRIVATE
		iSegData
		iSegInterface 
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
	VS_SET_PROPERTY(BiasCorrection.ext "Plugins")

	SET(PLUGIN_NAMES ${PLUGIN_NAMES};BiasCorrection.ext CACHE INTERNAL "")
ENDIF()