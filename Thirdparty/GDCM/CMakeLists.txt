##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##

FILE(GLOB HEADERS *.h)
SET(SOURCES
	vtkGDCMImageReader.cxx
	vtkGDCMImageWriter.cxx
	vtkGDCMMedicalImageProperties.cxx
	vtkLookupTable16.cxx
	vtkMyGDCMPolyDataReader.cxx 
)

USE_VTK()
USE_ITK() # for gdcm

ADD_LIBRARY(vtkGDCM ${SOURCES} ${HEADERS})
TARGET_LINK_LIBRARIES(vtkGDCM ${MY_EXTERNAL_LINK_LIBRARIES})
VS_SET_PROPERTY(vtkGDCM Thirdparty)