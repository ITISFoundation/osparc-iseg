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

#include "Outline.h"

namespace iseg {

void OutlineLine::AddPoint(point_type P)
{
	m_Line.push_back(P);
}

void OutlineLine::AddPoints(std::vector<point_type>* P_vec)
{
	for (auto it = (*P_vec).begin(); it != (*P_vec).end(); it++)
	{
		m_Line.push_back(*it);
	}
}

void OutlineLine::Clear()
{
	m_Line.clear();
}

unsigned OutlineLine::ReturnLength() { return (unsigned)m_Line.size(); }

point_type* OutlineLine::ReturnLine()
{
	unsigned l = (unsigned)m_Line.size();
	point_type* result = (point_type*)malloc(sizeof(point_type) * l);
	for (unsigned i = 0; i < l; i++)
	{
		result[i].px = m_Line[i].px;
		result[i].py = m_Line[i].py;
	}
	return result;
}

FILE* OutlineLine::Print(FILE* fp)
{
	unsigned l = (unsigned)m_Line.size();
	fprintf(fp, "%u: ", l);
	//	for(unsigned i=0;i<l;i++) fprintf(fp,"%f,%f ",line[i].px,line[i].py);
	for (unsigned i = 0; i < l; i++)
		fprintf(fp, "%i,%i ", m_Line[i].px, m_Line[i].py);
	/*	for(vector<point_type>::iterator it=line.begin();it!=line.end();it++)
			fprintf(fp,"%f,%f ",(*it).px,(*it).py);*/
	fprintf(fp, "\n");

	return fp;
}

FILE* OutlineLine::Read(FILE* fp)
{
	unsigned l;
	fscanf(fp, "%u: ", &l);
	point_type p;
	int i1, i2;
	m_Line.clear();
	for (unsigned i = 0; i < l; i++)
	{
		//		fscanf(fp,"%f,%f ",&p.px,&p.py);
		fscanf(fp, "%i,%i ", &i1, &i2);
		p.px = (short)i1;
		p.py = (short)i2;
		m_Line.push_back(p);
	}

	return fp;
}

void OutlineLine::DougPeuck(float epsilon, bool /*closed*/)
{
	unsigned int n = (unsigned int)m_Line.size();
	if (n > 2)
	{
		std::vector<bool> v1;
		v1.resize(n);

		for (unsigned int i = 0; i < n; i++)
			v1[i] = false;
		v1[0] = true;
		v1[n - 1] = true;
		DougPeuckSub(epsilon, 0, n - 1, &v1);

		auto it = m_Line.begin();
		for (unsigned int i = 0; i < n; i++)
		{
			if (!v1[i])
				it = m_Line.erase(it);
			else
				it++;
		}
	}
}

void OutlineLine::DougPeuckSub(float epsilon, const unsigned int p1, const unsigned int p2, std::vector<bool>* v1_p)
{
	//	cout << p1<<" "<<p2<<endl;
	if (p2 <= p1 + 1)
		return;
	float dv, l;
	float max_dist = 0;
	Point sij, siv;
	unsigned int max_pos = p1;

	sij.px = m_Line[p2].px - m_Line[p1].px;
	sij.py = m_Line[p2].py - m_Line[p1].py;

	l = dist(&sij);

	//	cout << l << endl;

	for (unsigned int i = p1 + 1; i < p2; i++)
	{
		siv.px = m_Line[i].px - m_Line[p1].px;
		siv.py = m_Line[i].py - m_Line[p1].py;
		dv = cross(&siv, &sij) / l;
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
		DougPeuckSub(epsilon, p1, max_pos, v1_p);
		DougPeuckSub(epsilon, max_pos, p2, v1_p);
	}
}

void OutlineLine::ShiftContour(int dx, int dy)
{
	unsigned l = (unsigned)m_Line.size();
	for (unsigned i = 0; i < l; i++)
	{
		m_Line[i].px += dx;
		m_Line[i].py += dy;
	}
}

//-------------------------------------------------------------------------------
OutlineSlice::OutlineSlice() { m_Thickness = 1; }

OutlineSlice::OutlineSlice(float thick) { m_Thickness = thick; }

void OutlineSlice::AddLine(tissues_size_t tissuetype, std::vector<point_type>* P_vec, bool outer)
{
	OutlineLine ol;
	//	ol.add_points(P_vec);
	if (outer)
	{
		m_OuterLines[tissuetype].push_back(ol);
		(*(--m_OuterLines[tissuetype].end())).AddPoints(P_vec);
	}
	else
	{
		m_InnerLines[tissuetype].push_back(ol);
		(*(--m_InnerLines[tissuetype].end())).AddPoints(P_vec);
	}
}

void OutlineSlice::AddPoints(tissues_size_t tissuetype, unsigned short linenr, std::vector<point_type>* P_vec, bool outer)
{
	if (outer)
	{
		(m_OuterLines[tissuetype])[linenr].AddPoints(P_vec);
	}
	else
	{
		(m_InnerLines[tissuetype])[linenr].AddPoints(P_vec);
	}
}

void OutlineSlice::AddPoint(tissues_size_t tissuetype, unsigned short linenr, point_type P, bool outer)
{
	if (outer)
	{
		(m_OuterLines[tissuetype])[linenr].AddPoint(P);
	}
	else
	{
		(m_InnerLines[tissuetype])[linenr].AddPoint(P);
	}
}

void OutlineSlice::Clear()
{
	m_OuterLines.clear();
	m_InnerLines.clear();
}

void OutlineSlice::Clear(tissues_size_t tissuetype)
{
	m_OuterLines.erase(tissuetype);
	m_InnerLines.erase(tissuetype);
}

void OutlineSlice::Clear(tissues_size_t tissuetype, unsigned short linenr, bool outer)
{
	if (outer)
	{
		if (m_OuterLines.find(tissuetype) != m_OuterLines.end())
		{
			m_OuterLines[tissuetype].erase(m_OuterLines[tissuetype].begin() +
																		 linenr);
		}
	}
	else if (m_InnerLines.find(tissuetype) != m_InnerLines.end())
	{
		m_InnerLines[tissuetype].erase(m_InnerLines[tissuetype].begin() + linenr);
	}
}

unsigned short OutlineSlice::ReturnNrlines(tissues_size_t tissuetype, bool outer)
{
	if (outer)
	{
		if (m_OuterLines.find(tissuetype) != m_OuterLines.end())
		{
			return (unsigned short)(m_OuterLines[tissuetype].size());
		}
	}
	else if (m_InnerLines.find(tissuetype) != m_InnerLines.end())
	{
		return (unsigned short)(m_InnerLines[tissuetype].size());
	}
	return 0;
}

unsigned OutlineSlice::ReturnLength(tissues_size_t tissuetype, unsigned short linenr, bool outer)
{
	if (outer)
	{
		if (m_OuterLines.find(tissuetype) != m_OuterLines.end())
		{
			return (unsigned)((m_OuterLines[tissuetype])[linenr].ReturnLength());
		}
	}
	else if (m_InnerLines.find(tissuetype) != m_InnerLines.end())
	{
		return (unsigned)((m_InnerLines[tissuetype])[linenr].ReturnLength());
	}
	return 0;
}

point_type* OutlineSlice::ReturnLine(tissues_size_t tissuetype, unsigned short linenr, bool outer)
{
	if (outer)
	{
		if (m_OuterLines.find(tissuetype) != m_OuterLines.end())
		{
			return m_OuterLines[tissuetype][linenr].ReturnLine();
		}
	}
	else if (m_InnerLines.find(tissuetype) != m_InnerLines.end())
	{
		return m_InnerLines[tissuetype][linenr].ReturnLine();
	}
	return nullptr;
}

FILE* OutlineSlice::Print(FILE* fp, tissues_size_t nr_tissues)
{
	fprintf(fp, "Thick: %f\n", m_Thickness);

	if (nr_tissues > 255)
	{ // Only print tissue indices which contain outlines

		// Collect used tissue indices in ascending order
		std::set<tissues_size_t> tissue_indices;
		InsertTissueIndices(tissue_indices);

		tissues_size_t curr_tissue_idx;
		std::vector<OutlineLine>::iterator it_vec;
		for (auto it_tissue_idx = tissue_indices.begin(); it_tissue_idx != tissue_indices.end(); ++it_tissue_idx)
		{
			curr_tissue_idx = *it_tissue_idx;

			// Print outer lines
			if (m_OuterLines.find(curr_tissue_idx) != m_OuterLines.end())
			{
				fprintf(fp, "T%u O%u\n", curr_tissue_idx, (unsigned)m_OuterLines[curr_tissue_idx].size());
				for (it_vec = m_OuterLines[curr_tissue_idx].begin();
						 it_vec != m_OuterLines[curr_tissue_idx].end(); ++it_vec)
					(*it_vec).Print(fp);
			}
			else
			{
				fprintf(fp, "T%u O%u\n", curr_tissue_idx, 0);
			}

			// Print inner lines
			if (m_InnerLines.find(curr_tissue_idx) != m_InnerLines.end())
			{
				fprintf(fp, "T%u I%u\n", curr_tissue_idx, (unsigned)m_InnerLines[curr_tissue_idx].size());
				for (it_vec = m_InnerLines[curr_tissue_idx].begin();
						 it_vec != m_InnerLines[curr_tissue_idx].end(); ++it_vec)
					(*it_vec).Print(fp);
			}
			else
			{
				fprintf(fp, "T%u I%u\n", curr_tissue_idx, 0);
			}
		}
	}
	else
	{ // Print all tissue indices within range of unsigned char

		std::vector<OutlineLine>::iterator it_vec;
		for (unsigned short i = 0; i < 256; ++i)
		{
			// Print outer lines
			if (m_OuterLines.find(i) != m_OuterLines.end())
			{
				fprintf(fp, "T%u O%u\n", i, (unsigned)m_OuterLines[i].size());
				for (it_vec = m_OuterLines[i].begin();
						 it_vec != m_OuterLines[i].end(); ++it_vec)
					(*it_vec).Print(fp);
			}
			else
			{
				fprintf(fp, "T%u O%u\n", i, 0);
			}

			// Print inner lines
			if (m_InnerLines.find(i) != m_InnerLines.end())
			{
				fprintf(fp, "T%u I%u\n", i, (unsigned)m_InnerLines[i].size());
				for (it_vec = m_InnerLines[i].begin();
						 it_vec != m_InnerLines[i].end(); ++it_vec)
					(*it_vec).Print(fp);
			}
			else
			{
				fprintf(fp, "T%u I%u\n", i, 0);
			}
		}
	}

	return fp;
}

FILE* OutlineSlice::Read(FILE* fp)
{
	unsigned j, l;

	fscanf(fp, "Thick: %f\n", &m_Thickness);

	//	for(unsigned i=0;i<nr_tissuetypes;i++){
	//		fscanf(fp,"T%u O%u\n",&j,&l);
	FILE* fphold = fp;
	while (fscanf(fp, "T%u O%u\n", &j, &l) == 2)
	{
		for (unsigned k = 0; k < l; k++)
		{
			OutlineLine ol;
			//			ol.read(fp);
			m_OuterLines[j].push_back(ol);
			(*(--m_OuterLines[j].end())).Read(fp);
		}
		fscanf(fp, "T%u I%u\n", &j, &l);
		for (unsigned k = 0; k < l; k++)
		{
			OutlineLine ol;
			//			ol.read(fp);
			m_InnerLines[j].push_back(ol);
			(*(--m_InnerLines[j].end())).Read(fp);
		}
		fphold = fp;
	}

	fp = fphold;
	return fp;
}

void OutlineSlice::SetThickness(float thick) { m_Thickness = thick; }

float OutlineSlice::GetThickness() const { return m_Thickness; }

void OutlineSlice::DougPeuck(float epsilon, bool closed)
{
	TissueOutlineMap_type::iterator it_tissue;
	std::vector<OutlineLine>::iterator it_outline;
	for (it_tissue = m_OuterLines.begin(); it_tissue != m_OuterLines.end();
			 ++it_tissue)
	{
		for (it_outline = it_tissue->second.begin();
				 it_outline != it_tissue->second.end(); ++it_outline)
		{
			it_outline->DougPeuck(epsilon, closed);
		}
	}
	for (it_tissue = m_InnerLines.begin(); it_tissue != m_InnerLines.end();
			 ++it_tissue)
	{
		for (it_outline = it_tissue->second.begin();
				 it_outline != it_tissue->second.end(); ++it_outline)
		{
			it_outline->DougPeuck(epsilon, closed);
		}
	}
}

void OutlineSlice::ShiftContours(int dx, int dy)
{
	TissueOutlineMap_type::iterator it_tissue;
	std::vector<OutlineLine>::iterator it_outline;
	for (it_tissue = m_OuterLines.begin(); it_tissue != m_OuterLines.end();
			 ++it_tissue)
	{
		for (it_outline = it_tissue->second.begin();
				 it_outline != it_tissue->second.end(); ++it_outline)
		{
			it_outline->ShiftContour(dx, dy);
		}
	}
	for (it_tissue = m_InnerLines.begin(); it_tissue != m_InnerLines.end();
			 ++it_tissue)
	{
		for (it_outline = it_tissue->second.begin();
				 it_outline != it_tissue->second.end(); ++it_outline)
		{
			it_outline->ShiftContour(dx, dy);
		}
	}
}

void OutlineSlice::InsertTissueIndices(std::set<tissues_size_t>& tissueIndices)
{
	// Inserts tissue indices contained in the outline into the set (ascending order)
	TissueOutlineMap_type::iterator it_tissue;
	for (it_tissue = m_OuterLines.begin(); it_tissue != m_OuterLines.end();
			 ++it_tissue)
	{
		tissueIndices.insert(it_tissue->first);
	}
	for (it_tissue = m_InnerLines.begin(); it_tissue != m_InnerLines.end();
			 ++it_tissue)
	{
		tissueIndices.insert(it_tissue->first);
	}
}

//-------------------------------------------------------------------------------
OutlineSlices::OutlineSlices(unsigned nrslices)
{
	m_Dx = m_Dy = 1;
	m_Slices.resize(nrslices);
}

OutlineSlices::OutlineSlices()
{
	m_Dx = m_Dy = 1;
}

OutlineSlices::OutlineSlices(unsigned nrslices, float thick)
{
	m_Slices.resize(nrslices);
	for (unsigned i = 0; i < nrslices; i++)
		m_Slices[i].SetThickness(thick);
}

void OutlineSlices::SetSizenr(unsigned nrslices)
{
	m_Slices.resize(nrslices);
}

void OutlineSlices::Clear()
{
	for (unsigned i = 0; i < (unsigned)m_Slices.size(); i++)
		m_Slices[i].Clear();
}

void OutlineSlices::Clear(tissues_size_t tissuetype)
{
	for (unsigned i = 0; i < (unsigned)m_Slices.size(); i++)
		m_Slices[i].Clear(tissuetype);
}

void OutlineSlices::ClearSlice(unsigned slicenr)
{
	m_Slices[slicenr].Clear();
}

void OutlineSlices::ClearSlice(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, bool outer)
{
	m_Slices[slicenr].Clear(tissuetype, linenr, outer);
}

void OutlineSlices::AddLine(unsigned slicenr, tissues_size_t tissuetype, std::vector<point_type>* P_vec, bool outer)
{
	m_Slices[slicenr].AddLine(tissuetype, P_vec, outer);
}

void OutlineSlices::AddPoints(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, std::vector<point_type>* P_vec, bool outer)
{
	m_Slices[slicenr].AddPoints(tissuetype, linenr, P_vec, outer);
}

void OutlineSlices::AddPoint(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, point_type P, bool outer)
{
	m_Slices[slicenr].AddPoint(tissuetype, linenr, P, outer);
}

unsigned OutlineSlices::ReturnNrslices() { return (unsigned)m_Slices.size(); }

unsigned short OutlineSlices::ReturnNrlines(unsigned slicenr, tissues_size_t tissuetype, bool outer)
{
	return m_Slices[slicenr].ReturnNrlines(tissuetype, outer);
}

unsigned OutlineSlices::ReturnLength(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, bool outer)
{
	return m_Slices[slicenr].ReturnLength(tissuetype, linenr, outer);
}

point_type* OutlineSlices::ReturnLine(unsigned slicenr, tissues_size_t tissuetype, unsigned short linenr, bool outer)
{
	return m_Slices[slicenr].ReturnLine(tissuetype, linenr, outer);
}

int OutlineSlices::Print(const char* filename, tissues_size_t nr_tissues)
{
	unsigned nr_slices = (unsigned)m_Slices.size();

	FILE* fp = Printprologue(filename, nr_slices, nr_tissues);

	if (fp == nullptr)
		return 0;

	for (unsigned i = 0; i < nr_slices; i++)
	{
		fprintf(fp, "S%u\n", i);
		m_Slices[i].Print(fp, nr_tissues);
	}

	fclose(fp);
	return 1;
}

FILE* OutlineSlices::Printprologue(const char* filename, unsigned nr_slices, tissues_size_t nr_tissues) const
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
		fprintf(fp, "PS%f %f\n", m_Dx, m_Dy);
	}

	return fp;
}

