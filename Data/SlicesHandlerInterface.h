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

#include "Transform.h"
#include "Types.h"

#include <string>
#include <vector>

namespace iseg {

class SlicesHandlerInterface
{
public:
	using tissue_type = tissues_size_t;
	using pixel_type = float;

	virtual unsigned short Width() const = 0;
	virtual unsigned short Height() const = 0;
	virtual unsigned short NumSlices() const = 0;

	virtual unsigned short StartSlice() const = 0;
	virtual unsigned short EndSlice() const = 0;

	virtual unsigned short ActiveSlice() const = 0;
	virtual void SetActiveSlice(unsigned short slice, bool signal_change) = 0;

	virtual Transform ImageTransform() const = 0;
	virtual Vec3 Spacing() const = 0;

	virtual tissuelayers_size_t ActiveTissuelayer() const = 0;
	virtual std::vector<const tissues_size_t*> TissueSlices(tissuelayers_size_t layeridx) const = 0;
	virtual std::vector<tissues_size_t*> TissueSlices(tissuelayers_size_t layeridx) = 0;

	virtual std::vector<const float*> SourceSlices() const = 0;
	virtual std::vector<float*> SourceSlices() = 0;

	virtual std::vector<const float*> TargetSlices() const = 0;
	virtual std::vector<float*> TargetSlices() = 0;

	virtual std::vector<std::string> TissueNames() const = 0;
	virtual std::vector<bool> TissueLocks() const = 0;
	virtual std::vector<tissues_size_t> TissueSelection() const = 0;
	virtual void SetTissueSelection(const std::vector<tissues_size_t>& sel) = 0;

	virtual bool HasColors() const = 0;
	virtual size_t NumberOfColors() const = 0;
	virtual void GetColor(size_t, unsigned char& r, unsigned char& g, unsigned char& b) const = 0;

	virtual void SetTargetFixedRange(bool on) = 0;
};

} // namespace iseg
