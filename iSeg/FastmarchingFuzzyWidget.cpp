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

#include "FastmarchingFuzzyWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Core/ImageForestingTransform.h"

#include "Interface/LayoutTools.h"

#include <QFormLayout>
#include <QStackedLayout>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qslider.h>
#include <qspinbox.h>

using namespace iseg;

namespace {
QWidget* add_to_widget(QLayout* layout)
{
	auto widget = new QWidget;
	widget->setLayout(layout);
	return widget;
}
}

FastmarchingFuzzyWidget::FastmarchingFuzzyWidget(SlicesHandler* hand3D,
		QWidget* parent, const char* name,
		Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"The Fuzzy tab actually provides access to two different "
			"segmentation techniques that have a very different "
			"background but a similar user and interaction interface : "
			"1) Fuzzy connectedness and 2) a Fast Marching "
			"implementation of a levelset technique. Both methods "
			"require a seed point to be specified."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	area = 0;

	sl_sigma = new QSlider(Qt::Horizontal, nullptr);
	sl_sigma->setRange(0, 100);
	sl_sigma->setValue(int(sigma / sigmamax * 100));
	sl_thresh = new QSlider(Qt::Horizontal, nullptr);
	sl_thresh->setRange(0, 100);

	sl_m1 = new QSlider(Qt::Horizontal, nullptr);
	sl_m1->setRange(0, 100);
	sl_m1->setToolTip(Format("The average gray value of the region to be segmented."));

	sl_s1 = new QSlider(Qt::Horizontal, nullptr);
	sl_s1->setRange(0, 100);
	sl_s1->setToolTip(Format(
			"A measure of how much the gray values are expected "
			"to deviate from m1 (standard deviation)."));

	sl_s2 = new QSlider(Qt::Horizontal, nullptr);
	sl_s2->setRange(0, 100);
	sl_s2->setToolTip(Format(
			"Sudden changes larger than s2 are considered to "
			"indicate boundaries."));

	rb_fastmarch = new QRadioButton(tr("Fast Marching"));
	rb_fastmarch->setToolTip(Format(
			"Fuzzy connectedness computes for each image "
			"point the likelihood of its belonging to the same region as the "
			"starting "
			"point. It achieves this by looking at each possible connecting path "
			"between the two points and assigning the image point probability as "
			"identical to the probability of this path lying entirely in the same "
			"tissue "
			"as the start point."));
	rb_fuzzy = new QRadioButton(tr("Fuzzy Connect."));
	rb_fuzzy->setToolTip(Format(
			"This tool simulates the evolution of a line (boundary) on a 2D "
			"image in time. "
			"The boundary is continuously expanding. The Sigma parameter "
			"(Gaussian smoothing) controls the impact of noise."));

	auto bg_method = make_button_group(this, {rb_fastmarch, rb_fuzzy});
	rb_fastmarch->setChecked(true);

	rb_drag = new QRadioButton(tr("Drag"));
	rb_drag->setToolTip(Format(
			"Drag allows the user to drag the mouse (after clicking on "
			"the start point and keeping the mouse button pressed down) "
			"specifying the "
			"extent of the region on the image."));
	rb_drag->setChecked(true);
	rb_drag->show();
	rb_slider = new QRadioButton(tr("Slider"));
	rb_slider->setToolTip(Format("Increase or decrease the region size."));

	auto bg_interact = new QButtonGroup(this);
	bg_interact->insert(rb_drag);
	bg_interact->insert(rb_slider);

	sl_extend = new QSlider(Qt::Horizontal, nullptr);
	sl_extend->setRange(0, 200);
	sl_extend->setValue(0);

	sb_thresh = new QSpinBox(10, 100, 10, nullptr);
	sb_thresh->setValue(30);
	sb_m1 = new QSpinBox(50, 1000, 50, nullptr);
	sb_m1->setValue(200);
	sb_s1 = new QSpinBox(50, 500, 50, nullptr);
	sb_s1->setValue(100);
	sb_s2 = new QSpinBox(10, 200, 10, nullptr);
	sb_s2->setValue(100);

	// layout
	auto method_layout = new QVBoxLayout;
	method_layout->addWidget(rb_fastmarch);
	method_layout->addWidget(rb_fuzzy);

	auto method_area = new QFrame;
	method_area->setLayout(method_layout);
	method_area->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	method_area->setLineWidth(1);

	auto fm_params = new QFormLayout;
	fm_params->addRow(tr("Sigma"), sl_sigma);
	fm_params->addRow(tr("Thresh"), make_hbox({sl_thresh, sb_thresh}));

	auto fuzzy_params = new QFormLayout;
	fuzzy_params->addRow(tr("m1"), make_hbox({sl_m1, sb_m1}));
	fuzzy_params->addRow(tr("s1"), make_hbox({sl_s1, sb_s1}));
	fuzzy_params->addRow(tr("s2"), make_hbox({sl_s2, sb_s2}));

	params_stack_layout = new QStackedLayout;
	params_stack_layout->addWidget(add_to_widget(fm_params));
	params_stack_layout->addWidget(add_to_widget(fuzzy_params));

	auto interact_layout = new QVBoxLayout;
	interact_layout->addWidget(rb_drag);
	interact_layout->addLayout(make_hbox({rb_slider, sl_extend}));

	auto params_layout = new QVBoxLayout;
	params_layout->addLayout(params_stack_layout);
	params_layout->addLayout(interact_layout);

	auto top_layout = new QHBoxLayout;
	top_layout->addWidget(method_area);
	top_layout->addLayout(params_layout);

	setLayout(top_layout);

	// connections
	connect(sl_extend, SIGNAL(valueChanged(int)), this, SLOT(slextend_changed(int)));
	connect(sl_extend, SIGNAL(sliderPressed()), this, SLOT(slextend_pressed()));
	connect(sl_extend, SIGNAL(sliderReleased()), this, SLOT(slextend_released()));
	connect(sl_sigma, SIGNAL(sliderMoved(int)), this, SLOT(slider_changed()));
	connect(sl_thresh, SIGNAL(sliderMoved(int)), this, SLOT(slider_changed()));
	connect(sl_m1, SIGNAL(sliderMoved(int)), this, SLOT(slider_changed()));
	connect(sl_s1, SIGNAL(sliderMoved(int)), this, SLOT(slider_changed()));
	connect(sl_s2, SIGNAL(sliderMoved(int)), this, SLOT(slider_changed()));

	connect(bg_method, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
	connect(bg_interact, SIGNAL(buttonClicked(int)), this, SLOT(interact_changed()));

	connect(sb_thresh, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
	connect(sb_m1, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
	connect(sb_s1, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
	connect(sb_s2, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));

	IFTmarch = nullptr;
	IFTfuzzy = nullptr;
	sigma = 1.0f;
	sigmamax = 5.0f;
	thresh = 100.0f;
	m1 = 110.0f;
	s1 = 20.0f;
	s2 = 50.0f;
	extend = 0.5f;

	method_changed();
	spinbox_changed();
	sl_sigma->setValue(int(sigma * 100 / sigmamax));
}

FastmarchingFuzzyWidget::~FastmarchingFuzzyWidget()
{
	if (IFTmarch != nullptr)
		delete IFTmarch;
	if (IFTfuzzy != nullptr)
		delete IFTfuzzy;
}

void FastmarchingFuzzyWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void FastmarchingFuzzyWidget::bmphand_changed(bmphandler* bmph)
{
	if (IFTmarch != nullptr)
		delete IFTmarch;
	if (IFTfuzzy != nullptr)
		delete IFTfuzzy;
	IFTmarch = nullptr;
	IFTfuzzy = nullptr;

	bmphand = bmph;

	sl_extend->setEnabled(false);
}

void FastmarchingFuzzyWidget::init()
{
	if (activeslice != handler3D->active_slice())
	{
		activeslice = handler3D->active_slice();
		bmphand = handler3D->get_activebmphandler();
	}

	area = bmphand->return_height() * (unsigned)bmphand->return_width();

	if (IFTmarch != nullptr)
		delete IFTmarch;
	if (IFTfuzzy != nullptr)
		delete IFTfuzzy;
	IFTmarch = nullptr;
	IFTfuzzy = nullptr;

	sl_extend->setEnabled(false);

	hideparams_changed();
}

void FastmarchingFuzzyWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
}

void FastmarchingFuzzyWidget::cleanup()
{
	if (IFTmarch != nullptr)
		delete IFTmarch;
	if (IFTfuzzy != nullptr)
		delete IFTfuzzy;
	IFTmarch = nullptr;
	IFTfuzzy = nullptr;
	sl_extend->setEnabled(false);
}

void FastmarchingFuzzyWidget::getrange()
{
	extendmax = 0;
	for (unsigned i = 0; i < area; i++)
		if (extendmax < map[i])
			extendmax = map[i];
}

void FastmarchingFuzzyWidget::on_mouse_clicked(Point p)
{
	if (rb_fastmarch->isChecked())
	{
		if (IFTmarch != nullptr)
			delete IFTmarch;

		IFTmarch = bmphand->fastmarching_init(p, sigma, thresh);

		//		if(map!=nullptr) free(map);
		map = IFTmarch->return_pf();
	}
	else
	{
		if (IFTfuzzy == nullptr)
		{
			IFTfuzzy = new ImageForestingTransformAdaptFuzzy;
			IFTfuzzy->fuzzy_init(bmphand->return_width(),
					bmphand->return_height(),
					bmphand->return_bmp(), p, m1, s1, s2);
		}
		else
		{
			IFTfuzzy->change_param(m1, s1, s2);
			IFTfuzzy->change_pt(p);
		}

		//		if(map!=nullptr) free(map);
		map = IFTfuzzy->return_pf();
	}

	if (rb_slider->isChecked())
	{
		getrange();
		if (extend > extendmax)
		{
			extend = extendmax;
		}

		sl_extend->setValue(int(extend / extendmax * 200));
		sl_extend->setEnabled(true);

		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);
		execute();
		emit end_datachange(this);
	}
}

void FastmarchingFuzzyWidget::on_mouse_released(Point p)
{
	if (rb_drag->isChecked())
	{
		vpdyn_arg.clear();
		emit vpdyn_changed(&vpdyn_arg);
		extend = map[unsigned(bmphand->return_width()) * p.py + p.px];

		iseg::DataSelection dataSelection;
		dataSelection.sliceNr = handler3D->active_slice();
		dataSelection.work = true;
		emit begin_datachange(dataSelection, this);
		execute();
		emit end_datachange(this);
		/*		float *workbits=bmphand->return_work();
		for(unsigned i=0;i<area;i++) {
			if(map[i]<extend) workbits=255.0f;
			else workbits=0.0f;
		}*/
	}
}

void FastmarchingFuzzyWidget::on_mouse_moved(Point p)
{
	if (rb_drag->isChecked())
	{
		vpdyn_arg.clear();
		unsigned short width = bmphand->return_width();
		unsigned short height = bmphand->return_height();
		extend = map[(unsigned)width * p.py + p.px];

		Point p;

		unsigned pos = 0;

		if ((map[pos] <= extend && map[pos + 1] > extend) ||
				(map[pos] <= extend && map[pos + width] > extend))
		{
			p.px = 0;
			p.py = 0;
			vpdyn_arg.push_back(p);
		}
		pos++;
		for (unsigned short j = 1; j + 1 < width; j++)
		{
			if ((map[pos] <= extend && map[pos + 1] > extend) ||
					(map[pos] <= extend && map[pos - 1] > extend) ||
					(map[pos] <= extend && map[pos + width] > extend))
			{
				p.px = j;
				p.py = 0;
				vpdyn_arg.push_back(p);
			}
			pos++;
		}
		if ((map[pos] <= extend && map[pos - 1] > extend) ||
				(map[pos] <= extend && map[pos + width] > extend))
		{
			p.px = width - 1;
			p.py = 0;
			vpdyn_arg.push_back(p);
		}
		pos++;

		for (unsigned short i = 1; i + 1 < height; i++)
		{
			if ((map[pos] <= extend && map[pos + 1] > extend) ||
					(map[pos] <= extend && map[pos + width] > extend) ||
					(map[pos] <= extend && map[pos - width] > extend))
			{
				p.px = 0;
				p.py = i;
				vpdyn_arg.push_back(p);
			}
			pos++;
			for (unsigned short j = 1; j + 1 < width; j++)
			{
				if ((map[pos] <= extend && map[pos + 1] > extend) ||
						(map[pos] <= extend && map[pos - 1] > extend) ||
						(map[pos] <= extend && map[pos + width] > extend) ||
						(map[pos] <= extend && map[pos - width] > extend))
				{
					p.px = j;
					p.py = i;
					vpdyn_arg.push_back(p);
				}
				pos++;
			}
			if ((map[pos] <= extend && map[pos + 1] > extend) ||
					(map[pos] <= extend && map[pos + width] > extend) ||
					(map[pos] <= extend && map[pos - width] > extend))
			{
				p.px = width - 1;
				p.py = i;
				vpdyn_arg.push_back(p);
			}
			pos++;
		}
		if ((map[pos] <= extend && map[pos + 1] > extend) ||
				(map[pos] <= extend && map[pos - width] > extend))
		{
			p.px = 0;
			p.py = height - 1;
			vpdyn_arg.push_back(p);
		}
		pos++;
		for (unsigned short j = 1; j + 1 < width; j++)
		{
			if ((map[pos] <= extend && map[pos + 1] > extend) ||
					(map[pos] <= extend && map[pos - 1] > extend) ||
					(map[pos] <= extend && map[pos - width] > extend))
			{
				p.px = j;
				p.py = height - 1;
				vpdyn_arg.push_back(p);
			}
			pos++;
		}
		if ((map[pos] <= extend && map[pos - 1] > extend) ||
				(map[pos] <= extend && map[pos - width] > extend))
		{
			p.px = width - 1;
			p.py = height - 1;
			vpdyn_arg.push_back(p);
		}

		emit vpdyn_changed(&vpdyn_arg);
		//		execute();
		/*		float *workbits=bmphand->return_work();
		for(unsigned i=0;i<area;i++) {
			if(map[i]<extend) workbits=255.0f;
			else workbits=0.0f;
		}*/
	}
}

void FastmarchingFuzzyWidget::execute()
{
	float* workbits = bmphand->return_work();
	for (unsigned i = 0; i < area; i++)
	{
		//		workbits[i]=map[i];
		if (map[i] <= extend)
			workbits[i] = 255.0f;
		else
			workbits[i] = 0.0f;
	}
	bmphand->set_mode(2, false);
}

void FastmarchingFuzzyWidget::slextend_changed(int val)
{
	extend = val * 0.005f * extendmax;
	if (rb_slider->isChecked())
		execute();

	return;
}

void FastmarchingFuzzyWidget::bmp_changed()
{
	bmphand = handler3D->get_activebmphandler();
	//	sl_extend->setEnabled(false);
	init();
}

void FastmarchingFuzzyWidget::method_changed()
{
	if (rb_fastmarch->isChecked())
	{
		params_stack_layout->setCurrentIndex(0);
	}
	else
	{
		params_stack_layout->setCurrentIndex(1);
	}
}

void FastmarchingFuzzyWidget::interact_changed()
{
	if (rb_drag->isChecked())
	{
		sl_extend->setVisible(false);
	}
	else
	{
		sl_extend->setVisible(true);
	}
}

void FastmarchingFuzzyWidget::spinbox_changed()
{
	if (thresh > sb_thresh->value())
	{
		thresh = sb_thresh->value();
		sl_thresh->setValue(100);
		if (IFTmarch != nullptr)
		{
			delete IFTmarch;
			IFTmarch = nullptr;
		}
		sl_extend->setEnabled(false);
	}
	else
	{
		sl_thresh->setValue(int(thresh * 100 / sb_thresh->value()));
	}

	if (m1 > sb_m1->value())
	{
		m1 = sb_m1->value();
		sl_m1->setValue(100);
		sl_extend->setEnabled(false);
	}
	else
	{
		sl_m1->setValue(int(m1 * 100 / sb_m1->value()));
	}

	if (s1 > sb_s1->value())
	{
		s1 = sb_s1->value();
		sl_s1->setValue(100);
		sl_extend->setEnabled(false);
	}
	else
	{
		sl_s1->setValue(int(s1 * 100 / sb_s1->value()));
	}

	if (s2 > sb_s2->value())
	{
		s2 = sb_s2->value();
		sl_s2->setValue(100);
		sl_extend->setEnabled(false);
	}
	else
	{
		sl_s2->setValue(int(s2 * 100 / sb_s2->value()));
	}

	return;
}

void FastmarchingFuzzyWidget::slider_changed()
{
	sigma = sl_sigma->value() * 0.01f * sigmamax;
	thresh = sl_thresh->value() * 0.01f * sb_thresh->value();
	m1 = sl_m1->value() * 0.01f * sb_m1->value();
	s1 = sl_s1->value() * 0.01f * sb_s1->value();
	s2 = sl_s2->value() * 0.01f * sb_s2->value();

	if (rb_fastmarch->isChecked() && IFTmarch != nullptr)
	{
		delete IFTmarch;
		IFTmarch = nullptr;
	}

	sl_extend->setEnabled(false);

	return;
}

void FastmarchingFuzzyWidget::slextend_pressed()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = true;
	emit begin_datachange(dataSelection, this);
}

