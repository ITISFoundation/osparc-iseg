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

#include "PickerWidget.h"

#include "Data/addLine.h"

#include "Interface/LayoutTools.h"

#include "Core/Pair.h"

#include <QFormLayout>
#include <QKeyEvent>

#include <fstream>

using namespace iseg;

PickerWidget::PickerWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format(
			"Copy and erase regions. Copying can be used to transfer "
			"segmented regions from one slice to another. All the	"
			"functions are based on the current region selection."));

	bmphand = handler3D->get_activebmphandler();

	width = height = 0;
	mask = currentselection = nullptr;
	valuedistrib = nullptr;

	hasclipboard = false;
	shiftpressed = false;

	rb_work = new QRadioButton(tr("Target"));
	rb_tissue = new QRadioButton(tr("Tissue"));
	auto worktissuegroup = new QButtonGroup(this);
	worktissuegroup->insert(rb_work);
	worktissuegroup->insert(rb_tissue);
	rb_tissue->setChecked(true);

	rb_erase = new QRadioButton(tr("Erase"));
	rb_erase->setToolTip(Format("The deleted regions will be left empty."));
	rb_fill = new QRadioButton(tr("Fill"));
	rb_fill->setToolTip(Format("Fill the resulting hole based on the neighboring regions."));

	auto erasefillgroup = new QButtonGroup(this);
	erasefillgroup->insert(rb_erase);
	erasefillgroup->insert(rb_fill);
	rb_erase->setChecked(true);

	pb_copy = new QPushButton("Copy");
	pb_copy->setToolTip(Format("Copies an image on the clip-board."));
	pb_paste = new QPushButton("Paste");
	pb_paste->setToolTip(Format("Clip-board is pasted into the current slice"));
	pb_cut = new QPushButton("Cut");
	pb_cut->setToolTip(Format(
			"Tissues or target image pixels inside the region "
			"selection are erased but a copy "
			"is placed on the clipboard."));
	pb_delete = new QPushButton("Delete");
	pb_delete->setToolTip(Format(
			"Tissues (resp. target image pixels) inside the "
			"region selection are erased."));

	// layout
	auto layout = new QFormLayout;
	layout->addRow(rb_work, rb_tissue);
	layout->addRow(rb_erase, rb_fill);
	layout->addRow(make_hbox({pb_copy, pb_paste, pb_cut, pb_delete}));
	setLayout(layout);

	// initialize
	selection.clear();
	update_active();
	showborder();

	// connections
	connect(worktissuegroup, SIGNAL(buttonClicked(int)), this, SLOT(worktissue_changed(int)));
	connect(pb_copy, SIGNAL(clicked()), this, SLOT(copy_pressed()));
	connect(pb_paste, SIGNAL(clicked()), this, SLOT(paste_pressed()));
	connect(pb_cut, SIGNAL(clicked()), this, SLOT(cut_pressed()));
	connect(pb_delete, SIGNAL(clicked()), this, SLOT(delete_pressed()));
}

PickerWidget::~PickerWidget()
{
	delete[] mask;
	delete[] currentselection;
	delete[] valuedistrib;
}

void PickerWidget::bmphand_changed(bmphandler* bmph)
{
	bmphand = bmph;

	if (width != bmph->return_width() || height != bmph->return_height())
	{
		delete[] mask;
		delete[] currentselection;
		delete[] valuedistrib;
		width = bmph->return_width();
		height = bmph->return_height();
		unsigned int area = bmph->return_area();
		mask = new bool[area];
		currentselection = new bool[area];
		valuedistrib = new float[area];
		for (unsigned int i = 0; i < area; i++)
		{
			mask[i] = currentselection[i] = false;
		}
		hasclipboard = false;
		showborder();
	}
}

void PickerWidget::on_slicenr_changed()
{
	bmphand = handler3D->get_activebmphandler();
}

void PickerWidget::init()
{
	bmphand_changed(handler3D->get_activebmphandler());
	showborder();
}

void PickerWidget::newloaded()
{
	bmphand_changed(handler3D->get_activebmphandler());
	showborder();
}

FILE* PickerWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 6)
	{
		int dummy;
		dummy = (int)(rb_work->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_tissue->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_erase->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_fill->isChecked());
		fwrite(&(dummy), 1, sizeof(int), fp);
	}

	return fp;
}

FILE* PickerWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 6)
	{
		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		rb_work->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_tissue->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_erase->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_fill->setChecked(dummy > 0);

		worktissue_changed(0);
	}
	return fp;
}

void PickerWidget::on_mouse_clicked(Point p)
{
	if (!shiftpressed)
	{
		unsigned int area = bmphand->return_area();
		for (unsigned int i = 0; i < area; i++)
		{
			currentselection[i] = false;
		}
	}
	bool addorsub = !(currentselection[p.px + (unsigned long)(p.py) * width]);
	if (rb_work->isChecked())
		bmphand->change2mask_connectedwork(currentselection, p, addorsub);
	else
		bmphand->change2mask_connectedtissue(handler3D->active_tissuelayer(), currentselection, p, addorsub);

	showborder();
}

