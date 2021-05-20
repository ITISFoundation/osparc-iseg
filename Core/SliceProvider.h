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

#include "iSegCore.h"

#include <cstdlib>
#include <list>
#include <stack>

namespace iseg {

class ISEG_CORE_API SliceProvider
{
public:
	SliceProvider(unsigned area1);
	~SliceProvider();
	unsigned ReturnArea() const;
	unsigned short ReturnNrslices();
	float* GiveMe();
	void Merge(SliceProvider* sp);
	void TakeBack(float* slice);

private:
	unsigned m_Area;
	std::stack<float*> m_Slicestack;
};

struct Spobj
{
	unsigned m_Area;
	SliceProvider* m_Spp;
	unsigned short m_Installnr;
};

class ISEG_CORE_API SliceProviderInstaller
{
public:
	static SliceProviderInstaller* Getinst();
	void ReturnInstance();
	SliceProvider* Install(unsigned area);
	void Uninstall(SliceProvider* sp);
	bool Unused();
	~SliceProviderInstaller();

	void Report() const;

private:
	static SliceProviderInstaller* inst;
	static unsigned short counter;
	std::list<Spobj> m_Splist;
	bool m_DeleteUnused = true;
	SliceProviderInstaller() = default;
	SliceProviderInstaller(SliceProviderInstaller const&) = delete;
	SliceProviderInstaller& operator=(SliceProviderInstaller const& rhs) = delete;
	class Waechter
	{
	public:
		~Waechter()
		{
			if (SliceProviderInstaller::inst != nullptr)
				delete SliceProviderInstaller::inst;
		}
	};
	friend class Waechter;
};

} // namespace iseg
