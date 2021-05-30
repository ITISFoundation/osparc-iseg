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

#include "Atlas.h"

#include <QDir>

namespace iseg {

Atlas::Atlas()
{
	for (int i = 0; i < maxnr; i++)
		m_MAtlasfilename[i] = "";
	m_MAtlasdir = QDir("");
}

} // namespace iseg
