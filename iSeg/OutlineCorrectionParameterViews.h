/*
 * Copyright (c) 2018 The Foundation for Research on Information Technologies in Society (IT'IS).
 * 
 * This file is part of iSEG
 * (see https://github.com/ITISFoundation/osparc-iseg).
 * 
 * This software is released under the MIT License.
 *  https://opensource.org/licenses/MIT
 */
#pragma once

#include "Data/Point.h"
#include "Data/SlicesHandlerInterface.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QRadioButton>
#include <QPushButton>
#include <QSpinBox>
#include <QWidget>

namespace iseg {

class ParamViewBase : public QWidget
{
public:
	ParamViewBase(QWidget* parent = nullptr) : QWidget(parent) {}

	virtual bool work() const { return _work; }
	virtual void set_work(bool v) { _work = v; }

	virtual float object_value() const { return _object_value; }
	virtual void set_object_value(float v) { _object_value = v; }

private:
	// cache setting
	bool _work = true;
	float _object_value = 0.f;
};

class OLCorrParamView : public ParamViewBase
{
	Q_OBJECT
public:
	OLCorrParamView(QWidget* parent = 0) : ParamViewBase(parent)
	{
		_target = new QRadioButton(QString("Target"));
		_tissues = new QRadioButton(QString("Tissues"));
		_target->setOn(true);
		auto input_group = make_button_group(this, {_target, _tissues});

		_select_object = new QPushButton(tr("Select"));
		_object_value = new QLineEdit(QString::number(255));

		auto input_hbox = make_hbox({_target, _tissues});
		auto object_hbox = make_hbox({_select_object, _object_value});

		auto layout = new QFormLayout;
		layout->addRow(tr("Input image"), input_hbox);
		layout->addRow(tr("Object value"), object_hbox);
		setLayout(layout);
	}

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override { _target->setOn(v); }
	float object_value() const override { return _object_value->text().toFloat(); }
	void set_object_value(float v) override { _object_value->setText(QString::number(v)); }

	// params
	QRadioButton* _target;
	QRadioButton* _tissues;

	QPushButton* _select_object;
	QLineEdit* _object_value;
};

class BrushParamView : public ParamViewBase
{
	Q_OBJECT
public:
	BrushParamView(QWidget* parent = 0) : ParamViewBase(parent)
	{
		// parameter fields
		_target = new QRadioButton(QString("Target"));
		_tissues = new QRadioButton(QString("Tissues"));
		auto input_group = make_button_group(this, {_target, _tissues});
		_target->setOn(true);

		_select_object = new QPushButton(tr("Select"));
		_object_value = new QLineEdit(QString::number(255));

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
	}

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override { _target->setOn(v); }
	float object_value() const override { return _object_value->text().toFloat(); }
	void set_object_value(float v) override { _object_value->setText(QString::number(v)); }

	std::vector<Point> draw_circle(Point p, float spacing_x, float spacing_y, int width, int height)
	{
		Point p1;
		std::vector<Point> vpdyn;
		if (_unit_mm->isOn())
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

	// params
	QRadioButton* _tissues;
	QRadioButton* _target;

	QPushButton* _select_object;
	QLineEdit* _object_value;

	QRadioButton* _modify;
	QRadioButton* _draw;
	QRadioButton* _erase;

	QLineEdit* _radius;
	QRadioButton* _unit_mm;
	QRadioButton* _unit_pixel;

	QCheckBox* _show_guide;
	QSpinBox* _guide_offset;
	QPushButton* _copy_guide;
	QPushButton* _copy_pick_guide;
};

// used for fill holes, remove islands, and fill gaps
class FillHolesParamView : public ParamViewBase
{
	Q_OBJECT
public:
	FillHolesParamView(QWidget* parent = 0) : ParamViewBase(parent)
	{
		_all_slices = new QCheckBox;
		_all_slices->setChecked(false);

		_target = new QRadioButton(QString("Target"));
		_tissues = new QRadioButton(QString("Tissues"));
		auto input_group = make_button_group(this, {_target, _tissues});
		_target->setOn(true);

		_select_object = new QPushButton(tr("Select"));
		_object_value = new QLineEdit(QString::number(255));

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

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override { _target->setOn(v); }
	float object_value() const override { return _object_value->text().toFloat(); }
	void set_object_value(float v) override { _object_value->setText(QString::number(v)); }

	// params
	QCheckBox* _all_slices;

	QRadioButton* _target;
	QRadioButton* _tissues;

	QPushButton* _select_object;
	QLineEdit* _object_value;

	QSpinBox* _object_size;
	QLabel* _object_size_label;

	QPushButton* _execute;
};

// \todo BL why does this tool not allow to choose object value?
class AddSkinParamView : public ParamViewBase
{
	Q_OBJECT
public:
	AddSkinParamView(QWidget* parent = 0) : ParamViewBase(parent)
	{
		_all_slices = new QCheckBox;
		_all_slices->setChecked(false);

		_target = new QRadioButton(QString("Target"));
		_tissues = new QRadioButton(QString("Tissues"));
		auto input_group = make_button_group(this, {_target, _tissues});
		_target->setOn(true);

		_thickness = new QLineEdit(QString::number(1));
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
	}

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override { _target->setOn(v); }

