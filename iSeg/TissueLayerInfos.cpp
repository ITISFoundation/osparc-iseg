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

#include "TissueLayerInfos.h"

namespace iseg {

tissuelayers_size_t TissueLayerInfos::GetTissueLayerCount()
{
	return (tissuelayers_size_t)tissue_layer_infos_vector.size();
}

TissueLayerInfoStruct*
		TissueLayerInfos::GetTissueLayerInfo(tissuelayers_size_t layerIdx)
{
	return &tissue_layer_infos_vector[layerIdx];
}

tissuelayers_size_t TissueLayerInfos::GetTissueLayerIndex(QString layerName)
{
	if (tissue_layer_index_map.find(layerName) != tissue_layer_index_map.end())
	{
		return tissue_layer_index_map[layerName];
	}
	return 0;
}

QString TissueLayerInfos::GetTissueLayerName(tissuelayers_size_t layerIdx)
{
	return tissue_layer_infos_vector[layerIdx].m_Name;
}

bool TissueLayerInfos::GetTissueLayerVisible(tissuelayers_size_t layerIdx)
{
	return tissue_layer_infos_vector[layerIdx].m_Visible;
}

float TissueLayerInfos::GetTissueLayerOpac(tissuelayers_size_t layerIdx)
{
	return tissue_layer_infos_vector[layerIdx].m_Opac;
}

eTissueLayerOverlayMode
		TissueLayerInfos::GetTissueLayerOverlayMode(tissuelayers_size_t layerIdx)
{
	return tissue_layer_infos_vector[layerIdx].m_Mode;
}

void TissueLayerInfos::SetTissueLayerName(tissuelayers_size_t layerIdx, QString val)
{
	tissue_layer_index_map.erase(tissue_layer_infos_vector[layerIdx].m_Name);
	tissue_layer_index_map.insert(TissueLayerIndexMapEntryType(val, layerIdx));
	tissue_layer_infos_vector[layerIdx].m_Name = val;
}

void TissueLayerInfos::SetTissueLayerVisible(tissuelayers_size_t layerIdx, bool val)
{
	tissue_layer_infos_vector[layerIdx].m_Visible = val;
}

void TissueLayerInfos::SetTissueLayersVisible(bool val)
{
	TissueLayerInfosVecType::iterator vec_it;
	for (vec_it = tissue_layer_infos_vector.begin() + 1;
			 vec_it != tissue_layer_infos_vector.end(); ++vec_it)
	{
		vec_it->m_Visible = val;
	}
}

void TissueLayerInfos::SetTissueLayerOpac(tissuelayers_size_t layerIdx, float val)
{
	tissue_layer_infos_vector[layerIdx].m_Opac = val;
}

void TissueLayerInfos::SetTissueLayerOverlayMode(tissuelayers_size_t layerIdx, eTissueLayerOverlayMode val)
{
	tissue_layer_infos_vector[layerIdx].m_Mode = val;
}

void TissueLayerInfos::AddTissueLayer(TissueLayerInfoStruct& layer)
{
	tissue_layer_infos_vector.push_back(layer);
	tissue_layer_index_map.insert(TissueLayerIndexMapEntryType(layer.m_Name, GetTissueLayerCount() - 1));
}

void TissueLayerInfos::RemoveTissueLayer(tissuelayers_size_t layerIdx)
{
	tissue_layer_infos_vector.erase(tissue_layer_infos_vector.begin() + layerIdx);
	CreateTissueLayerIndexMap();
}

void TissueLayerInfos::RemoveAllTissueLayers()
{
	tissue_layer_index_map.clear();
	tissue_layer_infos_vector.clear();
}

void TissueLayerInfos::CreateTissueLayerIndexMap()
{
	tissue_layer_index_map.clear();
	for (tissuelayers_size_t idx = 0; idx < GetTissueLayerCount(); ++idx)
	{
		tissue_layer_index_map.insert(TissueLayerIndexMapEntryType(tissue_layer_infos_vector[idx].m_Name, idx));
	}
}

TissueLayerInfosVecType TissueLayerInfos::tissue_layer_infos_vector;
TissueLayerIndexMapType TissueLayerInfos::tissue_layer_index_map;

} // namespace iseg
