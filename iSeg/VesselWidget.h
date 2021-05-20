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

#include "SlicesHandler.h"
#include "World.h"

#include "Interface/WidgetInterface.h"

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
	VesselWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~VesselWidget() override;
	FILE* SaveParams(FILE* fp, int version) override;
	FILE* LoadParams(FILE* fp, int version) override;
	QSize sizeHint() const override;
	void Init() override;
	void NewLoaded() override;
	std::string GetName() override { return std::string("Vessel"); }
	QIcon GetIcon(QDir picdir) override;
	void Cleanup() override;

private:
	void OnSlicenrChanged() override;
	void MarksChanged() override;

	void Getlabels();
	void ResetBranchTree();

	BranchTree m_BranchTree;
	SlicesHandler* m_Handler3D;
	std::vector<AugmentedMark> m_Labels;
	std::vector<AugmentedMark> m_Selectedlabels;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3VBox* m_Vbox1;
	QLabel* m_TxtStart;
	QLabel* m_TxtNrend;
	QLabel* m_TxtEndnr;
	QLabel* m_TxtEnd;
	QLabel* m_TxtInfo;
	QComboBox* m_CbbLb1;
	QComboBox* m_CbbLb2;
	QSpinBox* m_SbNrend;
	QSpinBox* m_SbEndnr;
	QPushButton* m_PbExec;
	QPushButton* m_PbStore;
	std::vector<Point> m_Vp;

signals:
	void Vp1Changed(std::vector<Point>* vp1);

private slots:
	void NrendChanged(int);
	void EndnrChanged(int);
	void Cbb1Changed(int);
	void Cbb2Changed(int);
	void Execute();
	void Savevessel();
};

} // namespace iseg
