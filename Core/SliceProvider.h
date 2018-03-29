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

#include "iSegCore.h"

#include <cstdlib>
#include <list>
#include <stack>

namespace iseg {

class iSegCore_API SliceProvider
{
public:
	SliceProvider(unsigned area1);
	~SliceProvider();
	unsigned return_area();
	unsigned short return_nrslices();
	float* give_me();
	void merge(SliceProvider* sp);
	void take_back(float* slice);

private:
	unsigned area;
	std::stack<float*> slicestack;
};

struct spobj
{
	unsigned area;
	SliceProvider* spp;
	unsigned short installnr;
};

class iSegCore_API SliceProviderInstaller
{
public:
	static SliceProviderInstaller* getinst();
	void return_instance();
	SliceProvider* install(unsigned area);
	void uninstall(SliceProvider* sp);
	bool unused();
	~SliceProviderInstaller();

	void report() const;

private:
	static SliceProviderInstaller* inst;
	static unsigned short counter;
	std::list<spobj> splist;
	bool delete_unused = true;
	SliceProviderInstaller(){};
	SliceProviderInstaller(SliceProviderInstaller const&);
	SliceProviderInstaller& operator=(SliceProviderInstaller const& rhs);
	class Waechter
	{
	public:
		~Waechter()
		{
			if (SliceProviderInstaller::inst != NULL)
				delete SliceProviderInstaller::inst;
		}
	};
	friend class Waechter;
};

} // namespace iseg
