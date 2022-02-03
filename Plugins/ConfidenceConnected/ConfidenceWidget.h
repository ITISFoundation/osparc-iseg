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

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>

#include <itkIndex.h>

#include <map>

class ConfidenceWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	ConfidenceWidget(iseg::SlicesHandlerInterface* hand3D);
	~ConfidenceWidget() override = default;
	void Init() override;
	void NewLoaded() override;
	void Cleanup() override;
	std::string GetName() override { return std::string("Confidence Filter"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absoluteFilePath(QString("Confidence.png"))); }

protected:
	void OnSlicenrChanged() override;
	void OnMouseClicked(iseg::Point p) override;

	template<typename TInput>
	void DoWorkNd(TInput* source, TInput* target);

	void GetSeeds(std::vector<itk::Index<2>>&);
	void GetSeeds(std::vector<itk::Index<3>>&);

private:
	iseg::SlicesHandlerInterface* m_Handler3D;
	unsigned short m_Activeslice;

	QCheckBox* m_AllSlices;
	QSpinBox* m_Iterations;
	QLineEdit* m_Multiplier;
	QSpinBox* m_Radius;
	QPushButton* m_ClearSeeds;
	QPushButton* m_ExecuteButton;

	std::map<unsigned, std::vector<iseg::Point>> m_Vpdyn;

private slots:
	void DoWork();
	void Clearmarks();
};
