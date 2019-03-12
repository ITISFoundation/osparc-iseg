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
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QWidget>

namespace iseg {

class ParamViewBase : public QWidget
{
	Q_OBJECT
public:
	ParamViewBase(QWidget* parent = nullptr) : QWidget(parent) {}

	virtual void init() {}

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
	OLCorrParamView(QWidget* parent = 0);

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override
	{
		_target->setOn(v);
		_tissues->setOn(!v);
	}
	float object_value() const override { return _object_value->text().toFloat(); }
	void set_object_value(float v) override;

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
	BrushParamView(QWidget* parent = 0);

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override
	{
		_target->setOn(v);
		_tissues->setOn(!v);
	}
	float object_value() const override { return _object_value->text().toFloat(); }
	void set_object_value(float v) override;

	std::vector<Point> draw_circle(Point p, float spacing_x, float spacing_y, int width, int height);

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

private slots:
	void unit_changed();
};

// used for fill holes, remove islands, and fill gaps
class FillHolesParamView : public ParamViewBase
{
	Q_OBJECT
public:
	FillHolesParamView(QWidget* parent = 0);

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override
	{
		_target->setOn(v);
		_tissues->setOn(!v);
	}
	float object_value() const override { return _object_value->text().toFloat(); }
	void set_object_value(float v) override;

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
	AddSkinParamView(QWidget* parent = 0);

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override
	{
		_target->setOn(v);
		_tissues->setOn(!v);
	}

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

private slots:
	void unit_changed();
};

class FillSkinParamView : public ParamViewBase
{
	Q_OBJECT
public:
	FillSkinParamView(SliceHandlerInterface* h, QWidget* parent = 0);

	void init() override;

	void select_background(QString tissueName, tissues_size_t nr);

	void select_skin(QString tissueName, tissues_size_t nr);

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
	void unit_changed();
	void on_select_background();
	void on_select_skin();

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
	FillAllParamView(QWidget* parent = 0);

	bool work() const override { return _target->isOn(); }
	void set_work(bool v) override
	{
		_target->setOn(v);
		_tissues->setOn(!v);
	}

	// params
	QCheckBox* _all_slices;

	QRadioButton* _target;
	QRadioButton* _tissues;

	QPushButton* _execute;
};

class SmoothTissuesParamView : public ParamViewBase
{
	Q_OBJECT
public:
	SmoothTissuesParamView(QWidget* parent = 0);

	// params
	QRadioButton* _active_slice;
	QRadioButton* _all_slices;
	QRadioButton* _3D;
	QLineEdit* _split_limit;
	QLineEdit* _sigma;
	QPushButton* _execute;
};

} // namespace iseg
