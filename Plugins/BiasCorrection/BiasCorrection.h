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

#include <vector>

#include "Data/SlicesHandlerInterface.h"
#include "Interface/WidgetInterface.h"

#include <q3vbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>

namespace itk {
class ProcessObject;
}

class BiasCorrectionWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	BiasCorrectionWidget(iseg::SliceHandlerInterface* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~BiasCorrectionWidget();

	QSize sizeHint() const override;
	void init() override;
	void newloaded() override;
	std::string GetName() override;
	QIcon GetIcon(QDir picdir) override;

private:
	void on_slicenr_changed() override;

	template<typename ImagePointer>
	ImagePointer DoBiasCorrection(ImagePointer inputImage, ImagePointer maskImage,
			const std::vector<unsigned int>& numIters,
			int shrinkFactor, double convergenceThreshold);

	iseg::SliceHandlerInterface* handler3D;
	unsigned short activeslice;
	Q3VBox* vbox1;
	QLabel* bias_header;
	QPushButton* bias_exec;
	Q3HBox* hbox2;
	Q3HBox* hbox3;
	Q3HBox* hbox4;
	QLabel* txt_h2;
	QLabel* txt_h3;
	QLabel* txt_h4;
	QSpinBox* edit_num_levels;
	QSpinBox* edit_shrink_factor;
	QSpinBox* edit_num_iterations;

	itk::ProcessObject* m_CurrentFilter;

private slots:
	void do_work();
	void cancel();
};
