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

#include "SlicesHandler.h"
#include "TissueHierarchy.h"
#include "TissueInfos.h"

#include "Data/Logger.h"
#include "Data/ScopedTimer.h"

#include "Core/HDF5Reader.h"
#include "Core/HDF5Writer.h"

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <vtkSmartPointer.h>

#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>

namespace iseg {

namespace {
std::string str_tolower(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); } // correct
	);
	return s;
}
} // namespace

TissueInfo* TissueInfos::GetTissueInfo(tissues_size_t tissuetype)
{
	return &tissue_infos_vector[tissuetype];
}

const Color& TissueInfos::GetTissueColor(tissues_size_t tissuetype)
{
	return tissue_infos_vector.at(tissuetype).m_Color;
}

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t> iseg::TissueInfos::GetTissueColorMapped(tissues_size_t tissuetype)
{
	if (int(tissuetype) < tissue_infos_vector.size())
	{
		return tissue_infos_vector[tissuetype].m_Color.ToUChar();
	}
	else
	{
		using rgb = std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>;
		if (tissuetype == Mark::red)
		{
			return rgb(255, 0, 0);
		}
		else if (tissuetype == Mark::green)
		{
			return rgb(0, 255, 0);
		}
		else if (tissuetype == Mark::blue)
		{
			return rgb(0, 0, 255);
		}
		else //if (tissuetype == Mark::WHITE)
		{
			return rgb(255, 255, 255);
		}
	}
}

void TissueInfos::GetTissueColorBlendedRGB(tissues_size_t tissuetype, unsigned char& r, unsigned char& g, unsigned char& b, unsigned char offset)
{
	auto& color = tissue_infos_vector.at(tissuetype).m_Color;
	auto opac = tissue_infos_vector.at(tissuetype).m_Opac;
	r = (unsigned char)(offset + opac * (255.0f * color[0] - offset));
	g = (unsigned char)(offset + opac * (255.0f * color[1] - offset));
	b = (unsigned char)(offset + opac * (255.0f * color[2] - offset));
}

float TissueInfos::GetTissueOpac(tissues_size_t tissuetype)
{
	return tissue_infos_vector[tissuetype].m_Opac;
}

std::string TissueInfos::GetTissueName(tissues_size_t tissuetype)
{
	return tissue_infos_vector[tissuetype].m_Name;
}

bool TissueInfos::GetTissueLocked(tissues_size_t tissuetype)
{
	if (tissuetype > tissue_infos_vector.size() - 1)
		return false;

	return tissue_infos_vector[tissuetype].m_Locked;
}

tissues_size_t TissueInfos::GetTissueType(std::string tissuename)
{
	std::string lower = str_tolower(tissuename);
	if (tissue_type_map.find(lower) != tissue_type_map.end())
	{
		return tissue_type_map[lower];
	}
	return 0;
}

void TissueInfos::SetTissueColor(tissues_size_t tissuetype, float r, float g, float b)
{
	tissue_infos_vector[tissuetype].m_Color = Color(r, g, b);
}

void TissueInfos::SetTissueOpac(tissues_size_t tissuetype, float val)
{
	tissue_infos_vector[tissuetype].m_Opac = val;
}

void TissueInfos::SetTissueName(tissues_size_t tissuetype, std::string val)
{
	tissue_type_map.erase(str_tolower(tissue_infos_vector[tissuetype].m_Name));
	tissue_type_map.insert(TissueTypeMapEntryType(str_tolower(val), tissuetype));
	tissue_infos_vector[tissuetype].m_Name = val;
}

void TissueInfos::SetTissueLocked(tissues_size_t tissuetype, bool val)
{
	tissue_infos_vector[tissuetype].m_Locked = val;
}

void TissueInfos::SetTissuesLocked(bool val)
{
	for (auto vec_it = tissue_infos_vector.begin() + 1; vec_it != tissue_infos_vector.end(); ++vec_it)
	{
		vec_it->m_Locked = val;
	}
}

