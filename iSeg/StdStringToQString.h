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

#include <QString>
#include <string>

namespace iseg {
inline QString ToQ(const std::string& str)
{
	return QString::fromStdString(str);
}

inline std::string ToStd(const QString& str)
{
	return str.toStdString();
}
} // namespace iseg