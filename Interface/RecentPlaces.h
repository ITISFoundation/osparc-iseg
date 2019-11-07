/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	static QString getOpenFileName(QWidget* parent = 0,
			const QString& caption = QString(),
			const QString& dir = QString(),
			const QString& filter = QString());
			
	static QString getSaveFileName(QWidget* parent = 0,
			const QString& caption = QString(),
			const QString& dir = QString(),
			const QString& filter = QString());

	static void addRecent(const QString& dir_or_file_path);

	static QString lastDirectory();

	static std::deque<QString> recentDirectories();
};

}