void TissueInfos::InitTissues()
{
	tissue_infos_vector.clear();
	tissue_infos_vector.resize(83);

	tissue_infos_vector[1].m_Name = "Adrenal_gland";
	tissue_infos_vector[1].m_Color = Color(0.338000f, 0.961000f, 0.725000f);
	tissue_infos_vector[2].m_Name = "Air_internal";
	tissue_infos_vector[2].m_Color = Color(0.000000f, 0.000000f, 0.000000f);
	tissue_infos_vector[3].m_Name = "Artery";
	tissue_infos_vector[3].m_Color = Color(0.800000f, 0.000000f, 0.000000f);
	tissue_infos_vector[4].m_Name = "Bladder";
	tissue_infos_vector[4].m_Color = Color(0.529400f, 0.854900f, 0.011800f);
	tissue_infos_vector[5].m_Name = "Blood_vessel";
	tissue_infos_vector[5].m_Color = Color(0.666700f, 0.003900f, 0.003900f);
	tissue_infos_vector[6].m_Name = "Bone";
	tissue_infos_vector[6].m_Color = Color(0.929412f, 0.839216f, 0.584314f);
	tissue_infos_vector[7].m_Name = "Brain_grey_matter";
	tissue_infos_vector[7].m_Color = Color(0.500000f, 0.500000f, 0.500000f);
	tissue_infos_vector[8].m_Name = "Brain_white_matter";
	tissue_infos_vector[8].m_Color = Color(0.900000f, 0.900000f, 0.900000f);
	tissue_infos_vector[9].m_Name = "Breast";
	tissue_infos_vector[9].m_Color = Color(0.996000f, 0.741000f, 1.000000f);
	tissue_infos_vector[10].m_Name = "Bronchi";
	tissue_infos_vector[10].m_Color = Color(0.528000f, 0.592000f, 1.000000f);
	tissue_infos_vector[11].m_Name = "Bronchi_lumen";
	tissue_infos_vector[11].m_Color = Color(0.368600f, 0.474500f, 0.635300f);
	tissue_infos_vector[12].m_Name = "Cartilage";
	tissue_infos_vector[12].m_Color = Color(0.627000f, 0.988000f, 0.969000f);
	tissue_infos_vector[13].m_Name = "Cerebellum";
	tissue_infos_vector[13].m_Color = Color(0.648000f, 0.599000f, 0.838000f);
	tissue_infos_vector[14].m_Name = "Cerebrospinal_fluid";
	tissue_infos_vector[14].m_Color = Color(0.474500f, 0.521600f, 0.854900f);
	tissue_infos_vector[15].m_Name = "Connective_tissue";
	tissue_infos_vector[15].m_Color = Color(1.000000f, 0.705882f, 0.000000f);
	tissue_infos_vector[16].m_Name = "Diaphragm";
	tissue_infos_vector[16].m_Color = Color(0.745000f, 0.188000f, 0.286000f);
	tissue_infos_vector[17].m_Name = "Ear_cartilage";
	tissue_infos_vector[17].m_Color = Color(0.627000f, 0.988000f, 0.969000f);
	tissue_infos_vector[18].m_Name = "Ear_skin";
	tissue_infos_vector[18].m_Color = Color(0.423500f, 0.611800f, 0.603900f);
	tissue_infos_vector[19].m_Name = "Epididymis";
	tissue_infos_vector[19].m_Color = Color(0.000000f, 0.359000f, 1.000000f);
	tissue_infos_vector[20].m_Name = "Esophagus";
	tissue_infos_vector[20].m_Color = Color(1.000000f, 0.585000f, 0.000000f);
	tissue_infos_vector[21].m_Name = "Esophagus_lumen";
	tissue_infos_vector[21].m_Color = Color(1.000000f, 0.789000f, 0.635000f);
	tissue_infos_vector[22].m_Name = "Eye_lens";
	tissue_infos_vector[22].m_Color = Color(0.007800f, 0.658800f, 0.996100f);
	tissue_infos_vector[23].m_Name = "Eye_vitreous_humor";
	tissue_infos_vector[23].m_Color = Color(0.331000f, 0.746000f, 0.937000f);
	tissue_infos_vector[24].m_Name = "Fat";
	tissue_infos_vector[24].m_Color = Color(0.984314f, 0.980392f, 0.215686f);
	tissue_infos_vector[25].m_Name = "Gallbladder";
	tissue_infos_vector[25].m_Color = Color(0.258800f, 0.972500f, 0.274500f);
	tissue_infos_vector[26].m_Name = "Heart_lumen";
	tissue_infos_vector[26].m_Color = Color(1.000000f, 0.000000f, 0.000000f);
	tissue_infos_vector[27].m_Name = "Heart_muscle";
	tissue_infos_vector[27].m_Color = Color(1.000000f, 0.000000f, 0.239000f);
	tissue_infos_vector[28].m_Name = "Hippocampus";
	tissue_infos_vector[28].m_Color = Color(0.915000f, 0.188000f, 1.000000f);
	tissue_infos_vector[29].m_Name = "Hypophysis";
	tissue_infos_vector[29].m_Color = Color(1.000000f, 0.000000f, 0.796000f);
	tissue_infos_vector[30].m_Name = "Hypothalamus";
	tissue_infos_vector[30].m_Color = Color(0.563000f, 0.239000f, 0.754000f);
	tissue_infos_vector[31].m_Name = "Intervertebral_disc";
	tissue_infos_vector[31].m_Color = Color(0.627500f, 0.988200f, 0.968600f);
	tissue_infos_vector[32].m_Name = "Kidney_cortex";
	tissue_infos_vector[32].m_Color = Color(0.000000f, 0.754000f, 0.200000f);
	tissue_infos_vector[33].m_Name = "Kidney_medulla";
	tissue_infos_vector[33].m_Color = Color(0.507000f, 1.000000f, 0.479000f);
	tissue_infos_vector[34].m_Name = "Large_intestine";
	tissue_infos_vector[34].m_Color = Color(1.000000f, 0.303000f, 0.176000f);
	tissue_infos_vector[35].m_Name = "Large_intestine_lumen";
	tissue_infos_vector[35].m_Color = Color(0.817000f, 0.556000f, 0.570000f);
	tissue_infos_vector[36].m_Name = "Larynx";
	tissue_infos_vector[36].m_Color = Color(0.937000f, 0.561000f, 0.950000f);
	tissue_infos_vector[37].m_Name = "Liver";
	tissue_infos_vector[37].m_Color = Color(0.478400f, 0.262700f, 0.141200f);
	tissue_infos_vector[38].m_Name = "Lung";
	tissue_infos_vector[38].m_Color = Color(0.225000f, 0.676000f, 1.000000f);
	tissue_infos_vector[39].m_Name = "Mandible";
	tissue_infos_vector[39].m_Color = Color(0.929412f, 0.839216f, 0.584314f);
	tissue_infos_vector[40].m_Name = "Marrow_red";
	tissue_infos_vector[40].m_Color = Color(0.937300f, 0.639200f, 0.498000f);
	tissue_infos_vector[41].m_Name = "Marrow_white";
	tissue_infos_vector[41].m_Color = Color(0.921600f, 0.788200f, 0.486300f);
	tissue_infos_vector[42].m_Name = "Meniscus";
	tissue_infos_vector[42].m_Color = Color(0.577000f, 0.338000f, 0.754000f);
	tissue_infos_vector[43].m_Name = "Midbrain";
	tissue_infos_vector[43].m_Color = Color(0.490200f, 0.682400f, 0.509800f);
	tissue_infos_vector[44].m_Name = "Muscle";
	tissue_infos_vector[44].m_Color = Color(0.745098f, 0.188235f, 0.286275f);
	tissue_infos_vector[45].m_Name = "Nail";
	tissue_infos_vector[45].m_Color = Color(0.873000f, 0.887000f, 0.880000f);
	tissue_infos_vector[46].m_Name = "Mucosa";
	tissue_infos_vector[46].m_Color = Color(1.000000f, 0.631373f, 0.745098f);
	tissue_infos_vector[47].m_Name = "Nerve";
	tissue_infos_vector[47].m_Color = Color(0.000000f, 0.754000f, 0.479000f);
	tissue_infos_vector[48].m_Name = "Ovary";
	tissue_infos_vector[48].m_Color = Color(0.718000f, 0.000000f, 1.000000f);
	tissue_infos_vector[49].m_Name = "Pancreas";
	tissue_infos_vector[49].m_Color = Color(0.506000f, 0.259000f, 0.808000f);
	tissue_infos_vector[50].m_Name = "Patella";
	tissue_infos_vector[50].m_Color = Color(0.929412f, 0.839216f, 0.584314f);
	tissue_infos_vector[51].m_Name = "Penis";
	tissue_infos_vector[51].m_Color = Color(0.000000f, 0.000000f, 1.000000f);
	tissue_infos_vector[52].m_Name = "Pharynx";
	tissue_infos_vector[52].m_Color = Color(0.368600f, 0.474500f, 0.635300f);
	tissue_infos_vector[53].m_Name = "Prostate";
	tissue_infos_vector[53].m_Color = Color(0.190000f, 0.190000f, 1.000000f);
	tissue_infos_vector[54].m_Name = "Scrotum";
	tissue_infos_vector[54].m_Color = Color(0.366000f, 0.549000f, 1.000000f);
	tissue_infos_vector[55].m_Name = "Skin";
	tissue_infos_vector[55].m_Color = Color(0.746000f, 0.613000f, 0.472000f);
	tissue_infos_vector[56].m_Name = "Skull";
	tissue_infos_vector[56].m_Color = Color(0.929412f, 0.839216f, 0.584314f);
	tissue_infos_vector[57].m_Name = "Small_intestine";
	tissue_infos_vector[57].m_Color = Color(1.000000f, 0.775000f, 0.690000f);
	tissue_infos_vector[58].m_Name = "Small_intestine_lumen";
	tissue_infos_vector[58].m_Color = Color(1.000000f, 0.474500f, 0.635300f);
	tissue_infos_vector[59].m_Name = "Spinal_cord";
	tissue_infos_vector[59].m_Color = Color(0.000000f, 0.732000f, 0.662000f);
	tissue_infos_vector[60].m_Name = "Spleen";
	tissue_infos_vector[60].m_Color = Color(0.682400f, 0.964700f, 0.788200f);
	tissue_infos_vector[61].m_Name = "Stomach";
	tissue_infos_vector[61].m_Color = Color(1.000000f, 0.500000f, 0.000000f);
	tissue_infos_vector[62].m_Name = "Stomach_lumen";
	tissue_infos_vector[62].m_Color = Color(1.000000f, 0.738000f, 0.503000f);
	tissue_infos_vector[63].m_Name = "SAT";
	tissue_infos_vector[63].m_Color = Color(1.000000f, 0.796079f, 0.341176f);
	tissue_infos_vector[64].m_Name = "Teeth";
	tissue_infos_vector[64].m_Color = Color(0.976471f, 0.960784f, 0.905882f);
	tissue_infos_vector[65].m_Name = "Tendon_Ligament";
	tissue_infos_vector[65].m_Color = Color(0.945098f, 0.960784f, 0.972549f);
	tissue_infos_vector[66].m_Name = "Testis";
	tissue_infos_vector[66].m_Color = Color(0.000000f, 0.606000f, 1.000000f);
	tissue_infos_vector[67].m_Name = "Thalamus";
	tissue_infos_vector[67].m_Color = Color(0.000000f, 0.415000f, 0.549000f);
	tissue_infos_vector[68].m_Name = "Thymus";
	tissue_infos_vector[68].m_Color = Color(0.439200f, 0.733300f, 0.549000f);
	tissue_infos_vector[69].m_Name = "Thyroid_gland";
	tissue_infos_vector[69].m_Color = Color(0.321600f, 0.023500f, 0.298000f);
	tissue_infos_vector[70].m_Name = "Tongue";
	tissue_infos_vector[70].m_Color = Color(0.800000f, 0.400000f, 0.400000f);
	tissue_infos_vector[71].m_Name = "Trachea";
	tissue_infos_vector[71].m_Color = Color(0.183000f, 1.000000f, 1.000000f);
	tissue_infos_vector[72].m_Name = "Trachea_lumen";
	tissue_infos_vector[72].m_Color = Color(0.613000f, 1.000000f, 1.000000f);
	tissue_infos_vector[73].m_Name = "Ureter_Urethra";
	tissue_infos_vector[73].m_Color = Color(0.376500f, 0.607800f, 0.007800f);
	tissue_infos_vector[74].m_Name = "Uterus";
	tissue_infos_vector[74].m_Color = Color(0.894000f, 0.529000f, 1.000000f);
	tissue_infos_vector[75].m_Name = "Vagina";
	tissue_infos_vector[75].m_Color = Color(0.608000f, 0.529000f, 1.000000f);
	tissue_infos_vector[76].m_Name = "Vein";
	tissue_infos_vector[76].m_Color = Color(0.000000f, 0.329000f, 1.000000f);
	tissue_infos_vector[77].m_Name = "Vertebrae";
	tissue_infos_vector[77].m_Color = Color(0.929412f, 0.839216f, 0.584314f);
	tissue_infos_vector[78].m_Name = "Pinealbody";
	tissue_infos_vector[78].m_Color = Color(1.000000f, 0.000000f, 0.000000f);
	tissue_infos_vector[79].m_Name = "Pons";
	tissue_infos_vector[79].m_Color = Color(0.000000f, 0.710000f, 0.700000f);
	tissue_infos_vector[80].m_Name = "Medulla_oblongata";
	tissue_infos_vector[80].m_Color = Color(0.370000f, 0.670000f, 0.920000f);
	tissue_infos_vector[81].m_Name = "Cornea";
	tissue_infos_vector[81].m_Color = Color(0.686275f, 0.000000f, 1.000000f);
	tissue_infos_vector[82].m_Name = "Eye_Sclera";
	tissue_infos_vector[82].m_Color = Color(1.000000f, 0.000000f, 0.780392f);

	CreateTissueTypeMap();
}

