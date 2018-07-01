/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
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

#include <q3listbox.h>
#include <q3mimefactory.h>
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
	InterpolationWidget(SlicesHandler *hand3D, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~InterpolationWidget();
	QSize sizeHint() const override;
	void init() override;
	void newloaded() override;
	FILE *SaveParams(FILE *fp, int version) override;
	FILE *LoadParams(FILE *fp, int version) override;
	std::string GetName() override { return std::string("Interpolate"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("interpolate.png"))); }

private:
	void on_tissuenr_changed(int i) override;
	void on_slicenr_changed() override;
	
	void on_mouse_clicked(Point p) override;
	void on_mouse_released(Point p) override;
	void on_mouse_moved(Point p) override;

	SlicesHandler *handler3D;
	BrushInteraction *brush;

	Q3HBox *hboxoverall;
	Q3VBox *vboxmethods;
	Q3VBox *vboxdataselect;
	Q3VBox *vboxparams;
	Q3VBox *vboxexecute;
	Q3HBox *hboxextra;
	Q3HBox *hboxbatch;
	QLabel *txt_slicenr;
	QSpinBox *sb_slicenr;
	QLabel *txt_batchstride;
	QSpinBox *sb_batchstride;
	QPushButton *pushexec;
	QPushButton *pushstart;
	QRadioButton *rb_tissue;
	QRadioButton *rb_tissueall;
	QRadioButton *rb_work;
	QButtonGroup *sourcegroup;
	QRadioButton *rb_inter;
	//	QRadioButton *rb_intergrey;
	QRadioButton *rb_extra;
	QRadioButton *rb_batchinter;
	QButtonGroup *modegroup;
	QRadioButton *rb_4connectivity;
	QRadioButton *rb_8connectivity;
	QButtonGroup *connectivitygroup;
	QCheckBox *cb_medianset;
	QCheckBox *cb_connectedshapebased;
	QLineEdit *brush_radius;
	unsigned short startnr;
	unsigned short nrslices;
	unsigned short tissuenr;

public slots:
	void handler3D_changed();

private slots:
	void startslice_pressed();
	void execute();
	void method_changed();
	void source_changed();
	void brush_changed();
};

} // namespace iseg
