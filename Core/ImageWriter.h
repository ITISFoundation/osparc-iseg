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
#include "iSegCore.h"

#include <string>
#include <vector>

namespace iseg {

class Transform;

/** \brief Image writer based on ITK image writer factory
	*/
class ISEG_CORE_API ImageWriter
{
public:
	ImageWriter(bool binary = true) : m_Binary(binary) {}

	template<typename T>
	bool writeVolume(const char* filename, const T** data, unsigned width,
					 unsigned height, unsigned nrslices, const float spacing[3],
					 const Transform& transform);

private:
	bool m_Binary;
};

iSegCore_TEMPLATE template ISEG_CORE_API bool ImageWriter::writeVolume<float>(
	const char* filename, const float** data, unsigned width, unsigned height,
	unsigned nrslices, const float spacing[3], const Transform& transform);

iSegCore_TEMPLATE template ISEG_CORE_API bool
	ImageWriter::writeVolume<tissues_size_t>(const char* filename,
											 const tissues_size_t** data,
											 unsigned width, unsigned height,
											 unsigned nrslices,
											 const float spacing[3],
											 const Transform& transform);

} // namespace iseg
