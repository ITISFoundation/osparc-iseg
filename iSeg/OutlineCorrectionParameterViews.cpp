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

#include "OutlineCorrectionParameterViews.h"

#include "Interface/FormatTooltip.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QScrollArea>
#include <QStackedLayout>

#include <initializer_list>

namespace iseg {

QHBoxLayout* make_hbox(std::initializer_list<QWidget*> list)
{
	auto hbox = new QHBoxLayout;
	for (auto w : list)
	{
		hbox->addWidget(w);
	}
	return hbox;
}

QButtonGroup* make_button_group(QWidget* parent, std::initializer_list<QRadioButton*> list)
{
	auto group = new QButtonGroup(parent);
	for (auto w : list)
	{
		group->addButton(w);
	}
	return group;
}

OLCorrParamView::OLCorrParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	_target = new QRadioButton(QString("Target"));
	_tissues = new QRadioButton(QString("Tissues"));
	_target->setOn(true);
	auto input_group = make_button_group(this, {_target, _tissues});

	_select_object = new QPushButton(tr("Select"));
	_select_object->setCheckable(true);
	_object_value = new QLineEdit(QString::number(255));
	_object_value->setValidator(new QDoubleValidator);

	auto input_hbox = make_hbox({_target, _tissues});
	auto object_hbox = make_hbox({_select_object, _object_value});

	auto layout = new QFormLayout;
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Object value"), object_hbox);
	setLayout(layout);
}

void OLCorrParamView::set_object_value(float v)
{
	_object_value->setText(QString::number(v));
	_select_object->setChecked(false);
}

BrushParamView::BrushParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	// parameter fields
	_target = new QRadioButton(QString("Target"));
	_tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {_target, _tissues});
	_target->setOn(true);

	_select_object = new QPushButton(tr("Select"));
	_select_object->setCheckable(true);
	_object_value = new QLineEdit(QString::number(255));
	_object_value->setValidator(new QDoubleValidator);

	_modify = new QRadioButton(QString("Modify"));
	_draw = new QRadioButton(QString("Draw"));
	_erase = new QRadioButton(QString("Erase"));
	auto mode_group = make_button_group(this, {_modify, _draw, _erase});
	_modify->setOn(true);

	_radius = new QLineEdit(QString::number(1));
	_unit_pixel = new QRadioButton(tr("Pixel"));
	_unit_mm = new QRadioButton(tr("Use spacing"));
	auto unit_group = make_button_group(this, {_unit_pixel, _unit_mm});
	_unit_pixel->setOn(true);

	_show_guide = new QCheckBox;
	_show_guide->setChecked(false);
	_guide_offset = new QSpinBox(-100, 100, 1, nullptr);
	_guide_offset->setValue(1);
	_copy_guide = new QPushButton(QString("Copy"), nullptr);
	_copy_pick_guide = new QPushButton(QString("Copy Picked"), nullptr);
	_copy_pick_guide->setCheckable(true);

	// layout
	auto input_hbox = make_hbox({_target, _tissues});
	auto object_hbox = make_hbox({_select_object, _object_value});
	auto mode_hbox = make_hbox({_modify, _draw, _erase});
	auto unit_hbox = make_hbox({_radius, _unit_pixel, _unit_mm});
	auto guide_hbox = make_hbox({_show_guide, _guide_offset, _copy_guide, _copy_pick_guide});

	auto layout = new QFormLayout;
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Object value"), object_hbox);
	layout->addRow(tr("Brush Mode"), mode_hbox);
	layout->addRow(tr("Brush Radius"), unit_hbox);
	layout->addRow(tr("Show guide at offset"), guide_hbox);
	setLayout(layout);

	connect(unit_group, SIGNAL(buttonClicked(int)), this, SLOT(unit_changed()));
}

void BrushParamView::unit_changed()
{
	if (_unit_mm->isChecked())
	{
		_radius->setValidator(new QDoubleValidator);
	}
	else
	{
		_radius->setText(QString::number(static_cast<int>(_radius->text().toFloat())));
		_radius->setValidator(new QIntValidator);
	}
}

void BrushParamView::set_object_value(float v)
{
	_object_value->setText(QString::number(v));
	_select_object->setChecked(false);
}

