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
#include "World.h"

#include "Interface/WidgetInterface.h"

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

class VesselWidget : public WidgetInterface
{
	Q_OBJECT
public:
	VesselWidget(SlicesHandler* hand3D, QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~VesselWidget();
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	QSize sizeHint() const override;
	void init() override;
	void newloaded() override;
	std::string GetName() override { return std::string("Vessel"); }
	QIcon GetIcon(QDir picdir) override;
	void cleanup() override;

private:
	void on_slicenr_changed() override;

	void getlabels();
	void reset_branchTree();

	BranchTree branchTree;
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

