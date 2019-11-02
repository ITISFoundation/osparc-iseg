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

#include <cstdint>

#define TISSUES_SIZE_TYPEDEF // TODO: Used to mark file IO where change of tissues size type still needs to be handled.

namespace iseg {

/*
* Tissues size type
*/
#ifdef TISSUES_SIZE_TYPEDEF
#	define TISSUES_SIZE_MAX 65535
using tissues_size_t = std::uint16_t;
#else // TISSUES_SIZE_TYPEDEF
#	define TISSUES_SIZE_MAX 255
using tissues_size_t = std::uint8_t; // Original tissues size type
#endif // TISSUES_SIZE_TYPEDEF

/*
* Tissue layers size type
*/
#define TISSUELAYERS_SIZE_MAX 255
using tissuelayers_size_t = std::uint8_t;

enum eScaleMode {
	kArbitraryRange= 1, ///< assumes an arbitrary range
	kFixedRange = 2		///< intensities are assumed to be in range [0,255]
};

} // namespace iseg
