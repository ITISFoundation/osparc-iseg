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

/// Common definitions for iSeg project.
namespace iseg {

ISEG_CORE_API int CombineTissuesVersion(const int version, const int tissuesVersion);

ISEG_CORE_API void ExtractTissuesVersion(const int combinedVersion, int& version, int& tissuesVersion);

} // namespace iseg
