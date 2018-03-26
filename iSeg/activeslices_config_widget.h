/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef ACTIVESLICESCONFIG_13FEB06
#define ACTIVESLICESCONFIG_13FEB06

#include <qpushbutton.h>
#include <q3hbox.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <qwidget.h>
#include <qdialog.h>
#include <qspinbox.h>
#include "SlicesHandler.h"

class ActiveSlicesConfig : public QDialog
{
	Q_OBJECT
public:
	ActiveSlicesConfig(SlicesHandler *hand3D, QWidget *parent=0, const char *name=0, Qt::WindowFlags wFlags=0);

private:
	SlicesHandler *handler3D;
	Q3VBox *vbox1;
	Q3VBox *vbox2;
	Q3VBox *vbox3;
	Q3HBox *hbox1;
	Q3HBox *hbox2;
	QLabel *lb_start;
	QLabel *lb_end;
	QSpinBox *sb_start;
	QSpinBox *sb_end;
	QPushButton *pb_close;
	QPushButton *pb_ok;

private slots:
	void ok_pressed();
	void startslice_changed(int startslice1);
	void endslice_changed(int endslice1);
};

#endif