/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef GEOMETRIC_PREDICATES_H
#define GEOMETRIC_PREDICATES_H

/**
This class wraps up several useful geometrical tests, which use the 
predicates by Jonathan Shewchuk.

Original author: Bryn Lloyd, Sept. 2011

TODO: use return codes for inside function
*/
namespace GeometricPredicates {
// Call this function before you use any of the exact arithmetic
// functions below.
double exactinit();

// get the machine epsilon computed in exactinit
double getepsilon();

// Compute the signed volume (orientation) of a 2d triangle
double orient2d(double a[2], double b[2], double c[2]);

// Compute the signed volume (orientation) of a tetrahedron
// This function assumes the three first points are oriented in
// anti-clockwise direction, viewed from the fourth. It returns
// the opposite sign to the version by Shewchuk
double orient3d(double a[3], double b[3], double c[3], double d[3]);

struct Point
{
	// These functions are not exact
	static double norm(const double x[3]);
	static double dot(const double x[3], const double y[3]);
	static void cross(const double x[3], const double y[3], double result[3]);
};

struct Segment
{
	enum SegmentSegmentIntersection {
		NO_INTER = 0,					 /* no intersection */
		SEGMENT_INTER = 1,		 /* segments intersect */
		PARALLEL_INTER = 2,		 /* parallel (might not intersect) */
		DEGENERATE_SEGMENT = 3 /* zero length segment */
	};

	// Compute if a point is inside a segment
	// (not exact)
	static int inside(double a[3], double b[3], double x[3]);

	// Compute the distance of a point x to a line through (a,b)
	// (not exact)
	static double distance(double a[3], double b[3], double x[3], double &t);

	// Compute the intersection between two line segments (a,b) and (c,d)
	// Optionally return points on segments closest to each other
	// (if there is an intersection, these are identical)
	// The intersection type uses exact arithmetic, the positions are
	// not exact.
	static int intersect(double a[3], double b[3], double c[3], double d[3],
											 double *x1 = 0, double *x2 = 0);
};

struct Triangle
{
	enum TriangleSegmentIntersection {
		NO_INTER = 0,					 /* no intersection */
		TRI_INTER = 1,				 /* intersection inside triangle */
		EDGE12_INTER = 2,			 /* intersection on edge (1,2) */
		EDGE23_INTER = 3,			 /* intersection on edge (2,3) */
		EDGE31_INTER = 4,			 /* intersection on edge (3,1) */
		NODE1_INTER = 5,			 /* intersection at node 1 */
		NODE2_INTER = 6,			 /* intersection at node 2 */
		NODE3_INTER = 7,			 /* intersection at node 3 */
		COPLANAR_INTER = 8,		 /* coplanar (might not intersect) */
		DEGENERATE_SEGMENT = 9 /* zero length segment */
		// TODO: DEGENERATE_TRIANGLE = 10 /* zero area triangle */
	};

	// Compute the intersection between the triangle (a,b,c) and the
	// line (xa,xe). Optionally, the intersection point p (barycentric
	// coordinates) is computed
	// Returns an integer, which says more about what kind of intersection
	// occurred (exact). Optional return values are
	//  - pcoords: the barycentric coordinates (not exact)
	//  - x:       intersection point (not exact)
	static int intersect(double a[3], double b[3], double c[3], double xa[3],
											 double xe[3], double *pcoords = 0, double *x = 0);

	// Compute if a point is inside a 3d triangle (exact)
	static int inside(double a[3], double b[3], double c[3], double x[3]);

	// Compute if a 2d point is inside a 2d triangle (not exact)
	static int inside2d(double a[2], double b[2], double c[2], double x[2]);

	// Project point in 3D onto triangle in x-y plane. Barycentric coordinates
	// are returned in u and v (not exact).
	// u = 1 corresponds to p1, v = 1 corresponds to p2
	static void project2d(double a[2], double b[2], double c[2], double pp[2],
												double &u, double &v);
};

struct Tetrahedron
{
	// Compute if a point x is inside a tetrahedron (exact)
	static int inside(double a[3], double b[3], double c[3], double d[3],
										double x[3]);
};
} // namespace GeometricPredicates

#endif
