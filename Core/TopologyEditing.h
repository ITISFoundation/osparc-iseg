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

#include "iSegCore.h"

#include "../Data/Types.h"

#include <boost/variant.hpp>

namespace iseg {

class SlicesHandlerInterface;

/// Fill holes and gaps in target image
ISEG_CORE_API bool FillLoopsAndGaps(SlicesHandlerInterface* handler, boost::variant<int, float> radius);

} // namespace iseg
