##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##

add_library (OCL-Utilities STATIC openCLUtilities.cpp openCLGLUtilities.cpp openCLUtilities.hpp openCLGLUtilities.hpp)
target_link_libraries(OCL-Utilities ${OpenCL_LIBRARY})
