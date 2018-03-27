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
#include "edge_widget.h"

#include "Core/Point.h"

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

edge_widget::edge_widget(SlicesHandler *hand3D, QWidget *parent,
												 const char *name, Qt::WindowFlags wFlags)
		: QWidget1(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("Various edge extraction routines. These are mostly useful "
										"as part of other segmentation techniques."));

	activeslice = handler3D->get_activeslice();
	bmphand = handler3D->get_activebmphandler();

	hboxoverall = new Q3HBox(this);
	vboxmethods = new Q3VBox(hboxoverall);
	vbox1 = new Q3VBox(hboxoverall);
	hbox1 = new Q3HBox(vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox3 = new Q3HBox(vbox1);
	btn_exec = new QPushButton("Execute", vbox1);

	txt_sigmal = new QLabel("Sigma: 0 ", hbox1);
	sl_sigma = new QSlider(Qt::Horizontal, hbox1);
	sl_sigma->setRange(1, 100);
	sl_sigma->setValue(30);
	txt_sigma2 = new QLabel(" 5", hbox1);

	txt_thresh11 = new QLabel("Thresh: 0 ", hbox2);
	sl_thresh1 = new QSlider(Qt::Horizontal, hbox2);
	sl_thresh1->setRange(1, 100);
	sl_thresh1->setValue(20);
	txt_thresh12 = new QLabel(" 150", hbox2);

	txt_thresh21 = new QLabel("Thresh high: 0 ", hbox3);
	;
	sl_thresh2 = new QSlider(Qt::Horizontal, hbox3);
	sl_thresh2->setRange(1, 100);
	sl_thresh2->setValue(80);
	txt_thresh22 = new QLabel(" 150", hbox3);

	rb_sobel = new QRadioButton(QString("Sobel"), vboxmethods);
	rb_laplacian = new QRadioButton(QString("Laplacian"), vboxmethods);
	rb_interquartile = new QRadioButton(QString("Interquart."), vboxmethods);
	rb_momentline = new QRadioButton(QString("Moment"), vboxmethods);
	rb_gaussline = new QRadioButton(QString("Gauss"), vboxmethods);
	rb_canny = new QRadioButton(QString("Canny"), vboxmethods);
	rb_laplacianzero = new QRadioButton(QString("Lapl. Zero"), vboxmethods);

	modegroup = new QButtonGroup(this);
	//	modegroup->hide();
	modegroup->insert(rb_sobel);
	modegroup->insert(rb_laplacian);
	modegroup->insert(rb_interquartile);
	modegroup->insert(rb_momentline);
	modegroup->insert(rb_gaussline);
	modegroup->insert(rb_canny);
	modegroup->insert(rb_laplacianzero);
	rb_sobel->setChecked(TRUE);

	// 2018.02.14: fix options view is too narrow
	vbox1->setMinimumWidth(std::max(300, vbox1->sizeHint().width()));

	vboxmethods->setMargin(5);
	vbox1->setMargin(5);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vboxmethods->setLineWidth(1);

	// 	vboxmethods->setFixedSize(vboxmethods->sizeHint());
	// 	hboxoverall->setFixedSize(hboxoverall->sizeHint());
	//	setFixedSize(vbox1->size());

	method_changed(0);

	QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
									 SLOT(method_changed(int)));
	QObject::connect(btn_exec, SIGNAL(clicked()), this, SLOT(execute()));
	QObject::connect(sl_sigma, SIGNAL(valueChanged(int)), this,
									 SLOT(slider_changed(int)));
	QObject::connect(sl_thresh1, SIGNAL(valueChanged(int)), this,
									 SLOT(slider_changed(int)));
	QObject::connect(sl_thresh2, SIGNAL(valueChanged(int)), this,
									 SLOT(slider_changed(int)));

	return;
}

edge_widget::~edge_widget()
{
	delete vbox1;
	delete modegroup;
}

void edge_widget::slicenr_changed()
{
	//	if(activeslice!=handler3D->get_activeslice()){
	activeslice = handler3D->get_activeslice();
	bmphand_changed(handler3D->get_activebmphandler());
	//	}
}

void edge_widget::bmphand_changed(bmphandler *bmph)
{
	bmphand = bmph;
	return;
}

void edge_widget::execute()
{
	common::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->get_activeslice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (rb_sobel->isOn())
	{
		bmphand->sobel();
	}
	else if (rb_laplacian->isOn())
	{
		bmphand->laplacian1();
	}
	else if (rb_interquartile->isOn())
	{
		bmphand->median_interquartile(false);
	}
	else if (rb_momentline->isOn())
	{
		bmphand->moment_line();
	}
	else if (rb_gaussline->isOn())
	{
		bmphand->gauss_line(sl_sigma->value() * 0.05f);
	}
	else if (rb_canny->isOn())
	{
		bmphand->canny_line(sl_sigma->value() * 0.05f, sl_thresh1->value() * 1.5f,
												sl_thresh2->value() * 1.5f);
	}
	else
	{
		bmphand->laplacian_zero(sl_sigma->value() * 0.05f,
														sl_thresh1->value() * 0.5f, false);
	}

	emit end_datachange(this);
}

