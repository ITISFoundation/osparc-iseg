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

USE_QT4()
USE_ITK()

SET(SOURCES
	Plugin.cpp
	Plugin.h
	InterfaceApi.h
	SlicesHandlerInterface.h
	WidgetInterface.h
	WidgetInterface.cpp
)

QT4_WRAP_CPP(MOCSrcs WidgetInterface.h)
ADD_LIBRARY(iSegInterface SHARED  ${SOURCES} ${MOCSrcs})
TARGET_LINK_LIBRARIES(iSegInterface ${MY_EXTERNAL_LINK_LIBRARIES} )
