/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef PROJECT_H
#define PROJECT_H

class QString;

class Project
{
public:
    Project();
	 QString m_loadprojfilename1;
	 QString m_loadprojfilename2;
	 QString m_loadprojfilename3;
	 QString m_loadprojfilename4;
	 QString m_filename;
	 int lpf1nr;
	 int lpf2nr;
	 int lpf3nr;
	 int lpf4nr;
	 int separatornr;
};

#endif // PROJECT_H
