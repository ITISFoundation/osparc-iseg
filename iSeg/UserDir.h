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

#include <QDir>

namespace iseg {

QDir TempDir()
{
	QDir tmpdir = QDir::home();
	if (!tmpdir.exists("iSeg"))
	{
		std::cerr << "iSeg folder does not exist, creating...\n";
		if (!tmpdir.mkdir(QString("iSeg")))
		{
			std::cerr << "failed to create iSeg folder, exiting...\n";
			exit(EXIT_FAILURE);
		}
	}
	if (!tmpdir.cd("iSeg"))
	{
		std::cerr << "failed to enter iSeg folder, exiting...\n";
		exit(EXIT_FAILURE);
	}

	if (!tmpdir.exists("tmp"))
	{
		std::cerr << "tmp folder does not exist, creating...\n";
		if (!tmpdir.mkdir(QString("tmp")))
		{
			std::cerr << "failed to create tmp folder, exiting...\n";
			exit(EXIT_FAILURE);
		}
	}
	if (!tmpdir.cd("tmp"))
	{
		std::cerr << "failed to enter tmp folder, exiting...\n";
		exit(EXIT_FAILURE);
	}
	return tmpdir;
}
} // namespace iseg