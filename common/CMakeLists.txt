##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
)

ADD_LIBRARY(Tools Tools.cxx Tools.h)
VS_SET_PROPERTY(Tools "Common")

ADD_SUBDIRECTORY(HDF5IO)
