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

#include "Data/Point.h"

#include "Core/SliceProvider.h"

#include <vector>

namespace iseg {

class Bmphandler;

class Levelset
{
public:
	Levelset();
	void Init(unsigned short h, unsigned short w, Point p, float* kbit, float* Pbit, float balloon, float epsilon1, float step_size);
	void Init(unsigned short h, unsigned short w, float* initial, float f, float* kbit, float* Pbit, float balloon, float epsilon1, float step_size);
	void Init(unsigned short h, unsigned short w, float* levlset, float* kbit, float* Pbit, float balloon, float epsilon1, float step_size);
	void Iterate(unsigned nrsteps, unsigned updatefreq);
	void SetK(float* kbit);
	void SetP(float* Pbit);
	void ReturnLevelset(float* output);
	void ReturnZerolevelset(std::vector<std::vector<Point>>* v1, std::vector<std::vector<Point>>* v2, int minsize);
	~Levelset();

private:
	bool m_Loaded;
	Bmphandler* m_Image;
	void Reinitialize();
	void MakeStep();
	float m_Stepsize;
	float m_Epsilon;
	float m_Balloon1;
	unsigned short m_Width;
	unsigned short m_Height;
	unsigned m_Area;
	//		float *levset;
	float* m_Devx;
	float* m_Devy;
	float* m_Devxx;
	float* m_Devxy;
	float* m_Devyy;
	float* m_Kbits;
	float* m_Pbits;
	float* m_Px;
	float* m_Py;
	void Diffx(float* input, float* output) const;
	void Diffy(float* input, float* output) const;
	void Diffxx(float* input, float* output) const;
	void Diffxy(float* input, float* output) const;
	void Diffyy(float* input, float* output) const;
	SliceProvider* m_Sliceprovide;
	SliceProviderInstaller* m_SliceprovideInstaller;
};

} // namespace iseg