	// params
	QCheckBox* _all_slices;

	QRadioButton* _target;
	QRadioButton* _tissues;

	QRadioButton* _inside;
	QRadioButton* _outside;

	QLineEdit* _thickness;
	QRadioButton* _unit_mm;
	QRadioButton* _unit_pixel;

	QPushButton* _execute;
};

class FillSkinParamView : public ParamViewBase
{
	Q_OBJECT
public:
	FillSkinParamView(SliceHandlerInterface* h, QWidget* parent = 0) : ParamViewBase(parent), _handler(h)
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
	}

	void select_background(QString tissueName, tissues_size_t nr)
	{
		_background_value->setText(tissueName);

		selectedBackgroundID = nr;
		backgroundSelected = true;

		if (backgroundSelected && skinSelected)
			_execute->setEnabled(true);
		else
			_execute->setEnabled(false);
	}

	void select_skin(QString tissueName, tissues_size_t nr)
	{
		_skin_value->setText(tissueName);

		selectedSkinID = nr;
		skinSelected = true;

		if (backgroundSelected && skinSelected)
			_execute->setEnabled(true);
		else
			_execute->setEnabled(false);
	}

	// params
	QCheckBox* _all_slices;

	QLineEdit* _thickness;
	QRadioButton* _unit_mm;
	QRadioButton* _unit_pixel;

	QPushButton* _select_background;
	QLineEdit* _background_value;

	QPushButton* _select_skin;
	QLineEdit* _skin_value;

	QPushButton* _execute;

private slots:
	void on_select_background()
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

	void on_select_skin()
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

private:
	// data
	SliceHandlerInterface* _handler;
	tissues_size_t selectedBackgroundID;
	tissues_size_t selectedSkinID;
	bool backgroundSelected = false;
	bool skinSelected = false;
};

class FillAllParamView : public ParamViewBase
{
	Q_OBJECT
public:
	FillAllParamView(QWidget* parent = 0) : ParamViewBase(parent)
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

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override { _target->setOn(v); }

	// params
	QCheckBox* _all_slices;

	QRadioButton* _target;
	QRadioButton* _tissues;

	QPushButton* _execute;
};

class AdaptParamView : public ParamViewBase
{
	Q_OBJECT
public:
	AdaptParamView(QWidget* parent = 0) : ParamViewBase(parent)
	{
		_all_slices = new QCheckBox;
		_all_slices->setChecked(false);

		_select_object = new QPushButton(tr("Select"));
		_object_value = new QLineEdit(QString::number(255));

		_execute = new QPushButton(tr("Execute"));

		auto object_hbox = make_hbox({_select_object, _object_value});

		auto layout = new QFormLayout;
		layout->addRow(tr("Apply to all slices"), _all_slices);
		layout->addRow(tr("Object value"), object_hbox);
		layout->addRow(_execute);
		setLayout(layout);
	}

	float object_value() const override { return _object_value->text().toFloat(); }
	void set_object_value(float v) override { _object_value->setText(QString::number(v)); }

	// params
	QCheckBox* _all_slices;

	QPushButton* _select_object;
	QLineEdit* _object_value;

	QPushButton* _execute;
};

class SmoothTissuesParamView : public ParamViewBase
{
	Q_OBJECT
public:
	SmoothTissuesParamView(QWidget* parent = 0) : ParamViewBase(parent)
	{
		setToolTip(Format(
				"This tool smooths all non-locked tissues by computing a smoothed signed distance "
				"function for each (non-locked) tissue and re-assigning the voxel to that of the most "
				"negative ('most inside') tissue signed distance."));

		auto label = new QLabel("Smooth Tissue");
		label->setFixedHeight(80);

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

		_execute = new QPushButton(QString("Execute"));

		auto hbox = make_hbox({_active_slice, _all_slices, _3D});

		auto layout = new QFormLayout;
		layout->addRow(label);
		layout->addRow(tr("Apply to"), hbox);
		layout->addRow(tr("Sigma"), _sigma);
		layout->addRow(_execute);
		setLayout(layout);
	}

	// params
	QRadioButton* _active_slice;
	QRadioButton* _all_slices;
	QRadioButton* _3D;
	QLineEdit* _sigma;
	QPushButton* _execute;
};

}