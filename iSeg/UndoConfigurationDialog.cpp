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

#include <q3vbox.h>
#include <qcheckbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qwidget.h>

using namespace iseg;

UndoConfigurationDialog::UndoConfigurationDialog(SlicesHandler* hand3D, QWidget* parent, const char* name,
												 Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	vbox1 = new Q3VBox(this);

	cb_undo3D = new QCheckBox(QString("Enable 3D undo"), vbox1);
	cb_undo3D->setChecked(handler3D->return_undo3D());

	hbox1 = new Q3HBox(vbox1);
	//	hbox2= new QHBox(vbox1);
	vbox2 = new Q3VBox(hbox1);
	vbox3 = new Q3VBox(hbox1);

	lb_nrundo = new QLabel("Maximal nr of undo steps: ", vbox2);
	sb_nrundo = new QSpinBox(1, 100, 1, vbox3);
	sb_nrundo->setValue(handler3D->GetNumberOfUndoSteps());
	lb_nrundoarrays = new QLabel("Maximal nr of stored images: ", vbox2);
	sb_nrundoarrays = new QSpinBox(6, 10000, 1, vbox3);
	sb_nrundoarrays->setValue(handler3D->GetNumberOfUndoArrays());

	pb_close = new QPushButton("Accept", vbox1);

	vbox1->show();
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	vbox2->setFixedSize(vbox2->sizeHint());
	vbox3->setFixedSize(vbox3->sizeHint());
	hbox1->setFixedSize(hbox1->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(pb_close, SIGNAL(clicked()), this, SLOT(ok_pressed()));

	return;
}

UndoConfigurationDialog::~UndoConfigurationDialog() { delete vbox1; }

void UndoConfigurationDialog::ok_pressed()
{
	handler3D->set_undo3D(cb_undo3D->isChecked());
	handler3D->SetNumberOfUndoArrays((unsigned)sb_nrundoarrays->value());
	handler3D->SetNumberOfUndoSteps((unsigned)sb_nrundo->value());

	close();
}
