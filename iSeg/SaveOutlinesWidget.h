/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#ifndef SOW_19April05
#define SOW_19April05

#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include <q3hbox.h>
#include <q3vbox.h>
#include <QButtonGroup>
#include <QCheckBox>
#include <qcombobox.h>
#include <QDialog>
#include <QImage>
#include <QLabel>
#include <qlayout.h>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>
#include <QWidget>

class QListWidget;

namespace iseg {

class SaveOutlinesWidget : public QDialog
{
	Q_OBJECT
public:
	SaveOutlinesWidget(SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~SaveOutlinesWidget() override;

private:
	SlicesHandler* m_Handler3D;
	//	char *filename;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	Q3HBox* m_Hbox5;
	Q3HBox* m_Hbox6;
	Q3HBox* m_Hbox7;
	Q3HBox* m_Hbox8;
	Q3HBox* m_Hbox9;
	Q3HBox* m_Hboxslicesbetween;
	Q3VBox* m_Vbox1;
	QListWidget* m_LboTissues;
	QPushButton* m_PbFile;
	QPushButton* m_PbExec;
	QPushButton* m_PbClose;
	QLineEdit* m_LeFile;
	QButtonGroup* m_Filetype;
	QButtonGroup* m_Simplif;
	QRadioButton* m_RbLine;
	QRadioButton* m_RbTriang;
	QRadioButton* m_RbNone;
	QRadioButton* m_RbDougpeuck;
	QRadioButton* m_RbDist;
	QLabel* m_LbSimplif;
	QLabel* m_LbFile;
	QLabel* m_LbDist;
	QLabel* m_LbF1;
	QLabel* m_LbF2;
	QLabel* m_LbMinsize;
	QLabel* m_LbBetween;
	QSpinBox* m_SbDist;
	QSpinBox* m_SbMinsize;
	QSpinBox* m_SbBetween;
	QSlider* m_SlF;
	QSpinBox* m_SbTopextrusion;
	QSpinBox* m_SbBottomextrusion;
	QLabel* m_LbTopextrusion;
	QLabel* m_LbBottomextrusion;
	QCheckBox* m_CbExtrusion;
	QCheckBox* m_CbSmooth;
	QCheckBox* m_CbMarchingcubes;

private slots:
	void ModeChanged();
	void SimplifChanged();
	void FilePushed();
	void SavePushed();
};

} // namespace iseg

#endif
