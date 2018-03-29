/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef MEASUREWIDGET_12DEZ07
#define MEASUREWIDGET_12DEZ07

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Plugin/WidgetInterface.h"

#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qwidget.h>

namespace iseg {

class MeasurementWidget : public WidgetInterface
{
	Q_OBJECT
public:
	MeasurementWidget(SlicesHandler* hand3D, QWidget* parent = 0,
					  const char* name = 0, Qt::WindowFlags wFlags = 0);
	~MeasurementWidget();
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	QSize sizeHint() const;
	void init();
	void cleanup();
	void newloaded();
	void getlabels();
	float calculate();
	float calculatevec(unsigned short orient);
	std::string GetName() { return std::string("Measurement"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("measurement.png")).ascii());
	};

private:
	bmphandler* bmphand;
	SlicesHandler* handler3D;
	std::vector<augmentedmark> labels;
	unsigned short activeslice;
	Q3HBox* hboxoverall;
	Q3VBox* vboxmethods;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	Q3VBox* vbox1;
	QLabel* txt_displayer;
	QLabel* txt_ccb1;
	QLabel* txt_ccb2;
	QLabel* txt_ccb3;
	QLabel* txt_ccb4;
	QRadioButton* rb_vector;
	QRadioButton* rb_dist;
	QRadioButton* rb_thick;
	QRadioButton* rb_angle;
	QRadioButton* rb_4ptangle;
	QRadioButton* rb_vol;
	QButtonGroup* modegroup;
	QRadioButton* rb_pts;
	QRadioButton* rb_lbls;
	QButtonGroup* inputgroup;
	QComboBox* cbb_lb1;
	QComboBox* cbb_lb2;
	QComboBox* cbb_lb3;
	QComboBox* cbb_lb4;
	int state;
	int pt[4][3];
	bool drawing;
	std::vector<Point> established;
	std::vector<Point> dynamic;
	Point p1;
	void set_coord(unsigned short posit, Point p, unsigned short slicenr);

signals:
	void vp1_changed(std::vector<Point>* vp1);
	void vpdyn_changed(std::vector<Point>* vpdyn);
	void vp1dyn_changed(std::vector<Point>* vp1, std::vector<Point>* vpdyn,
						bool also_points = false);

private slots:
	void marks_changed();
	void bmphand_changed(bmphandler* bmph);
	void pt_clicked(Point);
	void pt_moved(Point);
	void cbb_changed(int);
	void method_changed(int);
	void inputtype_changed(int);
	void update_visualization();
};

} // namespace iseg

#endif
