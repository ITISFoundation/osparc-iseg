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

#include "iSegCore.h"

#include "Pair.h"
#include "Types.h"

#include "Plugin/Point.h"

#include <map>
#include <set>
#include <stdio.h>
#include <vector>

namespace iseg {

typedef Point Point_type;
struct Point_type2
{
	float px;
	float py;
};

class iSegCore_API OutlineLine
{
public:
	void add_point(Point_type P);
	void add_points(std::vector<Point_type>* P_vec);
	void clear();
	unsigned return_length();
	Point_type* return_line();
	FILE* print(FILE* fp);
	FILE* read(FILE* fp);
	void doug_peuck(float epsilon, bool closed);
	void shift_contour(int dx, int dy);

private:
	std::vector<Point_type> line;
	void doug_peuck_sub(float epsilon, const unsigned int p1,
						const unsigned int p2, std::vector<bool>* v1_p);
};

class iSegCore_API OutlineSlice
{
public:
	OutlineSlice();
	OutlineSlice(float thickness);
	void add_line(tissues_size_t tissuetype, std::vector<Point_type>* P_vec,
				  bool outer);
	void add_points(tissues_size_t tissuetype, unsigned short linenr,
					std::vector<Point_type>* P_vec, bool outer);
	void add_point(tissues_size_t tissuetype, unsigned short linenr,
				   Point_type P, bool outer);
	void clear();
	void clear(tissues_size_t tissuetype);
	void clear(tissues_size_t tissuetype, unsigned short linenr, bool outer);
	unsigned short return_nrlines(tissues_size_t tissuetype, bool outer);
	unsigned return_length(tissues_size_t tissuetype, unsigned short linenr,
						   bool outer);
	Point_type* return_line(tissues_size_t tissuetype, unsigned short linenr,
							bool outer);
	FILE* print(FILE* fp, tissues_size_t nr_tissues);
	FILE* read(FILE* fp);
	void set_thickness(float thick);
	float get_thickness();
	void doug_peuck(float epsilon, bool closed);
	void shift_contours(int dx, int dy);
	void insert_tissue_indices(std::set<tissues_size_t>& tissueIndices);

private:
	float thickness;

	typedef std::map<unsigned int, std::vector<OutlineLine>>
		TissueOutlineMap_type;
	TissueOutlineMap_type outer_lines;
	TissueOutlineMap_type inner_lines;
};

class iSegCore_API OutlineSlices
{
public:
	OutlineSlices();
	OutlineSlices(unsigned nrslices);
	OutlineSlices(unsigned nrslices, float thick);
	void set_sizenr(unsigned nrslices);
	void clear();
	void clear(tissues_size_t tissuetype);
	void clear_slice(unsigned slicenr);
	void clear_slice(unsigned slicenr, tissues_size_t tissuetype,
					 unsigned short linenr, bool outer);
	void add_line(unsigned slicenr, tissues_size_t tissuetype,
				  std::vector<Point_type>* P_vec, bool outer);
	void add_points(unsigned slicenr, tissues_size_t tissuetype,
					unsigned short linenr, std::vector<Point_type>* P_vec,
					bool outer);
	void add_point(unsigned slicenr, tissues_size_t tissuetype,
				   unsigned short linenr, Point_type P, bool outer);
	unsigned return_nrslices();
	unsigned short return_nrlines(unsigned slicenr, tissues_size_t tissuetype,
								  bool outer);
	unsigned return_length(unsigned slicenr, tissues_size_t tissuetype,
						   unsigned short linenr, bool outer);
	Point_type* return_line(unsigned slicenr, tissues_size_t tissuetype,
							unsigned short linenr, bool outer);
	int print(const char* filename, tissues_size_t nr_tissues);
	FILE* printprologue(const char* filename, unsigned nr_slices,
						tissues_size_t nr_tissues);
	FILE* printsection(FILE* fp, unsigned startslice, unsigned endslice,
					   unsigned offset, tissues_size_t nr_tissues);
	int read(const char* filename);
	void set_thickness(float thick, unsigned slicenr);
	void set_thickness(float thick);
	float get_thickness(unsigned slicenr);
	void set_pixelsize(float dx1, float dy1);
	Point_type2 get_pixelsize();
	void doug_peuck(float epsilon, bool closed);
	void shift_contours(int dx, int dy);
	void insert_tissue_indices(std::set<tissues_size_t>& tissueIndices);

private:
	std::vector<OutlineSlice> slices;
	float dx;
	float dy;
};

} // namespace iseg
