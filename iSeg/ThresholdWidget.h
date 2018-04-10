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

class ThresholdWidget : public WidgetInterface
{
	Q_OBJECT
public:
	ThresholdWidget(SlicesHandler *hand3D, QWidget *parent = 0,
			const char *name = 0, Qt::WindowFlags wFlags = 0);
	~ThresholdWidget();
	FILE *SaveParams(FILE *fp, int version) override;
	FILE *LoadParams(FILE *fp, int version) override;
	QSize sizeHint() const override;
	void init() override;
	void newloaded() override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("Threshold"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("thresholding.png"))); }

private:
	void on_tissuenr_changed(int i) override;
	void on_slicenr_changed() override;

	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	std::vector<QString> filenames;
	Q3HBox *hboxoverall;
	Q3VBox *vboxmethods;
	Q3HBox *hbox1;
	Q3HBox *hbox1b;
	Q3HBox *hbox2;
	Q3HBox *hbox2a;
	Q3HBox *hbox3;
	Q3HBox *hbox4;
	Q3HBox *hbox5;
	//	Q3HBox *hbox6;
	Q3HBox *hbox7;
	Q3HBox *hbox8;
	Q3HBox *hboxfilenames;
	Q3VBox *vbox1;
	Q3VBox *vbox2;
	Q3VBox *vbox3;
	Q3VBox *vbox4;
	Q3VBox *vbox5;
	QLabel *txt_nrtissues;
	QLabel *txt_dim;
	QLabel *txt_useCenterFile;
	QLabel *txt_tissuenr;
	QLabel *txt_px;
	QLabel *txt_py;
	QLabel *txt_lx;
	QLabel *txt_ly;
	QLabel *txt_iternr;
	QLabel *txt_converge;
	QLabel *txt_minpix;
	QLabel *txt_slider;
	QLabel *txt_ratio;
	QLabel *txt_lower;
	QLabel *txt_upper;
	QLabel *txt_filename;
	QSlider *slider;
	QSlider *ratio;
	QSpinBox *sb_nrtissues;
	QSpinBox *sb_dim;
	QSpinBox *sb_tissuenr;
	QSpinBox *sb_px;
	QSpinBox *sb_py;
	QSpinBox *sb_lx;
	QSpinBox *sb_ly;
	QSpinBox *sb_iternr;
	QSpinBox *sb_converge;
	QSpinBox *sb_minpix;
	QPushButton *pushexec;
	QPushButton *pushfilename;
	//	QPushButton *pushrange;
	QCheckBox *subsect;
	QRadioButton *rb_manual;
	QRadioButton *rb_histo;
	QRadioButton *rb_kmeans;
	QRadioButton *rb_EM;
	QButtonGroup *modegroup;
	QCheckBox *allslices;
	float lower, upper;
	float threshs[21];
	float weights[20];
	float *bits[20];
	unsigned bits1[20];
	bool dontundo;
	QLineEdit *le_borderval;
	QLineEdit *le_filename;
	QPushButton *pb_saveborders;
	QPushButton *pb_loadborders;
	//QButtonGroup *buttonGroup;
	QCheckBox *buttonR;
	QCheckBox *buttonG;
	QCheckBox *buttonB;
	QCheckBox *buttonA;
	QCheckBox *cb_useCenterFile;
	QLineEdit *le_centerFilename;
	QPushButton *pushcenterFilename;
	QString centerFilename;

private slots:
	void bmphand_changed(bmphandler *bmph);
	void subsect_toggled();
	void execute();
	void method_changed(int);
	void getrange();
	void dim_changed(int newval);
	void nrtissues_changed(int newval);
	void slider_changed(int newval);
	void slider_pressed();
	void slider_released();
	void bmp_changed();
	void saveborders_execute();
	void loadborders_execute();
	void le_borderval_returnpressed();
	void select_pushed();
	void RGBA_changed(int change);
	void useCenterFile_changed(int change);
	void selectCenterFile_pushed();
};

} // namespace iseg
