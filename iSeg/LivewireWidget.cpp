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

#include "LivewireWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/addLine.h"

#include "Core/ImageForestingTransform.h"
#include "Core/Pair.h"

#include <q3listbox.h>
#include <q3vbox.h>
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

#define UNREFERENCED_PARAMETER(P) (P)

using namespace std;
using namespace iseg;

LivewireWidget::LivewireWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("Use the Auto Trace to follow ideal contour path or draw "
										"contours around a tissue to segment it."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	hboxoverall = new Q3HBox(this);
	hboxoverall->setMargin(8);
	vboxmethods = new Q3VBox(hboxoverall);
	vbox1 = new Q3VBox(hboxoverall);

	drawing = false;
	lw = lwfirst = nullptr;

	drawmode = new QButtonGroup(this);

	straight = new QRadioButton(QString("Straight"), vboxmethods);
	autotrace = new QRadioButton(QString("Auto Trace"), vboxmethods);
	autotrace->setToolTip(Format(
			"The Livewire (intelligent scissors) algorithm "
			"to automatically identify the ideal contour path.This algorithm uses "
			"information "
			"about the strength and orientation of the(smoothed) image gradient, "
			"the zero - crossing of the Laplacian (for fine tuning) together with "
			"some weighting "
			"to favor straighter lines to determine the most likely contour path. "
			"The "
			"contouring is started by clicking the left mouse button. Each "
			"subsequent left "
			"button click fixes another point and the suggested contour line in "
			"between. "
			"<br>"
			"Successive removing of unwanted points is achieved by clicking the "
			"middle "
			"mouse button. A double left click closes the contour while a double "
			"middle "
			"click aborts the line drawing process."));
	freedraw = new QRadioButton(QString("Free"), vboxmethods);
	drawmode->insert(straight);
	drawmode->insert(autotrace);
	drawmode->insert(freedraw);
	autotrace->setChecked(TRUE);

	hbox2 = new Q3HBox(vbox1);
	cb_freezing = new QCheckBox("Freezing", hbox2);
	cb_freezing->setToolTip(
			Format("Specify the number of seconds after which a line segment is "
						 "frozen even without mouse click if it has not changed."));
	lb_freezing1 = new QLabel("Delay: ", hbox2);
	sb_freezing = new QSpinBox(1, 10, 1, hbox2);
	sb_freezing->setValue(3);
	lb_freezing2 = new QLabel("seconds", hbox2);

	hbox3 = new Q3HBox(vbox1);
	cb_closing = new QCheckBox("Close Contour", hbox3);
	cb_closing->setChecked(true);

	vboxmethods->setMargin(5);
	vbox1->setMargin(5);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);

	cooling = false;
	cb_freezing->setChecked(cooling);

	hbox2->setFixedSize(hbox2->sizeHint() + QSize(100, 0));
	hbox3->setFixedSize(hbox3->sizeHint() + QSize(100, 0));
	vbox1->setFixedSize(vbox1->sizeHint() + QSize(50, 0));

	sbfreezing_changed(sb_freezing->value());
	freezing_changed();
	mode_changed();

	QObject::connect(drawmode, SIGNAL(buttonClicked(int)), this,
			SLOT(mode_changed()));
	QObject::connect(cb_freezing, SIGNAL(clicked()), this,
			SLOT(freezing_changed()));
	QObject::connect(sb_freezing, SIGNAL(valueChanged(int)), this,
			SLOT(sbfreezing_changed(int)));

	return;
}

LivewireWidget::~LivewireWidget()
{
	delete vbox1;
	delete drawmode;
	if (lw != nullptr)
		delete lw;
	if (lwfirst != nullptr)
		delete lwfirst;
}

QSize LivewireWidget::sizeHint() const { return hboxoverall->sizeHint(); }

