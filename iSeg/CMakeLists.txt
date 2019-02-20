##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
OPTION(PLUGIN_VESSEL_WIDGET "Register vessel widget" OFF)
IF(PLUGIN_VESSEL_WIDGET)
	ADD_DEFINITIONS("-DPLUGIN_VESSEL_WIDGET")
ENDIF()

USE_QT4()
USE_VTK()
USE_ITK()
USE_BOOST()
USE_OPENMP()

INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR} # hack to enable vtkDiscreteFlyingEdges.cxx to find Precompiled.h
	${CMAKE_SOURCE_DIR}/Thirdparty/CImg
	${CMAKE_SOURCE_DIR}/Thirdparty/GeometricPredicates
	${CMAKE_SOURCE_DIR}/Thirdparty/GDCM
	${CMAKE_SOURCE_DIR}/Thirdparty/VTK/GUISupport/Qt
	${CMAKE_SOURCE_DIR}/GeometricPredicates
)


FILE(GLOB ViewerHeaders *.h)

SET(ViewerSrcs
	ActiveslicesConfigWidget.cpp
	Atlas.cpp
	AtlasViewer.cpp
	AtlasWidget.cpp
	AvwReader.cpp
	bmp_read_1.cpp
	ImageViewerWidget.cpp
	ChannelExtractor.cpp
	DicomReader.cpp
	EdgeWidget.cpp
	FastmarchingFuzzyWidget.cpp
	FeatureWidget.cpp
	HystereticGrowingWidget.cpp
	ImageForestingTransformRegionGrowingWidget.cpp
	ImageInformationDialogs.cpp
	InterpolationWidget.cpp
	levelset.cpp
	LivewireWidget.cpp
	LoaderWidgets.cpp
	main.cpp
	MainWindow.cpp
	MeasurementWidget.cpp
	MorphologyWidget.cpp
	MultiDatasetWidget.cpp
	OutlineCorrectionParameterViews.cpp
	OutlineCorrectionWidget.cpp
	PickerWidget.cpp
	Precompiled.cpp
	Project.cpp
	RadiotherapyStructureSetImporter.cpp
	SaveOutlinesWidget.cpp
	Settings.cpp
	SlicesHandler.cpp
	SliceTransform.cpp
	SliceViewerWidget.cpp
	SmoothingWidget.cpp
	SurfaceViewerWidget.cpp
	ThresholdWidget.cpp
	TissueCleaner.cpp
	TissueHierarchy.cpp
	TissueInfos.cpp
	TissueLayerInfos.cpp
	TissueTreeWidget.cpp
	TransformWidget.cpp
	UndoConfigurationDialog.cpp
	VesselWidget.cpp
	vtkEdgeCollapse.cpp
	vtkGenericDataSetWriter.cpp
	vtkImageExtractCompatibleMesher.cpp
	vtkTemplateTriangulator.cpp
	VolumeViewerWidget.cpp 
	WatershedWidget.cpp
	WidgetCollection.cpp
	World.cpp
	XdmfImageMerger.cpp
	XdmfImageReader.cpp
	XdmfImageWriter.cpp
)

SET(MyMocHeaders
	ActiveslicesConfigWidget.h
	AtlasViewer.h
	AtlasWidget.h
	EdgeWidget.h
	FastmarchingFuzzyWidget.h
	FeatureWidget.h
	HystereticGrowingWidget.h
	ImageForestingTransformRegionGrowingWidget.h
	ImageInformationDialogs.h
	ImageViewerWidget.h
	InterpolationWidget.h
	LivewireWidget.h
	LoaderWidgets.h
	MainWindow.h
	MeasurementWidget.h
	MorphologyWidget.h
	MultiDatasetWidget.h
	OutlineCorrectionParameterViews.h
	OutlineCorrectionWidget.h
	PickerWidget.h
	RadiotherapyStructureSetImporter.h
	SaveOutlinesWidget.h
	Settings.h
	SliceViewerWidget.h
	SmoothingWidget.h
	SurfaceViewerWidget.h
	TissueTreeWidget.h
	ThresholdWidget.h
	TransformWidget.h
	UndoConfigurationDialog.h
	VesselWidget.h
	VolumeViewerWidget.h
	WatershedWidget.h
	WidgetCollection.h
)

IF(${VTK_MAJOR_VERSION}.${VTK_MINOR_VERSION}.${VTK_BUILD_VERSION} VERSION_LESS "8.2.0")
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/Thirdparty/VTK/FlyingEdges)
	LIST(APPEND ViewerSrcs ${CMAKE_SOURCE_DIR}/Thirdparty/VTK/FlyingEdges/vtkDiscreteFlyingEdges3D.cxx)
	LIST(APPEND ViewerHeaders ${CMAKE_SOURCE_DIR}/Thirdparty/VTK/FlyingEdges/vtkDiscreteFlyingEdges3D.h)
