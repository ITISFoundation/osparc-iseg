/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef PROJECT_H
#define PROJECT_H

#include <QString>

namespace iseg {

class Project
{
public:
	Project();
	QString m_MLoadprojfilename1;
	QString m_MLoadprojfilename2;
	QString m_MLoadprojfilename3;
	QString m_MLoadprojfilename4;
	QString m_MFilename;
	int m_Lpf1nr;
	int m_Lpf2nr;
	int m_Lpf3nr;
	int m_Lpf4nr;
	int m_Separatornr;
};

} // namespace iseg

#endif // PROJECT_H
