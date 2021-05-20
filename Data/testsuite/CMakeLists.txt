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
		test_DataMain.cpp

		test_Brush.cpp
		test_Logging.cpp
		test_iSegImageAdaptor.cpp
		#test_SuperPixel.cpp
		test_Transform.cpp
	)
	
	ADD_TESTSUITE(TestSuite_iSegData ${SOURCES} ${HEADERS})
	TARGET_LINK_LIBRARIES(TestSuite_iSegData
		iSegData
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
ENDIF()
