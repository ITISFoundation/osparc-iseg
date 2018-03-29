/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef WATERSHED_8MARCH05
#define WATERSHED_8MARCH05

#include "Addon/qwidget1.h"

#include "Core/DataSelection.h"

#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
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

class SlicesHandler;
class bmphandler;

class watershed_widget : public QWidget1
{
	Q_OBJECT
public:
	watershed_widget(SlicesHandler* hand3D, QWidget* parent = 0,
					 const char* name = 0, Qt::WindowFlags wFlags = 0);
	~watershed_widget();
	QSize sizeHint() const;
	void init();
	void newloaded();
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	void hideparams_changed();
	std::string GetName() { return std::string("Watershed"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("watershed.png")).ascii());
	};

private:
	unsigned int* usp;
	int sbh_old;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3VBox* vbox1;
	QLabel* txt_h;
	QSpinBox* sb_h;
	QSlider* sl_h;
	QPushButton* btn_exec;

	void recalc1();

signals:
	void begin_datachange(iseg::DataSelection& dataSelection,
						  QWidget* sender = NULL, bool beginUndo = true);
	void end_datachange(QWidget* sender = NULL,
						iseg::EndUndoAction undoAction = iseg::EndUndo);

public slots:
	void slicenr_changed();
	void marks_changed();

private slots:
	void bmphand_changed(bmphandler* bmph);
	void hsl_changed();
	void slider_pressed();
	void slider_released();
	void hsb_changed(int value);
	void execute();
	void recalc();
};

} // namespace iseg

#endif
