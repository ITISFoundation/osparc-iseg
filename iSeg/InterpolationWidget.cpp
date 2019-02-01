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

#include "InterpolationWidget.h"
#include "SlicesHandler.h"

#include "Data/BrushInteraction.h"

#include <q3vbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qdialog.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qspinbox.h>
#include <qwidget.h>

#include <algorithm>

using namespace iseg;

InterpolationWidget::InterpolationWidget(SlicesHandler* hand3D, QWidget* parent,
		const char* name, Qt::WindowFlags wFlags)
		: WidgetInterface(parent, name, wFlags), handler3D(hand3D)
{
	setToolTip(Format("Interpolate/extrapolate between segmented slices."));

	brush = nullptr;

	nrslices = handler3D->num_slices();

	hboxoverall = new Q3HBox(this);
	hboxoverall->setMargin(8);
	vboxmethods = new Q3VBox(hboxoverall);
	vboxdataselect = new Q3VBox(hboxoverall);
	vboxparams = new Q3VBox(hboxoverall);
	vboxexecute = new Q3VBox(hboxoverall);

	// Methods
	vboxmethods->layout()->setAlignment(Qt::AlignTop);
	rb_inter = new QRadioButton(QString("Interpolation"), vboxmethods);
	rb_extra = new QRadioButton(QString("Extrapolation"), vboxmethods);
	rb_batchinter = new QRadioButton(QString("Batch Interpol."), vboxmethods);
	rb_batchinter->setToolTip(Format(
			"Select the stride of (distance between) segmented slices and the "
			"start slice to batch segment multiple slices."));
	modegroup = new QButtonGroup(this);
	modegroup->insert(rb_inter);
	modegroup->insert(rb_extra);
	modegroup->insert(rb_batchinter);
	rb_inter->setChecked(TRUE);

	// Data selection
	vboxdataselect->layout()->setAlignment(Qt::AlignTop);
	rb_tissue = new QRadioButton(QString("Selected Tissue"), vboxdataselect);
	rb_tissue->setToolTip(Format(
			"Works on the tissue distribution of the currently selected tissue."));
	rb_tissueall = new QRadioButton(QString("All Tissues"), vboxdataselect);
	rb_tissueall->setToolTip(Format(
			"Works on all tissue at once (not available for extrapolation)."));
	rb_work = new QRadioButton(QString("TargetPict"), vboxdataselect);
	rb_work->setToolTip(Format("Works on the Target image."));

	sourcegroup = new QButtonGroup(this);
	sourcegroup->insert(rb_tissue);
	sourcegroup->insert(rb_tissueall);
	sourcegroup->insert(rb_work);
	rb_tissue->setChecked(TRUE);

	// Parameters
	vboxparams->layout()->setAlignment(Qt::AlignTop);

	hboxextra = new Q3HBox(vboxparams);
	txt_slicenr = new QLabel("Target Slice: ", hboxextra);
	sb_slicenr = new QSpinBox(1, nrslices, 1, hboxextra);
	sb_slicenr->setValue(1);
	sb_slicenr->setToolTip(Format("Set the slice index where the Tissue/Target "
																"distribution will be extrapolated."));

	hboxbatch = new Q3HBox(vboxparams);
	txt_batchstride = new QLabel("Stride: ", hboxbatch);
	sb_batchstride = new QSpinBox(2, nrslices - 1, 1, hboxbatch);
	sb_batchstride->setValue(2);
	sb_batchstride->setToolTip(Format(
			"The stride is the distance between segmented slices. For example, "
			"you may segment every fifth slice (stride=5), then interpolate in "
			"between."));

	cb_connectedshapebased = new QCheckBox(QString("Connected Shape-Based"), vboxparams);
	cb_connectedshapebased->setToolTip(Format(
			"Align corresponding foreground objects by their center of mass, "
			"to ensure shape-based interpolation connects these objects"));

	cb_medianset = new QCheckBox(QString("Median Set"), vboxparams);
	cb_medianset->setChecked(false);
	cb_medianset->setToolTip(Format(
			"If Median Set is ON, the algorithm described in [1] "
			"is employed. Otherwise shape-based interpolation [2] is used. "
			"Multiple "
			"objects (with "
			"different gray values or tissue assignments, respectively) can be "
			"interpolated jointly "
			"without introducing gaps or overlap.<br>"
			"[1] S. Beucher. Sets, partitions and functions interpolations. "
			"1998.<br>"
			"[2] S. P. Raya and J. K. Udupa. Shape-based interpolation of "
			"multidimensional objects. 1990."));
	rb_4connectivity = new QRadioButton(QString("4-connectivity"), vboxparams);
	rb_8connectivity = new QRadioButton(QString("8-connectivity"), vboxparams);
	connectivitygroup = new QButtonGroup(this);
	connectivitygroup->insert(rb_4connectivity);
	connectivitygroup->insert(rb_8connectivity);
	rb_8connectivity->setChecked(TRUE);

	brush_radius = new QLineEdit(QString::number(1));
	brush_radius->setValidator(new QDoubleValidator);
	brush_radius->setToolTip(Format("Set the radius of the brush in physical units, i.e. typically mm."));
	
	auto brush_param = new QWidget(vboxparams);
	auto brush_layout = new QHBoxLayout;
	brush_layout->addWidget(new QLabel("Brush radius"));
	brush_layout->addWidget(brush_radius);
	brush_param->setLayout(brush_layout);

	// Execute
	vboxexecute->layout()->setAlignment(Qt::AlignTop);
	pushstart = new QPushButton("Start Slice", vboxexecute);
	pushstart->setToolTip(Format(
			"Interpolation/extrapolation is based on 2 slices. Click start to "
			"select the first slice and Execute to select the second slice. Interpolation "
			"automatically interpolates the intermediate slices."
			"<br>"
			"Note:<br>The result is displayed in the Target but is not directly "
			"added to the tissue distribution. "
			"The user can add it with Adder function. The 'All Tissues' option "
			"adds the result directly to the tissue."));
	pushexec = new QPushButton("Execute", vboxexecute);
	pushexec->setEnabled(false);

	vboxmethods->setMargin(5);
	vboxdataselect->setMargin(5);
	vboxparams->setMargin(5);
	vboxexecute->setMargin(5);
	vboxmethods->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
	vboxmethods->setLineWidth(1);

	vboxmethods->setFixedSize(vboxparams->sizeHint());
	vboxdataselect->setFixedSize(vboxparams->sizeHint());
	vboxparams->setFixedSize(vboxparams->sizeHint());
	vboxexecute->setFixedSize(vboxparams->sizeHint());

	QObject::connect(brush_radius, SIGNAL(textEdited(const QString &)), this, SLOT(brush_changed()));
	QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this, SLOT(brush_changed()));

	QObject::connect(pushstart, SIGNAL(clicked()), this, SLOT(startslice_pressed()));
	QObject::connect(cb_medianset, SIGNAL(stateChanged(int)), this, SLOT(method_changed()));
	QObject::connect(cb_connectedshapebased, SIGNAL(stateChanged(int)), this, SLOT(method_changed()));
	QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this, SLOT(method_changed()));
	QObject::connect(sourcegroup, SIGNAL(buttonClicked(int)), this, SLOT(source_changed()));

	QObject::connect(pushexec, SIGNAL(clicked()), this, SLOT(execute()));

	method_changed();
	source_changed();
}

