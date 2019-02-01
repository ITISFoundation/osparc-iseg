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

#include "SlicesHandler.h"
#include "SmoothingWidget.h"
#include "bmp_read_1.h"

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qsize.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

#define UNREFERENCED_PARAMETER(P) (P)

using namespace iseg;

SmoothingWidget::SmoothingWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("Smoothing and noise removal filters."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	hboxoverall = new Q3HBox(this);
	hboxoverall->setMargin(8);
	vboxmethods = new Q3VBox(hboxoverall);
	vbox1 = new Q3VBox(hboxoverall);
	hbox1 = new Q3HBox(vbox1);
	hbox2 = new Q3HBox(vbox1);
	vbox2 = new Q3VBox(vbox1);
	hbox3 = new Q3HBox(vbox2);
	hbox5 = new Q3HBox(vbox2);
	hbox4 = new Q3HBox(vbox1);
	allslices = new QCheckBox(QString("Apply to all slices"), vbox1);
	pushexec = new QPushButton("Execute", vbox1);
	contdiff = new QPushButton("Cont. Diffusion", vbox1);

	txt_n = new QLabel("n: ", hbox1);
	sb_n = new QSpinBox(1, 11, 2, hbox1);
	sb_n->setValue(5);
	sb_n->setToolTip("'n' is the width of the kernel in pixels.");

	txt_sigma1 = new QLabel("Sigma: 0 ", hbox2);
	sl_sigma = new QSlider(Qt::Horizontal, hbox2);
	sl_sigma->setRange(1, 100);
	sl_sigma->setValue(20);
	sl_sigma->setToolTip(
			"Sigma gives the radius of the smoothing filter. Larger "
			"values remove more details.");
	txt_sigma2 = new QLabel(" 5", hbox2);

	txt_dt = new QLabel("dt: 1    ", hbox3);
	txt_iter = new QLabel("Iterations: ", hbox3);
	sb_iter = new QSpinBox(1, 100, 1, hbox3);
	sb_iter->setValue(20);

	txt_k = new QLabel("Sigma: 0 ", hbox4);
	sl_k = new QSlider(Qt::Horizontal, hbox4);
	sl_k->setRange(0, 100);
	sl_k->setValue(50);
	sl_k->setToolTip(
			"Together with value on the right, defines the Sigma of the "
			"smoothing filter.");
	sb_kmax = new QSpinBox(1, 100, 1, hbox4);
	sb_kmax->setValue(10);
	sb_kmax->setToolTip("Gives the max. radius of the smoothing filter. This "
											"value defines the scale used in the slider bar.");

	txt_restrain1 = new QLabel("Restraint: 0 ", hbox5);
	sl_restrain = new QSlider(Qt::Horizontal, hbox5);
	sl_restrain->setRange(0, 100);
	sl_restrain->setValue(0);
	txt_restrain2 = new QLabel(" 1", hbox5);

	rb_gaussian = new QRadioButton(QString("Gaussian"), vboxmethods);
	rb_gaussian->setToolTip("Gaussian smoothing blurs the image and can remove "
													"noise and details (high frequency).");
	rb_average = new QRadioButton(QString("Average"), vboxmethods);
	rb_average->setToolTip(
			"Mean smoothing blurs the image and can remove noise and details.");
	rb_median = new QRadioButton(QString("Median"), vboxmethods);
	rb_median->setToolTip(
			"Median filtering can remove (speckle or salt-and-pepper) noise.");
	rb_sigmafilter = new QRadioButton(QString("Sigma Filter"), vboxmethods);
	rb_sigmafilter->setToolTip(
			"Sigma filtering is a mixture between Gaussian and Average filtering. "
			"It "
			"preserves edges better than Average filtering.");
	rb_anisodiff =
			new QRadioButton(QString("Anisotropic Diffusion"), vboxmethods);
	rb_anisodiff->setToolTip("Anisotropic diffusion can remove noise, while "
													 "preserving significant details, such as edges.");

	modegroup = new QButtonGroup(this);
	modegroup->insert(rb_gaussian);
	modegroup->insert(rb_average);
	modegroup->insert(rb_median);
	modegroup->insert(rb_sigmafilter);
	modegroup->insert(rb_anisodiff);
	rb_gaussian->setChecked(TRUE);

	sl_restrain->setFixedWidth(300);
	sl_k->setFixedWidth(300);
	sl_sigma->setFixedWidth(300);

	vboxmethods->setMargin(5);
	vbox1->setMargin(5);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vboxmethods->setLineWidth(1);

	hbox1->setFixedSize(hbox1->sizeHint());
	hbox2->setFixedSize(hbox2->sizeHint());
	hbox3->setFixedSize(hbox3->sizeHint());
	hbox4->setFixedSize(hbox4->sizeHint());
	hbox5->setFixedSize(hbox5->sizeHint());
	vbox2->setFixedSize(vbox2->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint());
	// 	vboxmethods->setFixedSize(vboxmethods->sizeHint());
	// 	hboxoverall->setFixedSize(hboxoverall->sizeHint());

	//	setFixedSize(vbox1->size());

	method_changed(0);

	QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
			SLOT(method_changed(int)));
	QObject::connect(pushexec, SIGNAL(clicked()), this, SLOT(execute()));
	QObject::connect(contdiff, SIGNAL(clicked()), this, SLOT(continue_diff()));
	QObject::connect(sl_sigma, SIGNAL(valueChanged(int)), this,
			SLOT(sigmaslider_changed(int)));
	QObject::connect(sl_sigma, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_sigma, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
	QObject::connect(sl_k, SIGNAL(valueChanged(int)), this,
			SLOT(kslider_changed(int)));
	QObject::connect(sl_k, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_k, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
	QObject::connect(sb_n, SIGNAL(valueChanged(int)), this,
			SLOT(n_changed(int)));
	QObject::connect(sb_kmax, SIGNAL(valueChanged(int)), this,
			SLOT(kmax_changed(int)));

	return;
}

SmoothingWidget::~SmoothingWidget()
{
	delete vbox1;
	delete modegroup;
}

void SmoothingWidget::execute()
{
	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (allslices->isChecked())
	{
		if (rb_gaussian->isOn())
		{
			handler3D->gaussian(sl_sigma->value() * 0.05f);
		}
		else if (rb_average->isOn())
		{
			handler3D->average((short unsigned)sb_n->value());
		}
		else if (rb_median->isOn())
		{
			handler3D->median_interquartile(true);
		}
		else if (rb_sigmafilter->isOn())
		{
			handler3D->sigmafilter(
					(sl_k->value() + 1) * 0.01f * sb_kmax->value(),
					(short unsigned)sb_n->value(), (short unsigned)sb_n->value());
		}
		else
		{
			handler3D->aniso_diff(1.0f, sb_iter->value(), f2,
					sl_k->value() * 0.01f * sb_kmax->value(),
					sl_restrain->value() * 0.01f);
		}
	}
	else
	{
		if (rb_gaussian->isOn())
		{
			bmphand->gaussian(sl_sigma->value() * 0.05f);
		}
		else if (rb_average->isOn())
		{
			bmphand->average((short unsigned)sb_n->value());
		}
		else if (rb_median->isOn())
		{
			bmphand->median_interquartile(true);
		}
		else if (rb_sigmafilter->isOn())
		{
			bmphand->sigmafilter((sl_k->value() + 1) * 0.01f * sb_kmax->value(),
					(short unsigned)sb_n->value(),
					(short unsigned)sb_n->value());
		}
		else
		{
			bmphand->aniso_diff(1.0f, sb_iter->value(), f2,
					sl_k->value() * 0.01f * sb_kmax->value(),
					sl_restrain->value() * 0.01f);
		}
	}
	emit end_datachange(this);
}

void SmoothingWidget::method_changed(int)
{
	if (rb_gaussian->isOn())
	{
		if (hideparams)
			hbox2->hide();
		else
			hbox2->show();
		hbox1->hide();
		hbox4->hide();
		vbox2->hide();
		contdiff->hide();
	}
	else if (rb_average->isOn())
	{
		if (hideparams)
			hbox1->hide();
		else
			hbox1->show();
		hbox2->hide();
		hbox4->hide();
		vbox2->hide();
		contdiff->hide();
	}
	else if (rb_median->isOn())
	{
		hbox1->hide();
		hbox2->hide();
		hbox4->hide();
		vbox2->hide();
		contdiff->hide();
	}
	else if (rb_sigmafilter->isOn())
	{
		if (hideparams)
			hbox1->hide();
		else
			hbox1->show();
		//		hbox2->show();
		hbox2->hide();
		vbox2->hide();
		if (hideparams)
			hbox4->hide();
		else
			hbox4->show();
		contdiff->hide();
		txt_k->setText("Sigma: 0 ");
	}
	else
	{
		hbox1->hide();
		hbox2->hide();
		txt_k->setText("k: 0 ");
		if (hideparams)
			hbox4->hide();
		else
			hbox4->show();
		if (hideparams)
			vbox2->hide();
		else
			vbox2->show();
		if (hideparams)
			contdiff->hide();
		else
			contdiff->show();
	}
}

void SmoothingWidget::continue_diff()
{
	if (!rb_anisodiff->isOn())
	{
		return;
	}

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (allslices->isChecked())
	{
		handler3D->cont_anisodiff(1.0f, sb_iter->value(), f2,
				sl_k->value() * 0.01f * sb_kmax->value(),
				sl_restrain->value() * 0.01f);
	}
	else
	{
		bmphand->cont_anisodiff(1.0f, sb_iter->value(), f2,
				sl_k->value() * 0.01f * sb_kmax->value(),
				sl_restrain->value() * 0.01f);
	}

	emit end_datachange(this);
}

void SmoothingWidget::sigmaslider_changed(int newval)
{
	UNREFERENCED_PARAMETER(newval);
	if (rb_gaussian->isOn())
	{
		if (allslices->isChecked())
			handler3D->gaussian(sl_sigma->value() * 0.05f);
		else
			bmphand->gaussian(sl_sigma->value() * 0.05f);
		emit end_datachange(this, iseg::NoUndo);
	}

	return;
}

void SmoothingWidget::kslider_changed(int newval)
{
	UNREFERENCED_PARAMETER(newval);
	if (rb_sigmafilter->isOn())
	{
		if (allslices->isChecked())
			handler3D->sigmafilter(
					(sl_k->value() + 1) * 0.01f * sb_kmax->value(),
					(short unsigned)sb_n->value(), (short unsigned)sb_n->value());
		else
			bmphand->sigmafilter((sl_k->value() + 1) * 0.01f * sb_kmax->value(),
					(short unsigned)sb_n->value(),
					(short unsigned)sb_n->value());
		emit end_datachange(this, iseg::NoUndo);
	}

	return;
}

void SmoothingWidget::n_changed(int newval)
{
	UNREFERENCED_PARAMETER(newval);

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (rb_sigmafilter->isOn())
	{
		if (allslices->isChecked())
		{
			handler3D->sigmafilter(
					(sl_k->value() + 1) * 0.01f * sb_kmax->value(),
					(short unsigned)sb_n->value(), (short unsigned)sb_n->value());
		}
		else
		{
			bmphand->sigmafilter((sl_k->value() + 1) * 0.01f * sb_kmax->value(),
					(short unsigned)sb_n->value(),
					(short unsigned)sb_n->value());
		}
	}
	else if (rb_average)
	{
		if (allslices->isChecked())
		{
			handler3D->average((short unsigned)sb_n->value());
		}
		else
		{
			bmphand->average((short unsigned)sb_n->value());
		}
	}
	emit end_datachange(this);
}

void SmoothingWidget::kmax_changed(int newval)
{
	UNREFERENCED_PARAMETER(newval);

	iseg::DataSelection dataSelection;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);

	if (rb_sigmafilter->isOn())
	{
		if (allslices->isChecked())
		{
			handler3D->sigmafilter(
					(sl_k->value() + 1) * 0.01f * sb_kmax->value(),
					(short unsigned)sb_n->value(), (short unsigned)sb_n->value());
		}
		else
		{
			bmphand->sigmafilter((sl_k->value() + 1) * 0.01f * sb_kmax->value(),
					(short unsigned)sb_n->value(),
					(short unsigned)sb_n->value());
		}
	}
	emit end_datachange(this);
}

