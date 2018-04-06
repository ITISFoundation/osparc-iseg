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

#include "bmp_read_1.h"
#include "Levelset.h"

using namespace std;
using namespace iseg;

Levelset::Levelset()
{
	loaded = false;
	image = new bmphandler;
	//	ownlvlset=false;
	return;
}

void Levelset::init(unsigned short h, unsigned short w, float* levlset,
					float* kbit, float* Pbit, float balloon, float epsilon1,
					float step_size)
{
	width = w;
	height = h;
	area = unsigned(width) * height;

	if (!image->isloaded())
		image->newbmp(w, h);

	sliceprovide_installer = SliceProviderInstaller::getinst();
	sliceprovide = sliceprovide_installer->install(area);

	//	levset=levlset;
	image->set_work(levlset, 1);
	//	image.SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testdump.bmp");

	devx = sliceprovide->give_me();
	devy = sliceprovide->give_me();
	devxx = sliceprovide->give_me();
	devxy = sliceprovide->give_me();
	devyy = sliceprovide->give_me();

	for (unsigned i = 0; i < area; ++i)
	{
		//		devxx[i]=devxy[i]=devyy[i]=0;
		devx[i] = devy[i] = devxx[i] = devxy[i] = devyy[i] = 0;
	}
	/*	kbits=sliceprovide->give_me();
	Pbits=sliceprovide->give_me();*/
	Px = sliceprovide->give_me();
	Py = sliceprovide->give_me();
	set_k(kbit);
	set_P(Pbit);
	epsilon = epsilon1;
	balloon1 = balloon;
	stepsize = step_size;
	loaded = true;

	return;
}

void Levelset::init(unsigned short h, unsigned short w, float* initial, float f,
					float* kbit, float* Pbit, float balloon, float epsilon1,
					float step_size)
{
	width = w;
	height = h;
	area = unsigned(width) * height;

	image->newbmp(w, h);
	image->copy2bmp(initial, 1);
	image->dead_reckoning(f);

	//	float *dummy=image->copy_work();
	//	ownlvlset=true;
	//	init(h, w, dummy, kbit, Pbit, balloon, epsilon1, step_size);
	init(h, w, image->return_work(), kbit, Pbit, balloon, epsilon1, step_size);

	return;
}

void Levelset::init(unsigned short h, unsigned short w, Point p, float* kbit,
					float* Pbit, float balloon, float epsilon1, float step_size)
{
	width = w;
	height = h;
	area = unsigned(width) * height;
	float px = p.px;
	float py = p.py;

	float* levlset = (float*)malloc(sizeof(float) * area);
	//	ownlvlset=true;
	unsigned n = 0;
	for (short i = 0; i < height; i++)
	{
		for (short j = 0; j < width; j++)
		{
			levlset[n] = -sqrt((px - j) * (px - j) + (py - i) * (py - i)) + 1;
			n++;
		}
	}

	init(h, w, levlset, kbit, Pbit, balloon, epsilon1, step_size);
	/*	Pair p1;
	image->copy2work(levlset);
	image->get_range(&p1);
	image->scale_colors(p1);
	image->SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testt.bmp");*/

	return;
}

void Levelset::iterate(unsigned nrsteps, unsigned updatefreq)
{
	for (unsigned i = 1; i <= nrsteps; ++i)
	{
		make_step();
		//		cout << i;
		if (i % updatefreq == 0 && updatefreq > 0)
			reinitialize();
		//		cout << ".";
	}

	return;
}

void Levelset::set_k(float* kbit)
{
	kbits = kbit;
	return;
}

void Levelset::set_P(float* Pbit)
{
	Pbits = Pbit;
	diffx(Pbits, Px);
	diffy(Pbits, Py);

	/*	bmphandler image2;
	Pair p;
	image2.newbmp(512,512);
	image2.copy2work(Pbits);
	image2.get_range(&p);
	image2.scale_colors(p);
	image2.SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testp.bmp");	
	image2.copy2work(Px);
	image2.get_range(&p);
	cout << p.low << " " << p.high << endl;
	image2.scale_colors(p);
	image2.SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testpx.bmp");
	image2.copy2work(Py);
	image2.SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testpy.bmp");*/

	return;
}

