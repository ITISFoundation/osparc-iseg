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

#include "MorphologyWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/Point.h"
#include "Data/Logger.h"

#include <QFormLayout>
#include <QProgressDialog>

#include <algorithm>

using namespace iseg;

MorphologyWidget::MorphologyWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"Apply morphological operations to the Target image. "
			"Morphological operations are "
			"based on expanding or shrinking (Dilate/Erode) regions by "
			"a given number of pixel layers (n)."
			"<br>"
			"The functions act on the Target image (to modify a tissue "
			"use Get Tissue and Adder)."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	// methods
	auto modegroup = new QButtonGroup(this);
	modegroup->insert(rb_open = new QRadioButton(QString("Open")));
	modegroup->insert(rb_close = new QRadioButton(QString("Close")));
	modegroup->insert(rb_erode = new QRadioButton(QString("Erode")));
	modegroup->insert(rb_dilate = new QRadioButton(QString("Dilate")));
	rb_open->setChecked(true);

	rb_open->setToolTip(Format(
			"First shrinking before growing is called Open and results in the "
			"deletion of small islands and thin links between structures."));
	rb_close->setToolTip(Format(
			"Growing followed by shrinking results in the "
			"closing of small (< 2n) gaps and holes."));
	rb_erode->setToolTip(Format("Erode or shrink the boundaries of regions of foreground pixels."));
	rb_dilate->setToolTip(Format("Enlarge the boundaries of regions of foreground pixels."));

	// method layout
	auto method_vbox = new QVBoxLayout;
	for (auto child : modegroup->buttons())
		method_vbox->addWidget(child);
	method_vbox->setMargin(5);

	auto method_area = new QFrame(this);
	method_area->setLayout(method_vbox);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);

	// params
	pixel_units = new QCheckBox("Pixel");
	pixel_units->setChecked(true);

	operation_radius = new QLineEdit(QString::number(1));
	operation_radius->setValidator(new QIntValidator(0, 100, operation_radius));
	operation_radius->setToolTip(Format("Radius of operation, either number of pixel layers or radius in length units."));

	node_connectivity = new QCheckBox;
	node_connectivity->setToolTip(Format("Use chess-board (8 neighbors) or city-block (4 neighbors) neighborhood."));

	true_3d = new QCheckBox;
	true_3d->setChecked(true);
	true_3d->setToolTip(Format("Run morphological operations in 3D or per-slice."));

	all_slices = new QCheckBox;
	all_slices->setToolTip(Format("Apply to active slices in 3D or to current slice"));

	execute_button = new QPushButton("Execute");

	auto unit_box = new QHBoxLayout;
	unit_box->addWidget(operation_radius);
	unit_box->addWidget(pixel_units);

	// params layout
	auto params_layout = new QFormLayout;
	params_layout->addRow("Apply to all slices", all_slices);
	params_layout->addRow("Radius", unit_box);
	params_layout->addRow("Full connectivity", node_connectivity);
	params_layout->addRow("True 3d", true_3d);
	params_layout->addRow(execute_button);
	auto params_area = new QWidget(this);
	params_area->setLayout(params_layout);

	// top-level layot
	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addWidget(params_area);
	setLayout(top_layout);

	// init
	all_slices_changed();

	// connect signal-slots
	QObject::connect(pixel_units, SIGNAL(stateChanged(int)), this, SLOT(units_changed()));
	QObject::connect(all_slices, SIGNAL(stateChanged(int)), this, SLOT(all_slices_changed()));
	QObject::connect(execute_button, SIGNAL(clicked()), this, SLOT(execute()));
}

void MorphologyWidget::execute()
{
	bool connect8 = node_connectivity->isChecked();
	bool true3d = true_3d->isChecked();

	iseg::DataSelection dataSelection;
	dataSelection.work = true;

	if (all_slices->isChecked())
	{
		boost::variant<int, float> radius;
		if (pixel_units->isChecked())
		{
			radius = static_cast<int>(operation_radius->text().toFloat());
		}
		else
		{
			radius = operation_radius->text().toFloat();
		}

		dataSelection.allSlices = true;
		emit begin_datachange(dataSelection, this);

		if (rb_open->isOn())
		{
			handler3D->open(radius, true3d);
		}
		else if (rb_close->isOn())
		{
			handler3D->closure(radius, true3d);
		}
		else if (rb_erode->isOn())
		{
			handler3D->erosion(radius, true3d);
		}
		else
		{
			handler3D->dilation(radius, true3d);
		}
	}
	else
	{
		auto radius = static_cast<int>(operation_radius->text().toFloat());

		dataSelection.sliceNr = handler3D->active_slice();
		emit begin_datachange(dataSelection, this);

		if (rb_open->isOn())
		{
			bmphand->open(radius, connect8);
		}
		else if (rb_close->isOn())
		{
			bmphand->closure(radius, connect8);
		}
		else if (rb_erode->isOn())
		{
			bmphand->erosion(radius, connect8);
		}
		else
		{
			bmphand->dilation(radius, connect8);
		}
	}
	emit end_datachange(this);
}

void MorphologyWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void MorphologyWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
}

void MorphologyWidget::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand_changed(handler3D->get_activebmphandler());
	}
	hideparams_changed();
}

void MorphologyWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
}

FILE* MorphologyWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		bool const connect8 = node_connectivity->isChecked();
		int radius = operation_radius->text().toFloat();

		int dummy;
		dummy = static_cast<int>(radius);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(connect8); // rb_8connect->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(!connect8); // rb_4connect->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_open->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_close->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_erode->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_dilate->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(all_slices->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* MorphologyWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		operation_radius->setText(QString::number(dummy));
		fread(&dummy, sizeof(int), 1, fp);
		node_connectivity->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		// skip, is used to set a radio button state
		fread(&dummy, sizeof(int), 1, fp);
		rb_open->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_close->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_erode->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_dilate->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		all_slices->setChecked(dummy > 0);
	}
	return fp;
}

namespace {

int hide_row(QFormLayout* layout, QWidget* widget)
{
	int row = -1;
	if (auto label = layout->labelForField(widget))
	{
		QFormLayout::ItemRole role;
		layout->getWidgetPosition(widget, &row, &role);
		widget->hide();
		label->hide();
		layout->removeWidget(widget);
		layout->removeWidget(label);
	}
	return row;
}

} // namespace

void MorphologyWidget::hideparams_changed()
{
	// \note this is the horrible side of using QFormLayout, show/hide row does not exist
	auto top_layout = dynamic_cast<QFormLayout*>(layout());
	if (top_layout)
	{
		if (hideparams)
		{
			hide_row(top_layout, operation_radius);
			hide_row(top_layout, node_connectivity);
		}
		else if (node_connectivity->isHidden())
		{
			operation_radius->show();
			node_connectivity->show();
			top_layout->insertRow(1, QString("Pixel Layers (n)"), operation_radius);
			top_layout->insertRow(2, QString("Full connectivity"), node_connectivity);
		}
	}
}

void iseg::MorphologyWidget::units_changed()
{
	if (pixel_units->isChecked())
	{
		auto radius = operation_radius->text().toFloat();
		operation_radius->setValidator(new QIntValidator(0, 100, operation_radius));
		operation_radius->setText(QString::number(static_cast<int>(radius)));
	}
	else
	{
		operation_radius->setValidator(new QDoubleValidator(0, 100, 2, operation_radius));
	}
}

void iseg::MorphologyWidget::all_slices_changed()
{
	node_connectivity->setEnabled(!all_slices->isChecked());
	true_3d->setEnabled(all_slices->isChecked());
}
