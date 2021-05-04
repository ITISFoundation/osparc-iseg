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

#include "fillcontour.h"

#include <list>
#include <math.h>
#include <vector>

namespace {

struct transition
{
	bool to_inside;
	float pos;
	inline bool operator<(const transition& a) const { return pos < a.pos; }
};

} // namespace

namespace iseg { namespace fillcontours {

float get_area(float* points, unsigned int nrpoints)
{
	if (nrpoints < 3)
		return 0;
	float area = 0;
	for (unsigned int pos = 0; pos + 5 < nrpoints * 3; pos += 3)
	{
		area += (points[pos + 1] + points[pos + 4]) *
				(points[pos + 3] - points[pos]);
	}
	area += (points[nrpoints * 3 - 2] + points[1]) *
			(points[0] - points[nrpoints * 3 - 3]);
	return area / 2;
}

bool pointinpoly(float* pt, unsigned int cnt, float* polypts)
{
	int oldquad, newquad;
	unsigned int thisptidx, lastptidx;
	//point thispt,lastpt;
	int a, b;
	int wind; /* current winding number */

	wind = 0;
	lastptidx = 3 * cnt - 3;
	oldquad = whichquad(&polypts[lastptidx], pt); /* get starting angle */
	for (unsigned int i = 0; i < cnt; i++)
	{ /* for each point in the polygon */
		thisptidx = 3 * i;
		newquad = whichquad(&polypts[thisptidx], pt);
		if (oldquad != newquad)
		{ /* adjust wind */
			/*
							* use mod 4 comparsions to see if we have
							* advanced or backed up one quadrant
							*/
			if (((oldquad + 1) % 4) == newquad)
			{
				wind++;
			}
			else if (((newquad + 1) % 4) == oldquad)
			{
				wind--;
			}
			else
			{
				/*
							* upper left to lower right, or
							* upper right to lower left. Determine
							* direction of winding  by intersection
							*  with x==0.
							*/
				a = polypts[lastptidx + 1] - polypts[thisptidx + 1];
				a *= (pt[0] - polypts[lastptidx]);
				b = polypts[lastptidx] - polypts[thisptidx];
				a += polypts[lastptidx + 1] * b;
				b *= pt[1];

				if (a > b)
				{
					wind += 2;
				}
				else
				{
					wind -= 2;
				}
			}
		}
		oldquad = newquad;
		lastptidx = thisptidx;
	}

	return wind != 0; /* non zero means point in poly */
}

/*
		 * Figure out which quadrant pt is in with respect to orig
		 */
int whichquad(float* pt, float* orig)
{
	int quad;
	if (pt[0] < orig[0])
	{
		if (pt[1] < orig[1])
			quad = 2;
		else
			quad = 1;
	}
	else
	{
		if (pt[1] < orig[1])
			quad = 3;
		else
			quad = 0;
	}
	return quad;
}

bool is_hole(float** points, unsigned int* nrpoints, unsigned int nrcontours,
			 unsigned int contournr)
{
	int nrinside = 0;
	for (unsigned int i = 0; i < nrcontours; i++)
	{
		if (i != contournr)
		{
			if (pointinpoly(points[contournr], nrpoints[i], points[i]))
				nrinside++;
		}
	}
	return nrinside % 2;
}

void fill_contour(bool* array, unsigned short* pixel_extents, float* origin,
				  float* pixel_size, float* direction_cosines, float** points,
				  unsigned int* nrpoints, unsigned int nrcontours,
				  bool clockwisefill)
{
	float tol = 0.1 * pixel_size[0];
	std::vector<std::list<transition>> inouts;
	inouts.resize(pixel_extents[1]);

	//FILE *fp3=fopen("C:\\test.txt","a");
	//for(unsigned int i=0;i<nrcontours;i++){
	//	fprintf(fp3,"c ");
	//	for(unsigned int j=0;j<nrpoints[i];j++)
	//		fprintf(fp3,"(%f %f %f)",points[i][3*j],points[i][3*j+1],points[i][3*j+2]);
	//	fprintf(fp3,"\n");
	//}
	//fclose(fp3);

	float x0, x1, y0, y1;
	unsigned short begin_h, end_h;
	float swap_x = direction_cosines[0];
	float swap_y = direction_cosines[4];
	for (unsigned int i = 0; i < nrcontours; i++)
	{
		bool clockwisefill1 = (get_area(points[i], nrpoints[i]) > 0);
		bool clockwisefill2 = is_hole(points, nrpoints, nrcontours, i);
		clockwisefill = (clockwisefill1 != clockwisefill2);
		if (swap_x * swap_y < 0)
			clockwisefill = !clockwisefill;
		if (clockwisefill)
		{
			for (unsigned long pos = 0; pos + 3 < 3 * nrpoints[i]; pos += 3)
			{
				x0 = swap_x * points[i][pos];
				y0 = swap_y * points[i][pos + 1];
				x1 = swap_x * points[i][pos + 3];
				y1 = swap_y * points[i][pos + 4];
				if (y0 != y1)
				{
					transition trans;
					float slope = (x0 - x1) / (y0 - y1);
					if (y0 > y1)
					{
						trans.to_inside = false;
						begin_h = (unsigned short)ceil(
							(y1 - swap_y * origin[1]) / pixel_size[1]);
						end_h = (unsigned short)floor(
							(y0 - swap_y * origin[1]) / pixel_size[1]);
						if (end_h >= pixel_extents[1])
							end_h = pixel_extents[1] - 1;
						for (unsigned short h = begin_h; h <= end_h; h++)
						{
							trans.pos = (x1 - swap_x * origin[0]) + tol +
										(pixel_size[1] * h +
										 (swap_y * origin[1] - y1)) *
											slope;
							inouts.at(h).push_back(trans);
						}
					}
					else
					{
						trans.to_inside = true;
						begin_h = (unsigned short)ceil(
							(y0 - swap_y * origin[1]) / pixel_size[1]);
						end_h = (unsigned short)floor(
							(y1 - swap_y * origin[1]) / pixel_size[1]);
						if (end_h >= pixel_extents[1])
							end_h = pixel_extents[1] - 1;
						for (unsigned short h = begin_h; h <= end_h; h++)
						{
							trans.pos = (x0 - swap_x * origin[0]) - tol +
										(pixel_size[1] * h +
										 (swap_y * origin[1] - y0)) *
											slope;
							inouts.at(h).push_back(trans);
						}
					}
				}
			}
			x0 = swap_x * points[i][0];
			y0 = swap_y * points[i][1];
			x1 = swap_x * points[i][3 * nrpoints[i] - 3];
			y1 = swap_y * points[i][3 * nrpoints[i] - 2];
			if (y1 != y0)
			{
				transition trans;
				float slope = (x1 - x0) / (y1 - y0);
				if (y1 > y0)
				{
					trans.to_inside = false;
					begin_h = (unsigned short)ceil((y0 - swap_y * origin[1]) /
												   pixel_size[1]);
					end_h = (unsigned short)floor((y1 - swap_y * origin[1]) /
												  pixel_size[1]);
					if (end_h >= pixel_extents[1])
						end_h = pixel_extents[1] - 1;
					for (unsigned short h = begin_h; h <= end_h; h++)
					{
						trans.pos =
							(x0 - swap_x * origin[0]) + tol +
							(pixel_size[1] * h + (swap_y * origin[1] - y0)) *
								slope;
						inouts.at(h).push_back(trans);
					}
				}
				else
				{
					trans.to_inside = true;
					begin_h = (unsigned short)ceil((y1 - swap_y * origin[1]) /
												   pixel_size[1]);
					end_h = (unsigned short)floor((y0 - swap_y * origin[1]) /
												  pixel_size[1]);
					if (end_h >= pixel_extents[1])
						end_h = pixel_extents[1] - 1;
					for (unsigned short h = begin_h; h <= end_h; h++)
					{
						trans.pos =
							(x1 - swap_x * origin[0]) - tol +
							(pixel_size[1] * h + (swap_y * origin[1] - y1)) *
								slope;
						inouts.at(h).push_back(trans);
					}
				}
			}
		}
		else
		{
			for (unsigned long pos = 0; pos + 3 < 3 * nrpoints[i]; pos += 3)
			{
				x0 = swap_x * points[i][pos];
				y0 = swap_y * points[i][pos + 1];
				x1 = swap_x * points[i][pos + 3];
				y1 = swap_y * points[i][pos + 4];
				if (y0 != y1)
				{
					transition trans;
					float slope = (x0 - x1) / (y0 - y1);
					if (y0 > y1)
					{
						trans.to_inside = true;
						begin_h = (unsigned short)ceil(
							(y1 - swap_y * origin[1]) / pixel_size[1]);
						end_h = (unsigned short)floor(
							(y0 - swap_y * origin[1]) / pixel_size[1]);
						if (end_h >= pixel_extents[1])
							end_h = pixel_extents[1] - 1;
						for (unsigned short h = begin_h; h <= end_h; h++)
						{
							trans.pos = (x1 - swap_x * origin[0]) - tol +
										(pixel_size[1] * h +
										 (swap_y * origin[1] - y1)) *
											slope;
							inouts.at(h).push_back(trans);
						}
					}
					else
					{
						trans.to_inside = false;
						begin_h = (unsigned short)ceil(
							(y0 - swap_y * origin[1]) / pixel_size[1]);
						end_h = (unsigned short)floor(
							(y1 - swap_y * origin[1]) / pixel_size[1]);
						if (end_h >= pixel_extents[1])
							end_h = pixel_extents[1] - 1;
						for (unsigned short h = begin_h; h <= end_h; h++)
						{
							trans.pos = (x0 - swap_x * origin[0]) + tol +
										(pixel_size[1] * h +
										 (swap_y * origin[1] - y0)) *
											slope;
							inouts.at(h).push_back(trans);
						}
					}
				}
			}
			x0 = swap_x * points[i][0];
			y0 = swap_y * points[i][1];
			x1 = swap_x * points[i][3 * nrpoints[i] - 3];
			y1 = swap_y * points[i][3 * nrpoints[i] - 2];
			if (y1 != y0)
			{
				transition trans;
				float slope = (x1 - x0) / (y1 - y0);
				if (y1 > y0)
				{
					trans.to_inside = true;
					begin_h = (unsigned short)ceil((y0 - swap_y * origin[1]) /
												   pixel_size[1]);
					end_h = (unsigned short)floor((y1 - swap_y * origin[1]) /
												  pixel_size[1]);
					if (end_h >= pixel_extents[1])
						end_h = pixel_extents[1] - 1;
					for (unsigned short h = begin_h; h <= end_h; h++)
					{
						trans.pos =
							(x0 - swap_x * origin[0]) - tol +
							(pixel_size[1] * h + (swap_y * origin[1] - y0)) *
								slope;
						inouts.at(h).push_back(trans);
					}
				}
				else
				{
					trans.to_inside = false;
					begin_h = (unsigned short)ceil((y1 - swap_y * origin[1]) /
												   pixel_size[1]);
					end_h = (unsigned short)floor((y0 - swap_y * origin[1]) /
												  pixel_size[1]);
					if (end_h >= pixel_extents[1])
						end_h = pixel_extents[1] - 1;
					for (unsigned short h = begin_h; h <= end_h; h++)
					{
						trans.pos =
							(x1 - swap_x * origin[0]) + tol +
							(pixel_size[1] * h + (swap_y * origin[1] - y1)) *
								slope;
						inouts.at(h).push_back(trans);
					}
				}
			}
		}
	}

	for (unsigned short h = 0; h < pixel_extents[1]; h++)
	{
		inouts.at(h).sort();
	}

	unsigned long position =
		pixel_extents[0] * (unsigned long)(pixel_extents[1] - 1);
	for (unsigned short h = 0; h < pixel_extents[1]; h++)
	{
		bool status = false;
		std::list<transition>::iterator it = inouts.at(h).begin();
		while (it != inouts.at(h).end() && it->pos < 0)
		{
			//status=!status;
			status = it->to_inside;
			it++;
		}
		for (unsigned short w = 0; w < pixel_extents[0]; w++)
		{
			while (it != inouts.at(h).end() && it->pos < w * pixel_size[0])
			{
				//status=!status;
				if (status && !it->to_inside)
				{
					it++;
					if (it == inouts.at(h).end() || it->to_inside)
						status = false;
				}
				else
				{
					status = it->to_inside;
					it++;
				}
			}
			array[position] = status;
			position++;
		}
		position -= 2 * pixel_extents[0];
	}

	//FILE *fp3=fopen("D:\\test100.txt","a");
	//for(unsigned short h=0;h<height;h++) {
	//	fprintf(fp3,"%i: ",h);
	//	std::list<transition>::iterator it=inouts[h].begin();
	//	while(it!=inouts[h].end())  {
	//		fprintf(fp3,"%f %i, ",it->pos,(int)it->to_inside);
	//		it++;
	//	}
	//	fprintf(fp3,"\n");
	//}
	//fclose(fp3);
}

}} // namespace iseg::fillcontours
