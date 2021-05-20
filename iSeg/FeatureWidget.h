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

#include "Interface/WidgetInterface.h"

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Core/ImageForestingTransform.h"

#include <qlabel.h>

namespace iseg {

class FeatureWidget : public WidgetInterface
{
	Q_OBJECT
public:
	FeatureWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	void Init() override;
	void NewLoaded() override;
	std::string GetName() override { return std::string("Feature"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("feature.png")).ascii()); }

private:
	void OnSlicenrChanged() override;
	void OnMouseClicked(Point p) override;
	void OnMouseMoved(Point p) override;
	void OnMouseReleased(Point p) override;

	bool m_Selecting;
	std::vector<Point> m_Dynamic;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	QLabel* m_LbMap;
	QLabel* m_LbAv;
	QLabel* m_LbStddev;
	QLabel* m_LbMin;
	QLabel* m_LbMax;
	QLabel* m_LbPt;
	QLabel* m_LbTissue;
	QLabel* m_LbGrey;
	QLabel* m_LbMapValue;
	QLabel* m_LbAvValue;
	QLabel* m_LbStddevValue;
	QLabel* m_LbMinValue;
	QLabel* m_LbMaxValue;
	QLabel* m_LbPtValue;
	QLabel* m_LbGreyValue;
	QLabel* m_LbWorkMapValue;
	QLabel* m_LbWorkAvValue;
	QLabel* m_LbWorkStddevValue;
	QLabel* m_LbWorkMinValue;
	QLabel* m_LbWorkMaxValue;
	QLabel* m_LbWorkPtValue;
	QLabel* m_LbWorkGreyValue;
	QLabel* m_LbTissuename;
	QLabel* m_LbDummy;

	Point m_Pstart;
};

} // namespace iseg
