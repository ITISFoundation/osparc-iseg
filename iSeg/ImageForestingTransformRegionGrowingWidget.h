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

#include <q3hbox.h>
#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

class ImageForestingTransformRegionGrowingWidget : public WidgetInterface
{
	Q_OBJECT
public:
	ImageForestingTransformRegionGrowingWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~ImageForestingTransformRegionGrowingWidget();
	void init() override;
	void newloaded() override;
	void cleanup() override;
	QSize sizeHint() const override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("IFT"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("iftrg.png"))); }

protected:
	void on_tissuenr_changed(int i) override;
	void on_slicenr_changed() override;

	void on_mouse_clicked(Point p) override;
	void on_mouse_released(Point p) override;
	void on_mouse_moved(Point p) override;

private:
	void init1();
	void removemarks(Point p);
	void getrange();

	float* lbmap;
	ImageForestingTransformRegionGrowing* IFTrg;
	Point last_pt;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Q3HBox* hbox1;
	Q3VBox* vbox1;
	QLabel* txt_lower;
	QLabel* txt_upper;
	QSlider* sl_thresh;
	QPushButton* pushexec;
	QPushButton* pushclear;
	QPushButton* pushremove;
	unsigned tissuenr;
	float thresh;
	float maxthresh;
	std::vector<Mark> vm;
	std::vector<Mark> vmempty;
	std::vector<Point> vmdyn;
	unsigned area;

private slots:
	void bmphand_changed(bmphandler* bmph);
	void execute();
	void clearmarks();
	void slider_changed(int i);
	void slider_pressed();
	void slider_released();
	void bmp_changed();
};

} // namespace iseg
