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

#include "Data/Types.h"

#include <qstring.h>

#include <map>
#include <set>
#include <vector>

namespace iseg {

enum eTissueLayerOverlayMode {
	Normal, // TODO
};

struct TissueLayerInfoStruct
{
	TissueLayerInfoStruct()
	{
		m_Name = "?";
		m_Visible = true;
		m_Opac = 1.0f;
		m_Mode = Normal;
	};

	TissueLayerInfoStruct(const TissueLayerInfoStruct& other)
	{
		m_Name = other.m_Name;
		m_Visible = other.m_Visible;
		m_Opac = other.m_Opac;
		m_Mode = other.m_Mode;
	};

	TissueLayerInfoStruct& operator=(const TissueLayerInfoStruct& other)
	= default;;

	QString m_Name;
	bool m_Visible;
	float m_Opac;
	eTissueLayerOverlayMode m_Mode;
};

using TissueLayerInfosVecType = std::vector<TissueLayerInfoStruct>;
using TissueLayerIndexMapType = std::map<QString, tissuelayers_size_t>;
using TissueLayerIndexMapEntryType = std::pair<QString, tissuelayers_size_t>;

class TissueLayerInfos
{
public:
	static tissuelayers_size_t GetTissueLayerCount();
	static TissueLayerInfoStruct*
			GetTissueLayerInfo(tissuelayers_size_t layerIdx);

	static tissuelayers_size_t GetTissueLayerIndex(QString layerName);
	static QString GetTissueLayerName(tissuelayers_size_t layerIdx);
	static bool GetTissueLayerVisible(tissuelayers_size_t layerIdx);
	static float GetTissueLayerOpac(tissuelayers_size_t layerIdx);
	static eTissueLayerOverlayMode
			GetTissueLayerOverlayMode(tissuelayers_size_t layerIdx);

	static void SetTissueLayerName(tissuelayers_size_t layerIdx, QString val);
	static void SetTissueLayerVisible(tissuelayers_size_t layerIdx, bool val);
	static void SetTissueLayersVisible(bool val);
	static void SetTissueLayerOpac(tissuelayers_size_t layerIdx, float val);
	static void SetTissueLayerOverlayMode(tissuelayers_size_t layerIdx, eTissueLayerOverlayMode val);

	static void AddTissueLayer(TissueLayerInfoStruct& layer);
	static void RemoveTissueLayer(tissuelayers_size_t layerIdx);
	static void RemoveAllTissueLayers();

protected:
	static void CreateTissueLayerIndexMap();

protected:
	static TissueLayerInfosVecType tissue_layer_infos_vector;
	static TissueLayerIndexMapType tissue_layer_index_map;
};

} // namespace iseg
