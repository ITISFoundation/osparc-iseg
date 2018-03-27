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
#include "outlinecorrection.h"

#include "Core/Point.h"
#include "Core/addLine.h"

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

using namespace std;
using namespace iseg;

OutlineCorr_widget::OutlineCorr_widget(SlicesHandler* hand3D, QWidget* parent,
									   const char* name, Qt::WindowFlags wFlags)
	: QWidget1(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("OutLine Correction routines that can be used to modify "
					  "the result of a "
					  "segmentation operation and to correct frequently "
					  "occurring segmentation "
					  "deficiencies."));

	f = 255;
	activeslice = handler3D->get_activeslice();
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
	pb_removeholes = new QPushButton("Fill Islands", vbox1);

	hbox3a = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);
	hbox5o = new Q3HBox(vbox1);
	hbox6 = new Q3HBox(vbox1);
	hbox5 = new Q3HBox(vbox1);
	hboxpixormm = new Q3HBox(vbox1);
	Q3HBox* dummyBox = new Q3HBox(vbox1);

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
	Q3HBox* hboxmethods4 = new Q3HBox(vboxmethods);

	olcorr = new QRadioButton(QString("Outline Corr"), hboxmethods1);
	olcorr->setToolTip(
		Format("Draw an alternative boundary segment for a region.This segment "
			   "will be connected to the region using the "
			   "shortest possible lines and will replace the boundary segment "
			   "between the connection points."));
	brush = new QRadioButton(QString("Brush"), hboxmethods1);
	brush->setToolTip(
		Format("Manual correction and segmentation tool: a brush."));
	holefill = new QRadioButton(QString("Fill Holes"), hboxmethods1);
	holefill->setToolTip(
		Format("Close all holes in the selected region or tissue that have a "
			   "size smaller than the number of "
			   "pixels specified by the Hole Size option."));
	removeislands = new QRadioButton(QString("Remove Islands"), hboxmethods2);
	removeislands->setToolTip(
		Format("Remove all islands (speckles and outliers) with the selected "
			   "gray value or tissue assignment."));
	gapfill = new QRadioButton(QString("Fill Gaps"), hboxmethods2);
	gapfill->setToolTip(Format(
		"Close gaps between multiple disconnected regions having the same "
		"gray value or belonging to the same tissue."));
	addskin = new QRadioButton(QString("Add Skin"), hboxmethods2);
	addskin->setToolTip(Format("Add a skin layer to a segmentation with a "
							   "specified Thickness (in pixels)."));
	fillskin = new QRadioButton(QString("Fill Skin"), hboxmethods3);
	fillskin->setToolTip(
		Format("Thicken a skin layer to ensure it has a minimum Thickness."));
	allfill = new QRadioButton(QString("Fill All"), hboxmethods3);
	allfill->setToolTip(Format("Make sure that there are no unassigned regions "
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
		"The Modify mode depends on where the mouse is initially pressed "
		"down. If it is pressed down within the selected region or tissue, it "
		"acts "
		"as a drawing brush. In contrast, when it is pressed down outside, it "
		"will overwrite the selected region or tissue with the gray value or "
		"tissue "
		"assignment of the point where the mouse has initially been pressed "
		"down."));
	drawbrush = new QRadioButton(QString("Draw"), hbox3a);
	drawbrush->setToolTip(
		Format("Use the brush to draw with the currently selected gray "
			   "value(TargetPict) or tissue assignment (Tissue)."));
	erasebrush = new QRadioButton(QString("Erase"), hbox3a);
	erasebrush->setToolTip(
		Format("Erase a region leaving black or no tissue assignment behind."));

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
	pb_selectobj->show();
	pb_selectobj->setToolTip(
		"Press this button and select the object in the target image. This "
		"object will be considered as foreground by the current operation.");
	fb = new QCheckBox(QString("Foreground"), hbox6);
	bg = new QCheckBox(QString("Background"), hbox6);

	in_or_out = new QButtonGroup(this);
	//	in_or_out->hide();
	inside = new QRadioButton(QString("Inside"), hbox5);
	outside = new QRadioButton(QString("Outside"), hbox5);
	in_or_out->insert(inside);
	in_or_out->insert(outside);
	inside->show();
	inside->setChecked(TRUE);
	outside->show();

	pixelormm = new QButtonGroup(this);
	//	target->hide();
	pixel = new QRadioButton(QString("pixel"), hboxpixormm);
	mm = new QRadioButton(QString("mm"), hboxpixormm);
	pixelormm->insert(pixel);
	pixelormm->insert(mm);
	pixel->show();
	pixel->setChecked(TRUE);
	mm->show();

	vboxmethods->setMargin(5);
	vbox1->setMargin(1);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vboxmethods->setLineWidth(1);

	hbox1->setFixedSize(hbox1->sizeHint());
	hbox2->setFixedSize(hbox2->sizeHint());
	hbox2a->setFixedSize(hbox2a->sizeHint());
	//	hbox3->setFixedSize(hbox3->sizeHint());
	//	hbox3cont->setFixedSize(hbox3cont->sizeHint());
	hbox3a->setFixedSize(hbox3a->sizeHint());
	hbox4->setFixedSize(hbox4->sizeHint());
	hbox5->setFixedSize(hbox5->sizeHint());
	hboxpixormm->setFixedSize(hboxpixormm->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint());
	// 	vboxmethods->setFixedSize(vboxmethods->sizeHint());
	// 	hboxoverall->setFixedSize(hboxoverall->sizeHint());//	setFixedSize(vbox1->size());

	method_changed();

	QObject::connect(method, SIGNAL(buttonClicked(int)), this,
					 SLOT(method_changed()));
	QObject::connect(target, SIGNAL(buttonClicked(int)), this,
					 SLOT(method_changed()));
	QObject::connect(allslices, SIGNAL(clicked()), this,
					 SLOT(method_changed()));
	QObject::connect(pixelormm, SIGNAL(buttonClicked(int)), this,
					 SLOT(pixmm_changed()));
	QObject::connect(pb_removeholes, SIGNAL(clicked()), this,
					 SLOT(removeholes_pushed()));
	QObject::connect(pb_selectobj, SIGNAL(clicked()), this,
					 SLOT(selectobj_pushed()));

	QObject::connect(getCurrentTissueBackground, SIGNAL(clicked()), this,
					 SLOT(request_selected_tissue_BG()));
	QObject::connect(getCurrentTissueSkin, SIGNAL(clicked()), this,
					 SLOT(request_selected_tissue_TS()));

	selectobj = false;

	backgroundSelected = false;
	skinSelected = false;

	method_changed();
	workbits_changed();
}