FILE* TissueInfos::SaveTissues(FILE* fp, unsigned short version)
{
	tissues_size_t tissuecount = GetTissueCount();
	fwrite(&tissuecount, 1, sizeof(tissues_size_t), fp);

	if (version >= 5)
	{
		float id = 1.2345f;
		fwrite(&id, 1, sizeof(float), fp);
		fwrite(&version, 1, sizeof(unsigned short), fp);
	}
	fwrite(&(tissue_infos_vector[0].m_Color[0]), 1, sizeof(float), fp);
	fwrite(&(tissue_infos_vector[0].m_Color[1]), 1, sizeof(float), fp);
	fwrite(&(tissue_infos_vector[0].m_Color[2]), 1, sizeof(float), fp);
	if (version >= 5)
	{
		fwrite(&(tissue_infos_vector[0].m_Opac), 1, sizeof(float), fp);
	}

	TissueInfosVecType::iterator vec_it;
	for (vec_it = tissue_infos_vector.begin() + 1;
			 vec_it != tissue_infos_vector.end(); ++vec_it)
	{
		fwrite(&(vec_it->m_Color[0]), 1, sizeof(float), fp);
		fwrite(&(vec_it->m_Color[1]), 1, sizeof(float), fp);
		fwrite(&(vec_it->m_Color[2]), 1, sizeof(float), fp);
		if (version >= 5)
		{
			fwrite(&(vec_it->m_Opac), 1, sizeof(float), fp);
		}
		int size = static_cast<int>(vec_it->m_Name.length());
		fwrite(&size, 1, sizeof(int), fp);
		fwrite(vec_it->m_Name.c_str(), 1, sizeof(char) * size, fp);
	}

	return fp;
}

