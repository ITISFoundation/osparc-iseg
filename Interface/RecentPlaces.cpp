/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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

std::deque<QString> list;

QString _getFileName(QWidget* parent, const QString& caption, const QString& dir, const QString& filter, QFileDialog::AcceptMode mode)
{
	QString last_dir = !dir.isEmpty() ? dir : RecentPlaces::LastDirectory();
	QList<QUrl> urls;
	for (const auto& d : RecentPlaces::RecentDirectories())
	{
		urls << QUrl::fromLocalFile(d);
	}

	QFileDialog dialog(parent);
	dialog.setAcceptMode(mode);
	dialog.setFileMode(mode == QFileDialog::AcceptOpen ? QFileDialog::ExistingFile : QFileDialog::AnyFile);
	dialog.setWindowTitle(caption);
	dialog.setFilter(filter);
	dialog.setDirectory(last_dir);
	dialog.setSidebarUrls(urls);
	if (dialog.exec() == QDialog::Accepted)
	{
		auto file_path = dialog.selectedFiles().value(0);
		if (QFileInfo(file_path).completeSuffix().isEmpty() && !dialog.selectedNameFilter().isEmpty())
		{
			// append extension
			auto filter = dialog.selectedNameFilter().toStdString();
			auto pos0 = filter.find_first_of('*') + 1;
			if (std::string::npos != pos0)
			{
				auto pos1 = std::min(filter.find_first_of(' ', pos0), filter.find_first_of(')', pos0));
				if (std::string::npos != pos1)
				{
					file_path += QString::fromStdString(filter.substr(pos0, pos1 - pos0));
				}
			}
		}

		RecentPlaces::AddRecent(file_path);
		return file_path;
	}
	return QString();
}

} // namespace

void RecentPlaces::AddRecent(const QString& path)
{
	if (path.isEmpty())
		return;

	// get directory
	// QFileInfo::absolutePath: Returns a file's path absolute path. This doesn't include the file name.
	QFileInfo fi(path);
	QString dir = (fi.isDir() == false) ? fi.absolutePath() : path;

	// push to top
	list.push_front(dir);

	// remove duplicates (keep first)
	{
		auto end = list.end();
		for (auto it = list.begin(); it != end; ++it)
		{
			end = std::remove(it + 1, end, *it);
		}
		list.erase(end, list.end());
	}

	// keep only last N results
	const size_t num_places = 5;
	if (list.size() > num_places)
	{
		list.resize(num_places);
	}
}

QString RecentPlaces::LastDirectory()
{
	if (!list.empty())
		return list.front();
	return QString::null;
}

std::deque<QString> RecentPlaces::RecentDirectories()
{
	return list;
}

QString RecentPlaces::GetOpenFileName(QWidget* parent, const QString& caption, const QString& dir, const QString& filter)
{
	return _getFileName(parent, caption, dir, filter, QFileDialog::AcceptOpen);
}

QString RecentPlaces::GetSaveFileName(QWidget* parent, const QString& caption, const QString& dir, const QString& filter)
{
	return _getFileName(parent, caption, dir, filter, QFileDialog::AcceptSave);
}

} // namespace iseg
