##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
IF(MSVC)
	ADD_DEFINITIONS("/D NOSURFACEGENERATIONTOOLSUPPORT")
ENDIF(MSVC)

OPTION(PLUGIN_VESSEL_WIDGET "Register vessel widget" OFF)
IF(PLUGIN_VESSEL_WIDGET)
	ADD_DEFINITIONS("/D PLUGIN_VESSEL_WIDGET")
ENDIF()

USE_QT4()
USE_VTK()
USE_ITK()
USE_BOOST()
USE_OPENMP()

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/Thirdparty/CImg
	${CMAKE_SOURCE_DIR}/Thirdparty/GeometricPredicates
	${CMAKE_SOURCE_DIR}/Thirdparty/GDCM
	${CMAKE_SOURCE_DIR}/Thirdparty/VTK/GUISupport/Qt
	${CMAKE_SOURCE_DIR}/GeometricPredicates
)


SET(MyMocHeaders
	ActiveslicesConfigWidget.h
	AtlasViewer.h
	AtlasWidget.h
	bmpshower.h
	EdgeWidget.h
	FastmarchingFuzzyWidget.h
	FeatureWidget.h
	HystereticGrowingWidget.h
	ImageForestingTransformRegionGrowingWidget.h
	ImageInformationDialogs.h
	InterpolationWidget.h
	LivewireWidget.h
	LoaderWidgets.h
	MainWindow.h
	MeasurementWidget.h
	MorphologyWidget.h
	OutlineCorrectionWidget.h
	PickerWidget.h
	RadiotherapyStructureSetImporter.h
	SaveOutlinesWidget.h
	Settings.h
	SliceViewerWidget.h
	SmoothingWidget.h
	ThresholdWidget.h
	TransformWidget.h
	UndoConfigurationDialog.h
	VesselWidget.h
	WatershedWidget.h
	WidgetCollection.h
)

FILE(GLOB ViewerHeaders *.h)

SET(ViewerSrcs
	ActiveslicesConfigWidget.cpp
	Atlas.cpp
	AtlasViewer.cpp
	AtlasWidget.cpp
	AvwReader.cpp
	bmp_read_1.cpp
	bmpshower.cpp
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
	ThresholdWidget.cpp
	TissueCleaner.cpp
	TissueHierarchy.cpp
	TissueInfos.cpp
	TissueLayerInfos.cpp
	TransformWidget.cpp
	UndoConfigurationDialog.cpp
	VesselWidget.cpp
	vtkEdgeCollapse.cpp
	vtkGenericDataSetWriter.cpp
	vtkImageExtractCompatibleMesher.cpp
	vtkTemplateTriangulator.cpp
	WatershedWidget.cpp
	WidgetCollection.cpp
	World.cpp
	XdmfImageMerger.cpp
	XdmfImageReader.cpp
	XdmfImageWriter.cpp
)

SET(UIS
	Settings.ui
)

SET(RCCS images.qrc)


QT4_WRAP_UI(UIHeaders ${UIS})
QT4_WRAP_CPP(MOCSrcs ${MyMocHeaders})
QT4_ADD_RESOURCES(RCCSrcs ${RCCS})

ADD_DEFINITIONS(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT_SCRIPT -DQT_THREAD_SUPPORT -DQT_NO_COMPAT -DQT3_SUPPORT -DISEG_RELEASEVERSION)
SET_SOURCE_FILES_PROPERTIES(${ViewerSrcs} PROPERTIES OBJECT_DEPENDS "${UIHeaders}")

ADD_EXECUTABLE(iSeg WIN32 MACOSX_BUNDLE ${ViewerSrcs} ${MOCSrcs} ${ViewerHeaders} ${RCCSrcs} ${CMAKE_BINARY_DIR}/config.h iSeg.rc)
TARGET_COMPILE_DEFINITIONS(iSeg PRIVATE ${ISEG_DEFINITIONS})
TARGET_LINK_LIBRARIES(iSeg 
	iSegCore
	Plugin
	QVTK
	predicates
	vtkGDCM
	${MY_EXTERNAL_LINK_LIBRARIES}
)

# To use SUBSYSTEM:WINDOWS instead of SUBSYSTEM:CONSOLE one needs to also link this library
IF(WIN32)
	TARGET_LINK_LIBRARIES(iSeg ${QT_QTMAIN_LIBRARY})
	MSVC_IGNORE_SPECIFIC_DEFAULT_LIBRARIES_DEBUG(MSVCRT.lib)
	MSVC_SOURCE_GROUP( "Moc Files"  ${MOCSrcs})
	MSVC_SOURCE_GROUP( "UI Headers" ${UIHeaders})
ENDIF()


IF(ISEG_BUILD_PRECOMPILED_HEADER)
	ADD_PRECOMPILED_HEADER( iSeg Precompiled.h )

	SET(sources_wo_pch ${MOCSrcs} ${RCCSrcs})
	FOREACH(src_file ${sources_wo_pch})
		SET_SOURCE_FILES_PROPERTIES(${src_file} 
			PROPERTIES COMPILE_FLAGS "/YuPrecompiled.h /FIPrecompiled.h")
	ENDFOREACH()
ENDIF()



# INSTALL
INSTALL(CODE "MESSAGE(\"Installing iSeg...\")")
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/iSeg/images/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/images FILES_MATCHING PATTERN "*.png" PATTERN ".svn" EXCLUDE)
IF(UNIX)
	INSTALL(FILES ${CMAKE_SOURCE_DIR}/iSeg/iSeg-launcher DESTINATION ${CMAKE_INSTALL_PREFIX}/bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
ENDIF()
INSTALL(TARGETS iSeg DESTINATION ${CMAKE_INSTALL_PREFIX}/bin CONFIGURATIONS RELEASE)
