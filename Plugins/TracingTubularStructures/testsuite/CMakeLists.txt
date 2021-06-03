##
## Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
		test_TraceTubesWidgetMain.cpp
		
		test_Metric.cpp
	)
	
	ADD_TESTSUITE(TestSuite_TraceTubesWidget ${SOURCES} ${HEADERS})
	TARGET_LINK_LIBRARIES(TestSuite_TraceTubesWidget
		iSegData
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
ENDIF()
