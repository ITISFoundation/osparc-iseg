/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

using namespace iseg;
using namespace iseg::ChannelExtractor;

bool ChannelExtractor::getSlice(const char* filename, float* slice, int channel,
								unsigned slicenr, unsigned width,
								unsigned height)
{
	QImage loadedImage(filename);

	if (loadedImage.height() != (int)height)
		return false;
	if (loadedImage.width() != (int)width)
		return false;

	int redFactor = 0;
	int greenFactor = 0;
	int blueFactor = 0;
	int alphaFactor = 0;

	switch (channel)
	{
	case ChannelEnum::kRed: redFactor = 1; break;
	case ChannelEnum::kGreen: greenFactor = 1; break;
	case ChannelEnum::kBlue: blueFactor = 1; break;
	case ChannelEnum::kAlpha: alphaFactor = 1; break;
	}

	unsigned int counter = 0;
	QColor oldColor;
	for (int y = loadedImage.height() - 1; y >= 0; y--)
	{
		for (int x = 0; x < loadedImage.width(); x++)
		{
			oldColor = QColor(loadedImage.pixel(x, y));
			slice[counter] = (unsigned char)(redFactor * oldColor.red() +
											 greenFactor * oldColor.green() +
											 blueFactor * oldColor.blue() +
											 alphaFactor * oldColor.alpha());
			counter++;
		}
	}

	return true;
}
