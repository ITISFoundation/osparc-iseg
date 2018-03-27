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

#include "Addon/SlicesHandlerInterface.h"
#include "Addon/qwidget1.h"

#include <q3vbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>

class CConfidence : public QWidget1
{
	Q_OBJECT
public:
	CConfidence(iseg::CSliceHandlerInterface *hand3D, QWidget *parent = 0,
							const char *name = 0, Qt::WindowFlags wFlags = 0);
	~CConfidence();
	QSize sizeHint() const;
	void init();
	void newloaded();
	std::string GetName() { return std::string("ConfidenceFilter"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("Confidence.png")).ascii());
	};

private:
	unsigned int *usp;
	iseg::CSliceHandlerInterface *handler3D;
	unsigned short activeslice;
	Q3VBox *vbox1;
	QLabel *bias_header;
	QPushButton *bias_exec;
	Q3HBox *hbox2;
	Q3HBox *hbox3;
	Q3HBox *hbox4;
	Q3HBox *hbox5;
	QLabel *txt_h2;
	QLabel *txt_h3;
	QLabel *txt_h4;
	QLabel *txt_h5;
	QSpinBox *sl_h2;
	QSpinBox *sl_h3;
	QSpinBox *sl_h4;
	QSpinBox *sl_h5;
	QSpinBox *sl_h6;
	QSpinBox *sl_h7;

public slots:
	void slicenr_changed();

private slots:
	void do_work();
};
