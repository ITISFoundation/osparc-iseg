/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

struct coef
{
	unsigned short a;
	float b;
	float c;
};

inline bool operator!=(coef c, unsigned short f) { return c.a != f; }

template<typename T>
class ImageForestingTransform
{
public:
	void IFTinit(unsigned short w, unsigned short h, float* E_bit,
			float* directivity_bit, T* lbl, bool connectivity)
	{
		width = w;
		height = h;
		area = (unsigned)width * height;
		parent = (unsigned*)malloc(sizeof(unsigned) * area);
		pf = (float*)malloc(sizeof(float) * area);
		Q = new IndexPriorityQueue(area, pf);
		processed = (bool*)malloc(sizeof(bool) * area);
		lb = (T*)malloc(sizeof(T) * area);
		directivity_bits = (float*)malloc(sizeof(float) * area);
		E_bits = (float*)malloc(sizeof(float) * area);
		for (unsigned i = 0; i < area; i++)
		{
			directivity_bits[i] = directivity_bit[i];
		}

		reinit(lbl, E_bit, connectivity);
	}
	void reinit(T* lbl, bool connectivity)
	{
		for (unsigned i = 0; i < area; i++)
		{
			lb[i] = lbl[i];
			processed[i] = false;
			parent[i] = area;
			if (lb[i] != 0)
			{
				Q->insert(i, 0);
			}
			else
			{
				pf[i] = 1E10;
			}
		}

		unsigned position;

		while (!Q->empty())
		{
			position = Q->pop();

			processed[position] = true;

			if (position % width != 0)
				update_step(position, position - 1, 270);
			if ((position + 1) % width != 0)
				update_step(position, position + 1, 270);
			if (position >= width)
				update_step(position, position - width, 180);
			if (position < area - width)
				update_step(position, position + width, 180);
			if (connectivity)
			{
				if (position >= width && position % width != 0)
					update_step(position, position - width - 1, 225);
				if ((position + 1) % width != 0 && position >= width)
					update_step(position, position - width + 1, 135);
				if (position < area - width && position % width != 0)
					update_step(position, position + width - 1, 135);
				if (position < area - width && (position + 1) % width != 0)
					update_step(position, position + width + 1, 225);
			}
		}
	}
	void reinit(T* lbl, bool connectivity, std::vector<unsigned> pts)
	{
		std::vector<unsigned>::iterator it = pts.begin();
		for (unsigned i = 0; i < area; i++)
		{
			lb[i] = lbl[i];
			processed[i] = false;
			parent[i] = area;
			if (lb[i] != 0)
			{
				Q->insert(i, 0);
			}
			else
			{
				pf[i] = 1E10;
			}
		}

		unsigned position;

		while (!Q->empty() && it != pts.end())
		{
			position = Q->pop();

			processed[position] = true;
			if (position == *it)
			{
				it++;
				while (it != pts.end() && processed[*it])
					it++;
			}

			if (position % width != 0)
				update_step(position, position - 1, 270);
			if ((position + 1) % width != 0)
				update_step(position, position + 1, 270);
			if (position >= width)
				update_step(position, position - width, 180);
			if (position < area - width)
				update_step(position, position + width, 180);
			if (connectivity)
			{
				if (position >= width && position % width != 0)
					update_step(position, position - width - 1, 225);
				if ((position + 1) % width != 0 && position >= width)
					update_step(position, position - width + 1, 135);
				if (position < area - width && position % width != 0)
					update_step(position, position + width - 1, 135);
				if (position < area - width && (position + 1) % width != 0)
					update_step(position, position + width + 1, 225);
			}
		}

		Q->clear();
	}
	void reinit(T* lbl, float* E_bit, bool connectivity)
	{
		if (E_bits != E_bit)
		{
			for (unsigned i = 0; i < area; i++)
			{
				E_bits[i] = E_bit[i];
			}
		}

		reinit(lbl, connectivity);
	}
	void reinit(T* lbl, float* E_bit, bool connectivity, std::vector<unsigned> pts)
	{
		if (E_bits != E_bit)
		{
			for (unsigned i = 0; i < area; i++)
			{
				E_bits[i] = E_bit[i];
			}
		}

		reinit(lbl, connectivity, pts);
	}
	float* return_pf() { return pf; }
	T* return_lb() { return lb; }
	void return_path(Point p, std::vector<Point>* Pt_vec)
	{
		Point p1;
		Pt_vec->clear();
		Pt_vec->push_back(p);
		unsigned pos = (unsigned)width * p.py + p.px;
		while ((pos = parent[pos]) != area)
		{
			p1.px = pos % width;
			p1.py = pos / width;
			Pt_vec->push_back(p1);
		}
	}
	void return_path(unsigned p, std::vector<unsigned>* Pt_vec)
	{
		Pt_vec->clear();
		unsigned pos = p;
		Pt_vec->push_back(pos);
		while ((pos = parent[pos]) != area)
		{
			Pt_vec->push_back(pos);
		}
	}
	void append_path(unsigned p, std::vector<unsigned>* Pt_vec)
	{
		unsigned pos = p;
		Pt_vec->push_back(pos);
		while ((pos = parent[pos]) != area)
		{
			Pt_vec->push_back(pos);
		}

		return;
	}
	virtual ~ImageForestingTransform()
	{
		free(pf);
		free(lb);
		free(parent);
		free(processed);
		free(E_bits);
		free(directivity_bits);
		free(Q);
	}

protected:
	float* pf; //path-function value
	float* directivity_bits;
	float* E_bits;
	T* lb;
	bool* processed;
	short unsigned width;
	short unsigned height;
	unsigned area;

private:
	IndexPriorityQueue* Q;
	unsigned* parent;
	inline void update_step(unsigned p, unsigned q, float direction)
	{
		float tmp;
		if (!processed[q])
		{
			tmp = compute_pf(p, q, direction);
			if (tmp < pf[q])
			{
				parent[q] = p;
				if (Q->in_queue(q))
				{
					recompute_lb(p, q);
					Q->make_smaller(q, tmp);
				}
				else
				{
					compute_lb(p, q);
					Q->insert(q, tmp);
				}
			}
		}
	}
	virtual inline void compute_lb(unsigned p, unsigned q)
	{
		lb[q] = lb[p];
	}
	virtual inline void recompute_lb(unsigned p, unsigned q)
	{
		lb[q] = lb[p];
	}
	virtual inline float compute_pf(unsigned p, unsigned q, float direction)
	{
		return 1;
	}
};

