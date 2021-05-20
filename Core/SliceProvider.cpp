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

#include "SliceProvider.h"

#include <cstdlib>

namespace iseg {

SliceProvider::SliceProvider(unsigned area1) { m_Area = area1; }

SliceProvider::~SliceProvider()
{
	while (!m_Slicestack.empty())
	{
		free(m_Slicestack.top());
		m_Slicestack.pop();
	}
}

float* SliceProvider::GiveMe()
{
	if (m_Slicestack.empty())
	{
		return (float*)malloc(sizeof(float) * m_Area);
	}
	else
	{
		float* result = m_Slicestack.top();
		m_Slicestack.pop();
		return result;
	}
}

void SliceProvider::TakeBack(float* slice)
{
	if (slice != nullptr)
		m_Slicestack.push(slice);
}

void SliceProvider::Merge(SliceProvider* sp)
{
	if (m_Area == sp->ReturnArea())
	{
		while (!m_Slicestack.empty())
			sp->TakeBack(GiveMe());
	}
}

unsigned SliceProvider::ReturnArea() const { return m_Area; }

unsigned short SliceProvider::ReturnNrslices()
{
	return (unsigned short)m_Slicestack.size();
}

SliceProviderInstaller* SliceProviderInstaller::inst = nullptr;

unsigned short SliceProviderInstaller::counter = 0;

SliceProviderInstaller* SliceProviderInstaller::Getinst()
{
	static Waechter w;
	if (inst == nullptr)
		inst = new SliceProviderInstaller;

	counter++;
	return inst;
}

void SliceProviderInstaller::ReturnInstance()
{
	//	if(--counter==0) ~sliceprovider_installer();
	//	if(--counter==0) delete this;
	--counter;
}

bool SliceProviderInstaller::Unused() { return counter == 0; }

SliceProvider* SliceProviderInstaller::Install(unsigned area1)
{
	auto it = m_Splist.begin();

	while (it != m_Splist.end() && (it->m_Area != area1))
		it++;

	if (it != m_Splist.end())
	{
		it->m_Installnr++;
		return it->m_Spp;
	}
	else
	{
		Spobj spobj1;
		spobj1.m_Installnr = 1;
		spobj1.m_Area = area1;
		spobj1.m_Spp = new SliceProvider(area1);
		m_Splist.push_back(spobj1);
		return spobj1.m_Spp;
	}
}

void SliceProviderInstaller::Uninstall(SliceProvider* sp)
{
	auto it = m_Splist.begin();
	while (it != m_Splist.end() && (it->m_Area != sp->ReturnArea()))
		it++;

	if (it != m_Splist.end())
	{
		if (--(it->m_Installnr) == 0 && m_DeleteUnused)
		{
			delete (it->m_Spp);
			m_Splist.erase(it);
			return;
		}
	}
}

SliceProviderInstaller::~SliceProviderInstaller()
{
	for (auto it = m_Splist.begin(); it != m_Splist.end(); it++)
	{
		free(it->m_Spp);
	}

	inst = nullptr;
}

void SliceProviderInstaller::Report() const
{
	std::map<int, int> area_counts;
	std::map<int, int> area_counts_empty;
	for (auto sp : m_Splist)
	{
		if (sp.m_Installnr)
		{
			area_counts[sp.m_Area]++;
		}
		else if (sp.m_Spp)
		{
			area_counts_empty[sp.m_Area] += sp.m_Spp->ReturnNrslices();
		}
	}

	std::cerr << "Debug:-------------------\n";
	std::cerr << "Debug: active sliceproviders\n";
	for (auto v : area_counts)
	{
		std::cerr << "area=" << v.first << " -> " << v.second << "\n";
	}
	std::cerr << "Debug: dead sliceproviders\n";
	for (auto v : area_counts_empty)
	{
		std::cerr << "area=" << v.first << " -> " << v.second << "\n";
	}
	std::cerr << "Debug:-------------------\n";
}

} // namespace iseg
