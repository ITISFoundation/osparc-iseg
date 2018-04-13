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

#include "Interface/SlicesHandlerInterface.h"
#include "Interface/WidgetInterface.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

class TissueSeparatorWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	TissueSeparatorWidget(iseg::SliceHandlerInterface* hand3D,
			QWidget* parent = 0, const char* name = 0,
			Qt::WindowFlags wFlags = 0);
	~TissueSeparatorWidget() {}
	void init() override;
	void newloaded() override;
	void cleanup() override;
	std::string GetName() override { return std::string("Separate Tissue"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("graphcut.png"))); }

private:
	void on_tissuenr_changed(int i) override;
	void on_slicenr_changed() override;

	void on_mouse_clicked(iseg::Point p) override;
	void on_mouse_released(iseg::Point p) override;
	void on_mouse_moved(iseg::Point p) override;

private slots:
	void do_work();
	void clearmarks();

private:
	iseg::SliceHandlerInterface* slice_handler;
	unsigned short current_slice;
	unsigned tissuenr;
	iseg::Point last_pt;
	std::vector<iseg::Point> vpdyn;
	std::vector<iseg::Mark> vm;

	//QWidget* vertical_grid;
	QCheckBox* all_slices;
	QCheckBox* use_source;
	QLineEdit* sigma_edit;
	QPushButton* clear_lines;
	QPushButton* execute_button;
};
