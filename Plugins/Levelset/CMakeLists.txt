##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_LEVELSET "Build levelset segmentation plugin" ON)
IF(PLUGIN_LEVELSET)
	USE_BOOST()
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Thirdparty)

	QT4_WRAP_CPP(MOCSrcslevel LevelsetWidget.h)

	FILE(GLOB PLUGIN_HEADERS *.h)
	ADD_LIBRARY(Levelset.ext SHARED 
		LevelsetPlugin.cpp 
		LevelsetWidget.cpp 
		${PLUGIN_HEADERS}
		${MOCSrcslevel}
	)

	TARGET_LINK_LIBRARIES( Levelset.ext PRIVATE
		iSegData
		iSegInterface 
		${MY_EXTERNAL_LINK_LIBRARIES} 
	)
	VS_SET_PROPERTY(Levelset.ext "Plugins")

	SET(PLUGIN_NAMES ${PLUGIN_NAMES};Levelset.ext CACHE INTERNAL "")
ENDIF()