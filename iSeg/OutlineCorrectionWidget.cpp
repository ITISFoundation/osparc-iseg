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

#include "OutlineCorrectionWidget.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"

#include "Data/AddConnected.h"
#include "Data/ExtractBoundary.h"
#include "Data/Point.h"
#include "Data/addLine.h"

#include "Core/SmoothTissues.h"

#include <QDialog>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QStackedLayout>

#include <initializer_list>

namespace iseg {

std::map<QListWidgetItem*, int> _widget_page;

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

class OLCorrParamView : public ParamViewBase
{
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

	// data
	SliceHandlerInterface* _handler;

private:
	tissues_size_t selectedBackgroundID;
	tissues_size_t selectedSkinID;
	bool backgroundSelected = false;
	bool skinSelected = false;
};

class FillAllParamView : public ParamViewBase
{
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

OutlineCorrectionWidget::OutlineCorrectionWidget(SlicesHandler* hand3D, QWidget* parent, const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"OutLine Correction routines that can be used to modify "
			"the result of a segmentation operation and to correct frequently "
			"occurring segmentation deficiencies."));
	setObjectName("OLC");

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	auto top_layout = new QHBoxLayout;

	// methods
	methods = new QListWidget(this);
	methods->setSelectionMode(QAbstractItemView::SingleSelection);
	methods->setFixedWidth(150);
	methods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	methods->setLineWidth(1);
	methods->setStyleSheet("QListWidget::item:selected { background: rgb(150,150,150); }");

	olcorr = new QListWidgetItem(tr("Outline Corr"), methods);
	olcorr->setToolTip(Format(
			"Draw an alternative boundary segment for a region.This segment "
			"will be connected to the region using the shortest possible lines "
			"and will replace the boundary segment between the connection points."));
	brush = new QListWidgetItem(tr("Brush"), methods);
	brush->setToolTip(Format("Manual correction and segmentation tool: a brush."));

	holefill = new QListWidgetItem(tr("Fill Holes"), methods);
	holefill->setToolTip(Format(
			"Close all holes in the selected region or tissue that have a "
			"size smaller than the number of pixels specified by the Hole Size option."));

	removeislands = new QListWidgetItem(tr("Remove Islands"), methods);
	removeislands->setToolTip(Format(
			"Remove all islands (speckles and outliers) with the selected "
			"gray value or tissue assignment."));

	gapfill = new QListWidgetItem(tr("Fill Gaps"), methods);
	gapfill->setToolTip(Format(
			"Close gaps between multiple disconnected regions having the same "
			"gray value or belonging to the same tissue."));

	addskin = new QListWidgetItem(tr("Add Skin"), methods);
	addskin->setToolTip(Format(
			"Add a skin layer to a segmentation with a "
			"specified Thickness (in pixels)."));

	fillskin = new QListWidgetItem(tr("Fill Skin"), methods);
	fillskin->setToolTip(Format("Thicken a skin layer to ensure it has a minimum Thickness."));

	allfill = new QListWidgetItem(tr("Fill All"), methods);
	allfill->setToolTip(Format(
			"Make sure that there are no unassigned regions "
			"inside the segmented model."));

	adapt = new QListWidgetItem(tr("Adapt"), methods);

	smooth_tissues = new QListWidgetItem(tr("Smooth Tissues"), methods);

#if 0
	// shared parameters
	auto param_vlayout = new QVBoxLayout;
	param_vlayout->setMargin(4);

	param_vlayout->addWidget(hbox1 = new Q3HBox);
	param_vlayout->addWidget(hbox2 = new Q3HBox);
	param_vlayout->addWidget(hbox2a = new Q3HBox);

	tissuesListBackground = new Q3HBox();
	tissuesListBackground->setFixedHeight(18);
	tissuesListSkin = new Q3HBox();
	tissuesListSkin->setFixedHeight(18);

	param_vlayout->addWidget(tissuesListBackground);
	param_vlayout->addWidget(tissuesListSkin);

	allslices = new QCheckBox(QString("Apply to all slices"), nullptr);
	pb_execute = new QPushButton("Fill Islands", nullptr);
	param_vlayout->addWidget(allslices);
	param_vlayout->addWidget(pb_execute);

	param_vlayout->addWidget(hbox3a = new Q3HBox());
	param_vlayout->addWidget(hbox4 = new Q3HBox());
	param_vlayout->addWidget(hbox5o = new Q3HBox());
	param_vlayout->addWidget(hbox6 = new Q3HBox());
	param_vlayout->addWidget(hbox5 = new Q3HBox());
	param_vlayout->addWidget(hboxpixormm = new Q3HBox());
	param_vlayout->addWidget(hbox_prev_slice = new Q3HBox());

	auto shared_method_params = new QWidget;
	shared_method_params->setLayout(param_vlayout);

	txt_radius = new QLabel("Thickness: ", hbox1);
	sb_radius = new QSpinBox(0, 99, 2, hbox1);
	sb_radius->setValue(1);
	sb_radius->show();
	mm_radius = new QLineEdit(QString::number(1), hbox1);
	mm_radius->hide();
	txt_unit = new QLabel(" pixels", hbox1);

