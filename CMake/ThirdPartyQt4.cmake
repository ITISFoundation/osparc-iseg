#
# Define how to use third party, i.e. 
# - include directory
# - library
# - preprocessor definitions needed by package
#

FIND_PACKAGE(Qt4 4.8.5 COMPONENTS QtCore QtGui QtScript QtXml QtOpenGL Qt3Support REQUIRED)

MACRO(USE_QT4)
	SET(QT_USE_QTOPENGL TRUE)

	IF(QT_USE_FILE)
		INCLUDE(${QT_USE_FILE})
		INCLUDE_DIRECTORIES(
			${QT_INCLUDES}
			${CMAKE_CURRENT_BINARY_DIR}
		)
	ELSE()
		MESSAGE(FATAL_ERROR "Qt not found!")
	ENDIF()

	LIST( APPEND MY_EXTERNAL_LINK_LIBRARIES ${QT_LIBRARIES} )
	
	IF(MSVC)
		ADD_DEFINITIONS("/D QT_DLL")
		REMEMBER_TO_CALL_THIS_INSTALL_MACRO( INSTALL_RUNTIME_LIBRARIES_QT )
	ENDIF()
ENDMACRO()

MACRO(INSTALL_RUNTIME_LIBRARIES_QT)
	MESSAGE( STATUS "--> ThirdPartyQt4: installing QT library ..." )
	IF(MSVC)
		get_filename_component(QT_DLL_PATH ${QT_QMAKE_EXECUTABLE} PATH)
		SET(MSVC_CONFIGURATION_TYPES ${CMAKE_CONFIGURATION_TYPES})
		FOREACH(BUILD_TYPE ${MSVC_CONFIGURATION_TYPES})
			STRING(TOLOWER ${BUILD_TYPE} build_type)
			IF(${build_type} MATCHES rel*)
				FILE(COPY ${QT_DLL_PATH}/
					DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BUILD_TYPE}
					FILES_MATCHING PATTERN "Qt*4.dll" PATTERN ".svn" PATTERN "Qt*d4.dll" EXCLUDE )
			ELSE()
				FILE(COPY ${QT_DLL_PATH}/
					DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BUILD_TYPE}
					FILES_MATCHING PATTERN "Qt*d4.dll" PATTERN ".svn" EXCLUDE )
			ENDIF()
		ENDFOREACH()
	ENDIF()
ENDMACRO()
