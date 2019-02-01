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

#include "HystereticGrowingWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/addLine.h"

#include "Core/Pair.h"

#include <qfiledialog.h>
#include <q3listbox.h>
#include <q3vbox.h>
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qimage.h>
#include <qinputdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpainter.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>
#include <qwidget.h>

using namespace std;
using namespace iseg;

HystereticGrowingWidget::HystereticGrowingWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("Segment a tissue by picking seed points and adding "
										"neighboring pixels with similar intensities."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	Pair p;
	p1.px = p1.py = 0;
	bmphand->get_bmprange(&p);
	lower_limit = p.low;
	upper_limit = p.high;

	vbox1 = new Q3VBox(this);
	vbox1->setMargin(8);
	hbox1 = new Q3HBox(vbox1);
	hbox2 = new Q3HBox(vbox1);
	hbox2a = new Q3HBox(vbox1);
	vbox2 = new Q3VBox(hbox1);
	vbox3 = new Q3VBox(hbox1);
	autoseed = new QCheckBox("AutoSeed: ", hbox2);
	if (autoseed->isOn())
		autoseed->toggle();
	vbox4 = new Q3VBox(hbox2);
	vbox5 = new Q3VBox(hbox2);
	vbox6 = new Q3VBox(hbox1);
	vbox7 = new Q3VBox(hbox2);
	allslices = new QCheckBox(QString("Apply to all slices"), hbox2a);
	pb_saveborders = new QPushButton("Save...", hbox2a);
	pb_loadborders = new QPushButton("Open...", hbox2a);
	txt_lower = new QLabel("Lower: ", vbox2);
	txt_upper = new QLabel("Upper: ", vbox2);
	txt_lowerhyster = new QLabel("Lower: ", vbox4);
	txt_upperhyster = new QLabel("Upper: ", vbox4);
	sl_lower = new QSlider(Qt::Horizontal, vbox3);
	sl_lower->setRange(0, 200);
	sl_lower->setValue(80);
	le_bordervall = new QLineEdit(vbox6);
	le_bordervall->setFixedWidth(80);
	sl_upper = new QSlider(Qt::Horizontal, vbox3);
	sl_upper->setRange(0, 200);
	sl_upper->setValue(120);
	le_bordervalu = new QLineEdit(vbox6);
	le_bordervalu->setFixedWidth(80);
	sl_lowerhyster = new QSlider(Qt::Horizontal, vbox5);
	sl_lowerhyster->setRange(0, 200);
	sl_lowerhyster->setValue(60);
	le_bordervallh = new QLineEdit(vbox7);
	le_bordervallh->setFixedWidth(80);
	sl_upperhyster = new QSlider(Qt::Horizontal, vbox5);
	sl_upperhyster->setRange(0, 200);
	sl_upperhyster->setValue(140);
	le_bordervaluh = new QLineEdit(vbox7);
	le_bordervaluh->setFixedWidth(80);
	hbox3 = new Q3HBox(vbox1);
	drawlimit = new QPushButton("Draw Limit", hbox3);
	clearlimit = new QPushButton("Clear Limit", hbox3);
	pushexec = new QPushButton("Execute", vbox1);

	sl_lower->setFixedWidth(400);
	sl_lowerhyster->setFixedWidth(300);

	vbox2->setFixedSize(vbox2->sizeHint());
	vbox3->setFixedSize(vbox3->sizeHint());
	vbox4->setFixedSize(vbox4->sizeHint());
	vbox5->setFixedSize(vbox5->sizeHint());
	vbox6->setFixedSize(vbox6->sizeHint());
	vbox7->setFixedSize(vbox7->sizeHint());
	hbox1->setFixedSize(hbox1->sizeHint());
	hbox2->setFixedSize(hbox2->sizeHint());
	hbox2a->setFixedSize(hbox2a->sizeHint());
	hbox3->setFixedSize(hbox3->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint());
	//	setFixedSize(vbox1->size());

	QObject::connect(pushexec, SIGNAL(clicked()), this, SLOT(execute()));
	QObject::connect(drawlimit, SIGNAL(clicked()), this, SLOT(limitpressed()));
	QObject::connect(clearlimit, SIGNAL(clicked()), this, SLOT(clearpressed()));
	QObject::connect(autoseed, SIGNAL(clicked()), this, SLOT(auto_toggled()));
	QObject::connect(sl_lower, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed()));
	QObject::connect(sl_upper, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed()));
	QObject::connect(sl_lowerhyster, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed()));
	QObject::connect(sl_upperhyster, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed()));
	QObject::connect(sl_lower, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_upper, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_lowerhyster, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_upperhyster, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_lower, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
	QObject::connect(sl_upper, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
	QObject::connect(sl_lowerhyster, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
	QObject::connect(sl_upperhyster, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));

	QObject::connect(le_bordervall, SIGNAL(returnPressed()), this,
			SLOT(le_bordervall_returnpressed()));
	QObject::connect(le_bordervalu, SIGNAL(returnPressed()), this,
			SLOT(le_bordervalu_returnpressed()));
	QObject::connect(le_bordervallh, SIGNAL(returnPressed()), this,
			SLOT(le_bordervallh_returnpressed()));
	QObject::connect(le_bordervaluh, SIGNAL(returnPressed()), this,
			SLOT(le_bordervaluh_returnpressed()));

	QObject::connect(pb_saveborders, SIGNAL(clicked()), this,
			SLOT(saveborders_execute()));
	QObject::connect(pb_loadborders, SIGNAL(clicked()), this,
			SLOT(loadborders_execute()));

	update_visible();
	getrange();
	init1();
}

void HystereticGrowingWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void HystereticGrowingWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;

	getrange();

	init1();
}

