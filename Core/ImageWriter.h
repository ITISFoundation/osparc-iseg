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

#include "Data/Types.h"
#include "Data/Vec3.h"

#include <string>
#include <vector>

namespace iseg {

class SliceHandlerInterface;

/** \brief Image writer based on ITK image writer factory
	*/
class ImageWriter
{
public:
	ImageWriter(bool binary = true) : m_Binary(binary) {}

	template<typename T>
	bool writeVolume(const std::string& filename, const std::vector<T*>& all_slices, bool active_slices, const SliceHandlerInterface* handler);

private:
	bool m_Binary;
};

iSegCore_TEMPLATE template ISEG_CORE_API bool ImageWriter::writeVolume<float>(
	const std::string& filename, const std::vector<float*>& all_slices, bool active_slices, const SliceHandlerInterface* handler);

iSegCore_TEMPLATE template ISEG_CORE_API bool ImageWriter::writeVolume<tissues_size_t>(
	const std::string& filename, const std::vector<tissues_size_t*>& all_slices, bool active_slices, const SliceHandlerInterface* handler);

} // namespace iseg