class ImageForestingTransformRegionGrowing
		: public ImageForestingTransform<float>
{
public:
	void rg_init(unsigned short w, unsigned short h, float* gradient,
			float* lbl)
	{
		IFTinit(w, h, gradient, gradient, lbl, false);
		return;
	}

private:
	inline float compute_pf(unsigned p, unsigned q, float /* direction */)
	{
		return std::max(pf[p], std::abs(E_bits[p] - E_bits[q]));
	}
};

class ImageForestingTransformLivewire
		: public ImageForestingTransform<unsigned short>
{
public:
	void lw_init(unsigned short w, unsigned short h, float* E_bits,
			float* direction, Point p)
	{
		lbl = (unsigned short*)malloc((unsigned)w * h * sizeof(unsigned short));
		for (unsigned i = 0; i < unsigned(w) * h; i++)
			lbl[i] = 0;
		pt = p.px + p.py * w;
		lbl[pt] = 1;
		IFTinit(w, h, E_bits, direction, lbl, true);
		return;
	}
	void change_pt(Point p)
	{
		lbl[pt] = 0;
		pt = p.px + p.py * width;
		lbl[pt] = 1;
		reinit(lbl, E_bits, true);
		return;
	}
	void change_pt(unsigned p, std::vector<unsigned> pts)
	{
		lbl[pt] = 0;
		pt = p;
		lbl[pt] = 1;
		reinit(lbl, E_bits, true, pts);
		return;
	}
	~ImageForestingTransformLivewire() { free(lbl); }

private:
	unsigned short* lbl;
	unsigned pt;
	inline float compute_pf(unsigned p, unsigned q, float direction)
	{
		return E_bits[q] + pf[p] +
					 (abs(direction + directivity_bits[p] -
								floor((direction + directivity_bits[p]) / 180) * 180 - 90) +
							 abs(direction + directivity_bits[q] -
									 floor((direction + directivity_bits[q]) / 180) * 180 -
									 90)) *
							 0.14f / 270;
	}
};

