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

#include "config.h"

#include "SlicesHandler.h"

#include "vtkMyGDCMPolyDataReader.h"

#include <q3hbox.h>
#include <q3vbox.h>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

#include <vector>

namespace iseg {

class RadiotherapyStructureSetImporter : public QDialog
{
	Q_OBJECT
public:
	RadiotherapyStructureSetImporter(QString filename, SlicesHandler* hand3D, QWidget* parent = nullptr, const char* name = nullptr, Qt::WindowFlags wFlags = Qt::Widget);
	~RadiotherapyStructureSetImporter() override;

private:
	void Storeparams();
	void Updatevisibility();

	SlicesHandler* m_Handler3D;
	Q3HBox* m_Hbox1;
	Q3HBox* m_Hbox2;
	Q3HBox* m_Hbox3;
	Q3HBox* m_Hbox4;
	Q3VBox* m_Vbox1;
	QSpinBox* m_SbPriority;
	QComboBox* m_CbSolids;
	QComboBox* m_CbNames;
	QLineEdit* m_LeName;
	QLabel* m_LbPriority;
	QLabel* m_LbNamele;
	QLabel* m_LbNamecb;
	QCheckBox* m_CbNew;
	QCheckBox* m_CbIgnore;
	QPushButton* m_PbCancel;
	QPushButton* m_PbOk;
	gdcmvtk_rtstruct::tissuevec m_Tissues;
	std::vector<bool> m_Vecnew;
	std::vector<bool> m_Vecignore;
	std::vector<tissues_size_t> m_Vectissuenrs;
	std::vector<int> m_Vecpriorities;
	std::vector<std::string> m_Vectissuenames;
	int m_Currentitem;

signals:
	void BeginDatachange(DataSelection& dataSelection, QWidget* sender = nullptr, bool beginUndo = true);
	void EndDatachange(QWidget* sender = nullptr, eEndUndoAction undoAction = iseg::EndUndo);

private slots:
	void SolidChanged(int);
	void NewChanged();
	void IgnoreChanged();
	void OkPressed();
	void ShowPriorityInfo();
};

} // namespace iseg
