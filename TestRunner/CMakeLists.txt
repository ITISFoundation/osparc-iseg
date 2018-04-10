/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
IF(ISEG_BUILD_TESTING)
	USE_BOOST()

	ADD_EXECUTABLE(TestRunner TestRunner.cpp)
	TARGET_LINK_LIBRARIES(TestRunner ${MY_EXTERNAL_LINK_LIBRARIES})
	VS_SET_PROPERTY(TestRunner "TestSuite")
ENDIF()