void HystereticGrowingWidget::on_mouse_clicked(Point p)
{
	if (limitdrawing)
	{
		last_pt = p;
	}
	else
	{
		if (!autoseed->isOn())
		{
			p1 = p;
			execute();
		}
	}
}

void HystereticGrowingWidget::update_visible()
{
	if (autoseed->isOn())
	{
		vbox4->show();
		vbox5->show();
		vbox7->show();
		allslices->setText("Apply to all slices");
		//		p1=nullptr;
	}
	else
	{
		vbox4->hide();
		vbox5->hide();
		vbox7->hide();
		allslices->setText("3D");
		//		if(p1!=nullptr)
	}

	return;
}

void HystereticGrowingWidget::auto_toggled()
{
	update_visible();

	if (autoseed->isOn())
	{
		execute();
	}

	return;
}

void HystereticGrowingWidget::execute()
{
	iseg::DataSelection dataSelection;
	dataSelection.work = dataSelection.limits = true;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	emit begin_datachange(dataSelection, this);

	execute1();

	emit end_datachange(this);
}

void HystereticGrowingWidget::execute1()
{
	if (autoseed->isOn())
	{
		float ll = lower_limit +
							 (upper_limit - lower_limit) * 0.005f * sl_lower->value();
		float uu = lower_limit +
							 (upper_limit - lower_limit) * 0.005f * sl_upper->value();
		float ul = ll + (uu - ll) * 0.005f * sl_lowerhyster->value();
		float lu = ll + (uu - ll) * 0.005f * sl_upperhyster->value();
		if (allslices->isChecked())
			handler3D->double_hysteretic_allslices(ll, ul, lu, uu, false,
					255.0f);
		else
			bmphand->double_hysteretic(ll, ul, lu, uu, false, 255.0f);

		le_bordervall->setText(QString::number(ll, 'g', 3));
		le_bordervalu->setText(QString::number(uu, 'g', 3));
		le_bordervallh->setText(
				QString::number(int(0.4f + (ul - ll) / (uu - ll) * 100), 'g', 3));
		le_bordervaluh->setText(
				QString::number(int(0.4f + (lu - ll) / (uu - ll) * 100), 'g', 3));
	}
	else
	{
		float ll = lower_limit +
							 (upper_limit - lower_limit) * 0.005f * sl_lower->value();
		float uu = lower_limit +
							 (upper_limit - lower_limit) * 0.005f * sl_upper->value();
		//		bmphand->thresholded_growing(p1,ll,uu,false,255.f);
		if (allslices->isChecked())
		{
			handler3D->thresholded_growing(handler3D->active_slice(), p1, ll,
					uu, 255.f);
		}
		else
		{
			bmphand->thresholded_growinglimit(p1, ll, uu, false, 255.f);
		}

		le_bordervall->setText(QString::number(ll, 'g', 3));
		le_bordervalu->setText(QString::number(uu, 'g', 3));
	}
}

