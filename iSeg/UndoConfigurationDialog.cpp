/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#include "Precompiled.h"

#include "UndoConfigurationDialog.h"

#include <QFormLayout>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qwidget.h>

using namespace iseg;

UndoConfigurationDialog::UndoConfigurationDialog(SlicesHandler* hand3D, QWidget* parent, const char* name,
												 Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	auto layout = new QFormLayout;

	cb_undo3D = new QCheckBox;
	cb_undo3D->setChecked(handler3D->return_undo3D());

	sb_nrundo = new QSpinBox(1, 100, 1, nullptr);
	sb_nrundo->setValue(handler3D->GetNumberOfUndoSteps());
	sb_nrundoarrays = new QSpinBox(6, 10000, 1, nullptr);
	sb_nrundoarrays->setValue(handler3D->GetNumberOfUndoArrays());

	pb_close = new QPushButton("Accept");

	// layout
	layout->addRow(tr("Enable 3D Undo"), cb_undo3D);
	layout->addRow(tr("Maximal nr of undo steps"), sb_nrundo);
	layout->addRow(tr("Maximal nr of stored images"), sb_nrundoarrays);
	layout->addRow(pb_close);

	setLayout(layout);

	// connections
	QObject::connect(pb_close, SIGNAL(clicked()), this, SLOT(ok_pressed()));
}

UndoConfigurationDialog::~UndoConfigurationDialog() 
{
	
}

void UndoConfigurationDialog::ok_pressed()
{
	handler3D->set_undo3D(cb_undo3D->isChecked());
	handler3D->SetNumberOfUndoArrays((unsigned)sb_nrundoarrays->value());
	handler3D->SetNumberOfUndoSteps((unsigned)sb_nrundo->value());

	close();
}
