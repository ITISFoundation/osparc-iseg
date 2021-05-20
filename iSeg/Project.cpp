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

#include "Project.h"

#include <QString>

namespace iseg {

Project::Project()
{
	m_MLoadprojfilename1 = QString("");
	m_MLoadprojfilename2 = QString("");
	m_MLoadprojfilename3 = QString("");
	m_MLoadprojfilename4 = QString("");
	m_MFilename = QString("");
}

} // namespace iseg