FILE* OutlineSlices::Printsection(FILE* fp, unsigned startslice, unsigned endslice, unsigned offset, tissues_size_t nr_tissues)
{
	for (unsigned i = startslice; i <= endslice; i++)
	{
		fprintf(fp, "S%u\n", i + offset);
		m_Slices[i].Print(fp, nr_tissues);
	}

	return fp;
}

int OutlineSlices::Read(const char* filename)
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

	fscanf(fp, "PS%f %f\n", &m_Dx, &m_Dy);
	m_Slices.clear();
	m_Slices.resize(j);

	for (unsigned i = 0; i < j; i++)
	{
		fscanf(fp, "S%u\n", &k);
		m_Slices[i].Read(fp);
	}

	fclose(fp);
	return 1;
}

void OutlineSlices::SetThickness(float thick, unsigned slicenr)
{
	m_Slices[slicenr].SetThickness(thick);
}

void OutlineSlices::SetThickness(float thick)
{
	for (unsigned i = 0; i < (unsigned)m_Slices.size(); i++)
		m_Slices[i].SetThickness(thick);
}

void OutlineSlices::SetPixelsize(float dx1, float dy1)
{
	m_Dx = dx1;
	m_Dy = dy1;
}

float OutlineSlices::GetThickness(unsigned slicenr)
{
	return m_Slices[slicenr].GetThickness();
}

PointType2 OutlineSlices::GetPixelsize() const
{
	PointType2 p;
	p.px = m_Dx;
	p.py = m_Dy;
	return p;
}

void OutlineSlices::DougPeuck(float epsilon, bool closed)
{
	unsigned n = (unsigned)m_Slices.size();
	for (unsigned i = 0; i < n; i++)
	{
		m_Slices[i].DougPeuck(epsilon, closed);
	}
}

void OutlineSlices::ShiftContours(int dx, int dy)
{
	unsigned n = (unsigned)m_Slices.size();
	for (unsigned i = 0; i < n; i++)
	{
		m_Slices[i].ShiftContours(dx, dy);
	}
}

void OutlineSlices::InsertTissueIndices(std::set<tissues_size_t>& tissueIndices)
{
	unsigned n = (unsigned)m_Slices.size();
	for (unsigned i = 0; i < n; i++)
	{
		m_Slices[i].InsertTissueIndices(tissueIndices);
	}
}

} // namespace iseg
