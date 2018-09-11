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

#include "Transform.h"
#include "Types.h"

#include <string>
#include <vector>

namespace iseg {

class SliceHandlerInterface
{
public:
	typedef tissues_size_t tissue_type;
	typedef float pixel_type;

	virtual unsigned short width() const = 0;
	virtual unsigned short height() const = 0;
	virtual unsigned short num_slices() const = 0;

	virtual unsigned short start_slice() const = 0;
	virtual unsigned short end_slice() const = 0;

	virtual unsigned short active_slice() const = 0;
	virtual void set_active_slice(unsigned short slice, bool signal_change) = 0;

	virtual Transform transform() const = 0;
	virtual Vec3 spacing() const = 0;

	virtual tissuelayers_size_t active_tissuelayer() const = 0;
	virtual std::vector<const tissues_size_t*> tissue_slices(tissuelayers_size_t layeridx) const = 0;
	virtual std::vector<tissues_size_t*> tissue_slices(tissuelayers_size_t layeridx) = 0;

	virtual std::vector<const float*> source_slices() const = 0;
	virtual std::vector<float*> source_slices() = 0;

	virtual std::vector<const float*> target_slices() const = 0;
	virtual std::vector<float*> target_slices() = 0;

	virtual std::vector<std::string> tissue_names() const = 0;
	virtual std::vector<bool> tissue_locks() const = 0;
	virtual std::vector<tissues_size_t> tissue_selection() const = 0;
	virtual void set_tissue_selection(const std::vector<tissues_size_t>& sel) = 0;

	virtual bool has_colors() const = 0;
	virtual size_t number_of_colors() const = 0;
	virtual void get_color(size_t, unsigned char& r, unsigned char& g, unsigned char& b) const = 0;

	virtual void set_target_fixed_range(bool on) = 0;
};

} // namespace iseg
