/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "iSegCore.h"

#include "Point.h"
#include <string>

typedef struct SMark
{
	SMark() {}
	SMark(const SMark &r) : p(r.p), mark(r.mark), name(r.name) {}

	Point p;
	unsigned mark;
	std::string name;
} mark;

typedef struct
{
	Point p;
	unsigned short slicenr;
	unsigned mark;
	std::string name;
} augmentedmark;

iSegCore_API bool operator!=(augmentedmark a, augmentedmark b);
