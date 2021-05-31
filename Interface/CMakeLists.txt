##
## Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
USE_QT4()
USE_ITK()
USE_BOOST()

FILE(GLOB HEADERS *.h)
SET(SOURCES
	CollapsibleWidget.cpp
	ProgressDialog.cpp
	PropertyWidget.cpp
	Plugin.cpp
	RecentPlaces.cpp
	SplitterHandle.cpp
	WidgetInterface.cpp
)

QT4_WRAP_CPP(MOCSrcs 
	CollapsibleWidget.h
	ProgressDialog.h
	PropertyWidget.h
	SplitterHandle.h
	WidgetInterface.h
)
ADD_LIBRARY(iSegInterface SHARED  ${SOURCES} ${HEADERS} ${MOCSrcs})
TARGET_LINK_LIBRARIES(iSegInterface PRIVATE
	iSegData
	${MY_EXTERNAL_LINK_LIBRARIES}
)