QSize SmoothingWidget::sizeHint() const { return vbox1->sizeHint(); }

void SmoothingWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void SmoothingWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
}

void SmoothingWidget::init()
{
	on_slicenr_changed();
	hideparams_changed();
}

void SmoothingWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
}

void SmoothingWidget::slider_pressed()
{
	if ((rb_gaussian->isOn() || rb_sigmafilter->isOn()))
	{
		iseg::DataSelection dataSelection;
		dataSelection.allSlices = allslices->isChecked();
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);
	}
}

void SmoothingWidget::slider_released()
{
	if (rb_gaussian->isOn() || rb_sigmafilter->isOn())
	{
		emit end_datachange(this);
	}
}

FILE* SmoothingWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sl_sigma->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_k->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_restrain->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_n->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_iter->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_kmax->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_gaussian->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_average->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_median->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_sigmafilter->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_anisodiff->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allslices->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* SmoothingWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(modegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(method_changed(int)));
		QObject::disconnect(sl_sigma, SIGNAL(valueChanged(int)), this,
				SLOT(sigmaslider_changed(int)));
		QObject::disconnect(sl_k, SIGNAL(valueChanged(int)), this,
				SLOT(kslider_changed(int)));
		QObject::disconnect(sb_n, SIGNAL(valueChanged(int)), this,
				SLOT(n_changed(int)));
		QObject::disconnect(sb_kmax, SIGNAL(valueChanged(int)), this,
				SLOT(kmax_changed(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sl_sigma->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_k->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_restrain->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_n->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_iter->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_kmax->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		rb_gaussian->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_average->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_median->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_sigmafilter->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_anisodiff->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		allslices->setChecked(dummy > 0);

		method_changed(0);

		QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(method_changed(int)));
		QObject::connect(sl_sigma, SIGNAL(valueChanged(int)), this,
				SLOT(sigmaslider_changed(int)));
		QObject::connect(sl_k, SIGNAL(valueChanged(int)), this,
				SLOT(kslider_changed(int)));
		QObject::connect(sb_n, SIGNAL(valueChanged(int)), this,
				SLOT(n_changed(int)));
		QObject::connect(sb_kmax, SIGNAL(valueChanged(int)), this,
				SLOT(kmax_changed(int)));
	}
	return fp;
}

void SmoothingWidget::hideparams_changed() { method_changed(0); }
