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

#include "FeatureWidget.h"
#include "SlicesHandler.h"
#include "TissueInfos.h"
#include "bmp_read_1.h"
#include "StdStringToQString.h"

#include "Data/addLine.h"

#include "Core/ImageForestingTransform.h"
#include "Core/Pair.h"

#include <QGridLayout>
#include <QSpacerItem>

using namespace iseg;

FeatureWidget::FeatureWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"Obtain information about the gray value and tissue distribution."
			"<br>"
			"A rectangular area is marked by pressing down the left mouse "
			"button and moving the mouse."));

	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();

	selecting = false;

	auto layout = new QGridLayout;
	layout->addWidget(lb_map 	= new QLabel(""), 0, 0);
	layout->addWidget(lb_av 	= new QLabel("Average: "), 1, 0);
	layout->addWidget(lb_stddev = new QLabel("Std Dev.: "), 2, 0);
	layout->addWidget(lb_min 	= new QLabel("Minimum: "), 3, 0);
	layout->addWidget(lb_max 	= new QLabel("Maximum: "), 4, 0);
	layout->addWidget(lb_grey 	= new QLabel("Grey val.:"), 5, 0);
	layout->addWidget(lb_pt 	= new QLabel("Coord.: "), 6, 0);
	layout->addWidget(lb_tissue = new QLabel("Tissue: "), 7, 0);

	layout->addWidget(lb_map_value 	= new QLabel("Source"), 0, 1);
	layout->addWidget(lb_av_value  	= new QLabel(""), 1, 1);
	layout->addWidget(lb_stddev_value = new QLabel(""), 2, 1);
	layout->addWidget(lb_min_value 	= new QLabel(""), 3, 1);
	layout->addWidget(lb_max_value 	= new QLabel(""), 4, 1);
	layout->addWidget(lb_grey_value = new QLabel(""), 5, 1);
	layout->addWidget(lb_pt_value 	= new QLabel(""), 6, 1);
	layout->addWidget(lb_dummy 		= new QLabel(""), 7, 1);

	layout->addWidget(lb_work_map_value = new QLabel("Target"), 0, 2);
	layout->addWidget(lb_work_av_value 	= new QLabel(""), 1, 2);
	layout->addWidget(lb_work_stddev_value = new QLabel(""), 2, 2);
	layout->addWidget(lb_work_min_value = new QLabel(""), 3, 2);
	layout->addWidget(lb_work_max_value = new QLabel(""), 4, 2);
	layout->addWidget(lb_work_grey_value = new QLabel(""), 5, 2);
	layout->addWidget(lb_work_pt_value 	= new QLabel(""), 6, 2);
	layout->addWidget(lb_tissuename 	= new QLabel(""), 7, 2);

	setLayout(layout);
}

void FeatureWidget::on_mouse_clicked(Point p)
{
	selecting = true;
	pstart = p;
}

void FeatureWidget::on_mouse_moved(Point p)
{
	if (selecting)
	{
		Point p1;
		dynamic.clear();
		p1.px = p.px;
		p1.py = pstart.py;
		addLine(&dynamic, p1, p);
		addLine(&dynamic, p1, pstart);
		p1.px = pstart.px;
		p1.py = p.py;
		addLine(&dynamic, p1, p);
		addLine(&dynamic, p1, pstart);
		emit vpdyn_changed(&dynamic);
	}

	Pair psize = handler3D->get_pixelsize();

	lb_pt_value->setText(QString::number(handler3D->width() - 1 - p.px) +
											 QString(":") + QString::number(p.py));
	lb_work_pt_value->setText(
			QString("(") +
			QString::number((handler3D->width() - 1 - p.px) * psize.high) +
			QString(":") + QString::number(p.py * psize.low) + QString(" mm)"));
	lb_grey_value->setText(QString::number(bmphand->bmp_pt(p)));
	lb_work_grey_value->setText(QString::number(bmphand->work_pt(p)));
	tissues_size_t tnr =
			bmphand->tissues_pt(handler3D->active_tissuelayer(), p);
	if (tnr == 0)
		lb_tissuename->setText("- (0)");
	else
		lb_tissuename->setText(ToQ(TissueInfos::GetTissueName(tnr)) + QString(" (") + QString::number((int)tnr) + QString(")"));
}

void FeatureWidget::on_mouse_released(Point p)
{
	dynamic.clear();
	emit vpdyn_changed(&dynamic);

	selecting = false;
	float av = bmphand->extract_feature(pstart, p);
	float stddev = bmphand->return_stdev();
	Pair extrema = bmphand->return_extrema();

	lb_av_value->setText(QString::number(av));
	lb_stddev_value->setText(QString::number(stddev));
	lb_min_value->setText(QString::number(extrema.low));
	lb_max_value->setText(QString::number(extrema.high));

	av = bmphand->extract_featurework(pstart, p);
	stddev = bmphand->return_stdev();
	extrema = bmphand->return_extrema();

	lb_work_av_value->setText(QString::number(av));
	lb_work_stddev_value->setText(QString::number(stddev));
	lb_work_min_value->setText(QString::number(extrema.low));
	lb_work_max_value->setText(QString::number(extrema.high));
}

void FeatureWidget::on_slicenr_changed()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
}

void FeatureWidget::init()
{
	on_slicenr_changed();
}

void FeatureWidget::newloaded()
{
	activeslice = handler3D->active_slice();
	bmphand = handler3D->get_activebmphandler();
}
