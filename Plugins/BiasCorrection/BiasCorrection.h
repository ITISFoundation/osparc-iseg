/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
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
	BiasCorrectionWidget(iseg::SlicesHandlerInterface* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~BiasCorrectionWidget() override;

	void Init() override;
	void NewLoaded() override;
	std::string GetName() override;
	QIcon GetIcon(QDir picdir) override;

private:
	void OnSlicenrChanged() override;

	template<typename ImagePointer>
	ImagePointer DoBiasCorrection(ImagePointer inputImage, ImagePointer maskImage, const std::vector<unsigned int>& numIters, int shrinkFactor, double convergenceThreshold);

	iseg::SlicesHandlerInterface* m_Handler3D;
	unsigned short m_Activeslice;

	QSpinBox* m_NumberLevels;
	QSpinBox* m_ShrinkFactor;
	QSpinBox* m_NumberIterations;
	QPushButton* m_Execute;

	itk::ProcessObject* m_CurrentFilter;

private slots:
	void DoWork();
	void Cancel();
};