InterpolationWidget::~InterpolationWidget()
{
	delete sourcegroup;
	delete modegroup;
	delete hboxoverall;
}

QSize InterpolationWidget::sizeHint() const { return hboxoverall->sizeHint(); }

void InterpolationWidget::init()
{
	if (handler3D->num_slices() != nrslices)
	{
		nrslices = handler3D->num_slices();
		sb_slicenr->setMaxValue((int)nrslices);
		sb_batchstride->setMaxValue((int)nrslices - 1);
		pushexec->setEnabled(false);
	}

	if (!brush)
	{
		brush = new BrushInteraction(handler3D,
				[this](iseg::DataSelection sel) { begin_datachange(sel, this); },
				[this](iseg::EndUndoAction a) { end_datachange(this, a); },
				[this](std::vector<Point>* vp) { vpdyn_changed(vp);} );
		brush_changed();
	}
	else
	{
		brush->init(handler3D);
	}
}

void InterpolationWidget::newloaded() {}

void InterpolationWidget::on_slicenr_changed() {}

void InterpolationWidget::on_tissuenr_changed(int tissuetype)
{
	tissuenr = (tissues_size_t)tissuetype + 1;
}

void InterpolationWidget::handler3D_changed()
{
	if (handler3D->num_slices() != nrslices)
	{
		nrslices = handler3D->num_slices();
		sb_slicenr->setMaxValue((int)nrslices);
		sb_batchstride->setMaxValue((int)nrslices - 1);
	}
	pushexec->setEnabled(false);
}

void InterpolationWidget::startslice_pressed()
{
	startnr = handler3D->active_slice();
	pushexec->setEnabled(true);
}

void InterpolationWidget::on_mouse_clicked(Point p)
{
	if (rb_work->isOn() || rb_tissue->isOn())
	{
		brush->set_tissue_value(tissuenr);
		brush->on_mouse_clicked(p);
	}
	else
	{
		WidgetInterface::on_mouse_clicked(p);
	}
}

