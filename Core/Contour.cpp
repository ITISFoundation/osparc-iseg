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

#include "Contour.h"

using namespace std;
using namespace iseg;

#define UNREFERENCED_PARAMETER(P) (P)

Contour::Contour()
{
	n = 0;
	//	plist.resize(n1);
	return;
}

Contour::Contour(vector<Point>* Pt_vec)
{
	n = (unsigned)(*Pt_vec).size();
	plist.clear();
	//	for(vector<Point>::iterator it=(*Pt_vec).begin();it!=(*Pt_vec).end();it++) plist.push_back(*it);
	plist = (*Pt_vec);
	return;
}

void Contour::clear()
{
	plist.clear();
	n = 0;
	return;
}

void Contour::add_point(Point p)
{
	plist.push_back(p);
	return;
}

void Contour::add_points(vector<Point>* Pt_vec)
{
	for (vector<Point>::iterator it = (*Pt_vec).begin(); it != (*Pt_vec).end();
			 it++)
		plist.push_back(*it);
	n = (unsigned)plist.size();
	return;
}

void Contour::print_contour()
{
	for (unsigned int i = 0; i < n; i++)
		cout << plist[i].px << ":" << plist[i].py << " ";
	cout << endl;
	return;
}

void Contour::doug_peuck(float epsilon, bool closed)
{
	UNREFERENCED_PARAMETER(closed);
	if (n > 2)
	{
		vector<bool> v1;
		v1.resize(n);

		for (unsigned int i = 0; i < n; i++)
			v1[i] = false;
		v1[0] = true;
		v1[n - 1] = true;
		doug_peuck_sub(epsilon, 0, n - 1, &v1);

		vector<Point>::iterator it = plist.begin();
		for (unsigned int i = 0; i < n; i++)
		{
			if (!v1[i])
				it = plist.erase(it);
			else
				it++;
		}

		n = (unsigned)plist.size();
	}

	return;
}

void Contour::presimplify(float d, bool closed)
{
	float d2 = d * d;
	if (n >= 2)
	{
		vector<Point>::iterator pit1 = plist.begin();
		vector<Point>::iterator pit2 = plist.begin();

		while (pit2 != plist.end())
		{
			pit2++;
			while (pit2 != plist.end() && dist2_b(&(*pit1), &(*pit2)) < d2)
			{
				pit2 = plist.erase(pit2);
			}
			pit1 = pit2;
		}

		if (closed)
		{
			pit1--;
			pit2 = plist.begin();
			while (pit2 != plist.end() && dist2_b(&(*pit1), &(*pit2)) < d2)
			{
				pit2 = plist.erase(pit2);
			}
		}

		n = unsigned(plist.size());
	}
	return;
}

unsigned int Contour::return_n() { return n; }

void Contour::return_contour(vector<Point>* Pt_vec)
{
	(*Pt_vec).clear();
	for (unsigned int i = 0; i < n; i++)
		(*Pt_vec).push_back(plist[i]);
	return;
}

void Contour::doug_peuck_sub(float epsilon, const unsigned int p1,
		const unsigned int p2, vector<bool>* v1_p)
{
	//	cout << p1<<" "<<p2<<endl;
	if (p2 <= p1 + 1)
		return;
	float dv, l;
	float max_dist = 0;
	Point SIJ, SIV;
	unsigned int max_pos = p1;

	SIJ.px = plist[p2].px - plist[p1].px;
	SIJ.py = plist[p2].py - plist[p1].py;

	l = dist(&SIJ);

	//	cout << l << endl;

	for (unsigned int i = p1 + 1; i < p2; i++)
	{
		SIV.px = plist[i].px - plist[p1].px;
		SIV.py = plist[i].py - plist[p1].py;
		dv = cross(&SIV, &SIJ) / l;
		//		cout << dv << ":";
		if (dv > max_dist)
		{
			max_dist = dv;
			max_pos = i;
		}
	}
	//	cout << endl;

	if (max_dist > epsilon)
	{
		(*v1_p)[max_pos] = true;
		doug_peuck_sub(epsilon, p1, max_pos, v1_p);
		doug_peuck_sub(epsilon, max_pos, p2, v1_p);
	}

	return;
}

