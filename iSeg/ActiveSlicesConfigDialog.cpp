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

#include "ActiveSlicesConfigDialog.h"

#include "SlicesHandler.h"

#include <QFormLayout>

using namespace iseg;

ActiveSlicesConfigDialog::ActiveSlicesConfigDialog(SlicesHandler* hand3D, QWidget* parent,
												   const char* name, Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	sb_start = new QSpinBox(1, (int)handler3D->num_slices(), 1, nullptr);
	sb_start->setValue((int)handler3D->start_slice() + 1);

	sb_end = new QSpinBox(1, (int)handler3D->num_slices(), 1, nullptr);
	sb_end->setValue((int)handler3D->end_slice());

	pb_ok = new QPushButton("OK");
	pb_close = new QPushButton("Cancel");

	// layout
	auto layout = new QFormLayout;
	layout->addRow(tr("Start slice"), sb_start);
	layout->addRow(tr("End slice"), sb_end);
	layout->addRow(pb_ok, pb_close);

	setLayout(layout);

	// connections
	connect(pb_close, SIGNAL(clicked()), this, SLOT(close()));
	connect(pb_ok, SIGNAL(clicked()), this, SLOT(ok_pressed()));
	connect(sb_start, SIGNAL(valueChanged(int)), this, SLOT(startslice_changed(int)));
	connect(sb_end, SIGNAL(valueChanged(int)), this, SLOT(endslice_changed(int)));
}

void ActiveSlicesConfigDialog::ok_pressed()
{
	if (sb_start->value() > sb_end->value())
		return;

	handler3D->set_startslice(sb_start->value() - 1);
	handler3D->set_endslice(sb_end->value());

	close();
}

void ActiveSlicesConfigDialog::startslice_changed(int startslice1)
{
	if (startslice1 > sb_end->value())
	{
		//sb_end->setValue(startslice1);
		pb_ok->setEnabled(false);
	}
	else
	{
		pb_ok->setEnabled(true);
	}
}

void ActiveSlicesConfigDialog::endslice_changed(int endslice1)
{
	if (endslice1 < sb_start->value())
	{
		pb_ok->setEnabled(false);
	}
	else
	{
		pb_ok->setEnabled(true);
	}
}