void InterpolationWidget::on_mouse_released(Point p)
{
	if (rb_work->isOn() || rb_tissue->isOn())
	{
		brush->on_mouse_released(p);
	}
	else
	{
		WidgetInterface::on_mouse_released(p);
	}
}

void InterpolationWidget::on_mouse_moved(Point p)
{
	if (rb_work->isOn() || rb_tissue->isOn())
	{
		brush->on_mouse_moved(p);
	}
	else
	{
		WidgetInterface::on_mouse_moved(p);
	}
}

void InterpolationWidget::execute()
{
	unsigned short batchstride = (unsigned short)sb_batchstride->value();
	bool const connected = cb_connectedshapebased->isVisible() && cb_connectedshapebased->isChecked();

	unsigned short current = handler3D->active_slice();
	if (current != startnr)
	{
		iseg::DataSelection dataSelection;
		if (rb_extra->isOn())
		{
			dataSelection.sliceNr = (unsigned short)sb_slicenr->value() - 1;
			dataSelection.work = true;
			emit begin_datachange(dataSelection, this);
			if (rb_work->isOn())
			{
				handler3D->extrapolatework(startnr, current, (unsigned short)sb_slicenr->value() - 1);
			}
			else
			{
				handler3D->extrapolatetissue(startnr, current, (unsigned short)sb_slicenr->value() - 1, tissuenr);
			}
			emit end_datachange(this);
		}
		else if (rb_inter->isOn())
		{
			dataSelection.allSlices = true;

			if (rb_work->isOn())
			{
				dataSelection.work = true;
				emit begin_datachange(dataSelection, this);
				if (cb_medianset->isChecked())
				{
					handler3D->interpolateworkgrey_medianset(startnr, current, rb_8connectivity->isOn());
				}
				else
				{
					handler3D->interpolateworkgrey(startnr, current, connected);
				}
			}
			else if (rb_tissue->isOn())
			{
				dataSelection.work = true;
				emit begin_datachange(dataSelection, this);
				if (cb_medianset->isChecked())
				{
					handler3D->interpolatetissue_medianset(startnr, current, tissuenr, rb_8connectivity->isOn());
				}
				else
				{
					handler3D->interpolatetissue(startnr, current, tissuenr, connected);
				}
			}
			else if (rb_tissueall->isOn())
			{
				dataSelection.tissues = true;
				emit begin_datachange(dataSelection, this);
				if (cb_medianset->isChecked())
				{
					handler3D->interpolatetissuegrey_medianset(startnr, current, rb_8connectivity->isOn());
				}
				else
				{
					handler3D->interpolatetissuegrey(startnr, current);
				}
			}
			emit end_datachange(this);
		}
		else if (rb_batchinter->isOn())
		{
			dataSelection.allSlices = true;

			if (rb_work->isOn())
			{
				dataSelection.work = true;
				emit begin_datachange(dataSelection, this);

				if (cb_medianset->isChecked())
				{
					unsigned short batchstart;
					for (batchstart = startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						handler3D->interpolateworkgrey_medianset(batchstart, batchstart + batchstride, rb_8connectivity->isOn());
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						handler3D->interpolateworkgrey_medianset(batchstart - batchstride, current, rb_8connectivity->isOn());
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						handler3D->interpolateworkgrey(batchstart, batchstart + batchstride, connected);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						handler3D->interpolateworkgrey(batchstart - batchstride, current, connected);
					}
				}
			}
			else if (rb_tissue->isOn())
			{
				dataSelection.work = true;
				emit begin_datachange(dataSelection, this);

				if (cb_medianset->isChecked())
				{
					unsigned short batchstart;
					for (batchstart = startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						handler3D->interpolatetissue_medianset(batchstart, batchstart + batchstride, tissuenr, rb_8connectivity->isOn());
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						handler3D->interpolatetissue_medianset(batchstart - batchstride, current, tissuenr, rb_8connectivity->isOn());
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						handler3D->interpolatetissue(batchstart, batchstart + batchstride, tissuenr, connected);
					}
					// Last batch with smaller stride
					if (batchstart > current && current - (batchstart - batchstride) >= 2)
					{
						handler3D->interpolatetissue(batchstart - batchstride, current, tissuenr, connected);
					}
				}
			}
			else if (rb_tissueall->isOn())
			{
				dataSelection.tissues = true;
				emit begin_datachange(dataSelection, this);

				if (cb_medianset->isChecked())
				{
					unsigned short batchstart;
					for (batchstart = startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						handler3D->interpolatetissuegrey_medianset(batchstart, batchstart + batchstride,
								rb_8connectivity->isOn());
					}
					// Last batch with smaller stride
					if (batchstart > current &&
							current - (batchstart - batchstride) >= 2)
					{
						handler3D->interpolatetissuegrey_medianset(batchstart - batchstride, current, rb_8connectivity->isOn());
					}
				}
				else
				{
					unsigned short batchstart;
					for (batchstart = startnr;
							 batchstart <= current - batchstride;
							 batchstart += batchstride)
					{
						handler3D->interpolatetissuegrey(batchstart, batchstart + batchstride);
					}
					// Last batch with smaller stride
					if (batchstart > current &&
							current - (batchstart - batchstride) >= 2)
					{
						handler3D->interpolatetissuegrey(batchstart - batchstride, current);
					}
				}
			}
			emit end_datachange(this);
		}
		pushexec->setEnabled(false);
	}
}

