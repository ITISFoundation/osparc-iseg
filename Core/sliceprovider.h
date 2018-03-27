/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SLICEPROVIDER
#define SLICEPROVIDER

#include "iSegCore.h"

#include <cstdlib>
#include <list>
#include <stack>

class iSegCore_API sliceprovider
{
public:
	sliceprovider(unsigned area1);
	~sliceprovider();
	unsigned return_area();
	unsigned short return_nrslices();
	float *give_me();
	void merge(sliceprovider *sp);
	void take_back(float *slice);

private:
	unsigned area;
	std::stack<float *> slicestack;
};

struct spobj
{
	unsigned area;
	sliceprovider *spp;
	unsigned short installnr;
};

class iSegCore_API sliceprovider_installer
{
public:
	static sliceprovider_installer *getinst();
	void return_instance();
	sliceprovider *install(unsigned area);
	void uninstall(sliceprovider *sp);
	bool unused();
	~sliceprovider_installer();

	void report() const;

private:
	static sliceprovider_installer *inst;
	static unsigned short counter;
	std::list<spobj> splist;
	bool delete_unused = true;
	sliceprovider_installer(){};
	sliceprovider_installer(sliceprovider_installer const &);
	sliceprovider_installer &operator=(sliceprovider_installer const &rhs);
	class Waechter
	{
	public:
		~Waechter()
		{
			if (sliceprovider_installer::inst != NULL)
				delete sliceprovider_installer::inst;
		}
	};
	friend class Waechter;
};

#endif