std::vector<iseg::Point> BrushParamView::draw_circle(Point p, float spacing_x, float spacing_y, int width, int height)
{
	Point p1;
	std::vector<Point> vpdyn;
	if (_unit_mm->isChecked())
	{
		float const radius = _radius->text().toFloat();
		float const radius_corrected =
				spacing_x > spacing_y
						? std::floor(radius / spacing_x + 0.5f) * spacing_x
						: std::floor(radius / spacing_y + 0.5f) * spacing_y;
		float const radius_corrected2 = radius_corrected * radius_corrected;

		int const xradius = std::ceil(radius_corrected / spacing_x);
		int const yradius = std::ceil(radius_corrected / spacing_y);
		for (p1.px = std::max(0, p.px - xradius);
				 p1.px <= std::min(width - 1, p.px + xradius);
				 p1.px++)
		{
			for (p1.py = std::max(0, p.py - yradius);
					 p1.py <= std::min(height - 1, p.py + yradius);
					 p1.py++)
			{
				if (std::pow(spacing_x * (p.px - p1.px), 2.f) + std::pow(spacing_y * (p.py - p1.py), 2.f) <= radius_corrected2)
				{
					vpdyn.push_back(p1);
				}
			}
		}
	}
	else
	{
		int const xradius = _radius->text().toInt();
		int const yradius = xradius;
		int const radius2 = xradius * xradius;
		for (p1.px = std::max(0, p.px - xradius);
				 p1.px <= std::min(width - 1, p.px + xradius);
				 p1.px++)
		{
			for (p1.py = std::max(0, p.py - yradius);
					 p1.py <= std::min(height - 1, p.py + yradius);
					 p1.py++)
			{
				if ((p.px - p1.px) * (p.px - p1.px) + (p.py - p1.py) * (p.py - p1.py) <= radius2)
				{
					vpdyn.push_back(p1);
				}
			}
		}
	}
	return vpdyn;
}

FillHolesParamView::FillHolesParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	_all_slices = new QCheckBox;
	_all_slices->setChecked(false);

	_target = new QRadioButton(QString("Target"));
	_tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {_target, _tissues});
	_target->setOn(true);

	_select_object = new QPushButton(tr("Select"));
	_select_object->setCheckable(true);
	_object_value = new QLineEdit(QString::number(255));
	_object_value->setValidator(new QDoubleValidator);

	_object_size = new QSpinBox(1, std::numeric_limits<int>::max(), 1, nullptr);
	_object_size_label = new QLabel(tr("Hole size"));

	_execute = new QPushButton(tr("Execute"));

	auto input_hbox = make_hbox({_target, _tissues});
	auto object_hbox = make_hbox({_select_object, _object_value});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to all slices"), _all_slices);
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Object value"), object_hbox);
	layout->addRow(_object_size_label, _object_size);
	layout->addRow(_execute);
	setLayout(layout);
}

void FillHolesParamView::set_object_value(float v)
{
	_object_value->setText(QString::number(v));
	_select_object->setChecked(false);
}

AddSkinParamView::AddSkinParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	_all_slices = new QCheckBox;
	_all_slices->setChecked(false);

	_target = new QRadioButton(QString("Target"));
	_tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {_target, _tissues});
	_target->setOn(true);

	_thickness = new QLineEdit(QString::number(1));
	_thickness->setValidator(new QIntValidator);
	_unit_pixel = new QRadioButton(tr("Pixel"));
	_unit_mm = new QRadioButton(tr("Use spacing"));
	auto unit_group = make_button_group(this, {_unit_pixel, _unit_mm});
	_unit_pixel->setOn(true);

	_inside = new QRadioButton(tr("Inside"));
	_outside = new QRadioButton(tr("Outside"));
	auto mode_group = make_button_group(this, {_inside, _outside});
	_inside->setOn(true);

	_execute = new QPushButton(tr("Execute"));

	auto input_hbox = make_hbox({_target, _tissues});
	auto thickness_hbox = make_hbox({_thickness, _unit_pixel, _unit_mm});
	auto mode_hbox = make_hbox({_inside, _outside});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to all slices"), _all_slices);
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(tr("Thickness"), thickness_hbox);
	layout->addRow(tr("Where"), mode_hbox);
	layout->addRow(_execute);
	setLayout(layout);

	connect(unit_group, SIGNAL(buttonClicked(int)), this, SLOT(unit_changed()));
}

void AddSkinParamView::unit_changed()
{
	if (_unit_mm->isChecked())
	{
		_thickness->setValidator(new QDoubleValidator);
	}
	else
	{
		_thickness->setText(QString::number(static_cast<int>(_thickness->text().toFloat())));
		_thickness->setValidator(new QIntValidator);
	}
}

