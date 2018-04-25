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
	BoneSegmentationWidget(iseg::SliceHandlerInterface* hand3D,
			QWidget* parent = 0, const char* name = 0, Qt::WindowFlags wFlags = 0);
	~BoneSegmentationWidget();
	QSize sizeHint() const override;
	void init() override;
	void newloaded() override;
	std::string GetName() override { return std::string("CT Auto-Bone"); }
	QIcon GetIcon(QDir picdir) override { return QIcon(picdir.absFilePath(QString("graphcut.png"))); }

private:
	void on_slicenr_changed() override;

	iseg::SliceHandlerInterface* m_Handler3D;
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
	QCheckBox* m_6Connectivity;
	QCheckBox* m_UseSliceRange;
	QSpinBox* m_Start;
	QSpinBox* m_End;

	itk::ProcessObject* m_CurrentFilter;

private slots:
	void do_work();
	void cancel();
	void showsliders();
};
