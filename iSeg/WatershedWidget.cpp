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

#include "SlicesHandler.h"
#include "WatershedWidget.h"
#include "bmp_read_1.h"

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

using namespace iseg;

WatershedWidget::WatershedWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
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

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	usp = nullptr;

	vbox1 = new Q3VBox(this);
	vbox1->setMargin(8);
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

	QObject::connect(sl_h, SIGNAL(valueChanged(int)), this,
			SLOT(hsl_changed()));
	QObject::connect(sl_h, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_h, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
	QObject::connect(sb_h, SIGNAL(valueChanged(int)), this,
			SLOT(hsb_changed(int)));
	QObject::connect(btn_exec, SIGNAL(clicked()), this, SLOT(execute()));
}

void WatershedWidget::hsl_changed()
{
	recalc1();
	emit end_datachange(this, iseg::NoUndo);
}

void WatershedWidget::hsb_changed(int value)
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

void WatershedWidget::execute()
{
	if (usp != nullptr)
	{
		free(usp);
		usp = nullptr;
	}

	usp = bmphand->watershed_sobel(false);

	recalc();
}

void WatershedWidget::recalc()
{
	if (usp != nullptr)
	{
		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		recalc1();

		emit end_datachange(this);
	}

	return;
}

void WatershedWidget::marks_changed()
{
	//	recalc();

	if (usp != nullptr)
	{
		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		recalc1();

		emit end_datachange(this, iseg::MergeUndo);
	}

	return;
}

void WatershedWidget::recalc1()
{
	if (usp != nullptr)
	{
		bmphand->construct_regions(
				(unsigned int)(sb_h->value() * sl_h->value() * 0.005f), usp);
	}

	return;
}

QSize WatershedWidget::sizeHint() const { return vbox1->sizeHint(); }

WatershedWidget::~WatershedWidget()
{
	delete vbox1;

	free(usp);
}

void WatershedWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void WatershedWidget::bmphand_changed(bmphandler* bmph)
{
	if (usp != nullptr)
	{
		free(usp);
		usp = nullptr;
	}

	bmphand = bmph;
	return;
}

void WatershedWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void WatershedWidget::newloaded()
{
	if (usp != nullptr)
	{
		free(usp);
		usp = nullptr;
	}

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
}

void WatershedWidget::slider_pressed()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);
}

void WatershedWidget::slider_released() { emit end_datachange(this); }

FILE* WatershedWidget::SaveParams(FILE* fp, int version)
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

FILE* WatershedWidget::LoadParams(FILE* fp, int version)
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

void WatershedWidget::hideparams_changed()
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
