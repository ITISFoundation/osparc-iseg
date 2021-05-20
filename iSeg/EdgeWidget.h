/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef EDGEWIDGET_3MARCH05
#define EDGEWIDGET_3MARCH05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

class EdgeWidget : public WidgetInterface
{
	Q_OBJECT
public:
	EdgeWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~EdgeWidget() override;
	QSize sizeHint() const override;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void HideParamsChanged() override;
	std::string GetName() override { return std::string("Edge"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("edge.png"))); }

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
	Q3VBox* m_Vbox1;
	QLabel* m_TxtSigmal;
	QLabel* m_TxtSigma2;
	QLabel* m_TxtThresh11;
	QLabel* m_TxtThresh12;
	QLabel* m_TxtThresh21;
	QLabel* m_TxtThresh22;
	QSlider* m_SlSigma;
	QSlider* m_SlThresh1;
	QSlider* m_SlThresh2;
	QCheckBox* m_Cb3d;
	QPushButton* m_BtnExportCenterlines;
	QPushButton* m_BtnExec;

	QRadioButton* m_RbSobel;
	QRadioButton* m_RbLaplacian;
	QRadioButton* m_RbInterquartile;
	QRadioButton* m_RbMomentline;
	QRadioButton* m_RbGaussline;
	QRadioButton* m_RbCanny;
	QRadioButton* m_RbLaplacianzero;
	QRadioButton* m_RbCenterlines;
	QButtonGroup* m_Modegroup;

private slots:
	void BmphandChanged(Bmphandler* bmph);
	void Execute();
	void ExportCenterlines();
	void MethodChanged(int);
	void SliderChanged(int newval);
};

} // namespace iseg

#endif
