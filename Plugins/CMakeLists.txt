##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
SUBDIR_LIST(subdirs ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/iSeg)

USE_QT4()
USE_ITK()

SET(PLUGIN_NAMES "" CACHE INTERNAL "")

FOREACH(dir ${subdirs})
  ADD_SUBDIRECTORY(${dir})
ENDFOREACH()
