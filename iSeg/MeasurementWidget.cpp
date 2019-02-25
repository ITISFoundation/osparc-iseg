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

#include "MeasurementWidget.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"
#include "StdStringToQString.h"

#include "Data/Point.h"
#include "Data/addLine.h"

#include "Interface/LayoutTools.h"

#include "Core/Pair.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QStackedWidget>

#include <fstream>
#include <initializer_list>

using namespace iseg;

MeasurementWidget::MeasurementWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"Perform different types of distance, angle or volume measurements"));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
	state = 0;

	// properties
	txt_displayer = new QLabel(" ");
	//txt_displayer->setWordWrap(true);

	rb_vector = new QRadioButton("Vector");
	rb_dist = new QRadioButton("Distance");
	rb_thick = new QRadioButton("Thickness");
	rb_angle = new QRadioButton("Angle");
	rb_4ptangle = new QRadioButton("4pt-Angle");
	rb_vol = new QRadioButton("Volume");
	auto modegroup = new QButtonGroup(this);
	modegroup->insert(rb_vector);
	modegroup->insert(rb_dist);
	modegroup->insert(rb_thick);
	modegroup->insert(rb_angle);
	modegroup->insert(rb_4ptangle);
	modegroup->insert(rb_vol);
	rb_dist->setChecked(true);

	rb_pts = new QRadioButton("Clicks");
	rb_lbls = new QRadioButton("Labels");
	auto inputgroup = new QButtonGroup(this);
	inputgroup->insert(rb_pts);
	inputgroup->insert(rb_lbls);
	rb_pts->setChecked(true);

	cbb_lb1 = new QComboBox;
	cbb_lb2 = new QComboBox;
	cbb_lb3 = new QComboBox;
	cbb_lb4 = new QComboBox;

	// layout
	auto method_layout = make_vbox({rb_vector, rb_dist, rb_thick, rb_angle, rb_4ptangle, rb_vol});
	method_layout->addSpacerItem(new QSpacerItem(1,1,QSizePolicy::Minimum, QSizePolicy::Expanding));
	auto method_area = new QFrame;
	method_area->setLayout(method_layout);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);

	input_area = new QWidget;
	input_area->setLayout(make_hbox({rb_pts, rb_lbls}));
	input_area->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	auto labels_layout = new QFormLayout;
	labels_layout->addRow("Point 1", cbb_lb1);
	labels_layout->addRow("Point 2", cbb_lb2);
	labels_layout->addRow("Point 3", cbb_lb3);
	labels_layout->addRow("Point 4", cbb_lb4);

	labels_area = new QWidget;
	labels_area->setLayout(labels_layout);

	auto params_layout = new QVBoxLayout;
	params_layout->addWidget(input_area);
	params_layout->addWidget(txt_displayer);
	params_layout->setAlignment(txt_displayer, Qt::AlignHCenter | Qt::AlignCenter);

	stacked_widget = new QStackedWidget;
	stacked_widget->addWidget(new QLabel(" "));
	stacked_widget->addWidget(labels_area);

	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addLayout(params_layout);
	top_layout->addWidget(stacked_widget);

	setLayout(top_layout);

	// initialize
	marks_changed();
	method_changed(0);
	inputtype_changed(0);

	drawing = false;
	established.clear();
	dynamic.clear();

	emit vp1dyn_changed(&established, &dynamic);

	// connections
	connect(modegroup, SIGNAL(buttonClicked(int)), this, SLOT(method_changed(int)));
	connect(inputgroup, SIGNAL(buttonClicked(int)), this, SLOT(inputtype_changed(int)));
	connect(cbb_lb1, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	connect(cbb_lb2, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	connect(cbb_lb3, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	connect(cbb_lb4, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
}

void MeasurementWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;

	getlabels();
}

void MeasurementWidget::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand_changed(handler3D->get_activebmphandler());
	}
	else
	{
		getlabels();
	}
}

