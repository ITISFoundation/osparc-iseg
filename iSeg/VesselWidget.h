/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef VESSELWIDGET_19DEZ07
#define VESSELWIDGET_19DEZ07

#include "SlicesHandler.h"
#include "world.h"

#include "Addon/qwidget1.h"

#include <q3mimefactory.h>
#include <q3vbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qwidget.h>

namespace iseg {

class vessel_widget : public QWidget1
{
	Q_OBJECT
public:
	vessel_widget(SlicesHandler* hand3D, QWidget* parent = 0,
				  const char* name = 0, Qt::WindowFlags wFlags = 0);
	~vessel_widget();
	FILE* SaveParams(FILE* fp, int version);
	FILE* LoadParams(FILE* fp, int version);
	QSize sizeHint() const;
	void init();
	void newloaded();
	void slicenr_changed();
	std::string GetName() { return std::string("Vessel"); };
	virtual QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("vessel.png")).ascii());
	};
	void clean_up();

private:
	BranchTree branchTree;
	void getlabels();
	void reset_branchTree();
	SlicesHandler* handler3D;
	std::vector<augmentedmark> labels;
	std::vector<augmentedmark> selectedlabels;
	Q3HBox* hbox1;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3VBox* vbox1;
	QLabel* txt_start;
	QLabel* txt_nrend;
	QLabel* txt_endnr;
	QLabel* txt_end;
	QLabel* txt_info;
	QComboBox* cbb_lb1;
	QComboBox* cbb_lb2;
	QSpinBox* sb_nrend;
	QSpinBox* sb_endnr;
	QPushButton* pb_exec;
	QPushButton* pb_store;
	std::vector<Point> vp;

signals:
	void vp1_changed(std::vector<Point>* vp1);

private slots:
	void marks_changed();
	void nrend_changed(int);
	void endnr_changed(int);
	void cbb1_changed(int);
	void cbb2_changed(int);
	void execute();
	void savevessel();
};

} // namespace iseg

#endif
