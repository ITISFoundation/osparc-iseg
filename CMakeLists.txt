##
## Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(ISEG C CXX)

SET(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Which Build Types to support" FORCE)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(BUILD_SHARED_LIBS ON)
INCLUDE(CTest)
ENABLE_TESTING()

LIST( APPEND ISEG_CMAKE_FILES_INCLUDES 
	${CMAKE_SOURCE_DIR}/CMake/CommonMacros.cmake
	${CMAKE_SOURCE_DIR}/CMake/CMakeSetProperty.cmake
	${CMAKE_SOURCE_DIR}/CMake/GetGitRevisionDescription.cmake
	${CMAKE_SOURCE_DIR}/CMake/InstallAllUsedLibraries.cmake
	${CMAKE_SOURCE_DIR}/CMake/PrecompiledHeaderSupport.cmake
	${CMAKE_SOURCE_DIR}/CMake/ThirdPartyBoost.cmake
	${CMAKE_SOURCE_DIR}/CMake/ThirdPartyHDF5.cmake
	${CMAKE_SOURCE_DIR}/CMake/ThirdPartyBlosc.cmake
	${CMAKE_SOURCE_DIR}/CMake/ThirdPartyQt4.cmake
	${CMAKE_SOURCE_DIR}/CMake/ThirdPartyITK.cmake
	${CMAKE_SOURCE_DIR}/CMake/ThirdPartyVTK.cmake
	${CMAKE_SOURCE_DIR}/CMake/ThirdPartyEigen.cmake
)
FOREACH( include_file ${ISEG_CMAKE_FILES_INCLUDES} )
	INCLUDE( ${include_file} )
ENDFOREACH()

MSVC_ONLY_EDIT_PROJECT( CMakeIncludes 
	${ISEG_CMAKE_FILES_INCLUDES}
	${INSTALL_ALL_USED_LIBRARIES_CMAKE} # to make sure, it is always added independent of option
)

MSVC_ONLY_EDIT_PROJECT( ProjectFiles 
	config.h.in
	LICENSE
	README.md
	.travis.yml
	.clang-format
	.clang-tidy
)


OPTION(ISEG_BUILD_TESTING "Build tests" ON)
OPTION(ISEG_BUILD_PRECOMPILED_HEADER "Build precompiled header files" OFF)
GET_GIT_HEAD_REVISION(GIT_REFSPEC GIT_SHA1)
SET(ISEG_VERSION "Open Source")
SET(ISEG_DESCRIPTION "Git Version: ${GIT_SHA1}")
SET(ISEG_PRODUCT_NAME "iSEG_v${ISEG_VERSION}")
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.in ${CMAKE_BINARY_DIR}/config.h)


SET_COMPILER_AND_LINKER_FLAGS()

# TODO: consider using INCLUDE_DIRECTORIES(SYSTEM
INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/Thirdparty/IJ/AlternativeMemoryModels
	${CMAKE_SOURCE_DIR}/Thirdparty/IJ/BinaryThinningImageFilter3D
	${CMAKE_SOURCE_DIR}/Thirdparty/OpenThinning
)

ADD_SUBDIRECTORY(Data)
ADD_SUBDIRECTORY(Core)
ADD_SUBDIRECTORY(Interface)
ADD_SUBDIRECTORY(iSeg)
ADD_SUBDIRECTORY(Plugins)
ADD_SUBDIRECTORY(TestRunner)
ADD_SUBDIRECTORY(Thirdparty)


# copy dlls and images to build folder
# TODO: be more clear about what happens during install vs configure
INSTALL_ALL_USED_LIBRARIES()
