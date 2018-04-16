/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef LWWIDGET_17March05
#define LWWIDGET_17March05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/WidgetInterface.h"

#include "Core/ImageForestingTransform.h"

#include <q3listbox.h>
#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdatetime.h>
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

class LivewireWidget : public WidgetInterface
{
	Q_OBJECT
public:
	LivewireWidget(SlicesHandler* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~LivewireWidget();
	void init() override;
	void newloaded() override;
	void cleanup() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	void hideparams_changed() override;
	QSize sizeHint() const override;
	std::string GetName() override { return std::string("Contour"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("contour.png"))); }

private:
	void on_slicenr_changed() override;

	void on_mouse_clicked(Point p) override;
	void on_mouse_moved(Point p) override;
	void on_mouse_released(Point p) override;

	void init1();

	ImageForestingTransformLivewire* lw;
	ImageForestingTransformLivewire* lwfirst;

	//	bool isactive;
	bool drawing;
	std::vector<Point> clicks;
	std::vector<Point> established;
	std::vector<Point> dynamic;
	std::vector<Point> dynamic1;
	std::vector<Point> dynamicold;
	std::vector<QTime> times;
	int tlimit1;
	int tlimit2;
	bool cooling;
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	unsigned short activeslice;
	Q3HBox* hboxoverall;
	Q3VBox* vboxmethods;
	Q3VBox* vbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	QCheckBox* cb_freezing;
	QCheckBox* cb_closing;
	QSpinBox* sb_freezing;
	QLabel* lb_freezing1;
	QLabel* lb_freezing2;
	QPushButton* pushexec;
	bool straightmode;
	QRadioButton* autotrace;
	QRadioButton* straight;
	QRadioButton* freedraw;
	QButtonGroup* drawmode;
	Point p1, p2;
	std::vector<int> establishedlengths;

signals:
	void vp1_changed(std::vector<Point>* vp1);
	void vp1dyn_changed(std::vector<Point>* vp1, std::vector<Point>* vpdyn);

private slots:
	void bmphand_changed(bmphandler* bmph);
	//	void bmphand_changed(bmphandler *bmph);
	void pt_doubleclicked(Point p);
	void pt_midclicked(Point p);
	void pt_doubleclickedmid(Point p);
	void bmp_changed();
	void mode_changed();
	void freezing_changed();
	void sbfreezing_changed(int i);
};

} // namespace iseg

#endif
