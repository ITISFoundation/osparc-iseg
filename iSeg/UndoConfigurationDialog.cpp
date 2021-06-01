/*
 * Copyright (c) 2021 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "UndoConfigurationDialog.h"

#include "Interface/QtConnect.h"

#include <QFormLayout>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

namespace iseg {

UndoConfigurationDialog::UndoConfigurationDialog(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: QDialog(parent, name, TRUE, wFlags), m_Handler3D(hand3D)
{
	auto layout = new QFormLayout;

	m_CbUndo3D = new QCheckBox;
	m_CbUndo3D->setChecked(m_Handler3D->ReturnUndo3D());

	m_SbNrundo = new QSpinBox(1, 100, 1, nullptr);
	m_SbNrundo->setValue(m_Handler3D->GetNumberOfUndoSteps());
	m_SbNrundoarrays = new QSpinBox(6, 10000, 1, nullptr);
	m_SbNrundoarrays->setValue(m_Handler3D->GetNumberOfUndoArrays());

	m_PbClose = new QPushButton("Accept");

	// layout
	layout->addRow(tr("Enable 3D Undo"), m_CbUndo3D);
	layout->addRow(tr("Maximal nr of undo steps"), m_SbNrundo);
	layout->addRow(tr("Maximal nr of stored images"), m_SbNrundoarrays);
	layout->addRow(m_PbClose);

	setLayout(layout);

	// connections
	QObject_connect(m_PbClose, SIGNAL(clicked()), this, SLOT(OkPressed()));
}

UndoConfigurationDialog::~UndoConfigurationDialog()
= default;

void UndoConfigurationDialog::OkPressed()
{
	m_Handler3D->SetUndo3D(m_CbUndo3D->isChecked());
	m_Handler3D->SetNumberOfUndoArrays((unsigned)m_SbNrundoarrays->value());
	m_Handler3D->SetNumberOfUndoSteps((unsigned)m_SbNrundo->value());

	close();
}

} // namespace iseg
