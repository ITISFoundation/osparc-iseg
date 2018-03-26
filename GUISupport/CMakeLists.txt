##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
INCLUDE(${CMAKE_SOURCE_DIR}/CMake/vtkDependentOption.cmake)

# I think this option is not needed anymore
VTK_DEPENDENT_OPTION(VTK_USE_QVTK_QTOPENGL "Use QtOpenGL in QVTK" OFF
                     "VTK_USE_QVTK" OFF)
MARK_AS_ADVANCED(VTK_USE_QVTK_QTOPENGL)

SUBDIRS(Qt)

