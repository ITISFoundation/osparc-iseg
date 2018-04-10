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

#include "Plugin/SlicesHandlerInterface.h"
#include "Plugin/WidgetInterface.h"

#include <q3vbox.h>
#include <qcheckbox.h>
#include <qcombobox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>

class TissueSeparatorWidget : public iseg::WidgetInterface
{
	Q_OBJECT
public:
	TissueSeparatorWidget(iseg::SliceHandlerInterface *hand3D,
			QWidget *parent = 0, const char *name = 0,
			Qt::WindowFlags wFlags = 0);
	~TissueSeparatorWidget();
	QSize sizeHint() const override;
	void init();
	void newloaded();
	std::string GetName() { return std::string("Separate Tissue"); }
	QIcon GetIcon(QDir picdir)
	{
		return QIcon(picdir.absFilePath(QString("graphcut.png")).ascii());
	}

private:
	iseg::SliceHandlerInterface *m_Handler3D;
	unsigned short m_CurrentSlice;
	Q3VBox *m_VerticalGrid;
	Q3HBox *m_Horizontal1;
	Q3HBox *m_Horizontal2;
	Q3HBox *m_Horizontal3;
	Q3HBox *m_Horizontal4;
	Q3HBox *m_Horizontal5;
	QLabel *m_LabelForeground;
	QLabel *m_LabelBackground;
	QLabel *m_LabelMaxFlowAlgorithm;
	QLabel *m_LabelStart;
	QLabel *m_LabelEnd;
	QSpinBox *m_ForegroundValue;
	QSpinBox *m_BackgroundValue;
	QCheckBox *USE_FB; // ?
	QCheckBox *m_UseIntensity;
	QComboBox *m_MaxFlowAlgorithm;
	QCheckBox *m_6Connectivity;
	QCheckBox *m_UseSliceRange;
	QSpinBox *m_Start;
	QSpinBox *m_End;
	QPushButton *m_Execute;

public slots:
	void slicenr_changed();

private slots:
	void do_work();
	void showsliders();
};