void MeasurementWidget::setActiveLabels(eActiveLabels act)
{
	bool b1=false, b2=false, b3=false, b4=false;
	switch(act)
	{
	case kP4:
		b4 = true;
	case kP3:
		b3 = true;
	case kP2:
		b2 = true;
	case kP1:
		b1 = true;
	}
	cbb_lb1->setEnabled(b1);
	cbb_lb2->setEnabled(b2);
	cbb_lb3->setEnabled(b3);
	cbb_lb4->setEnabled(b4);
}

void MeasurementWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

FILE* MeasurementWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 4)
	{
		int dummy;
		dummy = (int)(rb_vector->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_dist->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_angle->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_4ptangle->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_vol->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_pts->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_lbls->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* MeasurementWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 3)
	{
		int dummy;
		if (version >= 4)
		{
			fread(&dummy, sizeof(int), 1, fp);
			rb_vector->setChecked(dummy > 0);
		}
		fread(&dummy, sizeof(int), 1, fp);
		rb_dist->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_angle->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_4ptangle->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_vol->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_pts->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_lbls->setChecked(dummy > 0);

		method_changed(0);
		inputtype_changed(0);
	}
	return fp;
}

void MeasurementWidget::on_mouse_clicked(Point p)
{
	if (rb_pts->isChecked())
	{
		if (rb_vector->isChecked())
		{
			if (state == 0)
			{
				p1 = p;
				drawing = true;
				txt_displayer->setText("Mark end point.");
				set_coord(0, p, handler3D->active_slice());
				state = 1;
				dynamic.clear();
				established.clear();
				emit vp1dyn_changed(&established, &dynamic);
			}
			else if (state == 1)
			{
				drawing = false;
				set_coord(1, p, handler3D->active_slice());
				state = 0;
				addLine(&established, p1, p);
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
				txt_displayer->setText(
						QString("(") + QString::number(calculatevec(0), 'g', 3) +
						QString(",") + QString::number(calculatevec(1), 'g', 3) +
						QString(",") + QString::number(calculatevec(2), 'g', 3) +
						QString(") mm (Mark new start point.)"));
			}
		}
		else if (rb_dist->isChecked())
		{
			if (state == 0)
			{
				p1 = p;
				drawing = true;
				txt_displayer->setText("Mark end point.");
				set_coord(0, p, handler3D->active_slice());
				state = 1;
				dynamic.clear();
				established.clear();
				emit vp1dyn_changed(&established, &dynamic, true);
			}
			else if (state == 1)
			{
				drawing = false;
				set_coord(1, p, handler3D->active_slice());
				state = 0;
				addLine(&established, p1, p);
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic, true);
				txt_displayer->setText(QString::number(calculate(), 'g', 3) +
															 QString(" mm (Mark new start point.)"));
			}
		}
		else if (rb_thick->isChecked())
		{
			if (state == 0)
			{
				p1 = p;
				drawing = true;
				txt_displayer->setText("Mark end point.");
				set_coord(0, p, handler3D->active_slice());
				state = 1;
				dynamic.clear();
				established.clear();
				emit vp1dyn_changed(&established, &dynamic);
			}
			else if (state == 1)
			{
				drawing = false;
				set_coord(1, p, handler3D->active_slice());
				state = 0;
				addLine(&established, p1, p);
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
				txt_displayer->setText(QString::number(calculate(), 'g', 3) +
															 QString(" mm Mark new start point.)"));
			}
		}
		else if (rb_angle->isChecked())
		{
			if (state == 0)
			{
				txt_displayer->setText("Mark second point.");
				set_coord(0, p, handler3D->active_slice());
				state = 1;
				drawing = true;
				p1 = p;
				established.clear();
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
			}
			else if (state == 1)
			{
				txt_displayer->setText("Mark third point.");
				set_coord(1, p, handler3D->active_slice());
				state = 2;
				addLine(&established, p1, p);
				p1 = p;
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
			}
			else if (state == 2)
			{
				drawing = false;
				set_coord(2, p, handler3D->active_slice());
				state = 0;
				addLine(&established, p1, p);
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
				txt_displayer->setText(QString::number(calculate(), 'g', 3) +
															 QString(" deg (Mark new first point.)"));
			}
		}
		else if (rb_4ptangle->isChecked())
		{
			if (state == 0)
			{
				txt_displayer->setText("Mark second point.");
				set_coord(0, p, handler3D->active_slice());
				state = 1;
				drawing = true;
				p1 = p;
				established.clear();
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
			}
			else if (state == 1)
			{
				txt_displayer->setText("Mark third point.");
				set_coord(1, p, handler3D->active_slice());
				state = 2;
				addLine(&established, p1, p);
				drawing = true;
				p1 = p;
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
			}
			else if (state == 2)
			{
				txt_displayer->setText("Mark third point.");
				set_coord(2, p, handler3D->active_slice());
				state = 3;
				addLine(&established, p1, p);
				drawing = true;
				p1 = p;
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
			}
			else if (state == 3)
			{
				set_coord(3, p, handler3D->active_slice());
				state = 0;
				drawing = false;
				addLine(&established, p1, p);
				dynamic.clear();
				emit vp1dyn_changed(&established, &dynamic);
				txt_displayer->setText(QString::number(calculate(), 'g', 3) +
															 QString(" deg (Mark new first point.)"));
			}
		}
		else if (rb_vol->isChecked())
		{
			state = 0;
			established.clear();
			dynamic.clear();
			emit vp1dyn_changed(&established, &dynamic);
			QString tissuename1;
			tissues_size_t tnr =
					handler3D->get_tissue_pt(p, handler3D->active_slice());
			if (tnr == 0)
				tissuename1 = QString("-");
			else
				tissuename1 = ToQ(TissueInfos::GetTissueName(tnr));
			QString note = QString("");
			if (!(handler3D->start_slice() == 0 && handler3D->end_slice() == handler3D->num_slices()))
			{
				note = QString("\nCalculated for active slices only.");
			}
			txt_displayer->setText(
					QString("Tissue '") + tissuename1 + QString("': ") +
					QString::number(handler3D->calculate_tissuevolume(
															p, handler3D->active_slice()),
							'g', 3) +
					QString(" mm^3\n") + QString("'Target': ") +
					QString::number(handler3D->calculate_volume(
															p, handler3D->active_slice()),
							'g', 3) +
					QString(" mm^3") + note + QString("\n(Select new object.)"));
		}
	}
	else if (rb_lbls->isChecked())
	{
		if (rb_vol->isChecked())
		{
			state = 0;
			established.clear();
			dynamic.clear();
			emit vp1dyn_changed(&established, &dynamic);
			QString tissuename1;
			tissues_size_t tnr = handler3D->get_tissue_pt(p, handler3D->active_slice());
			if (tnr == 0)
				tissuename1 = QString("-");
			else
				tissuename1 = ToQ(TissueInfos::GetTissueName(tnr));
			QString note = QString("");
			if (!(handler3D->start_slice() == 0 && handler3D->end_slice() == handler3D->num_slices()))
			{
				note = QString("\nCalculated for active slices only.");
			}
			txt_displayer->setText(
					QString("Tissue '") + tissuename1 + QString("': ") +
					QString::number(handler3D->calculate_tissuevolume(p, handler3D->active_slice()), 'g', 3) +
					QString(" mm^3\n") + QString("'Target': ") +
					QString::number(handler3D->calculate_volume(p, handler3D->active_slice()), 'g', 3) +
					QString(" mm^3") + note + QString("\n(Select new object.)"));
		}
	}
}

