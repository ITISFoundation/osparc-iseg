/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */

/// This file defines several macros, which can be used in tests to check
/// e.g. if two values are equal.
/// When using these macros, you should define the variable 'error_counter'.
/// After running the tests, this variable can be used to check if the tests
/// failed.

#pragma once

#define check(expr)                                                            \
	if ((expr) == false)                                                       \
	{                                                                          \
		ISEG_ERROR() << "Expression false: " << (#expr) << std::endl;      \
		error_counter++;                                                       \
	}
#define check_equal(expr1, expr2)                                              \
	if ((expr1) != (expr2))                                                    \
	{                                                                          \
		ISEG_ERROR() << "Expression false: " << (expr1)                    \
				  << " != " << (expr2) << std::endl;                           \
		error_counter++;                                                       \
	}
#define check_not_equal(expr1, expr2)                                          \
	if ((expr1) == (expr2))                                                    \
	{                                                                          \
		ISEG_ERROR() << "Expression false: " << (expr1)                    \
				  << " == " << (expr2) << std::endl;                           \
		error_counter++;                                                       \
	}
