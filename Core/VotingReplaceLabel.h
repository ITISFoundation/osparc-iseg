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

#include <array>

namespace iseg {

class SlicesHandlerInterface;

ISEG_CORE_API size_t VotingReplaceLabel(SlicesHandlerInterface* handler, tissues_size_t foreground, tissues_size_t background, std::array<unsigned int, 3> iradius, unsigned int majority_threshold, unsigned int max_iterations);

} // namespace iseg
