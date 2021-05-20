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

#include <qdir.h>

namespace iseg {

class Atlas
{
public:
	Atlas();
	static const int maxnr = 20;
	QString m_MAtlasfilename[maxnr];
	QDir m_MAtlasdir;
	int m_Atlasnr[maxnr];
	int m_Separatornr;
	int m_Nratlases;
};

} // namespace iseg
