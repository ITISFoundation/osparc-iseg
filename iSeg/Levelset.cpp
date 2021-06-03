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

#include "Levelset.h"
#include "bmp_read_1.h"

namespace iseg {

Levelset::Levelset()
{
	m_Loaded = false;
	m_Image = new Bmphandler;
}

void Levelset::Init(unsigned short h, unsigned short w, float* levlset, float* kbit, float* Pbit, float balloon, float epsilon1, float step_size)
{
	m_Width = w;
	m_Height = h;
	m_Area = unsigned(m_Width) * m_Height;

	if (!m_Image->Isloaded())
		m_Image->Newbmp(w, h);

	m_SliceprovideInstaller = SliceProviderInstaller::Getinst();
	m_Sliceprovide = m_SliceprovideInstaller->Install(m_Area);

	//	levset=levlset;
	m_Image->SetWork(levlset, 1);
	//	image.SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testdump.bmp");

	m_Devx = m_Sliceprovide->GiveMe();
	m_Devy = m_Sliceprovide->GiveMe();
	m_Devxx = m_Sliceprovide->GiveMe();
	m_Devxy = m_Sliceprovide->GiveMe();
	m_Devyy = m_Sliceprovide->GiveMe();

	for (unsigned i = 0; i < m_Area; ++i)
	{
		//		devxx[i]=devxy[i]=devyy[i]=0;
		m_Devx[i] = m_Devy[i] = m_Devxx[i] = m_Devxy[i] = m_Devyy[i] = 0;
	}
	/*	kbits=sliceprovide->give_me();
	Pbits=sliceprovide->give_me();*/
	m_Px = m_Sliceprovide->GiveMe();
	m_Py = m_Sliceprovide->GiveMe();
	SetK(kbit);
	SetP(Pbit);
	m_Epsilon = epsilon1;
	m_Balloon1 = balloon;
	m_Stepsize = step_size;
	m_Loaded = true;
}

void Levelset::Init(unsigned short h, unsigned short w, float* initial, float f, float* kbit, float* Pbit, float balloon, float epsilon1, float step_size)
{
	m_Width = w;
	m_Height = h;
	m_Area = unsigned(m_Width) * m_Height;

	m_Image->Newbmp(w, h);
	m_Image->Copy2bmp(initial, 1);
	m_Image->DeadReckoning(f);

	//	float *dummy=image->copy_work();
	//	ownlvlset=true;
	//	init(h, w, dummy, kbit, Pbit, balloon, epsilon1, step_size);
	Init(h, w, m_Image->ReturnWork(), kbit, Pbit, balloon, epsilon1, step_size);
}

void Levelset::Init(unsigned short h, unsigned short w, Point p, float* kbit, float* Pbit, float balloon, float epsilon1, float step_size)
{
	m_Width = w;
	m_Height = h;
	m_Area = unsigned(m_Width) * m_Height;
	float px = p.px;
	float py = p.py;

	float* levlset = (float*)malloc(sizeof(float) * m_Area);
	//	ownlvlset=true;
	unsigned n = 0;
	for (short i = 0; i < m_Height; i++)
	{
		for (short j = 0; j < m_Width; j++)
		{
			levlset[n] = -sqrt((px - j) * (px - j) + (py - i) * (py - i)) + 1;
			n++;
		}
	}

	Init(h, w, levlset, kbit, Pbit, balloon, epsilon1, step_size);
}

void Levelset::Iterate(unsigned nrsteps, unsigned updatefreq)
{
	for (unsigned i = 1; i <= nrsteps; ++i)
	{
		MakeStep();
		//		cout << i;
		if (i % updatefreq == 0 && updatefreq > 0)
			Reinitialize();
		//		cout << ".";
	}

	}

void Levelset::SetK(float* kbit)
{
	m_Kbits = kbit;
}

void Levelset::SetP(float* Pbit)
{
	m_Pbits = Pbit;
	Diffx(m_Pbits, m_Px);
	Diffy(m_Pbits, m_Py);
}

void Levelset::Reinitialize()
{
	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 0;
	m_Image->SwapBmpwork();
	//	image.copy2bmp(levset);

	m_Image->Threshold(thresh);
	m_Image->SwapBmpwork();
	m_Image->DeadReckoning(255);
}

void Levelset::MakeStep()
{
	float* levset;
	levset = m_Image->ReturnWork();
	Diffx(levset, m_Devx);
	Diffy(levset, m_Devy);
	if (m_Epsilon != 0)
	{
		Diffxx(levset, m_Devxx);
		Diffxy(levset, m_Devxy);
		Diffyy(levset, m_Devyy);
	}

	float dummy, dummy1;

	for (unsigned i = 0; i < m_Area; i++)
	{
		dummy = m_Devx[i] * m_Devx[i] + m_Devy[i] * m_Devy[i];
		if (dummy != 0)
		{
			dummy1 = sqrt(dummy);
			levset[i] +=
					m_Stepsize * (m_Kbits[i] * (dummy1 * m_Balloon1 +
																				 m_Epsilon *
																						 (m_Devyy[i] * m_Devx[i] * m_Devx[i] -
																								 2 * m_Devx[i] * m_Devy[i] * m_Devxy[i] +
																								 m_Devxx[i] * m_Devy[i] * m_Devy[i]) /
																						 dummy) -
													 m_Px[i] * m_Devx[i] - m_Py[i] * m_Devy[i]);
		}
	}

	}

void Levelset::Diffx(float* input, float* output) const
{
	unsigned k = 1;

	for (unsigned short j = 0; j < m_Height; ++j)
	{
		for (unsigned short i = 1; i < m_Width - 1; ++i)
		{
			output[k] = (input[k + 1] - input[k - 1]) / 2;
			++k;
		}
		k += 2;
	}

	for (unsigned k = 0; k < m_Area; k += m_Width)
		output[k] = input[k + 1] - input[k];
	for (unsigned k = m_Width - 1; k < m_Area; k += m_Width)
		output[k] = input[k] - input[k - 1];
}

void Levelset::Diffy(float* input, float* output) const
{
	unsigned k = m_Width;
	for (unsigned short j = 1; j < m_Height - 1; ++j)
	{
		for (unsigned short i = 0; i < m_Width; ++i)
		{
			output[k] = (input[k + m_Width] - input[k - m_Width]) / 2;
			++k;
		}
	}

	for (unsigned k = 0; k < m_Width; k++)
		output[k] = input[k + m_Width] - input[k];
	for (unsigned k = m_Area - m_Width; k < m_Area; k++)
		output[k] = input[k] - input[k - m_Width];
}

void Levelset::Diffxx(float* input, float* output) const
{
	unsigned k = 1;
	for (unsigned short j = 0; j < m_Height; ++j)
	{
		for (unsigned short i = 1; i < m_Width - 1; ++i)
		{
			output[k] = input[k + 1] - input[k] + input[k - 1];
			++k;
		}
		k += 2;
	}

	for (unsigned k = 0; k < m_Area; k += m_Width)
		output[k] = 0;
	for (unsigned k = m_Width - 1; k < m_Area; k += m_Width)
		output[k] = 0;
}

void Levelset::Diffxy(float* input, float* output) const
{
	unsigned k = m_Width + 1;
	for (unsigned short j = 1; j < m_Height - 1; ++j)
	{
		for (unsigned short i = 1; i < m_Width - 1; ++i)
		{
			output[k] = (input[k + 1 + m_Width] - input[k - 1 + m_Width] -
											input[k + 1 - m_Width] + input[k - m_Width - 1]) /
									2;
			++k;
		}
		k += 2;
	}

	for (unsigned k = 0; k < m_Area; k += m_Width)
		output[k] = 0;
	for (unsigned k = m_Width - 1; k < m_Area; k += m_Width)
		output[k] = 0;
	for (unsigned k = 1; k < m_Width; k++)
		output[k] = 0;
	for (unsigned k = m_Area - m_Width + 1; k < m_Area; k++)
		output[k] = 0;
}

void Levelset::Diffyy(float* input, float* output) const
{
	unsigned k = m_Width;
	for (unsigned short j = 1; j < m_Height - 1; ++j)
	{
		for (unsigned short i = 0; i < m_Width; ++i)
		{
			output[k] = input[k + m_Width] - input[k] + input[k - m_Width];
			++k;
		}
	}

	for (unsigned k = 0; k < m_Width; k++)
		output[k] = 0;
	for (unsigned k = m_Area - m_Width; k < m_Area; k++)
		output[k] = 0;
}

void Levelset::ReturnLevelset(float* output)
{
	//	image->SaveWorkBitmap("D:\\Development\\segmentation\\sample images\\testdump.bmp");

	/*	Bmphandler image2;
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

	float* levset = m_Image->ReturnWork();
	for (unsigned i = 0; i < m_Area; ++i)
		output[i] = levset[i];
}

void Levelset::ReturnZerolevelset(std::vector<std::vector<Point>>* v1, std::vector<std::vector<Point>>* v2, int minsize)
{
	m_Image->SwapBmpwork();

	float thresh[2];
	thresh[0] = 1;
	thresh[1] = 0;
	m_Image->Threshold(thresh);

	v1->clear();
	v2->clear();

	m_Image->GetContours(255.0f, v1, v2, minsize);
	m_Image->SwapBmpwork();
}

Levelset::~Levelset()
{
	if (m_Loaded)
	{
		//		sliceprovide->take_back(levset);
		m_Sliceprovide->TakeBack(m_Devx);
		m_Sliceprovide->TakeBack(m_Devy);
		m_Sliceprovide->TakeBack(m_Devxx);
		m_Sliceprovide->TakeBack(m_Devxy);
		m_Sliceprovide->TakeBack(m_Devyy);
		m_Sliceprovide->TakeBack(m_Px);
		m_Sliceprovide->TakeBack(m_Py);
		//		if(ownlvlset)
		//			sliceprovide->take_back(levset);

		//		if(ownsliceprovider)
		m_SliceprovideInstaller->Uninstall(m_Sliceprovide);
		//		free(bmpinfo);
	}

	m_SliceprovideInstaller->ReturnInstance();
	if (m_SliceprovideInstaller->Unused())
		delete m_SliceprovideInstaller;

	delete m_Image;
}

} // namespace iseg
