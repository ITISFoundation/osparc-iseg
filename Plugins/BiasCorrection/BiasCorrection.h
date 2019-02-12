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

#include <qpushbutton.h>
#include <qspinbox.h>

#include <vector>

namespace itk {
class ProcessObject;
}

class BiasCorrectionWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	BiasCorrectionWidget(iseg::SlicesHandlerInterface* hand3D, QWidget* parent = 0,
			const char* name = 0, Qt::WindowFlags wFlags = 0);
	~BiasCorrectionWidget();

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

	iseg::SlicesHandlerInterface* handler3D;
	unsigned short activeslice;

	QSpinBox* number_levels;
	QSpinBox* shrink_factor;
	QSpinBox* number_iterations;
	QPushButton* execute;

	itk::ProcessObject* m_CurrentFilter;

private slots:
	void do_work();
	void cancel();
};