void edge_widget::method_changed(int)
{
	if (hideparams)
	{
		if (!rb_laplacian->isOn())
		{
			rb_laplacian->hide();
		}
		if (!rb_interquartile->isOn())
		{
			rb_interquartile->hide();
		}
		if (!rb_momentline->isOn())
		{
			rb_momentline->hide();
		}
		if (!rb_gaussline->isOn())
		{
			rb_gaussline->hide();
		}
		if (!rb_gaussline->isOn())
		{
			rb_laplacianzero->hide();
		}
	}
	else
	{
		rb_laplacian->show();
		rb_interquartile->show();
		rb_momentline->show();
		rb_gaussline->show();
		rb_laplacianzero->show();
	}

	if (rb_sobel->isOn())
	{
		hbox1->hide();
		hbox2->hide();
		hbox3->hide();
		btn_exec->show();
	}
	else if (rb_laplacian->isOn())
	{
		hbox1->hide();
		hbox2->hide();
		hbox3->hide();
		btn_exec->show();
	}
	else if (rb_interquartile->isOn())
	{
		hbox1->hide();
		hbox2->hide();
		hbox3->hide();
		btn_exec->show();
	}
	else if (rb_momentline->isOn())
	{
		hbox1->hide();
		hbox2->hide();
		hbox3->hide();
		btn_exec->show();
	}
	else if (rb_gaussline->isOn())
	{
		if (hideparams)
			hbox1->hide();
		else
			hbox1->show();
		hbox2->hide();
		hbox3->hide();
		btn_exec->show();
	}
	else if (rb_canny->isOn())
	{
		txt_thresh11->setText("Thresh low:  0 ");
		txt_thresh12->setText(" 150");
		if (hideparams)
			hbox1->hide();
		else
			hbox1->show();
		if (hideparams)
			hbox2->hide();
		else
			hbox2->show();
		if (hideparams)
			hbox3->hide();
		else
			hbox3->show();
		btn_exec->show();
	}
	else
	{
		txt_thresh11->setText("Thresh: 0 ");
		txt_thresh12->setText(" 50");
		if (hideparams)
			hbox1->hide();
		else
			hbox1->show();
		if (hideparams)
			hbox2->hide();
		else
			hbox2->show();
		hbox3->hide();
		btn_exec->show();
	}
}

void edge_widget::slider_changed(int newval)
{
	UNREFERENCED_PARAMETER(newval);
	execute();

	return;
}

QSize edge_widget::sizeHint() const { return vbox1->sizeHint(); }

void edge_widget::newloaded()
{
	activeslice = handler3D->get_activeslice();
	bmphand = handler3D->get_activebmphandler();
}

void edge_widget::init()
{
	slicenr_changed();
	hideparams_changed();
}

FILE *edge_widget::SaveParams(FILE *fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sl_sigma->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_thresh1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_thresh2->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_sobel->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_laplacian->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_interquartile->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_momentline->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_gaussline->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_canny->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_laplacianzero->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE *edge_widget::LoadParams(FILE *fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(modegroup, SIGNAL(buttonClicked(int)), this,
												SLOT(method_changed(int)));
		QObject::disconnect(sl_sigma, SIGNAL(valueChanged(int)), this,
												SLOT(slider_changed(int)));
		QObject::disconnect(sl_thresh1, SIGNAL(valueChanged(int)), this,
												SLOT(slider_changed(int)));
		QObject::disconnect(sl_thresh2, SIGNAL(valueChanged(int)), this,
												SLOT(slider_changed(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sl_sigma->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_thresh1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_thresh2->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		rb_sobel->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_laplacian->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_interquartile->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_momentline->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_gaussline->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_canny->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_laplacianzero->setChecked(dummy > 0);

		method_changed(0);

		QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
										 SLOT(method_changed(int)));
		QObject::connect(sl_sigma, SIGNAL(valueChanged(int)), this,
										 SLOT(slider_changed(int)));
		QObject::connect(sl_thresh1, SIGNAL(valueChanged(int)), this,
										 SLOT(slider_changed(int)));
		QObject::connect(sl_thresh2, SIGNAL(valueChanged(int)), this,
										 SLOT(slider_changed(int)));
	}
	return fp;
}

void edge_widget::hideparams_changed() { method_changed(0); }