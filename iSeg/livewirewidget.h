/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef LWWIDGET_17March05
#define LWWIDGET_17March05

#include <algorithm>
#include <q3listbox.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdatetime.h>
#include <qdialog.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>
//Added by qt3to4:
#include "Addon/qwidget1.h"
#include "Core/IFT2.h"
#include "Core/common.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"
#include <q3mimefactory.h>
#include <qpixmap.h>

class livewire_widget : public QWidget1
{
	Q_OBJECT
public:
	livewire_widget(SlicesHandler *hand3D, QWidget *parent = 0,
									const char *name = 0, Qt::WindowFlags wFlags = 0);
	~livewire_widget();
	void init();
	void newloaded();
	void cleanup();
	FILE *SaveParams(FILE *fp, int version);
	FILE *LoadParams(FILE *fp, int version);
	void hideparams_changed();
	QSize sizeHint() const;
	IFT_livewire *lw;
	IFT_livewire *lwfirst;
	std::string GetName() { return std::string("Contour"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("contour.png")).ascii());
	};

private:
	void init1();
	//	bool isactive;
	bool drawing;
	std::vector<Point> clicks;
	std::vector<Point> established;
	std::vector<Point> dynamic;
	std::vector<Point> dynamic1;
	std::vector<Point> dynamicold;
	std::vector<QTime> times;
	int tlimit1;
	int tlimit2;
	bool cooling;
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	Q3HBox *hboxoverall;
	Q3VBox *vboxmethods;
	Q3VBox *vbox1;
	Q3HBox *hbox2;
	Q3HBox *hbox3;
	QCheckBox *cb_freezing;
	QCheckBox *cb_closing;
	QSpinBox *sb_freezing;
	QLabel *lb_freezing1;
	QLabel *lb_freezing2;
	QPushButton *pushexec;
	bool straightmode;
	QRadioButton *autotrace;
	QRadioButton *straight;
	QRadioButton *freedraw;
	QButtonGroup *drawmode;
	Point p1, p2;
	std::vector<int> establishedlengths;

signals:
	void vp1_changed(std::vector<Point> *vp1);
	void vpdyn_changed(std::vector<Point> *vpdyn);
	void vp1dyn_changed(std::vector<Point> *vp1, std::vector<Point> *vpdyn);
	void begin_datachange(common::DataSelection &dataSelection,
												QWidget *sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget *sender = NULL,
											common::EndUndoAction undoAction = common::EndUndo);

public slots:
	void slicenr_changed();

private slots:
	void bmphand_changed(bmphandler *bmph);
	//	void bmphand_changed(bmphandler *bmph);
	void pt_clicked(Point p);
	void pt_doubleclicked(Point p);
	void pt_midclicked(Point p);
	void pt_doubleclickedmid(Point p);
	void pt_moved(Point p);
	void pt_released(Point p);
	void bmp_changed();
	void mode_changed();
	void freezing_changed();
	void sbfreezing_changed(int i);
};

#endif