namespace {
std::vector<TissueHierarchyItem*> GetLeaves(TissueHierarchyItem* hiearchy)
{
	std::vector<TissueHierarchyItem*> leaves;
	for (auto item : *hiearchy->GetChildren())
	{
		if (item->GetIsFolder())
		{
			auto item_leaves = GetLeaves(item);
			leaves.insert(leaves.end(), item_leaves.begin(), item_leaves.end());
		}
		else
		{
			leaves.insert(leaves.end(), item);
		}
	}
	return leaves;
}

std::map<std::string, std::string>
		BuildHiearchyMap(TissueHierarchyItem* hiearchy)
{
	std::map<std::string, std::string> result;

	if (hiearchy)
	{
		auto leaves = GetLeaves(hiearchy);
		for (auto item : leaves)
		{
			std::vector<std::string> parents;
			auto current = item->GetParent();
			while (current && current != hiearchy)
			{
				parents.push_back(current->GetName().toLocal8Bit().constData());
				current = current->GetParent();
			}
			std::string sep = "/";
			std::string path = boost::algorithm::join(parents, sep);
			std::string tissue = item->GetName().toLocal8Bit().constData();
			result.insert(std::make_pair(tissue, path));
		}
	}

	return result;
}
} // namespace

bool TissueInfos::SaveTissuesHDF(const char* filename, TissueHierarchyItem* hiearchy, bool naked, unsigned short version)
{
	ScopedTimer timer("Write Tissue List");

	namespace fs = boost::filesystem;

	int compression = 1;

	fs::path q_file_name(filename);
	std::string basename = (q_file_name.parent_path() / q_file_name.stem()).string();
	std::string suffix = q_file_name.extension().string();

	// save working directory
	auto oldcwd = fs::current_path();

	// enter the xmf file folder so relative names for hdf5 files work
	fs::current_path(q_file_name.parent_path());

	HDF5Writer writer;
	writer.m_Loud = false;
	std::string fname;
	if (naked)
		fname = basename + suffix;
	else
		fname = basename + ".h5";

	if (!writer.Open(fname, "append"))
	{
		ISEG_ERROR("opening " << fname);
	}
	writer.m_Compression = compression;

	auto hiearchy_map = BuildHiearchyMap(hiearchy);

	if (!writer.CreateGroup(std::string("Tissues")))
	{
		ISEG_ERROR_MSG("creating tissues section");
	}

	float rgbo[4];
	int index1[1];
	std::vector<HDF5Writer::size_type> dim1;
	dim1.resize(1);
	dim1[0] = 4;
	std::vector<HDF5Writer::size_type> dim2;
	dim2.resize(1);
	dim2[0] = 1;

	index1[0] = (int)version;
	if (!writer.Write(index1, dim2, std::string("/Tissues/version")))
	{
		ISEG_ERROR_MSG("writing version");
	}

	rgbo[0] = tissue_infos_vector[0].m_Color[0];
	rgbo[1] = tissue_infos_vector[0].m_Color[1];
	rgbo[2] = tissue_infos_vector[0].m_Color[2];
	rgbo[3] = tissue_infos_vector[0].m_Opac;
	if (!writer.Write(rgbo, dim1, std::string("/Tissues/bkg_rgbo")))
	{
		ISEG_ERROR_MSG("writing rgbo");
	}

	int counter = 1;
	for (auto vec_it = tissue_infos_vector.begin() + 1; vec_it != tissue_infos_vector.end(); ++vec_it)
	{
		std::string tissuename1 = vec_it->m_Name;
		boost::replace_all(tissuename1, "\\", "_");
		boost::replace_all(tissuename1, "/", "_");
		std::string tissuename = tissuename1; //BL TODO tissuename1.toLocal8Bit().constData();

		std::string groupname = std::string("/Tissues/") + tissuename;
		writer.CreateGroup(groupname);
		rgbo[0] = vec_it->m_Color[0];
		rgbo[1] = vec_it->m_Color[1];
		rgbo[2] = vec_it->m_Color[2];
		rgbo[3] = vec_it->m_Opac;

		std::string path = hiearchy_map[tissuename];
		if (!writer.WriteAttribute(path, groupname + "/path"))
		{
			ISEG_ERROR_MSG("writing path");
		}
		if (!writer.Write(rgbo, dim1, groupname + "/rgbo"))
		{
			ISEG_ERROR_MSG("writing rgbo");
		}
		index1[0] = counter++;

		if (!writer.Write(index1, dim2, groupname + "/index"))
		{
			ISEG_ERROR_MSG("writing index");
		}
	}

	writer.Close();

	return true;
}