void HystereticGrowingWidget::getrange()
{
	float ll =
			lower_limit + (upper_limit - lower_limit) * 0.005f * sl_lower->value();
	float uu =
			lower_limit + (upper_limit - lower_limit) * 0.005f * sl_upper->value();
	float ul = ll + (uu - ll) * 0.005f * sl_lowerhyster->value();
	float lu = ll + (uu - ll) * 0.005f * sl_upperhyster->value();

	Pair p;
	bmphand->get_bmprange(&p);
	lower_limit = p.low;
	upper_limit = p.high;

	getrange_sub(ll, uu, ul, lu);
}

void HystereticGrowingWidget::getrange_sub(float ll, float uu, float ul, float lu)
{
	if (ll < lower_limit)
	{
		ll = lower_limit;
		sl_lower->setValue(0);
	}
	else if (ll > upper_limit)
	{
		ll = upper_limit;
		sl_lower->setValue(200);
	}
	else
	{
		if (upper_limit == lower_limit)
			sl_lower->setValue(100);
		else
			sl_lower->setValue(int(
					0.5f + (ll - lower_limit) / (upper_limit - lower_limit) * 200));
	}

	if (uu < lower_limit)
	{
		uu = lower_limit;
		sl_upper->setValue(0);
	}
	else if (uu > upper_limit)
	{
		uu = upper_limit;
		sl_upper->setValue(200);
	}
	else
	{
		if (upper_limit == lower_limit)
			sl_upper->setValue(100);
		else
			sl_upper->setValue(int(
					0.5f + (uu - lower_limit) / (upper_limit - lower_limit) * 200));
	}

	if (ul < ll)
	{
		ul = ll;
		sl_lowerhyster->setValue(0);
	}
	else if (ul > uu)
	{
		ul = uu;
		sl_lowerhyster->setValue(200);
	}
	else
	{
		if (uu == ll)
			sl_lowerhyster->setValue(100);
		else
			sl_lowerhyster->setValue(int(0.5f + (ul - ll) / (uu - ll) * 200));
	}

	if (lu < ll)
	{
		lu = ll;
		sl_upperhyster->setValue(0);
	}
	else if (lu > uu)
	{
		lu = uu;
		sl_upperhyster->setValue(200);
	}
	else
	{
		if (uu == ll)
			sl_upperhyster->setValue(100);
		else
			sl_upperhyster->setValue(int(0.5f + (lu - ll) / (uu - ll) * 200));
	}

	le_bordervall->setText(QString::number(ll, 'g', 3));
	le_bordervalu->setText(QString::number(uu, 'g', 3));
	le_bordervallh->setText(
			QString::number(int(0.4f + (ul - ll) / (uu - ll) * 100), 'g', 3));
	le_bordervaluh->setText(
			QString::number(int(0.4f + (lu - ll) / (uu - ll) * 100), 'g', 3));

	return;
}

void HystereticGrowingWidget::slider_changed() { execute1(); }