void PickerWidget::showborder()
{
	selection.clear();
	Point dummy;

	unsigned long pos = 0;
	for (unsigned int y = 0; y + 1 < height; y++)
	{
		for (unsigned int x = 0; x + 1 < width; x++)
		{
			if (currentselection[pos] != currentselection[pos + 1])
			{
				dummy.py = y;
				if (currentselection[pos])
					dummy.px = x;
				else
					dummy.px = x + 1;
				selection.push_back(dummy);
			}
			if (currentselection[pos] != currentselection[pos + width])
			{
				dummy.px = x;
				if (!currentselection[pos])
					dummy.py = y;
				else
					dummy.py = y + 1;
				selection.push_back(dummy);
			}
			pos++;
		}
		pos++;
	}
	pos = 0;
	for (unsigned int y = 0; y < height; y++)
	{
		if (currentselection[pos])
		{
			dummy.px = 0;
			dummy.py = y;
			selection.push_back(dummy);
		}
		pos += width - 1;
		if (currentselection[pos])
		{
			dummy.px = width - 1;
			dummy.py = y;
			selection.push_back(dummy);
		}
		pos++;
	}
	pos = width * (unsigned long)(height - 1);
	for (unsigned int x = 0; x < width; x++)
	{
		if (currentselection[x])
		{
			dummy.px = x;
			dummy.py = 0;
			selection.push_back(dummy);
		}
		if (currentselection[pos])
		{
			dummy.px = x;
			dummy.py = height - 1;
			selection.push_back(dummy);
		}
		pos++;
	}

	emit vp1_changed(&selection);
}

void PickerWidget::worktissue_changed(int) { update_active(); }

void PickerWidget::cleanup()
{
	selection.clear();
	emit vp1_changed(&selection);
}

void PickerWidget::update_active()
{
	if (hasclipboard && (rb_work->isChecked() == clipboardworkortissue))
	{
		pb_paste->show();
	}
	else
	{
		pb_paste->hide();
	}
}

void PickerWidget::copy_pressed()
{
	unsigned int area = bmphand->return_area();
	for (unsigned int i = 0; i < area; i++)
	{
		mask[i] = currentselection[i];
	}
	clipboardworkortissue = rb_work->isChecked();
	if (clipboardworkortissue)
	{
		bmphand->copy_work(valuedistrib);
		mode = bmphand->return_mode(false);
	}
	else
	{
		auto tissues = bmphand->return_tissues(handler3D->active_tissuelayer());
		for (unsigned int i = 0; i < area; i++)
		{
			valuedistrib[i] = (float)tissues[i];
		}
	}
	hasclipboard = true;
	update_active();
}

void PickerWidget::cut_pressed()
{
	copy_pressed();
	delete_pressed();
}

void PickerWidget::paste_pressed()
{
	if (clipboardworkortissue != rb_work->isChecked())
		return;
	unsigned int area = bmphand->return_area();

	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = clipboardworkortissue;
	dataSelection.tissues = !clipboardworkortissue;
	emit begin_datachange(dataSelection, this);

	if (clipboardworkortissue)
	{
		bmphand->copy2work(valuedistrib, mask, mode);
	}
	else
	{
		tissues_size_t* valuedistrib2 = new tissues_size_t[area];
		for (unsigned int i = 0; i < area; i++)
		{
			valuedistrib2[i] = (tissues_size_t)valuedistrib[i];
		}
		bmphand->copy2tissue(handler3D->active_tissuelayer(), valuedistrib2,
				mask);
		delete[] valuedistrib2;
	}

	emit end_datachange(this);
}

void PickerWidget::delete_pressed()
{
	iseg::DataSelection dataSelection;
	dataSelection.sliceNr = handler3D->active_slice();
	dataSelection.work = rb_work->isChecked();
	dataSelection.tissues = !rb_work->isChecked();
	emit begin_datachange(dataSelection, this);

	if (rb_work->isChecked())
	{
		if (rb_erase->isChecked())
			bmphand->erasework(currentselection);
		else
			bmphand->floodwork(currentselection);
	}
	else
	{
		if (rb_erase->isChecked())
			bmphand->erasetissue(handler3D->active_tissuelayer(),
					currentselection);
		else
			bmphand->floodtissue(handler3D->active_tissuelayer(),
					currentselection);
	}

	emit end_datachange(this);
}

void PickerWidget::keyPressEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Shift)
	{
		shiftpressed = true;
	}
}

void PickerWidget::keyReleaseEvent(QKeyEvent* event)
{
	if (event->key() == Qt::Key_Shift)
	{
		shiftpressed = false;
	}
}
