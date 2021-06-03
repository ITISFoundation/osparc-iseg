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
#include <QSlider>
#include <QSpinBox>

namespace iseg {

class SlicesHandler;
class Bmphandler;

class WatershedWidget : public WidgetInterface
{
	Q_OBJECT
public:
	WatershedWidget(SlicesHandler* hand3D);
	~WatershedWidget() override;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	std::string GetName() override { return std::string("Watershed"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("watershed.png"))); }

private:
	void OnSlicenrChanged() override;
	void MarksChanged() override;

	void Recalc1();

	unsigned int* m_Usp;
	int m_SbhOld;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	QSpinBox* m_SbH;
	QSlider* m_SlH;
	QPushButton* m_BtnExec;

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void HslChanged();
	void SliderPressed();
	void SliderReleased();
	void HsbChanged(int value);
	void Execute();
	void Recalc();
};

} // namespace iseg
