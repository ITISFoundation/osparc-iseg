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

#include "ImageForestingTransformRegionGrowingWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Data/addLine.h"

#include "Core/ImageForestingTransform.h"

#include <q3hbox.h>
#include <q3vbox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qslider.h>
#include <qwidget.h>

#include <algorithm>

using namespace std;
using namespace iseg;

ImageForestingTransformRegionGrowingWidget::ImageForestingTransformRegionGrowingWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"Segment multiple tissues by drawing lines in the current slice based "
			"on "
			"the Image Foresting Transform. "
			"These lines are drawn with the color of the currently selected "
			"tissue. "
			"Multiple lines of different colours can be drawn "
			"and they are subsequently used as seeds to grow regions based on a "
			"local homogeneity criterion. Through competitive growing the best "
			"boundaries "
			"between regions grown from lines with different colours are "
			"identified."
			"<br>"
			"The result is stored in the Target. To assign a segmented region to a "
			"tissue the 'Adder' must be used."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	area = 0;

	vbox1 = new Q3VBox(this);
	vbox1->setMargin(8);

	pushclear = new QPushButton("Clear Lines", vbox1);
	pushremove = new QPushButton("Remove Line", vbox1);
	pushremove->setToggleButton(true);
	pushremove->setToolTip(Format(
			"Remove Line followed by a click on a line deletes "
			"this line and automatically updates the segmentation. If Remove "
			"Line has "
			"been pressed accidentally, a second press will deactivate the "
			"function again."));

	hbox1 = new Q3HBox(vbox1);

	sl_thresh = new QSlider(Qt::Horizontal, vbox1);
	sl_thresh->setRange(0, 100);
	sl_thresh->setValue(60);
	sl_thresh->setEnabled(false);
	sl_thresh->setFixedWidth(400);

	hbox1->setFixedSize(hbox1->sizeHint());
	vbox1->setFixedSize(vbox1->sizeHint());

	IFTrg = nullptr;
	lbmap = nullptr;
	thresh = 0;

	QObject::connect(pushclear, SIGNAL(clicked()), this, SLOT(clearmarks()));
	QObject::connect(sl_thresh, SIGNAL(sliderMoved(int)), this,
			SLOT(slider_changed(int)));
	QObject::connect(sl_thresh, SIGNAL(sliderPressed()), this,
			SLOT(slider_pressed()));
	QObject::connect(sl_thresh, SIGNAL(sliderReleased()), this,
			SLOT(slider_released()));
}

ImageForestingTransformRegionGrowingWidget::~ImageForestingTransformRegionGrowingWidget()
{
	delete vbox1;

	if (IFTrg != nullptr)
		delete IFTrg;
	if (lbmap != nullptr)
		delete lbmap;
	return;
}

void ImageForestingTransformRegionGrowingWidget::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand = handler3D->get_activebmphandler();
		init1();
		if (sl_thresh->isEnabled())
		{
			getrange();
		}
	}
	else
	{
		init1();
	}

	hideparams_changed();
}

void ImageForestingTransformRegionGrowingWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
}

void ImageForestingTransformRegionGrowingWidget::init1()
{
	vector<vector<Mark>>* vvm = bmphand->return_vvm();
	vm.clear();
	for (vector<vector<Mark>>::iterator it = vvm->begin(); it != vvm->end();
			 it++)
	{
		vm.insert(vm.end(), it->begin(), it->end());
	}
	emit vm_changed(&vm);
	area = bmphand->return_height() * (unsigned)bmphand->return_width();
	if (lbmap != nullptr)
		free(lbmap);
	lbmap = (float*)malloc(sizeof(float) * area);
	for (unsigned i = 0; i < area; i++)
		lbmap[i] = 0;
	unsigned width = (unsigned)bmphand->return_width();
	for (vector<Mark>::iterator it = vm.begin(); it != vm.end(); it++)
	{
		lbmap[width * it->p.py + it->p.px] = (float)it->mark;
	}
	for (vector<Point>::iterator it = vmdyn.begin(); it != vmdyn.end(); it++)
	{
		lbmap[width * it->py + it->px] = (float)tissuenr;
	}

	if (IFTrg != nullptr)
		delete (IFTrg);
	IFTrg = bmphand->IFTrg_init(lbmap);

	thresh = 0;

	if (!vm.empty())
		sl_thresh->setEnabled(true);
}

