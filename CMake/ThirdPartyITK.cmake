#
# Define how to use third party, i.e. 
# - include directory
# - library
# - preprocessor definitions needed by package
#

FIND_PACKAGE(ITK COMPONENTS 
		ITKCommon ITKSmoothing ITKRegionGrowing ITKBinaryMathematicalMorphology ITKImageGradient ITKDistanceMap
		ITKLabelMap ITKVTK ITKCurvatureFlow ITKBiasCorrection ITKImageFeature ITKFastMarching
		ITKImageIO
		ITKReview
		# ITKSuperPixel
		REQUIRED)
SET(_itk_version "${ITK_VERSION_MAJOR}.${ITK_VERSION_MINOR}.${ITK_VERSION_BUILD}")
IF(${_itk_version} VERSION_LESS "5.0")
	MESSAGE(FATAL_ERROR "ITK version (${_itk_version}) is lower than 5.0. Some parts of iSEG may depend on newer features")
ENDIF()

MACRO(USE_ITK)
	INCLUDE( ${ITK_USE_FILE} )
	LIST( APPEND MY_EXTERNAL_LINK_LIBRARIES ${ITK_LIBRARIES} )
	REMEMBER_TO_CALL_THIS_INSTALL_MACRO( INSTALL_RUNTIME_LIBRARIES_ITK )
ENDMACRO()

MACRO(INSTALL_RUNTIME_LIBRARIES_ITK)
	IF(MSVC)
		MESSAGE( STATUS "--> ThirdPartyITK: installing ITK dlls ..." )
		FOREACH(BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
		STRING(TOLOWER ${BUILD_TYPE} build_type)
			IF(${build_type} MATCHES rel*)
				FILE(COPY ${ITK_DIR}/bin/${BUILD_TYPE}/
						DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BUILD_TYPE}
						FILES_MATCHING PATTERN "ITK*.dll" PATTERN "ITK*.pdb" EXCLUDE)
			ELSE()
				FILE(COPY ${ITK_DIR}/bin/${BUILD_TYPE}/
					DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BUILD_TYPE}
					FILES_MATCHING PATTERN "ITK*.dll" PATTERN "ITK*.pdb")
			ENDIF()
		ENDFOREACH()
	ENDIF()
ENDMACRO()
