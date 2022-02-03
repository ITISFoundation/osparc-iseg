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

#include "SlicesHandler.h"

#include "Interface/WidgetInterface.h"

#include "Data/Property.h"

namespace iseg {

class BrushInteraction;

class InterpolationWidget : public WidgetInterface
{
	Q_OBJECT
public:
	InterpolationWidget(SlicesHandler* hand3D);
	~InterpolationWidget() override;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	std::string GetName() override { return std::string("Interpolate"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath("interpolate.png")); }

private:
	void OnTissuenrChanged(int i) override;
	void OnSlicenrChanged() override;

	void OnMouseClicked(Point p) override;
	void OnMouseReleased(Point p) override;
	void OnMouseMoved(Point p) override;

	void StartslicePressed();
	void Execute();

	SlicesHandler* m_Handler3D;
	BrushInteraction* m_Brush = nullptr;

	PropertyInt_ptr m_SbSlicenr;
	PropertyInt_ptr m_SbBatchstride;

	enum eSourceType {
		kTissue = 0,
		kTissueAll,
		kWork
	};
	PropertyEnum_ptr m_Sourcegroup;

	enum eModeType {
		kInterpolation = 0,
		kExtrapolation,
		kBatchInterpolation
	};
	PropertyEnum_ptr m_Modegroup;

	enum eConnectivityType {
		kCityBlock = 0,
		kChess
	};
	PropertyEnum_ptr m_Connectivitygroup;

	PropertyBool_ptr m_CbMedianset;
	PropertyBool_ptr m_CbConnectedshapebased;
	PropertyBool_ptr m_CbBrush;
	PropertyReal_ptr m_BrushRadius;

	PropertyButton_ptr m_Pushexec;

	unsigned short m_Startnr;
	unsigned short m_Nrslices;
	unsigned short m_Tissuenr;

public slots:
	void Handler3DChanged();

private slots:
	void MethodChanged();
	void SourceChanged();
	void BrushChanged();
};

} // namespace iseg
