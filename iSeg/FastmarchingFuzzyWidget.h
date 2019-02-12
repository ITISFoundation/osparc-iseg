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

#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>

class QStackedLayout;

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

	QStackedLayout* params_stack_layout;
	QSlider* sl_sigma;
	QSlider* sl_thresh;
	QSpinBox* sb_thresh;
	QSlider* sl_m1;
	QSpinBox* sb_m1;
	QSlider* sl_s1;
	QSpinBox* sb_s1;
	QSlider* sl_s2;
	QSpinBox* sb_s2;
	QSlider* sl_extend;
	QRadioButton* rb_fastmarch;
	QRadioButton* rb_fuzzy;
	QRadioButton* rb_drag;
	QRadioButton* rb_slider;

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
