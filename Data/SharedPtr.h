/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include <memory>

/** Helper macro which creates smart pointer types of a given type.
	*/
#define ISEG_DECL_TYPE_PTR(T)                  \
	using T##_ptr = std::shared_ptr<T>;        \
	using T##_cptr = std::shared_ptr<T const>; \
	using T##_wptr = std::weak_ptr<T>;         \
	using T##_cwptr = std::weak_ptr<T const>;

#define ISEG_DECL_CLASS_PTR(classT) \
	class classT;                   \
	ISEG_DECL_TYPE_PTR(classT)
