
#find_package(QT NAMES Qt4 COMPONENTS Widgets REQUIRED)
#find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
USE_QT4()

QT4_ADD_RESOURCES(RCCSrcs dark/style.qrc)

add_library(QDarkStyleSheet OBJECT
	dark.cpp
	${RCCSrcs}
	${RESOURCE_FILES}
)

target_include_directories(
	QDarkStyleSheet PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:include>
)

target_link_libraries(
	QDarkStyleSheet
		#Qt${QT_VERSION_MAJOR}::Widgets
		${QT_LIBRARIES}
)
