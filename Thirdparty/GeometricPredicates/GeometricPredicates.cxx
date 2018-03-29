/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "GeometricPredicates.h"

#include <iostream>
#include <cmath>
#include <cassert>


#include "predicates.h"

namespace GeometricPredicates
{

double exactinit()
{
	return ::exactinit();
}

double getepsilon()
{
	return ::getepsilon();
}

double orient2d( double a[2], double b[2],
	double c[2] )
{
	return ::orient2d(a,b,c);
}

double orient3d( double a[3], double b[3], 
	double c[3], double d[3] )
{
	return ::orient3d(a,b,d,c);
}


int Triangle::intersect(double a[3], double b[3], double c[3],
	double xa[3], double xe[3], double *pcoords, double *xinter)
{
	double EPS = getepsilon();
	if (std::fabs(xe[0]-xa[0]) < EPS && std::fabs(xe[1]-xa[1]) < EPS && std::fabs(xe[2]-xa[2]) < EPS)
		return DEGENERATE_SEGMENT;

	// end points of segment are on same side of triangle
	if(::orient3d(a, b, c, xa) * ::orient3d(a, b, c, xe) > 0.0) return NO_INTER;

	// test if line passes to left (right) of directed triangle edge
	double det12 = ::orient3d(a, b, xa, xe);
	double det23 = ::orient3d(b, c, xa, xe);
	double det31 = ::orient3d(c, a, xa, xe);
	if( (det12 <= 0.0 && det23 <= 0.0 && det31 <= 0.0) || 
		(det12 >= 0.0 && det23 >= 0.0 && det31 >= 0.0)) 
	{
		// can compute intersection point by normalizing determinants
		if(pcoords)
		{
			double f = 1.0 / (det12 + det23 + det31);
			double u = det23 * f;
			double v = det31 * f;
			double w = 1.0 - u - v;
			pcoords[0] = u;
			pcoords[1] = v;
			pcoords[2] = w;
		}
		if(xinter && pcoords)
		{
			xinter[0] = pcoords[0]*a[0] + pcoords[1]*b[0] + pcoords[2]*c[0];
			xinter[1] = pcoords[0]*a[1] + pcoords[1]*b[1] + pcoords[2]*c[1];
			xinter[2] = pcoords[0]*a[2] + pcoords[1]*b[2] + pcoords[2]*c[2];
		}
		if(det12 == 0.0) {
			if(det23 == 0.0)
				if(det31 == 0.0)
					return COPLANAR_INTER;
				else
					return NODE2_INTER;
			else if(det31 == 0)
				return NODE1_INTER;
			else return EDGE12_INTER;
		}
		else if(det23 == 0.0) {
			if(det31 == 0.0)
				return NODE3_INTER;
			else return EDGE23_INTER;
		}
		else if(det31 == 0.0) {
			return EDGE31_INTER;
		}
		else return TRI_INTER;
	}

	return NO_INTER;
}


void Triangle::project2d(double p1[2], double p2[2], double p3[2], 
	double pp[2], double &u, double &v)
{
	// u*(x1-x3) + v*(x2-x3) = x-x3
	// u*(y1-y3) + v*(y2-y3) = y-y3

	// u*(y1-y3) + v*(x2-x3)*(y1-y3)/(x1-x3) = (x-x3)*(y1-y3)/(x1-x3)
	// u*(y1-y3) + v*(y2-y3) = y-y3

	// v*(x2-x3)*(y1-y3)/(x1-x3) - v*(y2-y3) = (x-x3)*(y1-y3)/(x1-x3) - (y-y3)

	// v*(x2-x3)*(y1-y3) - v*(y2-y3)*(x1-x3) = (x-x3)*(y1-y3) - (y-y3)*(x1-x3)
	double TINY = 1e-12;
	double det = (p2[0] - p3[0]) * (p1[1] - p3[1]) - (p2[1] - p3[1]) * (p1[0] - p3[0]);
	if(std::fabs(det) < TINY) std::cerr << "triangle is degenerate" << std::endl;

	v = ((pp[0] - p3[0]) * (p1[1] - p3[1]) - (pp[1] - p3[1]) * (p1[0] - p3[0])) / det;
	if(std::fabs(p1[0] - p3[0]) > TINY)
		u = (pp[0] - p3[0] - v * (p2[0] - p3[0])) / (p1[0] - p3[0]);
	else u = (pp[1] - p3[1] - v * (p2[1] - p3[1])) / (p1[1] - p3[1]);
}

//int Triangle::inside( double a[3], double b[3], double c[3], double x[3] )
//{
//	return 0;
//}

int Triangle::inside2d( double p0[3], double p1[3], double p2[3], double x[2] )
{
	double det_abx = orient2d(p0, p1, x);
	double det_bcx = orient2d(p1, p2, x);
	double det_cax = orient2d(p2, p0, x);
	if(det_abx >= 0.0 && det_bcx >= 0.0 && det_cax >= 0.0)
		return 1;
	else if(det_abx <= 0.0 && det_bcx <= 0.0 && det_cax <= 0.0)
		return 1;
	else return 0;
}


int Tetrahedron::inside( double p0[3], double p1[3], double p2[3], double p3[3], double x[3] )
{
	double v1 = orient3d(p1,p2,p3, x);
	double v2 = orient3d(p2,p0,p3, x);
	double v3 = orient3d(p0,p1,p3, x);
	double v4 = orient3d(p0,p2,p1, x);
	if(v1 >= 0.0 && v2 >= 0.0 && v3 >= 0.0 && v4 >= 0.0)
		return 1;
	else if(v1 <= 0.0 && v2 <= 0.0 && v3 <= 0.0 && v4 <= 0.0)
		return 1;
	else return 0;
}


int Segment::inside( double a[3], double b[3], double x[3] )
{
	double t;
	double d = distance(a, b, x, t);
	if(t >= 0.0 && t <= 1.0 && d < 1e-12)
		return (1);
	else
		return 0;
}

double Segment::distance( double p1[3], double p2[3], double p0[3], double &t )
{
	double p12[] = {p1[0]-p2[0], p1[1]-p2[1], p1[2]-p2[2]};
	double p10[] = {p1[0]-p0[0], p1[1]-p0[1], p1[2]-p0[2]};
	double d12sq = Point::dot(p12, p12);
	assert(d12sq > 0.0);

	t = Point::dot(p12, p10) / d12sq;

	double x[3];
	Point::cross(p12, p10, x);
	return Point::norm(x) / std::sqrt(d12sq);
}

int Segment::intersect( double p1[3], double p2[3], 
	double p3[3], double p4[3], 
	double *xa /*= 0*/, double *xb /*= 0*/ )
{
	// Input: Line (p1,p2) and line (p3,p4)
	// Output: point xa on line (p1,p2) and xb on line (p3,p4)
	//         and intersection type

	double det = ::orient3d(p1, p2, p3, p4);
	// if the volume of the tetrahedron (p1,p2,p3,p4) is zero, then 
	// the points are coplanar, or lie at the same position
	if(det == 0.0)
	{
		double EPS = ::getepsilon();
		double p13[3],p43[3],p21[3];
		double d1343,d4321,d1321,d4343,d2121;
		double numer,denom;

		p13[0] = p1[0] - p3[0];
		p13[1] = p1[1] - p3[1];
		p13[2] = p1[2] - p3[2];
		p43[0] = p4[0] - p3[0];
		p43[1] = p4[1] - p3[1];
		p43[2] = p4[2] - p3[2];
		if (std::fabs(p43[0]) < EPS && std::fabs(p43[1]) < EPS && std::fabs(p43[2]) < EPS)
			return DEGENERATE_SEGMENT;
		p21[0] = p2[0] - p1[0];
		p21[1] = p2[1] - p1[1];
		p21[2] = p2[2] - p1[2];
		if (std::fabs(p21[0]) < EPS && std::fabs(p21[1]) < EPS && std::fabs(p21[2]) < EPS)
			return DEGENERATE_SEGMENT;

		d1343 = p13[0] * p43[0] + p13[1] * p43[1] + p13[2] * p43[2];
		d4321 = p43[0] * p21[0] + p43[1] * p21[1] + p43[2] * p21[2];
		d1321 = p13[0] * p21[0] + p13[1] * p21[1] + p13[2] * p21[2];
		d4343 = p43[0] * p43[0] + p43[1] * p43[1] + p43[2] * p43[2];
		d2121 = p21[0] * p21[0] + p21[1] * p21[1] + p21[2] * p21[2];

		denom = d2121 * d4343 - d4321 * d4321;
		if (std::fabs(denom) < EPS)
			return PARALLEL_INTER;
		numer = d1343 * d4321 - d1321 * d4343;

		double mua = numer / denom;
		double mub = (d1343 + d4321 * mua) / d4343;

		if(xa)
		{
			xa[0] = p1[0] + mua * p21[0];
			xa[1] = p1[1] + mua * p21[1];
			xa[2] = p1[2] + mua * p21[2];
		}
		if(xb)
		{
			xb[0] = p3[0] + mub * p43[0];
			xb[1] = p3[1] + mub * p43[1];
			xb[2] = p3[2] + mub * p43[2];
		}

		if(mua >= 0.0 && mua <= 1.0 && 
			mub >= 0.0 && mub <= 1.0)
			return SEGMENT_INTER;
		else
			return NO_INTER;
	}
	return NO_INTER;
}


double Point::norm( const double x[3] )
{
	return std::sqrt(dot(x, x));
}

double Point::dot( const double x[3], const double y[3] )
{
	return x[0]*y[0] + x[1]*y[1] + x[2]*y[2];
}

void Point::cross( const double x[3], const double y[3], double res[3] )
{
	res[0] = x[1] * y[2] - x[2] * y[1];
	res[1] = x[2] * y[0] - x[0] * y[2];
	res[2] = x[0] * y[1] - x[1] * y[0];
}


}
