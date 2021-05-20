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

#include "Data/Types.h"
#include "Data/Vec3.h"

#include <string>
#include <vector>

namespace iseg {

class SlicesHandlerInterface;

/** \brief Image writer based on ITK image writer factory
	*/
class ImageWriter
{
public:
	ImageWriter(bool binary = true) : m_Binary(binary) {}

	enum eImageSelection {
		kSource = 0,
		kTarget = 1,
		kTissue = 2
	};

	enum eSliceSelection {
		kSlice = 0,
		kActiveSlices = 1,
		kAllSlices = 2
	};

	ISEG_CORE_API bool WriteVolume(const std::string& filename, SlicesHandlerInterface* handler, eImageSelection img_selection, eSliceSelection slice_selection);

	template<typename T>
	bool WriteVolume(const std::string& filename, const std::vector<T*>& all_slices, eSliceSelection selection, const SlicesHandlerInterface* handler);

private:
	bool m_Binary;
};

iSegCore_TEMPLATE template ISEG_CORE_API bool ImageWriter::WriteVolume<float>(const std::string& filename, const std::vector<float*>& all_slices, eSliceSelection selection, const SlicesHandlerInterface* handler);

iSegCore_TEMPLATE template ISEG_CORE_API bool ImageWriter::WriteVolume<tissues_size_t>(const std::string& filename, const std::vector<tissues_size_t*>& all_slices, eSliceSelection selection, const SlicesHandlerInterface* handler);

} // namespace iseg