void InterpolationWidget::brush_changed()
{
	if (brush)
	{
		brush->set_brush_target(rb_work->isOn());
		brush->set_radius(brush_radius->text().toDouble());
	}
}

void InterpolationWidget::method_changed()
{
	if (rb_extra->isOn())
	{
		hboxextra->show();
		hboxbatch->hide();
		cb_medianset->hide();
		cb_connectedshapebased->hide();
		rb_4connectivity->hide();
		rb_8connectivity->hide();
		if (rb_tissueall->isChecked())
		{
			rb_tissue->setChecked(true);
		}
		rb_tissueall->setEnabled(false);
	}
	else if (rb_inter->isOn())
	{
		hboxextra->hide();
		hboxbatch->hide();
		rb_tissueall->setEnabled(true);
		cb_medianset->show();
		if (cb_medianset->isChecked())
		{
			cb_connectedshapebased->hide();
			rb_4connectivity->show();
			rb_8connectivity->show();
		}
		else
		{
			cb_connectedshapebased->setVisible(!rb_tissueall->isOn());
			rb_4connectivity->hide();
			rb_8connectivity->hide();
		}
	}
	else if (rb_batchinter->isOn())
	{
		hboxextra->hide();
		hboxbatch->show();
		rb_tissueall->setEnabled(true);
		cb_medianset->show();
		if (cb_medianset->isChecked())
		{
			rb_4connectivity->show();
			rb_8connectivity->show();
		}
		else
		{
			rb_4connectivity->hide();
			rb_8connectivity->hide();
		}
	}
}

void InterpolationWidget::source_changed()
{
	method_changed();

	if (brush)
	{
		brush->set_brush_target(rb_work->isOn());
	}
}

FILE* InterpolationWidget::SaveParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		int dummy;
		dummy = sb_slicenr->value();
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_tissue->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_work->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_inter->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		dummy = (int)(rb_extra->isOn());
		fwrite(&(dummy), 1, sizeof(int), fp);
		if (version >= 11)
		{
			dummy = (int)(rb_tissueall->isOn());
			fwrite(&(dummy), 1, sizeof(int), fp);
			dummy = (int)(rb_batchinter->isOn());
			fwrite(&(dummy), 1, sizeof(int), fp);
			if (version >= 12)
			{
				dummy = (int)(cb_medianset->isChecked());
				fwrite(&(dummy), 1, sizeof(int), fp);
				dummy = (int)(rb_4connectivity->isOn());
				fwrite(&(dummy), 1, sizeof(int), fp);
				dummy = (int)(rb_8connectivity->isOn());
				fwrite(&(dummy), 1, sizeof(int), fp);
			}
		}
	}

	return fp;
}

FILE* InterpolationWidget::LoadParams(FILE* fp, int version)
{
	if (version >= 2)
	{
		QObject::disconnect(modegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(method_changed()));
		QObject::disconnect(sourcegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(source_changed()));

		int dummy;
		fread(&dummy, sizeof(int), 1, fp);
		sb_slicenr->setValue(dummy);
		fread(&dummy, sizeof(int), 1, fp);
		rb_tissue->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_work->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_inter->setChecked(dummy > 0);
		fread(&dummy, sizeof(int), 1, fp);
		rb_extra->setChecked(dummy > 0);
		if (version >= 11)
		{
			fread(&dummy, sizeof(int), 1, fp);
			rb_tissueall->setChecked(dummy > 0);
			fread(&dummy, sizeof(int), 1, fp);
			rb_batchinter->setChecked(dummy > 0);
			if (version >= 12)
			{
				fread(&dummy, sizeof(int), 1, fp);
				cb_medianset->setChecked(dummy > 0);
				fread(&dummy, sizeof(int), 1, fp);
				rb_4connectivity->setChecked(dummy > 0);
				fread(&dummy, sizeof(int), 1, fp);
				rb_8connectivity->setChecked(dummy > 0);
			}
		}

		method_changed();
		source_changed();

		QObject::connect(modegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(method_changed()));
		QObject::connect(sourcegroup, SIGNAL(buttonClicked(int)), this,
				SLOT(source_changed()));
	}
	return fp;
}
