##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_GRAPHCUT "build Graph Cut support" ON)
IF(PLUGIN_GRAPHCUT)
	INCLUDE_DIRECTORIES(
		${CMAKE_SOURCE_DIR}/Thirdparty/Gc
	)

	QT4_WRAP_CPP(MOCSrcsext 
		BoneSegmentationWidget.h
		TissueSeparatorWidget.h
	)

	ADD_LIBRARY(GraphCut.ext SHARED 
		GraphCutPlugin.cpp 
		GraphCutPlugin.h 
		BoneSegmentationWidget.cpp 
		BoneSegmentationWidget.h 
		TissueSeparatorWidget.cpp
		TissueSeparatorWidget.h
		ImageGraphCut3DFilter.h 
		ImageGraphCut3DFilter.hxx 
		${MOCSrcsext})

	# todo check libs in linker command
	TARGET_LINK_LIBRARIES( GraphCut.ext
		iSegInterface 
		Gc 
		${MY_EXTERNAL_LINK_LIBRARIES} 
	)
	VS_SET_PROPERTY(GraphCut.ext "Plugins")

	SET(PLUGIN_NAMES ${PLUGIN_NAMES};GraphCut.ext CACHE INTERNAL "")
ENDIF()