FILE* TissueInfos::SaveTissueLocks(FILE* fp)
{
	TissueInfosVecType::iterator vec_it;
	for (vec_it = tissue_infos_vector.begin() + 1;
			 vec_it != tissue_infos_vector.end(); ++vec_it)
	{
		unsigned char dummy = (unsigned char)vec_it->m_Locked;
		fwrite(&(dummy), 1, sizeof(unsigned char), fp);
	}

	return fp;
}

FILE* TissueInfos::LoadTissueLocks(FILE* fp)
{
	unsigned char dummy;
	tissue_infos_vector[0].m_Locked = false;
	TissueInfosVecType::iterator vec_it;
	for (vec_it = tissue_infos_vector.begin() + 1;
			 vec_it != tissue_infos_vector.end(); ++vec_it)
	{
		fread(&(dummy), sizeof(unsigned char), 1, fp);
		vec_it->m_Locked = (bool)dummy;
	}

	return fp;
}

FILE* TissueInfos::LoadTissues(FILE* fp, int tissuesVersion)
{
	char name[100];
	tissues_size_t tissuecount;
	if (tissuesVersion > 0)
	{
		fread(&tissuecount, sizeof(tissues_size_t), 1, fp);
	}
	else
	{
		unsigned char uchar_buffer;
		fread(&uchar_buffer, sizeof(unsigned char), 1, fp);
		tissuecount = (tissues_size_t)uchar_buffer;
	}

	tissue_infos_vector.clear();
	tissue_infos_vector.resize(tissuecount + 1);

	float id;
	fread(&id, sizeof(float), 1, fp);
	unsigned short opac_version = 0;
	if (id == 1.2345f)
	{
		fread(&opac_version, sizeof(unsigned short), 1, fp);
		fread(&(tissue_infos_vector[0].m_Color[0]), sizeof(float), 1, fp);
	}
	else
	{
		tissue_infos_vector[0].m_Color[0] = id;
		tissue_infos_vector[0].m_Opac = 0.5f;
	}
	fread(&(tissue_infos_vector[0].m_Color[1]), sizeof(float), 1, fp);
	fread(&(tissue_infos_vector[0].m_Color[2]), sizeof(float), 1, fp);
	if (opac_version >= 5)
	{
		fread(&(tissue_infos_vector[0].m_Opac), sizeof(float), 1, fp);
	}

	TissueInfosVecType::iterator vec_it;
	for (vec_it = tissue_infos_vector.begin() + 1;
			 vec_it != tissue_infos_vector.end(); ++vec_it)
	{
		vec_it->m_Locked = false;
		fread(&(vec_it->m_Color[0]), sizeof(float), 1, fp);
		fread(&(vec_it->m_Color[1]), sizeof(float), 1, fp);
		fread(&(vec_it->m_Color[2]), sizeof(float), 1, fp);
		if (opac_version >= 5)
		{
			fread(&(vec_it->m_Opac), sizeof(float), 1, fp);
		}
		else
		{
			vec_it->m_Opac = 0.5f;
		}
		int size;
		fread(&size, sizeof(int), 1, fp);
		fread(name, sizeof(char) * size, 1, fp);
		if (size > 99)
			throw std::runtime_error("ERROR: in tissue file: setting.bin");
		name[size] = '\0';
		std::string s(name);
		vec_it->m_Name = s;
	}

	CreateTissueTypeMap();

	return fp;
}

