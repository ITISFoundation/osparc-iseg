#
# Define how to use third party, i.e. 
# - include directory
# - library
# - preprocessor definitions needed by package
#
OPTION(ISEG_WITH_BLOSC "Build with Blosc HDF5 compression filter" OFF)
IF(ISEG_WITH_BLOSC)
	IF(NOT BLOSC_ROOT)
		set(BLOSC_ROOT "not defined" CACHE PATH "path to BLOSC root folder")	
		message(FATAL_ERROR "Please define BLOSC root folder")
	endif(NOT BLOSC_ROOT)
	if (BLOSC_ROOT STREQUAL "not defined")
		message(FATAL_ERROR "Please define BLOSC root folder")
	endif(BLOSC_ROOT STREQUAL "not defined")

	SET(BLOSC_DEFINITIONS "-DUSE_HDF5_BLOSC")
	IF(MSCV)
		SET(BLOSC_LIBRARIES ${BLOSC_ROOT}/lib/blosc_filter.dll)
	ELSEIF(APPLE)
		SET(BLOSC_LIBRARIES ${BLOSC_ROOT}/lib/libblosc_filter.dylib ${BLOSC_ROOT}/lib/libblosc.a)
	ENDIF()
ENDIF()


MACRO(USE_BLOSC)
	IF(ISEG_WITH_BLOSC)
		INCLUDE_DIRECTORIES(${BLOSC_ROOT}/include)
		ADD_DEFINITIONS(${BLOSC_DEFINITIONS})
		
		LIST( APPEND MY_EXTERNAL_LINK_LIBRARIES ${BLOSC_LIBRARIES})
		REMEMBER_TO_CALL_THIS_INSTALL_MACRO( INSTALL_RUNTIME_LIBRARIES_BLOSC )
	ENDIF()
ENDMACRO()

MACRO(INSTALL_RUNTIME_LIBRARIES_BLOSC)
	MESSAGE( STATUS "--> ThirdPartyBlosc: installing BLOSC library ..." )
	IF(MSVC)
		# anything to copy?
	ENDIF()
ENDMACRO()