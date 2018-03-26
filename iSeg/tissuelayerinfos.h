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

#include "Core/Types.h"

#include <vector>
#include <map>
#include <set>
#include <qstring.h>

enum TissueLayerOverlayMode {
	Normal,
	// TODO
};

struct TissueLayerInfoStruct
{
	TissueLayerInfoStruct()
	{
		name = "?";
		visible = true;
		opac = 1.0f;
		mode = Normal;
	};

	TissueLayerInfoStruct(const TissueLayerInfoStruct& other)
	{
		name = other.name;
		visible = other.visible;
		opac = other.opac;
		mode = other.mode;
	};

	TissueLayerInfoStruct& operator=(const TissueLayerInfoStruct& other)
	{
		name = other.name;
		visible = other.visible;
		opac = other.opac;
		mode = other.mode;
		return *this;
	};

	QString name;
	bool visible;
	float opac;
	TissueLayerOverlayMode mode;
};

typedef std::vector<TissueLayerInfoStruct> TissueLayerInfosVecType;
typedef std::map<QString, tissuelayers_size_t> TissueLayerIndexMapType;
typedef std::pair<QString, tissuelayers_size_t> TissueLayerIndexMapEntryType;


class TissueLayerInfos
{
public:
	static tissuelayers_size_t GetTissueLayerCount();
	static TissueLayerInfoStruct *GetTissueLayerInfo(tissuelayers_size_t layerIdx);

	static tissuelayers_size_t GetTissueLayerIndex(QString layerName);
	static QString GetTissueLayerName(tissuelayers_size_t layerIdx);
	static bool GetTissueLayerVisible(tissuelayers_size_t layerIdx);
	static float GetTissueLayerOpac(tissuelayers_size_t layerIdx);
	static TissueLayerOverlayMode GetTissueLayerOverlayMode(tissuelayers_size_t layerIdx);

	static void SetTissueLayerName(tissuelayers_size_t layerIdx, QString val);
	static void SetTissueLayerVisible(tissuelayers_size_t layerIdx, bool val);
	static void SetTissueLayersVisible(bool val);
	static void SetTissueLayerOpac(tissuelayers_size_t layerIdx, float val);
	static void SetTissueLayerOverlayMode(tissuelayers_size_t layerIdx, TissueLayerOverlayMode val);

	static void AddTissueLayer(TissueLayerInfoStruct &layer);
	static void RemoveTissueLayer(tissuelayers_size_t layerIdx);
	static void RemoveAllTissueLayers();

protected:
	static void CreateTissueLayerIndexMap();

protected:
	static TissueLayerInfosVecType tissueLayerInfosVector;
	static TissueLayerIndexMapType tissueLayerIndexMap;
};

