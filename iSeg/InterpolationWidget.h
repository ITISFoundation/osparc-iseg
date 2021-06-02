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
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("interpolate.png"))); }

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

	std::shared_ptr<PropertyInt> m_SbSlicenr;
	std::shared_ptr<PropertyInt> m_SbBatchstride;

	enum eSourceType {
		kTissue = 0,
		kTissueAll,
		kWork
	};
	std::shared_ptr<PropertyEnum> m_Sourcegroup;

	enum eModeType {
		kInterpolation = 0,
		kExtrapolation,
		kBatchInterpolation
	};
	std::shared_ptr<PropertyEnum> m_Modegroup;

	enum eConnectivityType {
		k4Connectivity = 0,
		k8Connectivity
	};
	std::shared_ptr<PropertyEnum> m_Connectivitygroup;

	std::shared_ptr<PropertyBool> m_CbMedianset;
	std::shared_ptr<PropertyBool> m_CbConnectedshapebased;
	std::shared_ptr<PropertyBool> m_CbBrush;
	std::shared_ptr<PropertyReal> m_BrushRadius;

	std::shared_ptr<PropertyButton> m_Pushexec;

	unsigned short m_Startnr;
	unsigned short m_Nrslices;
	unsigned short m_Tissuenr;

	boost::signals2::scoped_connection m_ModeConnection;
	boost::signals2::scoped_connection m_SourceConnection;

public slots:
	void Handler3DChanged();

private slots:
	void MethodChanged();
	void SourceChanged();
	void BrushChanged();
};

} // namespace iseg
