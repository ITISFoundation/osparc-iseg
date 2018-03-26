##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_BIAS "Build MRI bias correction plugin" ON)
IF(PLUGIN_BIAS)
	QT4_WRAP_CPP(MOCSrcsbias BiasCorrection.h)
	ADD_LIBRARY(BiasCorrection.ext SHARED BiasCorrectionAddon.cpp BiasCorrectionAddon.h BiasCorrection.cpp BiasCorrection.h ${MOCSrcsbias})
	TARGET_LINK_LIBRARIES( BiasCorrection.ext Addon ${ITK_LIBRARIES} QVTK ${QT_LIBRARIES}  )
	VS_SET_PROPERTY(BiasCorrection.ext "Addons")
ENDIF()