FillSkinParamView::FillSkinParamView(SliceHandlerInterface* h, QWidget* parent /*= 0*/) : ParamViewBase(parent), _handler(h)
{
	// parameter fields
	_all_slices = new QCheckBox;
	_all_slices->setChecked(false);

	_thickness = new QLineEdit(QString::number(1));
	_unit_pixel = new QRadioButton(tr("Pixel"));
	_unit_mm = new QRadioButton(tr("Use spacing"));
	auto unit_group = make_button_group(this, {_unit_pixel, _unit_mm});
	_unit_pixel->setOn(true);

	_select_background = new QPushButton(tr("Get Selected"));
	_background_value = new QLineEdit;

	_select_skin = new QPushButton(tr("Get Selected"));
	_skin_value = new QLineEdit;

	_execute = new QPushButton(tr("Execute"));

	// layout
	auto thickness_hbox = make_hbox({_thickness, _unit_pixel, _unit_mm});
	auto bg_hbox = make_hbox({_select_background, _background_value});
	auto fg_hbox = make_hbox({_select_skin, _skin_value});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to all slices"), _all_slices);
	layout->addRow(tr("Thickness"), thickness_hbox);
	layout->addRow(tr("Background"), bg_hbox);
	layout->addRow(tr("Skin"), fg_hbox);
	layout->addRow(_execute);
	setLayout(layout);

	connect(_select_background, SIGNAL(clicked()), this, SLOT(on_select_background()));
	connect(_select_skin, SIGNAL(clicked()), this, SLOT(on_select_skin()));
	connect(unit_group, SIGNAL(buttonClicked(int)), this, SLOT(unit_changed()));
}

void FillSkinParamView::select_background(QString tissueName, tissues_size_t nr)
{
	_background_value->setText(tissueName);

	selectedBackgroundID = nr;
	backgroundSelected = true;

	_execute->setEnabled(backgroundSelected && skinSelected);
}

void FillSkinParamView::select_skin(QString tissueName, tissues_size_t nr)
{
	_skin_value->setText(tissueName);

	selectedSkinID = nr;
	skinSelected = true;

	_execute->setEnabled(backgroundSelected && skinSelected);
}

void FillSkinParamView::init()
{
	_execute->setEnabled(backgroundSelected && skinSelected);
}

void FillSkinParamView::unit_changed()
{
	if (_unit_mm->isChecked())
	{
		_thickness->setValidator(new QDoubleValidator);
	}
	else
	{
		_thickness->setText(QString::number(static_cast<int>(_thickness->text().toFloat())));
		_thickness->setValidator(new QIntValidator);
	}
}

void FillSkinParamView::on_select_background()
{
	auto sel = _handler->tissue_selection();
	if (sel.size() == 1)
	{
		select_background(_handler->tissue_names().at(sel[0]).c_str(), sel[0]);
	}
	else
	{
		ISEG_WARNING("Please select only one tissue");
	}
}

void FillSkinParamView::on_select_skin()
{
	auto sel = _handler->tissue_selection();
	if (sel.size() == 1)
	{
		select_skin(_handler->tissue_names().at(sel[0]).c_str(), sel[0]);
	}
	else
	{
		ISEG_WARNING("Please select only one tissue");
	}
}

FillAllParamView::FillAllParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	_all_slices = new QCheckBox;
	_all_slices->setChecked(false);

	_target = new QRadioButton(QString("Target"));
	_tissues = new QRadioButton(QString("Tissues"));
	auto input_group = make_button_group(this, {_target, _tissues});
	_target->setOn(true);

	_execute = new QPushButton(tr("Execute"));

	auto input_hbox = make_hbox({_target, _tissues});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to all slices"), _all_slices);
	layout->addRow(tr("Input image"), input_hbox);
	layout->addRow(_execute);
	setLayout(layout);
}

SmoothTissuesParamView::SmoothTissuesParamView(QWidget* parent /*= 0*/) : ParamViewBase(parent)
{
	setToolTip(Format(
			"This tool smooths all non-locked tissues by computing a smoothed signed distance "
			"function for each (non-locked) tissue and re-assigning the voxel to that of the most "
			"negative ('most inside') tissue signed distance."));

	_active_slice = new QRadioButton(tr("Current slice"));
	_all_slices = new QRadioButton(tr("All slices (2D)"));
	_3D = new QRadioButton(tr("Fully 3D"));
	auto where_group = make_button_group(this, {_active_slice, _all_slices, _3D});
	_active_slice->setOn(true);

	_sigma = new QLineEdit(QString::number(0.3));
	_sigma->setValidator(new QDoubleValidator);
	_sigma->setToolTip(Format(
			"Gaussian Sigma defines the radius of influence "
			"of the Gaussian smoothing used to smooth the tissues "
			"in world coordinates, e.g. mm. Sigma is the sqrt(variance)."));

	_split_limit = new QLineEdit(QString::number(500));
	_split_limit->setValidator(new QIntValidator);
	_split_limit->setToolTip(Format(
			"Splits active slices into batches of # slices to reduce memory requirements."
			"This option is only used for 3D smoothing."));

	_execute = new QPushButton(QString("Execute"));

	auto hbox = make_hbox({_active_slice, _all_slices, _3D});

	auto layout = new QFormLayout;
	layout->addRow(tr("Apply to"), hbox);
	layout->addRow(tr("Sigma"), _sigma);
	layout->addRow(tr("Max Slices in 3D"), _split_limit);
	layout->addRow(_execute);
	setLayout(layout);
}

} // namespace iseg