OutlineCorr_widget::~OutlineCorr_widget()
{
	delete vbox1;
	delete method;
	delete target;
	delete brushtype;
}

void OutlineCorr_widget::request_selected_tissue_BG()
{
	emit signal_request_selected_tissue_BG();
}

void OutlineCorr_widget::request_selected_tissue_TS()
{
	emit signal_request_selected_tissue_TS();
}

void OutlineCorr_widget::Select_selected_tissue_BG(QString tissueName,
												   tissues_size_t nr)
{
	backgroundText->clear();
	backgroundText->setText(tissueName);

	selectedBacgroundID = nr;
	backgroundSelected = true;

	if (backgroundSelected && skinSelected)
		pb_removeholes->setEnabled(true);
	else
		pb_removeholes->setEnabled(false);
}

void OutlineCorr_widget::Select_selected_tissue_TS(QString tissueName,
												   tissues_size_t nr)
{
	skinText->clear();
	skinText->setText(tissueName);

	selectedSkinID = nr;
	skinSelected = true;

	if (backgroundSelected && skinSelected)
		pb_removeholes->setEnabled(true);
	else
		pb_removeholes->setEnabled(false);
}

QSize OutlineCorr_widget::sizeHint() const { return vbox1->sizeHint(); }

void OutlineCorr_widget::draw_circle(Point p)
{
	Point p1;
	vpdyn.clear();
	if (mm->isOn())
	{
		float const radius = mm_radius->text().toFloat();
		float const radius_corrected =
			spacing[0] > spacing[1]
				? std::ceil(radius / spacing[0] + 0.5f) * spacing[0]
				: std::ceil(radius / spacing[1] + 0.5f) * spacing[1];

		int const xradius = std::ceil(radius_corrected / spacing[0]);
		int const yradius = std::ceil(radius_corrected / spacing[1]);
		for (p1.px = max(0, p.px - xradius);
			 p1.px <= min((int)bmphand->return_width() - 1, p.px + xradius);
			 p1.px++)
		{
			for (p1.py = max(0, p.py - yradius);
				 p1.py <=
				 min((int)bmphand->return_height() - 1, p.py + yradius);
				 p1.py++)
			{
				if (std::pow(spacing[0] * (p.px - p1.px), 2.f) +
						std::pow(spacing[1] * (p.py - p1.py), 2.f) <=
					radius_corrected * radius_corrected)
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
		for (p1.px = max(0, p.px - xradius);
			 p1.px <= min((int)bmphand->return_width() - 1, p.px + xradius);
			 p1.px++)
		{
			for (p1.py = max(0, p.py - yradius);
				 p1.py <= min((int)bmphand->return_width() - 1, p.py + yradius);
				 p1.py++)
			{
				if ((p.px - p1.px) * (p.px - p1.px) +
						(p.py - p1.py) * (p.py - p1.py) <=
					sb_radius->value() * sb_radius->value())
				{
					vpdyn.push_back(p1);
				}
			}
		}
	}

	emit vpdyn_changed(&vpdyn);
	vpdyn.clear();
}

void OutlineCorr_widget::mouse_clicked(Point p)
{
	if (selectobj)
	{
		f = bmphand->work_pt(p);
		pb_selectobj->setDown(false);
		return;
	}

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
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
				if (bmphand->tissues_pt(handler3D->get_active_tissuelayer(),
										p) == tissuenr)
				{
					draw = true;
				}
				else
				{
					draw = false;
					tissuenrnew = bmphand->tissues_pt(
						handler3D->get_active_tissuelayer(), p);
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
			if (bmphand->tissues_pt(handler3D->get_active_tissuelayer(), p) ==
				tissuenr)
				tissuenrnew = 0;
			else
				tissuenrnew =
					bmphand->tissues_pt(handler3D->get_active_tissuelayer(), p);
		}

		emit begin_datachange(dataSelection, this);
		if (work->isOn())
		{
			if (fb->isChecked())
				f = 5000; // \hack by SK, remove
			if (bg->isChecked())
				f = -5000; // \hack by SK, remove

			if (mm->isOn())
				bmphand->brush(f, p, mm_radius->text().toFloat(), spacing[0],
							   spacing[1], draw);
			else
				bmphand->brush(f, p, sb_radius->value(), draw);
		}
		else
		{
			auto idx = handler3D->get_active_tissuelayer();
			if (mm->isOn())
				bmphand->brushtissue(idx, tissuenr, p,
									 mm_radius->text().toFloat(), spacing[0],
									 spacing[1], draw, tissuenrnew);
			else
				bmphand->brushtissue(idx, tissuenr, p, sb_radius->value(), draw,
									 tissuenrnew);
		}
		emit end_datachange(this, iseg::NoUndo);

		draw_circle(p);
	}
}

void OutlineCorr_widget::mouse_moved(Point p)
{
	if (!selectobj)
	{
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

			vector<Point> vps;
			vps.clear();
			addLine(&vps, last_pt, p);
			for (vector<Point>::iterator it = ++(vps.begin()); it != vps.end();
				 it++)
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
					auto idx = handler3D->get_active_tissuelayer();
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
			rect.setLeft(max(0, dummy1 - sb_radius->value()));
			rect.setRight(min((int)bmphand->return_width() - 1,
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
			rect.setTop(max(0, dummy1 - sb_radius->value()));
			rect.setBottom(min((int)bmphand->return_height() - 1,
							   dummy2 + sb_radius->value()));
			emit end_datachange(rect, this, iseg::NoUndo);
			last_pt = p;
		}
	}
}

void OutlineCorr_widget::mouse_released(Point p)
{
	if (selectobj)
	{
		selectobj = false;
	}
	else
	{
		if (olcorr->isOn())
		{
			vpdyn.pop_back();
			addLine(&vpdyn, last_pt, p);
			if (work->isOn())
				bmphand->correct_outline(f, &vpdyn);
			else
				bmphand->correct_outlinetissue(
					handler3D->get_active_tissuelayer(), tissuenr, &vpdyn);
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
					auto idx = handler3D->get_active_tissuelayer();
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

void OutlineCorr_widget::method_changed()
{
	tissuesListBackground->hide();
	tissuesListSkin->hide();
	pb_removeholes->setEnabled(true);

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
		pb_removeholes->hide();
		if (work->isOn())
		{
			pb_selectobj->show();
			bg->hide();
			fb->hide();
		}
		else
		{
			pb_selectobj->hide();
			bg->hide();
			fb->hide();
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
		allslices->hide();
		pb_removeholes->hide();
		if (work->isOn())
		{
			pb_selectobj->show();
			bg->show();
			fb->show();
		}
		else
		{
			pb_selectobj->hide();
			bg->hide();
			fb->hide();
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
		pb_removeholes->setText("Fill Holes");
		pb_removeholes->show();
		if (work->isOn())
		{
			pb_selectobj->show();
			bg->hide();
			fb->hide();
		}
		else
		{
			pb_selectobj->hide();
			bg->hide();
			fb->hide();
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
		pb_removeholes->setText("Remove Islands");
		pb_removeholes->show();
		if (work->isOn())
		{
			pb_selectobj->show();
			bg->hide();
			fb->hide();
		}
		else
		{
			pb_selectobj->hide();
			bg->hide();
			fb->hide();
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
		pb_removeholes->setText("Fill Gaps");
		pb_removeholes->show();
		pb_selectobj->hide();
		fb->hide();
		bg->hide();
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
		pb_removeholes->setText("Add Skin");
		pb_removeholes->show();
		pb_selectobj->hide();
		fb->hide();
		bg->hide();
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
		pb_removeholes->setText("Fill Skin");
		pb_removeholes->show();
		if (backgroundSelected && skinSelected)
			pb_removeholes->setEnabled(true);
		else
			pb_removeholes->setEnabled(false);
		pb_selectobj->hide();
		fb->hide();
		bg->hide();
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
		pb_removeholes->setText("Fill All");
		pb_removeholes->show();
		pb_selectobj->hide();
		fb->hide();
		bg->hide();
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
		pb_removeholes->setText("Adapt");
		pb_removeholes->show();
		pb_selectobj->show();
		fb->hide();
		bg->hide();
	}
	pixmm_changed();
}

void OutlineCorr_widget::removeholes_pushed()
{
	//	bmphand->fill_holes(255.0f,sb_holesize->value());

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->get_activeslice();
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
				bmphand->fill_holestissue(handler3D->get_active_tissuelayer(),
										  tissuenr, sb_holesize->value());
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
				bmphand->remove_islandstissue(
					handler3D->get_active_tissuelayer(), tissuenr,
					sb_holesize->value());
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
				bmphand->fill_gapstissue(handler3D->get_active_tissuelayer(),
										 sb_gapsize->value(), false);
		}
	}
	else if (addskin->isOn())
	{
		bool atBoundary = false;
		if (outside->isChecked())
		{
			if (allslices->isChecked())
			{
				//if(work->isOn()) handler3D->add_skin(sb_radius->value());xxxa
				if (mm->isOn())
				{
					float thick1 = handler3D->get_slicethickness();
					Pair ps1 = handler3D->get_pixelsize();
					if (work->isOn())
					{
						float setto = handler3D->add_skin3D_outside2(
							int(sb_radius->value() / ps1.high + 0.1f),
							int(sb_radius->value() / ps1.low + 0.1f),
							int(sb_radius->value() / thick1 + 0.1f));
						atBoundary = handler3D->value_at_boundary3D(setto);
					}
					else
					{
						handler3D->add_skintissue3D_outside2(
							int(sb_radius->value() / ps1.high + 0.1f),
							int(sb_radius->value() / ps1.low + 0.1f),
							int(sb_radius->value() / thick1 + 0.1f), tissuenr);
						atBoundary =
							handler3D->tissuevalue_at_boundary3D(tissuenr);
					}
				}
				else
				{
					if (work->isOn())
					{
						float setto =
							handler3D->add_skin3D_outside(sb_radius->value());
						atBoundary = handler3D->value_at_boundary3D(setto);
					}
					else
					{
						handler3D->add_skintissue3D_outside(sb_radius->value(),
															tissuenr);
						atBoundary =
							handler3D->tissuevalue_at_boundary3D(tissuenr);
					}
				}
			}
			else
			{
				if (work->isOn())
				{
					float setto = bmphand->add_skin_outside(sb_radius->value());
					atBoundary = bmphand->value_at_boundary(setto);
				}
				else
				{
					bmphand->add_skintissue_outside(
						handler3D->get_active_tissuelayer(), sb_radius->value(),
						tissuenr);
					atBoundary = bmphand->tissuevalue_at_boundary(
						handler3D->get_active_tissuelayer(), tissuenr);
				}
			}
		}
		else
		{
			if (allslices->isChecked())
			{
				//if(work->isOn()) handler3D->add_skin(sb_radius->value());xxxa
				if (mm->isOn())
				{
					float mm_rad = mm_radius->text().toFloat();
					float thick1 = handler3D->get_slicethickness();
					Pair ps1 = handler3D->get_pixelsize();
					if (work->isOn())
					{
						//convert mm into pixels
						float setto =
							handler3D->add_skin3D(int(mm_rad / ps1.high + 0.1f),
												  int(mm_rad / ps1.low + 0.1f),
												  int(mm_rad / thick1 + 0.1f));
						atBoundary = handler3D->value_at_boundary3D(setto);
					}
					else
					{
						handler3D->add_skintissue3D(
							int(mm_rad / ps1.high + 0.1f),
							int(mm_rad / ps1.low + 0.1f),
							int(mm_rad / thick1 + 0.1f), tissuenr);
						atBoundary =
							handler3D->tissuevalue_at_boundary3D(tissuenr);
					}
				}
				else
				{
					if (work->isOn())
					{
						float setto = handler3D->add_skin3D(sb_radius->value());
						atBoundary = handler3D->value_at_boundary3D(setto);
					}
					else
					{
						handler3D->add_skintissue3D(
							sb_radius->value(), sb_radius->value(),
							sb_radius->value(), tissuenr);
						atBoundary =
							handler3D->tissuevalue_at_boundary3D(tissuenr);
					}
				}
			}
			else
			{
				if (mm->isOn())
				{
					float mm_rad = mm_radius->text().toFloat();
					Pair ps1 = handler3D->get_pixelsize();
					if (work->isOn())
					{
						float setto =
							bmphand->add_skin(int(mm_rad / ps1.high + 0.1f));
						atBoundary = bmphand->value_at_boundary(setto);
					}
					else
					{
						bmphand->add_skintissue(
							handler3D->get_active_tissuelayer(),
							int(mm_rad / ps1.high + 0.1f), tissuenr);
						atBoundary = bmphand->tissuevalue_at_boundary(
							handler3D->get_active_tissuelayer(), tissuenr);
					}
				}
				else
				{
					if (work->isOn())
					{
						float setto = bmphand->add_skin(sb_radius->value());
						atBoundary = bmphand->value_at_boundary(setto);
					}
					else
					{
						bmphand->add_skintissue(
							handler3D->get_active_tissuelayer(),
							sb_radius->value(), tissuenr);
						atBoundary = bmphand->tissuevalue_at_boundary(
							handler3D->get_active_tissuelayer(), tissuenr);
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
		int xThick, yThick, zThick;
		if (mm->isOn())
		{
			float thickZ = handler3D->get_slicethickness();
			Pair ps1 = handler3D->get_pixelsize();
			xThick = sb_radius->value() / ps1.high + 0.1f;
			yThick = sb_radius->value() / ps1.low + 0.1f;
			zThick = sb_radius->value() / thickZ + 0.1f;
		}
		else
		{
			xThick = sb_radius->value();
			yThick = sb_radius->value();
			zThick = sb_radius->value();
		}

		if (allslices->isChecked())
			handler3D->fill_skin_3d(xThick, yThick, zThick, selectedBacgroundID,
									selectedSkinID);
		else
			bmphand->fill_skin(xThick, yThick, selectedBacgroundID,
							   selectedSkinID);
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
				bmphand->fill_unassignedtissue(
					handler3D->get_active_tissuelayer(), tissuenr);
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

void OutlineCorr_widget::selectobj_pushed()
{
	selectobj = true;
	pb_selectobj->setDown(true);
}

void OutlineCorr_widget::workbits_changed()
{
	bmphand = handler3D->get_activebmphandler();
	float* workbits = bmphand->return_work();
	unsigned area =
		unsigned(bmphand->return_width()) * bmphand->return_height();
	unsigned i = 0;
	while (i < area && workbits[i] != f)
		i++;
	if (i == area)
	{
		Pair p;
		bmphand->get_range(&p);
		f = p.high;
	}
}

void OutlineCorr_widget::slicenr_changed()
{
	if (activeslice != handler3D->get_activeslice())
	{
		activeslice = handler3D->get_activeslice();
		bmphand_changed(handler3D->get_activebmphandler());
	}
	else
		workbits_changed();
}

void OutlineCorr_widget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;

	handler3D->get_spacing(spacing);

	workbits_changed();
}

void OutlineCorr_widget::init()
{
	slicenr_changed();
	hideparams_changed();
}

void OutlineCorr_widget::newloaded()
{
	handler3D->get_spacing(spacing);
	activeslice = handler3D->get_activeslice();
	bmphand = handler3D->get_activebmphandler();
}

void OutlineCorr_widget::tissuenr_changed(int tissuenr1)
{
	tissuenr = (tissues_size_t)(tissuenr1 + 1);
}

void OutlineCorr_widget::cleanup()
{
	vpdyn.clear();
	emit vpdyn_changed(&vpdyn);
}

FILE* OutlineCorr_widget::SaveParams(FILE* fp, int version)
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

FILE* OutlineCorr_widget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(method, SIGNAL(buttonClicked(int)), this,
							SLOT(method_changed()));
		QObject::disconnect(target, SIGNAL(buttonClicked(int)), this,
							SLOT(method_changed()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sb_radius->setValue(dummy);
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

		QObject::connect(method, SIGNAL(buttonClicked(int)), this,
						 SLOT(method_changed()));
		QObject::connect(target, SIGNAL(buttonClicked(int)), this,
						 SLOT(method_changed()));
	}
	return fp;
}

void OutlineCorr_widget::hideparams_changed() { method_changed(); }

void OutlineCorr_widget::pixmm_changed()
{
	bool add_skin_mm = mm->isOn() && addskin->isOn() && allslices->isChecked();
	bool brush_mm = mm->isOn() && brush->isOn() && allslices->isChecked();
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
