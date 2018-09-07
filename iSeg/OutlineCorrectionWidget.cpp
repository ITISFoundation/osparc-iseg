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

#include <QLabel>
#include <q3vbox.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qwidget.h>

using namespace iseg;

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

	hboxoverall = new Q3HBox(this);
	vboxmethods = new Q3VBox(hboxoverall);

	vbox1 = new Q3VBox(hboxoverall);
	hbox1 = new Q3HBox(vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox2a = new Q3HBox(vbox1);

	tissuesListBackground = new Q3HBox(vbox1);
	tissuesListBackground->setFixedHeight(18);
	tissuesListSkin = new Q3HBox(vbox1);
	tissuesListSkin->setFixedHeight(18);

	allslices = new QCheckBox(QString("Apply to all slices"), vbox1);
	pb_execute = new QPushButton("Fill Islands", vbox1);

	hbox3a = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);
	hbox5o = new Q3HBox(vbox1);
	hbox6 = new Q3HBox(vbox1);
	hbox5 = new Q3HBox(vbox1);
	hboxpixormm = new Q3HBox(vbox1);
	hbox_prev_slice = new Q3HBox(vbox1);

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

	method = new QButtonGroup(this);

	Q3HBox* hboxmethods1 = new Q3HBox(vboxmethods);
	Q3HBox* hboxmethods2 = new Q3HBox(vboxmethods);
	Q3HBox* hboxmethods3 = new Q3HBox(vboxmethods);
	//Q3HBox* hboxmethods4 = new Q3HBox(vboxmethods);

	olcorr = new QRadioButton(QString("Outline Corr"), hboxmethods1);
	olcorr->setToolTip(Format(
			"Draw an alternative boundary segment for a region.This segment "
			"will be connected to the region using the shortest possible lines "
			"and will replace the boundary segment between the connection points."));
	brush = new QRadioButton(QString("Brush"), hboxmethods1);
	brush->setToolTip(Format("Manual correction and segmentation tool: a brush."));
	holefill = new QRadioButton(QString("Fill Holes"), hboxmethods1);
	holefill->setToolTip(Format(
			"Close all holes in the selected region or tissue that have a "
			"size smaller than the number of pixels specified by the Hole Size option."));
	removeislands = new QRadioButton(QString("Remove Islands"), hboxmethods2);
	removeislands->setToolTip(Format(
			"Remove all islands (speckles and outliers) with the selected "
			"gray value or tissue assignment."));
	gapfill = new QRadioButton(QString("Fill Gaps"), hboxmethods2);
	gapfill->setToolTip(Format(
			"Close gaps between multiple disconnected regions having the same "
			"gray value or belonging to the same tissue."));
	addskin = new QRadioButton(QString("Add Skin"), hboxmethods2);
	addskin->setToolTip(Format(
			"Add a skin layer to a segmentation with a "
			"specified Thickness (in pixels)."));
	fillskin = new QRadioButton(QString("Fill Skin"), hboxmethods3);
	fillskin->setToolTip(Format("Thicken a skin layer to ensure it has a minimum Thickness."));
	allfill = new QRadioButton(QString("Fill All"), hboxmethods3);
	allfill->setToolTip(Format(
			"Make sure that there are no unassigned regions "
			"inside the segmented model."));
	adapt = new QRadioButton(QString("Adapt"), hboxmethods3);

	method->insert(olcorr);
	method->insert(brush);
	method->insert(holefill);
	method->insert(removeislands);
	method->insert(gapfill);
	method->insert(addskin);
	method->insert(fillskin);
	method->insert(allfill);
	method->insert(adapt);

	brush->setChecked(true);

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

	vboxmethods->setMargin(5);
	vbox1->setMargin(4);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vboxmethods->setLineWidth(1);
	vboxmethods->setMargin(4);

	method_changed();

	QObject::connect(method, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
	QObject::connect(target, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
	QObject::connect(allslices, SIGNAL(clicked()), this, SLOT(method_changed()));
	QObject::connect(pixelormm, SIGNAL(buttonClicked(int)), this, SLOT(pixmm_changed()));
	QObject::connect(pb_execute, SIGNAL(clicked()), this, SLOT(execute_pushed()));
	QObject::connect(pb_selectobj, SIGNAL(clicked()), this, SLOT(selectobj_pushed()));

	QObject::connect(cb_show_guide, SIGNAL(clicked()), this, SLOT(draw_guide()));
	QObject::connect(sb_guide_offset, SIGNAL(valueChanged(int)), this, SLOT(draw_guide()));
	QObject::connect(pb_copy_guide, SIGNAL(clicked()), this, SLOT(copy_guide()));
	QObject::connect(pb_copy_pick_guide, SIGNAL(clicked()), this, SLOT(copy_pick_pushed()));

	QObject::connect(getCurrentTissueBackground, SIGNAL(clicked()), this, SLOT(on_select_background()));
	QObject::connect(getCurrentTissueSkin, SIGNAL(clicked()), this, SLOT(on_select_skin()));

	selectobj = false;

	backgroundSelected = false;
	skinSelected = false;

	method_changed();
	workbits_changed();
}

OutlineCorrectionWidget::~OutlineCorrectionWidget()
{
	delete vbox1;
	delete method;
	delete target;
	delete brushtype;
}

void OutlineCorrectionWidget::on_select_background()
{
	auto sel = handler3D->tissue_selection();
	if (sel.size() == 1)
	{
		select_background(handler3D->tissue_names().at(sel[0]).c_str(), sel[0]);
	}
	else
	{
		ISEG_WARNING("Please select only one tissue");
	}
}

void OutlineCorrectionWidget::on_select_skin()
{
	auto sel = handler3D->tissue_selection();
	if (sel.size() == 1)
	{
		select_skin(handler3D->tissue_names().at(sel[0]).c_str(), sel[0]);
	}
	else
	{
		ISEG_WARNING("Please select only one tissue");
	}
}

void OutlineCorrectionWidget::select_background(QString tissueName, tissues_size_t nr)
{
	backgroundText->clear();
	backgroundText->setText(tissueName);

	selectedBackgroundID = nr;
	backgroundSelected = true;

	if (backgroundSelected && skinSelected)
		pb_execute->setEnabled(true);
	else
		pb_execute->setEnabled(false);
}

void OutlineCorrectionWidget::select_skin(QString tissueName, tissues_size_t nr)
{
	skinText->clear();
	skinText->setText(tissueName);

	selectedSkinID = nr;
	skinSelected = true;

	if (backgroundSelected && skinSelected)
		pb_execute->setEnabled(true);
	else
		pb_execute->setEnabled(false);
}

QSize OutlineCorrectionWidget::sizeHint() const { return vbox1->sizeHint(); }

void OutlineCorrectionWidget::draw_circle(Point p)
{
	Point p1;
	vpdyn.clear();
	if (mm->isOn())
	{
		float const radius = mm_radius->text().toFloat();
		float const radius_corrected =
				spacing[0] > spacing[1]
						? std::floor(radius / spacing[0] + 0.5f) * spacing[0]
						: std::floor(radius / spacing[1] + 0.5f) * spacing[1];
		float const radius_corrected2 = radius_corrected * radius_corrected;

		int const xradius = std::ceil(radius_corrected / spacing[0]);
		int const yradius = std::ceil(radius_corrected / spacing[1]);
		for (p1.px = std::max(0, p.px - xradius);
				 p1.px <= std::min((int)bmphand->return_width() - 1, p.px + xradius);
				 p1.px++)
		{
			for (p1.py = std::max(0, p.py - yradius);
					 p1.py <= std::min((int)bmphand->return_height() - 1, p.py + yradius);
					 p1.py++)
			{
				if (std::pow(spacing[0] * (p.px - p1.px), 2.f) + std::pow(spacing[1] * (p.py - p1.py), 2.f) <= radius_corrected2)
				{
					vpdyn.push_back(p1);
				}
			}
		}
	}
	else
	{
		int const xradius = sb_radius->value();
		int const yradius = sb_radius->value();
		int const radius2 = sb_radius->value() * sb_radius->value();
		for (p1.px = std::max(0, p.px - xradius);
				 p1.px <= std::min((int)bmphand->return_width() - 1, p.px + xradius);
				 p1.px++)
		{
			for (p1.py = std::max(0, p.py - yradius);
					 p1.py <= std::min((int)bmphand->return_height() - 1, p.py + yradius);
					 p1.py++)
			{
				if ((p.px - p1.px) * (p.px - p1.px) + (p.py - p1.py) * (p.py - p1.py) <= radius2)
				{
					vpdyn.push_back(p1);
				}
			}
		}
	}

	emit vpdyn_changed(&vpdyn);
	vpdyn.clear();
}

void OutlineCorrectionWidget::draw_guide()
{
	if (brush->isOn() && cb_show_guide->isChecked())
	{
		int slice = static_cast<int>(handler3D->active_slice()) + sb_guide_offset->value();
		unsigned slice_clamped = std::min(std::max(slice, 0), handler3D->num_slices() - 1);
		if (slice == slice_clamped)
		{
			std::vector<Mark> marks;
			auto w = handler3D->width();
			auto h = handler3D->height();
			if (work->isOn())
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
	int slice = static_cast<int>(handler3D->active_slice()) + sb_guide_offset->value();
	unsigned slice_clamped = std::min(std::max(slice, 0), handler3D->num_slices() - 1);
	if (slice == slice_clamped)
	{
		unsigned w = handler3D->width();
		unsigned h = handler3D->height();

		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = work->isOn();
		dataSelection.tissues = !work->isOn();
		emit begin_datachange(dataSelection, this);

		if (work->isOn())
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
			pb_copy_pick_guide->setDown(false);
		}
		else
		{
			auto v = bmphand->work_pt(p);
			object_value->setText(QString::number(v));
			pb_selectobj->setDown(false);
		}
		return;
	}

	float const f = get_object_value();

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = work->isOn();
	dataSelection.tissues = !work->isOn();

	if (olcorr->isOn())
	{
		vpdyn.clear();
		last_pt = p;
		vpdyn.push_back(p);
		emit begin_datachange(dataSelection, this);
	}
	else if (brush->isOn())
	{
		last_pt = p;

		if (modifybrush->isOn())
		{
			if (work->isOn())
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
		else if (drawbrush->isOn())
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
		if (work->isOn())
		{
			if (mm->isOn())
				bmphand->brush(f, p, mm_radius->text().toFloat(), spacing[0], spacing[1], draw);
			else
				bmphand->brush(f, p, sb_radius->value(), draw);
		}
		else
		{
			auto idx = handler3D->active_tissuelayer();
			if (mm->isOn())
				bmphand->brushtissue(idx, tissuenr, p, mm_radius->text().toFloat(), spacing[0], spacing[1], draw, tissuenrnew);
			else
				bmphand->brushtissue(idx, tissuenr, p, sb_radius->value(), draw, tissuenrnew);
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

		if (olcorr->isOn())
		{
			vpdyn.pop_back();
			addLine(&vpdyn, last_pt, p);
			last_pt = p;
			emit vpdyn_changed(&vpdyn);
		}
		else if (brush->isOn())
		{
			draw_circle(p);

			std::vector<Point> vps;
			vps.clear();
			addLine(&vps, last_pt, p);
			for (auto it = ++(vps.begin()); it != vps.end(); it++)
			{
				if (work->isOn())
				{
					if (mm->isOn())
						bmphand->brush(f, *it, mm_radius->text().toFloat(),
								spacing[0], spacing[1], draw);
					else
						bmphand->brush(f, *it, sb_radius->value(), draw);
				}
				else
				{
					auto idx = handler3D->active_tissuelayer();
					if (mm->isOn())
						bmphand->brushtissue(
								idx, tissuenr, *it, mm_radius->text().toFloat(),
								spacing[0], spacing[1], draw, tissuenrnew);
					else
						bmphand->brushtissue(idx, tissuenr, *it,
								sb_radius->value(), draw,
								tissuenrnew);
				}
			}
			int dummy1, dummy2;
			QRect rect;
			if (p.px < last_pt.px)
			{
				dummy1 = (int)p.px;
				dummy2 = (int)last_pt.px;
			}
			else
			{
				dummy1 = (int)last_pt.px;
				dummy2 = (int)p.px;
			}
			rect.setLeft(std::max(0, dummy1 - sb_radius->value()));
			rect.setRight(std::min((int)bmphand->return_width() - 1,
					dummy2 + sb_radius->value()));
			if (p.py < last_pt.py)
			{
				dummy1 = (int)p.py;
				dummy2 = (int)last_pt.py;
			}
			else
			{
				dummy1 = (int)last_pt.py;
				dummy2 = (int)p.py;
			}
			rect.setTop(std::max(0, dummy1 - sb_radius->value()));
			rect.setBottom(std::min((int)bmphand->return_height() - 1,
					dummy2 + sb_radius->value()));
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

		if (olcorr->isOn())
		{
			vpdyn.pop_back();
			addLine(&vpdyn, last_pt, p);
			if (work->isOn())
				bmphand->correct_outline(f, &vpdyn);
			else
				bmphand->correct_outlinetissue(handler3D->active_tissuelayer(), tissuenr, &vpdyn);
			emit end_datachange(this);

			vpdyn.clear();
			emit vpdyn_changed(&vpdyn);
		}
		else if (brush->isOn())
		{
			vpdyn.clear();
			addLine(&vpdyn, last_pt, p);
			for (auto it = ++(vpdyn.begin()); it != vpdyn.end(); it++)
			{
				if (work->isOn())
				{
					if (mm->isOn())
						bmphand->brush(f, *it, mm_radius->text().toFloat(),
								spacing[0], spacing[1], draw);
					else
						bmphand->brush(f, *it, sb_radius->value(), draw);
				}
				else
				{
					auto idx = handler3D->active_tissuelayer();
					if (mm->isOn())
						bmphand->brushtissue(
								idx, tissuenr, *it, mm_radius->text().toFloat(),
								spacing[0], spacing[1], draw, tissuenrnew);
					else
						bmphand->brushtissue(idx, tissuenr, *it,
								sb_radius->value(), draw,
								tissuenrnew);
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
	tissuesListBackground->hide();
	tissuesListSkin->hide();
	pb_execute->setEnabled(true);
	selectobj = false; // make sure we are not expecting a mouse click
	copy_mode = false; // ensure this is reset

	if (olcorr->isOn())
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
	else if (brush->isOn())
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
	else if (holefill->isOn())
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
	else if (removeislands->isOn())
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
	else if (gapfill->isOn())
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
	else if (addskin->isOn())
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
	else if (fillskin->isOn())
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
	else if (allfill->isOn())
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
	else if (adapt->isOn())
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
	pixmm_changed();
	draw_guide();
}

void OutlineCorrectionWidget::execute_pushed()
{
	float const f = get_object_value();

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = work->isOn() || adapt->isOn();
	dataSelection.tissues = !(work->isOn() || adapt->isOn());
	emit begin_datachange(dataSelection, this);

	if (holefill->isOn())
	{
		if (allslices->isChecked())
		{
			if (work->isOn())
				handler3D->fill_holes(f, sb_holesize->value());
			else
				handler3D->fill_holestissue(tissuenr, sb_holesize->value());
		}
		else
		{
			if (work->isOn())
				bmphand->fill_holes(f, sb_holesize->value());
			else
				bmphand->fill_holestissue(handler3D->active_tissuelayer(), tissuenr, sb_holesize->value());
		}
	}
	else if (removeislands->isOn())
	{
		if (allslices->isChecked())
		{
			if (work->isOn())
				handler3D->remove_islands(f, sb_holesize->value());
			else
				handler3D->remove_islandstissue(tissuenr, sb_holesize->value());
		}
		else
		{
			if (work->isOn())
				bmphand->remove_islands(f, sb_holesize->value());
			else
				bmphand->remove_islandstissue(handler3D->active_tissuelayer(), tissuenr, sb_holesize->value());
		}
	}
	else if (gapfill->isOn())
	{
		if (allslices->isChecked())
		{
			if (work->isOn())
				handler3D->fill_gaps(sb_gapsize->value(), false);
			else
				handler3D->fill_gapstissue(sb_gapsize->value(), false);
		}
		else
		{
			if (work->isOn())
				bmphand->fill_gaps(sb_gapsize->value(), false);
			else
				bmphand->fill_gapstissue(handler3D->active_tissuelayer(), sb_gapsize->value(), false);
		}
	}
	else if (addskin->isOn())
	{
		float mm_rad = mm_radius->text().toFloat();
		int const rx = mm->isOn() ? static_cast<int>(mm_rad / spacing[0] + 0.1f) : sb_radius->value();
		int const ry = mm->isOn() ? static_cast<int>(mm_rad / spacing[1] + 0.1f) : sb_radius->value();
		int const rz = mm->isOn() ? static_cast<int>(mm_rad / spacing[2] + 0.1f) : sb_radius->value();
		bool atBoundary = false;
		if (outside->isChecked())
		{
			if (allslices->isChecked())
			{
				if (mm->isOn())
				{
					// \warning creates block shaped kernel, instead of ellipsoid
					if (work->isOn())
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
					if (work->isOn())
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
				if (work->isOn())
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
			if (allslices->isChecked())
			{
				if (mm->isOn())
				{
					// \warning creates block shaped kernel, instead of ellipsoid
					float mm_rad = mm_radius->text().toFloat();
					if (work->isOn())
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
					if (work->isOn())
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
				if (mm->isOn())
				{
					// \warning if spacing is anisotropic, skin thickness will be non-uniform
					if (work->isOn())
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
					if (work->isOn())
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
	else if (fillskin->isOn())
	{
		float mm_rad = mm_radius->text().toFloat();
		int const xThick = mm->isOn() ? static_cast<int>(mm_rad / spacing[0] + 0.1f) : sb_radius->value();
		int const yThick = mm->isOn() ? static_cast<int>(mm_rad / spacing[1] + 0.1f) : sb_radius->value();
		int const zThick = mm->isOn() ? static_cast<int>(mm_rad / spacing[2] + 0.1f) : sb_radius->value();

		if (allslices->isChecked())
			handler3D->fill_skin_3d(xThick, yThick, zThick, selectedBackgroundID, selectedSkinID);
		else
			bmphand->fill_skin(xThick, yThick, selectedBackgroundID, selectedSkinID);
	}
	else if (allfill->isOn())
	{
		if (allslices->isChecked())
		{
			if (work->isOn())
				handler3D->fill_unassigned();
			else
				handler3D->fill_unassignedtissue(tissuenr);
		}
		else
		{
			if (work->isOn())
				bmphand->fill_unassigned();
			else
				bmphand->fill_unassignedtissue(handler3D->active_tissuelayer(), tissuenr);
		}
	}
	else if (adapt->isOn())
	{
		if (allslices->isChecked())
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
	pb_selectobj->setDown(true);
}

void OutlineCorrectionWidget::copy_pick_pushed()
{
	selectobj = true;
	copy_mode = true;
	pb_copy_pick_guide->setDown(true);
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
			object_value->setText(QString::number(p.high));
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
		dummy = sb_radius->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_holesize->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_gapsize->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(brush->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(olcorr->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(holefill->isOn() || removeislands->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(gapfill->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allfill->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(addskin->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(fillskin->isOn());
		//fwrite(&(dummy), 1,sizeof(int), fp);
		dummy = (int)(tissue->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(work->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(erasebrush->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(drawbrush->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(modifybrush->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(adapt->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(inside->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(outside->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allslices->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* OutlineCorrectionWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(method, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
		QObject::disconnect(target, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sb_radius->setValue(dummy);
		mm_radius->setText(QString(dummy));
		fread(&dummy, sizeof(int), 1, fp);
		sb_holesize->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_gapsize->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		brush->setChecked(dummy > 0);
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
		tissue->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		work->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		erasebrush->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		drawbrush->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		modifybrush->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		adapt->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		inside->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		outside->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		allslices->setChecked(dummy > 0);

		method_changed();

		QObject::connect(method, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
		QObject::connect(target, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
	}
	return fp;
}

void OutlineCorrectionWidget::hideparams_changed() { method_changed(); }

void OutlineCorrectionWidget::pixmm_changed()
{
	bool add_skin_mm = mm->isOn() && addskin->isOn() && allslices->isChecked();
	bool brush_mm = mm->isOn() && brush->isOn();
	if (add_skin_mm || brush_mm)
	{
		mm_radius->show();
		sb_radius->hide();
		txt_unit->setText(" mm");
	}
	else
	{
		sb_radius->show();
		mm_radius->hide();
		txt_unit->setText(" pixel");
	}
}

float OutlineCorrectionWidget::get_object_value() const
{
	bool ok = false;
	float f = object_value->text().toFloat(&ok);
	if (!ok)
	{
		f = 255.f;
	}
	return f;
}
