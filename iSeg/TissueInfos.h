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

#include "Data/Color.h"
#include "Data/Types.h"

#include <array>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace iseg {

class SlicesHandler;			 // BL TODO get rid of this?
class TissueHierarchyItem; // BL TODO get rid of this?

class TissueInfo
{
public:
	std::string name = "?";
	Color color = {0.f, 0.f, 0.f};
	float opac = 0.5f;
	bool locked = false;
};

class TissueInfos
{
public:
	using TissueInfosVecType = std::vector<TissueInfo>;
	using TissueTypeMapType = std::map<std::string, tissues_size_t>;
	using TissueTypeMapEntryType = std::pair<std::string, tissues_size_t>;

	static tissues_size_t GetTissueCount();
	static TissueInfo* GetTissueInfo(tissues_size_t tissuetype);

	static const Color& GetTissueColor(tissues_size_t tissuetype);
	static std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> GetTissueColorMapped(tissues_size_t tissuetype);
	static void GetTissueColorBlendedRGB(tissues_size_t tissuetype, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char offset = 0);
	static float GetTissueOpac(tissues_size_t tissuetype);
	static std::string GetTissueName(tissues_size_t tissuetype);
	static tissues_size_t GetTissueType(std::string tissuename);
	static bool GetTissueLocked(tissues_size_t tissuetype);

	static void SetTissueColor(tissues_size_t tissuetype, float r, float g, float b);
	static void SetTissueOpac(tissues_size_t tissuetype, float val);
	static void SetTissueName(tissues_size_t tissuetype, std::string val);
	static void SetTissueLocked(tissues_size_t tissuetype, bool val);
	static void SetTissuesLocked(bool val);

	static void InitTissues();

	static void AddTissue(TissueInfo& tissue);
	static void RemoveTissue(tissues_size_t tissuetype);
	static void RemoveTissues(const std::set<tissues_size_t>& tissuetypes);
	static void RemoveAllTissues();

	static std::set<tissues_size_t> GetSelectedTissues();
	// \warning The GUI does not observe changes to this selection, they are set from the GUI
	static void SetSelectedTissues(const std::set<tissues_size_t>& sel);

	static FILE* SaveTissues(FILE* fp, unsigned short version);
	static FILE* LoadTissues(FILE* fp, int tissuesVersion);
	static bool LoadTissuesHDF(const char* filename, int tissuesVersion);
	static FILE* SaveTissueLocks(FILE* fp);
	static FILE* LoadTissueLocks(FILE* fp);

	static bool SaveTissuesHDF(const char* filename, TissueHierarchyItem* hiearchy, bool naked, unsigned short version);
	static bool SaveTissuesReadable(const char* filename, unsigned short version);
	static bool LoadTissuesReadable(const char* filename, SlicesHandler* handler3D, tissues_size_t& removeTissuesRange);

	static bool SaveDefaultTissueList(const char* filename);
	static bool LoadDefaultTissueList(const char* filename);

	static bool ExportSurfaceGenerationToolXML(const char* filename);

protected:
	static void CreateTissueTypeMap();

protected:
	static TissueInfosVecType tissueInfosVector;
	static TissueTypeMapType tissueTypeMap;
};

} // namespace iseg
