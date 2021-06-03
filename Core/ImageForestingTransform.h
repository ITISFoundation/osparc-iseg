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

#include "Core/IndexPriorityQueue.h"

#include <algorithm>
#include <functional>

namespace iseg {

struct Coef
{
	unsigned short m_A;
	float m_B;
	float m_C;
};

inline bool operator!=(Coef c, unsigned short f) { return c.m_A != f; }

template<typename T>
class ImageForestingTransform
{
public:
	void IFTinit(unsigned short w, unsigned short h, float* E_bit, float* directivity_bit, T* lbl, bool connectivity)
	{
		m_Width = w;
		m_Height = h;
		m_Area = (unsigned)m_Width * m_Height;
		m_Parent = (unsigned*)malloc(sizeof(unsigned) * m_Area);
		m_Pf = (float*)malloc(sizeof(float) * m_Area);
		m_Q = new IndexPriorityQueue(m_Area, m_Pf);
		m_Processed = (bool*)malloc(sizeof(bool) * m_Area);
		m_Lb = (T*)malloc(sizeof(T) * m_Area);
		m_DirectivityBits = (float*)malloc(sizeof(float) * m_Area);
		m_EBits = (float*)malloc(sizeof(float) * m_Area);
		for (unsigned i = 0; i < m_Area; i++)
		{
			m_DirectivityBits[i] = directivity_bit[i];
		}

		Reinit(lbl, E_bit, connectivity);
	}
	void Reinit(T* lbl, bool connectivity)
	{
		for (unsigned i = 0; i < m_Area; i++)
		{
			m_Lb[i] = lbl[i];
			m_Processed[i] = false;
			m_Parent[i] = m_Area;
			if (m_Lb[i] != 0)
			{
				m_Q->Insert(i, 0);
			}
			else
			{
				m_Pf[i] = 1E10;
			}
		}

		unsigned position;

		while (!m_Q->Empty())
		{
			position = m_Q->Pop();

			m_Processed[position] = true;

			if (position % m_Width != 0)
				UpdateStep(position, position - 1, 270);
			if ((position + 1) % m_Width != 0)
				UpdateStep(position, position + 1, 270);
			if (position >= m_Width)
				UpdateStep(position, position - m_Width, 180);
			if (position < m_Area - m_Width)
				UpdateStep(position, position + m_Width, 180);
			if (connectivity)
			{
				if (position >= m_Width && position % m_Width != 0)
					UpdateStep(position, position - m_Width - 1, 225);
				if ((position + 1) % m_Width != 0 && position >= m_Width)
					UpdateStep(position, position - m_Width + 1, 135);
				if (position < m_Area - m_Width && position % m_Width != 0)
					UpdateStep(position, position + m_Width - 1, 135);
				if (position < m_Area - m_Width && (position + 1) % m_Width != 0)
					UpdateStep(position, position + m_Width + 1, 225);
			}
		}
	}
	void Reinit(T* lbl, bool connectivity, std::vector<unsigned> pts)
	{
		std::vector<unsigned>::iterator it = pts.begin();
		for (unsigned i = 0; i < m_Area; i++)
		{
			m_Lb[i] = lbl[i];
			m_Processed[i] = false;
			m_Parent[i] = m_Area;
			if (m_Lb[i] != 0)
			{
				m_Q->Insert(i, 0);
			}
			else
			{
				m_Pf[i] = 1E10;
			}
		}

		unsigned position;

		while (!m_Q->Empty() && it != pts.end())
		{
			position = m_Q->Pop();

			m_Processed[position] = true;
			if (position == *it)
			{
				it++;
				while (it != pts.end() && m_Processed[*it])
					it++;
			}

			if (position % m_Width != 0)
				UpdateStep(position, position - 1, 270);
			if ((position + 1) % m_Width != 0)
				UpdateStep(position, position + 1, 270);
			if (position >= m_Width)
				UpdateStep(position, position - m_Width, 180);
			if (position < m_Area - m_Width)
				UpdateStep(position, position + m_Width, 180);
			if (connectivity)
			{
				if (position >= m_Width && position % m_Width != 0)
					UpdateStep(position, position - m_Width - 1, 225);
				if ((position + 1) % m_Width != 0 && position >= m_Width)
					UpdateStep(position, position - m_Width + 1, 135);
				if (position < m_Area - m_Width && position % m_Width != 0)
					UpdateStep(position, position + m_Width - 1, 135);
				if (position < m_Area - m_Width && (position + 1) % m_Width != 0)
					UpdateStep(position, position + m_Width + 1, 225);
			}
		}

		m_Q->Clear();
	}
	void Reinit(T* lbl, float* E_bit, bool connectivity)
	{
		if (m_EBits != E_bit)
		{
			for (unsigned i = 0; i < m_Area; i++)
			{
				m_EBits[i] = E_bit[i];
			}
		}

		Reinit(lbl, connectivity);
	}
	void Reinit(T* lbl, float* E_bit, bool connectivity, std::vector<unsigned> pts)
	{
		if (m_EBits != E_bit)
		{
			for (unsigned i = 0; i < m_Area; i++)
			{
				m_EBits[i] = E_bit[i];
			}
		}

		Reinit(lbl, connectivity, pts);
	}
	float* ReturnPf() { return m_Pf; }
	T* ReturnLb() { return m_Lb; }
	void ReturnPath(Point p, std::vector<Point>* Pt_vec)
	{
		Point p1;
		Pt_vec->clear();
		Pt_vec->push_back(p);
		unsigned pos = (unsigned)m_Width * p.py + p.px;
		while ((pos = m_Parent[pos]) != m_Area)
		{
			p1.px = pos % m_Width;
			p1.py = pos / m_Width;
			Pt_vec->push_back(p1);
		}
	}
	void ReturnPath(unsigned p, std::vector<unsigned>* Pt_vec)
	{
		Pt_vec->clear();
		unsigned pos = p;
		Pt_vec->push_back(pos);
		while ((pos = m_Parent[pos]) != m_Area)
		{
			Pt_vec->push_back(pos);
		}
	}
	void AppendPath(unsigned p, std::vector<unsigned>* Pt_vec)
	{
		unsigned pos = p;
		Pt_vec->push_back(pos);
		while ((pos = m_Parent[pos]) != m_Area)
		{
			Pt_vec->push_back(pos);
		}
	}
	virtual ~ImageForestingTransform()
	{
		free(m_Pf);
		free(m_Lb);
		free(m_Parent);
		free(m_Processed);
		free(m_EBits);
		free(m_DirectivityBits);
		free(m_Q);
	}

protected:
	float* m_Pf; //path-function value
	float* m_DirectivityBits;
	float* m_EBits;
	T* m_Lb;
	bool* m_Processed;
	short unsigned m_Width;
	short unsigned m_Height;
	unsigned m_Area;

private:
	IndexPriorityQueue* m_Q;
	unsigned* m_Parent;
	inline void UpdateStep(unsigned p, unsigned q, float direction)
	{
		float tmp;
		if (!m_Processed[q])
		{
			tmp = ComputePf(p, q, direction);
			if (tmp < m_Pf[q])
			{
				m_Parent[q] = p;
				if (m_Q->InQueue(q))
				{
					RecomputeLb(p, q);
					m_Q->MakeSmaller(q, tmp);
				}
				else
				{
					ComputeLb(p, q);
					m_Q->Insert(q, tmp);
				}
			}
		}
	}
	virtual inline void ComputeLb(unsigned p, unsigned q)
	{
		m_Lb[q] = m_Lb[p];
	}
	virtual inline void RecomputeLb(unsigned p, unsigned q)
	{
		m_Lb[q] = m_Lb[p];
	}
	virtual inline float ComputePf(unsigned p, unsigned q, float direction)
	{
		return 1;
	}
};

class ImageForestingTransformRegionGrowing
		: public ImageForestingTransform<float>
{
public:
	void RgInit(unsigned short w, unsigned short h, float* gradient, float* lbl)
	{
		IFTinit(w, h, gradient, gradient, lbl, false);
	}

private:
	inline float compute_pf(unsigned p, unsigned q, float /* direction */)
	{
		return std::max(m_Pf[p], std::abs(m_EBits[p] - m_EBits[q]));
	}
};

class ImageForestingTransformLivewire
		: public ImageForestingTransform<unsigned short>
{
public:
	void LwInit(unsigned short w, unsigned short h, float* E_bits, float* direction, Point p)
	{
		m_Lbl = (unsigned short*)malloc((unsigned)w * h * sizeof(unsigned short));
		for (unsigned i = 0; i < unsigned(w) * h; i++)
			m_Lbl[i] = 0;
		m_Pt = p.px + p.py * w;
		m_Lbl[m_Pt] = 1;
		IFTinit(w, h, E_bits, direction, m_Lbl, true);
	}
	void ChangePt(Point p)
	{
		m_Lbl[m_Pt] = 0;
		m_Pt = p.px + p.py * m_Width;
		m_Lbl[m_Pt] = 1;
		Reinit(m_Lbl, m_EBits, true);
	}
	void ChangePt(unsigned p, std::vector<unsigned> pts)
	{
		m_Lbl[m_Pt] = 0;
		m_Pt = p;
		m_Lbl[m_Pt] = 1;
		Reinit(m_Lbl, m_EBits, true, pts);
	}
	~ImageForestingTransformLivewire() override { free(m_Lbl); }

private:
	unsigned short* m_Lbl;
	unsigned m_Pt;
	inline float compute_pf(unsigned p, unsigned q, float direction)
	{
		return m_EBits[q] + m_Pf[p] +
					 (abs(direction + m_DirectivityBits[p] -
								floor((direction + m_DirectivityBits[p]) / 180) * 180 - 90) +
							 abs(direction + m_DirectivityBits[q] -
									 floor((direction + m_DirectivityBits[q]) / 180) * 180 -
									 90)) *
							 0.14f / 270;
	}
};

class ImageForestingTransformAdaptFuzzy : public ImageForestingTransform<float>
{
public:
	void FuzzyInit(unsigned short w, unsigned short h, float* E_bits, Point p, float fm1, float fs1, float fs2)
	{
		m_M1 = 2 * fm1;
		m_S1 = -1 / (fs1 * fs1 * 8);
		m_S2 = -1 / (fs2 * 2);
		m_Lbl = (float*)malloc((unsigned)w * h * sizeof(float));
		for (unsigned i = 0; i < unsigned(w) * h; i++)
			m_Lbl[i] = 0;
		m_Pt = p.px + p.py * (unsigned)w;
		m_Lbl[m_Pt] = 1;
		IFTinit(w, h, E_bits, E_bits, m_Lbl, false);
	}
	void ChangePt(Point p)
	{
		m_Lbl[m_Pt] = 0;
		m_Pt = p.px + p.py * m_Width;
		m_Lbl[m_Pt] = 1;
		Reinit(m_Lbl, true);
	}
	void ChangeParam(float fm1, float fs1, float fs2)
	{
		m_M1 = 2 * fm1;
		m_S1 = -1 / (fs1 * fs1 * 8);
		m_S2 = -1 / (fs2 * 2);
	}

