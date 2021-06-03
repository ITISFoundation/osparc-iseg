/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "dark.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

#include <cassert>

namespace dark {

void setStyleSheet(QApplication *app) {
	QDir app_dir(app->applicationDirPath());
	QFile theme(app_dir.absoluteFilePath("dark/style.qss"));
	if (theme.open(QFile::ReadOnly | QFile::Text))
	{
		QTextStream themeStream(&theme);
		app->setStyleSheet(themeStream.readAll());
	}
	else
	{
		assert(false && "style shee not loaded");
	}
}

}
