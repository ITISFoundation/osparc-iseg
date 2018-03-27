/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Atlas.h"
#include "Precompiled.h"
#include <QDir>

Atlas::Atlas()
{
	for (int i = 0; i < maxnr; i++)
		m_atlasfilename[i] = QString("");
	m_atlasdir = QDir("");
}
