/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "RecentPlaces.h"

#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QList>
#include <QUrl>

namespace iseg {

namespace {

std::deque<QString> _list;

QString _getFileName(QWidget* parent,
		const QString& caption,
		const QString& dir,
		const QString& filter,
		QFileDialog::FileMode mode)
{
	QString last_dir = !dir.isEmpty() ? dir : RecentPlaces::lastDirectory();
	QList<QUrl> urls;
	for (const auto& d : RecentPlaces::recentDirectories())
	{
		urls << QUrl::fromLocalFile(d);
	}

	QFileDialog dialog(parent);
	dialog.setFileMode(mode);
	dialog.setCaption(caption);
	dialog.setFilter(filter);
	dialog.setDirectory(last_dir);
	dialog.setSidebarUrls(urls);
	if (dialog.exec() == QDialog::Accepted)
	{
		auto file_path = dialog.selectedFiles().value(0);
		RecentPlaces::addRecent(file_path);
		return file_path;
	}
	return QString();
}

} // namespace

void RecentPlaces::addRecent(const QString& path)
{
	if (path.isEmpty())
		return;

	// get directory
	// QFileInfo::absolutePath: Returns a file's path absolute path. This doesn't include the file name.
	QString dir = QFileInfo(path).isFile() ? QFileInfo(path).absolutePath() : path;

	// push to top
	_list.push_front(dir);

	// remove duplicates (keep first)
	{
		auto end = _list.end();
		for (auto it = _list.begin(); it != end; ++it)
		{
			end = std::remove(it + 1, end, *it);
		}
		_list.erase(end, _list.end());
	}

	// keep only last 5 results
	if (_list.size() > 5)
	{
		_list.resize(5);
	}
}

QString RecentPlaces::lastDirectory()
{
	if (!_list.empty())
		return _list.front();
	return QString::null;
}

std::deque<QString> RecentPlaces::recentDirectories()
{
	return _list;
}

QString RecentPlaces::getOpenFileName(QWidget* parent,
		const QString& caption,
		const QString& dir,
		const QString& filter)
{
	return _getFileName(parent, caption, dir, filter, QFileDialog::ExistingFile);
}

QString RecentPlaces::getSaveFileName(QWidget* parent,
		const QString& caption,
		const QString& dir,
		const QString& filter)
{
	return _getFileName(parent, caption, dir, filter, QFileDialog::AnyFile);
}

}