ENDIF()

SET(UIS
	Settings.ui
)

SET(RCCS images.qrc)

QT4_WRAP_UI(UIHeaders ${UIS})
QT4_WRAP_CPP(MOCSrcs ${MyMocHeaders})
QT4_ADD_RESOURCES(RCCSrcs ${RCCS})

ADD_DEFINITIONS(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT_SCRIPT -DQT_THREAD_SUPPORT -DQT_NO_COMPAT -DQT3_SUPPORT -DISEG_RELEASEVERSION)
SET_SOURCE_FILES_PROPERTIES(${ViewerSrcs} PROPERTIES OBJECT_DEPENDS "${UIHeaders}")


IF(WIN32)
	FOREACH(BUILD_TYPE ${CMAKE_CONFIGURATION_TYPES})
		FILE(COPY ${CMAKE_CURRENT_SOURCE_DIR}/images/ DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BUILD_TYPE}/images NO_SOURCE_PERMISSIONS FILES_MATCHING PATTERN "*.png" PATTERN ".svn" EXCLUDE)
		CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/images/isegicon64.ico ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${BUILD_TYPE}/images/isegicon.ico COPYONLY)
	ENDFOREACH()
ELSE()
	IF(APPLE)
		# NOTE: Don't include the path in MACOSX_BUNDLE_ICON_FILE -- this is
		# the property added to Info.plist
		SET(MACOSX_BUNDLE_ICON_FILE isegicon64.icns)

		# and this part tells CMake where to find and install the file itself
		SET_SOURCE_FILES_PROPERTIES(images/isegicon64.icns
			PROPERTIES MACOSX_PACKAGE_LOCATION "Resources"		
		)

		# install other icons
		FILE(GLOB PNG_FILES images/*.png)
		SET_SOURCE_FILES_PROPERTIES(${ICON_FILES}
			PROPERTIES MACOSX_PACKAGE_LOCATION "MacOS/images"		
		)
		SET(ICON_FILES ${PNG_FILES} images/isegicon64.icns)
	ENDIF()
ENDIF()

ADD_EXECUTABLE(iSeg WIN32 MACOSX_BUNDLE ${ViewerSrcs} ${MOCSrcs} ${ViewerHeaders} ${RCCSrcs} ${CMAKE_BINARY_DIR}/config.h iSeg.rc ${ICON_FILES})
TARGET_COMPILE_DEFINITIONS(iSeg PRIVATE ${ISEG_DEFINITIONS})
TARGET_LINK_LIBRARIES(iSeg 
	iSegData
	iSegInterface
	iSegCore
	QVTK
	predicates
	vtkGDCM
	${MY_EXTERNAL_LINK_LIBRARIES}
)

# To use SUBSYSTEM:WINDOWS instead of SUBSYSTEM:CONSOLE one needs to also link this library
IF(MSVC)
	TARGET_LINK_LIBRARIES(iSeg ${QT_QTMAIN_LIBRARY})
	MSVC_IGNORE_SPECIFIC_DEFAULT_LIBRARIES_DEBUG(MSVCRT.lib)
	MSVC_SOURCE_GROUP( "Moc Files"  ${MOCSrcs})
	MSVC_SOURCE_GROUP( "UI Headers" ${UIHeaders})
ENDIF()


IF(ISEG_BUILD_PRECOMPILED_HEADER)
	ADD_PRECOMPILED_HEADER( iSeg Precompiled.h )

	IF(MSVC)
		SET(sources_wo_pch ${MOCSrcs} ${RCCSrcs})
		FOREACH(src_file ${sources_wo_pch})
			SET_SOURCE_FILES_PROPERTIES(${src_file} 
				PROPERTIES COMPILE_FLAGS "/YuPrecompiled.h /FIPrecompiled.h")
		ENDFOREACH()
	ENDIF()
ENDIF()

IF(APPLE)
	INIT_BUNDLE(iSeg)
	
	INSTALL(TARGETS iSeg
		BUNDLE DESTINATION ${BUNDLE_INSTALL_TARGET}
		RUNTIME DESTINATION ${BUNDLE_INSTALL_TARGET}
	    COMPONENT executable
	)

	FILE(GLOB plugins ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/*.ext.*)
	INSTALL(FILES ${plugins} DESTINATION ${BUNDLE_INSTALL_DIRECTORY})
	
	FOREACH(lib ${plugins})
        GET_FILENAME_COMPONENT(libfile ${lib} NAME)
		MESSAGE("Adding ${libfile} to bundle")
        LIST(APPEND BUNDLE_LIBRARIES ${BUNDLE_INSTALL_DIRECTORY}/${libfile})
	ENDFOREACH()

	INSTALL_BUNDLE()
ENDIF()
