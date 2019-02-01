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

#include "Core/Pair.h"

#include <QLabel>

#include <fstream>

using namespace iseg;

MeasurementWidget::MeasurementWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"Perform different types of distance, angle or volume measurements"));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	hboxoverall = new Q3HBox(this);
	hboxoverall->setMargin(8);
	vboxmethods = new Q3VBox(hboxoverall);
	vbox1 = new Q3VBox(hboxoverall);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	hbox4 = new Q3HBox(vbox1);
	txt_displayer = new QLabel(" ", vbox1);

	state = 0;

	rb_vector = new QRadioButton(QString("Vect."), vboxmethods);
	rb_dist = new QRadioButton(QString("Dist."), vboxmethods);
	rb_thick = new QRadioButton(QString("Thick."), vboxmethods);
	rb_angle = new QRadioButton(QString("Angle"), vboxmethods);
	rb_4ptangle = new QRadioButton(QString("4pt-Angle"), vboxmethods);
	rb_vol = new QRadioButton(QString("Volume"), vboxmethods);
	modegroup = new QButtonGroup(this);
	//	modegroup->hide();
	modegroup->insert(rb_vector);
	modegroup->insert(rb_dist);
	modegroup->insert(rb_thick);
	modegroup->insert(rb_angle);
	modegroup->insert(rb_4ptangle);
	modegroup->insert(rb_vol);
	rb_dist->setChecked(TRUE);

	rb_pts = new QRadioButton(QString("Clicks"), hbox2);
	rb_lbls = new QRadioButton(QString("Labels"), hbox2);
	inputgroup = new QButtonGroup(this);
	//	inputgroup->hide();
	inputgroup->insert(rb_pts);
	inputgroup->insert(rb_lbls);
	rb_pts->setChecked(TRUE);

	txt_ccb1 = new QLabel("Pt. 1:", hbox3);
	cbb_lb1 = new QComboBox(hbox3);
	txt_ccb2 = new QLabel(" Pt. 2:", hbox3);
	cbb_lb2 = new QComboBox(hbox3);
	txt_ccb3 = new QLabel("Pt. 3:", hbox4);
	cbb_lb3 = new QComboBox(hbox4);
	txt_ccb4 = new QLabel(" Pt. 4:", hbox4);
	cbb_lb4 = new QComboBox(hbox4);

	vboxmethods->setMargin(5);
	vbox1->setMargin(5);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vboxmethods->setLineWidth(1);

	hbox2->setFixedSize(hbox2->sizeHint());
	hbox3->setFixedSize(hbox3->sizeHint());
	// 	vboxmethods->setFixedSize(vboxmethods->sizeHint());
	// 	hboxoverall->setFixedSize(hboxoverall->sizeHint());//
	vbox1->setFixedSize(vbox1->sizeHint());

	marks_changed();
	method_changed(0);
	inputtype_changed(0);

	drawing = false;
	established.clear();
	dynamic.clear();

	emit vp1dyn_changed(&established, &dynamic);

	QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
			SLOT(method_changed(int)));
	QObject::connect(inputgroup, SIGNAL(buttonClicked(int)), this,
			SLOT(inputtype_changed(int)));
	QObject::connect(cbb_lb1, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::connect(cbb_lb2, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::connect(cbb_lb3, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::connect(cbb_lb4, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
}

MeasurementWidget::~MeasurementWidget()
{
	delete vbox1;
	delete modegroup;
	delete inputgroup;
}

QSize MeasurementWidget::sizeHint() const { return vbox1->sizeHint(); }

void MeasurementWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;

	getlabels();

	return;
}

void MeasurementWidget::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand_changed(handler3D->get_activebmphandler());
	}
	else
		getlabels();
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
		dummy = (int)(rb_vector->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_dist->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_angle->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_4ptangle->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_vol->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_pts->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_lbls->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* MeasurementWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 3)
	{
		QObject::disconnect(modegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(method_changed(int)));
		QObject::disconnect(inputgroup, SIGNAL(buttonClicked(int)), this,
				SLOT(inputtype_changed(int)));

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

		QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(method_changed(int)));
		QObject::connect(inputgroup, SIGNAL(buttonClicked(int)), this,
				SLOT(inputtype_changed(int)));
	}
	return fp;
}