void Levelset::reinitialize()
{
	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 0;
	image->swap_bmpwork();
	//	image.copy2bmp(levset);

	image->threshold(thresh);
	image->swap_bmpwork();
	image->dead_reckoning(255);
	//	image->IFT_distance1(255);
	/*	Pair p,p1;
	image->get_range(&p);
//	image->bmp_add(200);
//	image->swap_bmpwork();
//	image->sobel();
	p1.low=-10;
	p1.high=0;
	image->scale_colors(p1);
	image->SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testt.bmp");
	p1.low=255;
	p1.high=265*25.5f;
	image->scale_colors(p1);
//	image->bmp_add(-200);*/
	//	levset=image->return_work();
	//	image->copy_work(levset);
	//	levset=image->return_work();
	/*	image->bmp_add(250);
	image->SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testt.bmp");
	image->bmp_add(-250);*/

	return;
}

void Levelset::make_step()
{
	float* levset;
	levset = image->return_work();
	diffx(levset, devx);
	diffy(levset, devy);
	if (epsilon != 0)
	{
		diffxx(levset, devxx);
		diffxy(levset, devxy);
		diffyy(levset, devyy);
	}

	float dummy, dummy1;

	for (unsigned i = 0; i < area; i++)
	{
		dummy = devx[i] * devx[i] + devy[i] * devy[i];
		if (dummy != 0)
		{
			dummy1 = sqrt(dummy);
			levset[i] +=
				stepsize * (kbits[i] * (dummy1 * balloon1 +
										epsilon *
											(devyy[i] * devx[i] * devx[i] -
											 2 * devx[i] * devy[i] * devxy[i] +
											 devxx[i] * devy[i] * devy[i]) /
											dummy) -
							Px[i] * devx[i] - Py[i] * devy[i]);
		}
	}

	return;
}

void Levelset::diffx(float* input, float* output)
{
	unsigned k = 1;

	for (unsigned short j = 0; j < height; ++j)
	{
		for (unsigned short i = 1; i < width - 1; ++i)
		{
			output[k] = (input[k + 1] - input[k - 1]) / 2;
			++k;
		}
		k += 2;
	}

	for (unsigned k = 0; k < area; k += width)
		output[k] = input[k + 1] - input[k];
	for (unsigned k = width - 1; k < area; k += width)
		output[k] = input[k] - input[k - 1];

	return;
}

void Levelset::diffy(float* input, float* output)
{
	unsigned k = width;
	for (unsigned short j = 1; j < height - 1; ++j)
	{
		for (unsigned short i = 0; i < width; ++i)
		{
			output[k] = (input[k + width] - input[k - width]) / 2;
			++k;
		}
	}

	for (unsigned k = 0; k < width; k++)
		output[k] = input[k + width] - input[k];
	for (unsigned k = area - width; k < area; k++)
		output[k] = input[k] - input[k - width];

	return;
}

void Levelset::diffxx(float* input, float* output)
{
	unsigned k = 1;
	for (unsigned short j = 0; j < height; ++j)
	{
		for (unsigned short i = 1; i < width - 1; ++i)
		{
			output[k] = input[k + 1] - input[k] + input[k - 1];
			++k;
		}
		k += 2;
	}

	for (unsigned k = 0; k < area; k += width)
		output[k] = 0;
	for (unsigned k = width - 1; k < area; k += width)
		output[k] = 0;

	return;
}

void Levelset::diffxy(float* input, float* output)
{
	unsigned k = width + 1;
	for (unsigned short j = 1; j < height - 1; ++j)
	{
		for (unsigned short i = 1; i < width - 1; ++i)
		{
			output[k] = (input[k + 1 + width] - input[k - 1 + width] -
						 input[k + 1 - width] + input[k - width - 1]) /
						2;
			++k;
		}
		k += 2;
	}

	for (unsigned k = 0; k < area; k += width)
		output[k] = 0;
	for (unsigned k = width - 1; k < area; k += width)
		output[k] = 0;
	for (unsigned k = 1; k < width; k++)
		output[k] = 0;
	for (unsigned k = area - width + 1; k < area; k++)
		output[k] = 0;

	return;
}

