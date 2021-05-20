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

#include "Contour.h"

namespace iseg {

Contour::Contour()
{
	m_N = 0;
}

Contour::Contour(std::vector<Point>* Pt_vec)
{
	m_N = (unsigned)(*Pt_vec).size();
	m_Plist.clear();
	m_Plist = (*Pt_vec);
}

void Contour::Clear()
{
	m_Plist.clear();
	m_N = 0;
}

void Contour::AddPoint(Point p)
{
	m_Plist.push_back(p);
}

void Contour::AddPoints(std::vector<Point>* Pt_vec)
{
	for (auto p : *Pt_vec)
		m_Plist.push_back(p);
	m_N = (unsigned)m_Plist.size();
}

void Contour::PrintContour()
{
	for (unsigned int i = 0; i < m_N; i++)
		std::cout << m_Plist[i].px << ":" << m_Plist[i].py << " ";
	std::cout << std::endl;
}

void Contour::DougPeuck(float epsilon, bool /*closed*/)
{
	if (m_N > 2)
	{
		std::vector<bool> v1;
		v1.resize(m_N);

		for (unsigned int i = 0; i < m_N; i++)
			v1[i] = false;
		v1[0] = true;
		v1[m_N - 1] = true;
		DougPeuckSub(epsilon, 0, m_N - 1, &v1);

		auto it = m_Plist.begin();
		for (unsigned int i = 0; i < m_N; i++)
		{
			if (!v1[i])
				it = m_Plist.erase(it);
			else
				it++;
		}

		m_N = (unsigned)m_Plist.size();
	}
}

void Contour::Presimplify(float d, bool closed)
{
	float d2 = d * d;
	if (m_N >= 2)
	{
		auto pit1 = m_Plist.begin();
		auto pit2 = m_Plist.begin();

		while (pit2 != m_Plist.end())
		{
			pit2++;
			while (pit2 != m_Plist.end() && dist2_b(&(*pit1), &(*pit2)) < d2)
			{
				pit2 = m_Plist.erase(pit2);
			}
			pit1 = pit2;
		}

		if (closed)
		{
			pit1--;
			pit2 = m_Plist.begin();
			while (pit2 != m_Plist.end() && dist2_b(&(*pit1), &(*pit2)) < d2)
			{
				pit2 = m_Plist.erase(pit2);
			}
		}

		m_N = unsigned(m_Plist.size());
	}
}

unsigned int Contour::ReturnN() const { return m_N; }

void Contour::ReturnContour(std::vector<Point>* Pt_vec)
{
	(*Pt_vec).clear();
	for (unsigned int i = 0; i < m_N; i++)
		(*Pt_vec).push_back(m_Plist[i]);
}

void Contour::DougPeuckSub(float epsilon, const unsigned int p1, const unsigned int p2, std::vector<bool>* v1_p)
{
	if (p2 <= p1 + 1)
		return;
	float dv, l;
	float max_dist = 0;
	Point sij, siv;
	unsigned int max_pos = p1;

	sij.px = m_Plist[p2].px - m_Plist[p1].px;
	sij.py = m_Plist[p2].py - m_Plist[p1].py;

	l = dist(&sij);

	for (unsigned int i = p1 + 1; i < p2; i++)
	{
		siv.px = m_Plist[i].px - m_Plist[p1].px;
		siv.py = m_Plist[i].py - m_Plist[p1].py;
		dv = cross(&siv, &sij) / l;
		if (dv > max_dist)
		{
			max_dist = dv;
			max_pos = i;
		}
	}

	if (max_dist > epsilon)
	{
		(*v1_p)[max_pos] = true;
		DougPeuckSub(epsilon, p1, max_pos, v1_p);
		DougPeuckSub(epsilon, max_pos, p2, v1_p);
	}
}

void Contour2::DougPeuck(float epsilon, std::vector<Point>* Pt_vec, std::vector<unsigned>* Meetings_vec, std::vector<Point>* Result_vec)
{
	m_N = Pt_vec->size();
	m_M = Meetings_vec->size();
	if (m_M == 0)
	{
		Meetings_vec->push_back(0);
		m_M = 1;
	}

	if (m_N > m_M)
	{
		std::vector<bool> v1;
		v1.resize(m_N);

		for (unsigned int i = 0; i < m_N; i++)
			v1[i] = false;
		for (unsigned int i = 0; i < m_M; i++)
			v1[(*Meetings_vec)[i]] = true;

		for (unsigned int i = 0; i + 1 < m_M; i++)
		{
			DougPeuckSub(epsilon, Pt_vec, (*Meetings_vec)[i], (*Meetings_vec)[i + 1], &v1);
		}
		DougPeuckSub2(epsilon, Pt_vec, (*Meetings_vec)[m_M - 1], (*Meetings_vec)[0], &v1);

		Result_vec->clear();
		for (unsigned int i = 0; i < m_N; i++)
		{
			if (v1[i])
				Result_vec->push_back((*Pt_vec)[i]);
		}
	}
}

void Contour2::DougPeuckSub(float epsilon, std::vector<Point>* Pt_vec, unsigned short p1, unsigned short p2, std::vector<bool>* v1_p)
{
	if (p2 <= p1 + 1)
		return;
	float dv, l;
	float max_dist = 0;
	Point sij, siv;
	unsigned int max_pos = p1;

	sij.px = (*Pt_vec)[p2].px - (*Pt_vec)[p1].px;
	sij.py = (*Pt_vec)[p2].py - (*Pt_vec)[p1].py;

	l = dist(&sij);

	if (sij.px > 0 || (sij.px == 0 && sij.py > 0))
	{
		for (unsigned int i = p1 + 1; i < p2; i++)
		{
			siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&siv, &sij) / l;
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
			siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&siv, &sij) / l;
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
		DougPeuckSub(epsilon, Pt_vec, p1, max_pos, v1_p);
		DougPeuckSub(epsilon, Pt_vec, max_pos, p2, v1_p);
	}
}

