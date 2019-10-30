#
# Define how to use third party, i.e. 
# - include directory
# - library
# - preprocessor definitions needed by package
#
OPTION(ISEG_VTK_OPENGL2 "Expect VTK with OpenGL2 backend (VTK_RENDERING_BACKEND=OpenGL2)" ON)
IF(ISEG_VTK_OPENGL2)
	FIND_PACKAGE(VTK REQUIRED
		COMPONENTS 
		vtkCommonDataModel
		vtkFiltersModeling
		vtkIOLegacy vtkIOGeometry vtkIOImage
		vtkImagingStencil
		vtkRenderingVolumeOpenGL2 vtkRenderingLOD
		vtkInteractionStyle vtkInteractionWidgets
	)
ELSE()
	FIND_PACKAGE(VTK REQUIRED
		COMPONENTS 
		COMPONENTS 
		vtkCommonDataModel
		vtkFiltersModeling
		vtkIOLegacy vtkIOGeometry vtkIOImage
		vtkImagingStencil
		vtkRenderingVolumeOpenGL vtkRenderingLOD
		vtkInteractionStyle vtkInteractionWidgets
	)
ENDIF()

IF(${VTK_VERSION_MAJOR}.${VTK_VERSION_MINOR}.${VTK_VERSION_BUILD} VERSION_LESS "7.1.0")
	MESSAGE(WARNING "VTK version is lower than 7.1. Some parts of iSEG may depend on newer features")
ENDIF()

MACRO(USE_VTK)
	INCLUDE(${VTK_USE_FILE})
	IF(ISEG_VTK_OPENGL2)
		ADD_DEFINITIONS(-DISEG_VTK_OPENGL2)
	ENDIF()
	
	LIST( APPEND MY_EXTERNAL_LINK_LIBRARIES ${VTK_LIBRARIES} )
	REMEMBER_TO_CALL_THIS_INSTALL_MACRO( INSTALL_RUNTIME_LIBRARIES_VTK )
ENDMACRO()

MACRO(INSTALL_RUNTIME_LIBRARIES_VTK)
	MESSAGE( STATUS "--> ThirdPartyVTK: installing VTK library ..." )
	IF(MSVC)
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