class ImageForestingTransformAdaptFuzzy : public ImageForestingTransform<float>
{
public:
	void fuzzy_init(unsigned short w, unsigned short h, float* E_bits, Point p,
			float fm1, float fs1, float fs2)
	{
		m1 = 2 * fm1;
		s1 = -1 / (fs1 * fs1 * 8);
		s2 = -1 / (fs2 * 2);
		lbl = (float*)malloc((unsigned)w * h * sizeof(float));
		for (unsigned i = 0; i < unsigned(w) * h; i++)
			lbl[i] = 0;
		pt = p.px + p.py * (unsigned)w;
		lbl[pt] = 1;
		IFTinit(w, h, E_bits, E_bits, lbl, false);
	}
	void change_pt(Point p)
	{
		lbl[pt] = 0;
		pt = p.px + p.py * width;
		lbl[pt] = 1;
		reinit(lbl, true);
		return;
	}
	void change_param(float fm1, float fs1, float fs2)
	{
		m1 = 2 * fm1;
		s1 = -1 / (fs1 * fs1 * 8);
		s2 = -1 / (fs2 * 2);
	}

	~ImageForestingTransformAdaptFuzzy() { free(lbl); }

private:
	unsigned pt;
	float m1, s1, s2;
	float* lbl;
	inline float compute_pf(unsigned p, unsigned q, float /* direction */)
	{
		float h1 = exp((E_bits[p] + E_bits[q] - m1) *
									 (E_bits[p] + E_bits[q] - m1) * s1);
		float h2 = exp((E_bits[p] - E_bits[q]) * (E_bits[p] - E_bits[q]) * s2);
		return std::max(pf[p], 1 - (h1 * h1 + h2 * h2) / (h1 + h2));
	}
};

class ImageForestingTransformFastMarching : public ImageForestingTransform<coef>
{
public:
	void fastmarch_init(unsigned short w, unsigned short h, float* E_bits,
			float* lbl)
	{
		Ebits = (float*)malloc((unsigned)w * h * sizeof(float));
		lb1 = (coef*)malloc((unsigned)w * h * sizeof(coef));
		for (unsigned i = 0; i < unsigned(w) * h; i++)
		{
			if (E_bits[i] != 0)
				Ebits[i] = 1 / (E_bits[i] * E_bits[i]);
			else
				Ebits[i] = 1E10;
			if (lbl[i] != 0)
			{
				lb1[i].a = 1;
			}
			else
			{
				lb1[i].a = 0;
			}
			lb1[i].b = lb1[i].c = 0;
		}
		IFTinit(w, h, Ebits, Ebits, lb1, false);
		return;
	}
	~ImageForestingTransformFastMarching()
	{
		free(Ebits);
		free(lb1);
	}

private:
	float* Ebits;
	coef* lb1;
	inline float compute_pf(unsigned p, unsigned q, float /* direction */)
	{
		(lb[q].a)++;
		lb[q].b += pf[p];
		lb[q].c += pf[p] * pf[p];
		float f = (lb[q].b +
									sqrt(lb[q].b * lb[q].b + E_bits[q] - lb[q].a * lb[q].c)) /
							(lb[q].a);
		return f;
	}
	inline void compute_lb(unsigned p, unsigned q)
	{
	}
	inline void recompute_lb(unsigned p, unsigned q)
	{
	}
};

class ImageForestingTransformDistance : public ImageForestingTransform<coef>
{
public:
	void distance_init(unsigned short w, unsigned short h, float f, float* lbl)
	{
		lbel = (coef*)malloc((unsigned)w * h * sizeof(coef));
		unsigned i = 0;
		for (unsigned short j = 0; j < h; j++)
		{
			for (unsigned short k = 0; k < w; k++)
			{
				lbel[i].a = 0;
				lbel[i].b = float(k); //
				lbel[i].c = float(j); //
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
					lbel[i].a = 1;
				}
				else if (lbl[i1] == f && lbl[i] != f)
				{
					lbel[i1].a = 1;
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
					lbel[i].a = 1;
				}
				else if (lbl[i1] == f && lbl[i] != f)
				{
					lbel[i1].a = 1;
				}
				i++;
				i1++;
			}
			i++;
			i1++;
		}
		//			float f1;

		IFTinit(w, h, lbl, lbl, lbel, false);
		return;
	}
	~ImageForestingTransformDistance() { free(lbel); }

private:
	float* Ebits;
	coef* lbel;
	inline float compute_pf(unsigned p, unsigned q, float /* direction */)
	{
		float x = float(q % width);
		float y = float(q / width);
		return sqrt((x - lb[p].b) * (x - lb[p].b) + (y - lb[p].c) * (y - lb[p].c));
	}
	inline void compute_lb(unsigned p, unsigned q)
	{
		lb[q].b = lb[p].b;
		lb[q].c = lb[p].c;
		return;
	}
	inline void recompute_lb(unsigned p, unsigned q)
	{
		lb[q].b = lb[p].b;
		lb[q].c = lb[p].c;
		return;
	}
};

} // namespace iseg
