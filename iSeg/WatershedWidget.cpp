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

#include <QFormLayout>
#include <QHBoxLayout>

using namespace iseg;

WatershedWidget::WatershedWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"Segment a tissue (2D) by selecting points in the current slice "
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

	sl_h = new QSlider(Qt::Horizontal, nullptr);
	sl_h->setRange(0, 200);
	sl_h->setValue(160);

	sb_h = new QSpinBox(10, 100, 10, nullptr);
	sb_h->setValue(40);
	sbh_old = sb_h->value();

	btn_exec = new QPushButton("Execute");

	// layout
	auto height_hbox = new QHBoxLayout;
	height_hbox->addWidget(sl_h);
	height_hbox->addWidget(sb_h);

	auto layout = new QFormLayout;
	layout->addRow(tr("Flooding height (h)"), height_hbox);
	layout->addRow(btn_exec);

	setLayout(layout);

	// connections
	connect(sl_h, SIGNAL(valueChanged(int)), this, SLOT(hsl_changed()));
	connect(sl_h, SIGNAL(sliderPressed()), this, SLOT(slider_pressed()));
	connect(sl_h, SIGNAL(sliderReleased()), this, SLOT(slider_released()));
	connect(sb_h, SIGNAL(valueChanged(int)), this, SLOT(hsb_changed(int)));
	connect(btn_exec, SIGNAL(clicked()), this, SLOT(execute()));
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
}

void WatershedWidget::marks_changed()
{
	if (usp != nullptr)
	{
		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		recalc1();

		emit end_datachange(this, iseg::MergeUndo);
	}
}

void WatershedWidget::recalc1()
{
	if (usp != nullptr)
	{
		bmphand->construct_regions(
				(unsigned int)(sb_h->value() * sl_h->value() * 0.005f), usp);
	}
}

WatershedWidget::~WatershedWidget()
{
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
		disconnect(sl_h, SIGNAL(valueChanged(int)), this, SLOT(hsl_changed()));
		disconnect(sb_h, SIGNAL(valueChanged(int)), this, SLOT(hsb_changed(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sb_h->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_h->setValue(dummy);

		fread(&sbh_old, sizeof(float), 1, fp);

		connect(sl_h, SIGNAL(valueChanged(int)), this, SLOT(hsl_changed()));
		connect(sb_h, SIGNAL(valueChanged(int)), this, SLOT(hsb_changed(int)));
	}
	return fp;
}
