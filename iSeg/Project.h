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

#include <QAction>
#include <QString>

#include <array>

namespace iseg {

class Project
{
public:
	Project();

	void InsertProjectFileName(const QString& name)
	{
		if (std::none_of(m_RecentProjectFileNames.begin(), m_RecentProjectFileNames.end(), [&](const QString& v) { return v == name; }))
		{
			// rotation to the right (move the last item to the first position)
			std::rotate(m_RecentProjectFileNames.rbegin(), m_RecentProjectFileNames.rbegin() + 1, m_RecentProjectFileNames.rend());
			m_RecentProjectFileNames[0] = name;
		}
		else
		{
			// moves the file back to the front, does not preserve order
			for (int i = 1; i < 4; ++i)
			{
				if (m_RecentProjectFileNames[i] == name)
				{
					m_RecentProjectFileNames[i].swap(m_RecentProjectFileNames[0]);
					break;
				}
			}
		}
	}

	QString m_CurrentFilename;

	std::array<QString, 4> m_RecentProjectFileNames{};
	std::array<QAction*, 4> m_LoadRecentProjects{};
	QAction* m_Separator = nullptr;
};

} // namespace iseg

#endif // PROJECT_H
