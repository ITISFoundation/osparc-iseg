/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef ATLAS_H
#define ATLAS_H

#include <qdir.h>

class Atlas
{
public:
	Atlas();
	static const int maxnr = 20;
	QString m_atlasfilename[maxnr];
	QDir m_atlasdir;
	int atlasnr[maxnr];
	int separatornr;
	int nratlases;
};

#endif // ATLAS_H
