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

class vtkLookupTable;

class iSegCore_API ColorLookupTable
{
public:
	ColorLookupTable();
	~ColorLookupTable();

	void SetNumberOfColors(size_t N);
	size_t NumberOfColors() const;

	void SetColor(size_t idx, unsigned char rgb[3]);
	void GetColor(double v, unsigned char rgb[3]) const;
	void GetColor(double v, unsigned char &r, unsigned char &g,
								unsigned char &b) const;

private:
	vtkLookupTable *m_Lut;
};
