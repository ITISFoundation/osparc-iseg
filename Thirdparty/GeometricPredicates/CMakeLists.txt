##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##

# Turn off optimization for predicates.cxx
IF(MSVC)
	SET_SOURCE_FILES_PROPERTIES(predicates.cxx PROPERTIES COMPILE_FLAGS "/Od")
ELSE(MSVC)
	SET_SOURCE_FILES_PROPERTIES(predicates.cxx PROPERTIES COMPILE_FLAGS "-O0")
ENDIF(MSVC)

ADD_LIBRARY(predicates STATIC predicates.cxx predicates.h)
TARGET_LINK_LIBRARIES(predicates)
VS_SET_PROPERTY(predicates Thirdparty)
