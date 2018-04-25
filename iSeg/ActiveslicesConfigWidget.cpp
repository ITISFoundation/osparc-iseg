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

#include "ActiveslicesConfigWidget.h"

#include <q3vbox.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qwidget.h>

using namespace iseg;

ActiveSlicesConfigWidget::ActiveSlicesConfigWidget(SlicesHandler* hand3D, QWidget* parent,
												   const char* name, Qt::WindowFlags wFlags)
	: QDialog(parent, name, TRUE, wFlags), handler3D(hand3D)
{
	vbox1 = new Q3VBox(this);
	hbox1 = new Q3HBox(vbox1);
	vbox2 = new Q3VBox(hbox1);
	vbox3 = new Q3VBox(hbox1);
	hbox2 = new Q3HBox(vbox1);

	lb_start = new QLabel("Start Slice: ", vbox2);
	lb_end = new QLabel("End Slice: ", vbox2);

	sb_start = new QSpinBox(1, (int)handler3D->num_slices(), 1, vbox3);
	sb_start->setValue((int)handler3D->start_slice() + 1);

	sb_end = new QSpinBox(1, (int)handler3D->num_slices(), 1, vbox3);
	sb_end->setValue((int)handler3D->end_slice());

	pb_ok = new QPushButton("OK", hbox2);
	pb_close = new QPushButton("Cancel", hbox2);

	vbox1->show();
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	hbox2->setFixedSize(hbox2->sizeHint());
	vbox2->setFixedSize(vbox2->sizeHint());
	vbox3->setFixedSize(vbox3->sizeHint());
	hbox1->setFixedSize(hbox1->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint());

	QObject::connect(pb_close, SIGNAL(clicked()), this, SLOT(close()));
	QObject::connect(pb_ok, SIGNAL(clicked()), this, SLOT(ok_pressed()));
	QObject::connect(sb_start, SIGNAL(valueChanged(int)), this,
					 SLOT(startslice_changed(int)));
	QObject::connect(sb_end, SIGNAL(valueChanged(int)), this,
					 SLOT(endslice_changed(int)));

	return;
}

void ActiveSlicesConfigWidget::ok_pressed()
{
	if (sb_start->value() > sb_end->value())
		return;

	handler3D->set_startslice(sb_start->value() - 1);
	handler3D->set_endslice(sb_end->value());

	close();
}

void ActiveSlicesConfigWidget::startslice_changed(int startslice1)
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

void ActiveSlicesConfigWidget::endslice_changed(int endslice1)
{
	if (endslice1 < sb_start->value())
	{
		//sb_start->setValue(endslice1);
		pb_ok->setEnabled(false);
	}
	else
	{
		pb_ok->setEnabled(true);
	}
}
