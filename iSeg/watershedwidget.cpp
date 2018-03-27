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

#include "FormatTooltip.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"
#include "watershedwidget.h"

#include "Core/Point.h"

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>
#include <sstream>

watershed_widget::watershed_widget(SlicesHandler *hand3D, QWidget *parent,
																	 const char *name, Qt::WindowFlags wFlags)
		: QWidget1(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(
			Format("Segment a tissue (2D) by selecting points in the current slice "
						 "based on the Watershed method."
						 "<br>"
						 "The method: the gradient of the slightly smoothed image is "
						 "calculated. High values are interpreted as mountains and "
						 "low values as valleys. Subsequently, a flooding with water is "
						 "simulated resulting in thousands of basins. "
						 "Higher water causes adjacent basins to merge. "));

	activeslice = handler3D->get_activeslice();
	bmphand = handler3D->get_activebmphandler();

	usp = NULL;

	vbox1 = new Q3VBox(this);
	hbox1 = new Q3HBox(vbox1);
	btn_exec = new QPushButton("Execute", vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	txt_h = new QLabel("Flooding height (h): ", hbox1);
	sl_h = new QSlider(Qt::Horizontal, hbox1);
	sl_h->setRange(0, 200);
	sl_h->setValue(160);
	sb_h = new QSpinBox(10, 100, 10, hbox1);
	sb_h->setValue(40);

	sl_h->setFixedWidth(300);
	hbox1->setFixedSize(hbox1->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint());

	sbh_old = sb_h->value();

	QObject::connect(sl_h, SIGNAL(valueChanged(int)), this, SLOT(hsl_changed()));
	QObject::connect(sl_h, SIGNAL(sliderPressed()), this, SLOT(slider_pressed()));
	QObject::connect(sl_h, SIGNAL(sliderReleased()), this,
									 SLOT(slider_released()));
	QObject::connect(sb_h, SIGNAL(valueChanged(int)), this,
									 SLOT(hsb_changed(int)));
	QObject::connect(btn_exec, SIGNAL(clicked()), this, SLOT(execute()));
}

void watershed_widget::hsl_changed()
{
	recalc1();
	emit end_datachange(this, common::NoUndo);
}

void watershed_widget::hsb_changed(int value)
{
	int oldval100 = sbh_old * sl_h->value();
	int sbh_new = value;
	if (oldval100 < (sbh_new * 200))
	{
		sl_h->setValue((int)(oldval100 / sbh_new));
	}
	else
	{
		recalc();
	}
	sbh_old = sbh_new;

	return;
}

void watershed_widget::execute()
{
	if (usp != NULL)
	{
		free(usp);
		usp = NULL;
	}

	usp = bmphand->watershed_sobel(false);

	recalc();
}

void watershed_widget::recalc()
{
	if (usp != NULL)
	{
		common::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->get_activeslice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		recalc1();

		emit end_datachange(this);
	}

	return;
}

void watershed_widget::marks_changed()
{
	//	recalc();

	if (usp != NULL)
	{
		common::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->get_activeslice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		recalc1();

		emit end_datachange(this, common::MergeUndo);
	}

	return;
}

void watershed_widget::recalc1()
{
	if (usp != NULL)
	{
		bmphand->construct_regions(
				(unsigned int)(sb_h->value() * sl_h->value() * 0.005f), usp);
	}

	return;
}

QSize watershed_widget::sizeHint() const { return vbox1->sizeHint(); }

watershed_widget::~watershed_widget()
{
	delete vbox1;

	free(usp);
}

void watershed_widget::slicenr_changed()
{
	//	if(activeslice!=handler3D->get_activeslice()){
	activeslice = handler3D->get_activeslice();
	bmphand_changed(handler3D->get_activebmphandler());
	//	}
}

void watershed_widget::bmphand_changed(bmphandler *bmph)
{
	if (usp != NULL)
	{
		free(usp);
		usp = NULL;
	}

	bmphand = bmph;
	return;
}

void watershed_widget::init()
{
	slicenr_changed();
	hideparams_changed();
}

void watershed_widget::newloaded()
{
	if (usp != NULL)
	{
		free(usp);
		usp = NULL;
	}

	activeslice = handler3D->get_activeslice();
	bmphand = handler3D->get_activebmphandler();
}

void watershed_widget::slider_pressed()
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);
}

void watershed_widget::slider_released() { emit end_datachange(this); }

FILE *watershed_widget::SaveParams(FILE *fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sb_h->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_h->value();
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&sbh_old, 1, sizeof(float), fp);
	}

	return fp;
}

FILE *watershed_widget::LoadParams(FILE *fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(sl_h, SIGNAL(valueChanged(int)), this,
												SLOT(hsl_changed()));
		QObject::disconnect(sb_h, SIGNAL(valueChanged(int)), this,
												SLOT(hsb_changed(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sb_h->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_h->setValue(dummy);

		fread(&sbh_old, sizeof(float), 1, fp);

		QObject::connect(sl_h, SIGNAL(valueChanged(int)), this,
										 SLOT(hsl_changed()));
		QObject::connect(sb_h, SIGNAL(valueChanged(int)), this,
										 SLOT(hsb_changed(int)));
	}
	return fp;
}

void watershed_widget::hideparams_changed()
{
	if (hideparams)
	{
		hbox1->hide();
	}
	else
	{
		hbox1->show();
	}
}