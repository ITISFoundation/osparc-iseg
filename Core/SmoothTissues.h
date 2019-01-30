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

#include "../Data/Types.h"

#include <array>

namespace iseg {

class SliceHandlerInterface;

ISEG_CORE_API bool SmoothTissues(SliceHandlerInterface* handler, size_t start_slice, size_t end_slice, double sigma, bool smooth3d);

} // namespace iseg
