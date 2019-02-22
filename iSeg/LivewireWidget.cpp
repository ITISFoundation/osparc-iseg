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

#include "Interface/LayoutTools.h"

#include "Core/ImageForestingTransform.h"
#include "Core/Pair.h"

#include <QTime>
#include <QFormLayout>

using namespace iseg;

LivewireWidget::LivewireWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("Use the Auto Trace to follow ideal contour path or draw "
										"contours around a tissue to segment it."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	drawing = false;
	lw = lwfirst = nullptr;

	// parameters
	straight = new QRadioButton(QString("Straight"));
	autotrace = new QRadioButton(QString("Auto Trace"));
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
	freedraw = new QRadioButton(QString("Free"));
	auto drawmode = make_button_group(this, {straight, autotrace, freedraw});
	autotrace->setChecked(true);

	cb_freezing = new QCheckBox;
	cb_freezing->setToolTip(Format(
			"Specify the number of seconds after which a line segment is "
			"frozen even without mouse click if it has not changed."));
	sb_freezing = new QSpinBox(1, 10, 1, nullptr);
	sb_freezing->setValue(3);

	cb_closing = new QCheckBox;
	cb_closing->setChecked(true);

	// layout
	auto method_layout = make_vbox({straight, autotrace, freedraw});
	auto method_area = new QFrame;
	method_area->setLayout(method_layout);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);
	method_area->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

	params_layout = new QFormLayout;
	params_layout->addRow("Close contour", cb_closing);
	params_layout->addRow("Freezing", cb_freezing);
	params_layout->addRow("Freezing delay (s)", sb_freezing);

	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addLayout(params_layout);

	setLayout(top_layout);

	// initialize
	cooling = false;
	cb_freezing->setChecked(cooling);

	sbfreezing_changed(sb_freezing->value());
	freezing_changed();
	mode_changed();

	// connections
	connect(drawmode, SIGNAL(buttonClicked(int)), this, SLOT(mode_changed()));
	connect(cb_freezing, SIGNAL(clicked()), this, SLOT(freezing_changed()));
	connect(sb_freezing, SIGNAL(valueChanged(int)), this, SLOT(sbfreezing_changed(int)));
}

LivewireWidget::~LivewireWidget()
{
	if (lw != nullptr)
		delete lw;
	if (lwfirst != nullptr)
		delete lwfirst;
}

void LivewireWidget::on_mouse_clicked(Point p)
{
	if (!drawing)
	{
		if (autotrace->isChecked())
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
		if (straight->isChecked())
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
	if (drawing && !freedraw->isChecked())
	{
		clicks.push_back(p);
		if (straight->isChecked())
		{
			addLine(&established, p1, p);
			addLine(&established, p2, p);
		}
		else if (autotrace->isChecked())
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
			if (straight->isChecked())
			{
				established.clear();
				dynamic.clear();
				clicks.pop_back();
				establishedlengths.pop_back();
				p1 = clicks.back();
				addLine(&dynamic, p1, p);
				auto it = clicks.begin();
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
			else if (autotrace->isChecked())
			{
				dynamic.clear();
				clicks.pop_back();
				establishedlengths.pop_back();
				auto it1 = established.begin();
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
					auto tit = times.begin();
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
	if (freedraw->isChecked() && drawing)
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
		if (freedraw->isChecked())
		{
			dynamic1.clear();
			addLine(&dynamic, p1, p);
			dynamic1.insert(dynamic1.begin(), dynamic.begin(), dynamic.end());
			addLine(&dynamic1, p2, p);
			emit vpdyn_changed(&dynamic1);
			p1 = p;
			clicks.push_back(p);
		}
		else if (straight->isChecked())
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
				std::vector<Point>::reverse_iterator rit, ritold;
				rit = dynamic.rbegin();
				ritold = dynamicold.rbegin();
				times.resize(dynamic.size());
				auto tit = times.begin();
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
	cb_freezing->setEnabled(autotrace->isChecked());
	params_layout->labelForField(cb_freezing)->setEnabled(autotrace->isChecked());
	freezing_changed();

	cb_closing->setEnabled(!freedraw->isChecked());
	params_layout->labelForField(cb_closing)->setEnabled(cb_closing->isEnabled());
}

void LivewireWidget::freezing_changed()
{
	cooling = cb_freezing->isChecked();
	sb_freezing->setEnabled(cooling && autotrace->isChecked());
	params_layout->labelForField(sb_freezing)->setEnabled(sb_freezing->isEnabled());
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
		dummy = (int)autotrace->isChecked();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)straight->isChecked();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)freedraw->isChecked();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(cb_freezing->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(cb_closing->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* LivewireWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		//disconnect(drawmode, SIGNAL(buttonClicked(int)), this, SLOT(mode_changed()));
		disconnect(cb_freezing, SIGNAL(clicked()), this, SLOT(freezing_changed()));
		disconnect(sb_freezing, SIGNAL(valueChanged(int)), this, SLOT(sbfreezing_changed(int)));

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

		//connect(drawmode, SIGNAL(buttonClicked(int)), this, SLOT(mode_changed()));
		connect(cb_freezing, SIGNAL(clicked()), this, SLOT(freezing_changed()));
		connect(sb_freezing, SIGNAL(valueChanged(int)), this, SLOT(sbfreezing_changed(int)));
	}
	return fp;
}

void LivewireWidget::hideparams_changed()
{
	mode_changed();
	//if (hideparams && !cb_freezing->isChecked())
	//	hbox2->hide();
}
