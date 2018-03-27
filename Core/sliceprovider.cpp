/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "sliceprovider.h"
#include "Precompiled.h"

#include <cstdlib>

using namespace std;

sliceprovider::sliceprovider(unsigned area1)
{
	area = area1;
	//for(unsigned short i=0;i<nrslices;++i){
	//	slicestack.push((T *)malloc(sizeof(T)*area));
	//}
	return;
}

sliceprovider::~sliceprovider()
{
	while (!slicestack.empty())
	{
		free(slicestack.top());
		slicestack.pop();
	}

	return;
}

float *sliceprovider::give_me()
{
	if (slicestack.empty())
	{
		return (float *)malloc(sizeof(float) * area);
	}
	else
	{
		float *result = slicestack.top();
		slicestack.pop();
		return result;
	}
}

void sliceprovider::take_back(float *slice)
{
	if (slice != NULL)
		slicestack.push(slice);
	return;
}

void sliceprovider::merge(sliceprovider *sp)
{
	if (area == sp->return_area())
	{
		while (!slicestack.empty())
			sp->take_back(give_me());
	}
	return;
}

unsigned sliceprovider::return_area() { return area; }

unsigned short sliceprovider::return_nrslices()
{
	return (unsigned short)slicestack.size();
}

sliceprovider_installer *sliceprovider_installer::inst = NULL;

unsigned short sliceprovider_installer::counter = 0;

sliceprovider_installer *sliceprovider_installer::getinst()
{
	static Waechter w;
	if (inst == NULL)
		inst = new sliceprovider_installer;

	counter++;
	return inst;
}

void sliceprovider_installer::return_instance()
{
	//	if(--counter==0) ~sliceprovider_installer();
	//	if(--counter==0) delete this;
	--counter;
	return;
}

bool sliceprovider_installer::unused() { return counter == 0; }

sliceprovider *sliceprovider_installer::install(unsigned area1)
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
		spobj1.spp = new sliceprovider(area1);
		splist.push_back(spobj1);
		return spobj1.spp;
	}
}

void sliceprovider_installer::uninstall(sliceprovider *sp)
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
	return;
}

sliceprovider_installer::~sliceprovider_installer()
{
	for (list<spobj>::iterator it = splist.begin(); it != splist.end(); it++)
	{
		free(it->spp);
	}

	inst = NULL;

	return;
}

void sliceprovider_installer::report() const
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
