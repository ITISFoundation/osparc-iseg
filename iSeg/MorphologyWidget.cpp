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
#include "MorphologyWidget.h"
#include "SlicesHandler.h"
#include "bmp_read_1.h"

#include "Interface/Point.h"

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

using namespace iseg;

MorphologyWidget::MorphologyWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(
			Format("Apply morphological operations to the Target image. "
						 "Morphological operations are "
						 "based on expanding or shrinking (Dilate/Erode) regions by "
						 "a given number of pixel layers (n)."
						 "<br>"
						 "The functions act on the Target image (to modify a tissue "
						 "use Get Tissue and Adder)."));

	activeslice = handler3D->get_activeslice();
	bmphand = handler3D->get_activebmphandler();

	hboxoverall = new Q3HBox(this);
	vboxmethods = new Q3VBox(hboxoverall);
	vbox1 = new Q3VBox(hboxoverall);
	hbox1 = new Q3HBox(vbox1);
	hbox2 = new Q3HBox(vbox1);
	allslices = new QCheckBox(QString("Apply to all slices"), vbox1);
	btn_exec = new QPushButton("Execute", vbox1);

	txt_n = new QLabel("Pixel Layers (n): ", hbox1);
	sb_n = new QSpinBox(1, 10, 1, hbox1);
	sb_n->setValue(1);

	connectgroup = new QButtonGroup(this);
	connectgroup->insert(
			rb_4connect = new QRadioButton(QString("4-connectivity"), hbox2));
	connectgroup->insert(
			rb_8connect = new QRadioButton(QString("8-connectivity"), hbox2));
	rb_4connect->setChecked(TRUE);

	modegroup = new QButtonGroup(this);
	modegroup->insert(rb_open = new QRadioButton(QString("Open"), vboxmethods));
	modegroup->insert(rb_close =
												new QRadioButton(QString("Close"), vboxmethods));
	modegroup->insert(rb_erode =
												new QRadioButton(QString("Erode"), vboxmethods));
	modegroup->insert(rb_dilate =
												new QRadioButton(QString("Dilate"), vboxmethods));
	rb_open->setChecked(TRUE);

	rb_open->setToolTip(Format(
			"First shrinking before growing is called Open and results in the "
			"deletion of small islands and thin links between structures."));
	rb_close->setToolTip(Format("Growing followed by shrinking results in the "
															"closing of small (< 2n) gaps and holes."));
	rb_erode->setToolTip(Format(
			"Erode or shrink the boundaries of regions of foreground pixels."));
	rb_dilate->setToolTip(
			Format("Enlarge the boundaries of regions of foreground pixels."));

	vboxmethods->setMargin(5);
	vbox1->setMargin(5);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vboxmethods->setLineWidth(1);

	hbox1->setMinimumSize(hbox1->sizeHint());

	// 	vboxmethods->setFixedSize(vboxmethods->sizeHint());
	// 	hboxoverall->setFixedSize(hboxoverall->sizeHint());
	QObject::connect(btn_exec, SIGNAL(clicked()), this, SLOT(execute()));

	return;
}

MorphologyWidget::~MorphologyWidget()
{
	delete vbox1;
	delete connectgroup;
	delete modegroup;
}

void MorphologyWidget::execute()
{
	bool connect8 = rb_8connect->isOn();

	//xxxa	handler3D->regrow(handler3D->get_activeslice()-1,handler3D->get_activeslice(),sb_n->value());
	//	emit work_changed();
	//	return;

	iseg::DataSelection dataSelection;
	dataSelection.work = true;

	if (allslices->isChecked())
	{
		dataSelection.allSlices = true;
		emit begin_datachange(dataSelection, this);

		if (rb_open->isOn())
		{
			handler3D->open(sb_n->value(), connect8);
		}
		else if (rb_close->isOn())
		{
			handler3D->closure(sb_n->value(), connect8);
		}
		else if (rb_erode->isOn())
		{
			handler3D->erosion(sb_n->value(), connect8);
		}
		else
		{
			handler3D->dilation(sb_n->value(), connect8);
		}
	}
	else
	{
		dataSelection.sliceNr = handler3D->get_activeslice();
		emit begin_datachange(dataSelection, this);

		if (rb_open->isOn())
		{
			bmphand->open(sb_n->value(), connect8);
		}
		else if (rb_close->isOn())
		{
			bmphand->closure(sb_n->value(), connect8);
		}
		else if (rb_erode->isOn())
		{
			bmphand->erosion(sb_n->value(), connect8);
		}
		else
		{
			bmphand->dilation(sb_n->value(), connect8);
		}
	}
	emit end_datachange(this);
}

QSize MorphologyWidget::sizeHint() const { return vbox1->sizeHint(); }

void MorphologyWidget::on_slicenr_changed()
{
	activeslice = handler3D->get_activeslice();
	bmphand_changed(handler3D->get_activebmphandler());
}

void MorphologyWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;
}

void MorphologyWidget::init()
{
	if (activeslice != handler3D->get_activeslice())
	{
		activeslice = handler3D->get_activeslice();
		bmphand_changed(handler3D->get_activebmphandler());
	}
	hideparams_changed();
}

void MorphologyWidget::newloaded()
{
	activeslice = handler3D->get_activeslice();
	bmphand = handler3D->get_activebmphandler();
}

FILE* MorphologyWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sb_n->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_8connect->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_4connect->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_open->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_close->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_erode->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_dilate->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(allslices->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* MorphologyWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sb_n->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		rb_8connect->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_4connect->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_open->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_close->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_erode->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_dilate->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		allslices->setChecked(dummy > 0);
	}
	return fp;
}

void MorphologyWidget::hideparams_changed()
{
	if (hideparams)
	{
		hbox1->hide();
		hbox2->hide();
	}
	else
	{
		hbox1->show();
		hbox2->show();
	}
}