void ImageForestingTransformRegionGrowingWidget::cleanup()
{
	vmdyn.clear();
	if (IFTrg != nullptr)
		delete IFTrg;
	if (lbmap != nullptr)
		delete lbmap;
	IFTrg = nullptr;
	lbmap = nullptr;
	sl_thresh->setEnabled(false);
	emit vpdyn_changed(&vmdyn);
	emit vm_changed(&vmempty);
}

void ImageForestingTransformRegionGrowingWidget::on_tissuenr_changed(int i)
{
	// \todo B
	std::cerr << "ImageForestingTransformRegionGrowingWidget: tissuenr = " << i << std::endl;
	tissuenr = (unsigned)i + 1;
}

void ImageForestingTransformRegionGrowingWidget::on_mouse_clicked(Point p)
{
	last_pt = p;
	if (pushremove->isOn())
	{
		removemarks(p);
	}
}

void ImageForestingTransformRegionGrowingWidget::on_mouse_moved(Point p)
{
	if (!pushremove->isOn())
	{
		addLine(&vmdyn, last_pt, p);
		last_pt = p;
		emit vpdyn_changed(&vmdyn);
	}
}

void ImageForestingTransformRegionGrowingWidget::on_mouse_released(Point p)
{
	if (!pushremove->isOn())
	{
		addLine(&vmdyn, last_pt, p);
		Mark m;
		m.mark = tissuenr;
		unsigned width = (unsigned)bmphand->return_width();
		vector<Mark> vmdummy;
		vmdummy.clear();
		for (vector<Point>::iterator it = vmdyn.begin(); it != vmdyn.end();
				 it++)
		{
			m.p = *it;
			vmdummy.push_back(m);
			lbmap[it->px + width * it->py] = tissuenr;
		}
		vm.insert(vm.end(), vmdummy.begin(), vmdummy.end());

		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		dataSelection.vvm = true;
		emit begin_datachange(dataSelection, this);

		bmphand->add_vm(&vmdummy);

		vmdyn.clear();
		emit vpdyn_changed(&vmdyn);
		emit vm_changed(&vm);
		execute();

		emit end_datachange(this);
	}
	else
	{
		pushremove->setOn(false);
	}
}

void ImageForestingTransformRegionGrowingWidget::execute()
{
	IFTrg->reinit(lbmap, false);
	if (hideparams)
		thresh = 0;
	getrange();
	float* f1 = IFTrg->return_lb();
	float* f2 = IFTrg->return_pf();
	float* work_bits = bmphand->return_work();

	float d = 255.0f / bmphand->return_vvmmaxim();
	for (unsigned i = 0; i < area; i++)
	{
		if (f2[i] < thresh)
			work_bits[i] = f1[i] * d;
		else
			work_bits[i] = 0;
	}
	sl_thresh->setEnabled(true);

	bmphand->set_mode(2, false);
}

void ImageForestingTransformRegionGrowingWidget::clearmarks()
{
	for (unsigned i = 0; i < area; i++)
		lbmap[i] = 0;

	vm.clear();
	vmdyn.clear();
	bmphand->clear_vvm();
	emit vpdyn_changed(&vmdyn);
	emit vm_changed(&vm);
}

void ImageForestingTransformRegionGrowingWidget::slider_changed(int i)
{
	thresh = i * 0.01f * maxthresh;
	if (IFTrg != nullptr)
	{
		float* f1 = IFTrg->return_lb();
		float* f2 = IFTrg->return_pf();
		float* work_bits = bmphand->return_work();

		float d = 255.0f / bmphand->return_vvmmaxim();
		for (unsigned i = 0; i < area; i++)
		{
			if (f2[i] < thresh)
				work_bits[i] = f1[i] * d;
			else
				work_bits[i] = 0;
		}
		bmphand->set_mode(2, false);
		emit end_datachange(this, iseg::NoUndo);
	}
}

