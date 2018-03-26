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

// BL TODO replace by ImageReader
namespace ChannelExtractor
{
	enum ChannelEnum
	{
		kRed,
		kGreen,
		kBlue,
		kAlpha
	};

	bool getSlice(const char *filename, float *slice, int channel, unsigned slicenr, unsigned width, unsigned height);
};