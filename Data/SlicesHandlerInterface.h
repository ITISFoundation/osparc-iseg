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

#include "Types.h"
#include "Transform.h"

#include <vector>

namespace iseg {

class SliceHandlerInterface
{
public:
	typedef tissues_size_t tissue_type;
	typedef float pixel_type;

	virtual unsigned short width() = 0;
	virtual unsigned short height() = 0;
	virtual unsigned short num_slices() = 0;

	virtual unsigned short start_slice() = 0;
	virtual unsigned short end_slice() = 0;
	virtual unsigned short active_slice() = 0;

	virtual Transform transform() const = 0;
	virtual Vec3 spacing() const = 0;

	virtual tissuelayers_size_t active_tissuelayer() = 0;
	virtual std::vector<const tissues_size_t*> tissue_slices(tissuelayers_size_t layeridx) const = 0;
	virtual std::vector<tissues_size_t*> tissue_slices(tissuelayers_size_t layeridx) = 0;

	virtual std::vector<const float*> source_slices() const = 0;
	virtual std::vector<float*> source_slices() = 0;

	virtual std::vector<const float*> target_slices() const = 0;
	virtual std::vector<float*> target_slices() = 0;
};

} // namespace iseg
