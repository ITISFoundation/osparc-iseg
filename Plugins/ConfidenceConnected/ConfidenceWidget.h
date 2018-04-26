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

#include "Data/SlicesHandlerInterface.h"
#include "Interface/WidgetInterface.h"

#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qlineedit.h>

#include <itkIndex.h>

#include <map>

class ConfidenceWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	ConfidenceWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~ConfidenceWidget() {}
	void init() override;
	void newloaded() override;
	void cleanup() override;
	std::string GetName() override { return std::string("Confidence Filter"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("Confidence.png"))); }

protected:
	void on_slicenr_changed() override;
	void on_mouse_clicked(iseg::Point p) override;

	template<typename TInput> 
	void do_work_nd(TInput* source, TInput* target);

	void get_seeds(std::vector<itk::Index<2>>&);
	void get_seeds(std::vector<itk::Index<3>>&);

private:
	iseg::SliceHandlerInterface* handler3D;
	unsigned short activeslice;

	QCheckBox* all_slices;
	QSpinBox* iterations;
	QLineEdit* multiplier;
	QSpinBox* radius;
	QPushButton* clear_seeds;
	QPushButton* execute_button;

	std::map<unsigned, std::vector<iseg::Point>> vpdyn;

private slots:
	void do_work();
	void clearmarks();
};
