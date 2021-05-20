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

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

class HystereticGrowingWidget : public WidgetInterface
{
	Q_OBJECT
public:
	HystereticGrowingWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~HystereticGrowingWidget() override = default;
	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	QSize sizeHint() const override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("Growing"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("growing.png"))); }

private:
	void OnSlicenrChanged() override;
	void OnMouseClicked(Point p) override;
	void OnMouseMoved(Point p) override;
	void OnMouseReleased(Point p) override;
	void BmpChanged() override;

	void Init1();
	void Getrange();
	void GetrangeSub(float ll, float uu, float ul, float lu);
	void Execute1();

	Point m_P1;
	Point m_LastPt;
	Bmphandler* m_Bmphand;
	SlicesHandler* m_Handler3D;
	unsigned short m_Activeslice;
	float m_UpperLimit;
	float m_LowerLimit;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox2a;
	Q3HBox* m_Hbox3;
	Q3VBox* m_Vbox1;
	Q3VBox* m_Vbox2;
	Q3VBox* m_Vbox3;
	Q3VBox* m_Vbox4;
	Q3VBox* m_Vbox5;
	Q3VBox* m_Vbox6;
	Q3VBox* m_Vbox7;
	QLabel* m_TxtLower;
	QLabel* m_TxtUpper;
	QLabel* m_TxtLowerhyster;
	QLabel* m_TxtUpperhyster;
	QSlider* m_SlLower;
	QSlider* m_SlUpper;
	QSlider* m_SlLowerhyster;
	QSlider* m_SlUpperhyster;
	QPushButton* m_Pushexec;
	QPushButton* m_Drawlimit;
	QPushButton* m_Clearlimit;
	bool m_Limitdrawing;
	QCheckBox* m_Autoseed;
	QCheckBox* m_Allslices;
	std::vector<Point> m_Vp1;
	std::vector<Point> m_Vpdyn;
	bool m_Dontundo;
	QLineEdit* m_LeBordervall;
	QLineEdit* m_LeBordervalu;
	QLineEdit* m_LeBordervallh;
	QLineEdit* m_LeBordervaluh;
	QPushButton* m_PbSaveborders;
	QPushButton* m_PbLoadborders;

signals:
	void Vp1Changed(std::vector<Point>* vp1_arg);
	void Vp1dynChanged(std::vector<Point>* vp1_arg, std::vector<Point>* vpdyn_arg);

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void AutoToggled();
	void UpdateVisible();
	void Execute();
	void Limitpressed();
	void Clearpressed();
	void SliderChanged();
	void SliderPressed();
	void SliderReleased();
	void SavebordersExecute();
	void LoadbordersExecute();
	void LeBordervallReturnpressed();
	void LeBordervaluReturnpressed();
	void LeBordervallhReturnpressed();
	void LeBordervaluhReturnpressed();
};

} // namespace iseg
