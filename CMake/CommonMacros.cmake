#
# use OpenMP features
#
MACRO(USE_OPENMP)
	FIND_PACKAGE(OpenMP)
	IF(OpenMP_FOUND)
		SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
		LIST( APPEND MY_EXTERNAL_LINK_LIBRARIES ${OpenMP_CXX_LIBRARIES} )
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
	ELSEIF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		# -std=c++11 -stdlib=libc++ 
		SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-inconsistent-missing-override -Wunreachable-code-aggressive -Wunused")
	ENDIF()
ENDMACRO()

MACRO(ADD_TESTSUITE project_name)
	ADD_LIBRARY( ${project_name} ${ARGN} )

	SET(TESTSUITE_NAMES ${TESTSUITE_NAMES};${project_name} CACHE INTERNAL "")
	
	VS_SET_PROPERTY(${project_name} "TestSuite")
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

MACRO(INIT_BUNDLE app_name)
	SET(BUNDLE_APPS)
	SET(BUNDLE_LIBRARIES)
	SET(BUNDLE_SEARCH_DIRS ${CMAKE_INSTALL_PREFIX} ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

	SET(BUNDLE_INSTALL_TARGET ${CMAKE_INSTALL_PREFIX})
	IF(APPLE)
    	SET(APP_BUNDLE_NAME "${app_name}.app")
		SET(BUNDLE_INSTALL_DIRECTORY ${BUNDLE_INSTALL_TARGET}/${APP_BUNDLE_NAME}/Contents/MacOS)
    	LIST(APPEND BUNDLE_APPS ${BUNDLE_INSTALL_TARGET}/${APP_BUNDLE_NAME})
	ELSE()
    	SET(APP_BUNDLE_NAME ${app_name})
		SET(BUNDLE_INSTALL_DIRECTORY ${CMAKE_INSTALL_PREFIX})
    	LIST(APPEND BUNDLE_APPS ${CMAKE_INSTALL_PREFIX}/${app_name})
	ENDIF(APPLE)
ENDMACRO()

#
# Note about BundleUtilities:
#
# the libs passed as the second argument to fixup_bundle MUST already be copied into the app bundle.
# fixup_bundle only copies dependencies it discovers. Any libs passed as the second argument will
# also have their dependencies copied in and fixed.
MACRO(INSTALL_BUNDLE)
	INSTALL(CODE "
	include(BundleUtilities)
	set(BU_CHMOD_BUNDLE_ITEMS 1)
	fixup_bundle(\"${BUNDLE_APPS}\" \"${BUNDLE_LIBRARIES}\" \"${BUNDLE_SEARCH_DIRS}\")
	" COMPONENT Runtime)
ENDMACRO()