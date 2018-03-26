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

USE_HDF5()
USE_BOOST()

FILE(GLOB HEADERS *.h *.inl)
ADD_LIBRARY(HDF5IO HDF5Reader.cxx HDF5Writer.cxx HDF5IO.cpp ${HEADERS})
TARGET_LINK_LIBRARIES(HDF5IO Tools ${HDF5_LIBRARIES})
VS_SET_PROPERTY(HDF5IO "Common")
