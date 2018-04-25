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

#include "Data/Types.h"

#include <map>
#include <set>
#include <string>
#include <vector>

namespace iseg {

class SlicesHandler;			 // BL TODO get rid of this?
class TissueHierarchyItem; // BL TODO get rid of this?

struct TissueInfoStruct
{
	TissueInfoStruct()
	{
		SetColor(0.0f, 0.0f, 0.0f, 0.5f);
		name = "?";
		locked = false;
	};

	TissueInfoStruct(const TissueInfoStruct& other)
	{
		SetColor(other.color[0], other.color[1], other.color[2], other.opac);
		name = other.name;
		locked = other.locked;
	};

	TissueInfoStruct& operator=(const TissueInfoStruct& other)
	{
		SetColor(other.color[0], other.color[1], other.color[2], other.opac);
		name = other.name;
		locked = other.locked;
		return *this;
	};

	void SetColor(float r, float g, float b)
	{
		color[0] = r;
		color[1] = g;
		color[2] = b;
	};

	void SetColor(float r, float g, float b, float a)
	{
		color[0] = r;
		color[1] = g;
		color[2] = b;
		opac = a;
	};

	void GetColorRGB(unsigned char& r, unsigned char& g, unsigned char& b)
	{
		r = (unsigned char)(255.0f * color[0]);
		g = (unsigned char)(255.0f * color[1]);
		b = (unsigned char)(255.0f * color[2]);
	};

	void GetColorBlendedRGB(unsigned char& r, unsigned char& g,
			unsigned char& b, unsigned char offset = 0)
	{
		r = (unsigned char)(offset + opac * (255.0f * color[0] - offset));
		g = (unsigned char)(offset + opac * (255.0f * color[1] - offset));
		b = (unsigned char)(offset + opac * (255.0f * color[2] - offset));
	};

	float color[3];
	float opac;
	std::string name;
	bool locked;
};

typedef std::vector<TissueInfoStruct> TissueInfosVecType;
typedef std::map<std::string, tissues_size_t> TissueTypeMapType;
typedef std::pair<std::string, tissues_size_t> TissueTypeMapEntryType;

class TissueInfos
{
public:
	static tissues_size_t GetTissueCount();
	static TissueInfoStruct* GetTissueInfo(tissues_size_t tissuetype);

	static float* GetTissueColor(tissues_size_t tissuetype);
	static void GetTissueColorRGB(tissues_size_t tissuetype, unsigned char& r,
			unsigned char& g, unsigned char& b);
	static void GetTissueColorBlendedRGB(tissues_size_t tissuetype,
			unsigned char& r, unsigned char& g,
			unsigned char& b,
			unsigned char offset = 0);
	static float GetTissueOpac(tissues_size_t tissuetype);
	static std::string GetTissueName(tissues_size_t tissuetype);
	static tissues_size_t GetTissueType(std::string tissuename);
	static bool GetTissueLocked(tissues_size_t tissuetype);

	static void SetTissueColor(tissues_size_t tissuetype, float r, float g,
			float b);
	static void SetTissueOpac(tissues_size_t tissuetype, float val);
	static void SetTissueName(tissues_size_t tissuetype, std::string val);
	static void SetTissueLocked(tissues_size_t tissuetype, bool val);
	static void SetTissuesLocked(bool val);

	static void InitTissues();

	static void AddTissue(TissueInfoStruct& tissue);
	static void RemoveTissue(tissues_size_t tissuetype);
	static void RemoveTissues(const std::set<tissues_size_t>& tissuetypes);
	static void RemoveAllTissues();

	static FILE* SaveTissues(FILE* fp, unsigned short version);
	static FILE* LoadTissues(FILE* fp, int tissuesVersion);
	static bool LoadTissuesHDF(const char* filename, int tissuesVersion);
	static FILE* SaveTissueLocks(FILE* fp);
	static FILE* LoadTissueLocks(FILE* fp);

	static bool SaveTissuesHDF(const char* filename,
			TissueHierarchyItem* hiearchy, bool naked,
			unsigned short version);

	static bool SaveTissuesReadable(const char* filename,
			unsigned short version);
	static bool LoadTissuesReadable(const char* filename,
			SlicesHandler* handler3D,
			tissues_size_t& removeTissuesRange);

	static bool SaveDefaultTissueList(const char* filename);
	static bool LoadDefaultTissueList(const char* filename);

	static bool ExportSurfaceGenerationToolXML(const char* filename);

protected:
	static void init_tissues1();
	static void CreateTissueTypeMap();

protected:
	static TissueInfosVecType tissueInfosVector;
	static TissueTypeMapType tissueTypeMap;
};

} // namespace iseg
