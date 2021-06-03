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

#include "Data/Point.h"

#include <iostream>
#include <vector>

namespace iseg {

class ISEG_CORE_API Contour
{
public:
	Contour();
	Contour(std::vector<Point>* Pt_vec);

	void AddPoint(Point p);
	void AddPoints(std::vector<Point>* Pt_vec);
	void PrintContour();
	void DougPeuck(float epsilon, bool closed);
	void Presimplify(float d, bool closed);
	unsigned int ReturnN() const;
	void ReturnContour(std::vector<Point>* Pt_vec);
	void Clear();

private:
	unsigned int m_N;
	std::vector<Point> m_Plist;
	void DougPeuckSub(float epsilon, const unsigned int p1, const unsigned int p2, std::vector<bool>* v1_p);
};

class ISEG_CORE_API Contour2
{
public:
	//abcd void doug_peuck(float epsilon,vector<Point> *Pt_vec,vector<unsigned short> *Meetings_vec,vector<Point> *Result_vec);
	void DougPeuck(float epsilon, std::vector<Point>* Pt_vec, std::vector<unsigned>* Meetings_vec, std::vector<Point>* Result_vec);

private:
	unsigned int m_N;
	unsigned int m_M;
	void DougPeuckSub(float epsilon, std::vector<Point>* Pt_vec, unsigned short p1, unsigned short p2, std::vector<bool>* v1_p);
	void DougPeuckSub2(float epsilon, std::vector<Point>* Pt_vec, unsigned short p1, unsigned short p2, std::vector<bool>* v1_p);
};
} // namespace iseg