void ImageForestingTransformRegionGrowingWidget::bmp_changed()
{
	bmphand = handler3D->get_activebmphandler();
	sl_thresh->setEnabled(false);
	init1();
}

void ImageForestingTransformRegionGrowingWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void ImageForestingTransformRegionGrowingWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;

	unsigned width = (unsigned)bmphand->return_width();

	vector<vector<Mark>>* vvm = bmphand->return_vvm();
	vm.clear();
	for (auto& line : *vvm)
	{
		vm.insert(vm.end(), line.begin(), line.end());
	}

	for (unsigned i = 0; i < area; i++)
		lbmap[i] = 0;
	for (auto& m : vm)
	{
		lbmap[width * m.p.py + m.p.px] = static_cast<float>(m.mark);
	}

	if (IFTrg != nullptr)
		delete (IFTrg);
	IFTrg = bmphand->IFTrg_init(lbmap);

	//	thresh=0;

	if (sl_thresh->isEnabled())
	{
		getrange();
	}

	emit vm_changed(&vm);

	return;
}

void ImageForestingTransformRegionGrowingWidget::getrange()
{
	float* pf = IFTrg->return_pf();
	maxthresh = 0;
	for (unsigned i = 0; i < area; i++)
	{
		if (maxthresh < pf[i])
		{
			maxthresh = pf[i];
		}
	}
	if (thresh > maxthresh || thresh == 0)
		thresh = maxthresh;
	if (maxthresh == 0)
		maxthresh = thresh = 1;
	sl_thresh->setValue(min(int(thresh * 100 / maxthresh), 100));

	return;
}

QSize ImageForestingTransformRegionGrowingWidget::sizeHint() const { return vbox1->sizeHint(); }

void ImageForestingTransformRegionGrowingWidget::removemarks(Point p)
{
	if (bmphand->del_vm(p, 3))
	{
		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		dataSelection.vvm = true;
		emit begin_datachange(dataSelection, this);

		vector<vector<Mark>>* vvm = bmphand->return_vvm();
		vm.clear();
		for (vector<vector<Mark>>::iterator it = vvm->begin(); it != vvm->end();
				 it++)
		{
			vm.insert(vm.end(), it->begin(), it->end());
			;
		}

		unsigned width = (unsigned)bmphand->return_width();
		for (unsigned i = 0; i < area; i++)
			lbmap[i] = 0;
		for (vector<Mark>::iterator it = vm.begin(); it != vm.end(); it++)
		{
			lbmap[width * it->p.py + it->p.px] = (float)it->mark;
		}

		emit vm_changed(&vm);
		execute();

		emit end_datachange(this);
	}
}

void ImageForestingTransformRegionGrowingWidget::slider_pressed()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);
}

void ImageForestingTransformRegionGrowingWidget::slider_released() { emit end_datachange(this); }

FILE* ImageForestingTransformRegionGrowingWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sl_thresh->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		fwrite(&thresh, 1, sizeof(float), fp);
		fwrite(&maxthresh, 1, sizeof(float), fp);
	}

	return fp;
}

FILE* ImageForestingTransformRegionGrowingWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(sl_thresh, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed(int)));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sl_thresh->setValue(dummy);
		fread(&thresh, sizeof(float), 1, fp);
		fread(&maxthresh, sizeof(float), 1, fp);

		QObject::connect(sl_thresh, SIGNAL(sliderMoved(int)), this,
				SLOT(slider_changed(int)));
	}
	return fp;
}

void ImageForestingTransformRegionGrowingWidget::hideparams_changed()
{
	if (hideparams)
	{
		sl_thresh->hide();
	}
	else
	{
		sl_thresh->show();
	}
}
