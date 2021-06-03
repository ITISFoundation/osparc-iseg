##
## Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
# set up sources to build
SET(QVTKLibSrcs
	vtkEventQtSlotConnect.cxx
	vtkQtConnection.cxx
	QVTKPaintEngine.cxx
	QVTKWidget.cxx
	QVTKInteractorAdapter.cxx
	QVTKInteractor.cxx
)

SET(QVTKMocHeaders
	QVTKInteractorAdapter.h
	QVTKInteractorInternal.h
	QVTKWidget.h 
	vtkQtConnection.h
)

FILE(GLOB HEADERS *.h)

USE_QT4()
USE_VTK()

QT4_WRAP_CPP(QVTKLibMocSrcs ${QVTKMocHeaders})

ADD_LIBRARY(QVTK ${QVTKLibSrcs} ${QVTKLibMocSrcs} ${HEADERS})
TARGET_LINK_LIBRARIES( QVTK 
	${QT_LIBRARIES}
	${VTK_LIBRARIES}
)
VS_SET_PROPERTY(QVTK Thirdparty)