bool TissueInfos::LoadTissuesHDF(const char* filename, int tissuesVersion)
{
	ScopedTimer timer("Read Tissue List");

	HDF5Reader reader;
	if (!reader.Open(filename))
	{
		ISEG_ERROR("opening " << filename);
		return false;
	}

	bool ok = true;
	std::vector<std::string> tissues = reader.GetGroupInfo("/Tissues");

	tissues_size_t tissuecount = static_cast<tissues_size_t>(tissues.size() - 2);
	tissue_infos_vector.clear();
	if (tissuecount == 0)
	{
		tissuecount = 1;
		tissue_infos_vector.resize(2);
		tissue_infos_vector[1].m_Locked = false;
		tissue_infos_vector[1].m_Color[0] = 1.0f;
		tissue_infos_vector[1].m_Color[1] = 0;
		tissue_infos_vector[1].m_Color[2] = 0;
		tissue_infos_vector[1].m_Opac = 0.5;
		tissue_infos_vector[1].m_Name = std::string("Tissue1");
	}
	else
	{
		tissue_infos_vector.resize(tissuecount + 1);
	}

	std::vector<float> rgbo;
	rgbo.resize(4);
	int index;
	for (std::vector<std::string>::iterator it = tissues.begin();
			 it != tissues.end(); it++)
	{
		if (std::string("version") == std::string(*it)) {}
		else if (std::string("bkg_rgbo") == std::string(*it))
		{
			ok = ok && (reader.Read(rgbo, "/Tissues/bkg_rgbo") != 0);
			tissue_infos_vector[0].m_Color[0] = rgbo[0];
			tissue_infos_vector[0].m_Color[1] = rgbo[1];
			tissue_infos_vector[0].m_Color[2] = rgbo[2];
			tissue_infos_vector[0].m_Opac = rgbo[3];
		}
		else
		{
			ok = ok && (reader.Read(&index, std::string("/Tissues/") + *it + std::string("/index")) != 0);
			ok = ok && (reader.Read(rgbo, std::string("/Tissues/") + *it + std::string("/rgbo")) != 0);
			tissue_infos_vector[index].m_Locked = false;
			tissue_infos_vector[index].m_Color[0] = rgbo[0];
			tissue_infos_vector[index].m_Color[1] = rgbo[1];
			tissue_infos_vector[index].m_Color[2] = rgbo[2];
			tissue_infos_vector[index].m_Opac = rgbo[3];
			tissue_infos_vector[index].m_Name = std::string(*it);
		}
	}

	CreateTissueTypeMap();

	reader.Close();

	return ok;
}

bool TissueInfos::SaveTissuesReadable(const char* filename, unsigned short version)
{
	FILE* fp;
	if ((fp = fopen(filename, "w")) == nullptr)
	{
		return false;
	}
	else
	{
		if (version >= 5)
		{
			fprintf(fp, "V%u\n", (unsigned)version);
		}
		tissues_size_t tissuecount = GetTissueCount();
		fprintf(fp, "N%u\n", tissuecount);

		TissueInfosVecType::iterator vec_it;
		if (version < 5)
		{
			for (vec_it = tissue_infos_vector.begin() + 1;
					 vec_it != tissue_infos_vector.end(); ++vec_it)
			{
				fprintf(fp, "C%f %f %f %s\n", vec_it->m_Color[0], vec_it->m_Color[1], vec_it->m_Color[2], vec_it->m_Name.c_str());
			}
		}
		else
		{
			for (vec_it = tissue_infos_vector.begin() + 1;
					 vec_it != tissue_infos_vector.end(); ++vec_it)
			{
				fprintf(fp, "C%f %f %f %f %s\n", vec_it->m_Color[0], vec_it->m_Color[1], vec_it->m_Color[2], vec_it->m_Opac, vec_it->m_Name.c_str());
			}
		}

		fclose(fp);
		return true;
	}
}

