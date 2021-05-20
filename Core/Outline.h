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

#include "iSegCore.h"

#include "Pair.h"

#include "Data/Point.h"
#include "Data/Types.h"

#include <cstdio>
#include <map>
#include <set>
#include <vector>

namespace iseg {

using point_type = Point;
struct PointType2
{
	float px, py; // NOLINT
};

class ISEG_CORE_API OutlineLine
{
public:
	void AddPoint(point_type P);
	void AddPoints(std::vector<point_type>* P_vec);
	void Clear();
	unsigned ReturnLength();
	point_type* ReturnLine();
	FILE* Print(FILE* fp);
	FILE* Read(FILE* fp);
	void DougPeuck(float epsilon, bool closed);
	void ShiftContour(int dx, int dy);

private:
	std::vector<point_type> m_Line;
	void DougPeuckSub(float epsilon, const unsigned int p1, const unsigned int p2, std::vector<bool>* v1_p);
};

class ISEG_CORE_API OutlineSlice
{
public:
	OutlineSlice();
	OutlineSlice(float thickness);
	void AddLine(tissues_size_t tissuetype, std::vector<point_type>* P_vec, bool outer);
	void AddPoints(tissues_size_t tissuetype, unsigned short linenr, std::vector<point_type>* P_vec, bool outer);
	void AddPoint(tissues_size_t tissuetype, unsigned short linenr, point_type P, bool outer);
	void Clear();
	void Clear(tissues_size_t tissuetype);
	void Clear(tissues_size_t tissuetype, unsigned short linenr, bool outer);
	unsigned short ReturnNrlines(tissues_size_t tissuetype, bool outer);
	unsigned ReturnLength(tissues_size_t tissuetype, unsigned short linenr, bool outer);
	point_type* ReturnLine(tissues_size_t tissuetype, unsigned short linenr, bool outer);
	FILE* Print(FILE* fp, tissues_size_t nr_tissues);
	FILE* Read(FILE* fp);
	void SetThickness(float thick);
	float GetThickness() const;
	void DougPeuck(float epsilon, bool closed);
	void ShiftContours(int dx, int dy);
	void InsertTissueIndices(std::set<tissues_size_t>& tissueIndices);

private:
	float m_Thickness;

	using TissueOutlineMap_type = std::map<unsigned int, std::vector<OutlineLine>>;
	TissueOutlineMap_type m_OuterLines;
	TissueOutlineMap_type m_InnerLines;
};

class ISEG_CORE_API OutlineSlices
{
public:
	OutlineSlices();
	OutlineSlices(unsigned nrslices);
	OutlineSlices(unsigned nrslices, float thick);
	void SetSizenr(unsigned nrslices);
	void Clear();
	void Clear(tissues_size_t tissuetype);
	void ClearSlice(unsigned slicenr);
	void ClearSlice(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, bool outer);
	void AddLine(unsigned slicenr, tissues_size_t tissuetype, std::vector<point_type>* P_vec, bool outer);
	void AddPoints(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, std::vector<point_type>* P_vec, bool outer);
	void AddPoint(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, point_type P, bool outer);
	unsigned ReturnNrslices();
	unsigned short ReturnNrlines(unsigned slicenr, tissues_size_t tissuetype, bool outer);
	unsigned ReturnLength(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, bool outer);
	point_type* ReturnLine(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, bool outer);
	int Print(const char* filename, tissues_size_t nr_tissues);
	FILE* Printprologue(const char* filename, unsigned nr_slices, tissues_size_t nr_tissues) const;
	FILE* Printsection(FILE* fp, unsigned startslice, unsigned endslice, unsigned offset, tissues_size_t nr_tissues);
	int Read(const char* filename);
	void SetThickness(float thick, unsigned slicenr);
	void SetThickness(float thick);
	float GetThickness(unsigned slicenr);
	void SetPixelsize(float dx1, float dy1);
	PointType2 GetPixelsize() const;
	void DougPeuck(float epsilon, bool closed);
	void ShiftContours(int dx, int dy);
	void InsertTissueIndices(std::set<tissues_size_t>& tissueIndices);

private:
	std::vector<OutlineSlice> m_Slices;
	float m_Dx;
	float m_Dy;
};

} // namespace iseg
