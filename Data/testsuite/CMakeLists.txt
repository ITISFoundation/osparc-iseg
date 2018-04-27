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
		test_DataMain.cpp

		test_Brush.cpp
		test_iSegImageAdaptor.cpp
		test_Transform.cpp
	)
	
	ADD_LIBRARY(TestSuite_iSegData ${SOURCES} ${HEADERS})
	TARGET_LINK_LIBRARIES(TestSuite_iSegData
		iSegData
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
	VS_SET_PROPERTY(TestSuite_iSegData "TestSuite")
ENDIF()
