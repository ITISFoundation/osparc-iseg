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

#include "Interface/WidgetInterface.h"

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

class WatershedWidget : public WidgetInterface
{
	Q_OBJECT
public:
	WatershedWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~WatershedWidget();
	QSize sizeHint() const override;
	void init() override;
	void newloaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("Watershed"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("watershed.png"))); }

private:
	void on_slicenr_changed() override;

	void recalc1();

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

public slots:
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