void Levelset::diffyy(float* input, float* output)
{
	unsigned k = width;
	for (unsigned short j = 1; j < height - 1; ++j)
	{
		for (unsigned short i = 0; i < width; ++i)
		{
			output[k] = input[k + width] - input[k] + input[k - width];
			++k;
		}
	}

	for (unsigned k = 0; k < width; k++)
		output[k] = 0;
	for (unsigned k = area - width; k < area; k++)
		output[k] = 0;

	return;
}

void Levelset::return_levelset(float* output)
{
	//	image->SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testdump.bmp");

	/*	bmphandler image2;
	Pair p;
	float *bbmmpp=(float *)malloc(sizeof(float)*area);
	image2.newbmp(512,512);
	float dummy,dummy1;

	float maxdx=devx[0];
	float maxdy=devy[0];
	float maxPx=Px[0];
	float maxPy=Py[0];
	float maxbbmmpp=-Px[0]*devx[0]-Py[0]*devy[0];

	for(unsigned i=0;i<area;i++){
		dummy=devx[i]*devx[i]+devy[i]*devy[i];
		if(dummy!=0){
			dummy1=sqrt(dummy);
			bbmmpp[i]=stepsize*(kbits[i]*(dummy1*balloon1+epsilon*(devyy[i]*devx[i]*devx[i]-2*devx[i]*devy[i]*devxy[i]+devxx[i]*devy[i]*devy[i])/dummy)-Px[i]*devx[i]-Py[i]*devy[i]);
			maxdx=max(devx[i],maxdx);
			maxdy=max(devy[i],maxdy);
			maxPx=max(Px[i],maxPx);
			maxPy=max(Py[i],maxPy);
			maxbbmmpp=max(maxbbmmpp,bbmmpp[i]);
			if(bbmmpp[i]>206270) cout << ";" <<kbits[i]<<";"<<(dummy1*balloon1+epsilon*(devyy[i]*devx[i]*devx[i]-2*devx[i]*devy[i]*devxy[i]+devxx[i]*devy[i]*devy[i])/dummy)<<";"<<-Px[i]*devx[i]-Py[i]*devy[i]<<";"<<Px[i]<<";"<<Py[i]<<";"<<devx[i]<<";"<<devy[i]<<";"<<i<<endl;
		}
	}
	cout << maxdx<<"," <<maxdy<<"," << maxPx<<","<<maxPy<<","<<maxbbmmpp<<endl;
	image2.copy2work(bbmmpp);
	image2.get_range(&p);
	cout <<"- " << p.low << " " << p.high<<endl;
	p.low=0;
	image2.scale_colors(p);
	image2.SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\teststeps.bmp");	
	free(bbmmpp);*/

	float* levset = image->return_work();
	for (unsigned i = 0; i < area; ++i)
		output[i] = levset[i];
	return;
}

void Levelset::return_zerolevelset(vector<vector<Point>>* v1,
								   vector<vector<Point>>* v2, int minsize)
{
	image->swap_bmpwork();
	//	image->swap_workhelp();
	//	image->set_bmp(levset);
	//	image->copy2bmp(levset);

	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 0;
	image->threshold(thresh);

	v1->clear();
	v2->clear();

	image->get_contours(255.0f, v1, v2, minsize);
	image->swap_bmpwork();
	//	image->swap_workhelp();

	return;
}

Levelset::~Levelset()
{
	if (loaded)
	{
		//		sliceprovide->take_back(levset);
		sliceprovide->take_back(devx);
		sliceprovide->take_back(devy);
		sliceprovide->take_back(devxx);
		sliceprovide->take_back(devxy);
		sliceprovide->take_back(devyy);
		sliceprovide->take_back(Px);
		sliceprovide->take_back(Py);
		//		if(ownlvlset)
		//			sliceprovide->take_back(levset);

		//		if(ownsliceprovider)
		sliceprovide_installer->uninstall(sliceprovide);
		//		free(bmpinfo);
	}

	sliceprovide_installer->return_instance();
	if (sliceprovide_installer->unused())
		delete sliceprovide_installer;

	delete image;

	return;
}
