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
	Addon.cpp
	Addon.h
	qwidget1.h
	qwidget1.cpp
	SlicesHandlerInterface.h
)

QT4_WRAP_CPP(MOCSrcsAddon qwidget1.h)
ADD_LIBRARY(Addon SHARED  ${SOURCES} ${MOCSrcsAddon})
TARGET_LINK_LIBRARIES(Addon ${QT_LIBRARIES} ${QT_QTMAIN_LIBRARY} )
VS_SET_PROPERTY(Addon "Addons")
