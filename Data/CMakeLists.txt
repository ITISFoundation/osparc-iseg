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

USE_ITK()

FILE(GLOB HEADERS *.h)
SET(SOURCES
	addLine.cpp
	Transform.cpp
)

ADD_LIBRARY(iSegData ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(iSegData ${MY_EXTERNAL_LINK_LIBRARIES} )
