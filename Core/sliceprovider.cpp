/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

using namespace std;
using namespace iseg;

SliceProvider::SliceProvider(unsigned area1) { area = area1; }

SliceProvider::~SliceProvider()
{
	while (!slicestack.empty())
	{
		free(slicestack.top());
		slicestack.pop();
	}
}

float* SliceProvider::give_me()
{
	if (slicestack.empty())
	{
		return (float*)malloc(sizeof(float) * area);
	}
	else
	{
		float* result = slicestack.top();
		slicestack.pop();
		return result;
	}
}

void SliceProvider::take_back(float* slice)
{
	if (slice != NULL)
		slicestack.push(slice);
}

void SliceProvider::merge(SliceProvider* sp)
{
	if (area == sp->return_area())
	{
		while (!slicestack.empty())
			sp->take_back(give_me());
	}
}

unsigned SliceProvider::return_area() { return area; }

unsigned short SliceProvider::return_nrslices()
{
	return (unsigned short)slicestack.size();
}

SliceProviderInstaller* SliceProviderInstaller::inst = NULL;

unsigned short SliceProviderInstaller::counter = 0;

SliceProviderInstaller* SliceProviderInstaller::getinst()
{
	static Waechter w;
	if (inst == NULL)
		inst = new SliceProviderInstaller;

	counter++;
	return inst;
}

void SliceProviderInstaller::return_instance()
{
	//	if(--counter==0) ~sliceprovider_installer();
	//	if(--counter==0) delete this;
	--counter;
}

bool SliceProviderInstaller::unused() { return counter == 0; }

SliceProvider* SliceProviderInstaller::install(unsigned area1)
{
	/*	FILE *fp1=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");
	fprintf(fp1,"E %u",(unsigned)&splist);
	fclose(fp1);*/

	list<spobj>::iterator it = splist.begin();

	/*	fp1=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");
	fprintf(fp1,"E1");
	fclose(fp1);*/

	while (it != splist.end() && (it->area != area1))
		it++;

	if (it != splist.end())
	{
		it->installnr++;
		return it->spp;
	}
	else
	{
		spobj spobj1;
		spobj1.installnr = 1;
		spobj1.area = area1;
		spobj1.spp = new SliceProvider(area1);
		splist.push_back(spobj1);
		return spobj1.spp;
	}
}

void SliceProviderInstaller::uninstall(SliceProvider* sp)
{
	list<spobj>::iterator it = splist.begin();
	while (it != splist.end() && (it->area != sp->return_area()))
		it++;

	if (it != splist.end())
	{
		if (--(it->installnr) == 0 && delete_unused)
		{
			delete (it->spp);
			splist.erase(it);
			return;
		}
	}
}

SliceProviderInstaller::~SliceProviderInstaller()
{
	for (list<spobj>::iterator it = splist.begin(); it != splist.end(); it++)
	{
		free(it->spp);
	}

	inst = NULL;
}

void SliceProviderInstaller::report() const
{
	std::map<int, int> area_counts;
	std::map<int, int> area_counts_empty;
	for (auto sp : splist)
	{
		if (sp.installnr)
		{
			area_counts[sp.area]++;
		}
		else if (sp.spp)
		{
			area_counts_empty[sp.area] += sp.spp->return_nrslices();
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
