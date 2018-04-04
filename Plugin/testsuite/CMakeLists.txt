##
## Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
## 
## This file is part of iSEG
## (see https://github.com/ITISFoundation/osparc-iseg).
## 
## This software is released under the MIT License.
##  https://opensource.org/licenses/MIT
##
IF(ISEG_BUILD_TESTING)
	USE_BOOST()
	USE_ITK()
	
	FILE(GLOB HEADERS *.h)
	SET(SOURCES
		test_PluginMain.cpp
	
		test_iSegImageAdaptor.cpp
	)
	
	ADD_LIBRARY(TestSuite_Plugin ${SOURCES} ${HEADERS})
	TARGET_LINK_LIBRARIES(TestSuite_Plugin
		Plugin
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
	VS_SET_PROPERTY(TestSuite_Plugin "TestSuite")
ENDIF()