void LivewireWidget::on_mouse_clicked(Point p)
{
	if (!drawing)
	{
		if (autotrace->isOn())
		{
			if (lw == nullptr)
			{
				lw = bmphand->livewireinit(p);
				lwfirst = bmphand->livewireinit(p);
			}
			else
			{
				lw->change_pt(p);
				lwfirst->change_pt(p);
			}
			//		lw=bmphand->livewireinit(p);
			//		lwfirst=bmphand->livewireinit(p);
		}
		p1 = p2 = p;
		clicks.clear();
		establishedlengths.clear();
		clicks.push_back(p);
		establishedlengths.push_back(0);
		drawing = true;
		if (cooling)
		{
			dynamicold.clear();
			times.clear();
		}
	}
	else
	{
		if (straight->isOn())
		{
			addLine(&established, p1, p);
			dynamic.clear();
			if (cb_closing->isChecked())
				addLine(&dynamic, p2, p);
			p1 = p;
		}
		else
		{
			lw->return_path(p, &dynamic);
			established.insert(established.end(), dynamic.begin(),
					dynamic.end());
			if (cb_closing->isChecked())
				lwfirst->return_path(p, &dynamic);
			else
				dynamic.clear();
			lw->change_pt(p);
			if (cooling)
			{
				times.clear();
				dynamicold.clear();
			}
		}
		establishedlengths.push_back(established.size());
		clicks.push_back(p);
		//		dynamic.clear();
		emit vp1dyn_changed(&established, &dynamic);
	}
}

void LivewireWidget::pt_doubleclicked(Point p)
{
	if (drawing && !freedraw->isOn())
	{
		clicks.push_back(p);
		if (straight->isOn())
		{
			addLine(&established, p1, p);
			addLine(&established, p2, p);
		}
		else if (autotrace->isOn())
		{
			lw->return_path(p, &dynamic);
			established.insert(established.end(), dynamic.begin(),
					dynamic.end());
			lwfirst->return_path(p, &dynamic);
			established.insert(established.end(), dynamic.begin(),
					dynamic.end());
		}

		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		bmphand->fill_contour(&established, true);

		emit end_datachange(this);

		dynamic.clear();
		established.clear();
		if (cooling)
		{
			dynamicold.clear();
			times.clear();
		}
		//		clicks.clear();
		drawing = false;
		//		delete lw;
		//		delete lwfirst;
		//		lw=lwfirst=nullptr;
		emit vp1dyn_changed(&established, &dynamic);
	}
}

void LivewireWidget::pt_midclicked(Point p)
{
	if (drawing)
	{
		if (clicks.size() == 1)
		{
			pt_doubleclickedmid(p);
			return;
		}
		else
		{
			if (straight->isOn())
			{
				established.clear();
				dynamic.clear();
				clicks.pop_back();
				establishedlengths.pop_back();
				p1 = clicks.back();
				addLine(&dynamic, p1, p);
				vector<Point>::iterator it = clicks.begin();
				Point pp = p2;
				it++;
				while (it != clicks.end())
				{
					addLine(&established, pp, *it);
					pp = *it;
					it++;
				}
				if (cb_closing->isChecked())
					addLine(&dynamic, p2, p);
				emit vp1dyn_changed(&established, &dynamic);
			}
			else if (autotrace->isOn())
			{
				dynamic.clear();
				clicks.pop_back();
				establishedlengths.pop_back();
				vector<Point>::iterator it1 = established.begin();
				for (int i = 0; i < establishedlengths.back(); i++)
					it1++;
				established.erase(it1, established.end());
				p1 = clicks.back();
				lw->change_pt(p1);
				lw->return_path(p, &dynamic);
				if (cooling)
				{
					times.resize(dynamic.size());
					QTime now = QTime::currentTime();
					vector<QTime>::iterator tit = times.begin();
					while (tit != times.end())
					{
						*tit = now;
						tit++;
					}
					dynamicold.clear();
				}
				if (cb_closing->isChecked())
					lwfirst->return_path(p, &dynamic);

				emit vp1dyn_changed(&established, &dynamic);
			}
		}
	}
}

void LivewireWidget::pt_doubleclickedmid(Point p)
{
	UNREFERENCED_PARAMETER(p);
	if (drawing)
	{
		dynamic.clear();
		established.clear();
		establishedlengths.clear();
		clicks.clear();
		drawing = false;
		//		delete lw;
		//		delete lwfirst;
		//		lw=lwfirst=nullptr;
		if (cooling)
		{
			dynamicold.clear();
			times.clear();
		}
		emit vp1dyn_changed(&established, &dynamic);
	}
}

