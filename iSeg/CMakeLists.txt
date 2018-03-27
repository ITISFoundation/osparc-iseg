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

SET(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1 /openmp")
SET(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /Ob2 /D NDEBUG /openmp")

USE_QT4()
USE_VTK()
USE_ITK()
USE_BOOST()

IF(ISEG_GPU_MC)
	FIND_PACKAGE(OpenGL REQUIRED)
	FIND_PACKAGE(OpenCL REQUIRED)
	
	INCLUDE_DIRECTORIES(
		${OpenCL_INCLUDE_DIR}
	)

	ADD_SUBDIRECTORY(OpenCLUtilities)
	
	ADD_DEFINITIONS("/D GLEW_BUILD")
	ADD_LIBRARY(GPUMC SHARED gpu-mc.cpp rawUtilities.cpp glew/glew.c GPUMC.cpp)
	TARGET_LINK_LIBRARIES(GPUMC OCL-Utilities ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${OpenCL_LIBRARY})
	
	CONFIGURE_FILE(gpu-mc.cl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gpu-mc.cl)
	CONFIGURE_FILE(gpu-mc-morton.cl ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/gpu-mc-morton.cl)
	
	IF(ISEG_BUILD_PRECOMPILED_HEADER)
		ADD_PRECOMPILED_HEADER( GPUMC GPUMC.h )
	ENDIF()

	SET(FastMc GPUMC)
ENDIF()

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/GUISupport/Qt
	${CMAKE_SOURCE_DIR}/GeometricPredicates
)


SET(ViewerSrcs main.cpp)

SET(MyMocHeaders
	activeslices_config_widget.h
	AtlasViewer.h
	AtlasWidget.h
	bmpshower.h
	edge_widget.h
	fastmarch_fuzzy_widget.h
	featurewidget.h
	hyster_widget.h
	IFT_rg.h
	interpolation_widget.h
	livewirewidget.h
	loaderwidgets.h
	MainWindow.h
	measurementwidget.h
	morphowidget.h
	outlinecorrection.h
	pickerwidget.h
	pixelsize_widget.h
	rtstruct_importer.h
	SaveOutlinesWidget.h
	Settings.h
	smoothwidget.h
	threshwidget.h
	transformwidget.h
	undoconfig_widget.h
	vessel_widget.h
	watershedwidget.h
	widgetcollection.h
	xyslice.h
)

SET(ViewerHeaders
	${MyMocHeaders}
	Atlas.h
	avw.h
	bmp_read_1.h
	ChannelExtractor.h
	dicomread.h
	levelset.h
	pixmapgenerator.h
	Project.h
	resource.h
	resource1.h
	resource2.h
	SlicesHandler.h
	slicetransform.h
	tissuecleaner.h
	TissueHierarchy.h
	tissueinfos.h
	tissuelayerinfos.h
	windowsmodernstyle.h
	world.h
	XdmfImageReader.h
	XdmfImageWriter.h
	XdmfImageMerger.h
	TestingMacros.h
	vtkGenericDataSetWriter.h
	vtkImageExtractCompatibleMesher.h
	vtkTemplateTriangulator.h
	vtkEdgeCollapse.h
)

SET(ViewerSrcs
	SlicesHandler.cxx
	activeslices_config_widget.cpp
	Atlas.cxx
	AtlasViewer.cxx
	AtlasWidget.cxx
	avw.cpp
	bmp_read_1.cpp
	bmpshower.cpp
	ChannelExtractor.cpp
	edge_widget.cpp
	dicomread.cpp
	fastmarch_fuzzy_widget.cpp
	featurewidget.cpp
	dicomread.cpp
	XdmfImageReader.cxx
	XdmfImageWriter.cxx
	XdmfImageMerger.cxx
	hyster_widget.cpp
	IFT_rg.cpp
	interpolation_widget.cpp
	levelset.cpp
	livewirewidget.cpp
	loaderwidgets.cpp
	measurementwidget.cpp
	morphowidget.cpp
	outlinecorrection.cpp
	pickerwidget.cpp
	pixelsize_widget.cpp
	pixmapgenerator.cpp
	Project.cxx
	SaveOutlinesWidget.cpp
	smoothwidget.cpp
	MainWindow.cxx
	rtstruct_importer.cpp
	Precompiled.cpp
	Settings.cxx
	slicetransform.cpp
	threshwidget.cpp
	tissuecleaner.cpp
	TissueHierarchy.cpp
	tissueinfos.cpp
	tissuelayerinfos.cpp
	transformwidget.cpp
	undoconfig_widget.cpp
	vessel_widget.cpp
	watershedwidget.cpp
	widgetcollection.cpp
	world.cpp
	xyslice.cpp
	vtkGenericDataSetWriter.cxx
	vtkImageExtractCompatibleMesher.cxx
	vtkTemplateTriangulator.cxx
	vtkEdgeCollapse.cxx
	${ViewerSrcs}
)

SET(UIS
	Settings.ui
)

SET(RCCS images.qrc)

IF(ISEG_GPU_MC)
	LIST(APPEND MyMocHeaders glwidget.h window.h)
	LIST(APPEND ViewerSrcs   glwidget.cpp window.cpp)
ENDIF()


QT4_WRAP_UI(UIHeaders ${UIS})
QT4_WRAP_CPP(MOCSrcs ${MyMocHeaders})
QT4_ADD_RESOURCES(RCCSrcs ${RCCS})

ADD_DEFINITIONS(-DQT_GUI_LIBS -DQT_CORE_LIB -DQT_SCRIPT -DQT_THREAD_SUPPORT -DQT_NO_COMPAT -DQT3_SUPPORT -DISEG_RELEASEVERSION)
SET_SOURCE_FILES_PROPERTIES(${ViewerSrcs} PROPERTIES OBJECT_DEPENDS "${UIHeaders}")

ADD_EXECUTABLE(iSeg WIN32 MACOSX_BUNDLE ${ViewerSrcs} ${MOCSrcs} ${ViewerHeaders} ${RCCSrcs} ${CMAKE_BINARY_DIR}/config.h iSeg.rc)
TARGET_COMPILE_DEFINITIONS(iSeg PRIVATE ${ISEG_DEFINITIONS})
TARGET_LINK_LIBRARIES(iSeg 
	iSegCore
	Addon
	QVTK
	${FastMc}
	${QT_LIBRARIES}
	${OPENGL_LIBRARIES}
	${VTK_LIBRARIES}
	${ITK_LIBRARIES}
	${LIBS}
	${ZLIB_LIBRARY}
	predicates
	vtkGDCM
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
