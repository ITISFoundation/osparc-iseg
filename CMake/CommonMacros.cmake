#
# use OpenMP features
#
MACRO(USE_OPENMP)
	FIND_PACKAGE(OpenMP)
	IF(OpenMP_FOUND)
		ADD_COMPILE_OPTIONS(${OpenMP_CXX_FLAGS})
	ELSE()
		MESSAGE("OpenMP not found. Disabling OpenMP parallelization.")
		ADD_DEFINITIONS(-DNO_OPENMP_SUPPORT)
	ENDIF()
ENDMACRO()

MACRO(MSVC_IGNORE_SPECIFIC_DEFAULT_LIBRARIES_DEBUG)
	IF (MSVC)
		FOREACH(flag ${ARGN})
			SET( CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:\"${flag}\"" )
			SET( CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:\"${flag}\"" )
		ENDFOREACH()
	ENDIF()
ENDMACRO()

#
# Add compiler flags used by all modules
#
MACRO(SET_COMPILER_AND_LINKER_FLAGS)
	IF( MSVC )
		ADD_DEFINITIONS("/D _CRT_SECURE_NO_WARNINGS /D _SCL_SECURE_NO_WARNINGS")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:precise" )
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP4")

		# add debug information in release configuration and other optimization flags
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /Ox /Oi /Ot /Oy /GS- /Gy /D _SECURE_SCL=0" )
		SET(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /Zi")

		SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /debug /time")
		SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} /debug /time")
		SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /debug /time")

		# Enable fastlink option for debug build
		SET(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /debug:fastlink")
		SET(CMAKE_MODULE_LINKER_FLAGS_DEBUG "${CMAKE_MODULE_LINKER_FLAGS_DEBUG} /debug:fastlink")
		SET(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /debug:fastlink")
	ELSEIF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override -Wunreachable-code-aggressive")
	ENDIF()
ENDMACRO()

#
# Visual studio specific: add files of a project to a specific filter filter_name
# 
MACRO(MSVC_SOURCE_GROUP filter_name)
	IF(MSVC)
		SOURCE_GROUP( ${filter_name} FILES ${ARGN} )
	ENDIF()
ENDMACRO()
#
# Visual studio specific: add files of a project to a specific filter filter_name
# 
MACRO(MSVC_ONLY_EDIT_PROJECT project_name)
	IF(MSVC)
		ADD_CUSTOM_TARGET( ${project_name} SOURCES ${ARGN} )
	ENDIF()
ENDMACRO()

MACRO(SUBDIR_LIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    IF(IS_DIRECTORY ${curdir}/${child})
      LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()