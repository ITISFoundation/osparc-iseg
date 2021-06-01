/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SMOOTHWIDGET_3MARCH05
#define SMOOTHWIDGET_3MARCH05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include <q3vbox.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <QLabel>
#include <qlayout.h>
#include <qpixmap.h>
#include <QPushButton>
#include <QRadioButton>
#include <qsize.h>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

#include <algorithm>

namespace iseg {

class SmoothingWidget : public WidgetInterface
{
	Q_OBJECT
public:
	SmoothingWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~SmoothingWidget() override;
	QSize sizeHint() const override;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("Smooth"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("smoothing.png"))); }

private:
	void OnSlicenrChanged() override;

	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;

	Q3HBox* m_Hboxoverall;
	Q3VBox* m_Vboxmethods;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	Q3HBox* m_Hbox5;
	//	Q3HBox *hbox6;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	QLabel* m_TxtN;
	QLabel* m_TxtSigma1;
	QLabel* m_TxtSigma2;
	QLabel* m_TxtDt;
	QLabel* m_TxtIter;
	QLabel* m_TxtK;
	QLabel* m_TxtRestrain1;
	QLabel* m_TxtRestrain2;
	QSlider* m_SlSigma;
	QSlider* m_SlK;
	QSlider* m_SlRestrain;
	QSpinBox* m_SbN;
	QSpinBox* m_SbIter;
	QSpinBox* m_SbKmax;
	//	QSpinBox *sb_restrainmax;
	QCheckBox* m_Allslices;
	QRadioButton* m_RbGaussian;
	QRadioButton* m_RbAverage;
	QRadioButton* m_RbMedian;
	QRadioButton* m_RbSigmafilter;
	QRadioButton* m_RbAnisodiff;
	QButtonGroup* m_Modegroup;
	QPushButton* m_Pushexec;
	QPushButton* m_Contdiff;

	bool m_Dontundo;

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void Execute();
	void ContinueDiff();
	void MethodChanged(int);
	void SigmasliderChanged(int newval);
	void KsliderChanged(int newval);
	void NChanged(int newval);
	void KmaxChanged(int newval);
	void SliderPressed();
	void SliderReleased();
};

} // namespace iseg

#endif