void MeasurementWidget::on_mouse_clicked(Point p)
{
	if (rb_pts->isOn())
	{
		if (rb_vector->isOn())
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
		else if (rb_dist->isOn())
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
		else if (rb_thick->isOn())
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
		else if (rb_angle->isOn())
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
		else if (rb_4ptangle->isOn())
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
		else if (rb_vol->isOn())
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
	else if (rb_lbls->isOn())
	{
		if (rb_vol->isOn())
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
			if (!(handler3D->start_slice() == 0 &&
							handler3D->end_slice() == handler3D->num_slices()))
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

void MeasurementWidget::set_coord(unsigned short posit, Point p,
		unsigned short slicenr)
{
	pt[posit][0] = (int)p.px;
	pt[posit][1] = (int)p.py;
	pt[posit][2] = (int)slicenr;
}

void MeasurementWidget::cbb_changed(int)
{
	if (rb_lbls->isOn() && !rb_vol->isOn())
	{
		state = 0;
		drawing = false;
		dynamic.clear();
		established.clear();
		if (rb_dist->isOn())
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
			txt_displayer->setText(QString::number(calculate(), 'g', 3) +
														 QString(" mm"));
		}
		else if (rb_thick->isOn())
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
			txt_displayer->setText(QString::number(calculate(), 'g', 3) +
														 QString(" mm"));
		}
		else if (rb_vector->isOn())
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
		else if (rb_angle->isOn())
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
			txt_displayer->setText(QString::number(calculate(), 'g', 3) +
														 QString(" deg"));
		}
		else if (rb_4ptangle->isOn())
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
			txt_displayer->setText(QString::number(calculate(), 'g', 3) +
														 QString(" deg"));
		}
		emit vp1dyn_changed(&established, &dynamic);
	}
}

void MeasurementWidget::method_changed(int) { update_visualization(); }

void MeasurementWidget::inputtype_changed(int) { update_visualization(); }

void MeasurementWidget::update_visualization()
{
	if (labels.empty())
	{
		QObject::disconnect(inputgroup, SIGNAL(buttonClicked(int)), this,
				SLOT(inputtype_changed(int)));
		hbox2->hide();
		rb_pts->setChecked(true);
		rb_lbls->setChecked(false);
		QObject::connect(inputgroup, SIGNAL(buttonClicked(int)), this,
				SLOT(inputtype_changed(int)));
	}
	else
	{
		hbox2->show();
	}

	if (rb_pts->isOn())
	{
		state = 0;
		drawing = false;

		established.clear();
		dynamic.clear();
		emit vp1dyn_changed(&established, &dynamic);
		hbox3->hide();
		hbox4->hide();
		if (rb_dist->isOn() || rb_thick->isOn() || rb_vector->isOn())
		{
			txt_displayer->setText("Mark start point.");
		}
		else if (rb_angle->isOn())
		{
			txt_displayer->setText("Mark first point.");
		}
		else if (rb_4ptangle->isOn())
		{
			txt_displayer->setText("Mark first point.");
		}
		else if (rb_vol->isOn())
		{
			hbox2->hide();
			txt_displayer->setText("Select object.");
		}
	}
	else if (rb_lbls->isOn())
	{
		if (rb_dist->isOn() || rb_thick->isOn() || rb_vector->isOn())
		{
			hbox3->show();
			hbox4->hide();
			//			txt_displayer->setText(QString::number(calculate(),'g',3)+QString(" mm"));
		}
		else if (rb_angle->isOn())
		{
			hbox3->show();
			hbox4->show();
			cbb_lb3->show();
			cbb_lb4->hide();
			txt_ccb4->hide();
			//			txt_displayer->setText(QString::number(calculate(),'g',3)+QString(" deg"));
		}
		else if (rb_4ptangle->isOn())
		{
			hbox3->show();
			hbox4->show();
			cbb_lb3->show();
			cbb_lb4->show();
			txt_ccb4->show();
			//			txt_displayer->setText(QString::number(calculate(),'g',3)+QString(" deg"));
		}
		else if (rb_vol->isOn())
		{
			hbox2->hide();
			hbox3->hide();
			hbox4->hide();
			txt_displayer->setText("Select object.");
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

	QObject::disconnect(cbb_lb1, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::disconnect(cbb_lb2, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::disconnect(cbb_lb3, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::disconnect(cbb_lb4, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));

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

	QObject::connect(cbb_lb1, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::connect(cbb_lb2, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::connect(cbb_lb3, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));
	QObject::connect(cbb_lb4, SIGNAL(activated(int)), this,
			SLOT(cbb_changed(int)));

	update_visualization();
}

void MeasurementWidget::marks_changed() { getlabels(); }

float MeasurementWidget::calculate()
{
	float thick = handler3D->get_slicethickness();
	Pair p1 = handler3D->get_pixelsize();

	float value = 0;
	if (rb_lbls->isOn())
	{
		//float thick=handler3D->get_slicethickness();
		//Pair p1=handler3D->get_pixelsize();
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

	if (rb_dist->isOn())
	{
		value = sqrt(
				(pt[0][0] - pt[1][0]) * (pt[0][0] - pt[1][0]) * p1.high * p1.high +
				(pt[0][1] - pt[1][1]) * (pt[0][1] - pt[1][1]) * p1.low * p1.low +
				(pt[0][2] - pt[1][2]) * (pt[0][2] - pt[1][2]) * thick * thick);
	}
	else if (rb_thick->isOn())
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
	else if (rb_angle->isOn())
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
	else if (rb_4ptangle->isOn())
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
	if (rb_lbls->isOn())
	{
		//float thick=handler3D->get_slicethickness();
		//Pair p1=handler3D->get_pixelsize();
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
