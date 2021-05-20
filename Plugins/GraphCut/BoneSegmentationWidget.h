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

#include <q3vbox.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>

namespace itk {
class ProcessObject;
}

class BoneSegmentationWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	BoneSegmentationWidget(iseg::SlicesHandlerInterface* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~BoneSegmentationWidget() override;
	QSize sizeHint() const override;
	void Init() override;
	void NewLoaded() override;
	std::string GetName() override { return std::string("CT Auto-Bone"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("graphcut.png"))); }

private:
	void OnSlicenrChanged() override;

	iseg::SlicesHandlerInterface* m_Handler3D;
	unsigned short m_CurrentSlice;
	Q3VBox* m_VGrid;
	Q3HBox* m_HGrid1;
	Q3HBox* m_HGrid2;
	Q3HBox* m_HGrid3;
	QLabel* m_LabelMaxFlowAlgorithm;
	QLabel* m_LabelStart;
	QLabel* m_LabelEnd;
	QComboBox* m_MaxFlowAlgorithm;
	QPushButton* m_Execute;
	QCheckBox* m_M6Connectivity;
	QCheckBox* m_UseSliceRange;
	QSpinBox* m_Start;
	QSpinBox* m_End;

	itk::ProcessObject* m_CurrentFilter;

private slots:
	void DoWork();
	void Cancel();
	void Showsliders();
};
