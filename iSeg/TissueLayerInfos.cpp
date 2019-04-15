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

#include "TissueLayerInfos.h"

namespace iseg {

tissuelayers_size_t TissueLayerInfos::GetTissueLayerCount()
{
	return (tissuelayers_size_t)tissueLayerInfosVector.size();
}

TissueLayerInfoStruct*
	TissueLayerInfos::GetTissueLayerInfo(tissuelayers_size_t layerIdx)
{
	return &tissueLayerInfosVector[layerIdx];
}

tissuelayers_size_t TissueLayerInfos::GetTissueLayerIndex(QString layerName)
{
	if (tissueLayerIndexMap.find(layerName) != tissueLayerIndexMap.end())
	{
		return tissueLayerIndexMap[layerName];
	}
	return 0;
}

QString TissueLayerInfos::GetTissueLayerName(tissuelayers_size_t layerIdx)
{
	return tissueLayerInfosVector[layerIdx].name;
}

bool TissueLayerInfos::GetTissueLayerVisible(tissuelayers_size_t layerIdx)
{
	return tissueLayerInfosVector[layerIdx].visible;
}

float TissueLayerInfos::GetTissueLayerOpac(tissuelayers_size_t layerIdx)
{
	return tissueLayerInfosVector[layerIdx].opac;
}

TissueLayerOverlayMode
	TissueLayerInfos::GetTissueLayerOverlayMode(tissuelayers_size_t layerIdx)
{
	return tissueLayerInfosVector[layerIdx].mode;
}

void TissueLayerInfos::SetTissueLayerName(tissuelayers_size_t layerIdx,
										  QString val)
{
	tissueLayerIndexMap.erase(tissueLayerInfosVector[layerIdx].name);
	tissueLayerIndexMap.insert(TissueLayerIndexMapEntryType(val, layerIdx));
	tissueLayerInfosVector[layerIdx].name = val;
}

void TissueLayerInfos::SetTissueLayerVisible(tissuelayers_size_t layerIdx,
											 bool val)
{
	tissueLayerInfosVector[layerIdx].visible = val;
}

void TissueLayerInfos::SetTissueLayersVisible(bool val)
{
	TissueLayerInfosVecType::iterator vecIt;
	for (vecIt = tissueLayerInfosVector.begin() + 1;
		 vecIt != tissueLayerInfosVector.end(); ++vecIt)
	{
		vecIt->visible = val;
	}
}

void TissueLayerInfos::SetTissueLayerOpac(tissuelayers_size_t layerIdx,
										  float val)
{
	tissueLayerInfosVector[layerIdx].opac = val;
}

void TissueLayerInfos::SetTissueLayerOverlayMode(tissuelayers_size_t layerIdx,
												 TissueLayerOverlayMode val)
{
	tissueLayerInfosVector[layerIdx].mode = val;
}

void TissueLayerInfos::AddTissueLayer(TissueLayerInfoStruct& layer)
{
	tissueLayerInfosVector.push_back(layer);
	tissueLayerIndexMap.insert(
		TissueLayerIndexMapEntryType(layer.name, GetTissueLayerCount() - 1));
}

void TissueLayerInfos::RemoveTissueLayer(tissuelayers_size_t layerIdx)
{
	tissueLayerInfosVector.erase(tissueLayerInfosVector.begin() + layerIdx);
	CreateTissueLayerIndexMap();
}

void TissueLayerInfos::RemoveAllTissueLayers()
{
	tissueLayerIndexMap.clear();
	tissueLayerInfosVector.clear();
}

void TissueLayerInfos::CreateTissueLayerIndexMap()
{
	tissueLayerIndexMap.clear();
	for (tissuelayers_size_t idx = 0; idx < GetTissueLayerCount(); ++idx)
	{
		tissueLayerIndexMap.insert(TissueLayerIndexMapEntryType(
			tissueLayerInfosVector[idx].name, idx));
	}
}

TissueLayerInfosVecType TissueLayerInfos::tissueLayerInfosVector;
TissueLayerIndexMapType TissueLayerInfos::tissueLayerIndexMap;

}// namespace iseg