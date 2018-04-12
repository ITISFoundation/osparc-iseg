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

#include "InterfaceApi.h"

#include "Interface/Point.h"

#include <vector>

namespace iseg {

ISEG_INTERFACE_API void addLine(std::vector<Point>* vP, Point p1, Point p2);
}