void HystereticGrowingWidget::bmp_changed()
{
	bmphand = handler3D->get_activebmphandler();

	getrange();
}

QSize HystereticGrowingWidget::sizeHint() const { return vbox1->sizeHint(); }

void HystereticGrowingWidget::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand = handler3D->get_activebmphandler();
		getrange();
	}
	init1();
	hideparams_changed();
}

void HystereticGrowingWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
	getrange();

	vector<vector<Point>>* vvp = bmphand->return_limits();
	vp1.clear();
	for (vector<vector<Point>>::iterator it = vvp->begin(); it != vvp->end();
			 it++)
	{
		vp1.insert(vp1.end(), it->begin(), it->end());
		;
	}
}

void HystereticGrowingWidget::init1()
{
	limitdrawing = false;
	drawlimit->setDown(false);

	vector<vector<Point>>* vvp = bmphand->return_limits();
	vp1.clear();
	for (vector<vector<Point>>::iterator it = vvp->begin(); it != vvp->end();
			 it++)
	{
		vp1.insert(vp1.end(), it->begin(), it->end());
		;
	}

	emit vp1_changed(&vp1);
}

void HystereticGrowingWidget::cleanup()
{
	limitdrawing = false;
	drawlimit->setDown(false);
	vpdyn.clear();
	emit vp1dyn_changed(&vpdyn, &vpdyn);
	return;
}

void HystereticGrowingWidget::on_mouse_moved(Point p)
{
	if (limitdrawing)
	{
		addLine(&vpdyn, last_pt, p);
		last_pt = p;
		emit vpdyn_changed(&vpdyn);
	}
}

void HystereticGrowingWidget::on_mouse_released(Point p)
{
	if (limitdrawing)
	{
		addLine(&vpdyn, last_pt, p);
		limitdrawing = false;
		vp1.insert(vp1.end(), vpdyn.begin(), vpdyn.end());

		iseg::DataSelection dataSelection;
		dataSelection.limits = true;
		dataSelection.sliceNr = handler3D->active_slice();
		emit begin_datachange(dataSelection, this);

		bmphand->add_limit(&vpdyn);

		emit end_datachange(this);

		vpdyn.clear();
		drawlimit->setDown(false);
		emit vp1dyn_changed(&vp1, &vpdyn);
	}
}

void HystereticGrowingWidget::limitpressed()
{
	drawlimit->setDown(true);
	limitdrawing = true;
	vpdyn.clear();
	emit vpdyn_changed(&vpdyn);
}

void HystereticGrowingWidget::clearpressed()
{
	limitdrawing = false;
	drawlimit->setDown(false);

	iseg::DataSelection dataSelection;
	dataSelection.limits = true;
	dataSelection.sliceNr = handler3D->active_slice();
	emit begin_datachange(dataSelection, this);

	bmphand->clear_limits();

	emit end_datachange(this);

	vpdyn.clear();
	vp1.clear();
	emit vp1dyn_changed(&vp1, &vpdyn);
}

void HystereticGrowingWidget::slider_pressed()
{
	iseg::DataSelection dataSelection;
	dataSelection.work = dataSelection.limits = true;
	dataSelection.allSlices = allslices->isChecked();
	dataSelection.sliceNr = handler3D->active_slice();
	emit begin_datachange(dataSelection, this);
}

void HystereticGrowingWidget::slider_released() { emit end_datachange(this); }

void HystereticGrowingWidget::saveborders_execute()
{
	QString savefilename = QFileDialog::getSaveFileName(
			QString::null, "Thresholds (*.txt)\n", this); //, filename);

	if (savefilename.length() > 4 && !savefilename.endsWith(QString(".txt")))
		savefilename.append(".txt");

	if (!savefilename.isEmpty())
	{
		float ll = lower_limit +
							 (upper_limit - lower_limit) * 0.005f * sl_lower->value();
		float uu = lower_limit +
							 (upper_limit - lower_limit) * 0.005f * sl_upper->value();
		float ul = ll + (uu - ll) * 0.005f * sl_lowerhyster->value();
		float lu = ll + (uu - ll) * 0.005f * sl_upperhyster->value();
		FILE* fp = fopen(savefilename.ascii(), "w");
		fprintf(fp, "%f %f %f %f \n", ll, ul, lu, uu);
		fclose(fp);
	}
}

