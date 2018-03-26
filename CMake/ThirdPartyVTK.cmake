#
# Define how to use third party, i.e. 
# - include directory
# - library
# - preprocessor definitions needed by package
#
FIND_PACKAGE(VTK 7.1 REQUIRED 
	COMPONENTS 
	vtkCommonCore vtkCommonDataModel vtkCommonSystem vtkCommonMisc vtkCommonExecutionModel vtkCommonTransforms vtkCommonMath
	vtkFiltersCore vtkFiltersSources vtkFiltersExtraction vtkFiltersGeneral vtkFiltersGeometry vtkFiltersHybrid vtkFiltersModeling vtkFiltersFlowPaths vtkFiltersVerdict
	vtkIOCore vtkIOLegacy vtkIOXML vtkIOParallel vtkIOImage vtkIOGeometry
	vtkImagingCore vtkImagingGeneral vtkImagingMath vtkImagingStatistics vtkImagingStencil vtkImagingHybrid
	vtkRenderingCore vtkRenderingOpenGL vtkInteractionWidgets
	vtksys
	vtkRenderingVolume vtkRenderingVolumeOpenGL vtkRenderingAnnotation vtkRenderingLOD
	vtkInteractionStyle)

MACRO(USE_VTK)
	INCLUDE(${VTK_USE_FILE})
	
	LIST( APPEND MY_EXTERNAL_LINK_LIBRARIES ${VTK_LIBRARIES} )
	REMEMBER_TO_CALL_THIS_INSTALL_MACRO( INSTALL_RUNTIME_LIBRARIES_VTK )
ENDMACRO()

MACRO(INSTALL_RUNTIME_LIBRARIES_VTK)
	MESSAGE( STATUS "--> ThirdPartyVTK: installing VTK library ..." )
	IF(CMAKE_COMPILER_IS_GNUCXX)
		IF(APPLE)
			FILE(COPY ${VTK_LIBRARY_DIR}/
				DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
				FILES_MATCHING PATTERN "*.dylib*")
		ELSE()
			FILE(COPY ${VTK_LIBRARY_DIR}/
				DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
				FILES_MATCHING PATTERN "*.so")
		ENDIF()
	ELSE()
		FOREACH(BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
			STRING(TOLOWER ${BUILD_TYPE} build_type)
			IF(${build_type} MATCHES rel*)
				FILE(COPY ${VTK_DIR}/bin/${BUILD_TYPE}/
						DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BUILD_TYPE}
						FILES_MATCHING PATTERN "vtk*.dll" PATTERN "vtk*.pdb" EXCLUDE)
			ELSE()
				FILE(COPY ${VTK_DIR}/bin/${BUILD_TYPE}/
					DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BUILD_TYPE}
					FILES_MATCHING PATTERN "vtk*.dll" PATTERN "vtk*.pdb")
			ENDIF()
		ENDFOREACH()
	ENDIF()	
ENDMACRO()
