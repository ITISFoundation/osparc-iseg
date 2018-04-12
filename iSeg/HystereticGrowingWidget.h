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

#include <q3listbox.h>
#include <q3mimefactory.h>
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
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

class HystereticGrowingWidget : public WidgetInterface
{
	Q_OBJECT
public:
	HystereticGrowingWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~HystereticGrowingWidget() {}
	void init() override;
	void newloaded() override;
	void cleanup() override;
	QSize sizeHint() const override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("Growing"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("growing.png"))); }

private:
	void on_slicenr_changed() override;
	void on_mouse_clicked(Point p) override;
	void on_mouse_moved(Point p) override;
	void on_mouse_released(Point p) override;

	void init1();
	Point p1;
	Point last_pt;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	float upper_limit;
	float lower_limit;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox2a;
	Q3HBox* hbox3;
	Q3VBox* vbox1;
	Q3VBox* vbox2;
	Q3VBox* vbox3;
	Q3VBox* vbox4;
	Q3VBox* vbox5;
	Q3VBox* vbox6;
	Q3VBox* vbox7;
	QLabel* txt_lower;
	QLabel* txt_upper;
	QLabel* txt_lowerhyster;
	QLabel* txt_upperhyster;
	QSlider* sl_lower;
	QSlider* sl_upper;
	QSlider* sl_lowerhyster;
	QSlider* sl_upperhyster;
	QPushButton* pushexec;
	QPushButton* drawlimit;
	QPushButton* clearlimit;
	bool limitdrawing;
	QCheckBox* autoseed;
	QCheckBox* allslices;
	void getrange();
	void getrange_sub(float ll, float uu, float ul, float lu);
	std::vector<Point> vp1;
	std::vector<Point> vpdyn;
	void execute1();
	bool dontundo;
	QLineEdit* le_bordervall;
	QLineEdit* le_bordervalu;
	QLineEdit* le_bordervallh;
	QLineEdit* le_bordervaluh;
	QPushButton* pb_saveborders;
	QPushButton* pb_loadborders;

signals:
	void vp1_changed(std::vector<Point>* vp1_arg);
	void vp1dyn_changed(std::vector<Point>* vp1_arg,
			std::vector<Point>* vpdyn_arg);

private slots:
	void bmphand_changed(bmphandler* bmph);
	void auto_toggled();
	void update_visible();
	void execute();
	void limitpressed();
	void clearpressed();
	void slider_changed();
	void bmp_changed();
	void slider_pressed();
	void slider_released();
	void saveborders_execute();
	void loadborders_execute();
	void le_bordervall_returnpressed();
	void le_bordervalu_returnpressed();
	void le_bordervallh_returnpressed();
	void le_bordervaluh_returnpressed();
};

} // namespace iseg
