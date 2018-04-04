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