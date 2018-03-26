/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <QString>

#pragma once

inline QString Format(const char* tooltip)
{
	QString fmt = "<html>\n";
	fmt += "<div style=\"width: 400px;\">" + QString(tooltip) + "</div>";
	fmt += "</html>";
	return fmt;
}