void MeasurementWidget::on_mouse_moved(Point p)
{
	if (drawing)
	{
		dynamic.clear();
		addLine(&dynamic, p1, p);
		emit vpdyn_changed(&dynamic);
	}
}

void MeasurementWidget::set_coord(unsigned short posit, Point p, unsigned short slicenr)
{
	pt[posit][0] = (int)p.px;
	pt[posit][1] = (int)p.py;
	pt[posit][2] = (int)slicenr;
}

void MeasurementWidget::cbb_changed(int)
{
	if (rb_lbls->isChecked() && !rb_vol->isChecked())
	{
		state = 0;
		drawing = false;
		dynamic.clear();
		established.clear();
		if (rb_dist->isChecked())
		{
			Point pc;
			pc.px = (short)handler3D->width() / 2;
			pc.py = (short)handler3D->height() / 2;
			Point p1, p2;
			if (cbb_lb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = labels[cbb_lb1->currentItem() - 1].p;
			if (cbb_lb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = labels[cbb_lb2->currentItem() - 1].p;
			addLine(&established, p1, p2);
			txt_displayer->setText(QString::number(calculate(), 'g', 3) + QString(" mm"));
		}
		else if (rb_thick->isChecked())
		{
			Point pc;
			pc.px = (short)handler3D->width() / 2;
			pc.py = (short)handler3D->height() / 2;
			Point p1, p2;
			if (cbb_lb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = labels[cbb_lb1->currentItem() - 1].p;
			if (cbb_lb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = labels[cbb_lb2->currentItem() - 1].p;
			addLine(&established, p1, p2);
			txt_displayer->setText(QString::number(calculate(), 'g', 3) + QString(" mm"));
		}
		else if (rb_vector->isChecked())
		{
			Point pc;
			pc.px = (short)handler3D->width() / 2;
			pc.py = (short)handler3D->height() / 2;
			Point p1, p2;
			if (cbb_lb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = labels[cbb_lb1->currentItem() - 1].p;
			if (cbb_lb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = labels[cbb_lb2->currentItem() - 1].p;
			addLine(&established, p1, p2);
			txt_displayer->setText(
					QString("(") + QString::number(calculatevec(0), 'g', 3) +
					QString(",") + QString::number(calculatevec(1), 'g', 3) +
					QString(",") + QString::number(calculatevec(2), 'g', 3) +
					QString(") mm (Mark new start point.)"));
		}
		else if (rb_angle->isChecked())
		{
			Point pc;
			pc.px = (short)handler3D->width() / 2;
			pc.py = (short)handler3D->height() / 2;
			Point p1, p2, p3;
			if (cbb_lb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = labels[cbb_lb1->currentItem() - 1].p;
			if (cbb_lb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = labels[cbb_lb2->currentItem() - 1].p;
			if (cbb_lb3->currentItem() == 0)
				p3 = pc;
			else
				p3 = labels[cbb_lb3->currentItem() - 1].p;
			addLine(&established, p1, p2);
			addLine(&established, p2, p3);
			txt_displayer->setText(QString::number(calculate(), 'g', 3) + QString(" deg"));
		}
		else if (rb_4ptangle->isChecked())
		{
			Point pc;
			pc.px = (short)handler3D->width() / 2;
			pc.py = (short)handler3D->height() / 2;
			Point p1, p2, p3, p4;
			if (cbb_lb1->currentItem() == 0)
				p1 = pc;
			else
				p1 = labels[cbb_lb1->currentItem() - 1].p;
			if (cbb_lb2->currentItem() == 0)
				p2 = pc;
			else
				p2 = labels[cbb_lb2->currentItem() - 1].p;
			if (cbb_lb3->currentItem() == 0)
				p3 = pc;
			else
				p3 = labels[cbb_lb3->currentItem() - 1].p;
			if (cbb_lb4->currentItem() == 0)
				p4 = pc;
			else
				p4 = labels[cbb_lb4->currentItem() - 1].p;
			addLine(&established, p1, p2);
			addLine(&established, p2, p3);
			addLine(&established, p3, p4);
			txt_displayer->setText(QString::number(calculate(), 'g', 3) + QString(" deg"));
		}
		emit vp1dyn_changed(&established, &dynamic);
	}
}

void MeasurementWidget::method_changed(int) { update_visualization(); }

void MeasurementWidget::inputtype_changed(int)
{ 
	update_visualization();
}

void MeasurementWidget::update_visualization()
{
	if (labels.empty())
	{
		//BLdisconnect(inputgroup, SIGNAL(buttonClicked(int)), this, SLOT(inputtype_changed(int)));
		input_area->hide();
		rb_pts->setChecked(true);
		rb_lbls->setChecked(false);
		//BLconnect(inputgroup, SIGNAL(buttonClicked(int)), this, SLOT(inputtype_changed(int)));
	}
	else
	{
		input_area->show();
	}

	if (rb_pts->isChecked() || rb_vol->isChecked())
	{
		state = 0;
		drawing = false;

		established.clear();
		dynamic.clear();
		emit vp1dyn_changed(&established, &dynamic);
		stacked_widget->setCurrentIndex(0);
		if (rb_dist->isChecked() || rb_thick->isChecked() || rb_vector->isChecked())
		{
			txt_displayer->setText("Mark start point.");
		}
		else if (rb_angle->isChecked())
		{
			txt_displayer->setText("Mark first point.");
		}
		else if (rb_4ptangle->isChecked())
		{
			txt_displayer->setText("Mark first point.");
		}
		else if (rb_vol->isChecked())
		{
			txt_displayer->setText("Select object.");
		}
	}
	else if (rb_lbls->isChecked())
	{
		stacked_widget->setCurrentIndex(1);
		if (rb_dist->isChecked() || rb_thick->isChecked() || rb_vector->isChecked())
		{
			setActiveLabels(kP2);
		}
		else if (rb_angle->isChecked())
		{
			setActiveLabels(kP3);
		}
		else if (rb_4ptangle->isChecked())
		{
			setActiveLabels(kP4);
		}
		cbb_changed(0);
	}
}

void MeasurementWidget::getlabels()
{
	handler3D->get_labels(&labels);
	cbb_lb1->clear();
	cbb_lb2->clear();
	cbb_lb3->clear();
	cbb_lb4->clear();

	disconnect(cbb_lb1, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	disconnect(cbb_lb2, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	disconnect(cbb_lb3, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	disconnect(cbb_lb4, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));

	cbb_lb1->insertItem(QString("Center"));
	cbb_lb2->insertItem(QString("Center"));
	cbb_lb3->insertItem(QString("Center"));
	cbb_lb4->insertItem(QString("Center"));

	for (size_t i = 0; i < labels.size(); i++)
	{
		cbb_lb1->insertItem(QString(labels[i].name.c_str()));
		cbb_lb2->insertItem(QString(labels[i].name.c_str()));
		cbb_lb3->insertItem(QString(labels[i].name.c_str()));
		cbb_lb4->insertItem(QString(labels[i].name.c_str()));
	}

	connect(cbb_lb1, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	connect(cbb_lb2, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	connect(cbb_lb3, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));
	connect(cbb_lb4, SIGNAL(activated(int)), this, SLOT(cbb_changed(int)));

	update_visualization();
}

void MeasurementWidget::marks_changed() { getlabels(); }

float MeasurementWidget::calculate()
{
	float thick = handler3D->get_slicethickness();
	Pair p1 = handler3D->get_pixelsize();

	float value = 0;
	if (rb_lbls->isChecked())
	{
		if (cbb_lb1->currentItem() == 0)
		{
			pt[0][0] = (int)handler3D->width() / 2;
			pt[0][1] = (int)handler3D->height() / 2;
			pt[0][2] = (int)handler3D->num_slices() / 2;
		}
		else
		{
			pt[0][0] = (int)labels[cbb_lb1->currentItem() - 1].p.px;
			pt[0][1] = (int)labels[cbb_lb1->currentItem() - 1].p.py;
			pt[0][2] = (int)labels[cbb_lb1->currentItem() - 1].slicenr;
		}
		if (cbb_lb2->currentItem() == 0)
		{
			pt[1][0] = (int)handler3D->width() / 2;
			pt[1][1] = (int)handler3D->height() / 2;
			pt[1][2] = (int)handler3D->num_slices() / 2;
		}
		else
		{
			pt[1][0] = (int)labels[cbb_lb2->currentItem() - 1].p.px;
			pt[1][1] = (int)labels[cbb_lb2->currentItem() - 1].p.py;
			pt[1][2] = (int)labels[cbb_lb2->currentItem() - 1].slicenr;
		}
		if (cbb_lb3->currentItem() == 0)
		{
			pt[2][0] = (int)handler3D->width() / 2;
			pt[2][1] = (int)handler3D->height() / 2;
			pt[2][2] = (int)handler3D->num_slices() / 2;
		}
		else
		{
			pt[2][0] = (int)labels[cbb_lb3->currentItem() - 1].p.px;
			pt[2][1] = (int)labels[cbb_lb3->currentItem() - 1].p.py;
			pt[2][2] = (int)labels[cbb_lb3->currentItem() - 1].slicenr;
		}
		if (cbb_lb4->currentItem() == 0)
		{
			pt[3][0] = (int)handler3D->width() / 2;
			pt[3][1] = (int)handler3D->height() / 2;
			pt[3][2] = (int)handler3D->num_slices() / 2;
		}
		else
		{
			pt[3][0] = (int)labels[cbb_lb4->currentItem() - 1].p.px;
			pt[3][1] = (int)labels[cbb_lb4->currentItem() - 1].p.py;
			pt[3][2] = (int)labels[cbb_lb4->currentItem() - 1].slicenr;
		}
	}

	if (rb_dist->isChecked())
	{
		value = sqrt(
				(pt[0][0] - pt[1][0]) * (pt[0][0] - pt[1][0]) * p1.high * p1.high +
				(pt[0][1] - pt[1][1]) * (pt[0][1] - pt[1][1]) * p1.low * p1.low +
				(pt[0][2] - pt[1][2]) * (pt[0][2] - pt[1][2]) * thick * thick);
	}
	else if (rb_thick->isChecked())
	{
		int xDist = abs(pt[0][0] - pt[1][0]);
		if (xDist != 0)
			xDist += 1;
		int yDist = abs(pt[0][1] - pt[1][1]);
		if (yDist != 0)
			yDist += 1;
		int zDist = abs(pt[0][2] - pt[1][2]);
		if (zDist != 0)
			zDist += 1;
		if (!(xDist || yDist || zDist))
			xDist = 1;
		value = sqrt((xDist) * (xDist)*p1.high * p1.high +
								 (yDist) * (yDist)*p1.low * p1.low +
								 (zDist) * (zDist)*thick * thick);
	}
	else if (rb_angle->isChecked())
	{
		float l1square =
				(pt[0][0] - pt[1][0]) * (pt[0][0] - pt[1][0]) * p1.high * p1.high +
				(pt[0][1] - pt[1][1]) * (pt[0][1] - pt[1][1]) * p1.low * p1.low +
				(pt[0][2] - pt[1][2]) * (pt[0][2] - pt[1][2]) * thick * thick;
		float l2square =
				(pt[2][0] - pt[1][0]) * (pt[2][0] - pt[1][0]) * p1.high * p1.high +
				(pt[2][1] - pt[1][1]) * (pt[2][1] - pt[1][1]) * p1.low * p1.low +
				(pt[2][2] - pt[1][2]) * (pt[2][2] - pt[1][2]) * thick * thick;
		if (l1square * l2square > 0)
			value = acos(((pt[0][0] - pt[1][0]) * (pt[2][0] - pt[1][0]) *
													 p1.high * p1.high +
											 (pt[0][1] - pt[1][1]) * (pt[2][1] - pt[1][1]) *
													 p1.low * p1.low +
											 (pt[0][2] - pt[1][2]) * (pt[2][2] - pt[1][2]) *
													 thick * thick) /
									 sqrt(l1square * l2square)) *
							180 / 3.141592f;
	}
	else if (rb_4ptangle->isChecked())
	{
		float d1[3];
		float d2[3];
		d1[0] = (pt[0][1] - pt[1][1]) * (pt[1][2] - pt[2][2]) * thick * p1.low -
						(pt[0][2] - pt[1][2]) * (pt[1][1] - pt[2][1]) * thick * p1.low;
		d1[1] =
				(pt[0][2] - pt[1][2]) * (pt[1][0] - pt[2][0]) * thick * p1.high -
				(pt[0][0] - pt[1][0]) * (pt[1][2] - pt[2][2]) * thick * p1.high;
		d1[2] =
				(pt[0][0] - pt[1][0]) * (pt[1][1] - pt[2][1]) * p1.high * p1.low -
				(pt[0][1] - pt[1][1]) * (pt[1][0] - pt[2][0]) * p1.high * p1.low;
		d2[0] = (pt[1][1] - pt[2][1]) * (pt[2][2] - pt[3][2]) * thick * p1.low -
						(pt[1][2] - pt[2][2]) * (pt[2][1] - pt[3][1]) * thick * p1.low;
		d2[1] =
				(pt[1][2] - pt[2][2]) * (pt[2][0] - pt[3][0]) * thick * p1.high -
				(pt[1][0] - pt[2][0]) * (pt[2][2] - pt[3][2]) * thick * p1.high;
		d2[2] =
				(pt[1][0] - pt[2][0]) * (pt[2][1] - pt[3][1]) * p1.high * p1.low -
				(pt[1][1] - pt[2][1]) * (pt[2][0] - pt[3][0]) * p1.high * p1.low;
		float l1square = (d1[0] * d1[0] + d1[1] * d1[1] + d1[2] * d1[2]);
		float l2square = (d2[0] * d2[0] + d2[1] * d2[1] + d2[2] * d2[2]);
		if (l1square * l2square > 0)
			value = acos((d1[0] * d2[0] + d1[1] * d2[1] + d1[2] * d2[2]) /
									 sqrt(l1square * l2square)) *
							180 / 3.141592f;
	}

	return value;
}

float MeasurementWidget::calculatevec(unsigned short orient)
{
	float thick = handler3D->get_slicethickness();
	Pair p1 = handler3D->get_pixelsize();

	float value = 0;
	if (rb_lbls->isChecked())
	{
		if (cbb_lb1->currentItem() == 0)
		{
			pt[0][0] = (int)handler3D->width() / 2;
			pt[0][1] = (int)handler3D->height() / 2;
			pt[0][2] = (int)handler3D->num_slices() / 2;
		}
		else
		{
			pt[0][0] = (int)labels[cbb_lb1->currentItem() - 1].p.px;
			pt[0][1] = (int)labels[cbb_lb1->currentItem() - 1].p.py;
			pt[0][2] = (int)labels[cbb_lb1->currentItem() - 1].slicenr;
		}
		if (cbb_lb2->currentItem() == 0)
		{
			pt[1][0] = (int)handler3D->width() / 2;
			pt[1][1] = (int)handler3D->height() / 2;
			pt[1][2] = (int)handler3D->num_slices() / 2;
		}
		else
		{
			pt[1][0] = (int)labels[cbb_lb2->currentItem() - 1].p.px;
			pt[1][1] = (int)labels[cbb_lb2->currentItem() - 1].p.py;
			pt[1][2] = (int)labels[cbb_lb2->currentItem() - 1].slicenr;
		}
	}

	if (orient == 0)
		value = (pt[1][0] - pt[0][0]) * p1.high;
	else if (orient == 1)
		value = (pt[1][1] - pt[0][1]) * p1.low;
	else if (orient == 2)
		value = (pt[1][2] - pt[0][2]) * thick;

	return value;
}

void MeasurementWidget::cleanup()
{
	dynamic.clear();
	established.clear();
	drawing = false;
	emit vp1dyn_changed(&established, &dynamic);
}
