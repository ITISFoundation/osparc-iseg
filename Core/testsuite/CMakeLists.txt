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
	USE_HDF5()
	USE_ITK()
	
	FILE(GLOB HEADERS *.h)
	SET(SOURCES
		test_iSegCoreMain.cpp
	
		test_ConnectedInterpolation.cpp
		test_HDF5IO.cpp
		test_ImageIO.cpp
		test_BinaryThinning.cpp
	)
	
	ADD_TESTSUITE(TestSuite_iSegCore ${SOURCES} ${HEADERS})
	TARGET_LINK_LIBRARIES(TestSuite_iSegCore
		iSegData
		iSegCore
		${MY_EXTERNAL_LINK_LIBRARIES}
	)
ENDIF()