void HystereticGrowingWidget::loadborders_execute()
{
	QString loadfilename = QFileDialog::getOpenFileName(QString::null,
			"Borders (*.txt)\n"
			"All (*.*)",
			this);

	if (!loadfilename.isEmpty())
	{
		FILE* fp = fopen(loadfilename.ascii(), "r");
		if (fp != nullptr)
		{
			float ll, uu, lu, ul;
			if (fscanf(fp, "%f %f %f %f", &ll, &ul, &lu, &uu) == 4)
			{
				getrange_sub(ll, uu, ul, lu);
			}
		}
		fclose(fp);
	}
}

void HystereticGrowingWidget::le_bordervall_returnpressed()
{
	bool b1;
	float val = le_bordervall->text().toFloat(&b1);
	float ll =
			lower_limit + (upper_limit - lower_limit) * 0.005f * sl_lower->value();
	if (b1)
	{
		if (val < lower_limit)
		{
			ll = lower_limit;
			sl_lower->setValue(0);
			le_bordervall->setText(QString::number(ll, 'g', 3));
		}
		else if (val > upper_limit)
		{
			ll = upper_limit;
			sl_lower->setValue(200);
			le_bordervall->setText(QString::number(ll, 'g', 3));
		}
		else
		{
			if (upper_limit == lower_limit)
			{
				ll = upper_limit;
				sl_lower->setValue(100);
				le_bordervall->setText(QString::number(ll, 'g', 3));
			}
			else
				sl_lower->setValue(int(0.5f + (val - lower_limit) /
																					(upper_limit - lower_limit) *
																					200));
		}

		execute();
	}
	else
	{
		QApplication::beep();
		le_bordervall->setText(QString::number(ll, 'g', 3));
	}
}

void HystereticGrowingWidget::le_bordervalu_returnpressed()
{
	bool b1;
	float val = le_bordervalu->text().toFloat(&b1);
	float uu =
			lower_limit + (upper_limit - lower_limit) * 0.005f * sl_upper->value();
	if (b1)
	{
		if (val < lower_limit)
		{
			uu = lower_limit;
			sl_upper->setValue(0);
			le_bordervalu->setText(QString::number(uu, 'g', 3));
		}
		else if (val > upper_limit)
		{
			uu = upper_limit;
			sl_upper->setValue(200);
			le_bordervalu->setText(QString::number(uu, 'g', 3));
		}
		else
		{
			if (upper_limit == lower_limit)
			{
				uu = upper_limit;
				sl_upper->setValue(100);
				le_bordervalu->setText(QString::number(uu, 'g', 3));
			}
			else
				sl_upper->setValue(int(0.5f + (val - lower_limit) /
																					(upper_limit - lower_limit) *
																					200));
		}

		execute();
	}
	else
	{
		QApplication::beep();
		le_bordervall->setText(QString::number(uu, 'g', 3));
	}
}