void LivewireWidget::on_mouse_released(Point p)
{
	if (freedraw->isOn() && drawing)
	{
		clicks.push_back(p);
		addLine(&dynamic, p1, p);
		addLine(&dynamic, p2, p);

		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);

		bmphand->fill_contour(&dynamic, true);

		emit end_datachange(this);

		dynamic.clear();
		dynamic1.clear();
		established.clear();
		//		clicks.clear();
		drawing = false;
		//		delete lw;
		//		delete lwfirst;
		//		lw=lwfirst=nullptr;
		emit vp1dyn_changed(&established, &dynamic);
	}
}

void LivewireWidget::on_mouse_moved(Point p)
{
	if (drawing)
	{
		if (freedraw->isOn())
		{
			dynamic1.clear();
			addLine(&dynamic, p1, p);
			dynamic1.insert(dynamic1.begin(), dynamic.begin(), dynamic.end());
			addLine(&dynamic1, p2, p);
			emit vpdyn_changed(&dynamic1);
			p1 = p;
			clicks.push_back(p);
		}
		else if (straight->isOn())
		{
			dynamic.clear();
			addLine(&dynamic, p1, p);
			if (cb_closing->isChecked())
				addLine(&dynamic, p2, p);
			emit vpdyn_changed(&dynamic);
		}
		else
		{
			lw->return_path(p, &dynamic);
			if (cb_closing->isChecked())
				lwfirst->return_path(p, &dynamic1);

			if (cooling)
			{
				vector<Point>::reverse_iterator rit, ritold;
				rit = dynamic.rbegin();
				ritold = dynamicold.rbegin();
				times.resize(dynamic.size());
				vector<QTime>::iterator tit = times.begin();
				while (rit != dynamic.rend() && ritold != dynamicold.rend() &&
							 (*rit).px == (*ritold).px && (*rit).py == (*ritold).py)
				{
					rit++;
					ritold++;
					tit++;
				}

				QTime now = QTime::currentTime();
				while (tit != times.end())
				{
					*tit = now;
					tit++;
				}

				dynamicold.clear();
				dynamicold.insert(dynamicold.begin(), dynamic.begin(),
						dynamic.end());

				/*FILE *fp3=fopen("D:\\Development\\segmentation\\sample images\\test100.txt","w");
				for(tit=times.begin();tit!=times.end();tit++)
					fprintf(fp3,"%f\n",(float)(*tit-now)/float(tlimit1)*3);
				fclose(fp3);*/

				tit = times.begin();

				if (tit->msecsTo(now) >= tlimit2)
				{
					rit = dynamic.rbegin();
					while (tit != times.end() && tit->msecsTo(now) >= tlimit1)
					{
						tit++;
						rit++;
					}

					rit--;
					lw->change_pt(*rit);
					rit++;
					established.insert(established.end(), dynamic.rbegin(),
							rit);
					times.erase(times.begin(), tit);
					//					dynamic.erase(dynamic.rbegin(),rit);

					if (cb_closing->isChecked())
						dynamic.insert(dynamic.end(), dynamic1.begin(),
								dynamic1.end());
					emit vp1dyn_changed(&established, &dynamic);
				}
				else
				{
					if (cb_closing->isChecked())
						dynamic.insert(dynamic.end(), dynamic1.begin(),
								dynamic1.end());
					emit vpdyn_changed(&dynamic);
				}
			}
			else
			{
				if (cb_closing->isChecked())
					dynamic.insert(dynamic.end(), dynamic1.begin(),
							dynamic1.end());
				emit vpdyn_changed(&dynamic);
			}
		}
	}
}

void LivewireWidget::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand = handler3D->get_activebmphandler();

		dynamic.clear();
		established.clear();
		clicks.clear();
		establishedlengths.clear();

		if (cooling)
		{
			dynamicold.clear();
			times.clear();
		}

		if (lw != nullptr)
		{
			delete lw;
			lw = nullptr;
		}
		if (lwfirst != nullptr)
		{
			delete lwfirst;
			lwfirst = nullptr;
		}

		emit vp1dyn_changed(&established, &dynamic);
	}

	init1();

	hideparams_changed();

	return;
}

void LivewireWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	dynamic.clear();
	established.clear();
	clicks.clear();
	establishedlengths.clear();

	if (cooling)
	{
		dynamicold.clear();
		times.clear();
	}

	if (lw != nullptr)
	{
		delete lw;
		lw = nullptr;
	}
	if (lwfirst != nullptr)
	{
		delete lwfirst;
		lwfirst = nullptr;
	}
}

void LivewireWidget::init1()
{
	Point p;
	p.px = p.py = 0;
	/*	lw=bmphand->livewireinit(p);
	lwfirst=bmphand->livewireinit(p);*/
	//	lwfirst=new IFT_livewire;
	//	lw->lw_init(bmphand->return_width(),bmphand->return_height(),lw->return_E(),lw->return_directivity(),p);
	drawing = false;
	//	isactive=true;
}

void LivewireWidget::cleanup()
{
	dynamic.clear();
	established.clear();
	clicks.clear();
	establishedlengths.clear();

	if (cooling)
	{
		dynamicold.clear();
		times.clear();
	}

	drawing = false;
	if (lw != nullptr)
		delete lw;
	if (lw != nullptr)
		delete lwfirst;
	lw = lwfirst = nullptr;
	emit vp1dyn_changed(&established, &dynamic);
	//	isactive=false;
}

void LivewireWidget::bmp_changed()
{
	cleanup();
	bmphand = handler3D->get_activebmphandler();
	init1();
}

void LivewireWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void LivewireWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;

	dynamic.clear();
	established.clear();
	clicks.clear();
	establishedlengths.clear();

	if (cooling)
	{
		dynamicold.clear();
		times.clear();
	}

	if (lw != nullptr)
	{
		delete lw;
		lw = nullptr;
	}
	if (lwfirst != nullptr)
	{
		delete lwfirst;
		lwfirst = nullptr;
	}

	init1();

	emit vp1dyn_changed(&established, &dynamic);
}

void LivewireWidget::mode_changed()
{
	if (autotrace->isOn())
	{
		hbox2->show();
	}
	else
	{
		hbox2->hide();
	}

	if (freedraw->isOn())
	{
		hbox3->hide();
	}
	else
	{
		hbox3->show();
	}
}

void LivewireWidget::freezing_changed()
{
	if (cb_freezing->isChecked())
	{
		cooling = true;
		sb_freezing->show();
		lb_freezing1->show();
		lb_freezing2->show();
	}
	else
	{
		cooling = false;
		sb_freezing->hide();
		lb_freezing1->hide();
		lb_freezing2->hide();
	}
}

void LivewireWidget::sbfreezing_changed(int i)
{
	tlimit1 = i * 1000;
	tlimit2 = (float(i) + 0.5f) * 1000;
}

FILE* LivewireWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sb_freezing->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)autotrace->isOn();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)straight->isOn();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)freedraw->isOn();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(cb_freezing->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(cb_closing->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* LivewireWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(drawmode, SIGNAL(buttonClicked(int)), this,
				SLOT(mode_changed()));
		QObject::disconnect(cb_freezing, SIGNAL(clicked()), this,
				SLOT(freezing_changed()));
		QObject::disconnect(sb_freezing, SIGNAL(valueChanged(int)), this,
				SLOT(sbfreezing_changed(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sb_freezing->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		autotrace->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		straight->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		freedraw->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		cb_freezing->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		cb_closing->setChecked(dummy > 0);

		sbfreezing_changed(sb_freezing->value());
		freezing_changed();
		mode_changed();

		QObject::connect(drawmode, SIGNAL(buttonClicked(int)), this,
				SLOT(mode_changed()));
		QObject::connect(cb_freezing, SIGNAL(clicked()), this,
				SLOT(freezing_changed()));
		QObject::connect(sb_freezing, SIGNAL(valueChanged(int)), this,
				SLOT(sbfreezing_changed(int)));
	}
	return fp;
}

void LivewireWidget::hideparams_changed()
{
	mode_changed();
	if (hideparams && !cb_freezing->isChecked())
		hbox2->hide();
}
