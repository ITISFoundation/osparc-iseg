/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include <stdio.h>
#include <iostream>
#include <vector>
#include <cassert>

#include "GeometricPredicates.h"

using namespace std;
using namespace GeometricPredicates;

#define myassert(expr, value) \
	if(!(expr)) { \
	std::cerr << value << ": assertion failed: " << __FILE__<< "(" << __LINE__ << ")" << std::endl; \
	error_cnt++; \
	}

int main(int argc, char **argv)
{
	int error_cnt = 0;

	double p1[3] = {0.0,0.0,0.0};
	double p2[3] = {1.0,0.0,0.0};
	double p3[3] = {0.0,1.0,0.0};

	//
	// Test segment triangle intersections
	//
	GeometricPredicates::exactinit();
	const double epsd   = 2.22045e-16;
	const double eps    = epsd;

	// intersect inside
	double a0[3] = {0.2,0.2,-1.0};
	double b0[3] = {0.2,0.2,1.0};
	double p[3], x[3];
	int inter = Triangle::intersect(p1,p2,p3, a0, b0, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(inter == Triangle::TRI_INTER, 1);

	a0[2] = 0.0;
	inter = Triangle::intersect(p1,p2,p3, a0, b0, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(inter == Triangle::TRI_INTER, 2);

	a0[2] = eps;
	inter = Triangle::intersect(p1,p2,p3, a0, b0, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(!inter, 3);

	a0[2] = b0[2] = 0.0;
	b0[1] = 2.0;
	inter = Triangle::intersect(p1,p2,p3, a0, b0, p, x);
	printf("inter=%d   p=%g %g %g (co-planar case!)\n",inter,p[0],p[1],p[2]);
	myassert(inter == Triangle::COPLANAR_INTER, 4);

	// test intersection with point p3
	double a1[3] = {0.0,1.0,0.0};
	double b1[3] = {0.0,1.0,1.0};
	inter = Triangle::intersect(p1,p2,p3, a1, b1, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(inter == Triangle::NODE3_INTER, 5);

	double a2[3] = {0.0,1.0,eps};
	double b2[3] = {-1.0,1.0,1.0};
	inter = Triangle::intersect(p1,p2,p3, a2, b2, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(!inter, 6);

	// test intersection with point p1
	double ak[3] = {0.0,0.0,0.0};
	double bk[3] = {0.0,0.0,1.0};
	inter = Triangle::intersect(p1,p2,p3, ak, bk, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(inter == Triangle::NODE1_INTER, 7);


	// intersect at edge 31
	double a3[3] = {0.0,0.5,0.0};
	double b3[3] = {0.0,5e-1,1.0};
	inter = Triangle::intersect(p1,p2,p3, a3, b3, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(inter == Triangle::EDGE31_INTER, 8);

	double a4[3] = {0.0,5e-1,eps};
	double b4[3] = {0.0,5e-1,1.0};
	inter = Triangle::intersect(p1,p2,p3, a4, b4, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(!inter, 9);


	// intersect at edge 12
	double a5[3] = {5e-1,0.0,0.0};
	double b5[3] = {5e-1,0.0,1.0};
	inter = Triangle::intersect(p1,p2,p3, a5, b5, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(inter == Triangle::EDGE12_INTER, 10);

	double a6[3] = {5e-1,0.0,eps};
	double b6[3] = {5e-1,0.0,1.0};
	inter = Triangle::intersect(p1,p2,p3, a6, b6, p, x);
	printf("inter=%d   p=%g %g %g\n",inter,p[0],p[1],p[2]);
	myassert(!inter, 11);


	// share edge 12
	inter = Triangle::intersect(p1,p2,p3, p1, p2, p, x);
	printf("inter=%d   p=%g %g %g (shared edge case!)\n",inter,p[0],p[1],p[2]);
	// share edge 13
	inter = Triangle::intersect(p1,p2,p3, p1, p3, p, x);
	printf("inter=%d   p=%g %g %g (shared edge case!)\n",inter,p[0],p[1],p[2]);
	// share edge 23
	inter = Triangle::intersect(p1,p2,p3, p2, p3, p, x);
	printf("inter=%d   p=%g %g %g (shared edge case!)\n",inter,p[0],p[1],p[2]);

	double p1m[3] = {0.0,-0.1,0.0};
	double p2m[3] = {1.0,-0.1,0.0};
	double p3m[3] = {0.0,-1.1,0.0};
	// TODO
	// These cases actually fail, since they return a COPLANAR_INTER, 
	// even for non intersecting triangle-edge combinations
	inter = Triangle::intersect(p1,p2,p3, p1m, p2m, p, x);
	printf("inter=%d   p=%g %g %g (parallel edge case!)\n",inter,p[0],p[1],p[2]);
	inter = Triangle::intersect(p1,p2,p3, p1m, p3m, p, x);
	printf("inter=%d   p=%g %g %g (parallel edge case!)\n",inter,p[0],p[1],p[2]);
	inter = Triangle::intersect(p1,p2,p3, p2m, p3m, p, x);
	printf("inter=%d   p=%g %g %g (parallel edge case!)\n",inter,p[0],p[1],p[2]);

	// This case illustrates a different test, copied from tetgen!
	//double p4[3] = {0.0,-1.0,0.0};
	//Triangle tri2(p1,p2,p4);
	//inter = tri.intersect(tri2);
	//printf("does inter? => %d   (shared edge case!)\n",inter);
	//myassert(!inter);



	//
	// Segment intersection tests
	//

	double xa[3], xb[3];
	inter = Segment::intersect(p1,p2, p1,p2, xa, xb);
	printf("inter=%d   parallel edge case\n",inter);
	myassert(inter == Segment::PARALLEL_INTER, 11);

	inter = Segment::intersect(p1,p2, p1,p3, xa, xb);
	printf("inter=%d   intersect at node case %g %g %g\n",inter, x[0],x[1],x[2]);
	myassert(inter == Segment::SEGMENT_INTER, 12);

	inter = Segment::intersect(p1,p2, p2,p3, xa);
	printf("inter=%d   intersect at node case %g %g %g\n",inter, x[0],x[1],x[2]);
	myassert(inter == Segment::SEGMENT_INTER, 13);


	inter = Segment::intersect(p1,p2, p1m,p2m, xa, xb);
	printf("inter=%d   parallel edge case\n",inter);
	myassert(inter == Segment::PARALLEL_INTER, 14);

	inter = Segment::intersect(p1,p2, p1m,p3m, xa, xb);
	printf("inter=%d   no intersection case\n",inter);
	myassert(inter == Segment::NO_INTER, 15);

	inter = Segment::intersect(p1,p2, p2m,p3m, xa, xb);
	printf("inter=%d   no intersection case\n",inter);
	myassert(inter == Segment::NO_INTER, 16);

	inter = Segment::intersect(p1,p2, p2m,p2m, xa, xb);
	printf("inter=%d   degenerate segment case\n",inter);
	myassert(inter == Segment::DEGENERATE_SEGMENT, 17);

	inter = Segment::intersect(p1,p1, p1m,p2m, xa, xb);
	printf("inter=%d   degenerate segment case\n",inter);
	myassert(inter == Segment::DEGENERATE_SEGMENT, 18);


	//
	// Tetrahedron inside tests
	//
	double x1[3] = {0.0,0.0,0.0};
	double x2[3] = {1.0,0.0,0.0};
	double x3[3] = {0.0,1.0,0.0};
	double x4[3] = {0.0,0.0,1.0};

	// on corner or edge or face
	double c1[3] = {0.0,0.0,0.0};
	double c2[3] = {1.0,0.0,0.0};
	double e12[3] = {0.5,0.0,0.0};
	double e23[3] = {0.5,0.5,0.0};
	double f123[3] = {0.25,0.25,0.0};
	double f124[3] = {0.25,0.0,0.25};
	// inside
	double in1[3] = {0.1,0.1,1e-6};
	// outside
	double out1[3] = {0.1,0.1,-1e-6};
	myassert(1 == Tetrahedron::inside(x1, x2, x3, x4, c1),  19);
	myassert(1 == Tetrahedron::inside(x1, x2, x3, x4, c2),  20);
	myassert(1 == Tetrahedron::inside(x1, x2, x3, x4, e12), 21);
	myassert(1 == Tetrahedron::inside(x1, x2, x3, x4, e23), 22);
	myassert(1 == Tetrahedron::inside(x1, x2, x3, x4, in1), 23);
	myassert(0 == Tetrahedron::inside(x1, x2, x3, x4, out1), 24);
	myassert(0 == Tetrahedron::inside(x1, x2, x3, x4, out1), 25);
	myassert(0 == Tetrahedron::inside(x1, x2, x3, x4, out1), 26);
	myassert(0 == Tetrahedron::inside(x1, x2, x3, x4, out1), 27);
	myassert(1 == Tetrahedron::inside(x1, x2, x3, x4, f123), 28);
	myassert(1 == Tetrahedron::inside(x1, x2, x3, x4, f124), 29);

	if(error_cnt > 0) printf("Test failed\n");
	else printf("Test succeeded\n");
	return error_cnt;
}
