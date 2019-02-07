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
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollArea>
#include <QStackedLayout>

namespace iseg {

std::map<QAbstractButton*, int> _widget_page;

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
	olcorr = new QRadioButton(tr("Outline Corr"));
	olcorr->setToolTip(Format(
			"Draw an alternative boundary segment for a region.This segment "
			"will be connected to the region using the shortest possible lines "
			"and will replace the boundary segment between the connection points."));
	brush = new QRadioButton(tr("Brush"));
	brush->setToolTip(Format("Manual correction and segmentation tool: a brush."));

	holefill = new QRadioButton(tr("Fill Holes"));
	holefill->setToolTip(Format(
			"Close all holes in the selected region or tissue that have a "
			"size smaller than the number of pixels specified by the Hole Size option."));

	removeislands = new QRadioButton(tr("Remove Islands"));
	removeislands->setToolTip(Format(
			"Remove all islands (speckles and outliers) with the selected "
			"gray value or tissue assignment."));

	gapfill = new QRadioButton(tr("Fill Gaps"));
	gapfill->setToolTip(Format(
			"Close gaps between multiple disconnected regions having the same "
			"gray value or belonging to the same tissue."));

	addskin = new QRadioButton(tr("Add Skin"));
	addskin->setToolTip(Format(
			"Add a skin layer to a segmentation with a "
			"specified Thickness (in pixels)."));

	fillskin = new QRadioButton(tr("Fill Skin"));
	fillskin->setToolTip(Format("Thicken a skin layer to ensure it has a minimum Thickness."));

	allfill = new QRadioButton(tr("Fill All"));
	allfill->setToolTip(Format(
			"Make sure that there are no unassigned regions "
			"inside the segmented model."));

	adapt = new QRadioButton(tr("Adapt"));

	smooth_tissues = new QRadioButton(tr("Smooth Tissues"));

	auto method_radio_buttons = {olcorr, brush, holefill, removeislands, gapfill, addskin, fillskin, allfill, adapt, smooth_tissues};
	methods = new QButtonGroup(parent);
	for (auto w : method_radio_buttons)
	{
		methods->addButton(w);
	}

	auto method_vbox = new QVBoxLayout;
	for (auto w : method_radio_buttons)
	{
		method_vbox->addWidget(w);
	}
	method_vbox->setMargin(5);

	auto method_area = new QFrame;
	method_area->setLayout(method_vbox);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);

	auto scroll_area = new QScrollArea(this);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll_area->setWidget(method_area);
	scroll_area->setFrameStyle(QFrame::NoFrame | QFrame::Plain);

	// parameter views
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

	top_layout->addWidget(scroll_area);
	top_layout->addWidget(parameter_area);
	setLayout(top_layout);

	// remember QStackedLayout page where parameters of method are added
	for (int i = 0; i < methods->buttons().size(); ++i)
	{
		_widget_page[methods->buttons().at(i)] = i;
	}

	// start with brush tool
	brush->setChecked(true);
	stacked_param_layout->setCurrentWidget(brush_params);

	// create connections
	connect(methods, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
	connect(olc_params->_select_object, SIGNAL(clicked()), this, SLOT(selectobj_pushed()));
	connect(brush_params->_select_object, SIGNAL(clicked()), this, SLOT(selectobj_pushed()));
	connect(fill_holes_params->_select_object, SIGNAL(clicked()), this, SLOT(selectobj_pushed()));
	connect(remove_islands_params->_select_object, SIGNAL(clicked()), this, SLOT(selectobj_pushed()));
	connect(fill_gaps_params->_select_object, SIGNAL(clicked()), this, SLOT(selectobj_pushed()));
	connect(adapt_params->_select_object, SIGNAL(clicked()), this, SLOT(selectobj_pushed()));

	connect(brush_params->_copy_guide, SIGNAL(clicked()), this, SLOT(copy_guide()));
	connect(brush_params->_copy_pick_guide, SIGNAL(clicked()), this, SLOT(copy_pick_pushed()));
	connect(brush_params->_show_guide, SIGNAL(clicked()), this, SLOT(draw_guide()));
	connect(brush_params->_guide_offset, SIGNAL(valueChanged(int)), this, SLOT(draw_guide()));

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
	if (brush->isChecked() && brush_params->_show_guide->isChecked())
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

	if (olcorr->isChecked())
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
	else if (brush->isChecked())
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

		if (olcorr->isChecked())
		{
			vpdyn.pop_back();
			addLine(&vpdyn, last_pt, p);
			last_pt = p;
			emit vpdyn_changed(&vpdyn);
		}
		else if (brush->isChecked())
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

		if (olcorr->isChecked())
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
		else if (brush->isChecked())
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
	if (_widget_page.count(methods->checkedButton()))
	{
		stacked_param_layout->setCurrentIndex(_widget_page[methods->checkedButton()]);
	}

	// keep selected object definition across tools
	auto new_widget = dynamic_cast<ParamViewBase*>(stacked_param_layout->currentWidget());
	if (current_params)
	{
		new_widget->set_work(current_params->work());
		new_widget->set_object_value(current_params->object_value());
	}
	current_params = new_widget;
	current_params->init();

	// make sure we are not expecting a mouse click
	selectobj = false;
	// ensure this is reset
	copy_mode = false;

	draw_guide();
}

void OutlineCorrectionWidget::execute_pushed()
{
	float const f = get_object_value();

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();

	if (holefill->isChecked())
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
	else if (removeislands->isChecked())
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
	else if (gapfill->isChecked())
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
	else if (addskin->isChecked())
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
	else if (fillskin->isChecked())
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
	else if (allfill->isChecked())
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
	else if (adapt->isChecked())
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
		dummy = (int)(brush->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(olcorr->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(holefill->isChecked() || removeislands->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(gapfill->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allfill->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(addskin->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(fillskin->isChecked());
		//fwrite(&(dummy), 1,sizeof(int), fp);
		bool w = current_params->work();
		dummy = (int)(!w);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(w);
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(brush_params->_erase->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(brush_params->_draw->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(brush_params->_modify->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(adapt->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(add_skin_params->_inside->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(add_skin_params->_outside->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(fill_holes_params->_all_slices->isChecked());
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
		brush->setChecked(dummy != 0);
		fread(&dummy, sizeof(int), 1, fp);
		olcorr->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		holefill->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		gapfill->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		allfill->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		addskin->setChecked(dummy > 0);

		//fread(&dummy,sizeof(int), 1, fp);
		fillskin->setChecked(false);

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
		adapt->setChecked(dummy > 0);
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