void HystereticGrowingWidget::le_bordervallh_returnpressed()
{
	bool b1;
	float val = le_bordervallh->text().toFloat(&b1);
	float ll =
			lower_limit + (upper_limit - lower_limit) * 0.005f * sl_lower->value();
	float uu =
			lower_limit + (upper_limit - lower_limit) * 0.005f * sl_upper->value();
	float ul = ll + (uu - ll) * 0.005f * sl_lowerhyster->value();
	if (b1)
	{
		if (val < 0)
		{
			ul = ll;
			sl_lowerhyster->setValue(0);
			le_bordervallh->setText(
					QString::number(int((ul - ll) / (uu - ll) * 100), 'g', 3));
		}
		else if (val > 100)
		{
			ul = uu;
			sl_lowerhyster->setValue(200);
			le_bordervallh->setText(
					QString::number(int((ul - ll) / (uu - ll) * 100), 'g', 3));
		}
		else
		{
			if (uu == ll)
			{
				ul = uu;
				sl_lowerhyster->setValue(100);
				le_bordervallh->setText(
						QString::number(int((ul - ll) / (uu - ll) * 100), 'g', 3));
			}
			else
				sl_lowerhyster->setValue(int(0.5f + val / 100 * 200));
		}

		execute();
	}
	else
	{
		QApplication::beep();
		le_bordervallh->setText(
				QString::number(int((ul - ll) / (uu - ll) * 100), 'g', 3));
	}
}

void HystereticGrowingWidget::le_bordervaluh_returnpressed()
{
	bool b1;
	float val = le_bordervaluh->text().toFloat(&b1);
	float ll =
			lower_limit + (upper_limit - lower_limit) * 0.005f * sl_lower->value();
	float uu =
			lower_limit + (upper_limit - lower_limit) * 0.005f * sl_upper->value();
	float lu = ll + (uu - ll) * 0.005f * sl_upperhyster->value();
	if (b1)
	{
		if (val < 0)
		{
			lu = ll;
			sl_upperhyster->setValue(0);
			le_bordervaluh->setText(
					QString::number(int((lu - ll) / (uu - ll) * 100), 'g', 3));
		}
		else if (val > 100)
		{
			lu = uu;
			sl_upperhyster->setValue(200);
			le_bordervaluh->setText(
					QString::number(int((lu - ll) / (uu - ll) * 100), 'g', 3));
		}
		else
		{
			if (uu == ll)
			{
				lu = uu;
				sl_upperhyster->setValue(100);
				le_bordervaluh->setText(
						QString::number(int((lu - ll) / (uu - ll) * 100), 'g', 3));
			}
			else
				sl_upperhyster->setValue(int(0.5f + val / 100 * 200));
		}

		execute();
	}
	else
	{
		QApplication::beep();
		le_bordervaluh->setText(
				QString::number(int((lu - ll) / (uu - ll) * 100), 'g', 3));
	}
}

FILE* HystereticGrowingWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sl_lower->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_upper->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_lowerhyster->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_upperhyster->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(autoseed->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allslices->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&upper_limit, 1, sizeof(float), fp);
		fwrite(&lower_limit, 1, sizeof(float), fp);
	}

	return fp;
}

FILE* HystereticGrowingWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(autoseed, SIGNAL(clicked()), this,
				SLOT(auto_toggled()));
		QObject::disconnect(sl_lower, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed()));
		QObject::disconnect(sl_upper, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed()));
		QObject::disconnect(sl_lowerhyster, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed()));
		QObject::disconnect(sl_upperhyster, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sl_lower->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_upper->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_lowerhyster->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_upperhyster->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		autoseed->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		allslices->setChecked(dummy > 0);

		update_visible();

		//		auto_toggled();

		fread(&upper_limit, sizeof(float), 1, fp);
		fread(&lower_limit, sizeof(float), 1, fp);

		QObject::connect(autoseed, SIGNAL(clicked()), this,
				SLOT(auto_toggled()));
		QObject::connect(sl_lower, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed()));
		QObject::connect(sl_upper, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed()));
		QObject::connect(sl_lowerhyster, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed()));
		QObject::connect(sl_upperhyster, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed()));
	}
	return fp;
}

void HystereticGrowingWidget::hideparams_changed()
{
	if (hideparams)
	{
		pb_saveborders->hide();
		pb_loadborders->hide();
	}
	else
	{
		pb_saveborders->show();
		pb_loadborders->show();
	}
}
