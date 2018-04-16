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

#include <q3hbox.h>
#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

namespace iseg {

class ImageForestingTransformAdaptFuzzy;
class ImageForestingTransformFastMarching;
class SlicesHandler;
class bmphandler;

class FastmarchingFuzzyWidget : public WidgetInterface
{
	Q_OBJECT
public:
	FastmarchingFuzzyWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~FastmarchingFuzzyWidget();
	void init() override;
	void newloaded() override;
	void cleanup() override;
	QSize sizeHint() const override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("Fuzzy"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("fuzzy.png"))); }

protected:
	void on_slicenr_changed() override;
	void on_mouse_clicked(Point p) override;
	void on_mouse_released(Point p) override;
	void on_mouse_moved(Point p) override;

private:
	float* map;
	ImageForestingTransformFastMarching* IFTmarch;
	ImageForestingTransformAdaptFuzzy* IFTfuzzy;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Q3HBox* hboxoverall;
	Q3VBox* vboxmethods;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3HBox* hbox5;
	Q3HBox* hbox7;
	Q3VBox* vbox1;
	Q3VBox* vboxfast;
	Q3VBox* vboxfuzzy;
	QLabel* txt_sigma;
	QLabel* txt_thresh;
	QLabel* txt_m1;
	QLabel* txt_s1;
	QLabel* txt_s2;
	QLabel* txt_extend;
	QSlider* sl_sigma;
	QSlider* sl_thresh;
	QSlider* sl_m1;
	QSlider* sl_s1;
	QSlider* sl_s2;
	QSlider* sl_extend;
	QSpinBox* sb_thresh;
	QSpinBox* sb_m1;
	QSpinBox* sb_s1;
	QSpinBox* sb_s2;
	//	QPushButton *pushexec;
	QRadioButton* rb_fastmarch;
	QRadioButton* rb_fuzzy;
	QRadioButton* rb_drag;
	QRadioButton* rb_slider;
	QButtonGroup* bg_method;
	QButtonGroup* bg_interact;
	void getrange();
	float sigma, thresh, m1, s1, s2;
	float sigmamax;
	float extend, extendmax;
	unsigned area;
	void execute();
	std::vector<Point> vpdyn_arg;

private slots:
	void bmphand_changed(bmphandler* bmph);
	void slextend_changed(int i);
	void slextend_pressed();
	void slextend_released();
	void bmp_changed();
	void method_changed();
	void interact_changed();
	void spinbox_changed();
	void slider_changed();
};

} // namespace iseg