bool TissueInfos::LoadTissuesReadable(const char* filename, SlicesHandler* handler3D, tissues_size_t& removeTissuesRange)
{
	// To replace the old tissue list, handler3D->remove_tissues(removeTissuesRange) has to be called afterwards!

	char name[100];
	FILE* fp;
	if ((fp = fopen(filename, "r")) == nullptr)
	{
		InitTissues();
		return false;
	}
	else
	{
		unsigned short version = 0;
		unsigned int tc;

		// Get version number
		if (fscanf(fp, "V%u\n", &tc) != 1)
		{
			fclose(fp);
			if ((fp = fopen(filename, "r")) == nullptr)
			{
				InitTissues();
				return false;
			}
		}
		else
		{
			version = tc;
		}

		// Get number of labels
		bool is_free_surfer = false;
		int r, g, b, a, res, curr_label;
		if (fscanf(fp, "N%u\n", &tc) != 1)
		{
			fclose(fp);
			if ((fp = fopen(filename, "r")) == nullptr)
			{
				InitTissues();
				return false;
			}

			// Detect FreeSurfer color table format by searching for Unknown label
			curr_label = res = 0;
			r = g = b = a = -1;
			while (res != EOF && res != 6)
			{
				res = fscanf(fp, "%d\t%s\t%d\t%d\t%d\t%d\n", &curr_label, name, &r, &g, &b, &a);
				if (curr_label == r == g == b == a == 0 &&
						std::string("Unknown") == std::string(name))
				{
					is_free_surfer = true;
				}
			}
			if (!is_free_surfer)
			{
				// Unknown format
				fclose(fp);
				InitTissues();
				return false;
			}

			// Get largest label index
			tissues_size_t label_max = 0;
			while ((res = fscanf(fp, "%d\t%s\t%d\t%d\t%d\t%d\n", &curr_label, name, &r, &g, &b, &a)) != EOF)
			{
				if (res == 6)
				{
					label_max = std::max(int(label_max), curr_label);
				}
			}
			tc = label_max;

			fclose(fp);
			if ((fp = fopen(filename, "r")) == nullptr)
			{
				InitTissues();
				return false;
			}
		}

		std::set<tissues_size_t> missing_tissues;
		for (tissues_size_t type = 0; type <= TissueInfos::GetTissueCount();
				 ++type)
		{
			missing_tissues.insert(type);
		}

		tissues_size_t tc1 = (tissues_size_t)tc;
		if (tc > TISSUES_SIZE_MAX)
			tc1 = TISSUES_SIZE_MAX;

		TissueInfosVecType new_tissue_infos_vec;
		if (is_free_surfer)
		{
			// Read first tissue from file
			curr_label = res = 0;
			while (res != EOF && (res != 6 || curr_label == 0))
			{
				res = fscanf(fp, "%d\t%s\t%d\t%d\t%d\t%d\n", &curr_label, name, &r, &g, &b, &a);
			}

			tissues_size_t dummy_idx = 1;
			for (tissues_size_t new_type_idx = 0; new_type_idx < tc1; ++new_type_idx)
			{
				TissueInfo new_tissue_info;
				new_tissue_info.m_Locked = false;
				if (curr_label == new_type_idx + 1)
				{
					new_tissue_info.m_Color[0] = r / 255.0f;
					new_tissue_info.m_Color[1] = g / 255.0f;
					new_tissue_info.m_Color[2] = b / 255.0f;
					new_tissue_info.m_Opac = a / 255.0f;
					new_tissue_info.m_Name = std::string(name);

					// Read next tissue from file
					res = 0;
					while (res != EOF && res != 6)
					{
						res = fscanf(fp, "%d\t%s\t%d\t%d\t%d\t%d\n", &curr_label, name, &r, &g, &b, &a);
					}
				}
				else
				{
					// Dummy tissue
					new_tissue_info.m_Color[0] = float(rand()) / RAND_MAX;
					new_tissue_info.m_Color[1] = float(rand()) / RAND_MAX;
					new_tissue_info.m_Color[2] = float(rand()) / RAND_MAX;
					new_tissue_info.m_Opac = 0.5f;
					new_tissue_info.m_Name = (boost::format("DummyTissue%1") % dummy_idx++).str();
				}
				new_tissue_infos_vec.push_back(new_tissue_info);

				// Check whether the tissue existed before
				tissues_size_t old_type = GetTissueType(new_tissue_info.m_Name);
				if (old_type > 0 &&
						missing_tissues.find(old_type) != missing_tissues.end())
				{
					missing_tissues.erase(missing_tissues.find(old_type));
				}
			}
		}
		else if (version < 5)
		{
			for (tissues_size_t new_type_idx = 0; new_type_idx < tc1; ++new_type_idx)
			{
				TissueInfo new_tissue_info;
				new_tissue_info.m_Locked = false;
				if (fscanf(fp, "C%f %f %f %s\n", &(new_tissue_info.m_Color[0]), &(new_tissue_info.m_Color[1]), &(new_tissue_info.m_Color[2]), name) != 4)
				{
					fclose(fp);
					InitTissues();
					return false;
				}
				new_tissue_info.m_Opac = 0.5f;
				new_tissue_info.m_Name = std::string(name);
				new_tissue_infos_vec.push_back(new_tissue_info);

				// Check whether the tissue existed before
				tissues_size_t old_type = GetTissueType(new_tissue_info.m_Name);
				if (old_type > 0 &&
						missing_tissues.find(old_type) != missing_tissues.end())
				{
					missing_tissues.erase(missing_tissues.find(old_type));
				}
			}
		}
		else
		{
			for (tissues_size_t new_type_idx = 0; new_type_idx < tc1; ++new_type_idx)
			{
				TissueInfo new_tissue_info;
				new_tissue_info.m_Locked = false;
				if (fscanf(fp, "C%f %f %f %f %s\n", &(new_tissue_info.m_Color[0]), &(new_tissue_info.m_Color[1]), &(new_tissue_info.m_Color[2]), &(new_tissue_info.m_Opac), name) != 5)
				{
					fclose(fp);
					InitTissues();
					return false;
				}
				new_tissue_info.m_Name = std::string(name);
				new_tissue_infos_vec.push_back(new_tissue_info);

				// Check whether the tissue existed before
				tissues_size_t old_type = GetTissueType(new_tissue_info.m_Name);
				if (old_type > 0 &&
						missing_tissues.find(old_type) != missing_tissues.end())
				{
					missing_tissues.erase(missing_tissues.find(old_type));
				}
			}
		}

		// Prepend existing tissues (including background) which the input file did not contain
		removeTissuesRange = static_cast<tissues_size_t>(missing_tissues.size() - 1);
		for (std::set<tissues_size_t>::reverse_iterator riter = missing_tissues.rbegin(); riter != missing_tissues.rend(); ++riter)
		{
			new_tissue_infos_vec.insert(new_tissue_infos_vec.begin(), tissue_infos_vector[*riter]);
		}

		// Permute tissue indices according to ordering in input file
		std::vector<tissues_size_t> idx_map(tissue_infos_vector.size());
		for (tissues_size_t old_idx = 0; old_idx < tissue_infos_vector.size(); ++old_idx)
		{
			idx_map[old_idx] = old_idx;
		}
		bool permute = false;
		for (tissues_size_t new_idx = 0; new_idx < new_tissue_infos_vec.size(); ++new_idx)
		{
			tissues_size_t old_idx = GetTissueType(new_tissue_infos_vec[new_idx].m_Name);
			if (old_idx > 0 && old_idx != new_idx)
			{
				idx_map[old_idx] = new_idx;
				permute = true;
			}
		}
		if (permute)
		{
			handler3D->MapTissueIndices(idx_map);
		}

		// Assign new infos vector
		tissue_infos_vector = new_tissue_infos_vec;

		fclose(fp);
		CreateTissueTypeMap();
		return true;
	}
}

