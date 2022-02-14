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

#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>

class QStackedLayout;

namespace iseg {

class ImageForestingTransformAdaptFuzzy;
class ImageForestingTransformFastMarching;
class SlicesHandler;
class Bmphandler;

class FastmarchingFuzzyWidget : public WidgetInterface
{
	Q_OBJECT
public:
	FastmarchingFuzzyWidget(SlicesHandler* hand3D);
	~FastmarchingFuzzyWidget() override;
	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("Fuzzy"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath("fuzzy.png")); }

protected:
	void OnSlicenrChanged() override;
	void OnMouseClicked(Point p) override;
	void OnMouseReleased(Point p) override;
	void OnMouseMoved(Point p) override;
	void BmpChanged() override;

private:
	void Getrange();
	void Execute();

	float* m_Map;
	ImageForestingTransformFastMarching* m_IfTmarch;
	ImageForestingTransformAdaptFuzzy* m_IfTfuzzy;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	QStackedLayout* m_ParamsStackLayout;
	QSlider* m_SlSigma;
	QSlider* m_SlThresh;
	QSpinBox* m_SbThresh;
	QSlider* m_SlM1;
	QSpinBox* m_SbM1;
	QSlider* m_SlS1;
	QSpinBox* m_SbS1;
	QSlider* m_SlS2;
	QSpinBox* m_SbS2;
	QSlider* m_SlExtend;
	QRadioButton* m_RbFastmarch;
	QRadioButton* m_RbFuzzy;
	QRadioButton* m_RbDrag;
	QRadioButton* m_RbSlider;

	float m_Sigma, m_Thresh, m_M1, m_S1, m_S2;
	float m_Sigmamax;
	float m_Extend, m_Extendmax;
	unsigned m_Area;
	std::vector<Point> m_VpdynArg;

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void SlextendChanged(int i);
	void SlextendPressed();
	void SlextendReleased();
	void MethodChanged();
	void InteractChanged();
	void SpinboxChanged();
	void SliderChanged();
};

} // namespace iseg
