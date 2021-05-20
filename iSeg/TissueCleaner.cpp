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

#include "TissueCleaner.h"
#include "TissueInfos.h"

#include <cstdlib>

namespace iseg {

int TissueCleaner::BaseConnection(int c)
{
	if (c == m_Map[c])
		return c;
	else
		return m_Map[c] = BaseConnection(m_Map[c]);
}

TissueCleaner::TissueCleaner(tissues_size_t** slices1, unsigned short n1, unsigned short width1, unsigned short height1)
{
	m_Slices = slices1;
	m_Nrslices = static_cast<size_t>(n1);
	m_Width = static_cast<size_t>(width1);
	m_Height = static_cast<size_t>(height1);
	m_Volume = nullptr;
	m_Map.clear();
}

TissueCleaner::~TissueCleaner() { free(m_Volume); }

bool TissueCleaner::Allocate()
{
	m_Volume = (int*)malloc(sizeof(int) * m_Width * m_Height * m_Nrslices);
	if (m_Volume == nullptr)
		return false;
	return true;
}

void TissueCleaner::ConnectedComponents()
{
	m_Map.clear();
	int newest = 0;

	size_t offset[4];
	offset[0] = 1;
	offset[1] = m_Width;
	offset[2] = m_Width * m_Height;
	offset[3] = offset[2] * m_Nrslices;

	size_t i1 = 0;
	size_t i2 = 0;

	// init first pixel
	m_Map.push_back(newest);
	m_Tissuemap.push_back(m_Slices[0][i1]);
	m_Volume[i1] = newest;
	newest++;
	i1++;
	// init first row
	for (unsigned short k = 1; k < m_Width; k++)
	{
		if (m_Slices[0][i1] == m_Slices[0][i1 - 1])
		{
			m_Volume[i1] = BaseConnection(m_Volume[i1 - 1]);
		}
		else
		{
			m_Map.push_back(newest);
			m_Tissuemap.push_back(m_Slices[0][i1]);
			m_Volume[i1] = newest;
			newest++;
		}
		i1++;
	}
	// init first slice
	for (unsigned short j = 1; j < m_Height; j++)
	{
		if (m_Slices[0][i1] == m_Slices[0][i1 - m_Width])
		{
			m_Volume[i1] = BaseConnection(m_Volume[i1 - m_Width]);
		}
		else
		{
			m_Map.push_back(newest);
			m_Tissuemap.push_back(m_Slices[0][i1]);
			m_Volume[i1] = newest;
			newest++;
		}
		i1++;
		for (unsigned short k = 1; k < m_Width; k++)
		{
			if (m_Slices[0][i1] == m_Slices[0][i1 - 1])
			{
				m_Volume[i1] = BaseConnection(m_Volume[i1 - 1]);
				if (m_Slices[0][i1] == m_Slices[0][i1 - m_Width])
				{
					m_Map[(int)BaseConnection(m_Volume[i1 - m_Width])] = m_Volume[i1];
				}
			}
			else
			{
				if (m_Slices[0][i1] == m_Slices[0][i1 - m_Width])
				{
					m_Volume[i1] = BaseConnection(m_Volume[i1 - m_Width]);
				}
				else
				{
					m_Map.push_back(newest);
					m_Tissuemap.push_back(m_Slices[0][i1]);
					m_Volume[i1] = newest;
					newest++;
				}
			}
			i1++;
		}
	}
	// for each slice >= 1
	for (unsigned short i = 1; i < m_Nrslices; i++)
	{
		i2 = 0;
		if (m_Slices[i][i2] == m_Slices[i - 1][i2])
			m_Volume[i1] = BaseConnection(m_Volume[i1 - offset[2]]);
		else
		{
			m_Map.push_back(newest);
			m_Tissuemap.push_back(m_Slices[i][i2]);
			m_Volume[i1] = newest;
			newest++;
		}
		i1++;
		i2++;
		for (unsigned short k = 1; k < m_Width; k++)
		{
			if (m_Slices[i][i2] == m_Slices[i][i2 - 1])
			{
				m_Volume[i1] = BaseConnection(m_Volume[i1 - 1]);
				if (m_Slices[i][i2] == m_Slices[i - 1][i2])
				{
					m_Map[(int)BaseConnection(m_Volume[i1 - offset[2]])] =
							m_Volume[i1];
				}
			}
			else
			{
				if (m_Slices[i][i2] == m_Slices[i - 1][i2])
					m_Volume[i1] = BaseConnection(m_Volume[i1 - offset[2]]);
				else
				{
					m_Map.push_back(newest);
					m_Tissuemap.push_back(m_Slices[i][i2]);
					m_Volume[i1] = newest;
					newest++;
				}
			}
			i1++;
			i2++;
		}
		for (unsigned short j = 1; j < m_Height; j++)
		{
			if (m_Slices[i][i2] == m_Slices[i][i2 - m_Width])
			{
				m_Volume[i1] = BaseConnection(m_Volume[i1 - m_Width]);
				if (m_Slices[i][i2] == m_Slices[i - 1][i2])
				{
					m_Map[(int)BaseConnection(m_Volume[i1 - offset[2]])] =
							m_Volume[i1];
				}
			}
			else
			{
				if (m_Slices[i][i2] == m_Slices[i - 1][i2])
					m_Volume[i1] = BaseConnection(m_Volume[i1 - offset[2]]);
				else
				{
					m_Map.push_back(newest);
					m_Tissuemap.push_back(m_Slices[i][i2]);
					m_Volume[i1] = newest;
					newest++;
				}
			}
			i1++;
			i2++;
			for (unsigned short k = 1; k < m_Width; k++)
			{
				if (m_Slices[i][i2] == m_Slices[i][i2 - 1])
				{
					m_Volume[i1] = BaseConnection(m_Volume[i1 - 1]);
					if (m_Slices[i][i2] == m_Slices[i][i2 - m_Width])
					{
						m_Map[(int)BaseConnection(m_Volume[i1 - m_Width])] =
								m_Volume[i1];
					}
					if (m_Slices[i][i2] == m_Slices[i - 1][i2])
					{
						m_Map[(int)BaseConnection(m_Volume[i1 - offset[2]])] =
								m_Volume[i1];
					}
				}
				else
				{
					if (m_Slices[i][i2] == m_Slices[i][i2 - m_Width])
					{
						m_Volume[i1] = BaseConnection(m_Volume[i1 - m_Width]);
						if (m_Slices[i][i2] == m_Slices[i - 1][i2])
						{
							m_Map[(int)BaseConnection(m_Volume[i1 - offset[2]])] =
									m_Volume[i1];
						}
					}
					else
					{
						if (m_Slices[i][i2] == m_Slices[i - 1][i2])
							m_Volume[i1] =
									BaseConnection(m_Volume[i1 - offset[2]]);
						else
						{
							m_Map.push_back(newest);
							m_Tissuemap.push_back(m_Slices[i][i2]);
							m_Volume[i1] = newest;
							newest++;
						}
					}
				}
				i1++;
				i2++;
			}
		}
	}

	// map connected regions to single ID
	for (size_t i = 0; i < offset[3]; i++)
		m_Volume[i] = BaseConnection(m_Volume[i]);
}

void TissueCleaner::MakeStat()
{
	for (unsigned i = 0; i < TISSUES_SIZE_MAX + 1; i++)
		m_Totvolumes[i] = 0;
	m_Volumes.resize(m_Map.size(), 0);
	if (m_Volume == nullptr)
		return;
	size_t maxi = m_Nrslices * (size_t)m_Width * size_t(m_Height);
	for (unsigned long long i = 0; i < maxi; i++)
	{
		m_Volumes[m_Volume[i]]++;
	}
	for (size_t i = 0; i < m_Map.size(); i++)
	{
		if (m_Map[i] == i)
			m_Totvolumes[m_Tissuemap[i]] += m_Volumes[i];
	}
}

void TissueCleaner::Clean(float ratio, unsigned minsize)
{
	if (m_Volume == nullptr)
		return;
	std::vector<bool> erasemap;
	erasemap.resize(m_Map.size(), false);
	for (size_t i = 0; i < m_Map.size(); i++)
	{
		if (m_Volumes[i] < minsize && m_Volumes[i] < ratio * m_Totvolumes[m_Tissuemap[i]])
		{
			// only remove small components if tissue is NOT locked!
			if (!TissueInfos::GetTissueLocked(m_Tissuemap[i]))
			{
				erasemap[i] = true;
			}
		}
	}

	tissues_size_t curchar = 0;
	size_t postot = 0;
	for (unsigned short i = 0; i < m_Nrslices; i++)
	{
		size_t pos = 0;
		for (unsigned short j = 0; j < m_Height; j++)
		{
			unsigned short k = 0;
			while (k < m_Width && erasemap[m_Volume[postot]])
			{
				postot++;
				k++;
			}
			if (k == m_Width)
			{
				pos += m_Width;
				//xxxa I should treat this case, but it is unlikely and I am tired...
			}
			else
			{
				size_t pos1 = pos + k;
				curchar = m_Slices[i][pos1];
				for (; pos < pos1; pos++)
					m_Slices[i][pos] = curchar;
				for (; k < m_Width; k++, pos++, postot++)
				{
					if (erasemap[m_Volume[postot]])
						m_Slices[i][pos] = curchar;
					else
						curchar = m_Slices[i][pos];
				}
			}
		}
	}
}

} // namespace iseg