	txt_holesize = new QLabel("Island Size: ", hbox2);
	sb_holesize = new QSpinBox(1, 10000, 1, hbox2);
	sb_holesize->setValue(100);
	sb_holesize->show();

	txt_gapsize = new QLabel("Gap Size: ", hbox2a);
	sb_gapsize = new QSpinBox(1, 10, 1, hbox2a);
	sb_gapsize->setValue(2);
	sb_gapsize->show();

	QLabel* tissuesListBackgroundLabel = new QLabel(tissuesListBackground);
	tissuesListBackgroundLabel->setText("Background");
	getCurrentTissueBackground = new QPushButton(tissuesListBackground);
	getCurrentTissueBackground->setText("Get Selected");
	backgroundText = new QLineEdit(tissuesListBackground);
	backgroundText->setText("Select Background");
	backgroundText->setEnabled(false);

	QLabel* tissuesListSkinLabel = new QLabel(tissuesListSkin);
	tissuesListSkinLabel->setText("Skin");
	getCurrentTissueSkin = new QPushButton(tissuesListSkin);
	getCurrentTissueSkin->setText("Get Selected");
	skinText = new QLineEdit(tissuesListSkin);
	skinText->setText("Select Skin");
	skinText->setEnabled(false);

	brushtype = new QButtonGroup(this);

	modifybrush = new QRadioButton(QString("Modify"), hbox3a);
	modifybrush->setToolTip(Format(
			"The Modify mode depends on where the mouse is initially pressed down. "
			"If it is pressed down within the selected region or tissue, it acts "
			"as a drawing brush. In contrast, when it is pressed down outside, it "
			"will overwrite the selected region or tissue with the gray value or "
			"tissue assignment of the point where the mouse has initially been pressed down."));
	drawbrush = new QRadioButton(QString("Draw"), hbox3a);
	drawbrush->setToolTip(Format(
			"Use the brush to draw with the currently selected gray "
			"value(TargetPict) or tissue assignment (Tissue)."));
	erasebrush = new QRadioButton(QString("Erase"), hbox3a);
	erasebrush->setToolTip(Format("Erase a region leaving black or no tissue assignment behind."));

	brushtype->insert(modifybrush);
	brushtype->insert(drawbrush);
	brushtype->insert(erasebrush);
	modifybrush->show();
	modifybrush->setChecked(TRUE);
	drawbrush->show();
	erasebrush->show();

	target = new QButtonGroup(this);
	//	target->hide();
	work = new QRadioButton(QString("TargetPict"), hbox4);
	tissue = new QRadioButton(QString("Tissue"), hbox4);
	target->insert(work);
	target->insert(tissue);
	work->show();
	work->setChecked(TRUE);
	tissue->show();

	pb_selectobj = new QPushButton("Select Object", hbox5o);
	pb_selectobj->setToolTip(Format(
			"Press this button and select the object in the target image. This "
			"object will be considered as foreground by the current operation."));
	auto obj_label = new QLabel(QString("Object value"), hbox6);
	object_value = new QLineEdit(QString::number(255.f), hbox6);
	object_value->setValidator(new QDoubleValidator);

	in_or_out = new QButtonGroup(this);
	inside = new QRadioButton(QString("Inside"), hbox5);
	outside = new QRadioButton(QString("Outside"), hbox5);
	in_or_out->insert(inside);
	in_or_out->insert(outside);
	inside->show();
	inside->setChecked(TRUE);
	outside->show();

	pixelormm = new QButtonGroup(this);
	pixel = new QRadioButton(QString("pixel"), hboxpixormm);
	mm = new QRadioButton(QString("mm"), hboxpixormm);
	pixelormm->insert(pixel);
	pixelormm->insert(mm);
	pixel->setChecked(TRUE);

	cb_show_guide = new QCheckBox(QString("Show guide"), hbox_prev_slice);
	cb_show_guide->setChecked(false);
	auto prev_offset_label = new QLabel(QString("Offset: "), hbox_prev_slice);
	sb_guide_offset = new QSpinBox(-100, 100, 1, hbox_prev_slice);
	sb_guide_offset->setValue(1);
	pb_copy_guide = new QPushButton(QString("Copy"), hbox_prev_slice);
	pb_copy_pick_guide = new QPushButton(QString("Copy Picked"), hbox_prev_slice);
#endif
	// other parameter views
	// todo
	olc_params = new OLCorrParamView;
	brush_params = new BrushParamView;
	fill_holes_params = new FillHolesParamView;
	remove_islands_params = new FillHolesParamView;
	remove_islands_params->_object_size_label->setText("Island size");
	fill_gaps_params = new FillHolesParamView;
	fill_gaps_params->_object_size_label->setText("Gap size");
	add_skin_params = new AddSkinParamView;
	fill_skin_params = new FillSkinParamView(handler3D);
	fill_all_params = new FillAllParamView;
	adapt_params = new AdaptParamView;
	smooth_tissues_params = new SmoothTissuesParamView;

