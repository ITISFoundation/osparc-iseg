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

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Core/ImageForestingTransform.h"

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

namespace iseg {

class FeatureWidget : public WidgetInterface
{
	Q_OBJECT
public:
	FeatureWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	void init() override;
	void newloaded() override;
	QSize sizeHint() const override;
	std::string GetName() override { return std::string("Feature"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("feature.png")).ascii()); }

private:
	void on_slicenr_changed() override;
	void on_mouse_clicked(Point p) override;
	void on_mouse_moved(Point p) override;
	void on_mouse_released(Point p) override;

	bool selecting;
	std::vector<Point> dynamic;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Q3VBox* vbox1;
	Q3VBox* vbox2;
	Q3VBox* vbox3;
	Q3HBox* hbox1;
	QLabel* lb_map;
	QLabel* lb_av;
	QLabel* lb_stddev;
	QLabel* lb_min;
	QLabel* lb_max;
	QLabel* lb_pt;
	QLabel* lb_tissue;
	QLabel* lb_grey;
	QLabel* lb_map_value;
	QLabel* lb_av_value;
	QLabel* lb_stddev_value;
	QLabel* lb_min_value;
	QLabel* lb_max_value;
	QLabel* lb_pt_value;
	QLabel* lb_grey_value;
	QLabel* lb_work_map_value;
	QLabel* lb_work_av_value;
	QLabel* lb_work_stddev_value;
	QLabel* lb_work_min_value;
	QLabel* lb_work_max_value;
	QLabel* lb_work_pt_value;
	QLabel* lb_work_grey_value;
	QLabel* lb_tissuename;
	QLabel* lb_dummy;
	Point pstart;
};

} // namespace iseg