bool TissueInfos::SaveDefaultTissueList(const char* filename)
{
	FILE* fp;
	fp = fopen(filename, "w");
	if (fp == nullptr)
	{
		return false;
	}

	TissueInfosVecType::iterator vec_it;
	for (vec_it = tissue_infos_vector.begin() + 1;
			 vec_it != tissue_infos_vector.end(); ++vec_it)
	{
		std::string tissuename1 = vec_it->m_Name;
		boost::replace_all(tissuename1, " ", "_");
		fprintf(fp, "%s %f %f %f %f\n", tissuename1.c_str(), vec_it->m_Color[0], vec_it->m_Color[1], vec_it->m_Color[2], vec_it->m_Opac);
	}
	fclose(fp);
	return true;
}

bool TissueInfos::LoadDefaultTissueList(const char* filename)
{
	FILE* fp;
	fp = fopen(filename, "r");
	if (fp == nullptr)
	{
		InitTissues();
	}
	else
	{
		Color rgb;
		float alpha;
		char name1[1000];

		tissue_infos_vector.clear();
		tissue_infos_vector.resize(1); // Background
		while (fscanf(fp, "%s %f %f %f %f", name1, &rgb.r, &rgb.g, &rgb.b, &alpha) == 5)
		{
			TissueInfo new_tissue_info;
			new_tissue_info.m_Locked = false;
			new_tissue_info.m_Color = rgb;
			new_tissue_info.m_Opac = alpha;
			new_tissue_info.m_Name = name1;
			tissue_infos_vector.push_back(new_tissue_info);
		}
		fclose(fp);
	}

	CreateTissueTypeMap();
	return true;
}

tissues_size_t TissueInfos::GetTissueCount()
{
	return static_cast<tissues_size_t>(tissue_infos_vector.size()) - 1; // Tissue index 0 is the background
}

void TissueInfos::AddTissue(TissueInfo& tissue)
{
	tissue_infos_vector.push_back(tissue);
	tissue_type_map.insert(TissueTypeMapEntryType(str_tolower(tissue.m_Name), GetTissueCount()));
}

void TissueInfos::RemoveTissue(tissues_size_t tissuetype)
{
	// tissueTypeMap.erase(tissueInfosVector[tissuetype].name.toLower());
	tissue_infos_vector.erase(tissue_infos_vector.begin() + tissuetype);
	CreateTissueTypeMap();
}

void TissueInfos::RemoveTissues(const std::set<tissues_size_t>& tissuetypes)
{
	// Remove in descending order
	for (auto riter = tissuetypes.rbegin(); riter != tissuetypes.rend(); ++riter)
	{
		// tissueTypeMap.erase(tissueInfosVector[*riter].name.toLower());
		tissue_infos_vector.erase(tissue_infos_vector.begin() + *riter);
	}
	CreateTissueTypeMap();
}

void TissueInfos::RemoveAllTissues()
{
	tissue_type_map.clear();
	tissue_infos_vector.clear();
	tissue_infos_vector.resize(1); // Background
}

void TissueInfos::CreateTissueTypeMap()
{
	tissue_type_map.clear();
	for (tissues_size_t type = 1; type <= GetTissueCount(); ++type)
	{
		tissue_type_map.insert(TissueTypeMapEntryType(str_tolower(tissue_infos_vector[type].m_Name), type));
	}
}

namespace {
bool is_valid(tissues_size_t i)
{
	return (i <= TissueInfos::GetTissueCount());
}
std::set<tissues_size_t> selection;
} // namespace

std::set<iseg::tissues_size_t> TissueInfos::GetSelectedTissues()
{
	std::set<tissues_size_t> r;
	std::copy_if(selection.begin(), selection.end(), std::inserter(r, r.end()), is_valid);
	return r;
}

void TissueInfos::SetSelectedTissues(const std::set<tissues_size_t>& sel)
{
	if (std::all_of(sel.begin(), sel.end(), is_valid))
	{
		selection = sel;
	}
}

TissueInfos::TissueInfosVecType TissueInfos::tissue_infos_vector;
TissueInfos::TissueTypeMapType TissueInfos::tissue_type_map;

} // namespace iseg
