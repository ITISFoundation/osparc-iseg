/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SMOOTHWIDGET_3MARCH05
#define SMOOTHWIDGET_3MARCH05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
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

class SmoothingWidget : public WidgetInterface
{
	Q_OBJECT
public:
	SmoothingWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~SmoothingWidget();
	QSize sizeHint() const override;
	void init() override;
	void newloaded() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void hideparams_changed() override;
	std::string GetName() override { return std::string("Smooth"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("smoothing.png"))); }

private:
	void on_slicenr_changed() override;

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
	//	Q3HBox *hbox6;
	Q3VBox* vbox1;
	Q3VBox* vbox2;
	QLabel* txt_n;
	QLabel* txt_sigma1;
	QLabel* txt_sigma2;
	QLabel* txt_dt;
	QLabel* txt_iter;
	QLabel* txt_k;
	QLabel* txt_restrain1;
	QLabel* txt_restrain2;
	QSlider* sl_sigma;
	QSlider* sl_k;
	QSlider* sl_restrain;
	QSpinBox* sb_n;
	QSpinBox* sb_iter;
	QSpinBox* sb_kmax;
	//	QSpinBox *sb_restrainmax;
	QPushButton* pushexec;
	QPushButton* contdiff;
	QRadioButton* rb_gaussian;
	QRadioButton* rb_average;
	QRadioButton* rb_median;
	QRadioButton* rb_sigmafilter;
	QRadioButton* rb_anisodiff;
	QButtonGroup* modegroup;
	QCheckBox* allslices;
	bool dontundo;

private slots:
	void bmphand_changed(bmphandler* bmph);
	void execute();
	void continue_diff();
	void method_changed(int);
	void sigmaslider_changed(int newval);
	void kslider_changed(int newval);
	void n_changed(int newval);
	void kmax_changed(int newval);
	void slider_pressed();
	void slider_released();
};

} // namespace iseg

#endif
