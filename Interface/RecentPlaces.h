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

#include "InterfaceApi.h"

#include <QString>

#include <deque>

class QWidget;

namespace iseg {

class ISEG_INTERFACE_API RecentPlaces
{
public:
	static QString GetOpenFileName(QWidget* parent = nullptr, const QString& caption = QString(), const QString& dir = QString(), const QString& filter = QString());

	static QString GetSaveFileName(QWidget* parent = nullptr, const QString& caption = QString(), const QString& dir = QString(), const QString& filter = QString());

	static void AddRecent(const QString& dir_or_file_path);

	static QString LastDirectory();

	static std::deque<QString> RecentDirectories();
};

} // namespace iseg
