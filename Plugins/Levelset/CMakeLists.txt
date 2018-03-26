##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_LEVELSET "Build levelset segmentation plugin" OFF)
IF(PLUGIN_LEVELSET)
	QT4_WRAP_CPP(MOCSrcslevel Levelset.h)
	ADD_LIBRARY(Levelset.ext SHARED LevelsetAddon.cpp LevelsetAddon.h Levelset.cpp Levelset.h ${MOCSrcslevel})
	TARGET_LINK_LIBRARIES( Levelset.ext Addon ${ITK_LIBRARIES} QVTK ${QT_LIBRARIES}  )
	VS_SET_PROPERTY(Levelset.ext "Addons")
ENDIF()