	~ImageForestingTransformAdaptFuzzy() override { free(m_Lbl); }

private:
	unsigned m_Pt;
	float m_M1, m_S1, m_S2;
	float* m_Lbl;
	inline float compute_pf(unsigned p, unsigned q, float /* direction */)
	{
		float h1 = exp((m_EBits[p] + m_EBits[q] - m_M1) *
									 (m_EBits[p] + m_EBits[q] - m_M1) * m_S1);
		float h2 = exp((m_EBits[p] - m_EBits[q]) * (m_EBits[p] - m_EBits[q]) * m_S2);
		return std::max(m_Pf[p], 1 - (h1 * h1 + h2 * h2) / (h1 + h2));
	}
};

class ImageForestingTransformFastMarching : public ImageForestingTransform<Coef>
{
public:
	void FastmarchInit(unsigned short w, unsigned short h, float* E_bits, float* lbl)
	{
		m_Ebits = (float*)malloc((unsigned)w * h * sizeof(float));
		m_Lb1 = (Coef*)malloc((unsigned)w * h * sizeof(Coef));
		for (unsigned i = 0; i < unsigned(w) * h; i++)
		{
			if (E_bits[i] != 0)
				m_Ebits[i] = 1 / (E_bits[i] * E_bits[i]);
			else
				m_Ebits[i] = 1E10;
			if (lbl[i] != 0)
			{
				m_Lb1[i].m_A = 1;
			}
			else
			{
				m_Lb1[i].m_A = 0;
			}
			m_Lb1[i].m_B = m_Lb1[i].m_C = 0;
		}
		IFTinit(w, h, m_Ebits, m_Ebits, m_Lb1, false);
	}
	~ImageForestingTransformFastMarching() override
	{
		free(m_Ebits);
		free(m_Lb1);
	}

private:
	float* m_Ebits;
	Coef* m_Lb1;
	inline float compute_pf(unsigned p, unsigned q, float /* direction */)
	{
		(m_Lb[q].m_A)++;
		m_Lb[q].m_B += m_Pf[p];
		m_Lb[q].m_C += m_Pf[p] * m_Pf[p];
		float f = (m_Lb[q].m_B +
									sqrt(m_Lb[q].m_B * m_Lb[q].m_B + m_EBits[q] - m_Lb[q].m_A * m_Lb[q].m_C)) /
							(m_Lb[q].m_A);
		return f;
	}
	inline void compute_lb(unsigned p, unsigned q)
	{
	}
	inline void recompute_lb(unsigned p, unsigned q)
	{
	}
};

class ImageForestingTransformDistance : public ImageForestingTransform<Coef>
{
public:
	void DistanceInit(unsigned short w, unsigned short h, float f, float* lbl)
	{
		m_Lbel = (Coef*)malloc((unsigned)w * h * sizeof(Coef));
		unsigned i = 0;
		for (unsigned short j = 0; j < h; j++)
		{
			for (unsigned short k = 0; k < w; k++)
			{
				m_Lbel[i].m_A = 0;
				m_Lbel[i].m_B = float(k); //
				m_Lbel[i].m_C = float(j); //
				i++;
			}
		}
		i = 0;
		unsigned i1 = w;
		for (unsigned short j = 0; j < h - 1; j++)
		{
			for (unsigned short k = 0; k < w; k++)
			{
				if (lbl[i] == f && lbl[i1] != f)
				{
					m_Lbel[i].m_A = 1;
				}
				else if (lbl[i1] == f && lbl[i] != f)
				{
					m_Lbel[i1].m_A = 1;
				}
				i++;
				i1++;
			}
		}
		i = 0;
		i1 = 1;
		for (unsigned short j = 0; j < h; j++)
		{
			for (unsigned short k = 0; k < w - 1; k++)
			{
				if (lbl[i] == f && lbl[i1] != f)
				{
					m_Lbel[i].m_A = 1;
				}
				else if (lbl[i1] == f && lbl[i] != f)
				{
					m_Lbel[i1].m_A = 1;
				}
				i++;
				i1++;
			}
			i++;
			i1++;
		}
		//			float f1;

		IFTinit(w, h, lbl, lbl, m_Lbel, false);
	}
	~ImageForestingTransformDistance() override { free(m_Lbel); }

private:
	Coef* m_Lbel;
	inline float compute_pf(unsigned p, unsigned q, float /* direction */)
	{
		float x = float(q % m_Width);
		float y = float(q / m_Width);
		return sqrt((x - m_Lb[p].m_B) * (x - m_Lb[p].m_B) + (y - m_Lb[p].m_C) * (y - m_Lb[p].m_C));
	}
	inline void compute_lb(unsigned p, unsigned q)
	{
		m_Lb[q].m_B = m_Lb[p].m_B;
		m_Lb[q].m_C = m_Lb[p].m_C;
	}
	inline void recompute_lb(unsigned p, unsigned q)
	{
		m_Lb[q].m_B = m_Lb[p].m_B;
		m_Lb[q].m_C = m_Lb[p].m_C;
	}
};

} // namespace iseg
