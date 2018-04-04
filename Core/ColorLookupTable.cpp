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

#include <vtkLookupTable.h>

#include "ColorLookupTable.h"

namespace {
inline double clamp(double v) { return (v > 255) ? 255 : (v < 0 ? 0 : v); }
} // namespace

namespace iseg {

ColorLookupTable::ColorLookupTable() { m_Lut = vtkLookupTable::New(); }

ColorLookupTable::~ColorLookupTable() { m_Lut->Delete(); }

void ColorLookupTable::SetNumberOfColors(size_t N)
{
	m_Lut->SetNumberOfTableValues(N);
	m_Lut->SetNumberOfColors(N);
	m_Lut->SetTableRange(0, N - 1);
}

size_t ColorLookupTable::NumberOfColors() const
{
	return m_Lut->GetNumberOfColors();
}

void ColorLookupTable::SetColor(size_t idx, unsigned char rgb[3])
{
	m_Lut->SetTableValue(idx, rgb[0] / 255.0, rgb[1] / 255.0, rgb[2] / 255.0,
						 1.0);
}

void ColorLookupTable::GetColor(double v, unsigned char uchar_rgb[3]) const
{
	double rgb[3];
	m_Lut->GetColor(v, rgb);

	uchar_rgb[0] = static_cast<unsigned char>(clamp(rgb[0] * 255.0));
	uchar_rgb[1] = static_cast<unsigned char>(clamp(rgb[1] * 255.0));
	uchar_rgb[2] = static_cast<unsigned char>(clamp(rgb[2] * 255.0));
}

void ColorLookupTable::GetColor(double v, unsigned char& r, unsigned char& g,
								unsigned char& b) const
{
	double rgb[3];
	m_Lut->GetColor(v, rgb);

	r = static_cast<unsigned char>(clamp(rgb[0] * 255.0));
	g = static_cast<unsigned char>(clamp(rgb[1] * 255.0));
	b = static_cast<unsigned char>(clamp(rgb[2] * 255.0));
}

} // namespace iseg