void FastmarchingFuzzyWidget::slextend_released() { emit end_datachange(this); }

FILE* FastmarchingFuzzyWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sl_sigma->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_thresh->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_m1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_s1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_s2->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sl_extend->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_thresh->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_m1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_s1->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = sb_s2->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_fastmarch->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_fuzzy->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_drag->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_slider->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);

		fwrite(&sigma, 1, sizeof(float), fp);
		fwrite(&thresh, 1, sizeof(float), fp);
		fwrite(&m1, 1, sizeof(float), fp);
		fwrite(&s1, 1, sizeof(float), fp);
		fwrite(&s2, 1, sizeof(float), fp);
		fwrite(&sigmamax, 1, sizeof(float), fp);
		fwrite(&extend, 1, sizeof(float), fp);
		fwrite(&extendmax, 1, sizeof(float), fp);
	}

	return fp;
}

FILE* FastmarchingFuzzyWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		disconnect(sl_extend, SIGNAL(valueChanged(int)), this, SLOT(slextend_changed(int)));
		disconnect(sb_thresh, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
		disconnect(sb_m1, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
		disconnect(sb_s1, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
		disconnect(sb_s2, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sl_sigma->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_thresh->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_m1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_s1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_s2->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sl_extend->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_thresh->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_m1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_s1->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		sb_s2->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		rb_fastmarch->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_fuzzy->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_drag->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_slider->setChecked(dummy > 0);

		fread(&sigma, sizeof(float), 1, fp);
		fread(&thresh, sizeof(float), 1, fp);
		fread(&m1, sizeof(float), 1, fp);
		fread(&s1, sizeof(float), 1, fp);
		fread(&s2, sizeof(float), 1, fp);
		fread(&sigmamax, sizeof(float), 1, fp);
		fread(&extend, sizeof(float), 1, fp);
		fread(&extendmax, sizeof(float), 1, fp);

		method_changed();
		interact_changed();

		connect(sl_extend, SIGNAL(valueChanged(int)), this, SLOT(slextend_changed(int)));
		connect(sb_thresh, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
		connect(sb_m1, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
		connect(sb_s1, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
		connect(sb_s2, SIGNAL(valueChanged(int)), this, SLOT(spinbox_changed()));
	}
	return fp;
}

void FastmarchingFuzzyWidget::hideparams_changed()
{
	method_changed();
	interact_changed();
}
