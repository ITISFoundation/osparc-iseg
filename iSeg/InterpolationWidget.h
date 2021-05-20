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

namespace iseg {

class BrushInteraction;

class InterpolationWidget : public WidgetInterface
{
	Q_OBJECT
public:
	InterpolationWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~InterpolationWidget() override;
	QSize sizeHint() const override;
	void Init() override;
	void NewLoaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	std::string GetName() override { return std::string("Interpolate"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("interpolate.png"))); }

private:
	void OnTissuenrChanged(int i) override;
	void OnSlicenrChanged() override;

	void OnMouseClicked(Point p) override;
	void OnMouseReleased(Point p) override;
	void OnMouseMoved(Point p) override;

	SlicesHandler* m_Handler3D;
	BrushInteraction* m_Brush;

	Q3HBox* m_Hboxoverall;
	Q3VBox* m_Vboxmethods;
	Q3VBox* m_Vboxdataselect;
	Q3VBox* m_Vboxparams;
	Q3VBox* m_Vboxexecute;
	Q3HBox* m_Hboxextra;
	Q3HBox* m_Hboxbatch;
	QLabel* m_TxtSlicenr;
	QSpinBox* m_SbSlicenr;
	QLabel* m_TxtBatchstride;
	QSpinBox* m_SbBatchstride;
	QPushButton* m_Pushexec;
	QPushButton* m_Pushstart;
	QRadioButton* m_RbTissue;
	QRadioButton* m_RbTissueall;
	QRadioButton* m_RbWork;
	QButtonGroup* m_Sourcegroup;
	QRadioButton* m_RbInter;
	//	QRadioButton *rb_intergrey;
	QRadioButton* m_RbExtra;
	QRadioButton* m_RbBatchinter;
	QButtonGroup* m_Modegroup;
	QRadioButton* m_Rb4connectivity;
	QRadioButton* m_Rb8connectivity;
	QButtonGroup* m_Connectivitygroup;
	QCheckBox* m_CbMedianset;
	QCheckBox* m_CbConnectedshapebased;
	QCheckBox* m_CbBrush;
	QLineEdit* m_BrushRadius;
	unsigned short m_Startnr;
	unsigned short m_Nrslices;
	unsigned short m_Tissuenr;

public slots:
	void Handler3DChanged();

private slots:
	void StartslicePressed();
	void Execute();
	void MethodChanged();
	void SourceChanged();
	void BrushChanged();
};

} // namespace iseg
