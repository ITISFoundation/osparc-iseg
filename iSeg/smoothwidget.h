/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SMOOTHWIDGET_3MARCH05
#define SMOOTHWIDGET_3MARCH05

#include <algorithm>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>
//Added by qt3to4:
#include "Addon/qwidget1.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"
#include <q3mimefactory.h>
#include <qpixmap.h>

class smooth_widget : public QWidget1
{
	Q_OBJECT
public:
	smooth_widget(SlicesHandler *hand3D, QWidget *parent = 0,
								const char *name = 0, Qt::WindowFlags wFlags = 0);
	~smooth_widget();
	QSize sizeHint() const;
	void init();
	void newloaded();
	FILE *SaveParams(FILE *fp, int version);
	FILE *LoadParams(FILE *fp, int version);
	void hideparams_changed();
	std::string GetName() { return std::string("Smooth"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("smoothing.png")).ascii());
	};

private:
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	Q3HBox *hboxoverall;
	Q3VBox *vboxmethods;
	Q3HBox *hbox1;
	Q3HBox *hbox2;
	Q3HBox *hbox3;
	Q3HBox *hbox4;
	Q3HBox *hbox5;
	//	Q3HBox *hbox6;
	Q3VBox *vbox1;
	Q3VBox *vbox2;
	QLabel *txt_n;
	QLabel *txt_sigma1;
	QLabel *txt_sigma2;
	QLabel *txt_dt;
	QLabel *txt_iter;
	QLabel *txt_k;
	QLabel *txt_restrain1;
	QLabel *txt_restrain2;
	QSlider *sl_sigma;
	QSlider *sl_k;
	QSlider *sl_restrain;
	QSpinBox *sb_n;
	QSpinBox *sb_iter;
	QSpinBox *sb_kmax;
	//	QSpinBox *sb_restrainmax;
	QPushButton *pushexec;
	QPushButton *contdiff;
	QRadioButton *rb_gaussian;
	QRadioButton *rb_average;
	QRadioButton *rb_median;
	QRadioButton *rb_sigmafilter;
	QRadioButton *rb_anisodiff;
	QButtonGroup *modegroup;
	QCheckBox *allslices;
	bool dontundo;

signals:
	void begin_datachange(common::DataSelection &dataSelection,
												QWidget *sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget *sender = NULL,
											common::EndUndoAction undoAction = common::EndUndo);

public slots:
	void slicenr_changed();

private slots:
	void bmphand_changed(bmphandler *bmph);
	void execute();
	void continue_diff();
	void method_changed(int);
	void sigmaslider_changed(int newval);
	void kslider_changed(int newval);
	void n_changed(int newval);
	void kmax_changed(int newval);
	void slider_pressed();
	void slider_released();
};

#endif