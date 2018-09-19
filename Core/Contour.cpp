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

namespace iseg {

Contour::Contour()
{
	n = 0;
}

Contour::Contour(std::vector<Point>* Pt_vec)
{
	n = (unsigned)(*Pt_vec).size();
	plist.clear();
	plist = (*Pt_vec);
}

void Contour::clear()
{
	plist.clear();
	n = 0;
}

void Contour::add_point(Point p)
{
	plist.push_back(p);
}

void Contour::add_points(std::vector<Point>* Pt_vec)
{
	for (auto p : *Pt_vec)
		plist.push_back(p);
	n = (unsigned)plist.size();
}

void Contour::print_contour()
{
	for (unsigned int i = 0; i < n; i++)
		std::cout << plist[i].px << ":" << plist[i].py << " ";
	std::cout << std::endl;
}

void Contour::doug_peuck(float epsilon, bool /*closed*/)
{
	if (n > 2)
	{
		std::vector<bool> v1;
		v1.resize(n);

		for (unsigned int i = 0; i < n; i++)
			v1[i] = false;
		v1[0] = true;
		v1[n - 1] = true;
		doug_peuck_sub(epsilon, 0, n - 1, &v1);

		auto it = plist.begin();
		for (unsigned int i = 0; i < n; i++)
		{
			if (!v1[i])
				it = plist.erase(it);
			else
				it++;
		}

		n = (unsigned)plist.size();
	}
}

void Contour::presimplify(float d, bool closed)
{
	float d2 = d * d;
	if (n >= 2)
	{
		auto pit1 = plist.begin();
		auto pit2 = plist.begin();

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
}

unsigned int Contour::return_n() { return n; }

void Contour::return_contour(std::vector<Point>* Pt_vec)
{
	(*Pt_vec).clear();
	for (unsigned int i = 0; i < n; i++)
		(*Pt_vec).push_back(plist[i]);
}

void Contour::doug_peuck_sub(float epsilon, const unsigned int p1, const unsigned int p2, std::vector<bool>* v1_p)
{
	if (p2 <= p1 + 1)
		return;
	float dv, l;
	float max_dist = 0;
	Point SIJ, SIV;
	unsigned int max_pos = p1;

	SIJ.px = plist[p2].px - plist[p1].px;
	SIJ.py = plist[p2].py - plist[p1].py;

	l = dist(&SIJ);

	for (unsigned int i = p1 + 1; i < p2; i++)
	{
		SIV.px = plist[i].px - plist[p1].px;
		SIV.py = plist[i].py - plist[p1].py;
		dv = cross(&SIV, &SIJ) / l;
		if (dv > max_dist)
		{
			max_dist = dv;
			max_pos = i;
		}
	}

	if (max_dist > epsilon)
	{
		(*v1_p)[max_pos] = true;
		doug_peuck_sub(epsilon, p1, max_pos, v1_p);
		doug_peuck_sub(epsilon, max_pos, p2, v1_p);
	}
}

void Contour2::doug_peuck(float epsilon, std::vector<Point>* Pt_vec,
		std::vector<unsigned>* Meetings_vec,
		std::vector<Point>* Result_vec)
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
		std::vector<bool> v1;
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
}

void Contour2::doug_peuck_sub(float epsilon, std::vector<Point>* Pt_vec,
		unsigned short p1, unsigned short p2, std::vector<bool>* v1_p)
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
}

void Contour2::doug_peuck_sub2(float epsilon, std::vector<Point>* Pt_vec,
		unsigned short p1, unsigned short p2, std::vector<bool>* v1_p)
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
}

} // namespace iseg