//abcd void contour_class2::doug_peuck(float epsilon,vector<Point> *Pt_vec,vector<unsigned short> *Meetings_vec,vector<Point> *Result_vec)
void Contour2::doug_peuck(float epsilon, vector<Point>* Pt_vec,
		vector<unsigned>* Meetings_vec,
		vector<Point>* Result_vec)
{
	n = Pt_vec->size();
	m = Meetings_vec->size();
	if (m == 0)
	{
		Meetings_vec->push_back(0);
		m = 1;
	}

	if (n > m)
	{
		vector<bool> v1;
		v1.resize(n);

		for (unsigned int i = 0; i < n; i++)
			v1[i] = false;
		for (unsigned int i = 0; i < m; i++)
			v1[(*Meetings_vec)[i]] = true;

		for (unsigned int i = 0; i + 1 < m; i++)
		{
			doug_peuck_sub(epsilon, Pt_vec, (*Meetings_vec)[i],
					(*Meetings_vec)[i + 1], &v1);
		}
		doug_peuck_sub2(epsilon, Pt_vec, (*Meetings_vec)[m - 1],
				(*Meetings_vec)[0], &v1);

		Result_vec->clear();
		for (unsigned int i = 0; i < n; i++)
		{
			if (v1[i])
				Result_vec->push_back((*Pt_vec)[i]);
		}
	}

	return;
}

void Contour2::doug_peuck_sub(float epsilon, vector<Point>* Pt_vec,
		unsigned short p1, unsigned short p2,
		vector<bool>* v1_p)
{
	if (p2 <= p1 + 1)
		return;
	float dv, l;
	float max_dist = 0;
	Point SIJ, SIV;
	unsigned int max_pos = p1;

	SIJ.px = (*Pt_vec)[p2].px - (*Pt_vec)[p1].px;
	SIJ.py = (*Pt_vec)[p2].py - (*Pt_vec)[p1].py;

	l = dist(&SIJ);

	if (SIJ.px > 0 || (SIJ.px == 0 && SIJ.py > 0))
	{
		for (unsigned int i = p1 + 1; i < p2; i++)
		{
			SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&SIV, &SIJ) / l;
			//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
			if (dv > max_dist + 0.001)
			{
				max_dist = dv;
				max_pos = i;
			}
		}
	}
	else
	{
		for (unsigned int i = p2 - 1; i > p1; i--)
		{
			SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&SIV, &SIJ) / l;
			//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
			if (dv > max_dist + 0.001)
			{
				max_dist = dv;
				max_pos = i;
			}
		}
	}

	if (max_dist > epsilon)
	{
		(*v1_p)[max_pos] = true;
		doug_peuck_sub(epsilon, Pt_vec, p1, max_pos, v1_p);
		doug_peuck_sub(epsilon, Pt_vec, max_pos, p2, v1_p);
	}

	return;
}

