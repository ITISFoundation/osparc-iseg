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

#include "Outline.h"

namespace iseg {

void OutlineLine::add_point(Point_type P)
{
	line.push_back(P);
}

void OutlineLine::add_points(std::vector<Point_type>* P_vec)
{
	for (auto it = (*P_vec).begin(); it != (*P_vec).end(); it++)
	{
		line.push_back(*it);
	}
}

void OutlineLine::clear()
{
	line.clear();
}

unsigned OutlineLine::return_length() { return (unsigned)line.size(); }

Point_type* OutlineLine::return_line()
{
	unsigned l = (unsigned)line.size();
	Point_type* result = (Point_type*)malloc(sizeof(Point_type) * l);
	for (unsigned i = 0; i < l; i++)
	{
		result[i].px = line[i].px;
		result[i].py = line[i].py;
	}
	return result;
}

FILE* OutlineLine::print(FILE* fp)
{
	unsigned l = (unsigned)line.size();
	fprintf(fp, "%u: ", l);
	//	for(unsigned i=0;i<l;i++) fprintf(fp,"%f,%f ",line[i].px,line[i].py);
	for (unsigned i = 0; i < l; i++)
		fprintf(fp, "%i,%i ", line[i].px, line[i].py);
	/*	for(vector<Point_type>::iterator it=line.begin();it!=line.end();it++)
			fprintf(fp,"%f,%f ",(*it).px,(*it).py);*/
	fprintf(fp, "\n");

	return fp;
}

FILE* OutlineLine::read(FILE* fp)
{
	unsigned l;
	fscanf(fp, "%u: ", &l);
	Point_type p;
	int i1, i2;
	line.clear();
	for (unsigned i = 0; i < l; i++)
	{
		//		fscanf(fp,"%f,%f ",&p.px,&p.py);
		fscanf(fp, "%i,%i ", &i1, &i2);
		p.px = (short)i1;
		p.py = (short)i2;
		line.push_back(p);
	}

	return fp;
}

void OutlineLine::doug_peuck(float epsilon, bool /*closed*/)
{
	unsigned int n = (unsigned int)line.size();
	if (n > 2)
	{
		std::vector<bool> v1;
		v1.resize(n);

		for (unsigned int i = 0; i < n; i++)
			v1[i] = false;
		v1[0] = true;
		v1[n - 1] = true;
		doug_peuck_sub(epsilon, 0, n - 1, &v1);

		auto it = line.begin();
		for (unsigned int i = 0; i < n; i++)
		{
			if (!v1[i])
				it = line.erase(it);
			else
				it++;
		}
	}
	return;
}

void OutlineLine::doug_peuck_sub(float epsilon, const unsigned int p1,
		const unsigned int p2, std::vector<bool>* v1_p)
{
	//	cout << p1<<" "<<p2<<endl;
	if (p2 <= p1 + 1)
		return;
	float dv, l;
	float max_dist = 0;
	Point SIJ, SIV;
	unsigned int max_pos = p1;

	SIJ.px = line[p2].px - line[p1].px;
	SIJ.py = line[p2].py - line[p1].py;

	l = dist(&SIJ);

	//	cout << l << endl;

	for (unsigned int i = p1 + 1; i < p2; i++)
	{
		SIV.px = line[i].px - line[p1].px;
		SIV.py = line[i].py - line[p1].py;
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
}

void OutlineLine::shift_contour(int dx, int dy)
{
	unsigned l = (unsigned)line.size();
	for (unsigned i = 0; i < l; i++)
	{
		line[i].px += dx;
		line[i].py += dy;
	}
}

//-------------------------------------------------------------------------------
OutlineSlice::OutlineSlice() { thickness = 1; }

OutlineSlice::OutlineSlice(float thick) { thickness = thick; }

void OutlineSlice::add_line(tissues_size_t tissuetype,
		std::vector<Point_type>* P_vec, bool outer)
{
	OutlineLine ol;
	//	ol.add_points(P_vec);
	if (outer)
	{
		outer_lines[tissuetype].push_back(ol);
		(*(--outer_lines[tissuetype].end())).add_points(P_vec);
	}
	else
	{
		inner_lines[tissuetype].push_back(ol);
		(*(--inner_lines[tissuetype].end())).add_points(P_vec);
	}
}

void OutlineSlice::add_points(tissues_size_t tissuetype, unsigned short linenr,
		std::vector<Point_type>* P_vec, bool outer)
{
	if (outer)
	{
		(outer_lines[tissuetype])[linenr].add_points(P_vec);
	}
	else
	{
		(inner_lines[tissuetype])[linenr].add_points(P_vec);
	}
}

void OutlineSlice::add_point(tissues_size_t tissuetype, unsigned short linenr,
		Point_type P, bool outer)
{
	if (outer)
	{
		(outer_lines[tissuetype])[linenr].add_point(P);
	}
	else
	{
		(inner_lines[tissuetype])[linenr].add_point(P);
	}
}

void OutlineSlice::clear()
{
	outer_lines.clear();
	inner_lines.clear();
}

void OutlineSlice::clear(tissues_size_t tissuetype)
{
	outer_lines.erase(tissuetype);
	inner_lines.erase(tissuetype);
}

void OutlineSlice::clear(tissues_size_t tissuetype, unsigned short linenr,
		bool outer)
{
	if (outer)
	{
		if (outer_lines.find(tissuetype) != outer_lines.end())
		{
			outer_lines[tissuetype].erase(outer_lines[tissuetype].begin() +
																		linenr);
		}
	}
	else if (inner_lines.find(tissuetype) != inner_lines.end())
	{
		inner_lines[tissuetype].erase(inner_lines[tissuetype].begin() + linenr);
	}
}

unsigned short OutlineSlice::return_nrlines(tissues_size_t tissuetype,
		bool outer)
{
	if (outer)
	{
		if (outer_lines.find(tissuetype) != outer_lines.end())
		{
			return (unsigned short)(outer_lines[tissuetype].size());
		}
	}
	else if (inner_lines.find(tissuetype) != inner_lines.end())
	{
		return (unsigned short)(inner_lines[tissuetype].size());
	}
	return 0;
}

unsigned OutlineSlice::return_length(tissues_size_t tissuetype,
		unsigned short linenr, bool outer)
{
	if (outer)
	{
		if (outer_lines.find(tissuetype) != outer_lines.end())
		{
			return (
					unsigned)((outer_lines[tissuetype])[linenr].return_length());
		}
	}
	else if (inner_lines.find(tissuetype) != inner_lines.end())
	{
		return (unsigned)((inner_lines[tissuetype])[linenr].return_length());
	}
	return 0;
}

Point_type* OutlineSlice::return_line(tissues_size_t tissuetype,
		unsigned short linenr, bool outer)
{
	if (outer)
	{
		if (outer_lines.find(tissuetype) != outer_lines.end())
		{
			return outer_lines[tissuetype][linenr].return_line();
		}
	}
	else if (inner_lines.find(tissuetype) != inner_lines.end())
	{
		return inner_lines[tissuetype][linenr].return_line();
	}
	return nullptr;
}

FILE* OutlineSlice::print(FILE* fp, tissues_size_t nr_tissues)
{
	fprintf(fp, "Thick: %f\n", thickness);

	if (nr_tissues > 255)
	{ // Only print tissue indices which contain outlines

		// Collect used tissue indices in ascending order
		std::set<tissues_size_t> tissueIndices;
		insert_tissue_indices(tissueIndices);

		tissues_size_t currTissueIdx;
		std::vector<OutlineLine>::iterator itVec;
		for (auto itTissueIdx = tissueIndices.begin(); itTissueIdx != tissueIndices.end(); ++itTissueIdx)
		{
			currTissueIdx = *itTissueIdx;

			// Print outer lines
			if (outer_lines.find(currTissueIdx) != outer_lines.end())
			{
				fprintf(fp, "T%u O%u\n", currTissueIdx,
						(unsigned)outer_lines[currTissueIdx].size());
				for (itVec = outer_lines[currTissueIdx].begin();
						 itVec != outer_lines[currTissueIdx].end(); ++itVec)
					(*itVec).print(fp);
			}
			else
			{
				fprintf(fp, "T%u O%u\n", currTissueIdx, 0);
			}

			// Print inner lines
			if (inner_lines.find(currTissueIdx) != inner_lines.end())
			{
				fprintf(fp, "T%u I%u\n", currTissueIdx,
						(unsigned)inner_lines[currTissueIdx].size());
				for (itVec = inner_lines[currTissueIdx].begin();
						 itVec != inner_lines[currTissueIdx].end(); ++itVec)
					(*itVec).print(fp);
			}
			else
			{
				fprintf(fp, "T%u I%u\n", currTissueIdx, 0);
			}
		}
	}
	else
	{ // Print all tissue indices within range of unsigned char

		std::vector<OutlineLine>::iterator itVec;
		for (unsigned short i = 0; i < 256; ++i)
		{
			// Print outer lines
			if (outer_lines.find(i) != outer_lines.end())
			{
				fprintf(fp, "T%u O%u\n", i, (unsigned)outer_lines[i].size());
				for (itVec = outer_lines[i].begin();
						 itVec != outer_lines[i].end(); ++itVec)
					(*itVec).print(fp);
			}
			else
			{
				fprintf(fp, "T%u O%u\n", i, 0);
			}

			// Print inner lines
			if (inner_lines.find(i) != inner_lines.end())
			{
				fprintf(fp, "T%u I%u\n", i, (unsigned)inner_lines[i].size());
				for (itVec = inner_lines[i].begin();
						 itVec != inner_lines[i].end(); ++itVec)
					(*itVec).print(fp);
			}
			else
			{
				fprintf(fp, "T%u I%u\n", i, 0);
			}
		}
	}

	return fp;
}

FILE* OutlineSlice::read(FILE* fp)
{
	unsigned j, l;

	fscanf(fp, "Thick: %f\n", &thickness);

	//	for(unsigned i=0;i<nr_tissuetypes;i++){
	//		fscanf(fp,"T%u O%u\n",&j,&l);
	FILE* fphold = fp;
	while (fscanf(fp, "T%u O%u\n", &j, &l) == 2)
	{
		for (unsigned k = 0; k < l; k++)
		{
			OutlineLine ol;
			//			ol.read(fp);
			outer_lines[j].push_back(ol);
			(*(--outer_lines[j].end())).read(fp);
		}
		fscanf(fp, "T%u I%u\n", &j, &l);
		for (unsigned k = 0; k < l; k++)
		{
			OutlineLine ol;
			//			ol.read(fp);
			inner_lines[j].push_back(ol);
			(*(--inner_lines[j].end())).read(fp);
		}
		fphold = fp;
	}

	fp = fphold;
	return fp;
}

void OutlineSlice::set_thickness(float thick) { thickness = thick; }

float OutlineSlice::get_thickness() { return thickness; }

void OutlineSlice::doug_peuck(float epsilon, bool closed)
{
	TissueOutlineMap_type::iterator itTissue;
	std::vector<OutlineLine>::iterator itOutline;
	for (itTissue = outer_lines.begin(); itTissue != outer_lines.end();
			 ++itTissue)
	{
		for (itOutline = itTissue->second.begin();
				 itOutline != itTissue->second.end(); ++itOutline)
		{
			itOutline->doug_peuck(epsilon, closed);
		}
	}
	for (itTissue = inner_lines.begin(); itTissue != inner_lines.end();
			 ++itTissue)
	{
		for (itOutline = itTissue->second.begin();
				 itOutline != itTissue->second.end(); ++itOutline)
		{
			itOutline->doug_peuck(epsilon, closed);
		}
	}
}

void OutlineSlice::shift_contours(int dx, int dy)
{
	TissueOutlineMap_type::iterator itTissue;
	std::vector<OutlineLine>::iterator itOutline;
	for (itTissue = outer_lines.begin(); itTissue != outer_lines.end();
			 ++itTissue)
	{
		for (itOutline = itTissue->second.begin();
				 itOutline != itTissue->second.end(); ++itOutline)
		{
			itOutline->shift_contour(dx, dy);
		}
	}
	for (itTissue = inner_lines.begin(); itTissue != inner_lines.end();
			 ++itTissue)
	{
		for (itOutline = itTissue->second.begin();
				 itOutline != itTissue->second.end(); ++itOutline)
		{
			itOutline->shift_contour(dx, dy);
		}
	}
}

void OutlineSlice::insert_tissue_indices(std::set<tissues_size_t>& tissueIndices)
{
	// Inserts tissue indices contained in the outline into the set (ascending order)
	TissueOutlineMap_type::iterator itTissue;
	for (itTissue = outer_lines.begin(); itTissue != outer_lines.end();
			 ++itTissue)
	{
		tissueIndices.insert(itTissue->first);
	}
	for (itTissue = inner_lines.begin(); itTissue != inner_lines.end();
			 ++itTissue)
	{
		tissueIndices.insert(itTissue->first);
	}
}

//-------------------------------------------------------------------------------
OutlineSlices::OutlineSlices(unsigned nrslices)
{
	dx = dy = 1;
	slices.resize(nrslices);
	return;
}

OutlineSlices::OutlineSlices()
{
	dx = dy = 1;
	return;
}

OutlineSlices::OutlineSlices(unsigned nrslices, float thick)
{
	slices.resize(nrslices);
	for (unsigned i = 0; i < nrslices; i++)
		slices[i].set_thickness(thick);
	return;
}

void OutlineSlices::set_sizenr(unsigned nrslices)
{
	slices.resize(nrslices);
	return;
}

void OutlineSlices::clear()
{
	for (unsigned i = 0; i < (unsigned)slices.size(); i++)
		slices[i].clear();
	return;
}

void OutlineSlices::clear(tissues_size_t tissuetype)
{
	for (unsigned i = 0; i < (unsigned)slices.size(); i++)
		slices[i].clear(tissuetype);
	return;
}

void OutlineSlices::clear_slice(unsigned slicenr)
{
	slices[slicenr].clear();
	return;
}

void OutlineSlices::clear_slice(unsigned slicenr, tissues_size_t tissuetype,
		unsigned short linenr, bool outer)
{
	slices[slicenr].clear(tissuetype, linenr, outer);
	return;
}

void OutlineSlices::add_line(unsigned slicenr, tissues_size_t tissuetype,
		std::vector<Point_type>* P_vec, bool outer)
{
	slices[slicenr].add_line(tissuetype, P_vec, outer);
	return;
}

void OutlineSlices::add_points(unsigned slicenr, tissues_size_t tissuetype,
		unsigned short linenr, std::vector<Point_type>* P_vec,
		bool outer)
{
	slices[slicenr].add_points(tissuetype, linenr, P_vec, outer);
	return;
}

void OutlineSlices::add_point(unsigned slicenr, tissues_size_t tissuetype,
		unsigned short linenr, Point_type P, bool outer)
{
	slices[slicenr].add_point(tissuetype, linenr, P, outer);
	return;
}

unsigned OutlineSlices::return_nrslices() { return (unsigned)slices.size(); }

unsigned short OutlineSlices::return_nrlines(unsigned slicenr,
		tissues_size_t tissuetype,
		bool outer)
{
	return slices[slicenr].return_nrlines(tissuetype, outer);
}

unsigned OutlineSlices::return_length(unsigned slicenr,
		tissues_size_t tissuetype,
		unsigned short linenr, bool outer)
{
	return slices[slicenr].return_length(tissuetype, linenr, outer);
}

Point_type* OutlineSlices::return_line(unsigned slicenr,
		tissues_size_t tissuetype,
		unsigned short linenr, bool outer)
{
	return slices[slicenr].return_line(tissuetype, linenr, outer);
}

int OutlineSlices::print(const char* filename, tissues_size_t nr_tissues)
{
	unsigned nr_slices = (unsigned)slices.size();

	FILE* fp = printprologue(filename, nr_slices, nr_tissues);

	if (fp == nullptr)
		return 0;

	for (unsigned i = 0; i < nr_slices; i++)
	{
		fprintf(fp, "S%u\n", i);
		slices[i].print(fp, nr_tissues);
	}

	fclose(fp);
	return 1;
}

FILE* OutlineSlices::printprologue(const char* filename, unsigned nr_slices,
		tissues_size_t nr_tissues)
{
	FILE* fp;

	if ((fp = fopen(filename, "w")) != nullptr)
	{
		if (nr_tissues > 255)
		{
			// Print version number (1)
			fprintf(fp, "V%u\n", 1);
		}
		fprintf(fp, "NS%u\n", nr_slices);
		fprintf(fp, "PS%f %f\n", dx, dy);
	}

	return fp;
}

FILE* OutlineSlices::printsection(FILE* fp, unsigned startslice,
		unsigned endslice, unsigned offset,
		tissues_size_t nr_tissues)
{
	for (unsigned i = startslice; i <= endslice; i++)
	{
		fprintf(fp, "S%u\n", i + offset);
		slices[i].print(fp, nr_tissues);
	}

	return fp;
}

int OutlineSlices::read(const char* filename)
{
	FILE* fp;

	if ((fp = fopen(filename, "r")) == nullptr)
		return 0;

	unsigned j, k, version;

	// Try reading version number
	int c = fgetc(fp);
	if (c == 'V')
	{
		// Read version number
		fscanf(fp, "%u\n", &version);

		// Read number of slices
		fscanf(fp, "NS%u\n", &j);
	}
	else
	{
		// First character was 'N'
		version = 0;

		// Read number of slices
		fscanf(fp, "S%u\n", &j);
	}

	fscanf(fp, "PS%f %f\n", &dx, &dy);
	slices.clear();
	slices.resize(j);

	for (unsigned i = 0; i < j; i++)
	{
		fscanf(fp, "S%u\n", &k);
		slices[i].read(fp);
	}

	fclose(fp);
	return 1;
}

void OutlineSlices::set_thickness(float thick, unsigned slicenr)
{
	slices[slicenr].set_thickness(thick);
	return;
}

void OutlineSlices::set_thickness(float thick)
{
	for (unsigned i = 0; i < (unsigned)slices.size(); i++)
		slices[i].set_thickness(thick);
	return;
}

void OutlineSlices::set_pixelsize(float dx1, float dy1)
{
	dx = dx1;
	dy = dy1;
}

float OutlineSlices::get_thickness(unsigned slicenr)
{
	return slices[slicenr].get_thickness();
}

Point_type2 OutlineSlices::get_pixelsize()
{
	Point_type2 p;
	p.px = dx;
	p.py = dy;
	return p;
}

void OutlineSlices::doug_peuck(float epsilon, bool closed)
{
	unsigned n = (unsigned)slices.size();
	for (unsigned i = 0; i < n; i++)
	{
		slices[i].doug_peuck(epsilon, closed);
	}

	return;
}

void OutlineSlices::shift_contours(int dx, int dy)
{
	unsigned n = (unsigned)slices.size();
	for (unsigned i = 0; i < n; i++)
	{
		slices[i].shift_contours(dx, dy);
	}
}

void OutlineSlices::insert_tissue_indices(std::set<tissues_size_t>& tissueIndices)
{
	unsigned n = (unsigned)slices.size();
	for (unsigned i = 0; i < n; i++)
	{
		slices[i].insert_tissue_indices(tissueIndices);
	}
}

} // namespace iseg
