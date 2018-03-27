/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef FMF_31March05
#define FMF_31March05

#include "Addon/qwidget1.h"

#include "Core/Point.h"
#include "Core/common.h"

#include <q3hbox.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>
//Added by qt3to4:
#include <q3mimefactory.h>
#include <qpixmap.h>

#include <algorithm>

class IFT_adaptfuzzy;
class IFT_fastmarch;
class SlicesHandler;
class bmphandler;

class FastMarchFuzzy_widget : public QWidget1
{
	Q_OBJECT
public:
	FastMarchFuzzy_widget(SlicesHandler *hand3D, QWidget *parent = 0,
												const char *name = 0, Qt::WindowFlags wFlags = 0);
	~FastMarchFuzzy_widget();
	void init();
	void newloaded();
	void cleanup();
	QSize sizeHint() const;
	FILE *SaveParams(FILE *fp, int version);
	FILE *LoadParams(FILE *fp, int version);
	void hideparams_changed();
	std::string GetName() { return std::string("Fuzzy"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("fuzzy.png")).ascii());
	};

private:
	float *map;
	IFT_fastmarch *IFTmarch;
	IFT_adaptfuzzy *IFTfuzzy;
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
	Q3HBox *hbox7;
	Q3VBox *vbox1;
	Q3VBox *vboxfast;
	Q3VBox *vboxfuzzy;
	QLabel *txt_sigma;
	QLabel *txt_thresh;
	QLabel *txt_m1;
	QLabel *txt_s1;
	QLabel *txt_s2;
	QLabel *txt_extend;
	QSlider *sl_sigma;
	QSlider *sl_thresh;
	QSlider *sl_m1;
	QSlider *sl_s1;
	QSlider *sl_s2;
	QSlider *sl_extend;
	QSpinBox *sb_thresh;
	QSpinBox *sb_m1;
	QSpinBox *sb_s1;
	QSpinBox *sb_s2;
	//	QPushButton *pushexec;
	QRadioButton *rb_fastmarch;
	QRadioButton *rb_fuzzy;
	QRadioButton *rb_drag;
	QRadioButton *rb_slider;
	QButtonGroup *bg_method;
	QButtonGroup *bg_interact;
	void getrange();
	float sigma, thresh, m1, s1, s2;
	float sigmamax;
	float extend, extendmax;
	unsigned area;
	void execute();
	std::vector<Point> vpdyn_arg;

signals:
	void vpdyn_changed(std::vector<Point> *vpdyn_arg);
	void begin_datachange(common::DataSelection &dataSelection,
												QWidget *sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget *sender = NULL,
											common::EndUndoAction undoAction = common::EndUndo);

public slots:
	void slicenr_changed();

private slots:
	void bmphand_changed(bmphandler *bmph);
	void mouse_clicked(Point p);
	void mouse_released(Point p);
	void mouse_moved(Point p);
	void slextend_changed(int i);
	void slextend_pressed();
	void slextend_released();
	void bmp_changed();
	void method_changed();
	void interact_changed();
	void spinbox_changed();
	void slider_changed();
};

#endif