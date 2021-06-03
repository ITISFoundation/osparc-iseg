/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "ChannelExtractor.h"

#include <QColor>
#include <QImage>

namespace iseg {

bool ChannelExtractor::getSlice(const char* filename, float* slice, int channel, unsigned slicenr, unsigned width, unsigned height)
{
	QImage loaded_image(filename);

	if (loaded_image.height() != (int)height)
		return false;
	if (loaded_image.width() != (int)width)
		return false;

	int red_factor = 0;
	int green_factor = 0;
	int blue_factor = 0;
	int alpha_factor = 0;

	switch (channel)
	{
	case eChannelEnum::kRed: red_factor = 1; break;
	case eChannelEnum::kGreen: green_factor = 1; break;
	case eChannelEnum::kBlue: blue_factor = 1; break;
	case eChannelEnum::kAlpha: alpha_factor = 1; break;
	}

	unsigned int counter = 0;
	QColor old_color;
	for (int y = loaded_image.height() - 1; y >= 0; y--)
	{
		for (int x = 0; x < loaded_image.width(); x++)
		{
			old_color = QColor(loaded_image.pixel(x, y));
			slice[counter] = (unsigned char)(red_factor * old_color.red() +
																			 green_factor * old_color.green() +
																			 blue_factor * old_color.blue() +
																			 alpha_factor * old_color.alpha());
			counter++;
		}
	}

	return true;
}

} // namespace iseg