void Contour2::doug_peuck_sub2(float epsilon, vector<Point>* Pt_vec,
		unsigned short p1, unsigned short p2,
		vector<bool>* v1_p)
{
	if ((p2 == 0 && p1 + 1 == n) || p1 < p2)
		return;

	float dv, l;
	float max_dist = 0;
	Point SIJ, SIV;

	if (p1 == p2)
	{
		if (n > 2)
		{
			unsigned short p3, p4;
			if ((unsigned int)(p1 + 1) < n)
				p3 = p1 + 1;
			else
				p3 = 0;
			if (p1 == 0)
				p4 = n - 1;
			else
				p4 = p1 - 1;
			unsigned int max_pos = p1;
			float epsilon1 = epsilon * epsilon;

			SIJ.px = (*Pt_vec)[p4].px - (*Pt_vec)[p3].px;
			SIJ.py = (*Pt_vec)[p4].py - (*Pt_vec)[p3].py;
			if (SIJ.px > 0 || (SIJ.px == 0 && SIJ.py > 0))
			{
				for (unsigned int i = p1 + 1; i < n; i++)
				{
					SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
					SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
					dv = float(SIV.px * SIV.px + SIV.py * SIV.py);
					//			if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
					if (dv > max_dist + 0.001)
					{
						max_dist = dv;
						max_pos = i;
					}
				}
				for (unsigned int i = 0; i < p2; i++)
				{
					SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
					SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
					dv = float(SIV.px * SIV.px + SIV.py * SIV.py);
					//			if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
					if (dv > max_dist + 0.001)
					{
						max_dist = dv;
						max_pos = i;
					}
				}
			}
			else
			{
				if (p2 > 0)
				{
					for (unsigned int i = p2 - 1; i > 0; i--)
					{
						SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
						SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
						dv = float(SIV.px * SIV.px + SIV.py * SIV.py);
						//			if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
						if (dv > max_dist + 0.001)
						{
							max_dist = dv;
							max_pos = i;
						}
					}
					SIV.px = (*Pt_vec)[0].px - (*Pt_vec)[p1].px;
					SIV.py = (*Pt_vec)[0].py - (*Pt_vec)[p1].py;
					dv = float(SIV.px * SIV.px + SIV.py * SIV.py);
					//			if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
					if (dv > max_dist + 0.001)
					{
						max_dist = dv;
						max_pos = 0;
					}
				}
				for (unsigned int i = n - 1; i > p1; i--)
				{
					SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
					SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
					dv = float(SIV.px * SIV.px + SIV.py * SIV.py);
					//			if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
					if (dv > max_dist + 0.001)
					{
						max_dist = dv;
						max_pos = i;
					}
				}
			}

			if (max_dist > epsilon1)
			{
				if (max_pos < p2)
				{
					(*v1_p)[max_pos] = true;
					doug_peuck_sub2(epsilon, Pt_vec, p1, max_pos, v1_p);
					doug_peuck_sub(epsilon, Pt_vec, max_pos, p2, v1_p);
				}
				else
				{
					(*v1_p)[max_pos] = true;
					doug_peuck_sub(epsilon, Pt_vec, p1, max_pos, v1_p);
					doug_peuck_sub2(epsilon, Pt_vec, max_pos, p2, v1_p);
				}
			}

			return;
		}
		else
		{
			float epsilon1 = epsilon * epsilon;
			if (p2 > 0)
			{
				SIV.px = (*Pt_vec)[0].px - (*Pt_vec)[p1].px;
				SIV.py = (*Pt_vec)[0].py - (*Pt_vec)[p1].py;
				dv = float(SIV.px * SIV.px + SIV.py * SIV.py);
				if (dv > epsilon1)
					(*v1_p)[0] = true;
			}
			else
			{
				SIV.px = (*Pt_vec)[1].px - (*Pt_vec)[p1].px;
				SIV.py = (*Pt_vec)[1].py - (*Pt_vec)[p1].py;
				dv = float(SIV.px * SIV.px + SIV.py * SIV.py);
				if (dv > epsilon1)
					(*v1_p)[1] = true;
			}
			return;
		}
	}

	max_dist = 0;
	unsigned int max_pos = p1;

	SIJ.px = (*Pt_vec)[p2].px - (*Pt_vec)[p1].px;
	SIJ.py = (*Pt_vec)[p2].py - (*Pt_vec)[p1].py;

	l = dist(&SIJ);

	if (SIJ.px > 0 || (SIJ.px == 0 && SIJ.py > 0))
	{
		for (unsigned int i = p1 + 1; i < n; i++)
		{
			SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&SIV, &SIJ) / l;
			//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
			if (dv > max_dist + 0.001)
			{
				max_dist = dv;
				max_pos = i;
			}
		}
		for (unsigned int i = 0; i < p2; i++)
		{
			SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&SIV, &SIJ) / l;
			//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
			if (dv > max_dist + 0.001)
			{
				max_dist = dv;
				max_pos = i;
			}
		}
	}
	else
	{
		if (p2 > 0)
		{
			for (unsigned int i = p2 - 1; i > 0; i--)
			{
				SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
				SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
				dv = cross(&SIV, &SIJ) / l;
				//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
				if (dv > max_dist + 0.001)
				{
					max_dist = dv;
					max_pos = i;
				}
			}
			SIV.px = (*Pt_vec)[0].px - (*Pt_vec)[p1].px;
			SIV.py = (*Pt_vec)[0].py - (*Pt_vec)[p1].py;
			dv = cross(&SIV, &SIJ) / l;
			//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
			if (dv > max_dist + 0.001)
			{
				max_dist = dv;
				max_pos = 0;
			}
		}
		for (unsigned int i = n - 1; i > p1; i--)
		{
			SIV.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			SIV.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&SIV, &SIJ) / l;
			//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
			if (dv > max_dist + 0.001)
			{
				max_dist = dv;
				max_pos = i;
			}
		}
	}

	if (max_dist > epsilon)
	{
		if (max_pos < p2)
		{
			(*v1_p)[max_pos] = true;
			doug_peuck_sub2(epsilon, Pt_vec, p1, max_pos, v1_p);
			doug_peuck_sub(epsilon, Pt_vec, max_pos, p2, v1_p);
		}
		else
		{
			(*v1_p)[max_pos] = true;
			doug_peuck_sub(epsilon, Pt_vec, p1, max_pos, v1_p);
			doug_peuck_sub2(epsilon, Pt_vec, max_pos, p2, v1_p);
		}
	}

	return;
}
