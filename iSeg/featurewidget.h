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

#include "Addon/qwidget1.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Core/IFT2.h"

#include <q3listbox.h>
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
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>
//Added by qt3to4:
#include <q3mimefactory.h>
#include <qpixmap.h>

class featurewidget : public QWidget1
{
	Q_OBJECT
public:
	featurewidget(SlicesHandler *hand3D, QWidget *parent = 0,
								const char *name = 0, Qt::WindowFlags wFlags = 0);
	void init();
	void newloaded();
	QSize sizeHint() const;
	std::string GetName() { return std::string("Feature"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("feature.png")).ascii());
	};

private:
	bool selecting;
	std::vector<Point> dynamic;
	bmphandler *bmphand;
	SlicesHandler *handler3D;
	unsigned short activeslice;
	Q3VBox *vbox1;
	Q3VBox *vbox2;
	Q3VBox *vbox3;
	Q3HBox *hbox1;
	QLabel *lb_map;
	QLabel *lb_av;
	QLabel *lb_stddev;
	QLabel *lb_min;
	QLabel *lb_max;
	QLabel *lb_pt;
	QLabel *lb_tissue;
	QLabel *lb_grey;
	QLabel *lb_map_value;
	QLabel *lb_av_value;
	QLabel *lb_stddev_value;
	QLabel *lb_min_value;
	QLabel *lb_max_value;
	QLabel *lb_pt_value;
	QLabel *lb_grey_value;
	QLabel *lb_work_map_value;
	QLabel *lb_work_av_value;
	QLabel *lb_work_stddev_value;
	QLabel *lb_work_min_value;
	QLabel *lb_work_max_value;
	QLabel *lb_work_pt_value;
	QLabel *lb_work_grey_value;
	QLabel *lb_tissuename;
	QLabel *lb_dummy;
	Point pstart;

signals:
	void vpdyn_changed(std::vector<Point> *vpdyn);

public slots:
	void slicenr_changed();

private slots:
	void pt_clicked(Point p);
	void pt_moved(Point p);
	void pt_released(Point p);
};
