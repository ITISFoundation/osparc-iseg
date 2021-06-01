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
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include <QLabel>
#include <QPushButton>
#include <QSlider>

namespace iseg {

class ImageForestingTransformRegionGrowingWidget : public WidgetInterface
{
	Q_OBJECT
public:
	ImageForestingTransformRegionGrowingWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~ImageForestingTransformRegionGrowingWidget() override;
	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("IFT"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("iftrg.png"))); }

protected:
	void OnTissuenrChanged(int i) override;
	void OnSlicenrChanged() override;

	void OnMouseClicked(Point p) override;
	void OnMouseReleased(Point p) override;
	void OnMouseMoved(Point p) override;
	void BmpChanged() override;

private:
	void Init1();
	void Removemarks(Point p);
	void Getrange();

	float* m_Lbmap;
	ImageForestingTransformRegionGrowing* m_IfTrg;
	Point m_LastPt;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	QSlider* m_SlThresh;
	QPushButton* m_Pushexec;
	QPushButton* m_Pushclear;
	QPushButton* m_Pushremove;

	unsigned m_Tissuenr;
	float m_Thresh;
	float m_Maxthresh;
	std::vector<Mark> m_Vm;
	std::vector<Mark> m_Vmempty;
	std::vector<Point> m_Vmdyn;
	unsigned m_Area;

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void Execute();
	void Clearmarks();
	void SliderChanged(int i);
	void SliderPressed();
	void SliderReleased();
};

} // namespace iseg
