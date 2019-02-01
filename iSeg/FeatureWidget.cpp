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
#include <qstring.h>
#include <qwidget.h>

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
	hbox1 = new Q3HBox(this);
	hbox1->setMargin(8);
	vbox1 = new Q3VBox(hbox1);
	vbox2 = new Q3VBox(hbox1);
	vbox3 = new Q3VBox(hbox1);

	selecting = false;

	lb_map = new QLabel("", vbox1);
	lb_av = new QLabel("Average: ", vbox1);
	lb_stddev = new QLabel("Std Dev.: ", vbox1);
	lb_min = new QLabel("Minimum: ", vbox1);
	lb_max = new QLabel("Maximum: ", vbox1);
	lb_grey = new QLabel("Grey val.:", vbox1);
	lb_pt = new QLabel("Coord.: ", vbox1);
	lb_tissue = new QLabel("Tissue: ", vbox1);
	lb_map_value = new QLabel("abcdefghijk", vbox2);
	lb_av_value = new QLabel("", vbox2);
	lb_stddev_value = new QLabel("", vbox2);
	lb_min_value = new QLabel("", vbox2);
	lb_max_value = new QLabel("", vbox2);
	lb_grey_value = new QLabel("", vbox2);
	lb_pt_value = new QLabel("", vbox2);
	lb_dummy = new QLabel("", vbox2);
	lb_work_map_value = new QLabel("xxxxxxxxxxxxxxxxxxxxxxx", vbox3);
	lb_work_av_value = new QLabel("", vbox3);
	lb_work_stddev_value = new QLabel("", vbox3);
	lb_work_min_value = new QLabel("", vbox3);
	lb_work_max_value = new QLabel("", vbox3);
	lb_work_grey_value = new QLabel("", vbox3);
	lb_work_pt_value = new QLabel("", vbox3);
	lb_tissuename = new QLabel("", vbox3);

	vbox1->setFixedSize(vbox1->sizeHint());
	vbox2->setFixedSize(vbox2->sizeHint());
	// BL: hack to make layout wide enought to show very long tissue names
	vbox3->setFixedSize(3 * vbox3->sizeHint().width(),
			vbox3->sizeHint().height());
	hbox1->setFixedSize(hbox1->sizeHint());

	lb_map_value->setText("Source:");
	lb_work_map_value->setText("Target:");
}

QSize FeatureWidget::sizeHint() const { return hbox1->sizeHint(); }

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