	// layouts
	parameter_area = new QWidget(this);
	stacked_param_layout = new QStackedLayout;
	stacked_param_layout->addWidget(olc_params);
	stacked_param_layout->addWidget(brush_params);
	stacked_param_layout->addWidget(fill_holes_params);
	stacked_param_layout->addWidget(remove_islands_params);
	stacked_param_layout->addWidget(fill_gaps_params);
	stacked_param_layout->addWidget(add_skin_params);
	stacked_param_layout->addWidget(fill_skin_params);
	stacked_param_layout->addWidget(fill_all_params);
	stacked_param_layout->addWidget(adapt_params);
	stacked_param_layout->addWidget(smooth_tissues_params);
	parameter_area->setLayout(stacked_param_layout);

	top_layout->addWidget(methods);
	top_layout->addWidget(parameter_area);
	setLayout(top_layout);

	// remember QStackedLayout page where parameters of method are added
	for (int i = 0; i < methods->count(); ++i)
	{
		_widget_page[methods->item(i)] = i;
	}

	// start with brush tool
	brush->setSelected(true);
	stacked_param_layout->setCurrentWidget(brush_params);

	// create connections
	connect(methods, SIGNAL(itemSelectionChanged()), this, SLOT(method_changed()));
#if 0
	connect(target, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
	connect(allslices, SIGNAL(clicked()), this, SLOT(method_changed()));
	connect(pixelormm, SIGNAL(buttonClicked(int)), this, SLOT(pixmm_changed()));
	connect(pb_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	connect(pb_selectobj, SIGNAL(clicked()), this, SLOT(selectobj_pushed()));

	connect(cb_show_guide, SIGNAL(clicked()), this, SLOT(draw_guide()));
	connect(sb_guide_offset, SIGNAL(valueChanged(int)), this, SLOT(draw_guide()));
	connect(pb_copy_guide, SIGNAL(clicked()), this, SLOT(copy_guide()));
	connect(pb_copy_pick_guide, SIGNAL(clicked()), this, SLOT(copy_pick_pushed()));

	connect(getCurrentTissueBackground, SIGNAL(clicked()), this, SLOT(on_select_background()));
	connect(getCurrentTissueSkin, SIGNAL(clicked()), this, SLOT(on_select_skin()));
#endif

	connect(fill_holes_params->_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	connect(remove_islands_params->_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	connect(fill_gaps_params->_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	connect(add_skin_params->_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	connect(fill_skin_params->_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	connect(fill_all_params->_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	connect(adapt_params->_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	connect(smooth_tissues_params->_execute, SIGNAL(clicked()), this, SLOT(smooth_tissues_pushed()));

	selectobj = false;

	method_changed();
	workbits_changed();
}

OutlineCorrectionWidget::~OutlineCorrectionWidget()
{
}

void OutlineCorrectionWidget::draw_circle(Point p)
{
	vpdyn = brush_params->draw_circle(p, spacing[0], spacing[1], bmphand->return_width(), bmphand->return_height());

	emit vpdyn_changed(&vpdyn);
	vpdyn.clear();
}

void OutlineCorrectionWidget::draw_guide()
{
	if (brush->isSelected() && brush_params->_show_guide->isChecked())
	{
		int slice = static_cast<int>(handler3D->active_slice()) + brush_params->_guide_offset->value();
		unsigned slice_clamped = std::min(std::max(slice, 0), handler3D->num_slices() - 1);
		if (slice == slice_clamped)
		{
			std::vector<Mark> marks;
			auto w = handler3D->width();
			auto h = handler3D->height();
			if (brush_params->_target->isOn())
			{
				Mark m(Mark::WHITE);
				marks = extract_boundary<Mark, float>(handler3D->target_slices().at(slice_clamped), w, h, m);
			}
			else
			{
				Mark m(tissuenr);
				marks = extract_boundary<Mark, tissues_size_t>(handler3D->tissue_slices(0).at(slice_clamped), w, h, m,
						[this](tissues_size_t v) { return (v == tissuenr); });
			}

			emit vm_changed(&marks);

			return;
		}
	}

	std::vector<Mark> marks;
	emit vm_changed(&marks);
}

void OutlineCorrectionWidget::copy_guide(Point* p)
{
	int slice = static_cast<int>(handler3D->active_slice()) + brush_params->_guide_offset->value();
	unsigned slice_clamped = std::min(std::max(slice, 0), handler3D->num_slices() - 1);
	if (slice == slice_clamped)
	{
		unsigned w = handler3D->width();
		unsigned h = handler3D->height();

		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = brush_params->_target->isOn();
		dataSelection.tissues = !dataSelection.work;
		emit begin_datachange(dataSelection, this);

		if (dataSelection.work)
		{
			auto ref = handler3D->target_slices().at(slice_clamped);
			auto current = handler3D->target_slices().at(handler3D->active_slice());

			if (p)
			{
				unsigned pos = p->px + w * p->py;
				add_connected_2d(ref, current, w, h, pos, 255.f,
						[&](unsigned idx) { return (ref[idx] != 0.f); });
			}
			else
			{
				std::transform(ref, ref + w * h, current, current,
						[](float r, float c) { return (r != 0.f ? r : c); });
			}
		}
		else
		{
			auto ref = handler3D->tissue_slices(0).at(slice_clamped);
			auto current = handler3D->tissue_slices(0).at(handler3D->active_slice());

			if (p)
			{
				unsigned pos = p->px + w * p->py;
				std::vector<tissues_size_t> temp(w * h, 0);
				add_connected_2d(ref, temp.data(), w, h, pos, tissuenr,
						[&](unsigned idx) { return ref[idx] == tissuenr; });
				ref = temp.data();
			}

			std::transform(ref, ref + w * h, current, current, [this](tissues_size_t r, tissues_size_t c) {
				return (r == tissuenr && !TissueInfos::GetTissueLocked(c)) ? r : c;
			});
		}

		emit end_datachange(this);
	}
	else
	{
		//BL
	}
}

void OutlineCorrectionWidget::on_mouse_clicked(Point p)
{
	if (selectobj)
	{
		if (copy_mode)
		{
			copy_mode = false;
			copy_guide(&p);
			brush_params->_copy_pick_guide->setDown(false);
		}
		else if (current_params)
		{
			auto v = bmphand->work_pt(p);
			current_params->set_object_value(v);
		}
		return;
	}

	if (olcorr->isSelected())
	{
		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = olc_params->_target->isOn();
		dataSelection.tissues = !dataSelection.work;

		vpdyn.clear();
		last_pt = p;
		vpdyn.push_back(p);
		emit begin_datachange(dataSelection, this);
	}
	else if (brush->isSelected())
	{
		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = brush_params->_target->isOn();
		dataSelection.tissues = !dataSelection.work;

		float const f = get_object_value();
		last_pt = p;

		if (brush_params->_modify->isOn())
		{
			if (brush_params->_target->isOn())
			{
				if (bmphand->work_pt(p) == f)
					draw = true;
				else
					draw = false;
			}
			else
			{
				if (bmphand->tissues_pt(handler3D->active_tissuelayer(), p) == tissuenr)
				{
					draw = true;
				}
				else
				{
					draw = false;
					tissuenrnew = bmphand->tissues_pt(handler3D->active_tissuelayer(), p);
				}
			}
		}
		else if (brush_params->_draw->isOn())
		{
			draw = true;
		}
		else
		{
			draw = false;
			if (bmphand->tissues_pt(handler3D->active_tissuelayer(), p) == tissuenr)
				tissuenrnew = 0;
			else
				tissuenrnew = bmphand->tissues_pt(handler3D->active_tissuelayer(), p);
		}

		emit begin_datachange(dataSelection, this);
		if (brush_params->_target->isOn())
		{
			if (brush_params->_unit_mm->isOn())
				bmphand->brush(f, p, brush_params->_radius->text().toFloat(), spacing[0], spacing[1], draw);
			else
				bmphand->brush(f, p, brush_params->_radius->text().toInt(), draw);
		}
		else
		{
			auto idx = handler3D->active_tissuelayer();
			if (brush_params->_unit_mm->isOn())
				bmphand->brushtissue(idx, tissuenr, p, brush_params->_radius->text().toFloat(), spacing[0], spacing[1], draw, tissuenrnew);
			else
				bmphand->brushtissue(idx, tissuenr, p, brush_params->_radius->text().toInt(), draw, tissuenrnew);
		}
		emit end_datachange(this, iseg::NoUndo);

		draw_circle(p);
	}
}

void OutlineCorrectionWidget::on_mouse_moved(Point p)
{
	if (!selectobj)
	{
		float const f = get_object_value();

		if (olcorr->isSelected())
		{
			vpdyn.pop_back();
			addLine(&vpdyn, last_pt, p);
			last_pt = p;
			emit vpdyn_changed(&vpdyn);
		}
		else if (brush->isSelected())
		{
			draw_circle(p);

			std::vector<Point> vps;
			vps.clear();
			addLine(&vps, last_pt, p);
			for (auto it = ++(vps.begin()); it != vps.end(); it++)
			{
				if (brush_params->_target->isOn())
				{
					if (brush_params->_unit_mm->isOn())
						bmphand->brush(f, *it, brush_params->_radius->text().toFloat(),
								spacing[0], spacing[1], draw);
					else
						bmphand->brush(f, *it, brush_params->_radius->text().toInt(), draw);
				}
				else
				{
					auto idx = handler3D->active_tissuelayer();
					if (brush_params->_unit_mm->isOn())
						bmphand->brushtissue(
								idx, tissuenr, *it, brush_params->_radius->text().toFloat(),
								spacing[0], spacing[1], draw, tissuenrnew);
					else
						bmphand->brushtissue(idx, tissuenr, *it,
								brush_params->_radius->text().toInt(), draw,
								tissuenrnew);
				}
			}
			emit end_datachange(this, iseg::NoUndo);
			last_pt = p;
		}
	}
}

void OutlineCorrectionWidget::on_mouse_released(Point p)
{
	if (selectobj)
	{
		selectobj = false;
	}
	else
	{
		float const f = get_object_value();

		if (olcorr->isSelected())
		{
			vpdyn.pop_back();
			addLine(&vpdyn, last_pt, p);
			if (olc_params->_target->isOn())
				bmphand->correct_outline(f, &vpdyn);
			else
				bmphand->correct_outlinetissue(handler3D->active_tissuelayer(), tissuenr, &vpdyn);
			emit end_datachange(this);

			vpdyn.clear();
			emit vpdyn_changed(&vpdyn);
		}
		else if (brush->isSelected())
		{
			vpdyn.clear();
			addLine(&vpdyn, last_pt, p);
			if (brush_params->_unit_mm->isOn())
			{
				auto mm_radius = brush_params->_radius->text().toFloat();
				for (auto it = ++(vpdyn.begin()); it != vpdyn.end(); it++)
				{
					if (brush_params->_target->isOn())
					{
						bmphand->brush(f, *it, mm_radius, spacing[0], spacing[1], draw);
					}
					else
					{
						auto layer = handler3D->active_tissuelayer();
						bmphand->brushtissue(layer, tissuenr, *it, mm_radius, spacing[0], spacing[1], draw, tissuenrnew);
					}
				}
			}
			else
			{
				auto pixel_radius = brush_params->_radius->text().toFloat();
				for (auto it = ++(vpdyn.begin()); it != vpdyn.end(); it++)
				{
					if (brush_params->_target->isOn())
					{
						bmphand->brush(f, *it, pixel_radius, draw);
					}
					else
					{
						auto layer = handler3D->active_tissuelayer();
						bmphand->brushtissue(layer, tissuenr, *it, pixel_radius, draw, tissuenrnew);
					}
				}
			}
			
			emit end_datachange(this);

			vpdyn.clear();
			emit vpdyn_changed(&vpdyn);
		}
	}
}

void OutlineCorrectionWidget::method_changed()
{
	if (_widget_page.count(methods->currentItem()))
	{
		stacked_param_layout->setCurrentIndex(_widget_page[methods->currentItem()]);
	}

	// keep selected object definition across tools
	auto new_widget = dynamic_cast<ParamViewBase*>(stacked_param_layout->currentWidget());
	if (current_params)
	{
		new_widget->set_work(current_params->work());
		new_widget->set_object_value(current_params->object_value());
	}
	current_params = new_widget;

	// make sure we are not expecting a mouse click
	selectobj = false;
	// ensure this is reset
	copy_mode = false;

#if 0
	tissuesListBackground->hide();
	tissuesListSkin->hide();
	pb_execute->setEnabled(true);

	if (olcorr->isSelected())
	{
		hbox1->hide();
		hbox2->hide();
		hbox2a->hide();
		hbox3a->hide();
		hbox4->show();
		hbox5->hide();
		hboxpixormm->hide();
		allslices->hide();
		pb_execute->hide();
		hbox6->hide();
		hbox_prev_slice->hide();
		if (work->isOn())
		{
			hbox5o->show();
		}
		else
		{
			hbox5o->hide();
		}
	}
	else if (brush->isSelected())
	{
		hbox1->show();
		txt_radius->setText("Radius: ");
		hbox2->hide();
		hbox2a->hide();
		hbox3a->show();
		hbox4->show();
		hbox5->hide();
		hboxpixormm->show();
		hbox_prev_slice->show();
		allslices->hide();
		pb_execute->hide();
		if (work->isOn())
		{
			hbox5o->show();
			hbox6->show();
		}
		else
		{
			hbox5o->hide();
			hbox6->hide();
		}
	}
	else if (holefill->isSelected())
	{
		hbox1->hide();
		txt_holesize->setText("Hole Size: ");
		if (hideparams)
			hbox2->hide();
		else
			hbox2->show();
		hbox2a->hide();
		hbox3a->hide();
		hbox4->show();
		hbox5->hide();
		hboxpixormm->hide();
		allslices->show();
		pb_execute->setText("Fill Holes");
		pb_execute->show();
		hbox6->hide();
		hbox_prev_slice->hide();
		if (work->isOn())
		{
			hbox5o->show();
		}
		else
		{
			hbox5o->hide();
		}
	}
	else if (removeislands->isSelected())
	{
		hbox1->hide();
		txt_holesize->setText("Island Size: ");
		if (hideparams)
			hbox2->hide();
		else
			hbox2->show();
		hbox2a->hide();
		hbox3a->hide();
		hbox4->show();
		hbox5->hide();
		hboxpixormm->hide();
		allslices->show();
		pb_execute->setText("Remove Islands");
		pb_execute->show();
		hbox6->hide();
		hbox_prev_slice->hide();
		if (work->isOn())
		{
			hbox5o->show();
		}
		else
		{
			hbox5o->hide();
		}
	}
	else if (gapfill->isSelected())
	{
		hbox1->hide();
		hbox2->hide();
		if (hideparams)
			hbox2a->hide();
		else
			hbox2a->show();
		hbox3a->hide();
		hbox4->show();
		hbox5->hide();
		hboxpixormm->hide();
		allslices->show();
		pb_execute->setText("Fill Gaps");
		pb_execute->show();
		hbox5o->hide();
		hbox6->hide();
		hbox_prev_slice->hide();
	}
	else if (addskin->isSelected())
	{
		if (hideparams)
			hbox1->hide();
		else
			hbox1->show();
		hbox2->hide();
		hbox2a->hide();
		hbox3a->hide();
		hbox4->show();
		if (hideparams)
			hbox5->hide();
		else
			hbox5->show();
		if (allslices->isChecked())
			hboxpixormm->show();
		else
			hboxpixormm->hide();
		txt_radius->setText("Thickness: ");
		allslices->show();
		pb_execute->setText("Add Skin");
		pb_execute->show();
		hbox5o->hide();
		hbox6->hide();
		hbox_prev_slice->hide();
	}
	else if (fillskin->isSelected())
	{
		tissuesListBackground->show();
		tissuesListSkin->show();
		if (hideparams)
			hbox1->hide();
		else
			hbox1->show();
		hbox2->hide();
		hbox2a->hide();
		hbox3a->hide();
		hbox4->hide();
		hbox5->hide();
		if (allslices->isChecked())
			hboxpixormm->show();
		else
			hboxpixormm->hide();
		txt_radius->setText("Thickness: ");
		allslices->show();
		pb_execute->setText("Fill Skin");
		pb_execute->show();
		if (backgroundSelected && skinSelected)
			pb_execute->setEnabled(true);
		else
			pb_execute->setEnabled(false);
		hbox5o->hide();
		hbox6->hide();
		hbox_prev_slice->hide();
	}
	else if (allfill->isSelected())
	{
		hbox1->hide();
		hbox2->hide();
		hbox2a->hide();
		hbox3a->hide();
		hbox4->show();
		hbox5->hide();
		hboxpixormm->hide();
		allslices->show();
		pb_execute->setText("Fill All");
		pb_execute->show();
		hbox5o->hide();
		hbox6->hide();
		hbox_prev_slice->hide();
	}
	else if (adapt->isSelected())
	{
		hbox1->hide();
		hbox2->hide();
		hbox2a->hide();
		hbox3a->hide();
		hbox4->hide();
		hbox5->hide();
		hboxpixormm->hide();
		allslices->show();
		pb_execute->setText("Adapt");
		pb_execute->show();
		hbox5o->show();
		hbox6->hide();
		hbox_prev_slice->hide();
	}
#endif
	draw_guide();
}

void OutlineCorrectionWidget::execute_pushed()
{
	float const f = get_object_value();

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();

	if (holefill->isSelected())
	{
		dataSelection.allSlices = fill_holes_params->_all_slices->isChecked();
		dataSelection.work = fill_holes_params->_target->isChecked();
		dataSelection.tissues = !dataSelection.work;
		emit begin_datachange(dataSelection, this);

		auto hole_size = fill_holes_params->_object_size->value();
		if (dataSelection.allSlices)
		{
			if (dataSelection.work)
				handler3D->fill_holes(f, hole_size);
			else
				handler3D->fill_holestissue(tissuenr, hole_size);
		}
		else
		{
			if (dataSelection.work)
				bmphand->fill_holes(f, hole_size);
			else
				bmphand->fill_holestissue(handler3D->active_tissuelayer(), tissuenr, hole_size);
		}
	}
	else if (removeislands->isSelected())
	{
		dataSelection.allSlices = remove_islands_params->_all_slices->isChecked();
		dataSelection.work = remove_islands_params->_target->isChecked();
		dataSelection.tissues = !dataSelection.work;
		emit begin_datachange(dataSelection, this);

		auto island_size = remove_islands_params->_object_size->value();
		if (dataSelection.allSlices)
		{
			if (dataSelection.work)
				handler3D->remove_islands(f, island_size);
			else
				handler3D->remove_islandstissue(tissuenr, island_size);
		}
		else
		{
			if (dataSelection.work)
				bmphand->remove_islands(f, island_size);
			else
				bmphand->remove_islandstissue(handler3D->active_tissuelayer(), tissuenr, island_size);
		}
	}
	else if (gapfill->isSelected())
	{
		dataSelection.allSlices = fill_gaps_params->_all_slices->isChecked();
		dataSelection.work = fill_gaps_params->_target->isChecked();
		dataSelection.tissues = !dataSelection.work;
		emit begin_datachange(dataSelection, this);

		auto gap_size = fill_gaps_params->_object_size->value();
		if (dataSelection.allSlices)
		{
			if (dataSelection.work)
				handler3D->fill_gaps(gap_size, false);
			else
				handler3D->fill_gapstissue(gap_size, false);
		}
		else
		{
			if (dataSelection.work)
				bmphand->fill_gaps(gap_size, false);
			else
				bmphand->fill_gapstissue(handler3D->active_tissuelayer(), gap_size, false);
		}
	}
	else if (addskin->isSelected())
	{
		dataSelection.allSlices = add_skin_params->_all_slices->isChecked();
		dataSelection.work = add_skin_params->_target->isChecked();
		dataSelection.tissues = !dataSelection.work;
		emit begin_datachange(dataSelection, this);

		auto radius = add_skin_params->_thickness->text().toFloat();
		auto mm_unit = add_skin_params->_unit_mm->isOn();
		auto outside = add_skin_params->_outside->isOn();

		int const rx = mm_unit ? static_cast<int>(radius / spacing[0] + 0.1f) : radius;
		int const ry = mm_unit ? static_cast<int>(radius / spacing[1] + 0.1f) : radius;
		int const rz = mm_unit ? static_cast<int>(radius / spacing[2] + 0.1f) : radius;
		bool atBoundary = false;
		if (outside)
		{
			if (dataSelection.allSlices)
			{
				if (mm_unit)
				{
					// \warning creates block shaped kernel, instead of ellipsoid
					if (dataSelection.work)
					{
						float setto = handler3D->add_skin3D_outside2(rx, ry, rz);
						atBoundary = handler3D->value_at_boundary3D(setto);
					}
					else
					{
						handler3D->add_skintissue3D_outside2(rx, ry, rz, tissuenr);
						atBoundary = handler3D->tissuevalue_at_boundary3D(tissuenr);
					}
				}
				else
				{
					if (dataSelection.work)
					{
						float setto = handler3D->add_skin3D_outside(rx);
						atBoundary = handler3D->value_at_boundary3D(setto);
					}
					else
					{
						handler3D->add_skintissue3D_outside(rx, tissuenr);
						atBoundary = handler3D->tissuevalue_at_boundary3D(tissuenr);
					}
				}
			}
			else // active slice
			{
				if (dataSelection.work)
				{
					float setto = bmphand->add_skin_outside(rx);
					atBoundary = bmphand->value_at_boundary(setto);
				}
				else
				{
					bmphand->add_skintissue_outside(handler3D->active_tissuelayer(), rx, tissuenr);
					atBoundary = bmphand->tissuevalue_at_boundary(handler3D->active_tissuelayer(), tissuenr);
				}
			}
		}
		else
		{
			if (dataSelection.allSlices)
			{
				if (mm_unit)
				{
					// \warning creates block shaped kernel, instead of ellipsoid
					if (dataSelection.work)
					{
						float setto = handler3D->add_skin3D(rx, ry, rz);
						atBoundary = handler3D->value_at_boundary3D(setto);
					}
					else
					{
						handler3D->add_skintissue3D(rx, ry, rz, tissuenr);
						atBoundary = handler3D->tissuevalue_at_boundary3D(tissuenr);
					}
				}
				else
				{
					if (dataSelection.work)
					{
						float setto = handler3D->add_skin3D(rx);
						atBoundary = handler3D->value_at_boundary3D(setto);
					}
					else
					{
						handler3D->add_skintissue3D(rx, ry, rz, tissuenr);
						atBoundary = handler3D->tissuevalue_at_boundary3D(tissuenr);
					}
				}
			}
			else // active slice
			{
				if (mm_unit)
				{
					// \warning if spacing is anisotropic, skin thickness will be non-uniform
					if (dataSelection.work)
					{
						float setto = bmphand->add_skin(rx);
						atBoundary = bmphand->value_at_boundary(setto);
					}
					else
					{
						bmphand->add_skintissue(handler3D->active_tissuelayer(), rx, tissuenr);
						atBoundary = bmphand->tissuevalue_at_boundary(handler3D->active_tissuelayer(), tissuenr);
					}
				}
				else
				{
					if (dataSelection.work)
					{
						float setto = bmphand->add_skin(rx);
						atBoundary = bmphand->value_at_boundary(setto);
					}
					else
					{
						bmphand->add_skintissue(handler3D->active_tissuelayer(), rx, tissuenr);
						atBoundary = bmphand->tissuevalue_at_boundary(handler3D->active_tissuelayer(), tissuenr);
					}
				}
			}
		}
		if (atBoundary)
		{
			QMessageBox::information(this, "iSeg",
					"Information:\nThe skin partially touches "
					"or\nintersects with the boundary.");
		}
	}
	else if (fillskin->isSelected())
	{
		float mm_rad = fill_skin_params->_thickness->text().toFloat();
		bool mm_unit = fill_skin_params->_unit_mm->isOn();
		int const xThick = mm_unit ? static_cast<int>(mm_rad / spacing[0] + 0.1f) : mm_rad;
		int const yThick = mm_unit ? static_cast<int>(mm_rad / spacing[1] + 0.1f) : mm_rad;
		int const zThick = mm_unit ? static_cast<int>(mm_rad / spacing[2] + 0.1f) : mm_rad;

		auto selectedBackgroundID = fill_skin_params->_background_value->text().toInt();
		auto selectedSkinID = fill_skin_params->_skin_value->text().toInt();
		if (fill_skin_params->_all_slices->isChecked())
			handler3D->fill_skin_3d(xThick, yThick, zThick, selectedBackgroundID, selectedSkinID);
		else
			bmphand->fill_skin(xThick, yThick, selectedBackgroundID, selectedSkinID);
	}
	else if (allfill->isSelected())
	{
		dataSelection.allSlices = fill_all_params->_all_slices->isChecked();
		dataSelection.work = fill_all_params->_target->isChecked();
		dataSelection.tissues = !dataSelection.work;
		emit begin_datachange(dataSelection, this);

		if (dataSelection.allSlices)
		{
			if (dataSelection.work)
				handler3D->fill_unassigned();
			else
				handler3D->fill_unassignedtissue(tissuenr);
		}
		else
		{
			if (dataSelection.work)
				bmphand->fill_unassigned();
			else
				bmphand->fill_unassignedtissue(handler3D->active_tissuelayer(), tissuenr);
		}
	}
	else if (adapt->isSelected())
	{
		dataSelection.allSlices = adapt_params->_all_slices->isChecked();
		dataSelection.work = false;
		dataSelection.tissues = !dataSelection.work;
		emit begin_datachange(dataSelection, this);

		if (dataSelection.allSlices)
		{
			handler3D->adaptwork2bmp(f);
		}
		else
		{
			bmphand->adaptwork2bmp(f);
		}
	}

	emit end_datachange(this);
}

void OutlineCorrectionWidget::selectobj_pushed()
{
	selectobj = true;
}

void OutlineCorrectionWidget::copy_pick_pushed()
{
	selectobj = true;
	copy_mode = true;
	brush_params->_copy_pick_guide->setDown(true);
}

void OutlineCorrectionWidget::workbits_changed()
{
	float const f = get_object_value();

	bmphand = handler3D->get_activebmphandler();
	float* workbits = bmphand->return_work();
	unsigned area = unsigned(bmphand->return_width()) * bmphand->return_height();
	unsigned i = 0;
	while (i < area && workbits[i] != f)
		i++;
	if (i == area)
	{
		Pair p;
		bmphand->get_range(&p);
		if (p.high > p.low)
		{
			current_params->set_object_value(p.high);
		}
	}
}

void OutlineCorrectionWidget::on_slicenr_changed()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand_changed(handler3D->get_activebmphandler());
	}
	else
	{
		workbits_changed();
	}
	draw_guide();
}

void OutlineCorrectionWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
	spacing = handler3D->spacing();
	workbits_changed();
}

void OutlineCorrectionWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
	spacing = handler3D->spacing();
}

void OutlineCorrectionWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
	spacing = handler3D->spacing();
	draw_guide();
}

void OutlineCorrectionWidget::on_tissuenr_changed(int tissuenr1)
{
	tissuenr = (tissues_size_t)(tissuenr1 + 1);
	draw_guide();
}

void OutlineCorrectionWidget::cleanup()
{
	vpdyn.clear();
	emit vpdyn_changed(&vpdyn);
	std::vector<Mark> vm;
	emit vm_changed(&vm);
}

FILE* OutlineCorrectionWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = brush_params->_radius->text().toInt();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = fill_holes_params->_object_size->text().toInt();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = fill_gaps_params->_object_size->text().toInt();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(brush->isSelected());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(olcorr->isSelected());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(holefill->isSelected() || removeislands->isSelected());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(gapfill->isSelected());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allfill->isSelected());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(addskin->isSelected());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(fillskin->isSelected());
		//fwrite(&(dummy), 1,sizeof(int), fp);
		bool w = current_params->work();
		dummy = (int)(!w);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(w);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(brush_params->_erase->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(brush_params->_draw->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(brush_params->_modify->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(adapt->isSelected());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(add_skin_params->_inside->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(add_skin_params->_outside->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(fill_holes_params->_all_slices->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* OutlineCorrectionWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		disconnect(methods, SIGNAL(itemSelectionChanged()), this, SLOT(method_changed()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		brush_params->_radius->setText(QString(dummy));
		fread(&dummy, sizeof(int), 1, fp);
		fill_holes_params->_object_size->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		fill_gaps_params->_object_size->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		brush->setSelected(dummy != 0);
		fread(&dummy, sizeof(int), 1, fp);
		olcorr->setSelected(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		holefill->setSelected(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		gapfill->setSelected(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		allfill->setSelected(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		addskin->setSelected(dummy > 0);

		//fread(&dummy,sizeof(int), 1, fp);
		fillskin->setSelected(false);

		fread(&dummy, sizeof(int), 1, fp);
		// tissue & work is exclusive;
		fread(&dummy, sizeof(int), 1, fp);
		current_params->set_work(dummy != 0);
		fread(&dummy, sizeof(int), 1, fp);
		brush_params->_erase->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		brush_params->_draw->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		brush_params->_modify->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		adapt->setSelected(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		add_skin_params->_inside->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		add_skin_params->_outside->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		fill_holes_params->_all_slices->setChecked(dummy > 0);

		method_changed();

		connect(methods, SIGNAL(itemSelectionChanged()), this, SLOT(method_changed()));
		//connect(target, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
	}
	return fp;
}

void OutlineCorrectionWidget::hideparams_changed() { method_changed(); }

float OutlineCorrectionWidget::get_object_value() const
{
	if (current_params)
	{
		return current_params->object_value();
	}
	return 0.f;
}

void OutlineCorrectionWidget::smooth_tissues_pushed()
{
	size_t start_slice = handler3D->start_slice();
	size_t end_slice = handler3D->end_slice();

	if (smooth_tissues_params->_active_slice->isOn())
	{
		start_slice = handler3D->active_slice();
		end_slice = start_slice + 1;
	}

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.allSlices = !smooth_tissues_params->_active_slice->isOn();
	dataSelection.tissues = true;
	emit begin_datachange(dataSelection, this);

	SmoothTissues(handler3D, start_slice, end_slice,
			smooth_tissues_params->_sigma->text().toDouble(),
			smooth_tissues_params->_3D->isOn());

	emit end_datachange(this);
}

} // namespace iseg