void Contour2::DougPeuckSub2(float epsilon, std::vector<Point>* Pt_vec, unsigned short p1, unsigned short p2, std::vector<bool>* v1_p)
{
	if ((p2 == 0 && p1 + 1 == m_N) || p1 < p2)
		return;

	float dv, l;
	float max_dist = 0;
	Point sij, siv;

	if (p1 == p2)
	{
		if (m_N > 2)
		{
			unsigned short p3, p4;
			if ((unsigned int)(p1 + 1) < m_N)
				p3 = p1 + 1;
			else
				p3 = 0;
			if (p1 == 0)
				p4 = m_N - 1;
			else
				p4 = p1 - 1;
			unsigned int max_pos = p1;
			float epsilon1 = epsilon * epsilon;

			sij.px = (*Pt_vec)[p4].px - (*Pt_vec)[p3].px;
			sij.py = (*Pt_vec)[p4].py - (*Pt_vec)[p3].py;
			if (sij.px > 0 || (sij.px == 0 && sij.py > 0))
			{
				for (unsigned int i = p1 + 1; i < m_N; i++)
				{
					siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
					siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
					dv = float(siv.px * siv.px + siv.py * siv.py);
					//			if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
					if (dv > max_dist + 0.001)
					{
						max_dist = dv;
						max_pos = i;
					}
				}
				for (unsigned int i = 0; i < p2; i++)
				{
					siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
					siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
					dv = float(siv.px * siv.px + siv.py * siv.py);
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
						siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
						siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
						dv = float(siv.px * siv.px + siv.py * siv.py);
						//			if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
						if (dv > max_dist + 0.001)
						{
							max_dist = dv;
							max_pos = i;
						}
					}
					siv.px = (*Pt_vec)[0].px - (*Pt_vec)[p1].px;
					siv.py = (*Pt_vec)[0].py - (*Pt_vec)[p1].py;
					dv = float(siv.px * siv.px + siv.py * siv.py);
					//			if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
					if (dv > max_dist + 0.001)
					{
						max_dist = dv;
						max_pos = 0;
					}
				}
				for (unsigned int i = m_N - 1; i > p1; i--)
				{
					siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
					siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
					dv = float(siv.px * siv.px + siv.py * siv.py);
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
					DougPeuckSub2(epsilon, Pt_vec, p1, max_pos, v1_p);
					DougPeuckSub(epsilon, Pt_vec, max_pos, p2, v1_p);
				}
				else
				{
					(*v1_p)[max_pos] = true;
					DougPeuckSub(epsilon, Pt_vec, p1, max_pos, v1_p);
					DougPeuckSub2(epsilon, Pt_vec, max_pos, p2, v1_p);
				}
			}

			return;
		}
		else
		{
			float epsilon1 = epsilon * epsilon;
			if (p2 > 0)
			{
				siv.px = (*Pt_vec)[0].px - (*Pt_vec)[p1].px;
				siv.py = (*Pt_vec)[0].py - (*Pt_vec)[p1].py;
				dv = float(siv.px * siv.px + siv.py * siv.py);
				if (dv > epsilon1)
					(*v1_p)[0] = true;
			}
			else
			{
				siv.px = (*Pt_vec)[1].px - (*Pt_vec)[p1].px;
				siv.py = (*Pt_vec)[1].py - (*Pt_vec)[p1].py;
				dv = float(siv.px * siv.px + siv.py * siv.py);
				if (dv > epsilon1)
					(*v1_p)[1] = true;
			}
			return;
		}
	}

	max_dist = 0;
	unsigned int max_pos = p1;

	sij.px = (*Pt_vec)[p2].px - (*Pt_vec)[p1].px;
	sij.py = (*Pt_vec)[p2].py - (*Pt_vec)[p1].py;

	l = dist(&sij);

	if (sij.px > 0 || (sij.px == 0 && sij.py > 0))
	{
		for (unsigned int i = p1 + 1; i < m_N; i++)
		{
			siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&siv, &sij) / l;
			//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
			if (dv > max_dist + 0.001)
			{
				max_dist = dv;
				max_pos = i;
			}
		}
		for (unsigned int i = 0; i < p2; i++)
		{
			siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&siv, &sij) / l;
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
				siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
				siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
				dv = cross(&siv, &sij) / l;
				//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
				if (dv > max_dist + 0.001)
				{
					max_dist = dv;
					max_pos = i;
				}
			}
			siv.px = (*Pt_vec)[0].px - (*Pt_vec)[p1].px;
			siv.py = (*Pt_vec)[0].py - (*Pt_vec)[p1].py;
			dv = cross(&siv, &sij) / l;
			//		if(dv>=max_dist-0.001) {max_dist=dv; max_pos=i;}
			if (dv > max_dist + 0.001)
			{
				max_dist = dv;
				max_pos = 0;
			}
		}
		for (unsigned int i = m_N - 1; i > p1; i--)
		{
			siv.px = (*Pt_vec)[i].px - (*Pt_vec)[p1].px;
			siv.py = (*Pt_vec)[i].py - (*Pt_vec)[p1].py;
			dv = cross(&siv, &sij) / l;
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
			DougPeuckSub2(epsilon, Pt_vec, p1, max_pos, v1_p);
			DougPeuckSub(epsilon, Pt_vec, max_pos, p2, v1_p);
		}
		else
		{
			(*v1_p)[max_pos] = true;
			DougPeuckSub(epsilon, Pt_vec, p1, max_pos, v1_p);
			DougPeuckSub2(epsilon, Pt_vec, max_pos, p2, v1_p);
		}
	}
}

} // namespace iseg
